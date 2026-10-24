// wind.h: the wind track object (0x7c bytes, ctor 0x461480, sample 0x461570). ONE definition for src/game: terrain_a/wind.cpp
// (init, sample), the face-generator window (char_sheet/facegen_init.cpp 0x4f49d0) and the inventory character view
// (inventory_panel/view_open.cpp) each built the two light tracks of a portrait world with a partial view of it.
// No includes: the includer must have rgl::vector4 declared (rgl_math.h or rgl.h).
#ifndef GAME_WIND_H
#define GAME_WIND_H

// Direction vector at +4 scaled by a fractal-noise factor driven by clock 2 (wrapped at 1e9 ticks) and a random per-instance
// phase (0x461480 init, 0x461570 sample). The portrait worlds store their light colours in v04 / v14 / v24 / v44 and 20.0 in
// f54 with type 2 (the track kind).
struct wind {
    int type;                       // +0x00 (0x461480 clears it; the face-gen / portrait worlds set 2)
    rgl::vector4 v04, v14, v24, v34, v44;   // +0x04.. (0x461480 clears the first four, v44 = (0, 1, 0))
    float f54, f58, f5c, f60, f64, f68, f6c;   // +0x54.. (f5c = 1.0)
    float strength;                 // +0x70 (0x461570)
    float noise_seed;               // +0x74 (0x461570: first argument of noise_fractal)
    int phase;                      // +0x78 (rand() % 0x3f9)
    wind() throw();                 // 0x461480 (the same function as init(); `new wind` in the portrait builders calls it)
    wind* init();                   // 0x461480  // old name of wind::wind, removed after the names merge
    rgl::vector4 sample();          // 0x461570 (hidden return pointer)
};

#endif // GAME_WIND_H
