// Slice s00420960: the entity's clear() (virtual slot 19) and four neighbours of an unrelated block of
// scene-graph code: a grid clear (rows of cells holding a vector, a list and a vector), a first-child
// push helper, a camera-like record constructor and the clear of a slot array.
#include <vector>
#include <list>
#include "entity.h"

using namespace rgl;

namespace rgl {

// @ 0x420960
void entity::clear()
{
    for (unsigned i = 0; i < meshes.size(); i++) {
        delete meshes[i];
    }
    meshes.resize(0, 0);
    for (unsigned i = 0; i < lods.size(); i++) {
        delete lods[i];
    }
    lods.resize(0, 0);
}

// Provisional types of the grid owner (0x420a70): a rows x cols table of cells (0x34 bytes each) and two
// helper members cleared by out-of-line calls.
struct Cell {
    std::vector<Obj*> a;        // 0x00
    std::list<int> b;           // 0x10
    int f1c;
    std::vector<int> c;         // 0x20
    int f30;
};
struct SubA { char b[0x14]; void release_a(); };   // 0x41f350
struct SubB { char b[0x14]; void release_b(); };   // 0x41f3d0
struct grid {
    char pad0[8];
    int nrows;                  // 0x08
    int ncols;                  // 0x0c
    char pad10[0x28 - 0x10];
    Cell** rows;                // 0x28
    SubA sa;                    // 0x2c
    SubB sb;                    // 0x40
    std::vector<Obj*> objs;     // 0x54
    void clear();               // 0x420a70
};

typedef std::vector<int> int_vector;

// @ 0x420a70
void grid::clear()
{
    for (std::vector<Obj*>::iterator it = objs.begin(); it < objs.end(); ++it)
        delete *it;
    objs.resize(0, 0);
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            Cell* cell = &rows[i][j];
            cell->a.resize(0, 0);
            cell->b.resize(0, 0);
            rows[i][j].c.~int_vector();
        }
    }
    sa.release_a();
    sb.release_b();
}

// A node whose child-object list lives at +0xb0, and its owner's list of nodes (at +0x19c).
struct Node { char pad[0xb0]; std::vector<Obj*> objs; };
struct NodeOwner {
    char pad[0x19c];
    std::vector<Node*> nodes;   // 0x19c
    void add_to_first(Obj* o);  // 0x420e90
};

// @ 0x420e90
void NodeOwner::add_to_first(Obj* o)
{
    if (nodes.size() != 0)
        nodes[0]->objs.push_back(o);
}

// The clear() of an array of slots (0x1dc bytes each): per slot a vector of polymorphic objects at +0x19c and
// a vector of LOD records at +0x1b0; plus one vector of each kind in the owner (+0x20 and +0x30).
struct Slot {
    char pad[0x19c];
    std::vector<mesh*> polys;       // 0x19c
    int f1ac;
    std::vector<lod_mesh2*> lods;   // 0x1b0
    char tail[0x1dc - 0x1c0];
};
struct slotarr {
    int n;                          // 0x00
    Slot* slots;                    // 0x04
    char pad8[0x20 - 8];
    std::vector<mesh*> polys;       // 0x20
    std::vector<lod_mesh2*> lods;   // 0x30
    void clear();                   // 0x420ec0
};

// @ 0x420ec0
void slotarr::clear()
{
    for (int i = 0; i < n; i++) {
        for (std::vector<mesh*>::iterator it = slots[i].polys.begin(); it != slots[i].polys.end(); ++it)
            delete *it;
        slots[i].polys.resize(0, 0);
    }
    for (std::vector<mesh*>::iterator it = polys.begin(); it < polys.end(); ++it)
        delete *it;
    polys.resize(0, 0);
    for (int i = 0; i < n; i++) {
        for (std::vector<lod_mesh2*>::iterator it = slots[i].lods.begin(); it < slots[i].lods.end(); ++it)
            delete *it;
        slots[i].lods.resize(0, 0);
    }
    for (std::vector<lod_mesh2*>::iterator it = lods.begin(); it < lods.end(); ++it)
        delete *it;
    lods.resize(0, 0);
}

// A block of scene settings (0x1b8 bytes, provisional). init() re-initialises it field by field, then empties
// the vector of 100-byte records at +0x0c (the vector itself is constructed elsewhere).
struct E100 { int v[25]; };
struct uv2 {                        // two floats built through a stack temporary
    float u, v;
    uv2(float a, float b) : u(a), v(b) {}
};
struct settings {
    int f0, f4, f8;
    std::vector<E100> recs;         // 0x0c
    float f1c, f20, f24;
    uv2 uv[10];                     // 0x28: ten texture-coordinate pairs
    uv2 half;                       // 0x78
    uv2 quad[4];                    // 0x80
    int fa0;
    vector4 va4, vb4, vc4;          // 0xa4, 0xb4, 0xc4: x axis, z axis, gravity
    float fd4, fd8, fdc, fe0;
    int fe4, fe8, fec, ff0;
    mat4 mf4, m134;                 // 0xf4, 0x134
    vector4 v174;                   // 0x174
    vector4 v184;                   // 0x184
    float f194;
    int f198, f19c, f1a0, f1a4, f1a8;
    int f1ac, f1b0, f1b4;
    void init();                    // 0x420b40
};

// @ 0x420b40
void settings::init()
{
    mat4 t1;
    E100 e;
    f0 = 0;
    f4 = 0;
    ff0 = 0;
    v184 = vector4(0.0f, 0.0f, 1.0f);
    f194 = 1.0f;
    f198 = 0; f19c = 0; f1a0 = 0; f1a4 = 0;
    fec = 0;
    m134 = *identity(&t1);
    mf4 = *identity(&t1);
    v174 = vector4(0.0f, 0.0f, 0.0f);
    va4 = vector4(1.0f, 0.0f, 0.0f);
    vb4 = vector4(0.0f, 0.0f, 1.0f);
    fe8 = 1;
    half = uv2(0.5f, 0.5f);
    uv[0] = uv2(0.0f, 1.0f);
    uv[1] = uv2(1.0f, 0.0f);
    uv[2] = uv2(0.0f, 1.0f);
    uv[3] = uv2(1.0f, 1.0f);
    uv[4] = uv2(0.0f, 1.0f);
    uv[5] = uv2(1.0f, 1.0f);
    uv[6] = uv2(0.0f, 1.0f);
    uv[7] = uv2(1.0f, 1.0f);
    uv[8] = uv2(0.0f, 1.0f);
    uv[9] = uv2(1.0f, 1.0f);
    vc4 = vector4(0.0f, 0.0f, -9.8f);
    fd4 = 1.0f;
    quad[0] = uv2(1.0f, 1.0f);
    quad[1] = uv2(0.0f, 1.0f);
    quad[2] = uv2(0.0f, 0.0f);
    quad[3] = uv2(1.0f, 0.0f);
    f20 = 0.0f; f1c = 1.0f; f24 = 0.5f;
    f8 = 0;
    f1a8 = -1; f1ac = 0; f1b0 = 0; f1b4 = 0;
    fd8 = 1.0f; fdc = 1.0f; fe0 = 1.0f;
    recs.resize(0, e);
}

} // namespace rgl
