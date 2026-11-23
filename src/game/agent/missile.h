// missile.h: the flying missile record (0xb0 bytes, 16 per page of the pool at Mission+0x30; agent3 Pool / PoolRec are the same
// pool: +4 in-use flag, +8 id, +0xc serial). ONE definition for src/game: agent4 (spawn_missile 0x44be50, swing_trace 0x45a170,
// hit_damage) and terrain_a (ammo_scale 0x467f80, init 0x467fb0) each saw a partial view. The includer must have rgl::vector4
// declared (rgl.h or rgl_math.h) and ItemRef (item_ref.h) available.
#ifndef GAME_MISSILE_H
#define GAME_MISSILE_H
#include "engine/core/rgl_base.h"   // rgl::vector4 (whichever declaration the includer uses)
#include "engine/core/layout32.h"
#include "game/data/item_ref.h"
#include "game/common/iface.h"         // IfaceB: the pooled record base (vptr, in_use, pool_id, serial)
#include "engine/core/timer.h"         // clock2_timer
namespace rgl { struct mesh; }

namespace rgl { struct entity; }
struct capsule;
struct Missile : IfaceB {                               // vtable 0x590b78 (slot 0 the vector deleting destructor 0x431750, slot 1 reset 0x437810)
    rgl::vector4 prev;                                  // +0x10 the previous position sample (update 0x47d3e0; spawn_missile 0x44be50)
    rgl::vector4 pos;                                   // +0x20
    rgl::vector4 start;                                 // +0x30 the position at the start of the frame (0x47d3e0)
    rgl::vector4 vel;                                   // +0x40 unit direction (its dot with the camera offset decides mode in init 0x467fb0)
    rgl::entity* ent;                                   // +0x50 the flying mesh (world::create_entity(9), spawn_missile 0x44be50; 0 once removed)
    float f54;                                          // +0x54 (init: 0.01)
    unsigned shooter;                                   // +0x58 the agent that shot it
    float speed;                                        // +0x5c
    float f60;                                          // +0x60
    float f64;                                          // +0x64
    int state;                                          // +0x68 0 flying, 1 stuck, 2 removed, 3 hit the ground (init: 0)
    char pad6c[4];
    clock2_timer clk;                                   // +0x70 (the constructor starts it on clock 2; init restamps it)
    ItemRef launcher;                                   // +0x80
    ItemRef ammo;                                       // +0x88 (ammo.item < 0: no ammunition item, ammo_scale 0.8)
    char spent;                                         // +0x90 the missile already hit (0x45a170); init clears it
    char pad91[0x94 - 0x91];
    float hit;                                          // +0x94 (init: 1.0; swing_trace's scale)
    float tumble;                                       // +0x98 (6.0 for itcf_throw_axe, 12.0 for itcf_throw_knife, else 0) (0x44e2b0)
    char f9c;                                           // +0x9c (init: 1)
    char pad9d[3];
    int mode;                                           // +0xa0 (init: 1 when the missile flies away from the camera)
    int sound;                                          // +0xa4 the flight sound channel (init: -1)
    float sound_t;                                      // +0xa8 (0x47d3e0)
    char padac[0xb0 - 0xac];
    Missile();                                          // 0x4377d0 (game/data/troop_proficiency.cpp)
    virtual void slot1();                               // 0x437810 reset
    void init();                                        // 0x467fb0
    void ray(capsule* out);                             // 0x46b410 (ret 4)
    float ammo_scale();                                 // 0x467f80 (the carried item's weight * 0.01; game/terrain/poly_mesh.cpp)
    bool settle(const capsule* c);                      // 0x472650 (game/terrain/poly_mesh.cpp: puts the missile on the ground under the swept capsule's end point)
    ItemRef ranged_ref();                               // 0x4351f0 (game/agent/agent_queries.cpp: the thrown launcher, else the ammunition)
    rgl::mesh* make_ranged_mesh();                      // 0x44f6a0 (game/agent/agent_logic.cpp: clones the display mesh of the carried ranged / thrown item)
    void update_ranged_mesh();                          // 0x47d320 (game/mission/scene_entities.cpp)
    void update(float dt);                              // 0x47d3e0 (game/mission/scene_entities.cpp: one integration step)
};
MB_LAYOUT32_ASSERT(missile_size_check, (sizeof(Missile) == 0xb0));

#endif // GAME_MISSILE_H
