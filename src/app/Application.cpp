#include "app/Application.h"
#include "core/Config.h"
#include "core/Config.h"
#include "core/Globals.h"
#include "core/Logger.h"
#include "game/GameWindow.h"
#include "core/GradientManager.h"
#include "game/GameConsole.h"
#include "game/GameConsole.h"
#include <format>
#include "app/AppVersion.h"
#include "features/Features.h"
#include "core/ConfigExport.h"
#include "game/GSIServer.h"
#include "graphics/Overlay.h"
#include <iostream>

void Application::pollExitKey() {
    const int key = Config::snapshot()->erScriptsExitBind;
    const auto now = std::chrono::steady_clock::now();
    if (exitKey < 0)
        exitKey = key;
    if (key != exitKey) {
        exitKey = key;
        exitKeyChanged = now;
    } else if (now - exitKeyChanged >= std::chrono::seconds(2) && (GetAsyncKeyState(key) & 0x8000))
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
    features.AutoAccept();
    features.BombTimer();
    features.KillSound();
    features.RoundStartAlert();

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
    features.PixelTrigger();
    features.Crosshair();
    features.RGBCrosshair();
    features.KnifeSwitch();
    features.AutoPistol();
    features.AntiAfk();
    features.CS2Binds();
    features.KillSay();
    features.AutoStop();
    features.ChatSpammer();

    auto lastPing = std::chrono::steady_clock::now();
    auto lastSettings = lastPing;
    while (!globals::finish) {
        pollExitKey();
        auto &exporter = ConfigExport::instance();
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
