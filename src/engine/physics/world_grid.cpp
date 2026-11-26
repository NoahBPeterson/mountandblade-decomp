// world_grid.cpp (0x424100..0x425370): the shape registration of an entity, the ground geometry's vertex / face-group
// store with its std::vector instantiations (VC7.1 <vector> for the 0x24-byte vertex record and the 0x88-byte group record),
// add_tree / add_shape. Migrated from slices s00423540 (0x423c00..0x423ea0), s00423ed0, s00424830 and s004251c0 (its
// first three functions and the _eh part). 0x423c00..0x424100 (World::add_item .. shape_vec::find and the
// std::vector<rgl::Obj*>::push_back / insert specializations) is the previous object, world_cells.cpp: reg_shape sees
// only the specialization's declaration (it reloads its local after the call, as the original).
// Boundary: see MIGRATION.md.
#include <vector>
#include "engine/core/vec_xlen.h"
#include <list>
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

// std::vector<rgl::Obj*>::push_back / insert (0x423fc0 / 0x424010, shared by every pointer vector): defined in world_cells.cpp,
// only declared here (the original's reg_shape does not see the body)
namespace std {
extern template void vector<rgl::Obj*>::push_back(rgl::Obj* const& _X);
extern template vector<rgl::Obj*>::iterator vector<rgl::Obj*>::insert(vector<rgl::Obj*>::iterator _P, rgl::Obj* const& _X);
}


// ---- entity::add_shape / register the shape with the world ---------------------------------------------------------
// @ 0x424100
void rgl::strategic_entity::add_shape(shape_base* o)
{
    items.push_back(o);
    o->set_mc(id);
    owner->reg_shape(o);
}

// @ 0x424170
void rgl::world::reg_shape(shape_base* o)
{
    if (o->shape_type() == 2) {
        o->m4 = ground->add(((face*)o)->count, ((face*)o)->w);
    } else if (o->shape_type() == 3) {
        composite* c = (composite*)o;
        for (int i = 0; i < c->count; i++) {
            if (c->bodies[i]->shape_type() == 2) {
                face* k = (face*)c->bodies[i];
                c->bodies[i]->m4 = ground->add(k->count, k->w);
            }
        }
    } else {
        o->shape_type();
    }
    rgl::Obj* t = (rgl::Obj*)o;
    objs.push_back(t);   // 0x423fc0
    add_tree((shape_base*)t);
}

// ---- the ground geometry: NavData (game/terrain/nav_data.h) ------------------------------------------------------
// @ 0x424220
int NavData::add(int n, const vector4* pts)
{
    unsigned base = verts.size();
    {
        NavVert t;
        verts.resize(base + n, t);
    }
    vector4 nrm = face_normal(pts, pts + 1, pts + 2);
    vector4 sum;
    sum.x = 0.0f; sum.y = 0.0f; sum.z = 0.0f;
    for (int i = 0; i < n; i++) {
        verts[base + i].p = pts[i];
        verts[base + i].n = nrm;
        verts[base + i].level = 0.0f;
        sum.x += pts[i].x;
        sum.y += pts[i].y;
        sum.z += pts[i].z;
    }
    unsigned idx = cells.size();
    NavCell g;
    cells.resize(idx + 1, g);
    cells[idx].npts = n;
    cells[idx].pos4() = sum;
    cells[idx].mask() = 0;
    cells[idx].f68 = 0;
    for (int k = 0; k < n; k++) {
        cells[idx].targets()[k] = -1;
        cells[idx].verts()[k] = base + k;
        std::vector<NavCell>::iterator it = cells.begin() + idx;
        it->dists()[k] = 0.0f;
    }
    return idx;
}

// @ 0x424650: unit normal of the triangle (a, b, c); returns the vector4 by value (hidden pointer)
vector4 face_normal(const vector4* a, const vector4* b, const vector4* c)
{
    float bx = b->x - a->x;
    float by = b->y - a->y;
    float bz = b->z - a->z;
    vector4 d;
    d.x = c->x - a->x;
    d.y = c->y - a->y;
    d.z = c->z - a->z;
    vector4 n;
    n.x = d.z * by - d.y * bz;
    n.y = bz * d.x - d.z * bx;
    n.z = d.y * bx - d.x * by;
    n.normalize();
    return n;
}

// (0x4246f0 is NavCell's inline constructor, game/terrain/nav_data.h, emitted out of line for add and read_map)

// ---- add_tree / add_shape -------------------------------------------------------------------------------------
// @ 0x425240
void rgl::world::add_tree(shape_base* s)
{
    if (s->shape_type() == 3) {
        composite* c = (composite*)s;
        for (int i = 0; i < c->count; i++) {
            if (c->bodies[i]->shape_type() == 3)
                add_tree(c->bodies[i]);
            else
                add_shape(c->bodies[i]);
        }
    } else if (s->shape_type() != 4) {
        add_shape(s);
    }
}

