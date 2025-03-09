#include "GSIServer.h"
#include "Logger.h"
#include "Globals.h"
#include <thread>

void GSIServer::run() {
    std::thread serverThread(&GSIServer::gsiServer, this);
    serverThread.detach();
}

void GSIServer::stop() {
    Logger::logInfo("Stopping GSI server");
    server.stop();
}

void GSIServer::gsiServer() {
    server.Post("/", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            this->handleJsonPayload(nlohmann::json::parse(req.body));
            res.set_content("OK", "text/plain");
            //system("cls");std::cout << std::endl << std::endl << req.body << std::endl << std::endl;
        }
        catch (const nlohmann::json::exception& e) {
            Logger::logWarning(std::format("Error processing request: {}", e.what()));
            res.status = 400;
            res.set_content("Invalid JSON", "text/plain");
        }
        });

    Logger::logInfo(std::format("GSI server starting on 127.0.0.1:{}", port));

    if (!server.listen("127.0.0.1", port)) {
        Logger::logWarning(std::format("Failed to start GSI server on 127.0.0.1:{}", port));
    }
}

void GSIServer::handleJsonPayload(const nlohmann::json& data) {
    globals::sniperCrosshairState = handleSniperCrosshairState(data);
    globals::bombState = handleBombState(data);
    globals::defusekitState = handleDefuseKitState(data);
    globals::knifeState = handleKnifeState(data);
    globals::pistolState = handlePistolState(data);
    globals::roundStartState = handleRoundStartState(data);
    globals::isBombInWeapons = handleIsBombInWeapons(data);

    //static bool oneTimeGet = true;
    //if (oneTimeGet) {
    //    globals::steamid = handleSteamID(data);
    //    Logger::logInfo(std::format("SteamID: {}", globals::steamid));

    //    if (!globals::steamid.empty()) {
    //        oneTimeGet = false;
    //    }
    //}
}

bool GSIServer::handleSniperCrosshairState(const nlohmann::json& data) {
    if (data.contains("player")) {
        if (data["player"].contains("weapons")) {
            for (const auto& [key, weapon] : data["player"]["weapons"].items()) {
                if (weapon.contains("type") && weapon.contains("state")) {
                    std::string type = weapon.at("type").get<std::string>();
                    std::string state = weapon.at("state").get<std::string>();

                    if (type == "SniperRifle" && state == "active") {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool GSIServer::handleBombState(const nlohmann::json& data) {
    if (data.contains("round")) {
        if (data["round"].contains("bomb")) {
            if (data["round"].at("bomb").get<std::string>() == "planted") {
                return true;
            }
        }
    }
    return false;
}

bool GSIServer::handleDefuseKitState(const nlohmann::json& data) {
    if (data.contains("player")) {
        if (data["player"].contains("state")) {
            if (data["player"]["state"].contains("defusekit")) {
                return true;
            }
        }
    }
    return false;
}

std::string GSIServer::handleSteamID(const nlohmann::json& data) {
    if (data.contains("provider")) {
        if (data["provider"].contains("steamid")) {
             return data["provider"]["steamid"].get<std::string>();
        }
    }
    return "";
}

bool GSIServer::handleKnifeState(const nlohmann::json& data) {
    if (!data.contains("player") || !data.contains("provider")) {
        return false;
    }
    if (!data["player"].contains("steamid") || !data["provider"].contains("steamid")) {
        return false;
    }
    if (data["player"]["steamid"].get<std::string>() != data["provider"]["steamid"].get<std::string>()) {
        return false;
    }
    if (!data["player"].contains("weapons") || !data["player"]["weapons"].is_object()) {
        return false;
    }

    for (const auto& [key, weapon] : data["player"]["weapons"].items()) {
        if (weapon.contains("type") && weapon.contains("state")) {
            std::string type = weapon.at("type").get<std::string>();
            std::string state = weapon.at("state").get<std::string>();
            if (type == "Knife" && state == "active") {
                return true;
            }
        }
    }

    return false;
}

bool GSIServer::handlePistolState(const nlohmann::json& data) {
    if (!data.contains("player") || !data.contains("provider")) {
        return false;
    }
    if (!data["player"].contains("steamid") || !data["provider"].contains("steamid")) {
        return false;
    }
    if (data["player"]["steamid"].get<std::string>() != data["provider"]["steamid"].get<std::string>()) {
        return false;
    }
    if (!data["player"].contains("weapons") || !data["player"]["weapons"].is_object()) {
        return false;
    }

    for (const auto& [key, weapon] : data["player"]["weapons"].items()) {
        if (weapon.contains("type") && weapon.contains("state")) {
            std::string type = weapon.at("type").get<std::string>();
            std::string state = weapon.at("state").get<std::string>();
            if (type == "Pistol" && state == "active") {
                return true;
            }
        }
    }

    return false;
}


bool GSIServer::handleRoundStartState(const nlohmann::json& data) {
    if (data.contains("round")) {
        if (data["round"].contains("phase")) {
            if (data["round"].at("phase").get<std::string>() == "live") {
                return true;
            }
        }
    }
    return false;
}

bool GSIServer::handleIsBombInWeapons(const nlohmann::json& data) {
    if (!data.contains("player") || !data.contains("provider")) {
        return false;
    }
    if (!data["player"].contains("steamid") || !data["provider"].contains("steamid")) {
        return false;
    }
    if (data["player"]["steamid"].get<std::string>() != data["provider"]["steamid"].get<std::string>()) {
        return false;
    }
    if (!data["player"].contains("weapons") || !data["player"]["weapons"].is_object()) {
        return false;
    }

    for (const auto& [key, weapon] : data["player"]["weapons"].items()) {
        if (weapon.contains("type") && weapon.contains("state")) {
            std::string type = weapon.at("type").get<std::string>();
            if (type == "C4") {
                return true;
            }
        }
    }

    return false;
}