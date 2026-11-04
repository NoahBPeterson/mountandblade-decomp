// world_grid.cpp (0x423c00..0x425370): the world's collision grid and the ground geometry. World::add_item (a shape or a
// composite of shapes goes into the grid cells it covers), the cell rectangle relocation, the cell's sorted shape
// vector, the shape registration of an entity, the ground geometry's vertex / face-group store with its std::vector
// instantiations (VC7.1 <vector> for the 0x24-byte vertex record and the 0x88-byte group record), add_tree /
// add_shape. Migrated from slices s00423540 (0x423c00..0x423ea0), s00423ed0, s00424830 and s004251c0 (its
// first three functions and the _eh part).
// Boundary: see MIGRATION.md.
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
void rgl::world::add_item(rgl::item_obj* e)
{
    world_grid* self = (world_grid*)this;
    grid_shape* o = (grid_shape*)e;
    if (o->shape_type() == 3) {
        grid_composite* c = (grid_composite*)o;
        for (int i = 0; i < c->count; i++) {
            if (c->bodies[i]->shape_type() == 3)
                add_item((rgl::item_obj*)c->bodies[i]);
            else
                self->relocate(c->bodies[i]);
        }
    } else if (o->shape_type() != 4) {
        self->relocate(o);
    }
}

// @ 0x423c80
void world_grid::relocate(grid_shape* o)
{
    float tmp[3];
    int2 nlo, nhi;
    to_cell((const float*)o->get_min((vector4*)tmp), &nlo.x);
    to_cell((const float*)o->get_max((vector4*)tmp), &nhi.x);
    if (nlo.x == o->min_x && nlo.y == o->min_y && nhi.x == o->max_x && nhi.y == o->max_y)
        return;
    const int2* lo = (const int2*)&o->min_x;
    const int2* hi = (const int2*)&o->max_x;
    int2 t1, t2;
    int2 mn = *min2(&t1, &nlo, lo);
    int2 mx = *max2(&t2, &nhi, hi);
    int2 p;
    for (p.x = mn.x; p.x <= mx.x; p.x++) {
        for (p.y = mn.y; p.y <= mx.y; p.y++) {
            if ((bool)in_rect(&p, lo, hi)) {
                if (!(bool)in_rect(&p, &nlo, &nhi)) {
                    grid_cell* c = &((grid_cell**)rows)[p.x][p.y];
                    c->remove(o);
                    shape_list::node* h = c->objs.head;
                    shape_list::node* n = h->next;
                    while (n != h) {
                        if (n->obj == o)
                            break;
                        n = n->next;
                    }
                    if (n != h) {
                        n->prev->next = n->next;
                        n->next->prev = n->prev;
                        operator delete(n);
                        c->objs.size--;
                    }
                }
            } else if ((bool)in_rect(&p, &nlo, &nhi)) {
                ((grid_cell**)rows)[p.x][p.y].add(o);
            }
        }
    }
    *(int2*)&o->min_x = nlo;
    *(int2*)&o->max_x = nhi;
}

// @ 0x423e70
int2* min2(int2* out, const int2* a, const int2* b)
{
    int x = a->x < b->x ? a->x : b->x;
    int y = a->y < b->y ? a->y : b->y;
    out->x = x;
    out->y = y;
    return out;
}
// @ 0x423ea0
int2* max2(int2* out, const int2* a, const int2* b)
{
    int x = a->x > b->x ? a->x : b->x;
    int y = a->y > b->y ? a->y : b->y;
    out->x = x;
    out->y = y;
    return out;
}

// @ 0x423ed0
int world_grid::in_rect(const int2* p, const int2* lo, const int2* hi)
{
    if (p->x >= lo->x && p->x <= hi->x && p->y >= lo->y && p->y <= hi->y)
        return 1;
    return 0;
}

