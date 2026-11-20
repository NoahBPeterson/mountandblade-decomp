// bone.h: one bone of a skeleton model (rgl/resource.h's frame_set::bones) with its ragdoll record, ONE definition for
// src/rgl (core/resource_io.cpp w_skeleton, core/resmgr.cpp read_frame_set / the constructor, dynamic/fx_anim.cpp,
// scene/fxdef_init.cpp) and src/game (agent4/agent_logic.cpp: the ragdoll 0x44fc60 / 0x4543a0 and the capsule hit test 0x43f6d0).
#ifndef RGL_CORE_BONE_H
#define RGL_CORE_BONE_H
#include <vector>
#include "../rgl_string.h"
#include "../rgl_math.h"

// one collision capsule of a bone (0x2c bytes): the axis end points a / b in the bone frame and the radius
// (skeleton_bodies.xml's capsules; Agent::capsule_hit 0x43f6d0, Agent::start_ragdoll 0x44fc60). The ctor and the
// loader are out of line in game/config/config.cpp (RagdollBone::load 0x4d3330 pushes CapDef() and loads the copy).
class TiXmlElement;
struct CapDef {
    rgl::vector4 a;                // +0      pos_x / pos_y / pos_z (the ctor leaves the 4th lane alone)
    rgl::vector4 b;                // +0x10   pos2_x / pos2_y / pos2_z
    float r;                       // +0x20   radius (0.05)
    unsigned char f24;             // +0x24   cleared by the ctor; no reader known
    unsigned char f25;             // +0x25   cleared by the ctor; no reader known
    int f28;                       // +0x28   cleared by the ctor; no reader known
    CapDef();                                       // 0x4bc170
    void load(TiXmlElement* src);                   // 0x4bc1f0 (radius / pos_* / pos2_* attributes, missing ones kept)
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
    // the names are the attributes options_panel/ragdoll_models.cpp RagdollBone::load 0x4d3330 reads; the same layout as RagdollBone from its name on)
    float bone_mass;               // +0x9c (2.0 in the ctor)
    float cone_min, cone_max, twist_limit;   // +0xa0 joint limits in degrees (the ragdoll passes them to add_bone by address)
    rgl::vector4 rotate;           // +0xac rotate_side, rotate_forward, rotate_up (degrees)
    rgl::vector4 socket_dir;       // +0xbc (0, 1, 0)
    std::vector<CapDef> bodies;    // +0xcc the collision capsules (first at +0xd0)
    Bone();                        // 0x41b0c0
    // the real destructor 0x41a510 frees the capsule vector and destroys the name (implicit)
};

// One bone of a ragdoll model (0x50 bytes): a <Bone> element of skeleton_bodies.xml, the element of RagModel::bones
// (include/rag_model.h). The field names are the attributes load 0x4d3330 reads; mesh2/mesh_2.cpp fix_skeletons
// 0x4cd850 copies them onto the skeleton's Bone (+0x8c.., the same layout; Bone's own ctor 0x41b0c0 sets other defaults,
// so the record is not a member of Bone). The ctor and the two copy members are out of line in the original
// (mesh_2.cpp); the destructor is implicit (frees the bodies, then the name).
struct RagdollBone {
    tstring name;                  // +0x00
    float bone_mass;               // +0x10 (2.0)
    float cone_min;                // +0x14 (-0.01)
    float cone_max;                // +0x18 (1.0)
    float twist_limit;             // +0x1c (1.0)
    rgl::vector4 rotate;           // +0x20 rotate_side, rotate_forward, rotate_up
    rgl::vector4 socket_dir;       // +0x30
    std::vector<CapDef> bodies;    // +0x40 the collision capsules (<Bodies>)
    RagdollBone();                                  // 0x4cd7a0
    RagdollBone(const RagdollBone& o);              // 0x4cd080
    RagdollBone& operator=(const RagdollBone& o);   // 0x4cd270
    void load(TiXmlElement* e);                     // 0x4d3330 (options_panel/ragdoll_models.cpp)
};

#endif // RGL_CORE_BONE_H
