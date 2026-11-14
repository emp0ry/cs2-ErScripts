#include "GSIServer.h"
#include "Logger.h"
#include "Globals.h"
#include <thread>
#include <optional>

std::optional<nlohmann::json> GSIServer::getLocalPlayerData(const nlohmann::json &data) {
    if (const auto player = gsi::localPlayer(data))
        return *player;
    return std::nullopt;
}

bool GSIServer::hasActiveWeaponType(const nlohmann::json &playerData, const std::string &weaponType) {
    if (!playerData.contains("weapons") || !playerData["weapons"].is_object()) {
        return false;
    }

    for (const auto &[_, weapon] : playerData["weapons"].items()) {
        if (weapon.is_object() && weapon.contains("type") && weapon["type"].is_string() && weapon.contains("state") &&
            weapon["state"].is_string()) {
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
    server.Post("/", [this](const httplib::Request &req, httplib::Response &res) {
        try {

            nlohmann::json jsonData = nlohmann::json::parse(req.body);
            handleJsonPayload(jsonData);
            res.set_content("OK", "text/plain");
        } catch (const nlohmann::json::exception &e) {
            Logger::logWarning(std::format("JSON parsing error: {}", e.what()));
            res.status = 400;
            res.set_content("Invalid JSON", "text/plain");
        }
    });

    server.set_read_timeout(2, 0);
    server.set_write_timeout(2, 0);
    server.set_payload_max_length(1024 * 1024);
    bool bound;
    if (port == 0) {
        port = server.bind_to_any_port("127.0.0.1");
        bound = port > 0;
    } else {
        bound = server.bind_to_port("127.0.0.1", port);
    }
    if (!bound) {
        Logger::logWarning(std::format("Failed to start GSI server on 127.0.0.1:{}", port));
        return;
    }
    Logger::logInfo(std::format("Starting GSI server on 127.0.0.1:{}", port));
    if (!server.listen_after_bind())
        Logger::logWarning("GSI server stopped after a listener error");
}

void GSIServer::run() {
    if (worker.joinable())
        return;
    worker = std::thread([this] {
        try {
            gsiServer();
        } catch (const std::exception &error) {
            server.stop();
            server.decommission();
            Logger::logWarning(std::format("GSI server stopped: {}", error.what()));
        }
    });
    // stop() must not race with listen() before cpp-httplib marks the server running.
    server.wait_until_ready();
}

GSIServer::~GSIServer() {
    stop();
}

void GSIServer::stop() {
    if (!worker.joinable())
        return;
    Logger::logInfo("Stopping GSI server");
    server.stop();
    worker.join();
}

void GSIServer::handleJsonPayload(const nlohmann::json &data) {
    std::lock_guard lock(payloadMutex);
    auto playerData = getLocalPlayerData(data);

    if (!globals::updatePlayerState(data))
        return;
    globals::bombState = handleBombState(data);
    globals::defusekitState = handleDefuseKitState(playerData);
    globals::roundStartState = handleRoundStartState(data);
    globals::knifeState = handleKnifeState(playerData);
    globals::pistolState = handlePistolState(playerData);
    globals::isBombInWeapons = handleIsBombInWeapons(playerData);
    globals::localPlayerKills = handleLocalPlayerKills(playerData);
    globals::localPlayerIsActivityPlaying = handleIsLocalPlayerActivityPlaying(playerData);
    globals::revolverState = handlActiveWeaponState(playerData, "weapon_revolver");
    globals::cz75aState = handlActiveWeaponState(playerData, "weapon_cz75a");
}

bool GSIServer::handleBombState(const nlohmann::json &data) {
    const auto round = data.find("round");
    return round != data.end() && gsi::text(*round, "bomb") == "planted";
}

bool GSIServer::handleDefuseKitState(const std::optional<nlohmann::json> &playerData) {
    if (!playerData || !playerData->contains("state") || !(*playerData)["state"].is_object())
        return false;
    const auto kit = (*playerData)["state"].find("defusekit");
    return kit != (*playerData)["state"].end() && kit->is_boolean() && kit->get<bool>();
}

bool GSIServer::handleRoundStartState(const nlohmann::json &data) {
    const auto round = data.find("round");
    return round != data.end() && gsi::text(*round, "phase") == "live";
}

int GSIServer::handleLocalPlayerKills(const std::optional<nlohmann::json> &playerData) {
    if (!playerData || !playerData->contains("match_stats") || !(*playerData)["match_stats"].contains("kills")) {
        return 0;
    }
    return std::max(0, gsi::count((*playerData)["match_stats"], "kills"));
}

std::string GSIServer::handleSteamID(const nlohmann::json &data) {
    if (data.contains("provider") && data["provider"].contains("steamid")) {
        return data["provider"]["steamid"].get<std::string>();
    }
    return "";
}

bool GSIServer::handleKnifeState(const std::optional<nlohmann::json> &playerData) {
    return playerData && hasActiveWeaponType(*playerData, "Knife");
}

bool GSIServer::handlePistolState(const std::optional<nlohmann::json> &playerData) {
    return playerData && hasActiveWeaponType(*playerData, "Pistol");
}

bool GSIServer::handleIsBombInWeapons(const std::optional<nlohmann::json> &playerData) {
    return playerData && hasActiveWeaponType(*playerData, "C4");
}

bool GSIServer::handleIsLocalPlayerActivityPlaying(const std::optional<nlohmann::json> &playerData) {
    return playerData && gsi::text(*playerData, "activity") == "playing";
}

bool GSIServer::handlActiveWeaponState(const std::optional<nlohmann::json> &playerData, const std::string weaponName) {
    if (!playerData || !playerData->contains("weapons") || !(*playerData)["weapons"].is_object()) {
        return false;
    }

    for (const auto &[_, weapon] : (*playerData)["weapons"].items()) {
        if (weapon.is_object() && weapon.contains("name") && weapon["name"].is_string() && weapon.contains("state") &&
            weapon["state"].is_string()) {
            std::string name = weapon["name"].get<std::string>();
            std::string state = weapon["state"].get<std::string>();
            if (name == weaponName && (state == "active")) {
                return true;
            }
        }
    }

    return false;
}
