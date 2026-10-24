// view_state.h: the 0x5374-byte view / camera sub-object of an agent (agent +0x258; a camera block reached by offset in
// 0x463370 / 0x463610). One definition for src/game/terrain_b (constructor 0x475e50, agent record) and src/game/ui
// (troop_inv.cpp: init 0x463370, get_dir 0x463610).
#ifndef GAME_VIEW_STATE_H
#define GAME_VIEW_STATE_H
#include "../include/rgl_base.h"
#include "../include/vec.h"

extern __int64 g_ticks[];                       // 0x5e2948 clock table, 1e-5 s ticks

// 8-byte tick values stored as two dwords: the agent's sub-objects are not 8-byte aligned (ViewState is 0x5374 long)
struct tick { int lo, hi; };
inline const tick& tk(int i) { return ((const tick*)g_ticks)[i]; }
struct Timer16 {
    int clock;                  // +0 index into g_ticks
    int pad4;
    tick start;                 // +8
    void init(int c) { clock = c; start = tk(clock); }
};
struct Vec4z { float x, y, z, w; };

// The view sub-object of an agent (0x5374 bytes at agent +0x258): five timers, the target agent id and two floats
// the team statistics write. The agent's constructor calls the timer set-up 0x475e50 (defined below as the constructor
// of ViewState, the slices' TimedObj) explicitly, after its first stores; init() 0x463370 is the reset path.
struct ViewBody {
    char pad0[0x58];
    Timer16 t58;                // +0x58
    char pad68[0x98 - 0x68];
    Timer16 t98;                // +0x98
    Timer16 ta8;                // +0xa8
    char padb8[0xc4 - 0xb8];
    int target;                 // +0xc4 (agent +0x31c) id of another agent (0x474e60)
    char padc8[0x50e8 - 0xc8];
    Vec2 v50e8;                 // +0x50e8 (agent +0x5340) (0x474e60)
    char pad50f0[0x5118 - 0x50f0];
    Timer16 t5118;              // +0x5118
    Timer16 t5128;              // +0x5128
    char pad5138[0x516c - 0x5138];
    Vec4z v516c;                // +0x516c
    char pad517c[0x5374 - 0x517c];
    void init_timers();         // 0x475e50  // old name of ViewState::ViewState, removed after the names merge
    void init();                // 0x463370 (src/game/ui/troop_inv.cpp: the reset path)
    rgl::vector4 get_dir(int a, int b);     // 0x463610 (troop_inv.cpp)
};
struct ViewState : ViewBody {
    ViewState();                // 0x475e50 (terrain_b.cpp)
};

#endif // GAME_VIEW_STATE_H
