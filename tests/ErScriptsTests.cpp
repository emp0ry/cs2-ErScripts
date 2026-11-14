#include "features/AutoStop.h"
#include "core/Config.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "platform/Input.h"
#include "game/GameConsole.h"
#include "game/GameConsole.h"
#include "graphics/ImagesShellCodes.h"
#include <cstdlib>
#include <algorithm>
#include "core/ConfigExport.h"
#include "graphics/CrosshairDrawing.h"
#include "graphics/D3DResize.h"
#include "features/Features.h"
#include "platform/FileMonitor.h"
#include "game/GSIServer.h"
#include "graphics/Overlay.h"
#include "platform/SimpleSound.h"
#include "graphics/SniperDynamics.h"
#include "update/Updater.h"
#include "TestSupport.h"
#include <atomic>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <mmsystem.h>
#include <barrier>

namespace fs = std::filesystem;
using namespace std::chrono_literals;
using namespace std::string_view_literals;

struct ConfigExportTestAccess {
    static void join(ConfigExport &exporter) {
        std::lock_guard owner(exporter.workerMutex);
        if (exporter.worker.joinable())
            exporter.worker.join();
    }
    static void capture(ConfigExport &exporter) {
        std::lock_guard lock(exporter.mutex);
        exporter.state.queued = false;
        exporter.state.capturing = true;
        exporter.token = "test_export";
    }
    static void expire(ConfigExport &exporter) {
        std::lock_guard lock(exporter.mutex);
        exporter.deadline = std::chrono::steady_clock::now() - 1s;
    }
    static auto deadline(ConfigExport &exporter) {
        std::lock_guard lock(exporter.mutex);
        return exporter.deadline;
    }
};

struct GSIServerTestAccess {
    static void useTestPort(GSIServer &server, int port = 0) { server.port = port; }
    static int port(const GSIServer &server) { return server.port; }
};

struct SteamToolsTestAccess {
    static auto libraries(std::wstring_view text) { return SteamTools::parseVdfContent(text); }
};

struct UpdaterTestAccess {
    static bool newer(std::string_view candidate, std::string_view current) {
        return Updater::newerVersion(candidate, current);
    }
};

struct SimpleSoundTestAccess {
    static const std::wstring &file(const SimpleSound &sound) { return sound.tempDefaultFile_; }
    static size_t requests(const SimpleSound &sound) {
        return static_cast<size_t>(std::distance(sound.playRequests_.begin(), sound.playRequests_.end()));
    }
};

void delayTests() {
    std::mt19937 rng(42);
    std::vector<int> recent;
    for (int i = 0; i < 1000; ++i) {
        const auto before = recent;
        const auto value = autostop::randomDelay(recent, 5, 30, rng);
        check(value >= 5 && value <= 30 && recent.size() <= 10 &&
                  std::find(before.begin(), before.end(), value) == before.end(),
              "Delay repeats within history");
    }
    recent = {1, 2, 3};
    for (int i = 0; i < 100; ++i)
        check(autostop::randomDelay(recent, 1, 3, rng) >= 1 && recent.size() <= 3, "Exhausted range failed");
    check(autostop::randomDelay(recent, 7, 7, rng) == 7, "Single-value range failed");
    autostop::randomDelay(recent, INT_MIN, INT_MAX, rng);
}

void inputLifetimeTests() {
    using Event = std::pair<int, bool>;
    static std::array<Event, 8> events;
    static size_t count = 0;
    count = 0;
    const auto send = +[](int key, bool down) noexcept {
        if (count == events.size())
            return false;
        events[count++] = {key, down};
        return true;
    };
    bool active = false;
    try {
        input::KeyPress alt(VK_MENU, send);
        input::KeyPress tab(VK_TAB, send);
        active = alt.active() && tab.active();
        throw std::runtime_error("Stop during a key chord");
    } catch (const std::runtime_error &) {
    }
    check(active, "Test input was not accepted");
    check(count == 4 && events[0] == Event{VK_MENU, true} && events[1] == Event{VK_TAB, true} &&
              events[2] == Event{VK_TAB, false} && events[3] == Event{VK_MENU, false},
          "Exception did not release synthesized keys in reverse order");
    count = 0;
    const auto denied = +[](int key, bool down) noexcept {
        events[count++] = {key, down};
        return false;
    };
    {
        input::KeyPress key('W', denied);
        check(!key.active(), "Rejected input was reported as delivered");
    }
    check(count == 1 && events[0] == Event{'W', true}, "Failed press produced an unrelated key release");
}

