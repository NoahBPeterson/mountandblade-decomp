// fx.h: the effect definition ("skeleton") and its parts, ONE definition for src/rgl (scene/fx*.cpp, scene_objects.cpp,
// render_scene.cpp), src/game (entity.cpp, agent3/agent_fx_sound.cpp) and src/game/terrain_a (the envelope record).
//   FxElem (0x1dc bytes, vtable 0x54b454 = { 0x420e90 add_to_first, 0x41f250 set_data, 0x41f2a0 find_kid }: three virtual
//          methods, NO virtual destructor): one bone of the definition with its psys / lod lists.
//          Constructor 0x428d10 (src/rgl/scene/fx_support.cpp), destructor 0x41f1d0 and set_data / find_kid (src/game/entity.cpp).
//          The two lists at +0x19c / +0x1b0 hold rgl::mesh* / rgl::lod_mesh2* (src/rgl); agent_fx_sound.cpp (0x442390 /
//          0x4423f0) pushes the same pointers as ints (it casts the lists to std::vector<int>).
//   FxDef  (0x7c bytes): owns an array of FxElem, its own lists and the envelope records (EnvRec). Constructor 0x428820,
//          destructor 0x421680, init 0x4288c0, update 0x421df0, render 0x428ff0, skin 0x4291c0 (src/rgl/scene);
//          attach_psys / attach_group 0x442460 / 0x4424a0 (src/game/agent3/agent_fx_sound.cpp).
//   EnvRec (0xd8 bytes): one envelope record. alloc 0x428c60 (rgl), the inline constructor / destructor and reset 0x4686a0
//          (src/game/terrain_a/terrain_a.cpp, FxDef2::set_env_count 0x46b9d0 allocates the array; the vector deleting
//          destructor ??_E is 0x41ed70).
// Offsets cite the function that reads or writes them. Needs rgl::vector4 / rgl::matrix (rgl_base.h picks the module's own).
// The slices' names: Slot, NodeOwner, slotarr (FxElem, FxDef).
#ifndef INCLUDE_FX_H
#define INCLUDE_FX_H
#include <vector>
#include "./rgl_base.h"
#include "./mat4.h"

namespace rgl { struct mesh; struct lod_mesh2; struct psys; struct scene; }
struct Psys;                                                    // src/game/agent3/agent_fx_sound.h (a particle mesh as the game sees it)
struct PsysGroup;
extern __int64 g_ticks[];                                       // 0x5e2948 clock table, 1e-5 s ticks (indexed by a clock id)

// unit-length-ish rotation as w, x, y, z (stored x y z first in the original's arrays; see FxElem::q)
struct quat4 {
    float v[4];
    quat4() {}
    quat4(float w, float x, float y, float z) { v[1] = x; v[2] = y; v[3] = z; v[0] = w; }
};
void quat_from_matrix(quat4* out, const float* m);              // 0x428b10 (cdecl): trace / largest-diagonal branches

// one record of the loaded skeleton resource (0xdc bytes, FxRes::recs)
struct FxRec {
    char pad0[4];
    int parent;                 // 0x04 (-1 = none)
    char pad1[4];
    rgl::mat4 m;                // 0x0c local frame
    char pad2[0xdc - 0x4c];
};
// the resource object returned by resource_manager::get_object70 (0x418cf0)
struct FxRes {
    char pad[0x18];
    int count;                  // 0x18
    FxRec* recs;                // 0x1c
};

// std::vector<mesh*> whose push_back is the out-of-line instance of the mesh object (0x4119d0): skeleton_entity::load
// calls it, while the other users in these objects reach their own copy
struct mesh_ptr_vec : std::vector<rgl::mesh*> {
    void push_back(rgl::mesh* const& x);                        // 0x4119d0
};

struct FxElem {                                                 // 0x1dc bytes, vtable 0x54b454
    virtual void add_to_first(rgl::psys* o);                    // 0: 0x420e90 (first psys' emitter list)
    virtual void set_data(void* p);                             // 1: 0x41f250 (entity.cpp)
    virtual int find_kid(int id);                               // 2: 0x41f2a0 (entity.cpp)
    rgl::mat4 m4;               // 0x04 world frame (parent * local)       FxDef::init 0x4288c0
    rgl::mat4 m44;             // 0x44 local frame as loaded
    rgl::mat4 m84;             // 0x84 copy of the world frame (FxDef::render 0x428ff0)
    quat4 q;                    // 0xc4 rotation of the local frame (quat_from_matrix)
    char padd4[0xe4 - 0xd4];
    quat4 qe4;                  // 0xe4 (identity)   ctor 0x428d10
    rgl::vector4 vf4;           // 0xf4
    int t[2][2];                // 0x104 (-1000)
    int c[2];                   // 0x114
    quat4 qa[2][2];             // 0x11c
    quat4 qb[2][2];             // 0x15c
    mesh_ptr_vec psys;          // 0x19c particle meshes (first at 0x1a0; agent_fx_sound.cpp 0x442390 pushes Psys* as int)
    int f1ac;                   // 0x1ac (agent_fx_sound.h called it ngroups)
    std::vector<rgl::lod_mesh2*> groups;    // 0x1b0 (first at 0x1b4; agent_fx_sound.cpp 0x4423f0 pushes PsysGroup* as int)
    int parent;                 // 0x1c0 (-1 = none)       entity.cpp 0x41f1d0
    int nchild;                 // 0x1c4                   entity.cpp 0x41f1d0
    int* children;              // 0x1c8                   entity.cpp 0x41f1d0
    const FxRec* src;           // 0x1cc the loaded record
    float f1d0, f1d4;           // 0x1d0, 0x1d4 envelope blend
    int f1d8;                   // 0x1d8

