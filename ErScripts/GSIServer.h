#pragma once

#include <httplib/httplib.h>
#include <nlohmann/json.hpp>
#include "ErScripts.h"
#include <thread>
#include <optional>

class GSIServer {
private:
    const int port = 23561; // Server port
    httplib::Server server; // HTTP server instance

    // Helper function to check if player is local player
    std::optional<nlohmann::json> getLocalPlayerData(const nlohmann::json& data);

    // Helper to check weapon states
    bool hasActiveWeaponType(const nlohmann::json& playerData, const std::string& weaponType);

    void gsiServer();                                                                           // Runs the server
    void handleJsonPayload(const nlohmann::json& data);                                         // Processes JSON data
    bool handleSniperCrosshairState(const nlohmann::json& data);                                // Checks for sniper rifle
    bool handleBombState(const nlohmann::json& data);                                           // Checks if bomb is planted
    bool handleDefuseKitState(const nlohmann::json& data);                                      // Checks for defuse kit
    std::string handleSteamID(const nlohmann::json& data);                                      // Gets SteamID
    bool handleKnifeState(const std::optional<nlohmann::json>& playerData);                     // Checks for knife
    bool handlePistolState(const std::optional<nlohmann::json>& playerData);                    // Checks for pistol
    bool handleRoundStartState(const nlohmann::json& data);                                     // Checks if round is live
    bool handleIsBombInWeapons(const std::optional<nlohmann::json>& playerData);                // Checks for bomb in inventory
    int handleLocalPlayerKills(const std::optional<nlohmann::json>& playerData);                // Gets local player kills
    bool handleIsLocalPlayerActivityPlaying(const std::optional<nlohmann::json>& playerData);   // Checks if player activity is "playing"
    bool handlActiveWeaponState(const std::optional<nlohmann::json>& playerData, const std::string weaponName); // Checks for active weapon

public:
    void run();     // Starts server in a thread
    void stop();    // Stops server
};