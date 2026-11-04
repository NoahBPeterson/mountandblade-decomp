// float_consts.h: single-precision constants the original kept in .rdata and read through memory (a literal would be
// an immediate or fld1 / a folded constant and change the code of the functions that load them). One declaration per
// constant; the definitions are in agent5/editor_dialog.cpp (g_one: the file that always defined it) and agent/agent.cpp
// (the rest, a TU that does not read them, so cl cannot fold them).
#ifndef GAME_FLOAT_CONSTS_H
#define GAME_FLOAT_CONSTS_H
extern float g_one;                        // 0x549cdc 1.0f
extern float g_hundred;                    // 0x54ab28 100.0f
extern const float g_chunk_scale;          // 0x549d68 0.3f
extern const float g_half_ratio;           // 0x549d78 -0.5f
extern const float g_k039;                 // 0x54b3b0 0.39f
extern const float g_rand_k;               // 0x54a6bc 1.0f / 0x3dc9 (rand() % 0x3dc9 scale)
extern const float g_k065;                 // 0x590b6c 0.65f
extern const float g_ratio_b;              // 0x597fe8 -2.1f
extern const float g_ratio_a;              // 0x597fec 0.525f
extern const float g_row_unit;             // 0x599ae8 0.088f (height of one row of the 3-column item grid)
#endif
