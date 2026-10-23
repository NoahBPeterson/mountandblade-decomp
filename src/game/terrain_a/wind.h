// wind.h: the wind object (0x7c bytes) and the agent hit-point initialiser of wind.cpp (0x461200..0x461630).
// Names provisional; each offset cites the function that reads or writes it.
#ifndef TERRAIN_A_WIND_H
#define TERRAIN_A_WIND_H
#include "./common.h"

// Agent record seen only through its hit points: init_max_hp (0x461200) reads +0x1bc and writes +0x1c0. The shared
// Agent headers (src/game/agent/agent.h, src/game/agent3/agent_types.h) do not name these two fields (agent_types.h
// calls +0x1bc `int f1bc`), so this module declares the two it needs.
struct Agent {
    char pad0[0x1bc];
    float hp;                       // +0x1bc (0x461200)
    float max_hp;                   // +0x1c0 (0x461200)
    void init_max_hp();             // 0x461200
};

// Wind: direction vector at +4 scaled by a fractal-noise factor driven by clock 2 (wrapped at 1e9 ticks) and a
// random per-instance phase (0x461480 init, 0x461570 sample).
struct wind {
    int f0;                         // +0x00
    rgl::vector4 v4, v14, v24, v34, v44;    // +0x04.. (0x461480 clears the first four, v44 = (0, 1, 0))
    float f54, f58, f5c, f60, f64, f68, f6c;   // +0x54.. (f5c = 1.0)
    float strength;                 // +0x70 (0x461570)
    float noise_seed;               // +0x74 (0x461570: first argument of noise_fractal)
    int phase;                      // +0x78 (rand() % 0x3f9)
    wind* init();                   // 0x461480
    rgl::vector4 sample();          // 0x461570 (hidden return pointer)
};

elem24* __cdecl copy_range(elem24* first, elem24* last, elem24* dest);          // 0x461230
elem24* __cdecl copy_backward24(elem24* first, elem24* last, elem24* dest);     // 0x461280
rgl::vector4 tri_normal(const rgl::vector4* p);                                  // 0x4612d0
unsigned pack_signed_color(const float* c);                                      // 0x461370
void set_clock_step(int* clock, float step);                                     // 0x461420

#endif // TERRAIN_A_WIND_H
