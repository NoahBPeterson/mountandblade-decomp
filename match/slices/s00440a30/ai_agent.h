// Shared declarations of slices s00440070 / s00440a30 (agent-vs-agent tests and the animation-id picker; all
// names provisional, every offset comes from the member functions at 0x440070..0x441360). Copied into each
// slice dir under the same name; keep identical.
#ifndef MB_AI_AGENT_H
#define MB_AI_AGENT_H
#include <stddef.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- collision shapes (layouts verified in s0040c670 / s0041bf40 / s004333c0; copied from s00439100.h) ----------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                            // slot 0
    virtual int shape_type();                           // slot 1
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9();
    virtual vector4* get_min(vector4* out);             // slot 10 (+0x28)
    virtual vector4* get_max(vector4* out);             // slot 11 (+0x2c)
};
struct sphere_shape : shape_base {                      // vtable 0x54a4f0, size 0x44
    vector4 v20, v30; int m40;
};
struct capsule : shape_base {                           // vtable 0x54a540, size 0x64
    vector4 p0, p1, p2, p3; float radius;
    capsule(float r, const vector4* a, const vector4* b);   // 0x433310
};
int shape_contact_count(shape_base* a, shape_base* b, int max, void* out);   // 0x40bfe0 (cdecl)

// level: the scene (first field of the mission object)
struct level {
    int trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60
};
extern shape_base* g_hit_shape;                         // 0x5e246c: last shape hit by trace_ray

// ---- positions ---------------------------------------------------------------------------------------------
struct pos : matrix {
    vector4 transform_point(const vector4& p) const;    // 0x408460
};

// ---- animator (see s0043edf0) ---------------------------------------------------------------------------------
struct Skeleton {
    int nbones;                                         // +0
    char* recs;                                         // +4 (0x1dc-byte records)
};
struct AgentBody {
    char pad0[0x58];
    pos m;                                              // +0x58
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                     // +0x304
};

// ---- the mission object ------------------------------------------------------------------------------------------
struct Agent;
struct AgentTable {                                     // mission+4 (see s0043ac40)
    int count;                                          // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
};
struct Mission {
    level* scene;                                       // +0
    AgentTable agents;                                  // +4
    char pad28[0x53c8 - 0x28];
    int relation[1];                                    // +0x53c8, stride 0x16c per team (1 = friend, -1 = enemy)
    bool friends(int a, int b) const
    {
        if (a >= 0 && a < 8 && b >= 0 && b < 8)
            return relation[a * 0x5b + b] == 1;
        return false;
    }
};
extern Mission* g_mission;                              // 0x5e292c

struct RangePair { float lo, hi; };
extern RangePair g_range_tbl[];                         // 0x590aac (indexed by Agent::f56c4)
float __cdecl blend_target(void* a, void* b);           // 0x4381e0
float __cdecl accel_factor(const float* a, const float* b, const float* c);   // 0x435ae0

struct Agent {                                          // 0x5928 bytes
    char pad0[8];
    int id;                                             // +8
    char padc[4];
    int f10;                                            // +0x10
    int f14;                                            // +0x14
    char pad18[0x2c - 0x18];
    int f2c;                                            // +0x2c
    char pad30[0x50 - 0x30];
    vector4 pos;                                        // +0x50
    char blk60[0x1c4 - 0x60];                           // +0x60 (blend_target arg)
    int team;                                           // +0x1c4
    char pad1c8[0x250 - 0x1c8];
    AgentBody* body;                                    // +0x250
    char pad254[0x294 - 0x254];
    char blk294[0x56b4 - 0x294];                        // +0x294 (blend_target arg)
    vector4 v56b4;                                      // +0x56b4
    int f56c4;                                          // +0x56c4
    char pad56c8[0x5928 - 0x56c8];

    void aim_point_and_dir(vector4* pos_out, vector4* dir_out);     // 0x43f560
    float duration();                                   // 0x438360
    bool line_of_sight(unsigned id, shape_base* ignore);            // 0x440e30
    int fire_check(unsigned id, int* out_blocker);      // 0x441020
};
#endif
