// skeleton.h: the animation controller (Skeleton, at AgentBody+0x304), its 0x1dc-byte bone records (Region) and the
// display-mesh node lists (node_list / node_groups = the lod mesh the agent's quiver and holster use). One definition
// for src/game/agent (camera_params 0x435c10), agent2 (agent_stats.cpp 0x438d80.. / 0x439ec0.., agent_ai.cpp) and
// agent3 (agent_combat.cpp 0x43e740.., agent_fx_sound.cpp 0x443aa0 / 0x444370).
// The record array and the +8 pointer are typed differently by the three modules, so they share anonymous unions of
// pointers (recs: raw bytes in agent3 / Region in agent2 / BoneRec in agent; +8: bone-mask table in agent2 / SkelData in
// agent3). Channel, SkelData, Anim and mnode are declared by the modules that use them (only pointers are stored here).
#ifndef GAME_SKELETON_H
#define GAME_SKELETON_H
#include <vector>
#include "../include/rgl_base.h"
#include "./agent/item_kind.h"        // pos2

struct shape_base;
struct Channel;
struct SkelData;
struct Anim;
struct mnode;

// raw view of a skeleton record, as Agent::camera_params reads it (0x435c10)
struct BoneRec {
    char pad0[0x46c];
    float f46c, f470, f474;
    char pad478[0xa00 - 0x478];
    float fa00, fa04, fa08;
    char pada0c[0x1170 - 0xa0c];
    rgl::vector4 v1170;
};

struct Region {                                         // 0x1dc bytes: one bone record (Skeleton::recs[i])
    char pad0[0x1ac];
    int count;                                          // 0x1ac (0x443aa0)
    std::vector<shape_base*> shapes;                    // 0x1b0 (0x439ec0, 0x43a130; 0x443aa0 pushes display meshes here as ints)
    char pad1c0[0x1d0 - 0x1c0];
    int f1d0;                                           // 0x1d0 (cleared by 0x43d040)
    char pad1d4[0x1dc - 0x1d4];
};

struct Skeleton {
    int nbones;                                         // +0
    union {
        char* recs;                                     // +4 raw 0x1dc-byte records (agent3: matrix at +0x84; +0x1140 aim bone, +0x1170 head)
        Region* regs;                                   //    the same array as Region (agent2)
        BoneRec* bones;                                 //    camera_params view (agent)
    };
    union {
        int* sel;                                       // +8 -> ints at +0x28/+0x2c/+0x30 (bone-mask selectors, agent2)
        SkelData* data;                                 //    (0x43f6d0, agent3)
    };
    char pad0c[0x30 - 0xc];
    std::vector<shape_base*> shapes;                    // +0x30 (0x439ec0)
    Channel* chans;                                     // +0x40 (0x4406a0)
    char pad44[0x50 - 0x44];
    char done;                                          // +0x50 (0x43e740, 0x43f260, 0x43f560, 0x43f6d0)
    char pad51[0x64 - 0x51];
    int has_poses;                                      // +0x64
    void update();                                      // 0x422130
    void update_poses();                                // 0x422080
    void ensure_updated();                              // 0x43e740
    char* rec(int i);                                   // 0x43f260 (animator record i, brought up to date first; matrix at +0x84)
    void remove_shape(shape_base* s);                   // 0x439ec0 (0x443aa0 passes a display mesh)
    void start_anim(int idx, Anim* anim, int a, int b); // 0x439fc0 (thiscall, ret 0x10)
    void all_shapes(shape_base** out, int* count);      // 0x43a130
    void reset_rec(int i);                              // 0x4339d0
    float rec_phase(int i);                             // 0x433ac0
    void set_rec_phase(int i, float p);                 // 0x433b20 (0x444370)
};

// ---- the display mesh's node lists (0x438f90, 0x439000) ----------------------------------------------------------
struct node_list {
    int al; mnode** first; mnode** last; mnode** end;
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
