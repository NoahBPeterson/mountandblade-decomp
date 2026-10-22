// fx.h: the effect definition ("skeleton") and its elements. FxElem (0x1dc bytes, vtable 0x54b454: three virtual
// methods, NO virtual destructor) is one bone of the definition with its psys / lod lists; FxDef (0x7c bytes) owns an
// array of them plus its own lists and the envelope records. Global classes, like src/game/entity.cpp's FxElem
// (whose destructor 0x41f1d0 and psys-list helpers 0x41f250 / 0x41f2a0 live there).
// Offsets cite the function that reads or writes them. The slices' names: Slot, NodeOwner, slotarr (FxElem, FxDef).
#ifndef SCENE_FX_H
#define SCENE_FX_H
#include <vector>
#include "../rgl_math.h"
#include "./scene_math.h"
#include "./scene.h"
#include "./emitter.h"

using rgl::vector4;

// unit-length-ish rotation as w, x, y, z (stored x y z first in the original's arrays; see FxElem::q)
struct quat4 {
    float v[4];
    quat4() {}
    quat4(float w, float x, float y, float z) { v[1] = x; v[2] = y; v[3] = z; v[0] = w; }
};
void quat_from_matrix(quat4* out, const float* m);              // 0x428b10 (cdecl): trace / largest-diagonal branches

// the 0x30-byte skinning record: four weights, four bone ids, the same ids as floats (mesh::v60 seen as skin data)
struct SkinVert {
    float w[4];                 // 0x00
    int bone[4];                // 0x10
    float bonef[4];             // 0x20
};
// pointer into the skinning records; operator+ stays out of line in FxDef::skin
struct skin_iter {
    SkinVert* cur;
    skin_iter() : cur(0) {}
    skin_iter(SkinVert* p) : cur(p) {}
    skin_iter operator+(int n) const;                           // 0x4296d0
};

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
    vector4 vf4;                // 0xf4
    int t[2][2];                // 0x104 (-1000)
    int c[2];                   // 0x114
    quat4 qa[2][2];             // 0x11c
    quat4 qb[2][2];             // 0x15c
    mesh_ptr_vec psys;          // 0x19c particle meshes (first at 0x1a0)
    int f1ac;                   // 0x1ac
    std::vector<rgl::lod_mesh2*> groups;    // 0x1b0
    int parent;                 // 0x1c0 (-1 = none)
    int nchild;                 // 0x1c4
    int* children;              // 0x1c8
    const FxRec* src;           // 0x1cc the loaded record
    float f1d0, f1d4;           // 0x1d0, 0x1d4 envelope blend
    int f1d8;                   // 0x1d8

    FxElem();                                                   // 0x428d10
    ~FxElem();                                                  // 0x41f1d0 (entity.cpp)
    void set_color(int c);                                      // 0x4219c0
};

// 0xd8-byte envelope record; the inline destructor frees three arrays (vector deleting dtor ??_E at 0x41ed70)
struct EnvRec {
    float cur;                  // 0x00
    char pad4[0x30 - 4];
    float target;               // 0x30
    float release;              // 0x34
    float attack;               // 0x38
    char pad3c[0x60 - 0x3c];
    vector4* k0; vector4* k1; vector4* k2;                      // 0x60 / 0x64 / 0x68
    char pad6c[0xd8 - 0x6c];
    ~EnvRec() { delete[] k0; delete[] k1; delete[] k2; }
    void alloc(int n);                                          // 0x428c60
};

struct FxDef {                                                  // 0x7c bytes
    int count;                  // 0x00 elements
    FxElem* elems;              // 0x04
    FxRes* res;                 // 0x08
    vector4 v0c;                // 0x0c
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
    vector4 v6c;                // 0x6c

    FxDef(int a);                                               // 0x428820
    ~FxDef();                                                   // 0x421680
    void clear_defs();                                          // 0x420ec0
    void set_color(int c);                                      // 0x4218e0
    void update(float dt, const rgl::matrix* m);                // 0x421df0
    void init(FxRes* r);                                        // 0x4288c0
    void render(rgl::scene* sc, const rgl::matrix* frame, const float* dist);   // 0x428ff0
    void skin(rgl::mesh* p, char flag);                         // 0x4291c0
    void skin_group(rgl::lod_mesh2* g);                         // 0x4296f0
};

#endif
