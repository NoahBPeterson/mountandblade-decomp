// missile.h: the flying projectile as far as the ground correction (0x472650) goes, and the item table entry whose
// ammunition scale it reads. MissileG is in an anonymous namespace so that cl knows which registers its (in-TU)
// ammo_scale touches: the original keeps the "true" result in DL across that call.
#ifndef TERRAIN_B_MISSILE_H
#define TERRAIN_B_MISSILE_H
#include "../../rgl/rgl_math.h"

struct ItemKindG { char pad0[0x20c]; int f20c; char pad210[0x238 - 0x210]; };   // 0x238-byte item records
extern ItemKindG* g_items_g;                       // 0x5e29d0
struct MissileOwner { char pad[0x88]; rgl::vector4 hit; };
struct QueryPt { char pad0[0x30]; float x, y, z; };
namespace {
struct MissileG {
    char pad0[0x20];
    rgl::vector4 pos;                              // +0x20
    char pad30[0x40 - 0x30];
    rgl::vector4 dir;                              // +0x40
    MissileOwner* owner;                           // +0x50
    char pad54[0x88 - 0x54];
    int ammo_item;                                 // +0x88
    __declspec(noinline) float ammo_scale();       // 0x467f80 (a copy of the other TU's function)
    bool settle(const char* q);                    // 0x472650
};
}

#endif // TERRAIN_B_MISSILE_H
