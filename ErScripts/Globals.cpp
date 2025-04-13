#include "Globals.h"

namespace globals {
	int width = GetSystemMetrics(0), height = GetSystemMetrics(1), posX = 0, posY = 0;
	bool finish = false;
	bool menuState = true;

	bool sniperCrosshairState = false, isScope = false;
	bool bombState = false;
	bool defusekitState = false;
	double bombTime = 0.0l;
	bool knifeState = false;
	bool pistolState = false;
	bool revolverState = false, cz75aState = false;
	bool roundStartState = false;
	bool isBombInWeapons = false;
	int localPlayerKills = 0, localPlayerSlotNumber = 0;
	bool localPlayerIsActivityPlaying = false;
	std::string steamid = "", nickname = "";

	std::optional<SteamTools::Config> config;

	int cs2_ping = 0;
}