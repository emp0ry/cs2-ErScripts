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
	bool roundStartState = false;
	bool isBombInWeapons = false;
	int localPlayerKills = 0;
	std::string steamid = "", nickname = "";

	bool crosshairUpdaterState = true;
	std::optional<SteamTools::Crosshair> crosshair;

	int cs2_ping = 0;
}