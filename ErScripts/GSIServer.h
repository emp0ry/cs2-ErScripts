#pragma once

#include <httplib/httplib.h>
#include <nlohmann/json.hpp>
#include "ErScripts.h"
#include <thread>

class GSIServer {
private:
    const int port = 23561;
    httplib::Server server;

    void gsiServer();
    void handleJsonPayload(const nlohmann::json& data);
    bool handleSniperCrosshairState(const nlohmann::json& data);
    bool handleBombState(const nlohmann::json& data);
    bool handleDefuseKitState(const nlohmann::json& data);
    std::string handleSteamID(const nlohmann::json& data);
    bool handleKnifeState(const nlohmann::json& data);
    bool handlePistolState(const nlohmann::json& data);
    bool handleRoundStartState(const nlohmann::json& data);
    bool handleIsBombInWeapons(const nlohmann::json& data);
    int handleLocalPlayerKills(const nlohmann::json& data);

public:
    void run();
    void stop();
};