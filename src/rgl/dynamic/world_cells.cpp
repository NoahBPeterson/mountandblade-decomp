// world_cells.cpp (0x423c00..0x424100): the world's collision grid cells. World::add_item (a shape or a composite of
// shapes goes into the grid cells it covers), the cell rectangle relocation, the cell's sorted shape vector and the
// std::vector<rgl::Obj*>::push_back / insert specializations shared by every pointer vector. Split from world_grid.cpp at
// the original's file boundary 0x424100: world::reg_shape 0x424170 reloads its local after push_back, i.e. its unit only
// declared the specialization (codegen.md 16, "a same-unit callee body can hide a translation-unit boundary").
#include <vector>
#include "../../include/vec_xlen.h"
#include <list>
#include <algorithm>
#include <new>
#include <memory>
#include "./world_grid.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using rgl::vector4;
namespace rgl { struct Obj; }        // entity.cpp: the element type of the std::vector<Obj*> instantiation whose resize 0x41fa00 the cell vectors call

// ---- World::add_item / relocate ------------------------------------------------------------------------------------
// @ 0x423c00
void rgl::world::add_item(shape_base* e)
{
    shape_base* o = e;
    if (o->shape_type() == 3) {
        composite* c = static_cast<composite*>(o);
        for (int i = 0; i < c->count; i++) {
            if (c->bodies[i]->shape_type() == 3)
                add_item(c->bodies[i]);
            else
                relocate(c->bodies[i]);
        }
    } else if (o->shape_type() != 4) {
        relocate(o);
    }
}

// Body the original's object saw (row owner rgl/core/frustum_grid.cpp 0x41ed00): with it visible lo/hi do not escape,
// the same body the other grid callers see (insert_obj, add_shape)
inline __declspec(noinline) void rgl::world::to_cell(const float* p, int* out)
{
    for (int i = 0; i < 2; i++) {
        float f = (p[i] - lo[i]) / cell_size[i];
        if (f <= 0.0f)
            out[i] = 0;
        else if ((float)(n[i] - 1) <= f)
            out[i] = n[i] - 1;
        else
            out[i] = (int)f + 1;
    }
}

// @ 0x423c80
void rgl::world::relocate(shape_base* o)
{
    int2 nlo, nhi;
    vector4 tmp;
    to_cell((const float*)o->get_min(&tmp), &nlo.x);
    to_cell((const float*)o->get_max(&tmp), &nhi.x);
    if (nlo.x == o->cell_min.x && nlo.y == o->cell_min.y && nhi.x == o->cell_max.x && nhi.y == o->cell_max.y)
        return;
    int2* lo = &o->cell_min;
    int2 mn, mx;
    { int2 t1; mn = *min2(&t1, &nlo, lo); }
    int2* hi = &o->cell_max;
    { int2 t2; mx = *max2(&t2, &nhi, hi); }
    int2 p;
    for (p.x = mn.x; p.x <= mx.x; p.x++) {
        for (p.y = mn.y; p.y <= mx.y; p.y++) {
            if ((char)in_rect(&p, lo, hi)) {
                if (!(char)in_rect(&p, &nlo, &nhi)) {
                    cells[p.x][p.y].remove(o);
                    std::list<shape_base*>& objs = cells[p.x][p.y].objs;
                    std::list<shape_base*>::iterator it = std::find(objs.begin(), objs.end(), o);
                    if (it != objs.end())
                        objs.erase(it);
                }
            } else if ((char)in_rect(&p, &nlo, &nhi)) {
                cells[p.x][p.y].add(o);
            }
        }
    }
    o->cell_min = nlo;
    o->cell_max = nhi;
}

// @ 0x423e70
__declspec(noinline) int2* min2(int2* out, const int2* a, const int2* b)
{
    int x = a->x < b->x ? a->x : b->x;
    int y = a->y < b->y ? a->y : b->y;
    out->x = x;
    out->y = y;
    return out;
}
// @ 0x423ea0
__declspec(noinline) int2* max2(int2* out, const int2* a, const int2* b)
{
    int x = a->x > b->x ? a->x : b->x;
    int y = a->y > b->y ? a->y : b->y;
    out->x = x;
    out->y = y;
    return out;
}

// @ 0x423ed0
__declspec(noinline) int rgl::world::in_rect(const int2* p, const int2* lo, const int2* hi)
{
    if (p->x >= lo->x && p->x <= hi->x && p->y >= lo->y && p->y <= hi->y)
        return 1;
    return 0;
}

// std::vector<rgl::Obj*>::push_back / insert (0x423fc0 / 0x424010, shared by every pointer vector): instantiated where the original has
// them (the end of this file); world_grid.cpp declares them extern, so its callers call them out of line (cl 13.10 refuses both in one unit)

// ---- the cell ----------------------------------------------------------------------------------------------------
// The cell's vector and list are instances the linker folded onto the shared pointer ones (docs/codegen.md 14.1):
// vector<shape_base*>::push_back = vector<rgl::Obj*>'s 0x423fc0 (below), resize = vector<rgl::Obj*>'s 0x41fa00
// (game/entity.cpp), list<shape_base*>::_Insert = list<int>'s 0x41f640 (game/entity.cpp). The original calls them out of line.
namespace std {
template<> __declspec(noinline) void vector<shape_base*>::push_back(shape_base* const& _X);                   // 0x423fc0
template<> void vector<shape_base*>::resize(vector<shape_base*>::size_type _N, shape_base* _X);               // 0x41fa00
template<> void list<shape_base*>::_Insert(list<shape_base*>::iterator _P, shape_base* const& _X);            // 0x41f640
}

// @ 0x423f10
void rgl::cell::add(shape_base* x)
{
    unsigned pos = (unsigned)-1;
    unsigned i = 0;
    for (;;) {
        if (i >= a.size())
            break;
        if (a[i] > x)
            pos = i;
        i++;
        if (pos != (unsigned)-1)
            break;
    }
    if (pos == (unsigned)-1) {
        a.push_back(x);
    } else {
        a.resize(a.size() + 1, 0);
        for (int j = a.size() - 1; j > (int)pos; j--)
            a[j] = a[j - 1];
        a[pos] = x;
    }
    objs.push_back(x);
}

// @ 0x4240d0
__declspec(noinline) inline int rgl::cell::find(shape_base* o) const
{
    for (unsigned i = 0; i < a.size(); i++) {
        if (a[i] == o)
            return i;
    }
    return -1;
}

// @ 0x424060
void rgl::cell::remove(shape_base* o)
{
    int idx = find(o);
    if (idx >= 0) {
        unsigned i = idx;
        while (i < a.size() - 1) {
            a[i] = a[i + 1];
            i++;
        }
        a.resize(a.size() - 1, 0);
    }
}

// @ 0x423fc0 / 0x424010
template void std::vector<rgl::Obj*>::push_back(rgl::Obj* const&);
template std::vector<rgl::Obj*>::iterator std::vector<rgl::Obj*>::insert(std::vector<rgl::Obj*>::iterator, rgl::Obj* const&);