// ---- the cell ----------------------------------------------------------------------------------------------------
// @ 0x423f10
void grid_cell::add(grid_shape* x)
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
        ((std::vector<rgl::Obj*>*)this)->resize(size() + 1, 0);   // 0x41fa00: the cell's shape vector is the std::vector<Obj*> instantiation of entity.cpp (identical-code folding)
        for (int j = size() - 1; j > (int)pos; j--)
            first[j] = first[j - 1];
        first[pos] = x;
    }
    shape_list* l = &objs;
    void (std::list<int>::*ins)(std::list<int>::iterator, const int&) = &std::list<int>::_Insert;   // called out of line, as the original does
    (((std::list<int>*)l)->*ins)(((std::list<int>*)l)->end(), (const int&)x);   // 0x41f640: entity.cpp's list<int>::_Insert, folded with every list of pointers
}

// @ 0x4240d0
__declspec(noinline) int shape_vec::find(grid_shape* o) const
{
    for (unsigned i = 0; first != 0 && i < (unsigned)(last - first); i++)
        if (first[i] == o)
            return i;
    return -1;
}

// @ 0x424060
void shape_vec::remove(grid_shape* o)
{
    int idx = find(o);
    if (idx >= 0) {
        unsigned i = idx;
        while (i < size() - 1) {
            grid_shape** f = first;
            f[i] = f[i + 1];
            i++;
        }
        ((std::vector<rgl::Obj*>*)this)->resize(size() - 1, 0);   // 0x41fa00 (see grid_cell::add)
    }
}

// ---- entity::add_shape / register the shape with the world ---------------------------------------------------------
// @ 0x424100
void rgl::strategic_entity::add_shape(rgl::item_obj* o)
{
    items.push_back(o);
    ((grid_shape*)o)->set_mc(id);
    ((world_grid*)owner)->reg_shape((grid_shape*)o);
}

// @ 0x424170
void world_grid::reg_shape(grid_shape* o)
{
    if (o->shape_type() == 2) {
        o->m4 = ((ground_store*)ground)->add(((grid_face*)o)->count, ((grid_face*)o)->w);
    } else if (o->shape_type() == 3) {
        grid_composite* c = (grid_composite*)o;
        for (int i = 0; i < c->count; i++) {
            if (c->bodies[i]->shape_type() == 2) {
                grid_face* k = (grid_face*)c->bodies[i];
                c->bodies[i]->m4 = ((ground_store*)ground)->add(k->count, k->w);
            }
        }
    } else {
        o->shape_type();
    }
    grid_shape* t = o;
    objs.push_back((rgl::Obj* const&)t);   // 0x423fc0
    add_tree(t);
}

