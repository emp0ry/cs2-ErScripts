#include "core/ConfigExport.h"
#include "game/GameConsole.h"
#include "core/Globals.h"
#include <nlohmann/json.hpp>
#include <windows.h>
#include <ctime>

namespace fs = std::filesystem;
namespace {
    std::string utf8(std::wstring_view value) {
        if (value.empty())
            return {};
        const int count =
            WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        std::string out(count, '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), out.data(), count, nullptr,
                            nullptr);
        return out;
    }
} // namespace
ConfigExport &ConfigExport::instance() {
    static ConfigExport exporter;
    return exporter;
}
ConfigExport::~ConfigExport() {
    std::lock_guard owner(workerMutex);
    worker.request_stop();
    if (worker.joinable())
        worker.join();
}
ConfigExport::Status ConfigExport::status() {
    std::lock_guard lock(mutex);
    if ((state.queued || state.capturing) && deadline.time_since_epoch().count() &&
        std::chrono::steady_clock::now() > deadline) {
        state.busy = state.queued = state.capturing = false;
        state.message = "Export timed out. Use Saved files or try again with CS2 focused.";
    }
    return state;
}
void ConfigExport::start(bool live, const fs::path &source) {
    std::lock_guard owner(workerMutex);
    {
        std::lock_guard lock(mutex);
        if (state.busy)
            return;
    }
    if (worker.joinable())
        worker.join();
    {
        std::lock_guard lock(mutex);
        state = {};
        state.busy = true;
        state.message = "Reading settings...";
        deadline = {};
        static std::atomic_uint64_t nextSession = 0;
        generation = ++nextSession;
    }
    try {
        worker = std::jthread([this, live, source](std::stop_token stop) { prepare(stop, live, source); });
    } catch (const std::system_error &error) {
        std::lock_guard lock(mutex);
        state.busy = false;
        state.message = std::format("Cannot start export: {}", error.what());
    }
}
void ConfigExport::cancel() {
    std::lock_guard owner(workerMutex);
    std::lock_guard lock(mutex);
    if (!state.busy || writing)
        return;
    worker.request_stop();
    state.busy = state.queued = state.capturing = false;
    state.message = "Cancelled. Source files were not changed.";
}
void ConfigExport::prepare(std::stop_token stop, bool live, const fs::path &selectedSource) {
    Status prepared;
    cs2config::Snapshot values;
    try {
        const auto source = selectedSource.empty() ? SteamTools::getCS2ConfigFolder() : selectedSource;
        if (!fs::is_directory(source))
            throw std::runtime_error("Open Steam and sign in before exporting.");
        const auto stamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        prepared.folder =
            fs::absolute("exports") / ("cs2-" + std::to_string(stamp) + "-" + std::to_string(GetCurrentProcessId()) +
                                       "-" + std::to_string(generation));
        if (!fs::create_directories(prepared.folder / "source"))
            throw std::runtime_error("Export folder already exists.");
        nlohmann::ordered_json manifest;
        manifest["format"] = 1;
        manifest["mode"] = live ? "live" : "saved";
        manifest["files"] = nlohmann::json::array();
        std::vector<fs::path> paths;
        for (const auto &entry : fs::directory_iterator(source)) {
            if (!entry.is_regular_file() || entry.is_symlink())
                continue;
            const auto name = entry.path().filename().string();
            if ((name.starts_with("cs2_") && (entry.path().extension() == ".vcfg" || name == "cs2_video.txt")) ||
                name == "video.txt" || name == "videodefaults.txt")
                paths.push_back(entry.path());
        }
        std::sort(paths.begin(), paths.end());
        for (const auto &path : paths) {
            if (stop.stop_requested())
                return;
            const auto text = cs2config::read(path);
            cs2config::write(prepared.folder / "source" / path.filename(), text);
            const auto name = path.filename().string();
            if (name == "cs2_machine_convars.vcfg" || name == "cs2_user_convars_0_slot0.vcfg" ||
                name == "cs2_user_keys_0_slot0.vcfg")
                values.vcfg(text);
            manifest["files"].push_back(name);
            ++prepared.files;
        }
        if (values.convars.empty())
            throw std::runtime_error("No CS2 settings found for the signed-in Steam account.");
        const auto game =
            selectedSource.empty()
                ? SteamTools::getAppInstallPath("730", {.possibleFolders = {L"Counter-Strike Global Offensive"}})
                : std::nullopt;
        if (game) {
            const auto configs = fs::path(*game) / "game/csgo/cfg";
            if (fs::is_directory(configs)) {
                fs::create_directory(prepared.folder / "scripts");
                for (const auto &entry : fs::directory_iterator(configs)) {
                    if (stop.stop_requested())
                        return;
                    if (!entry.is_regular_file() || entry.is_symlink() || entry.path().extension() != ".cfg")
                        continue;
                    const auto name = entry.path().filename().string();
                    if (name.starts_with("erscripts") || name.starts_with("gamestate_integration"))
                        continue;
                    cs2config::write(prepared.folder / "scripts" / entry.path().filename(),
                                     cs2config::read(entry.path()));
                    ++prepared.files;
                }
            }
        }
        cs2config::write(prepared.folder / "manifest.json", manifest.dump(2));
        std::unique_lock lock(mutex);
        if (stop.stop_requested())
            return;
        prepared.busy = true;
        state = std::move(prepared);
        snapshot = std::move(values);
        capture.clear();
        section = 0;
        liveRows = 0;
        if (live) {
            token = "ers_export_" + std::to_string(stamp) + "_" + std::to_string(generation);
            state.queued = true;
            state.message = "Waiting for CS2 input. Return to the game and close its console.";
            deadline = std::chrono::steady_clock::now() + std::chrono::minutes(2);
        } else
            finish(lock);
    } catch (const std::exception &error) {
        std::lock_guard lock(mutex);
        if (stop.stop_requested())
            return;
        if (state.folder.empty())
            state.folder = prepared.folder;
        state.busy = state.queued = state.capturing = false;
        state.message = error.what();
    }
}
void ConfigExport::finish(std::unique_lock<std::mutex> &lock) {
    auto data = std::move(snapshot);
    auto session = std::move(capture);
    const auto folder = state.folder;
    const auto rows = liveRows;
    state.queued = state.capturing = false;
    state.message = "Writing export...";
    writing = true;
    lock.unlock();
    std::string error;
    try {
        cs2config::write(folder / "erscripts-export.cfg", data.cfg());
        nlohmann::ordered_json values;
        values["convars"] = data.convars;
        values["bindings"] = data.binds;
        values["aliases"] = data.aliases;
        values["session_convars"] = data.session;
        cs2config::write(folder / "settings.json", values.dump(2));
        if (!session.empty())
            cs2config::write(folder / "session.txt", session);
        cs2config::write(
            folder / "README.txt",
            "CS2 settings export\r\n\r\n"
            "erscripts-export.cfg: saved client convars and primary bindings, updated by live values when "
            "available.\r\n"
            "Copy this file into game/csgo/cfg, then run: exec erscripts-export\r\n"
            "No unbindall is inserted. Existing unlisted keys are retained.\r\n"
            "source/: exact saved settings, video, machine and additional split-screen files.\r\n"
            "scripts/: copies of local CFG files; none are executed.\r\n"
            "settings.json / session.txt: captured values and original live output for inspection.\r\n\r\n"
            "Video and machine-specific settings are preserved as raw files, not portable console commands.\r\n"
            "Saved files may lag behind unsaved in-game changes. Live capture reads cvarlist, binds and "
            "aliases.\r\n"
            "Hidden/protected variables and engine state are not guaranteed to be exposed. This is not a memory "
            "dump.\r\n");
    } catch (const std::exception &failure) {
        error = failure.what();
    }
    lock.lock();
    writing = false;
    state.busy = false;
    if (!error.empty()) {
        state.message = std::move(error);
        return;
    }
    state.convars = data.convars.size();
    state.binds = data.binds.size();
    state.aliases = data.aliases.size();
    state.ready = true;
    state.message = session.empty() || rows ? "Export saved."
                                            : "Saved files exported. Live output was not recognized; see session.txt.";
}
void ConfigExport::flush(GameConsole &console) {
    std::unique_lock lock(mutex);
    if ((state.queued || state.capturing) && std::chrono::steady_clock::now() > deadline) {
        state.busy = state.queued = state.capturing = false;
        state.message = "Live capture timed out. Saved files are available in the partial export.";
        return;
    }
    if (!state.queued || globals::menuState)
        return;
    const auto currentToken = token;
    const auto queuedDeadline = deadline;
    const auto command = "echo " + token + "_vars; cvarlist; echo " + token + "_binds; key_listboundkeys; echo " +
                         token + "_aliases; alias; echo " + token + "_end";
    state.queued = false;
    state.capturing = true;
    section = 0;
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    state.message = "Capturing live settings...";
    lock.unlock();
    const bool sent = console.send(6, command);
    lock.lock();
    if (!sent && state.capturing && token == currentToken && section == 0) {
        deadline = queuedDeadline;
        state.capturing = false;
        state.queued = true;
        state.message = "Waiting for CS2 input. Return to the game and close its console.";
    }
}
void ConfigExport::consume(std::wstring_view raw) {
    std::unique_lock lock(mutex);
    if (!state.capturing)
        return;
    try {
        const auto line = utf8(raw);
        const auto marker = cs2config::trim(line.starts_with("[Console]") ? line.substr(9) : line);
        if (marker == token + "_vars") {
            section = 1;
            return;
        }
        if (marker == token + "_binds" && section == 1) {
            section = 2;
            state.message = "Reading bindings...";
            return;
        }
        if (marker == token + "_aliases" && section == 2) {
            section = 3;
            state.message = "Reading aliases...";
            return;
        }
        if (marker == token + "_end") {
            if (section != 3)
                throw std::runtime_error("Incomplete live capture.");
            finish(lock);
            return;
        }
        if (!section)
            return;
        if (capture.size() + line.size() > 8 * 1024 * 1024)
            throw std::runtime_error("Live output exceeded the export limit.");
        capture += line + '\n';
        if (section == 1 && snapshot.convarLine(line))
            ++liveRows;
        if (section == 2 && snapshot.bindLine(line))
            ++liveRows;
        if (section == 3 && snapshot.aliasLine(line))
            ++liveRows;
    } catch (const std::exception &error) {
        state.busy = state.queued = state.capturing = false;
        state.message = error.what();
    }
}