void configTests() {
    fs::create_directory("configs");
    Config config;
    config.triggerState = true;
    config.killSayText = "kept";
    const auto initial = config;
    cs2config::write("configs/bad.json", "{bad");
    config.load("bad");
    check(config == initial, "Malformed JSON changed settings");
    cs2config::write("configs/bad.json", R"({"keystrokes":false})");
    config.load("bad");
    check(config == initial, "Invalid section changed settings");
    config.load("missing");
    check(config == initial, "Missing file changed settings");
    cs2config::write("configs/huge.json", std::string(4 * 1024 * 1024 + 1, ' '));
    config.load("huge");
    check(config == initial, "Oversized configuration changed settings");

    config.bombTimerPos[0] = 12;
    config.bombTimerPos[1] = 34;
    config.triggerDisplacement[0] = 3;
    config.triggerDisplacement[1] = 4;
    cs2config::write("configs/fields.json", R"({
        "bomb-timer":{"pos":[99,"bad"]},
        "pixel-trigger":{"bind":1e30,"displacement":[9,"bad"],"threshold":2.5,"delay":-50},
        "gradient-manager":{"end-hue]":0.25},
        "fps-limiter":{"fps":0}
    })");
    config.load("fields");
    check(config.bombTimerPos[0] == 12 && config.bombTimerPos[1] == 34, "Partial float array applied");
    check(config.triggerDisplacement[0] == 3 && config.triggerDisplacement[1] == 4, "Partial int array applied");
    check(config.triggerBind == 0 && config.triggerThreshold == 20, "Invalid integer converted");
    check(config.triggerDelayMs == 0 && config.fpsLimiter == 1, "Timing bounds not enforced");
    check(config.gradient.endHue == .25f, "Legacy gradient key rejected");
    check(!config.triggerState, "Risk consent reset changed");
    cs2config::write("configs/bounds.json", R"({"pixel-trigger":{"displacement":[2147483647,-2147483648]}})");
    config.load("bounds");
    check(config.triggerDisplacement[0] == 20 && config.triggerDisplacement[1] == 0,
          "Pixel offsets can overflow screen coordinates");

    config.killSayText = "quotes \" and UTF-8: \xD0\xBF\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82";
    config.keystrokesPos[0] = 45;
    config.killSoundFileName = "custom.wav";
    check(config.save("roundtrip"), "Saving config failed");
    Config loaded;
    loaded.load("roundtrip");
    check(config == loaded, "Configuration round trip changed values");
    fs::create_directory("configs/blocked.json");
    check(!config.save("blocked"), "Failed save reported success");
    check(fs::is_directory("configs/blocked.json") && !fs::exists("configs/blocked.json.tmp"),
          "Failed save damaged destination or left a partial file");

    Config published;
    published.triggerThreshold = published.triggerDelayMs = 0;
    published.publish();
    const auto previous = Config::snapshot();
    std::atomic_bool inconsistent = false;
    std::jthread reader([&](std::stop_token stop) {
        while (!stop.stop_requested()) {
            const auto current = Config::snapshot();
            if (current->triggerThreshold != current->triggerDelayMs)
                inconsistent = true;
        }
    });
    for (int i = 1; i <= 100; ++i) {
        published.triggerThreshold = published.triggerDelayMs = i;
        published.killSayText = std::to_string(i);
        published.publish();
    }
    reader.request_stop();
    reader.join();
    check(!inconsistent && previous->triggerThreshold == 0 && Config::snapshot()->killSayText == "100",
          "Published configuration was mutable or inconsistent");
}