// ---- the ground geometry store ---------------------------------------------------------------------------------
// @ 0x424220
int ground_store::add(int n, const vector4* pts)
{
    unsigned base = verts.size();
    {
        ground_v36 t36;
        verts.resize(base + n, t36);
    }
    vector4 nrm = face_normal(pts, pts + 1, pts + 2);
    vector4 sum;
    sum.x = 0.0f; sum.y = 0.0f; sum.z = 0.0f;
    for (int i = 0; i < n; i++) {
        verts[base + i].p = pts[i];
        verts[base + i].n = nrm;
        verts[base + i].f20 = 0;
        sum.x += pts[i].x;
        sum.y += pts[i].y;
        sum.z += pts[i].z;
    }
    unsigned idx = groups.size();
    ground_g136 g;
    groups.resize(idx + 1, g);
    groups._Myfirst[idx].n = n;
    groups._Myfirst[idx].c = sum;
    groups._Myfirst[idx].f24 = 0;
    groups._Myfirst[idx].f68 = 0;
    for (int k = 0; k < n; k++) {
        groups._Myfirst[idx].link[k] = -1;
        groups._Myfirst[idx].vidx[k] = base + k;
        vec136::iterator_ it = groups.begin() + idx;
        it._Ptr->f38[k] = 0;
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

// @ 0x4246f0
ground_g136::ground_g136()
{
    f24 = 0;
    f6c = 0;
    f70 = 0;
    f68 = 0;
    f84 = 0;
    f64 = 1.0f;
    f48 = -1;
    f4c = -1;
    f50 = -1;
}

// @ 0x424720
vec136::iterator_ vec136::iterator_::operator+(int off) const { iterator_ r; r._Ptr = _Ptr + off; return r; }

// ---- add_tree / add_shape -------------------------------------------------------------------------------------
// @ 0x425240
void world_grid::add_tree(grid_shape* s)
{
    if (s->shape_type() == 3) {
        grid_composite* c = (grid_composite*)s;
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

// @ 0x4252c0
void world_grid::add_shape(grid_shape* s)
{
    int lo[2], hi[2];
    vector4 t0, t1;
    to_cell((const float*)s->get_min(&t0), lo);
    to_cell((const float*)s->get_max(&t1), hi);
    for (int i = lo[0]; i <= hi[0]; i++)
        for (int j = lo[1]; j <= hi[1]; j++)
            ((grid_cell**)rows)[i][j].add(s);
    s->min_x = lo[0];
    s->min_y = lo[1];
    s->max_x = hi[0];
    s->max_y = hi[1];
}

// ---- the vector instantiations for the 0x24-byte vertex record (ground_v36) and the 0x88-byte group record (ground_g136) -----------
// These bodies do not come out of the stock <vector> with cl 13.10.3052 (it leaves allocator::construct and the
// _Ucopy/_Ufill wrappers out of line, and stops inlining std::copy for elements of >= 128 bytes), so the vector members
// are written out as a small class with the same members as the VC7.1 <vector>. _Xlen stays an undefined external
// (0x43e2a0): with a body cl sees it never returns. The 9-byte stubs after _Uninit_* and the cold tails after
// _Insert_n are catch(...) funclets that cl emits in the parent's COMDAT, with no symbol of their own.
namespace std {
// @ 0x424bc0
template<> ground_v36* _Uninit_copy<ground_v36*, ground_v36*, allocator<ground_v36> >(ground_v36* _First, ground_v36* _Last, ground_v36* _Dest, allocator<ground_v36>& _Al, _Nonscalar_ptr_iterator_tag)
{
    ground_v36* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            new ((void*)_Dest) ground_v36(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}
// @ 0x424c40
template<> void _Uninit_fill_n<ground_v36*, unsigned int, ground_v36, allocator<ground_v36> >(ground_v36* _First, unsigned int _Count, const ground_v36& _Val, allocator<ground_v36>& _Al, _Nonscalar_ptr_iterator_tag)
{
    ground_v36* _Next = _First;
    try {
        for (; 0 < _Count; --_Count, ++_First)
            new ((void*)_First) ground_v36(_Val);
    } catch (...) {
        for (; _Next != _First; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
}
// @ 0x425140
template<> ground_g136* _Uninit_copy<ground_g136*, ground_g136*, allocator<ground_g136> >(ground_g136* _First, ground_g136* _Last, ground_g136* _Dest, allocator<ground_g136>& _Al, _Nonscalar_ptr_iterator_tag)
{
    ground_g136* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            new ((void*)_Dest) ground_g136(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}
// @ 0x4251c0 (cl 13.10 leaves allocator::construct out of line once the copied element exceeds 128 bytes, while the original inlined it)
template<> __declspec(noinline) void _Uninit_fill_n<ground_g136*, unsigned int, ground_g136, allocator<ground_g136> >(ground_g136* _First, unsigned int _Count, const ground_g136& _Val, allocator<ground_g136>& _Al, _Nonscalar_ptr_iterator_tag)
{
    ground_g136* _Next = _First;
    try {
        for (; 0 < _Count; --_Count, ++_First)
            new ((void*)_First) ground_g136(_Val);
    } catch (...) {
        for (; _Next != _First; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
}
}

#define SZ() (_Myfirst == 0 ? 0 : _Mylast - _Myfirst)

// _Insert_n body shared by both element types (the VC7.1 <vector> source, with size() written out).
#define INSERT_N_BODY(T) \
    T _Tmp = _Val; \
    size_type _Capacity = capacity(); \
    if (_Count == 0) \
        ; \
    else if (max_size() - SZ() < _Count) \
        _Xlen(); \
    else if (_Capacity < SZ() + _Count) { \
        _Capacity = max_size() - _Capacity / 2 < _Capacity ? 0 : _Capacity + _Capacity / 2; \
        if (_Capacity < SZ() + _Count) \
            _Capacity = SZ() + _Count; \
        T* _Newvec = _Alval.allocate(_Capacity); \
        T* _Ptr = _Newvec; \
        try { \
            _Ptr = _Ucopy(_Myfirst, _Where, _Newvec); \
            _Ptr = _Ufill(_Ptr, _Count, _Tmp); \
            _Ucopy(_Where, _Mylast, _Ptr); \
        } catch (...) { \
            _Destroy(_Newvec, _Ptr); \
            _Alval.deallocate(_Newvec, _Capacity); \
            throw; \
        } \
        _Count += SZ(); \
        if (_Myfirst != 0) { \
            _Destroy(_Myfirst, _Mylast); \
            _Alval.deallocate(_Myfirst, _Myend - _Myfirst); \
        } \
        _Myend = _Newvec + _Capacity; \
        _Mylast = _Newvec + _Count; \
        _Myfirst = _Newvec; \
    } else if ((size_type)(_Mylast - _Where) < _Count) { \
        _Ucopy(_Where, _Mylast, _Where + _Count); \
        try { \
            _Ufill(_Mylast, _Count - (_Mylast - _Where), _Tmp); \
        } catch (...) { \
            _Destroy(_Where + _Count, _Mylast + _Count); \
            throw; \
        } \
        _Mylast += _Count; \
        { \
            T* _Fe = _Mylast - _Count; \
            for (T* _Q = _Where; _Q != _Fe; ++_Q) \
                *_Q = _Tmp; \
        } \
    } else { \
        T* _Oldend = _Mylast; \
        _Mylast = _Ucopy(_Oldend - _Count, _Oldend, _Mylast); \
        { \
            T* _L = _Oldend - _Count; \
            T* _D = _Oldend; \
            while (_Where != _L) \
                *--_D = *--_L; \
        } \
        { \
            T* _Q = _Where; \
            T* _Fe = _Where + _Count; \
            for (; _Q != _Fe; ++_Q) \
                *_Q = _Tmp; \
        } \
    }

struct vec36 {
    VEC_COMMON(ground_v36)
    void _Insert_n(iterator _Where, size_type _Count, const ground_v36& _Val);   // 0x424830
};
// @ 0x424830
void vec36::_Insert_n(iterator _Where, size_type _Count, const ground_v36& _Val)
{
    INSERT_N_BODY(ground_v36)
}

__forceinline ground_g136* copy_nonscalar(ground_g136* f, ground_g136* l, ground_g136* d) { for (; f != l; ++d, ++f) *d = *f; return d; }

// @ 0x424db0
void vec136::_Insert_n(iterator _Where, size_type _Count, const ground_g136& _Val)
{
    INSERT_N_BODY(ground_g136)
}
// @ 0x424cc0
void vec136::resize(size_type _Newsize, ground_g136 _Val)
{
    if (size() < _Newsize)
        insert(_Mylast, _Newsize - size(), _Val);
    else if (_Newsize < size())
        erase(iterator_::mk(_Myfirst + _Newsize), iterator_::mk(_Mylast));
}
// @ 0x424d60
vec136::iterator_ vec136::erase(iterator_ _First, iterator_ _Last)
{
    if (_First._Ptr != _Last._Ptr) {
        ground_g136* _Ptr = copy_nonscalar(_Last._Ptr, _Mylast, _First._Ptr);
        _Destroy(_Ptr, _Mylast);
        _Mylast = _Ptr;
    }
    return _First;
}

namespace std {
// @ 0x424740
template void vector<ground_v36>::resize(size_t, ground_v36);
// @ 0x4247e0
template vector<ground_v36>::iterator vector<ground_v36>::erase(vector<ground_v36>::iterator, vector<ground_v36>::iterator);
}
