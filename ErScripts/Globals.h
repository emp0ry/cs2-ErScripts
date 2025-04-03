#pragma once

#include "SteamTools.h"
#include <windows.h>

namespace globals {
	extern int width, height, posX, posY;
	extern bool finish;
	extern bool menuState;

	extern bool sniperCrosshairState, isScope;
	extern bool bombState;
	extern bool defusekitState;
	extern double bombTime;
	extern bool knifeState;
	extern bool pistolState;
	extern bool revolverState, cz75aState;
	extern bool roundStartState;
	extern bool isBombInWeapons;
	extern int localPlayerKills, localPlayerSlotNumber;
	extern bool localPlayerIsActivityPlaying;
	extern std::string steamid, nickname;

	extern bool crosshairUpdaterState;
	extern std::optional<SteamTools::Crosshair> crosshair;

	extern int cs2_ping;
}