// Body the original's object saw (row owner engine/scene/frustum_grid.cpp 0x41ed00): with it visible lo/hi do not escape,
// so the corner cells are not reloaded around the add calls
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

// @ 0x4252c0
void rgl::world::add_shape(shape_base* s)
{
    int2 lo, hi;
    { vector4 t; to_cell((const float*)s->get_min(&t), &lo.x); }
    { vector4 t; to_cell((const float*)s->get_max(&t), &hi.x); }
    int2 p;
    for (p.x = lo.x; p.x <= hi.x; p.x++)
        for (p.y = lo.y; p.y <= hi.y; p.y++)
            cells[p.x][p.y].add(s);
    s->cell_min = lo;
    s->cell_max = hi;
}

// ---- the std::vector members of the ground geometry's vertex (NavVert, 0x24 bytes) and cell (NavCell, 0x88 bytes)
// vectors that the original's object holds. cl 13.10.3052's stock <vector> leaves allocator::construct and the
// _Ucopy / _Ufill wrappers out of line and stops inlining std::copy for elements of >= 128 bytes where the original
// inlined them, so the _Insert_n / erase / _Uninit_* rows are not byte-exact (the real classes are kept: user ranking).
// std::copy for the 0x88-byte cells, spelled out: cl 13.10 does not inline the stock copy for elements of >= 128 bytes
__forceinline NavCell* copy_nonscalar(NavCell* f, NavCell* l, NavCell* d) { for (; f != l; ++d, ++f) *d = *f; return d; }
namespace std {
// The original's four element helpers are out-of-line explicit specializations (hand-written try/catch loops: the
// _Nonscalar_ptr_iterator_tag overloads of the stock <memory>); declared first so _Insert_n calls them, defined in address order.
// _Xlen is the folded 0x43e2a0, an ordinary call (the epilogue follows it)
template<> void vector<NavVert, allocator<NavVert> >::_Xlen() const;
template<> void vector<NavCell, allocator<NavCell> >::_Xlen() const;
template<> NavVert* _Uninit_copy<NavVert*, NavVert*, allocator<NavVert> >(NavVert*, NavVert*, NavVert*, allocator<NavVert>&, _Nonscalar_ptr_iterator_tag);
template<> void _Uninit_fill_n<NavVert*, unsigned int, NavVert, allocator<NavVert> >(NavVert*, unsigned int, const NavVert&, allocator<NavVert>&, _Nonscalar_ptr_iterator_tag);
template<> NavCell* _Uninit_copy<NavCell*, NavCell*, allocator<NavCell> >(NavCell*, NavCell*, NavCell*, allocator<NavCell>&, _Nonscalar_ptr_iterator_tag);
template<> void _Uninit_fill_n<NavCell*, unsigned int, NavCell, allocator<NavCell> >(NavCell*, unsigned int, const NavCell&, allocator<NavCell>&, _Nonscalar_ptr_iterator_tag);
// the stock fill / copy_backward / _Ucopy / _Ufill, expanded (cl 13.10 keeps them out of line for these elements; the original has them inline)
template<> __forceinline void fill<NavVert*, NavVert>(NavVert* _First, NavVert* _Last, const NavVert& _Val)
{
    for (; _First != _Last; ++_First)
        *_First = _Val;
}
template<> __forceinline NavVert* copy_backward<NavVert*, NavVert*>(NavVert* _First, NavVert* _Last, NavVert* _Dest)
{
    while (_First != _Last)
        *--_Dest = *--_Last;
    return _Dest;
}
template<> __forceinline NavVert* _Uninitialized_copy<NavVert*, NavVert*, allocator<NavVert> >(NavVert* _First, NavVert* _Last, NavVert* _Dest, allocator<NavVert>& _Al)
{
    return _Uninit_copy(_First, _Last, _Dest, _Al, _Ptr_cat(_First, _Dest));
}
template<> template<> __forceinline NavVert* vector<NavVert, allocator<NavVert> >::_Ucopy<NavVert*>(NavVert* _First, NavVert* _Last, NavVert* _Ptr)
{
    return _Uninitialized_copy(_First, _Last, _Ptr, this->_Alval);
}
template<> __forceinline NavVert* vector<NavVert, allocator<NavVert> >::_Ufill(NavVert* _Ptr, size_type _Count, const NavVert& _Val)
{
    _Uninit_fill_n(_Ptr, _Count, _Val, this->_Alval, _Ptr_cat(_Ptr, _Ptr));
    return (_Ptr + _Count);
}
template<> __forceinline void fill<NavCell*, NavCell>(NavCell* _First, NavCell* _Last, const NavCell& _Val)
{
    for (; _First != _Last; ++_First)
        *_First = _Val;
}
template<> __forceinline NavCell* copy_backward<NavCell*, NavCell*>(NavCell* _First, NavCell* _Last, NavCell* _Dest)
{
    while (_First != _Last)
        *--_Dest = *--_Last;
    return _Dest;
}
template<> __forceinline NavCell* _Uninitialized_copy<NavCell*, NavCell*, allocator<NavCell> >(NavCell* _First, NavCell* _Last, NavCell* _Dest, allocator<NavCell>& _Al)
{
    return _Uninit_copy(_First, _Last, _Dest, _Al, _Ptr_cat(_First, _Dest));
}
template<> template<> __forceinline NavCell* vector<NavCell, allocator<NavCell> >::_Ucopy<NavCell*>(NavCell* _First, NavCell* _Last, NavCell* _Ptr)
{
    return _Uninitialized_copy(_First, _Last, _Ptr, this->_Alval);
}
template<> __forceinline NavCell* vector<NavCell, allocator<NavCell> >::_Ufill(NavCell* _Ptr, size_type _Count, const NavCell& _Val)
{
    _Uninit_fill_n(_Ptr, _Count, _Val, this->_Alval, _Ptr_cat(_Ptr, _Ptr));
    return (_Ptr + _Count);
}
// @ 0x424720
template vector<NavCell>::iterator vector<NavCell>::iterator::operator+(vector<NavCell>::difference_type) const;
// @ 0x424740
template void vector<NavVert>::resize(size_t, NavVert);
// @ 0x4247e0
template vector<NavVert>::iterator vector<NavVert>::erase(vector<NavVert>::iterator, vector<NavVert>::iterator);
// @ 0x424830
template void vector<NavVert>::_Insert_n(vector<NavVert>::iterator, size_t, const NavVert&);
// @ 0x424bc0
template<> NavVert* _Uninit_copy<NavVert*, NavVert*, allocator<NavVert> >(NavVert* _First, NavVert* _Last, NavVert* _Dest, allocator<NavVert>& _Al, _Nonscalar_ptr_iterator_tag)
{
    NavVert* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            new ((void*)_Dest) NavVert(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}
// @ 0x424c40
template<> void _Uninit_fill_n<NavVert*, unsigned int, NavVert, allocator<NavVert> >(NavVert* _First, unsigned int _Count, const NavVert& _Val, allocator<NavVert>& _Al, _Nonscalar_ptr_iterator_tag)
{
    NavVert* _Next = _First;
    try {
        for (; 0 < _Count; --_Count, ++_First)
            new ((void*)_First) NavVert(_Val);
    } catch (...) {
        for (; _Next != _First; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
}
// @ 0x424cc0
// The shrink path of resize calls erase out of line (0x424d60): a bodyless specialization declaration keeps the
// call site from inlining the stock body (cl 13.10 would otherwise inline std::copy's tail into resize).
template<> __declspec(noinline) vector<NavCell>::iterator vector<NavCell>::erase(vector<NavCell>::iterator _First, vector<NavCell>::iterator _Last);
template void vector<NavCell>::resize(size_t, NavCell);
// @ 0x424d60
template<> vector<NavCell>::iterator vector<NavCell>::erase(vector<NavCell>::iterator _First, vector<NavCell>::iterator _Last)
{   // erase [_First, _Last): the stock <vector> body, defined here as the specialization the declaration above names
if (_First != _Last)
    {   // worth doing, copy down over hole
    NavCell* _Ptr = copy_nonscalar(_ITER_BASE(_Last), _Mylast, _ITER_BASE(_First));
    _Destroy(_Ptr, _Mylast);
    _Mylast = _Ptr;
    }
return (_First);
}
// @ 0x424db0
template void vector<NavCell>::_Insert_n(vector<NavCell>::iterator, size_t, const NavCell&);
// @ 0x425140
template<> NavCell* _Uninit_copy<NavCell*, NavCell*, allocator<NavCell> >(NavCell* _First, NavCell* _Last, NavCell* _Dest, allocator<NavCell>& _Al, _Nonscalar_ptr_iterator_tag)
{
    NavCell* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            new ((void*)_Dest) NavCell(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}
// @ 0x4251c0
template<> void _Uninit_fill_n<NavCell*, unsigned int, NavCell, allocator<NavCell> >(NavCell* _First, unsigned int _Count, const NavCell& _Val, allocator<NavCell>& _Al, _Nonscalar_ptr_iterator_tag)
{
    NavCell* _Next = _First;
    try {
        for (; 0 < _Count; --_Count, ++_First)
            new ((void*)_First) NavCell(_Val);
    } catch (...) {
        for (; _Next != _First; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
}
}
