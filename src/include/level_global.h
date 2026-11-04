// level_global.h: the current level / scene pointer g_level (0xa732b4), the rgl::world the game loop builds in Game::build_level
// (0x51d2b0, party/game.cpp defines it). ONE declaration: every module names it with this type and casts it to the view of the
// scene it uses (CellGrid, Scene, World, LevelGrid ...); a header that declared its own typed g_level is gone.
// Now the member ScriptState::level of g_script (include/script_state.h, itself free of heavy includes: terrain_b.h, game.h,
// world_data.h and the char_sheet headers cannot include rgl/scene/world.h). This header is kept as the name those modules include.
#ifndef MB_LEVEL_GLOBAL_H
#define MB_LEVEL_GLOBAL_H
#include "./script_state.h"
#endif // MB_LEVEL_GLOBAL_H