void parsingTests() {
    cs2config::Snapshot snapshot;
    snapshot.vcfg(R"("root" { "convars" { "sensitivity" "1.25" } "bindings" { "MOUSE1" "+attack" } })");
    check(snapshot.convars.at("sensitivity") == "1.25", "Saved convar missing");
    const auto before = snapshot.convars;
    bool rejected = false;
    try {
        snapshot.vcfg(R"("root" { "convars" { "sensitivity" "9" })");
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    check(rejected && snapshot.convars == before, "Malformed VCFG partially changed the snapshot");
    check(snapshot.convarLine(R"([Console] sensitivity = "2.5")"), "Live value rejected");
    check(snapshot.convarLine("new_variable 4"), "Session value rejected");
    check(snapshot.convars.at("sensitivity") == "2.5" && !snapshot.convars.contains("new_variable"),
          "Live capture changed the saved-variable set");
    check(!snapshot.convarLine("sensitivity nan"), "Nonfinite console number accepted");
    check(snapshot.bindLine(R"(bind "F1" "say \"hello\"")"), "Escaped binding rejected");
    check(snapshot.aliasLine(R"(alias "hello": "say hello; echo done")"), "Alias rejected");
    check(snapshot.binds.at("F1") == "say \"hello\"", "Binding escape was lost");
    check(snapshot.cfg().find("unbindall") == std::string::npos, "Export unbinds unrelated keys");
    check(cs2config::quote("a\nb\r\0c"sv) == "\"abc\"", "Command line escaping failed");

    const auto size = video::saved(R"("video.cfg" { "setting.defaultres" "1920" "setting.defaultresheight" "1080" })");
    check(size && *size == video::Size{1920, 1080}, "Video settings not parsed");
    check(!video::saved(R"("video.cfg" { "setting.defaultres" "999999" "setting.defaultresheight" "1080" })"),
          "Invalid video dimensions accepted");
    video::ModeChanges changes;
    changes.consume(L"[RenderSystem] Swap Chain Mode Changing:");
    changes.consume(L"m_nWidth: 1920 -> 1280");
    const auto changed = changes.consume(L"m_nHeight: 1080 -> 960");
    check(changed && *changed == video::Size{1280, 960}, "Console video transition not parsed");

    const std::wstring library = LR"("libraryfolders"
{
    "10"
    {
        "path" "D:\\SteamLibrary"
        "apps"
        {
            "730" "123"
        }
    }
})";
    const auto trailing = library + L"\n\"11\"\n{\n\"path\" bad\n}";
    const auto libraries = SteamToolsTestAccess::libraries(std::wstring_view(trailing).substr(0, library.size()));
    check(libraries.contains("730") && libraries.at("730").front() == L"D:\\SteamLibrary\\steamapps\\common",
          "Multi-digit Steam library or bounded string view failed");
    check(UpdaterTestAccess::newer("v2.10.0", "2.9.9"), "Versions compared lexicographically");
    check(!UpdaterTestAccess::newer("2.0.0", "2.0.0.0") && !UpdaterTestAccess::newer("invalid", "2.0.0"),
          "Equal or invalid version accepted as newer");
}

nlohmann::json payload(int clip = 5, int timestamp = 100) {
    return {{"provider", {{"steamid", "local"}, {"appid", 730}, {"timestamp", timestamp}}},
            {"map", {{"name", "test"}, {"round", 1}}},
            {"round", {{"phase", "live"}}},
            {"player",
             {{"steamid", "local"},
              {"activity", "playing"},
              {"team", "CT"},
              {"state", {{"health", 100}, {"defusekit", true}}},
              {"match_stats", {{"kills", 7}}},
              {"weapons",
               {{"weapon_0",
                 {{"state", "active"},
                  {"type", "SniperRifle"},
                  {"name", "weapon_awp"},
                  {"ammo_clip", clip},
                  {"ammo_reserve", 30},
                  {"ammo_clip_max", 5}}}}}}}};
}

void playerTests() {
    const auto time = gsi::Clock::time_point{} + 100s;
    gsi::Tracker tracker;
    check(tracker.update(payload(), time), "Initial payload rejected");
    const auto session = tracker.snapshot(time).weaponSession;
    check(tracker.snapshot(time).sniper(), "Sniper not identified");
    tracker.update(payload(4, 101), time + 100ms);
    check(tracker.snapshot(time + 100ms).confirmedShots == 1, "Shot was not counted");
    check(!tracker.update(payload(3, 100), time + 200ms), "Late payload accepted");
    check(tracker.snapshot(time + 200ms).clip == 4, "Late payload changed ammo");
    auto reload = payload(2, 102);
    reload["player"]["weapons"]["weapon_0"]["state"] = "reloading";
    tracker.update(reload, time + 300ms);
    check(tracker.snapshot(time + 300ms).confirmedShots == 1, "Reload counted as a shot");
    tracker.update(payload(5, 103), time + 400ms);
    check(tracker.snapshot(time + 400ms).weaponSession == session, "Reload changed weapon session");
    check(!tracker.snapshot(time + 26s).playing, "Stale player remained active");
    check(tracker.update(payload(5, 1), time + 26s), "Provider clock reset rejected after disconnect");
    check(tracker.snapshot(time + 26s).weaponSession != session, "Reconnect reused weapon session");
    auto spectator = payload(4, 2);
    spectator["player"]["steamid"] = "spectated";
    tracker.update(spectator, time + 27s);
    check(!tracker.snapshot(time + 27s).playing, "Spectated player treated as local");
    check(!tracker.update(nlohmann::json::array({1, 2}), time + 28s), "Malformed payload accepted");
    tracker.update(payload(5, 3), time + 28s);
    auto invalidProvider = payload(4, 4);
    invalidProvider["provider"]["appid"] = 123;
    check(!tracker.update(invalidProvider, time + 29s) && tracker.snapshot(time + 29s).clip == 5,
          "Foreign game payload corrupted active state");
    invalidProvider = payload(4, 4);
    invalidProvider["provider"]["timestamp"] = -1;
    check(!tracker.update(invalidProvider, time + 29s), "Negative provider time accepted");
    auto switched = payload(10, 4);
    switched["player"]["weapons"]["weapon_0"]["name"] = "weapon_ssg08";
    switched["player"]["weapons"]["weapon_0"]["ammo_clip_max"] = 10;
    const auto beforeSwitch = tracker.snapshot(time + 29s).weaponSession;
    tracker.update(switched, time + 29s);
    check(tracker.snapshot(time + 29s).weaponSession != beforeSwitch &&
              tracker.snapshot(time + 29s).confirmedShots == 0,
          "Weapon switch retained shot state");

    crosshair::Settings settings;
    settings.modernFields = 15;
    crosshair::SniperDynamics dynamics;
    gsi::Tracker shots;
    shots.update(payload(), time);
    dynamics.update(shots.snapshot(time), time);
    const auto idle = dynamics.frame(settings, 1080, time).spread;
    shots.update(payload(4, 101), time + 100ms);
    dynamics.update(shots.snapshot(time + 100ms), time + 100ms);
    const auto fired = dynamics.frame(settings, 1080, time + 100ms);
    check(fired.spread > idle && fired.shotFeedback > 0, "Sniper shot feedback missing");
    check(std::abs(dynamics.frame(settings, 1080, time + 10s).spread - idle) < .01f, "Sniper recovery failed");
    check(dynamics.frame(settings, 1080, time + 30s).spread == 0, "Stale sniper dynamics remained active");
}

void gameStateTests() {
    const auto now = gsi::Clock::time_point{} + 100s;
    gsi::Tracker tracker;
    auto game = payload();
    game["round"]["bomb"] = "planted";
    game["player"]["weapons"]["weapon_0"]["type"] = "Pistol";
    game["player"]["weapons"]["weapon_0"]["name"] = "weapon_cz75a";
    check(tracker.update(game, now), "Game state rejected");
    const auto initial = tracker.gameSnapshot(now);
    check(initial.connected && initial.inGame && initial.alive && initial.bombPlanted && initial.defuseKit &&
              initial.roundLive && initial.pistol && initial.cz75 && !initial.revolver && initial.kills == 7,
          "Packet fields were not published together");
    check(tracker.gameSnapshot(now + gsi::staleAfter + 1ms) == gsi::GameState{},
          "Stale weapon, bomb or player state remained active");
    auto invalid = game;
    invalid["provider"]["appid"] = 999;
    check(!tracker.update(invalid, now + 1s) && tracker.gameSnapshot(now + 1s) == initial,
          "Invalid provider changed game state");
    auto older = game;
    older["provider"]["timestamp"] = 99;
    older["round"]["bomb"] = "defused";
    check(!tracker.update(older, now + 1s) && tracker.gameSnapshot(now + 1s) == initial,
          "Out-of-order packet changed non-sniper state");
    game["provider"]["timestamp"] = 101;
    game["player"]["match_stats"]["kills"] = "invalid";
    tracker.update(game, now + 1s);
    check(tracker.gameSnapshot(now + 1s).kills == -1, "Invalid kill count became a zero-count history");
    game["provider"]["timestamp"] = 1;
    tracker.update(game, now + 30s);
    check(tracker.gameSnapshot(now + 30s).session != initial.session, "Reconnect retained old event history");
    const auto reconnected = tracker.gameSnapshot(now + 30s).session;
    game["map"]["name"] = "other-map";
    game["provider"]["timestamp"] = 2;
    tracker.update(game, now + 31s);
    check(tracker.gameSnapshot(now + 31s).session != reconnected, "Map transition retained old event history");
    game["player"]["weapons"]["weapon_1"] = game["player"]["weapons"]["weapon_0"];
    tracker.update(game, now + 32s);
    check(!tracker.gameSnapshot(now + 32s).pistol && !tracker.snapshot(now + 32s).sniper(),
          "Ambiguous active weapons enabled automation");
    game["player"]["weapons"].erase("weapon_1");
    game["player"]["state"]["health"] = 0;
    tracker.update(game, now + 33s);
    check(tracker.gameSnapshot(now + 33s).inGame && !tracker.gameSnapshot(now + 33s).alive,
          "Dead local player was treated as alive");
}

void geometryTests() {
    crosshair::Settings settings;
    settings.modernFields = 15;
    settings.style = 4;
    const auto dimensions = crosshair::dimensions(settings, 1080);
    check(dimensions.length == 8 && dimensions.thickness == 2 && dimensions.gap == 4, "Pixel dimensions changed");
    const auto scene = crosshair::build(settings, 1080);
    check(scene.size() == 4 && scene.front().geometry == crosshair::Vec4{-12, -1, -5, 0},
          "Static cross pixel alignment changed");
    const auto spans = crosshair::rasterize(scene, {-100, -100, 100, 100});
    int pixels = 0;
    for (const auto &span : spans)
        pixels += span.x1 - span.x0;
    check(pixels == 160, "Crosshair outline coverage changed");
    settings.tStyle = true;
    check(crosshair::build(settings, 1080).size() == 3, "T style has a top bar");
    settings.tStyle = false;
    const size_t counts[] = {4, 1, 4, 1, 4, 4, 1, 4, 4, 1};
    for (int style = 0; style < 10; ++style) {
        settings.style = style;
        const auto primitives = crosshair::build(settings, 1080);
        check(primitives.size() == counts[style], "Crosshair style primitive count changed");
        for (const auto &span : crosshair::rasterize(primitives, {-100, -100, 100, 100}))
            check(span.x0 >= -100 && span.x1 <= 100 && span.y >= -100 && span.y < 100, "Raster escaped clip");
    }
    settings.style = 6;
    settings.alpha = 128;
    settings.outlineAlpha = 128;
    const auto sample = crosshair::sample(crosshair::build(settings, 1080), 0, 0);
    const float alpha = 128.f / 255.f;
    check(std::abs(sample.w - (alpha + alpha * (1 - alpha) * (1 - alpha))) < .0001f, "Alpha compositing changed");
    check(crosshair::number("nan") == std::nullopt && crosshair::number("1.2junk") == std::nullopt,
          "Invalid crosshair number accepted");
    for (int i = 0; i <= 255; ++i) {
        const float channel = static_cast<float>(i) / 255.f;
        check(std::abs(crosshair::srgb(crosshair::linear(channel)) - channel) < .00001f,
              "sRGB round trip changed channel values");
    }
    settings.modernFields = 15;
    settings.size = settings.thickness = 0;
    settings.gap = -3840;
    for (int height : {1, 240, 1080, 4320}) {
        const auto zero = crosshair::dimensions(settings, height);
        check(zero.length == 0 && zero.thickness == 0 && zero.gap < 0, "Zero-size or negative-gap geometry changed");
        for (const auto &span : crosshair::rasterize(crosshair::build(settings, height), {-16, -16, 16, 16}))
            check(span.x0 >= -16 && span.x1 <= 16 && span.y >= -16 && span.y < 16, "Boundary geometry escaped clip");
    }
}

void exportTests() {
    GameConsole console;
    fs::create_directory("source");
    const auto source = fs::absolute("source");
    const std::string convars = R"("root" { "convars" { "sensitivity" "1.25" "cl_crosshairstyle" "4" } })";
    cs2config::write(source / "cs2_user_convars_0_slot0.vcfg", convars);
    cs2config::write(source / "cs2_user_keys_0_slot0.vcfg", R"("root" { "bindings" { "MOUSE1" "+attack" } })");
    ConfigExport exporter;
    exporter.start(false, source);
    ConfigExportTestAccess::join(exporter);
    auto state = exporter.status();
    check(state.ready && !state.busy && state.convars == 2 && state.binds == 1, "Saved export failed");
    check(cs2config::read(state.folder / "source/cs2_user_convars_0_slot0.vcfg") == convars, "Source copy changed");
    check(cs2config::read(source / "cs2_user_convars_0_slot0.vcfg") == convars, "Source settings changed");
    check(cs2config::read(state.folder / "erscripts-export.cfg").find("bind \"MOUSE1\" \"+attack\"") !=
              std::string::npos,
          "Export binding missing");

    const auto startLive = [&] {
        std::this_thread::sleep_for(2ms);
        exporter.start(true, source);
        ConfigExportTestAccess::join(exporter);
        check(exporter.status().queued, "Live export was not queued");
        ConfigExportTestAccess::capture(exporter);
    };
    startLive();
    exporter.consume(L"test_export_vars");
    exporter.consume(L"sensitivity 2.5");
    exporter.consume(L"test_export_binds");
    exporter.consume(L"bind F1 \"say hello\"");
    exporter.consume(L"test_export_aliases");
    exporter.consume(L"alias hello: \"echo hi\"");
    exporter.consume(L"test_export_end");
    state = exporter.status();
    check(state.ready && state.aliases == 1 && state.binds == 2, "Live export failed");
    check(cs2config::read(state.folder / "erscripts-export.cfg").find("sensitivity \"2.5\"") != std::string::npos,
          "Live export did not override a saved value");
    startLive();
    exporter.consume(L"test_export_end");
    check(!exporter.status().ready && !exporter.status().busy, "Incomplete capture succeeded");
    startLive();
    exporter.cancel();
    exporter.consume(L"test_export_end");
    check(!exporter.status().ready && !exporter.status().busy, "Cancelled export revived");
    startLive();
    ConfigExportTestAccess::expire(exporter);
    exporter.flush(console);
    check(!exporter.status().busy && !exporter.status().ready, "Export timeout failed");
    std::this_thread::sleep_for(2ms);
    exporter.start(true, source);
    ConfigExportTestAccess::join(exporter);
    const auto deadline = ConfigExportTestAccess::deadline(exporter);
    globals::menuState = false;
    exporter.flush(console);
    check(exporter.status().queued && ConfigExportTestAccess::deadline(exporter) == deadline,
          "Failed command extended the capture deadline");
    exporter.cancel();
    std::this_thread::sleep_for(2ms);
    exporter.start(false, source);
    exporter.cancel();
    ConfigExportTestAccess::join(exporter);
    check(!exporter.status().busy && !exporter.status().ready, "Preparation cancellation failed");
    std::barrier start(12);
    std::vector<std::jthread> callers;
    for (int i = 0; i < 12; ++i)
        callers.emplace_back([&] {
            start.arrive_and_wait();
            exporter.start(true, source);
        });
    callers.clear();
    ConfigExportTestAccess::join(exporter);
    const auto first = exporter.status();
    check(first.busy && first.queued && !first.ready, "Concurrent starts corrupted export state");
    exporter.cancel();
    exporter.start(true, source);
    ConfigExportTestAccess::join(exporter);
    check(exporter.status().folder != first.folder && exporter.status().queued,
          "New session reused export destination");
    exporter.consume(L"obsolete_export_end");
    check(exporter.status().queued, "Obsolete capture changed a new session");
    exporter.cancel();
    callers.clear();
    for (int i = 0; i < 4; ++i)
        callers.emplace_back([&] {
            for (int n = 0; n < 8; ++n) {
                exporter.start(true, source);
                exporter.cancel();
            }
        });
    callers.clear();
    exporter.cancel();
    ConfigExportTestAccess::join(exporter);
    check(!exporter.status().busy && !exporter.status().ready, "Concurrent cancellation revived a session");
}

void monitorTests() {
    const auto file = fs::absolute("console.log");
    cs2config::write(file, "old\n");
    FileMonitor monitor(file.wstring());
    monitor.start();
    const auto append = [&](std::string_view text) {
        std::ofstream stream(file, std::ios::binary | std::ios::app);
        stream << text;
    };
    append("partial");
    std::this_thread::sleep_for(100ms);
    check(!monitor.getNewLines(), "Incomplete log line emitted");
    append("\r\n\xD0");
    std::this_thread::sleep_for(100ms);
    append("\xAF\n");
    std::deque<std::wstring> received;
    until([&] {
        if (auto lines = monitor.getNewLines())
            received.insert(received.end(), lines->begin(), lines->end());
        return received.size() == 2;
    });
    check(received[0] == L"partial" && received[1] == L"\u042F", "Fragmented UTF-8 or CRLF parsing failed");
    cs2config::write(file, "new\n");
    until([&] {
        auto lines = monitor.getNewLines();
        return lines && lines->size() == 1 && lines->front() == L"new";
    });
    monitor.stop();
    append("restart\n");
    monitor.start();
    until([&] {
        auto lines = monitor.getNewLines();
        return lines && lines->front() == L"restart";
    });
    monitor.stop();
    append("saved partial");
    monitor.start();
    std::this_thread::sleep_for(100ms);
    monitor.stop();
    append(" tail\n");
    monitor.start();
    until([&] {
        auto lines = monitor.getNewLines();
        return lines && lines->front() == L"saved partial tail";
    });
    fs::rename(file, "rotated.log");
    cs2config::write(file, "replacement starts at zero despite being larger than the old offset\n");
    until([&] {
        auto lines = monitor.getNewLines();
        return lines && lines->front().starts_with(L"replacement starts at zero");
    });
    append(std::string(1024 * 1024 + 1, 'x') + "\nafter large line\n");
    until([&] {
        auto lines = monitor.getNewLines();
        if (!lines)
            return false;
        check(lines->size() == 1, "Oversized log line was emitted");
        return lines->front() == L"after large line";
    });
    append("\xff\nvalid\n");
    until([&] {
        auto lines = monitor.getNewLines();
        return lines && lines->size() == 1 && lines->front() == L"valid";
    });
    std::string burst;
    for (int i = 0; i < 20000; ++i)
        burst += "line\n";
    append(burst);
    std::this_thread::sleep_for(200ms);
    const auto bounded = monitor.getNewLines();
    check(bounded && bounded->size() <= 16384, "Monitor queue exceeded its line limit");
    monitor.stop();
    std::vector<std::jthread> operations;
    for (int i = 0; i < 4; ++i)
        operations.emplace_back([&] {
            for (int n = 0; n < 8; ++n) {
                monitor.start();
                monitor.stop();
            }
        });
    operations.clear();
    monitor.stop();
    const auto oldSize = fs::file_size(file);
    cs2config::write(file, "rewritten from byte zero\n" + std::string(static_cast<size_t>(oldSize) + 32, 'y') + "\n");
    monitor.start();
    until([&] {
        auto lines = monitor.getNewLines();
        return lines && lines->front() == L"rewritten from byte zero";
    });
    monitor.stop();
    cs2config::write(file, "old unfinished event");
    FileMonitor initial(file.wstring());
    initial.start();
    append(" continuation\nfresh event\n");
    until([&] {
        auto lines = initial.getNewLines();
        return lines && lines->size() == 1 && lines->front() == L"fresh event";
    });
    initial.stop();
}

void soundTests() {
    if (waveOutGetNumDevs() == 0)
        std::cout << "SKIP: audio output unavailable; checking the sound failure and cleanup paths\n";
    // 100 ms of unsigned 8-bit silence at 8 kHz.
    std::vector<unsigned char> wav(44 + 800, 128);
    const unsigned char header[] = {'R', 'I', 'F', 'F', 0x44, 0x03, 0,   0,   'W', 'A',  'V',  'E',  'f', 'm',  't',
                                    ' ', 16,  0,   0,   0,    1,    0,   1,   0,   0x40, 0x1f, 0,    0,   0x40, 0x1f,
                                    0,   0,   1,   0,   8,    0,    'd', 'a', 't', 'a',  0x20, 0x03, 0,   0};
    std::copy(std::begin(header), std::end(header), wav.begin());
    std::wstring file;
    {
        SimpleSound sound(wav.data(), static_cast<unsigned int>(wav.size()));
        file = SimpleSoundTestAccess::file(sound);
        sound.play();
        std::this_thread::sleep_for(200ms);
        sound.play();
        check(SimpleSoundTestAccess::requests(sound) <= 2, "Completed sound requests accumulated");
    }
    check(!fs::exists(file), "Temporary WAV remained after playback shutdown");
    wav.resize(44 + 64000, 128);
    const auto setSize = [&](size_t offset, unsigned int value) {
        for (unsigned int i = 0; i < 4; ++i)
            wav[offset + i] = static_cast<unsigned char>(value >> (8 * i));
    };
    setSize(4, 64036);
    setSize(40, 64000);
    auto stopped = std::chrono::steady_clock::now();
    {
        SimpleSound sound(wav.data(), static_cast<unsigned int>(wav.size()));
        file = SimpleSoundTestAccess::file(sound);
        for (int i = 0; i < 40; ++i)
            sound.play();
        check(SimpleSoundTestAccess::requests(sound) <= 16, "Concurrent sound requests exceeded the limit");
        std::this_thread::sleep_for(50ms);
        stopped = std::chrono::steady_clock::now();
    }
    check(std::chrono::steady_clock::now() - stopped < 1s && !fs::exists(file), "Long WAV prevented prompt shutdown");
}

void serverTests() {
    GSIServer server;
    GSIServerTestAccess::useTestPort(server);
    check(server.run(), "GSI listener failed to start");
    httplib::Client client("127.0.0.1", GSIServerTestAccess::port(server));
    client.set_connection_timeout(1, 0);
    client.set_read_timeout(2, 0);
    client.set_keep_alive(true);
    const auto good = client.Post("/", payload(5, 1000).dump(), "application/json");
    check(good && good->status == 200 && globals::getGameState().kills == 7, "GSI request failed");
    auto older = payload(4, 999);
    older["player"]["match_stats"]["kills"] = 99;
    const auto late = client.Post("/", older.dump(), "application/json");
    check(late && late->status == 200 && globals::getGameState().kills == 7, "Late GSI changed feature state");
    const auto bad = client.Post("/", "{bad", "application/json");
    check(bad && bad->status == 400, "Invalid GSI JSON accepted");
    auto malformed = payload(4, 1001);
    malformed["round"] = {{"bomb", 42}, {"phase", false}};
    malformed["player"]["match_stats"]["kills"] = "invalid";
    const auto typed = client.Post("/", malformed.dump(), "application/json");
    check(typed && typed->status == 200 && !globals::getGameState().bombPlanted && globals::getGameState().kills == -1,
          "Malformed GSI field changed unrelated state or escaped validation");
    {
        GSIServer occupied;
        GSIServerTestAccess::useTestPort(occupied, -1);
        check(!occupied.run(), "Invalid GSI listener reported successful startup");
    }
    {
        httplib::Client other("127.0.0.1", GSIServerTestAccess::port(server));
        other.set_read_timeout(0, 500000);
        const auto result = other.Post("/", payload(5, 1002).dump(), "application/json");
        check(result && result->status == 200, "Idle keep-alive client blocked another GSI request");
    }
    auto switching = payload(5, 1002);
    switching["player"]["weapons"]["weapon_0"]["type"] = "Pistol";
    const auto publish = [&](bool revolver) {
        switching["player"]["weapons"]["weapon_0"]["name"] = revolver ? "weapon_revolver" : "weapon_cz75a";
        switching["player"]["state"]["defusekit"] = revolver;
        switching["player"]["match_stats"]["kills"] = revolver ? 77 : 33;
        check(globals::updateGameState(switching), "Concurrent snapshot packet rejected");
    };
    publish(true);
    std::atomic_bool inconsistent = false;
    std::jthread reader([&](std::stop_token stop) {
        while (!stop.stop_requested()) {
            const auto game = globals::getGameState();
            if (!game.pistol || game.cz75 == game.revolver || game.defuseKit != game.revolver ||
                game.kills != (game.revolver ? 77 : 33))
                inconsistent = true;
        }
    });
    for (int i = 0; i < 1000; ++i)
        publish(i % 2 == 0);
    reader.request_stop();
    reader.join();
    check(!inconsistent, "Concurrent readers observed a partial GSI packet");
    globals::localPlayerSlotNumber = 42;
    publish(true);
    check(globals::localPlayerSlotNumber == 42, "A continuous GSI session cleared the local slot");
    switching["map"]["name"] = "new-session";
    switching["provider"]["timestamp"] = 1003;
    check(globals::updateGameState(switching) && globals::localPlayerSlotNumber == -1,
          "New session retained an old server's player slot");

    const auto before = std::chrono::steady_clock::now();
    server.stop();
    server.stop();
    check(std::chrono::steady_clock::now() - before < 3s, "GSI shutdown blocked");
}

void serverShutdownTests() {
    for (bool body : {false, true}) {
        GSIServer server;
        GSIServerTestAccess::useTestPort(server);
        server.run();
        struct Connection {
            SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            ~Connection() {
                if (socket != INVALID_SOCKET)
                    closesocket(socket);
            }
        } connection;
        check(connection.socket != INVALID_SOCKET, "Cannot create partial-request socket");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<unsigned short>(GSIServerTestAccess::port(server)));
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        check(connect(connection.socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0,
              "Cannot connect partial-request client");
        const std::string prefix = body ? "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 1000000\r\n\r\n{"
                                        : "POST / HTTP/1.1\r\nHost: localhost\r\nX-Slow: ";
        check(send(connection.socket, prefix.data(), static_cast<int>(prefix.size()), 0) ==
                  static_cast<int>(prefix.size()),
              "Cannot send partial request");
        std::array<Connection, 8> queued;
        for (auto &pending : queued) {
            check(pending.socket != INVALID_SOCKET &&
                      connect(pending.socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0,
                  "Cannot connect queued GSI client");
            check(send(pending.socket, prefix.data(), static_cast<int>(prefix.size()), 0) ==
                      static_cast<int>(prefix.size()),
                  "Cannot send queued partial request");
        }
        std::jthread producer([&](std::stop_token stop) {
            const auto deadline = std::chrono::steady_clock::now() + 4s;
            while (!stop.stop_requested() && std::chrono::steady_clock::now() < deadline) {
                if (send(connection.socket, " ", 1, 0) != 1)
                    break;
                std::this_thread::sleep_for(20ms);
            }
            shutdown(connection.socket, SD_BOTH);
        });
        std::this_thread::sleep_for(100ms);
        const auto before = std::chrono::steady_clock::now();
        server.stop();
        const auto elapsed = std::chrono::steady_clock::now() - before;
        producer.request_stop();
        producer.join();
        std::cout << "Partial GSI " << (body ? "body" : "header")
                  << " shutdown: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() << " ms\n";
        check(elapsed < 3s, "Continuously incomplete HTTP request blocked shutdown");
    }
}

struct OverlayTestAccess {
    static void failure() {
        WNDCLASSEXW existing{};
        existing.cbSize = sizeof(existing);
        existing.lpfnWndProc = DefWindowProcW;
        existing.hInstance = GetModuleHandleW(nullptr);
        existing.lpszClassName = Overlay::windowClassName;
        check(RegisterClassExW(&existing) != 0, "Cannot reserve the fixture window class");
        globals::previewMode = true;
        globals::finish = false;
        {
            Overlay overlay;
            overlay.OverlayLoop();
            check(globals::finish && !overlay.window_handle && !overlay.g_pd3dDevice && !ImGui::GetCurrentContext(),
                  "Failed overlay initialization retained resources");
        }
        WNDCLASSEXW retained{};
        retained.cbSize = sizeof(retained);
        check(GetClassInfoExW(existing.hInstance, existing.lpszClassName, &retained),
              "Overlay unregistered a window class it did not own");
        check(UnregisterClassW(existing.lpszClassName, existing.hInstance), "Fixture window class was not released");
        globals::previewMode = false;
    }
    static void test() {
        Overlay overlay;
        overlay.window_handle = CreateWindowExW(0, L"STATIC", L"ErScripts resource test", WS_OVERLAPPEDWINDOW, 0, 0,
                                                640, 480, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        check(overlay.window_handle != nullptr, "Cannot create test window");
        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferDesc.Width = 640;
        desc.BufferDesc.Height = 480;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferCount = 2;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow = overlay.window_handle;
        desc.SampleDesc.Count = 1;
        desc.Windowed = TRUE;
        D3D_FEATURE_LEVEL level;
        const HRESULT created = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &desc, &overlay.g_pSwapChain,
            &overlay.g_pd3dDevice, &level, &overlay.g_pd3dDeviceContext);
        check(SUCCEEDED(created), "WARP device creation failed");
        overlay.CreateRenderTarget();
        check(overlay.g_mainRenderTargetView != nullptr, "Render target missing");
        check(graphics::resize(overlay.g_pd3dDevice, overlay.g_pd3dDeviceContext, overlay.g_pSwapChain,
                               overlay.g_mainRenderTargetView, 1024, 768) == S_OK,
              "Swap chain resize failed");
        check(graphics::resize(overlay.g_pd3dDevice, overlay.g_pd3dDeviceContext, overlay.g_pSwapChain,
                               overlay.g_mainRenderTargetView, 1024, 768) == S_FALSE,
              "Unchanged target recreated");
        check(graphics::resize(overlay.g_pd3dDevice, overlay.g_pd3dDeviceContext, overlay.g_pSwapChain,
                               overlay.g_mainRenderTargetView, 0, 768) == E_INVALIDARG,
              "Invalid resize accepted");
        const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        check(SUCCEEDED(apartment), "Test COM apartment unavailable");
        auto *texture = overlay.LoadTextureFromPNGShellcode(c4Icon, c4IconLen);
        check(texture != nullptr, "WIC texture creation failed in an existing COM apartment");
        texture->Release();
        const HRESULT stillInitialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        check(stillInitialized == S_FALSE, "Texture loading uninitialized the caller's COM apartment");
        CoUninitialize();
        CoUninitialize();

        ImGui::CreateContext();
        auto &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        io.DisplaySize = {1280, 900};
        io.DeltaTime = 1.f / 60.f;
        overlay.menu_font = overlay.arial_font = overlay.bold_font = io.Fonts->AddFontDefault();
        io.Fonts->Build();
        globals::width = 1280;
        globals::height = 900;
        globals::setWindowBounds({1280, 900, 0, 0});
        globals::previewMode = true;
        globals::menuState = true;
        cfg->bombTimerState = cfg->keystrokesState = true;
        ImGui::NewFrame();
        ImGui::Begin("Test root");
        overlay.draw_list = ImGui::GetBackgroundDrawList();
        overlay.Render();
        overlay.Menu();
        ImGui::End();
        ImGui::Render();
        check(ImGui::GetDrawData()->TotalVtxCount > 0, "Overlay and menu produced no draw data");
        ImGui::DestroyContext();
        globals::previewMode = false;
        overlay.CleanupDeviceD3D();
        check(!overlay.g_pSwapChain && !overlay.g_pd3dDevice && !overlay.g_mainRenderTargetView,
              "D3D cleanup incomplete");
        DestroyWindow(overlay.window_handle);
        overlay.window_handle = nullptr;
    }
};

void timerTests() {
    globals::finish = false;
    cfg->bombTimerState = false;
    cfg->publish();
    auto game = payload(5, 2000);
    int timestamp = 2000;
    const auto send = [&](const char *bomb) {
        game["provider"]["timestamp"] = ++timestamp;
        game["round"]["bomb"] = bomb;
        check(globals::updateGameState(game), "Bomb state packet rejected");
    };
    send("defused");
    {
        GameConsole console;
        Features scripts(console);
        scripts.BombTimer();
        send("planted");
        until([] { return globals::bombTime > 0; });
        const auto first = globals::bombTime.load();
        std::this_thread::sleep_for(50ms);
        cfg->bombTimerState = true;
        cfg->publish();
        check(globals::bombTime < first - 20, "Enabling the display restarted the planted bomb timer");
        send("defused");
        until([] { return globals::bombTime == 0; });
        send("planted");
        until([] { return globals::bombTime > 39900; });
        std::this_thread::sleep_for(50ms);
        const auto beforeMapChange = globals::bombTime.load();
        game["map"]["name"] = "new-map";
        send("planted");
        until([&] { return globals::bombTime > beforeMapChange + 20; });
        send("defused");
    }
}

void workerTests() {
    *cfg = Config{};
    cfg->publish();
    globals::finish = false;
    const auto before = std::chrono::steady_clock::now();
    {
        GameConsole console;
        Features scripts(console);
        scripts.AutoAccept();
        scripts.BombTimer();
        scripts.KillSound();
        scripts.RoundStartAlert();
        scripts.PixelTrigger();
        scripts.Crosshair();
        scripts.RGBCrosshair();
        scripts.KnifeSwitch();
        scripts.AutoPistol();
        scripts.AntiAfk();
        scripts.CS2Binds();
        scripts.KillSay();
        scripts.AutoStop();
        scripts.ChatSpammer();
        std::this_thread::sleep_for(30ms);
    }
    check(globals::finish && std::chrono::steady_clock::now() - before < 2s, "Feature workers did not join promptly");
}

int main() {
    try {
        Fixture fixture;
        const std::pair<const char *, void (*)()> tests[] = {
            {"input lifetime", inputLifetimeTests},
            {"bounded AutoStop delays", delayTests},
            {"configuration", configTests},
            {"parsing and versions", parsingTests},
            {"GSI and sniper dynamics", playerTests},
            {"coherent game snapshots", gameStateTests},
            {"crosshair geometry", geometryTests},
            {"export", exportTests},
            {"file monitoring", monitorTests},
            {"sound lifetime", soundTests},
            {"GSI server lifecycle", serverTests},
            {"partial GSI request shutdown", serverShutdownTests},
            {"D3D and WIC resources", OverlayTestAccess::test},
            {"overlay initialization failure", OverlayTestAccess::failure},
            {"bomb timer transitions", timerTests},
            {"feature worker lifecycle", workerTests}};
        for (const auto &[name, test] : tests) {
            test();
            std::cout << "PASS: " << name << '\n';
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
