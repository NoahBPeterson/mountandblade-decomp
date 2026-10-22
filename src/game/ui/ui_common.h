// ui_common.h: globals shared by the modules of src/game/ui (each comment gives the variable's VA in the image).
// Names provisional. The tick table is the game's set of 64-bit clocks (units of 1e-5 s); element 0 is game time.
#ifndef GAME_UI_COMMON_H
#define GAME_UI_COMMON_H

extern __int64 g_ticks[];                   // 0x5e2948 clock table, indexed by a per-object clock index (0x4625d0, 0x488560, 0x48a420)

extern unsigned g_rand_seed;                // 0x5d7078 (seeded by 0x462e20, read back after the multiply steps)

namespace rgl { struct matrix; matrix* identity_pos(matrix* out); }   // 0x40cd00 (cdecl; writes the identity frame, returns out)

#endif // GAME_UI_COMMON_H
