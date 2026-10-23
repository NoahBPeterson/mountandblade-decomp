// agent_stats.h: the classes only agent_stats.cpp (0x437830..0x43b100) defines or needs beyond agent_types.h: the
// translation table (StrTab), the collision-shape views the level queries walk, and the two PhysSet members
// of this object. Offsets are the ones read by a function of this module (the address is cited). Names provisional.
#ifndef AGENT2_AGENT_STATS_H
#define AGENT2_AGENT_STATS_H
#include "./agent_types.h"

// ---- the translation table (object at 0x5e2a18) ----------------------------------------------------------------
unsigned __stdcall hash_string(const tstring* s);               // 0x433bb0
struct StrBucket { std::vector<tstring> v; };                   // 16 bytes
struct StrTab {
    StrBucket keys[0xdf3];             // +0x0000 (0x43a1e0)
    StrBucket vals[0xdf3];             // +0xdf30
    tstring notfound;                  // +0x1be60
    tstring* lookup(const tstring& key);        // 0x43a1e0
    tstring* find(const tstring* key);          // 0x4c6010 (null when absent; later object)
};
extern StrTab g_strtable;              // 0x5e2a18

// ---- collision shapes: shape_base, sphere (0x40c030 copy), capsule (0x40a580, 0x433310), face (0x40ae70, contains_xy
// 0x4333c0), composite (0x433470) and contact are src/include/shape.h's. manifold is a view: build_composite 0x40cb60
// is all this object calls (the full class needs src/rgl/shapes/shapes.h's vec<T>; shape.h has the view).
#include "../../include/shape.h"
bool in_box(const vector4* lo, const vector4* hi, const vector4* p);   // 0x4336d0 (cdecl)

// a body resource: the shape at +0x18 is cloned by type (0x439100)
struct body_res {
    char pad0[0x18];
    shape_base* shape;                                  // +0x18
    shape_base* copy_shape();                           // 0x439100
};

// ---- physics set (src/game/agent/phys_set.h has the full class): the two members in this object -----------------
struct NewtonBody; struct NewtonWorld; struct NewtonCollision; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonTreeCollisionEndBuild(const NewtonCollision* c, int optimize);              // 0x54926c
__declspec(dllimport) NewtonBody* __cdecl NewtonCreateBody(const NewtonWorld* w, const NewtonCollision* c);          // 0x549270
__declspec(dllimport) void __cdecl NewtonReleaseCollision(const NewtonWorld* w, const NewtonCollision* c);           // 0x549274
__declspec(dllimport) void __cdecl NewtonBodySetMatrix(const NewtonBody* b, const float* m);                          // 0x549278
__declspec(dllimport) int __cdecl NewtonBodyGetSleepingState(const NewtonBody* b);                                    // 0x549290
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);                       // 0x549294
}
matrix* __cdecl identity_pos(matrix* out);              // 0x40cd00 (cdecl; writes the identity frame to *out and returns it)
#include "../../include/phys_set.h"      // PhysRec, PhysSet (none_sleeping 0x43a8a0, rebuild_static_body 0x43a900)

int __stdcall probe_shape_contact(shape_base* s, vector4* out_point, shape_base** out_shape, const char* flag);  // 0x43a720

#endif // AGENT2_AGENT_STATS_H
