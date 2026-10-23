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

// ---- collision shapes (src/rgl/shapes/shapes.h has the full classes; it cannot be included together with rgl.h):
// only the views the level queries need. No constructor/destructor is defined here, so no vtable is emitted.
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base() {}                            // slot 0 (inline and empty: the local capsule of 0x439d60 has no destructor call)
    virtual int shape_type();                           // slot 1 (0 sphere, 1 capsule, 2 face, 3 composite, 4 manifold)
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9();
    virtual vector4* get_min(vector4* out);             // slot 10 (+0x28)
    virtual vector4* get_max(vector4* out);             // slot 11 (+0x2c)
};
struct sphere_shape : shape_base {                      // vtable 0x54a4f0, size 0x44
    vector4 v20, v30; int m40;
    sphere_shape(const sphere_shape& o);                // 0x40c030
};
struct capsule : shape_base {                           // vtable 0x54a540, size 0x64
    vector4 p0, p1, p2, p3; float radius;
    capsule(const capsule& o);                          // 0x40a580
    capsule(float r, const vector4* a, const vector4* b);   // 0x433310
};
struct face_shape : shape_base {                        // vtable 0x54a588, size 0xb4
    vector4 v[8];                                       // +0x20 polygon vertices
    vector4 normal;                                     // +0xa0
    int count;                                          // +0xb0
    face_shape(const face_shape& o);                    // 0x40ae70
    bool contains_xy(const float* p) const;             // 0x4333c0
};
struct composite : shape_base {                         // vtable 0x54a63c, size 0x28
    shape_base** bodies; int count;
    composite(const composite& o);                      // 0x433470
};
struct manifold : shape_base {                          // vtable 0x54a5f4, size 0x50
    char pad[0x30];
    void* build_composite();                            // 0x40cb60
};
struct contact { vector4 a; vector4 point; int extra; };    // 0x24 bytes; point at +0x10 is copied out (0x43a720)
bool in_box(const vector4* lo, const vector4* hi, const vector4* p);   // 0x4336d0 (cdecl)
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, contact* out);   // 0x40bfe0 (cdecl)

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
struct PhysRec {                                        // 0x38 bytes
    int f0;
    const NewtonWorld* world;                           // +4
    int f8;
    std::vector<const NewtonRagDollBone*> bones;        // +0xc (first at +0x10, last at +0x14)
    char pad18[0x2c - 0x1c];
    const NewtonCollision* collision;                   // +0x2c
    const NewtonBody* body;                             // +0x30
    int f34;
};
struct PhysSet {
    std::vector<PhysRec> recs;                          // +0 (first at +4)
    bool none_sleeping(int idx);                        // 0x43a8a0
    void rebuild_static_body(int idx);                  // 0x43a900
};

int __stdcall probe_shape_contact(shape_base* s, vector4* out_point, shape_base** out_shape, const char* flag);  // 0x43a720

#endif // AGENT2_AGENT_STATS_H
