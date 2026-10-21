// Slice s0041ffe0: scene entity (vtable 0x54b460) bounds recomputation and render, the LOD selector,
// aabb::extend_box, a list-backed object pool (two element sizes), vector<Obj*>::push_back and the
// clear() of an object group.
#include <vector>
#include <list>
#include "entity.h"

using namespace rgl;

// @ 0x420650
template void std::vector<Obj*>::push_back(Obj* const&);

namespace rgl {

// 0x40cd60 (three identity basis rows)
__declspec(noinline) mat3* identity3(mat3* out);
// 0x40cd00-style identity is not used here.

// identity_pos (0x40cd00) as inlined by its callers: the zero origin is built before the basis rows
static __forceinline mat4* ident4(mat4* out)
{
    vector4 o(0.0f, 0.0f, 0.0f);
    mat3 m;
    *(mat3*)out = *identity3(&m);
    out->o = o;
    return out;
}

// @ 0x41ffe0
void entity::recalc()
{
    box.reset();
    for (unsigned i = 0; i < meshes.size(); i++) {
        mesh** pm = &vat(meshes, i);
        if (*pm) {
            mat4 a;
            ident4(&a);
            a.s.x = a.s.x * scale.x;
            a.s.y = a.s.y * scale.x;
            a.s.z = a.s.z * scale.x;
            a.f.x = a.f.x * scale.y;
            a.f.y = a.f.y * scale.y;
            a.f.z = a.f.z * scale.y;
            a.u.x = a.u.x * scale.z;
            a.u.y = a.u.y * scale.z;
            a.u.z = a.u.z * scale.z;
            box.extend_box(&(*pm)->v22c, &(a * (*pm)->m_cc));
        }
    }
    box.update();
    fbc = box.radius;
    vc4 = box.center;
    vc4.x = vc4.x / scale.x;
    vc4.y = vc4.y / scale.y;
    vc4.z = vc4.z / scale.z;
}

// @ 0x4201b0
void aabb::extend_box(const aabb* b, const mat4* m)
{
    if (b->min.x > b->max.x)
        return;
    {
        float c[8][4];
        c[0][0] = b->min.x; c[0][1] = b->min.y; c[0][2] = b->min.z;
        c[1][0] = b->max.x; c[1][1] = b->min.y; c[1][2] = b->min.z;
        c[2][0] = b->min.x; c[2][1] = b->max.y; c[2][2] = b->min.z;
        c[3][0] = b->max.x; c[3][1] = b->max.y; c[3][2] = b->min.z;
        c[4][0] = b->min.x; c[4][1] = b->min.y; c[4][2] = b->max.z;
        c[5][0] = b->max.x; c[5][1] = b->min.y; c[5][2] = b->max.z;
        c[6][0] = b->min.x; c[6][1] = b->max.y; c[6][2] = b->max.z;
        c[7][0] = b->max.x; c[7][1] = b->max.y; c[7][2] = b->max.z;
        for (int i = 0; i < 8; i++) {
            float* q = c[i];
            vector4 p;
            p.x = q[1] * m->f.x + q[2] * m->u.x + q[0] * m->s.x + m->o.x;
            p.y = m->u.y * q[2] + q[0] * m->s.y + q[1] * m->f.y + m->o.y;
            float t = m->s.z * q[0] + q[1] * m->f.z;   // the temporary keeps the (x, y) pair summed first
            p.z = t + q[2] * m->u.z + m->o.z;
            extend(&p);
        }
    }
}

// @ 0x420310
void entity::render(scene* sc, render_info* info)
{
    if (f140) {
        for (unsigned i = 0; i < meshes.size(); i++) {
            if (meshes[i]) {
                if (!meshes[i]->res) {
                    mstr n("default");
                    meshes[i]->res = g_resmgr.lookup(n);
                }
                if (meshes[i]->res->mat->f20 & 0x100) {
                    if (meshes[i]->get70().size() > 0) {
                        info->valid = 1;
                        const vector4& p0 = (iter32(&*meshes[i]->get70().begin()) + 0)._Ptr->a;
                        info->depth = local.transform_point(meshes[i]->m_cc.transform_point(p0)).z;
                    }
                }
                meshes[i]->render(sc, local);
            }
        }
        for (unsigned i = 0; i < lods.size(); i++) {
            if (lods[i]) {
                lods[i]->select(&f144);
                if (lods[i]->count != 0 && lods[i]->lists[0].v.size() != 0 &&
                    (lods[i]->lists[0].v[0]->res->mat->f20 & 0x100) != 0) {
                    if (lods[i]->lists[0].v[0]->get70_().size() > 0) {
                        info->valid = 1;
                        info->depth = local.transform_point(lods[i]->m8c.transform_point(((v70_view&)lods[i]->lists[0].v[0]->get70_()).at(0).a)).z;
                    }
                }
                lods[i]->render(sc, local);
            }
        }
    }
}

// @ 0x4205b0
void lod_mesh2::select(const float* d)
{
    cur = 0;
    if (*d < 0.1f && count > 1) {
        cur = 1;
        if (*d < 0.05f && count > 2) {
            cur = 2;
            if (*d < 0.025f && count > 3) {
                cur = 3;
                if (*d < 0.0125f && count > 4)
                    cur = 4;
            }
        }
    }
}

// List-backed object pools: a std::list of malloc'd blocks plus a free list threaded through the cells of
// the newest block. (Two instantiations: 12-byte and 16-byte cells.)
struct Cell12 { Cell12* next; int a, b; };
struct Cell16 { Cell16* next; int a, b, c; };
template <class Cell> struct pool : std::list<Cell*> {
    int count;                 // cells per block
    Cell* freelist;
    void grow();
};
// @ 0x4206a0
template <> void pool<Cell12>::grow()
{
    Cell12* blk = new Cell12[count];
    push_back(blk);
    for (int i = 0; i < count - 1; i++)
        blk[i].next = &blk[i + 1];
    ((int*)blk)[count * 3 - 3] = 0;
    freelist = blk;
}
// @ 0x420720
template <> void pool<Cell16>::grow()
{
    Cell16* blk = new Cell16[count];
    push_back(blk);
    for (int i = 0; i < count - 1; i++)
        blk[i].next = &blk[i + 1];
    ((int*)blk)[count * 4 - 4] = 0;
    freelist = blk;
}

// An owner of scene objects (provisional): two lists of polymorphic objects, raw blocks, records.
struct Rec { char pad[0xc]; std::vector<int> v; };       // v at +0xc (first at +0x10)
struct objgrp {
    char pad0[0x78];
    int f78;
    std::vector<Obj*> v7c;      // 0x7c
    std::vector<Obj*> v8c;      // 0x8c
    int next_id;                // 0x9c
    char pada0[0xb8 - 0xa0];
    std::vector<mesh*> vb8;     // 0xb8: raw blocks
    std::vector<Poly*> vc8;     // 0xc8
    char pad_d8[0x32c - 0xc8 - sizeof(std::vector<Poly*>)];
    std::vector<Obj*> v32c;     // 0x32c: records (Rec*)
    int f33c;

    void add(Obj* o);           // 0x4207a0
    void clear();               // 0x4207e0
};

// @ 0x4207a0
void objgrp::add(Obj* o)
{
    if (o) {
        o->id = next_id;
        next_id++;
        v8c.push_back(o);
        o->owner = (int)this;
    }
}

// @ 0x4207e0
void objgrp::clear()
{
    for (std::vector<Obj*>::iterator it = v7c.begin(); it < v7c.end(); ++it)
        delete *it;
    v7c.resize(0, 0);
    for (std::vector<Obj*>::iterator it = v8c.begin(); it < v8c.end(); ++it)
        delete *it;
    v8c.resize(0, 0);
    for (unsigned i = 0; i < vb8.size(); i++)
        operator delete((void*)vb8[i]);
    vb8.resize(0, 0);
    for (unsigned i = 0; i < vc8.size(); i++) {
        Poly* p = vc8[i];
        if (p)
            delete p;
    }
    vc8.resize(0, 0);
    for (unsigned i = 0; i < v32c.size(); i++) {
        Rec* r = (Rec*)v32c[i];
        if (r)
            delete r;
    }
    v32c.resize(0, 0);
    f78 = 0;
    f33c = 0;
    next_id = 1;
}

} // namespace rgl
