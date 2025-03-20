#include "GSIServer.h"
#include "Logger.h"
#include "Globals.h"
#include <thread>
#include <optional>

// Helper function to check if player is local player
std::optional<nlohmann::json> GSIServer::getLocalPlayerData(const nlohmann::json& data) {
    if (!data.contains("player") || !data.contains("provider")) {
        return std::nullopt;
    }
    if (!data["player"].contains("steamid") || !data["provider"].contains("steamid")) {
        return std::nullopt;
    }
    if (data["player"]["steamid"].get<std::string>() != data["provider"]["steamid"].get<std::string>()) {
        return std::nullopt;
    }
    return data["player"];
}

// Helper to check weapon states
bool GSIServer::hasActiveWeaponType(const nlohmann::json& playerData, const std::string& weaponType) {
    if (!playerData.contains("weapons") || !playerData["weapons"].is_object()) {
        return false;
    }

    for (const auto& [_, weapon] : playerData["weapons"].items()) {
        if (weapon.contains("type") && weapon.contains("state")) {
            std::string type = weapon["type"].get<std::string>();
            std::string state = weapon["state"].get<std::string>();
            if (type == weaponType && (state == "active" || weaponType == "C4")) {
                return true;
            }
        }
    }
    return false;
}

void GSIServer::gsiServer() {
    server.Post("/", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            //system("cls"); std::cout << req.body << std::endl;
            nlohmann::json jsonData = nlohmann::json::parse(req.body);
            handleJsonPayload(jsonData);
            res.set_content("OK", "text/plain");
        }
        catch (const nlohmann::json::exception& e) {
            Logger::logWarning(std::format("JSON parsing error: {}", e.what()));
            res.status = 400;
            res.set_content("Invalid JSON", "text/plain");
        }
        });

    Logger::logInfo(std::format("Starting GSI server on 127.0.0.1:{}", port));
    if (!server.listen("127.0.0.1", port)) {
        Logger::logWarning(std::format("Failed to start GSI server on 127.0.0.1:{}", port));
    }
}

void GSIServer::run() {
    std::thread serverThread(&GSIServer::gsiServer, this);
    serverThread.detach();
}

void GSIServer::stop() {
    Logger::logInfo("Stopping GSI server");
    server.stop();
}

void GSIServer::handleJsonPayload(const nlohmann::json& data) {
    auto playerData = getLocalPlayerData(data);

    globals::sniperCrosshairState = handleSniperCrosshairState(data);
    globals::bombState = handleBombState(data);
    globals::defusekitState = handleDefuseKitState(data);
    globals::roundStartState = handleRoundStartState(data);
    globals::knifeState = handleKnifeState(playerData);
    globals::pistolState = handlePistolState(playerData);
    globals::isBombInWeapons = handleIsBombInWeapons(playerData);
    globals::localPlayerKills = handleLocalPlayerKills(playerData);
    globals::localPlayerIsActivityPlaying = handleIsLocalPlayerActivityPlaying(playerData);
    globals::revolverState = handlIsActiveWeaponRevolver(playerData);
}

bool GSIServer::handleSniperCrosshairState(const nlohmann::json& data) {
    if (!data.contains("player") || !data["player"].contains("weapons")) {
        return false;
    }
    return hasActiveWeaponType(data["player"], "SniperRifle");
}

bool GSIServer::handleBombState(const nlohmann::json& data) {
    return data.contains("round") &&
        data["round"].contains("bomb") &&
        data["round"]["bomb"].get<std::string>() == "planted";
}

bool GSIServer::handleDefuseKitState(const nlohmann::json& data) {
    return data.contains("player") &&
        data["player"].contains("state") &&
        data["player"]["state"].contains("defusekit");
}

bool GSIServer::handleRoundStartState(const nlohmann::json& data) {
    return data.contains("round") &&
        data["round"].contains("phase") &&
        data["round"]["phase"].get<std::string>() == "live";
}

int GSIServer::handleLocalPlayerKills(const std::optional<nlohmann::json>& playerData) {
    if (!playerData || !playerData->contains("match_stats") || !(*playerData)["match_stats"].contains("kills")) {
        return 0;
    }
    return (*playerData)["match_stats"]["kills"].get<int>();
}

std::string GSIServer::handleSteamID(const nlohmann::json& data) {
    if (data.contains("provider") && data["provider"].contains("steamid")) {
        return data["provider"]["steamid"].get<std::string>();
    }
    return "";
}

bool GSIServer::handleKnifeState(const std::optional<nlohmann::json>& playerData) {
    return playerData && hasActiveWeaponType(*playerData, "Knife");
}

bool GSIServer::handlePistolState(const std::optional<nlohmann::json>& playerData) {
    return playerData && hasActiveWeaponType(*playerData, "Pistol");
}

bool GSIServer::handleIsBombInWeapons(const std::optional<nlohmann::json>& playerData) {
    return playerData && hasActiveWeaponType(*playerData, "C4");
}

bool GSIServer::handleIsLocalPlayerActivityPlaying(const std::optional<nlohmann::json>& playerData) {
    return playerData && playerData->contains("activity") && (*playerData)["activity"].get<std::string>() == "playing";
}

bool GSIServer::handlIsActiveWeaponRevolver(const std::optional<nlohmann::json>& playerData) {
    if (!playerData && !playerData->contains("weapons") || !(*playerData)["weapons"].is_object()) {
        return false;
    }

    for (const auto& [_, weapon] : (*playerData)["weapons"].items()) {
        if (weapon.contains("name") && weapon.contains("state")) {
            std::string name = weapon["name"].get<std::string>();
            std::string state = weapon["state"].get<std::string>();
            if (name == "weapon_revolver" && (state == "active")) {
                return true;
            }
        }
    }

    return false;
}