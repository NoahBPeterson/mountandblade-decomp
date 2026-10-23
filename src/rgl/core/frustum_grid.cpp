// frustum_grid.cpp (0x41e470..0x41ef1a): the view frustum's plane test and plane builder, the world's cell-grid
// helpers (row table destructor, point -> cell index, entity link release), a record with three owned arrays and
// the std::list<item_t*> node allocator. Migrated from slices s0041e470 (all five rows) and s0041ee20 (its first
// two rows, 0x41ee20 and 0x41eeb0).
// Boundaries: content only (class change resmgr | frustum at 0x41e470, no call crosses it). This object may be
// the first part of the scene/world_items.cpp object (0x41ef30..): `world::release_links` belongs to the same class
// as `world::partition_items`, and the list<item_t*> instantiations are spread over 0x41eeb0..0x41f680 (the
// linker keeps one copy of a template instance, at the first object that needs it).
#include <math.h>
#include <new>
#include <list>
#include <vector>
#include "../scene/frustum.h"
#include "../scene/world.h"
#include "./frustum_grid.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using namespace rgl;

static inline vector4 sub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// @ 0x41e470
int plane::in_front(const vector4* p, float eps) const
{
    vector4 d;
    d = sub(*p, pos);
    float dist = d.x * normal.x + (d.z * normal.z + d.y * normal.y);
    if (dist < -eps) return 0;
    return 1;
}

static inline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static inline vector4 smul(const float& s, const vector4& a) { return vector4(s * a.x, s * a.y, s * a.z); }
static inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline vector4 vneg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }

// @ 0x41e4f0
// planes: top, bottom, right, left (the sides), far, near; the camera frame has s = right, f = up, u = backwards
void frustum::build_planes()
{
    nplanes = 6;
    if (perspective) {
        planes[0].pos = m.o;
        planes[1].pos = m.o;
        planes[2].pos = m.o;
        planes[3].pos = m.o;
        planes[0].normal = vsub(smul(nearz, vneg(m.f)), smul(top, m.u));
        planes[1].normal = vadd(smul(nearz, m.f), smul(bottom, m.u));
        planes[2].normal = vsub(vmul(vneg(m.s), nearz), vmul(m.u, right));
        planes[3].normal = vadd(vmul(m.s, nearz), smul(left, m.u));
        planes[0].normal.normalize();
        planes[1].normal.normalize();
        planes[2].normal.normalize();
        planes[3].normal.normalize();
        planes[4].pos = vsub(m.o, vmul(m.u, farz));
        planes[5].pos = vsub(m.o, smul(nearz, m.u));
        planes[4].normal = m.u;
        planes[5].normal = vneg(m.u);
    } else {
        planes[0].pos = vadd(smul(top, m.f), m.o);
        planes[1].pos = vadd(smul(bottom, m.f), m.o);
        planes[2].pos = vadd(smul(right, m.s), m.o);
        planes[3].pos = vadd(smul(left, m.s), m.o);
        planes[4].pos = vsub(m.o, vmul(m.u, farz));
        planes[5].pos = vsub(m.o, vmul(m.u, nearz));
        planes[0].normal = vneg(m.f);
        planes[1].normal = m.f;
        planes[2].normal = vneg(m.s);
        planes[3].normal = m.s;
        planes[4].normal = m.u;
        planes[5].normal = vneg(m.u);
    }
}

// @ 0x41ecc0
world_aux::~world_aux()
{
    cell_rows_view* t = (cell_rows_view*)this;
    if (t->rows) {
        for (int i = 0; i <= t->last; i++)
            delete[] t->rows[i];
        delete[] t->rows;
    }
}

// @ 0x41ed00
// Clamps a 2D point to cell indices (0 .. n-1).
void world::to_cell(const float* p, int* out)
{
    const grid_view* g = (const grid_view*)this;
    for (int i = 0; i < 2; i++) {
        float f = (p[i] - g->lo[i]) / g->step[i];
        if (f <= 0.0f)
            out[i] = 0;
        else if ((float)(g->n[i] - 1) <= f)
            out[i] = g->n[i] - 1;
        else
            out[i] = (int)f + 1;
    }
}

// @ 0x41ed70
buf3::~buf3() { delete[] a; delete[] b; delete[] c; }
buf3* make_buf3_array(int n) { return new buf3[n]; }

// @ 0x41ee20
// Takes the entity out of every cell list it is in and returns the links and the reference chain to the pools.
void world::release_links(entity* e)
{
    cellref* r = (cellref*)e->refs;
    cellref* first = r;
    for (; r != 0; r = r->next) {
        cellnode* l = r->node;
        if (l->prev != 0)
            l->prev->next = l->next;
        else
            rows[r->x][r->y].head = l->next;
        if (l->next != 0)
            l->next->prev = l->prev;
        l->next = nodes.freelist;
        nodes.freelist = l;
    }
    e->refs = 0;
    if (first != 0) {
        cellref* last = first;
        while (last->next != 0)
            last = last->next;
        last->next = refs.freelist;
        refs.freelist = first;
    }
}

// @ 0x41eeb0 (+ the catch funclet 0x41ef19): std::list<int>::_Buynode, shared by every pointer list (the
// original instance is list<item_t*>'s; game/entity.cpp instantiates the other members of list<int>)
template std::list<int>::_Nodeptr std::list<int>::_Buynode(std::list<int>::_Nodeptr, std::list<int>::_Nodeptr, const int&);
