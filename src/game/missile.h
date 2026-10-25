// missile.h: the flying missile record (0xb0 bytes, 16 per page of the pool at Mission+0x30; agent3 Pool / PoolRec are the same
// pool: +4 in-use flag, +8 id, +0xc serial). ONE definition for src/game: agent4 (spawn_missile 0x44be50, swing_trace 0x45a170,
// hit_damage) and terrain_a (ammo_scale 0x467f80, init 0x467fb0) each saw a partial view. The includer must have rgl::vector4
// declared (rgl.h or rgl_math.h) and ItemRef (item_ref.h) available.
#ifndef GAME_MISSILE_H
#define GAME_MISSILE_H
#include "./item_ref.h"

struct SceneEnt;
struct capsule;
struct Missile {
    char pad0[0x10];
    rgl::vector4 p10;                                   // +0x10 (0x44be50)
    rgl::vector4 p20;                                   // +0x20
    char pad30[0x40 - 0x30];
    rgl::vector4 p40;                                   // +0x40 (its dot with the camera offset decides fa0 in init 0x467fb0)
    SceneEnt* ent;                                      // +0x50
    float f54;                                          // +0x54 (init: 0.01)
    unsigned shooter;                                   // +0x58
    float f5c;                                          // +0x5c
    float f60;                                          // +0x60
    float f64;                                          // +0x64
    int f68;                                            // +0x68 (init: 0)
    char pad6c[4];
    int clock;                                          // +0x70 clock index of t0 (init: g_ticks[clock])
    char pad74[4];
    __int64 t0;                                         // +0x78 creation tick
    ItemRef launcher;                                   // +0x80
    ItemRef ammo;                                       // +0x88 (ammo.item < 0: no ammunition item, ammo_scale 0.8)
    char spent;                                         // +0x90 the missile already hit (0x45a170); init clears it
    char pad91[0x94 - 0x91];
    float f94;                                          // +0x94 (init: 1.0)
    float f98;                                          // +0x98 (6.0 for itcf_throw_axe, 12.0 for itcf_throw_knife, else 0) (0x44e2b0)
    char f9c;                                           // +0x9c (init: 1)
    char pad9d[3];
    int fa0;                                            // +0xa0 (init: 1 when the missile flies away from the camera)
    int fa4;                                            // +0xa4 (init: -1)
    char pada8[0xb0 - 0xa8];
    void init();                                        // 0x467fb0
    void ray(capsule* out);                    // 0x46b410 (ret 4)
    float ammo_scale();                                 // 0x467f80 (the carried item's length * 0.01)
};
typedef char missile_size_check[(sizeof(Missile) == 0xb0) ? 1 : -1];

#endif // GAME_MISSILE_H
