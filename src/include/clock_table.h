// clock_table.h: the game's clock table g_ticks (0x5e2948): 64-bit ticks of 1e-5 s, one per clock id (Timer::clock, Marker clocks,
// widget animation clocks index it); g_ticks[0] is advanced by GameApp::Update (inventory_panel/game_app.cpp), the others by the
// timer lists. ONE declaration for every module. No includes.
#ifndef MB_CLOCK_TABLE_H
#define MB_CLOCK_TABLE_H
extern __int64 g_ticks[7];                   // 0x5e2948 (7 clocks up to g_tick_cur 0x5e2980)
extern float g_step_tbl[5];                  // 0x5a9c20 per-clock step (seconds); [0] is the frame step (game_tick.cpp)
#endif // MB_CLOCK_TABLE_H
