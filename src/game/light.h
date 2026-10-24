// light.h: the 0x7c-byte scene light (type 2 = directional). One definition for src/game/agent2 (light_at 0x43ac40),
// src/game/strat (Ctx::first_dir_light 0x42a1a0, sample_lights 0x42ad60, StratEnt::render 0x42a090) and
// src/game/script (add_point_light: ctor-style init 0x461480, flicker update 0x487620).
// The constructor/updater share their addresses with the wind object (terrain_a/wind.h `wind`, terrain_b `Wind`): the
// object is a flickering light whose first vector is its colour and whose direction vector starts as (0, 1, 0).
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
    char pad58[0x6c - 0x58];
    unsigned flags;                     // +0x6c bit 0: disabled / half intensity, bit 1: no shadows (0x43ac40, 0x42a1a0)
    float flicker_magnitude;            // +0x70
    float flicker_interval;             // +0x74
    int phase;                          // +0x78
    Light* init();                      // 0x461480 (ctor)  // old name of wind::wind, removed after the names merge
    void update();                      // 0x487620  // old name of TimeKey::calc, removed after the names merge
};

#endif // GAME_LIGHT_H
