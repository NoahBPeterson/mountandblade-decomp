// world_cells.cpp (0x423c00..0x424100): the world's collision grid cells. World::add_item (a shape or a composite of
// shapes goes into the grid cells it covers), the cell rectangle relocation, the cell's sorted shape vector and the
// std::vector<rgl::Obj*>::push_back / insert specializations shared by every pointer vector. Split from world_grid.cpp at
// the original's file boundary 0x424100: world::reg_shape 0x424170 reloads its local after push_back, i.e. its unit only
// declared the specialization (codegen.md 16, "a same-unit callee body can hide a translation-unit boundary").
#include <vector>
#include "../../include/vec_xlen.h"
#include <list>
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
    if (nlo.x == o->min_x && nlo.y == o->min_y && nhi.x == o->max_x && nhi.y == o->max_y)
        return;
    const int2* lo = (const int2*)&o->min_x;
    const int2* hi = (const int2*)&o->max_x;
    int2 mn, mx;
    { int2 t1; mn = *min2(&t1, &nlo, lo); }
    { int2 t2; mx = *max2(&t2, &nhi, hi); }
    int2 p;
    for (p.x = mn.x; p.x <= mx.x; p.x++) {
        for (p.y = mn.y; p.y <= mx.y; p.y++) {
            if ((char)in_rect(&p, lo, hi)) {
                if (!(char)in_rect(&p, &nlo, &nhi)) {
                    ((grid_cell**)cells)[p.x][p.y].remove(o);
                    shape_list::node* h = ((grid_cell**)cells)[p.x][p.y].objs.head;
                    shape_list::node* n = ((grid_cell**)cells)[p.x][p.y].objs.head->next;
                    while (n != h) {
                        if (n->obj == o)
                            break;
                        n = n->next;
                    }
                    if (n != h) {
                        n->prev->next = n->next;
                        n->next->prev = n->prev;
                        operator delete(n);
                        ((grid_cell**)cells)[p.x][p.y].objs.size--;
                    }
                }
            } else if ((char)in_rect(&p, &nlo, &nhi)) {
                ((grid_cell**)cells)[p.x][p.y].add(o);
            }
        }
    }
    *(int2*)&o->min_x = nlo;
    *(int2*)&o->max_x = nhi;
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

// std::vector<rgl::Obj*>::push_back / insert (0x423fc0 / 0x424010, shared by every pointer vector): the callers call them out of line; the
// specializations are declared before the callers and defined where the original has them
namespace std {
template<> __declspec(noinline) void vector<rgl::Obj*>::push_back(rgl::Obj* const& _X);
template<> __declspec(noinline) vector<rgl::Obj*>::iterator vector<rgl::Obj*>::insert(vector<rgl::Obj*>::iterator _P, rgl::Obj* const& _X);
}

// ---- the cell ----------------------------------------------------------------------------------------------------
// @ 0x423f10
void grid_cell::add(shape_base* x)
{
    unsigned pos = (unsigned)-1;
    unsigned i = 0;
    for (;;) {
        if (i >= size())
            break;
        if (first[i] > x)
            pos = i;
        i++;
        if (pos != (unsigned)-1)
            break;
    }
    if (pos == (unsigned)-1) {
        ((std::vector<rgl::Obj*>*)this)->push_back((rgl::Obj* const&)x);   // 0x423fc0
    } else {
        ((std::vector<rgl::Obj*>*)this)->resize(size() + 1, 0);   // 0x41fa00: the cell's shape vector is the std::vector<rgl::Obj*> instantiation of entity.cpp (identical-code folding)
        for (int j = size() - 1; j > (int)pos; j--)
            first[j] = first[j - 1];
        first[pos] = x;
    }
    shape_list* l = &objs;
    void (std::list<int>::*ins)(std::list<int>::iterator, const int&) = &std::list<int>::_Insert;   // called out of line, as the original does
    (((std::list<int>*)l)->*ins)(((std::list<int>*)l)->end(), (const int&)x);   // 0x41f640: entity.cpp's list<int>::_Insert, folded with every list of pointers
}

// @ 0x4240d0
__declspec(noinline) inline int shape_vec::find(shape_base* o) const
{
    for (unsigned i = 0; i < size(); i++) {
        if (first[i] == o)
            return i;
    }
    return -1;
}

// @ 0x424060
void shape_vec::remove(shape_base* o)
{
    int idx = find(o);
    if (idx >= 0) {
        unsigned i = idx;
        while (i < size() - 1) {
            shape_base** f = first;
            f[i] = f[i + 1];
            i++;
        }
        ((std::vector<rgl::Obj*>*)this)->resize(size() - 1, 0);   // 0x41fa00 (see grid_cell::add)
    }
}

// @ 0x423fc0 / 0x424010
namespace std {
template<> __declspec(noinline) vector<rgl::Obj*>::iterator vector<rgl::Obj*>::insert(vector<rgl::Obj*>::iterator _P, rgl::Obj* const& _X)
{
    size_type _Off = size() == 0 ? 0 : _P - begin();
    _Insert_n(_P, (size_type)1, _X);
    return (begin() + _Off);
}
template<> __declspec(noinline) void vector<rgl::Obj*>::push_back(rgl::Obj* const& _X)
{
    if (size() < capacity())
        _Mylast = _Ufill(_Mylast, 1, _X);
    else
        insert(end(), _X);
}
}
template void std::vector<rgl::Obj*>::push_back(rgl::Obj* const&);
template std::vector<rgl::Obj*>::iterator std::vector<rgl::Obj*>::insert(std::vector<rgl::Obj*>::iterator, rgl::Obj* const&);
