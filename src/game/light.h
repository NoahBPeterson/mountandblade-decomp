// light.h: the 0x7c-byte scene light (type 2 = directional). One definition for src/game/agent2 (light_at 0x43ac40),
// src/game/strat (Ctx::first_dir_light 0x42a1a0, sample_lights 0x42ad60, StratEnt::render 0x42a090) and
// src/game/script (add_point_light: ctor-style init 0x461480, flicker update 0x487620).
// The wind object of terrain_a / terrain_b and the effect object of world_data are this record too: a flickering light whose
// first vector is its colour and whose direction vector starts as (0, 1, 0). Direction vector scaled by a fractal-noise factor
// driven by clock 2 (wrapped at 1e9 ticks) and a random per-instance phase (0x461480 init, 0x461570 sample).
#ifndef GAME_LIGHT_H
#define GAME_LIGHT_H
#include "../include/rgl_base.h"

struct Light {
    int type;                           // +0x00 0 = point (flicker), 2 = directional (0x43ac40, 0x42a1a0)
    rgl::vector4 color;                 // +0x04 r, g, b in x, y, z (0x43ac40 reads them as l->r / g / b)
    rgl::vector4 base_color;            // +0x14 colour before the flicker (script add_point_light)
    rgl::vector4 pos;                   // +0x24 point-light position (0x43ac40, 0x42ad60)
    rgl::vector4 v34;                   // +0x34 (the script's add_point_light stores the position delta here)
    rgl::vector4 dir;                   // +0x44 directional lights (0x43ac40, 0x42a1a0)
    float range;                        // +0x54 reach of a point light (0x43ac40)
    float f58;                          // +0x58 (0x461480 clears it)
    float f5c;                          // +0x5c (1.0 after 0x461480)
    float f60, f64, f68;                // +0x60 .. +0x68 (0x461480 clears them)
    unsigned flags;                     // +0x6c bit 0: disabled / half intensity, bit 1: no shadows (0x43ac40, 0x42a1a0)
    float flicker_magnitude;            // +0x70 (0x461570 multiplies the noise by it)
    float flicker_interval;             // +0x74 (0x461570: first argument of noise_fractal)
    int phase;                          // +0x78 (rand() % 0x3f9)
    Light() throw();                    // 0x461480 (zeroes the vectors, direction (0, 1, 0), random phase)
    rgl::vector4 sample();              // 0x461570 (hidden return pointer)
    void calc();                      // 0x487620
};

#endif // GAME_LIGHT_H
