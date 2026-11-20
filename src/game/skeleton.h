// skeleton.h: the display-mesh node lists (node_list / node_groups = the lod mesh the agent's quiver and holster use).
// The animation controller at the body's +0x304 is FxDef (include/fx.h: its bone records FxElem, its envelope records
// EnvRec); the body is rgl::strategic_entity (rgl/scene/scene_entity.h).
#ifndef GAME_SKELETON_H
#define GAME_SKELETON_H
#include <vector>
#include "../rgl/rgl_math.h"   // first: see agent4/agent_logic.h
#include "../include/rgl_base.h"
#include "./agent/item_kind.h"        // pos2
#include "./mat4.h"                   // rgl::mat4

struct shape_base;
struct node_groups;

struct NewtonRagDollBone;
namespace rgl { struct mesh; }

// ---- the display mesh's node lists (0x438f90, 0x439000) ----------------------------------------------------------
struct node_list : std::vector<rgl::mesh*> {          // rgl/core/resmgr.h's mesh_owner without the owning destructor
    void cap_all(const float* limit);                   // 0x438f90
};
struct node_groups {                                    // the lod mesh (rgl::lod_mesh); agent3 calls it Mesh
    char pad0[0x34];
    node_list lists[5];                                 // +0x34 (16 bytes each)
    int nlists;                                         // +0x84
    char pad88[4];
    pos2 frame;                                         // +0x8c
    unsigned vis;                                       // +0xcc visibility mask (0xffff shown, 0 hidden; 0x43a800, 0x4379c0)
    void cap_all(const float* limit);                   // 0x439000
};

#endif // GAME_SKELETON_H
