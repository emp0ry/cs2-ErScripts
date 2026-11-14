#include "core/config_export.hpp"

#include "core/globals.hpp"
#include "game/game_console.hpp"

#include <ctime>
#include <windows.h>

#include <nlohmann/json.hpp>

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
ConfigExport& ConfigExport::instance() {
    static ConfigExport exporter;
    return exporter;
}
ConfigExport::~ConfigExport() {
    std::lock_guard owner(workerMutex_);
    worker_.request_stop();
    if (worker_.joinable())
        worker_.join();
}
ConfigExport::Status ConfigExport::status() {
    std::lock_guard lock(mutex_);
    if ((state_.queued || state_.capturing) && deadline_.time_since_epoch().count() &&
        std::chrono::steady_clock::now() > deadline_) {
        state_.busy = state_.queued = state_.capturing = false;
        state_.message = "Export timed out. Use Saved files or try again with CS2 focused.";
    }
    return state_;
}
void ConfigExport::start(bool live, const fs::path& source) {
    std::lock_guard owner(workerMutex_);
    {
        std::lock_guard lock(mutex_);
        if (state_.busy)
            return;
    }
    if (worker_.joinable())
        worker_.join();
    {
        std::lock_guard lock(mutex_);
        state_ = {};
        state_.busy = true;
        state_.message = "Reading settings...";
        deadline_ = {};
        static std::atomic_uint64_t nextSession = 0;
        generation_ = ++nextSession;
    }
    try {
        worker_ = std::jthread([this, live, source](std::stop_token stop) { prepare(stop, live, source); });
    } catch (const std::system_error& error) {
        std::lock_guard lock(mutex_);
        state_.busy = false;
        state_.message = std::format("Cannot start export: {}", error.what());
    }
}
void ConfigExport::cancel() {
    std::lock_guard owner(workerMutex_);
    std::lock_guard lock(mutex_);
    if (!state_.busy || writing_)
        return;
    worker_.request_stop();
    state_.busy = state_.queued = state_.capturing = false;
    state_.message = "Cancelled. Source files were not changed.";
}
void ConfigExport::prepare(std::stop_token stop, bool live, const fs::path& selectedSource) {
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
                                       "-" + std::to_string(generation_));
        if (!fs::create_directories(prepared.folder / "source"))
            throw std::runtime_error("Export folder already exists.");
        nlohmann::ordered_json manifest;
        manifest["format"] = 1;
        manifest["mode"] = live ? "live" : "saved";
        manifest["files"] = nlohmann::json::array();
        std::vector<fs::path> paths;
        for (const auto& entry : fs::directory_iterator(source)) {
            if (!entry.is_regular_file() || entry.is_symlink())
                continue;
            const auto name = entry.path().filename().string();
            if ((name.starts_with("cs2_") && (entry.path().extension() == ".vcfg" || name == "cs2_video.txt")) ||
                name == "video.txt" || name == "videodefaults.txt")
                paths.push_back(entry.path());
        }
        std::sort(paths.begin(), paths.end());
        for (const auto& path : paths) {
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
                for (const auto& entry : fs::directory_iterator(configs)) {
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
        std::unique_lock lock(mutex_);
        if (stop.stop_requested())
            return;
        prepared.busy = true;
        state_ = std::move(prepared);
        snapshot_ = std::move(values);
        capture_.clear();
        section_ = 0;
        liveRows_ = 0;
        if (live) {
            token_ = "ers_export_" + std::to_string(stamp) + "_" + std::to_string(generation_);
            state_.queued = true;
            state_.message = "Waiting for CS2 input. Return to the game and close its console.";
            deadline_ = std::chrono::steady_clock::now() + std::chrono::minutes(2);
        } else
            finish(lock);
    } catch (const std::exception& error) {
        std::lock_guard lock(mutex_);
        if (stop.stop_requested())
            return;
        if (state_.folder.empty())
            state_.folder = prepared.folder;
        state_.busy = state_.queued = state_.capturing = false;
        state_.message = error.what();
    }
}
void ConfigExport::finish(std::unique_lock<std::mutex>& lock) {
    auto data = std::move(snapshot_);
    auto session = std::move(capture_);
    const auto folder = state_.folder;
    const auto rows = liveRows_;
    state_.queued = state_.capturing = false;
    state_.message = "Writing export...";
    writing_ = true;
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
    } catch (const std::exception& failure) {
        error = failure.what();
    }
    lock.lock();
    writing_ = false;
    state_.busy = false;
    if (!error.empty()) {
        state_.message = std::move(error);
        return;
    }
    state_.convars = data.convars.size();
    state_.binds = data.binds.size();
    state_.aliases = data.aliases.size();
    state_.ready = true;
    state_.message = session.empty() || rows ? "Export saved."
                                             : "Saved files exported. Live output was not recognized; see session.txt.";
}
void ConfigExport::flush(GameConsole& console) {
    std::unique_lock lock(mutex_);
    if ((state_.queued || state_.capturing) && std::chrono::steady_clock::now() > deadline_) {
        state_.busy = state_.queued = state_.capturing = false;
        state_.message = "Live capture timed out. Saved files are available in the partial export.";
        return;
    }
    if (!state_.queued || globals::menuState)
        return;
    const auto currentToken = token_;
    const auto queuedDeadline = deadline_;
    const auto command = "echo " + token_ + "_vars; cvarlist; echo " + token_ + "_binds; key_listboundkeys; echo " +
                         token_ + "_aliases; alias; echo " + token_ + "_end";
    state_.queued = false;
    state_.capturing = true;
    section_ = 0;
    deadline_ = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    state_.message = "Capturing live settings...";
    lock.unlock();
    const bool sent = console.send(6, command);
    lock.lock();
    if (!sent && state_.capturing && token_ == currentToken && section_ == 0) {
        deadline_ = queuedDeadline;
        state_.capturing = false;
        state_.queued = true;
        state_.message = "Waiting for CS2 input. Return to the game and close its console.";
    }
}
void ConfigExport::consume(std::wstring_view raw) {
    std::unique_lock lock(mutex_);
    if (!state_.capturing)
        return;
    try {
        const auto line = utf8(raw);
        const auto marker = cs2config::trim(line.starts_with("[Console]") ? line.substr(9) : line);
        if (marker == token_ + "_vars") {
            section_ = 1;
            return;
        }
        if (marker == token_ + "_binds" && section_ == 1) {
            section_ = 2;
            state_.message = "Reading bindings...";
            return;
        }
        if (marker == token_ + "_aliases" && section_ == 2) {
            section_ = 3;
            state_.message = "Reading aliases...";
            return;
        }
        if (marker == token_ + "_end") {
            if (section_ != 3)
                throw std::runtime_error("Incomplete live capture.");
            finish(lock);
            return;
        }
        if (!section_)
            return;
        if (capture_.size() + line.size() > 8 * 1024 * 1024)
            throw std::runtime_error("Live output exceeded the export limit.");
        capture_ += line + '\n';
        if (section_ == 1 && snapshot_.convarLine(line))
            ++liveRows_;
        if (section_ == 2 && snapshot_.bindLine(line))
            ++liveRows_;
        if (section_ == 3 && snapshot_.aliasLine(line))
            ++liveRows_;
    } catch (const std::exception& error) {
        state_.busy = state_.queued = state_.capturing = false;
        state_.message = error.what();
    }
}
