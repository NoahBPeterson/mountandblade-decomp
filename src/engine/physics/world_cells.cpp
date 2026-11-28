// world_cells.cpp (0x423c00..0x424100): the world's collision grid cells. World::add_item (a shape or a composite of
// shapes goes into the grid cells it covers), the cell rectangle relocation, the cell's sorted shape vector and the
// std::vector<rgl::Obj*>::push_back / insert specializations shared by every pointer vector. Split from world_grid.cpp at
// the original's file boundary 0x424100: world::reg_shape 0x424170 reloads its local after push_back, i.e. its unit only
// declared the specialization (codegen.md 16, "a same-unit callee body can hide a translation-unit boundary").
#include <vector>
#include "engine/core/vec_xlen.h"
#include <list>
#include <algorithm>
#include <new>
#include <memory>
#include "engine/physics/world_grid.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/scene/frustum.h"
#include "engine/fx/keyframes.h"
#include "engine/math/mat4_include.h"
#include "engine/render/mesh_prims.h"
#include "engine/math/vec.h"
#include "engine/render/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/render/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

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

// Body the original's object saw (row owner engine/scene/frustum_grid.cpp 0x41ed00): with it visible lo/hi do not escape,
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
    mx = *max2((int2*)&tmp, &nhi, hi);       // out aliases the dead bounding-box scratch (no frame slot of its own: 0x44)
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
