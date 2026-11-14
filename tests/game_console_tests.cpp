#include "core/globals.hpp"
#include "game/game_console.hpp"
#include "test_support.hpp"

#include <fstream>
#include <iostream>

struct GameConsoleTestAccess {
    static void consume(GameConsole& console, const std::wstring& line) { console.consume(line); }
    static void expire(GameConsole& console) { console.lastConvar_ -= 3s; }
    static void log(GameConsole& console, const fs::path& path) { console.consoleLog_ = path.wstring(); }
};

void parserTests() {
    GameConsole console;
    const auto consume = [&](const wchar_t* line) { GameConsoleTestAccess::consume(console, line); };
    check(!console.ready() && !console.takeMatch(), "Console started with stale signals");
    consume(L"Server confirmed all players");
    check(console.takeMatch() && !console.takeMatch(), "Match signal was not consumed exactly once");
    consume(L"[*] ErScripts Bind Initialization");
    check(console.ready(), "Current bind acknowledgement was not recognized");
    GameConsole legacy;
    GameConsoleTestAccess::consume(legacy, L"[*] ErScripts Bind Initalization");
    check(legacy.ready(), "Legacy bind acknowledgement lost compatibility");
    check(!console.send(1, "echo test") && !console.bind(), "Uninitialized console attempted to send input");
    check(!console.send(0, "echo test") && !console.send(13, "echo test"), "Invalid command slots accepted");

    globals::cs2Ping = 0;
    consume(L"Misc: rate 786432 latency 37 msec");
    consume(L"Misc: rate 786432 latency 999999999999999999999999 msec");
    check(globals::cs2Ping == 37, "Invalid latency replaced the last valid value");
    globals::localPlayerSlotNumber = -1;
    consume(L"[Client] source   : slot -1");
    check(globals::localPlayerSlotNumber == -1, "Negative slot accepted");
    consume(L"[Client] source   : slot 7");
    check(globals::localPlayerSlotNumber == 7, "Valid slot ignored");

    SteamTools::Config baseline;
    baseline.sensitivity = 1.75f;
    globals::setConfig(baseline);
    consume(L"ConVars (non-default):");
    consume(L"sensitivity 2.5");
    check(globals::getConfig()->sensitivity == 1.75f, "Incomplete convars became visible");
    consume(L"[ErScripts] convars_end");
    check(globals::getConfig()->sensitivity == 2.5f, "Complete convars were not published");
    consume(L"ConVars (non-default):");
    consume(L"sensitivity 9");
    GameConsoleTestAccess::expire(console);
    consume(L"[ErScripts] convars_end");
    check(globals::getConfig()->sensitivity == 2.5f, "Expired partial convars replaced the current snapshot");
}

void lifetimeTests() {
    const auto log = fs::absolute("console.log");
    std::ofstream(log).close();
    globals::finish = false;
    const auto started = std::chrono::steady_clock::now();
    {
        GameConsole console;
        GameConsoleTestAccess::log(console, log);
        console.start();
        console.start();
        until([&] {
            if (console.ready())
                return true;
            std::ofstream output(log, std::ios::app);
            output << "[*] ErScripts Bind Initialization\n";
            return false;
        });
    }
    check(!globals::finish, "Console shutdown changed the application stop flag");
    check(std::chrono::steady_clock::now() - started < 2s, "Console worker did not stop independently");
}

int main() {
    try {
        Fixture fixture;
        parserTests();
        lifetimeTests();
        std::cout << "PASS: console parsing and independent shutdown\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