    FxElem();                                                   // 0x428d10
    ~FxElem();                                                  // 0x41f1d0 (entity.cpp)
    void set_color(int c);                                      // 0x4219c0
    void add_psys(int p);                                       // 0x442390 (agent_fx_sound.cpp)
    void add_group(int g);                                      // 0x4423f0 (agent_fx_sound.cpp)
};

// One quad of four floats at EnvRec+0xc0 (zeroed member-wise, then x = 1; EnvRec::reset 0x4686a0)
struct Quad4 {
    float x, y, z, w;
    void zero() { x = 0; z = 0; y = 0; w = 0; }
};

// 0xd8-byte envelope record; the inline destructor frees three arrays (vector deleting dtor ??_E at 0x41ed70)
struct EnvRec {
    union {
        float cur;              // 0x00 (rgl: current value; FxDef::update 0x421df0)
        int f0;                 // 0x00 (terrain_a ctor: 0)
    };
    char pad4[4];
    int f8, fc;                 // 0x08 / 0x0c (ctor: -1)
    char pad10[0x10];
    int clock;                  // 0x20 (ctor: 0)
    char pad24[4];
    __int64 t0;                 // 0x28 (ctor: g_ticks[0])
    union {
        float target;           // 0x30
        int f30;
    };
    union {
        float release;          // 0x34
        float f34;
    };
    union {
        float attack;           // 0x38
        float f38;
    };
    int f3c;                    // 0x3c
    float f40;                  // 0x40
    char pad44[8];
    int f4c, f50, f54, f58, f5c;                                // 0x4c..
    rgl::vector4* k0; rgl::vector4* k1; rgl::vector4* k2;       // 0x60 / 0x64 / 0x68
    char pad6c[0x7c - 0x6c];
    rgl::vector4 v7c[2];        // 0x7c
    int f9c;                    // 0x9c
    float fa0;                  // 0xa0
    int fa4, fa8, fac;          // 0xa4..
    char fb0;                   // 0xb0
    char padb1[3];
    int fb4, fb8;               // 0xb4 / 0xb8
    float fbc;                  // 0xbc
    Quad4 q;                    // 0xc0
    int fd0;                    // 0xd0 (clock index copy)
    char padd4[4];
    EnvRec() : clock(0), t0(g_ticks[0]) {
        fd0 = 0;
        f8 = -1;
        fc = -1;
        f0 = 0;
        k0 = 0; k1 = 0; k2 = 0;
        reset();
    }
    ~EnvRec() { delete[] k0; delete[] k1; delete[] k2; }
    void reset();                                               // 0x4686a0 (terrain_a.cpp)
    void alloc(int n);                                          // 0x428c60
};

struct FxDef {                                                  // 0x7c bytes
    int count;                  // 0x00 elements
    FxElem* elems;              // 0x04
    FxRes* res;                 // 0x08
    rgl::vector4 v0c;           // 0x0c
    int f1c;                    // 0x1c
    std::vector<rgl::mesh*> psys;   // 0x20 (first at 0x24)
    std::vector<rgl::lod_mesh2*> groups;    // 0x30 (first at 0x34)
    EnvRec* env;                // 0x40
    int nenv;                   // 0x44
    int* ints;                  // 0x48
    int f4c;                    // 0x4c
    bool b50;                   // 0x50 pose valid
    float f54;                  // 0x54 elapsed time
    bool b58;                   // 0x58 frozen
    int f5c;                    // 0x5c
    int f60;                    // 0x60
    void* p64;                  // 0x64 second pose source
    char b68;                   // 0x68
    rgl::vector4 v6c;           // 0x6c

    FxDef(int a);                                               // 0x428820
    ~FxDef();                                                   // 0x421680
    void clear_defs();                                          // 0x420ec0
    void set_color(int c);                                      // 0x4218e0
    void update(float dt, const rgl::matrix* m);                // 0x421df0
    void init(FxRes* r);                                        // 0x4288c0
    void render(rgl::scene* sc, const rgl::matrix* frame, const float* dist);   // 0x428ff0
    void skin(rgl::mesh* p, char flag);                         // 0x4291c0
    void skin_group(rgl::lod_mesh2* g);                         // 0x4296f0
    void attach_psys(int idx, Psys* p, const rgl::mat4* m);     // 0x442460 (agent_fx_sound.cpp)
    void attach_group(int idx, PsysGroup* g);                   // 0x4424a0 (agent_fx_sound.cpp)
};

#endif // INCLUDE_FX_H
