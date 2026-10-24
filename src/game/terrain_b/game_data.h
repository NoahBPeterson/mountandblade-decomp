// game_data.h: the game-data object (global pointer g_game, 0x5e2930) as far as terrain_b.cpp / mission.cpp read it:
// the party manager at +0xf8, the troop table (+0x13a00), the scene-object / level table (+0x13a50), the tick index of
// the mission clock and two flags. GameData, Troop, Party, PartyMgr, Stack and Level494 are the shared classes of
// src/include/game_data.h (one definition for src/game and src/rgl); offsets are cited there.
#ifndef TERRAIN_B_GAME_DATA_H
#define TERRAIN_B_GAME_DATA_H
#include <vector>
#include "../tstring.h"
#include "../../include/game_data.h"      // GameData, g_game, Level494 (the shared class headers troop.h / party.h / game_recs.h come with it)

#endif // TERRAIN_B_GAME_DATA_H
