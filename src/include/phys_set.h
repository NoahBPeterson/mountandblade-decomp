// phys_set.h: the Newton physics set. One PhysSet object, g_phys (0xa181f8): a std::vector of 0x38-byte world
// records (ragdoll, static tree collision, extra bodies) plus the world count. Members come from several objects:
// src/game/agent/phys_set.cpp (0x441360..0x4416c0), src/game/agent2/agent_stats.cpp (0x43a8a0, 0x43a900),
// src/game/terrain_b/terrain_b.cpp (0x472c50, 0x472dc0, called from src/rgl/mesh2/gui_edit.cpp as well).
// create_world 0x44fad0 is only referenced. Needs rgl::matrix (rgl_base.h picks the module's own declaration).
#ifndef MB_PHYS_SET_H
#define MB_PHYS_SET_H
#include <vector>
#include "./rgl_base.h"

struct NewtonWorld; struct NewtonBody; struct NewtonCollision; struct NewtonRagDoll; struct NewtonRagDollBone;

struct PhysRec {                                  // 0x38 bytes
    int f0;
    const NewtonWorld* world;                     // +4 (0x441360, 0x4413e0)
    const NewtonRagDoll* ragdoll;                 // +8
    std::vector<const NewtonRagDollBone*> bones;  // +0xc (first at +0x10, last at +0x14; 0x43a8a0)
    std::vector<const NewtonBody*> bodies;        // +0x1c (first at +0x20, last at +0x24)
    const NewtonCollision* collision;             // +0x2c (0x441610, 0x4417d0, 0x43a900)
    const NewtonBody* body;                       // +0x30
    int material;                                 // +0x34
};

struct PhysSet {
    std::vector<PhysRec> recs;                    // +0 (first at +4)
    char pad10[0x3c - 0x10];
    int nworlds;                                  // +0x3c (0x441360)
    void destroy_world(int idx);                  // 0x441360
    const NewtonRagDollBone* add_bone(int idx, const NewtonRagDollBone* parent, const float* mass, rgl::matrix m0,
                                      const float* radius, const float* height, rgl::matrix m1, const float* cone_dir,
                                      const float* lim_min, const float* lim_max, const float* lim_twist);   // 0x4413e0 (ret 0xa4)
    void begin_static_build(int idx);             // 0x441610
    NewtonBody* add_capsule_body(int idx, const float* radius, const float* height, const float* mass, rgl::matrix* m0, rgl::matrix* m1, bool list);   // 0x4416c0 (ret 0x1c)
    unsigned create_world();                      // 0x44fad0 (src/game/agent4/agent_update.cpp: first free record or a new one)
    bool none_sleeping(int idx);                  // 0x43a8a0 (src/game/agent2/agent_stats.cpp)
    void rebuild_static_body(int idx);            // 0x43a900 (src/game/agent2/agent_stats.cpp)
    void clamp_rec(int i);                        // 0x472c50 (thiscall, ret 4; src/game/terrain_b/terrain_b.cpp)
    void update(float dt);                        // 0x472dc0
};
extern PhysSet g_phys;                            // 0xa181f8

#endif // MB_PHYS_SET_H
