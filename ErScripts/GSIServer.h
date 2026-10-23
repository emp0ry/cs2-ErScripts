#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "ErScripts.h"
#include <thread>
#include <optional>

class GSIServer {
  private:
    const int port = 23561;
    httplib::Server server;
    std::mutex payloadMutex;

    std::optional<nlohmann::json> getLocalPlayerData(const nlohmann::json &data);

    bool hasActiveWeaponType(const nlohmann::json &playerData, const std::string &weaponType);

    void gsiServer();
    void handleJsonPayload(const nlohmann::json &data);
    bool handleBombState(const nlohmann::json &data);
    bool handleDefuseKitState(const std::optional<nlohmann::json> &playerData);
    std::string handleSteamID(const nlohmann::json &data);
    bool handleKnifeState(const std::optional<nlohmann::json> &playerData);
    bool handlePistolState(const std::optional<nlohmann::json> &playerData);
    bool handleRoundStartState(const nlohmann::json &data);
    bool handleIsBombInWeapons(const std::optional<nlohmann::json> &playerData);
    int handleLocalPlayerKills(const std::optional<nlohmann::json> &playerData);
    bool handleIsLocalPlayerActivityPlaying(const std::optional<nlohmann::json> &playerData);
    bool handlActiveWeaponState(const std::optional<nlohmann::json> &playerData, const std::string weaponName);

  public:
    void run();
    void stop();
};
