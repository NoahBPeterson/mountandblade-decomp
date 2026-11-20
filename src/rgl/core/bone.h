// bone.h: one bone of a skeleton model (rgl/resource.h's frame_set::bones) with its ragdoll record, ONE definition for
// src/rgl (core/resource_io.cpp w_skeleton, core/resmgr.cpp read_frame_set / the constructor, dynamic/fx_anim.cpp,
// scene/fxdef_init.cpp) and src/game (agent4/agent_logic.cpp: the ragdoll 0x44fc60 / 0x4543a0 and the capsule hit test 0x43f6d0).
#ifndef RGL_CORE_BONE_H
#define RGL_CORE_BONE_H
#include <vector>
#include "../rgl_string.h"
#include "../rgl_math.h"

// one collision capsule of a bone (0x2c bytes): the axis end points a / b in the bone frame and the radius
// (skeleton_bodies.xml's capsules; Agent::capsule_hit 0x43f6d0, Agent::start_ragdoll 0x44fc60)
struct CapDef {
    rgl::vector4 a;                // +0
    rgl::vector4 b;                // +0x10
    float r;                       // +0x20
    int pad[2];                    // +0x24
};

// One bone of a skeleton model (0xdc bytes; ctor 0x41b0c0, dtor 0x41a510, vector deleting dtor 0x41a550):
// w_skeleton 0x416490 and read_frame_set 0x41b180 give the first 0x9c bytes, the ctor the rest.
struct Bone {
    int f0;                        // +0
    int parent;                    // +4  (< 0: root)
    unsigned flags;                // +8
    rgl::matrix local;             // +0xc
    rgl::matrix world;             // +0x4c
    rgl::string name;              // +0x8c
    // +0x9c..: the bone's ragdoll record (mesh2/mesh_2.cpp fix_skeletons 0x4cd850 copies skeleton_bodies.xml's bones here;
    // the names are the attributes options_panel/ragdoll_models.cpp RagBone::load 0x4d3330 reads)
    float bone_mass;               // +0x9c (2.0 in the ctor)
    float cone_min, cone_max, twist_limit;   // +0xa0 joint limits in degrees (the ragdoll passes them to add_bone by address)
    rgl::vector4 rotate;           // +0xac rotate_side, rotate_forward, rotate_up (degrees)
    rgl::vector4 socket_dir;       // +0xbc (0, 1, 0)
    std::vector<CapDef> bodies;    // +0xcc the collision capsules (first at +0xd0)
    Bone();                        // 0x41b0c0
    // the real destructor 0x41a510 frees the capsule vector and destroys the name (implicit)
};

#endif // RGL_CORE_BONE_H
