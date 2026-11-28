// world_cells_tail.cpp (0x423e70..0x424100): the rectangle helpers min2 / max2 / world::in_rect and the cell's sorted shape vector, split from
// world_cells.cpp (add_item, relocate: 0x423c00..0x423e70). World::relocate (0x423c80) only matches when cl 13.10 sees min2, max2 and
// in_rect as declarations: with their bodies visible it takes the cell rectangle through different temporaries (frame 0x4c, not 0x44) and
// keeps registers differently, so the original compiled them apart (docs/codegen.md 16, "a same-unit callee body can hide a translation-unit
// boundary"; the same split as collision_vminmax.cpp).
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
// (engine/scene/fx_element_pool.cpp), list<shape_base*>::_Insert = list<int>'s 0x41f640 (engine/scene/fx_element_pool.cpp). The original calls them out of line.
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

// @ 0x424010
// insert is the stock body spelled with the public insert(where, 1, x), out of line (cl 13.10 would inline the stock one into push_back)
namespace std {
template<> __declspec(noinline) vector<rgl::Obj*>::iterator vector<rgl::Obj*>::insert(vector<rgl::Obj*>::iterator _P, rgl::Obj* const& _X)
{
    size_type _Off = size() == 0 ? 0 : _P - begin();
    insert(_P, (size_type)1, _X);
    return (begin() + _Off);
}
}
// @ 0x423fc0 / 0x424010
template void std::vector<rgl::Obj*>::push_back(rgl::Obj* const&);
template std::vector<rgl::Obj*>::iterator std::vector<rgl::Obj*>::insert(std::vector<rgl::Obj*>::iterator, rgl::Obj* const&);
