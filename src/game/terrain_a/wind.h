// wind.h: the wind object (0x7c bytes) and the agent hit-point initialiser of wind.cpp (0x461200..0x461630).
// Names provisional; each offset cites the function that reads or writes it.
#ifndef TERRAIN_A_WIND_H
#define TERRAIN_A_WIND_H
#include "./common.h"

// The agent record (init_max_hp 0x461200 reads +0x1bc and writes +0x1c0): src/game/agent_obj.h.
#include "../agent_obj.h"

#include "../wind.h"                         // wind (the 0x7c-byte wind track; shared with char_sheet and inventory_panel)

elem24* __cdecl copy_range(elem24* first, elem24* last, elem24* dest);          // 0x461230
elem24* __cdecl copy_backward24(elem24* first, elem24* last, elem24* dest);     // 0x461280
rgl::vector4 tri_normal(const rgl::vector4* p);                                  // 0x4612d0
unsigned pack_signed_color(const float* c);                                      // 0x461370
void set_clock_step(int* clock, float step);                                     // 0x461420

#endif // TERRAIN_A_WIND_H
