// clock_table.h: the game's clocks (Timer::clock, Marker clocks, widget animation clocks index them), ONE declaration for every module.
// Two arrays of five 64-bit values, adjacent in the original's data: g_ticks[i] is clock i's time in 1e-5 s and g_tick_dt[i] the length of
// its last step in ticks (g_step_tbl[i] the same in seconds). g_ticks[0] is advanced by GameApp::Update (inventory_panel/game_app.cpp),
// g_ticks[2] by the mission frame (mesh_slot/mesh_game.cpp), the others by set_clock_step (terrain_a/wind.cpp). No includes.
#ifndef MB_CLOCK_TABLE_H
#define MB_CLOCK_TABLE_H
extern __int64 g_ticks[5];                   // 0x5e2948 (to 0x5e2970)
extern __int64 g_tick_dt[5];                 // 0x5e2970 (to 0x5e2998; [2] is the last mission frame's tick count)
extern float g_step_tbl[5];                  // 0x5a9c20 per-clock step (seconds); [0] is the frame step (game_tick.cpp)
#endif // MB_CLOCK_TABLE_H
