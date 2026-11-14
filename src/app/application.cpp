#include "app/application.hpp"

#include "app/app_version.h"
#include "core/color_gradient.hpp"
#include "core/config.hpp"
#include "core/config_export.hpp"
#include "core/globals.hpp"
#include "core/logger.hpp"
#include "features/features.hpp"
#include "game/game_console.hpp"
#include "game/game_window.hpp"
#include "game/gsi_server.hpp"
#include "graphics/overlay.hpp"

#include <format>
#include <iostream>

void Application::pollExitKey() {
    const int key = Config::snapshot()->erScriptsExitBind;
    const auto now = std::chrono::steady_clock::now();
    if (exitKey_ < 0)
        exitKey_ = key;
    if (key != exitKey_) {
        exitKey_ = key;
        exitKeyChanged_ = now;
    } else if (now - exitKeyChanged_ >= std::chrono::seconds(2) && (GetAsyncKeyState(key) & 0x8000))
        globals::finish = true;
}

int Application::run(bool preview) {
    if (preview) {
        globals::previewMode = true;
        globals::width = 1280;
        globals::height = 900;
        crosshair::Settings settings;
        settings.modernFields = 15;
        settings.loaded = true;
        globals::setConfig(settings);
        SetConsoleTitleA("ErScripts preview");
        Overlay overlay;
        overlay.run();
        while (!globals::finish)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 0;
    }
    SetConsoleTitleA(std::format("ErScripts {}", APP_VERSION).c_str());
    std::cout << "[-] *---------------------------------------*\n"
                 "[>] |  ErScripts by emp0ry                  |\n"
                 "[>] |  GitHub - https://github.com/emp0ry/  |\n"
                 "[-] *---------------------------------------*\n";
    cfg->load("default");
    colorGradient().setConfig(cfg->gradient);
    cfg->publish();

    GameConsole console;
    console.initialize();
    if (!game::window()) {
        Logger::logInfo("Please start CS2!");
        while (!globals::finish && !game::window()) {
            pollExitKey();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    if (globals::finish)
        return 1;
    GSIServer gsi;
    Overlay overlay;
    // Reverse destruction joins feature workers before their console dependency.
    Features features(console);
    overlay.run();
    if (!gsi.run())
        throw std::runtime_error("Cannot start the GSI listener");
    console.start();
    features.autoAccept();
    features.bombTimer();
    features.killSound();
    features.roundStartAlert();

    auto firstBind = std::chrono::steady_clock::time_point{};
    auto nextBind = std::chrono::steady_clock::now();
    while (!globals::finish && !console.ready()) {
        pollExitKey();
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextBind) {
            nextBind = now + std::chrono::milliseconds(100);
            if (console.bind() && firstBind == std::chrono::steady_clock::time_point{})
                firstBind = now;
        }
        if (firstBind != std::chrono::steady_clock::time_point{} && now - firstBind >= std::chrono::seconds(10)) {
            Logger::logWarning("Initialization failed: CS2 did not confirm the binds within 10 seconds");
            globals::finish = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    if (globals::finish)
        return 1;
    Logger::logSuccess("ErScripts binds initialized");
    features.pixelTrigger();
    features.crosshair();
    features.rgbCrosshair();
    features.knifeSwitch();
    features.autoPistol();
    features.antiAfk();
    features.cS2Binds();
    features.killSay();
    features.autoStop();
    features.chatSpammer();

    auto lastPing = std::chrono::steady_clock::now();
    auto lastSettings = lastPing;
    while (!globals::finish) {
        pollExitKey();
        auto& exporter = ConfigExport::instance();
        exporter.flush(console);
        if (!exporter.status().busy && game::hasFocus() && game::cursorHidden()) {
            const auto settings = Config::snapshot();
            const auto now = std::chrono::steady_clock::now();
            if (settings->watermarkState && now - lastPing >= std::chrono::seconds(settings->watermarkPingUpdateRate)) {
                console.send(5, "sys_info");
                lastPing = now;
            }
            if (now - lastSettings >= std::chrono::milliseconds(1500)) {
                console.send(5, "print_changed_convars; echo [ErScripts] convars_end");
                lastSettings = now;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    ConfigExport::instance().cancel();
    return 0;
}
