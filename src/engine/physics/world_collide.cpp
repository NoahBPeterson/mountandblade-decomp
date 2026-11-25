// world_collide.cpp (0x426490..0x427d80): split from rigid_body.cpp. The world's per-cell shape and entity
// queries with their box tests and the group-mask ordering predicate, std::unique and the introsort instances behind
// the query's sort, dynamic_entity::on_entity / on_shape, the std::vector<Contact> members and the collision entity's
// destructor. Boundary at 0x426490: dynamic_entity::collide 0x4263d0 re-reads its address-taken `count` after every
// on_shape call, i.e. it was compiled without world::query_shapes' body (with it visible cl keeps count in ebx;
// docs/codegen.md 16, "a same-unit callee body can hide a translation-unit boundary"). The functions from 0x426490 on
// are one contiguous range, so they stay together here.
#include <math.h>
#include "engine/physics/rigid_body.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/scene/frustum.h"
#include "engine/fx/keyframes.h"
#include "engine/math/mat4_include.h"
#include "engine/render/mesh_prims.h"
#include "engine/math/vec.h"
#include "engine/render/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/physics/shape.h"   // contact, shape_base, shape_contact_count
#include "engine/render/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

// std::vector<point20>::iterator::operator+ stays out of line (0x40f880, a COMDAT in mesh.cpp), like the original's callers here
namespace std {
template<> vector<rgl::point20, allocator<rgl::point20> >::iterator vector<rgl::point20, allocator<rgl::point20> >::iterator::operator+(typename vector<rgl::point20>::_Dift) const;
}
using namespace rgl;


// @ 0x427450: bodiless declaration, definition after on_shape: on_shape's call site expands it inline (the original's does
// too: the size test at 0x4279d0, the fill call 0x427ca0, then the slow path through insert and iterator::operator+)
// the std::vector<Contact> members expanded inline at on_shape's call site (docs/codegen.md 17.6): the fast path fills
// through the out-of-line 0x427ca0 and the slow path calls insert, which stays out of line as the original's does
struct contact_inl : std::vector<rgl::Contact> {
    __forceinline Contact* _Ufill(Contact* _Ptr, size_type _Count, const Contact& _Val)
    {   // copy initializing _Count * _Val, using allocator
        _Uninitialized_fill_n(_Ptr, _Count, _Val, this->_Alval);
        return (_Ptr + _Count);
    }
    __forceinline iterator insert(iterator _Where, const Contact& _Val)
    {   // insert _Val at _Where
        size_type _Off = size() == 0 ? 0 : _ITER_BASE(_Where) - _Mylast;
        _Insert_n(_Where, (size_type)1, _Val);
        return (iterator(_Mylast + _Off));
    }
    __forceinline void push(const Contact& _Val)
    {   // insert element at end
        if (size() < capacity()) {
            Contact* p = _Mylast;
            *p = _Val;
            p = p + 1;
            _Mylast = p;
        } else
            insert(end(), _Val);
    }
};
namespace std {
// @ 0x427450
template<> void vector<Contact, allocator<Contact> >::push_back(const Contact& _Val)
{
    if (size() < capacity())
        _Mylast = _Ufill(_Mylast, 1, _Val);
    else
        insert(end(), _Val);
}
}

void sort_range(std::vector<entity*>::iterator first, std::vector<entity*>::iterator last, int ideal);          // 0x426a30

// ---- the world's queries ------------------------------------------------------------------------------------------
// @ 0x426490
void world::query_shapes(entity* e, shape_base** out, int* count, int* use3d, unsigned mask)
{
    *count = 0;
    for (cellref* r = e->refs; r != 0; r = r->next) {
        cell* c = &cells[r->x][r->y];
        c->collect(e->get_v9c(), e->get_vac(), use3d, mask, out, count);
    }
}

// box tests: 3D (0x426690) and 2D xy (0x426700) overlap of [a, b] with [c, d]. The callers (collect) see them as
// returning a byte (test al, al), the bodies return int: they are called through the bool-returning twins above.
// @ 0x426690
int __declspec(noinline) box_overlap3(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return 1;
    return 0;
}

// the same 3D test inlined (0x426750)
static __forceinline bool box3_inl(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return true;
    return false;
}

// @ 0x426700
int __declspec(noinline) box_overlap2(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y)
        return 1;
    return 0;
}

// @ 0x426500
void cell::collect(const vector4* bmin, const vector4* bmax, int* use3d, unsigned mask, shape_base** out, int* count)
{
    shape_base* buf[0x2000];
    int n = 0;
    unsigned i = 0;                 // lives in memory in the original (reloaded every iteration)
    int k = 0;
    while (i < a.size()) {
        shape_base** it = &a[i];
        shape_base* sh = a[i];
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        vector4 tv[2];                  // both query temporaries live in one block (min below max)
        const vector4* smax = sh->get_max(&tv[1]);
        const vector4* smin = sh->get_min(&tv[0]);
        char hit;
        if (*use3d == 0)
            hit = box_overlap2(bmin, bmax, smin, smax);
        else
            hit = box_overlap3(bmin, bmax, smin, smax);
        if (!hit) {
            i++;
            continue;
        }
        if (k < *count) {
            shape_base* o = out[k];
            shape_base* cur = *it;
            if (cur >= o) {
                k++;
                if (cur == o) {
                    buf[n++] = a[i];
                    i++;
                } else {
                    buf[n++] = o;
                }
                continue;
            }
        }
        shape_base* cur2 = *it;
        i++;
        buf[n++] = cur2;
    }
    if (k < *count) {
        int m = *count - k;
        memcpy(&buf[n], &out[k], m * sizeof(shape_base*));
        n += m;
    }
    *count = n;
    for (int j = 0; j < *count; j++)
        out[j] = buf[j];
}

// does a type mask a pass the filter b: equal, or the lowest differing bit is clear in b
static __forceinline bool type_ok(unsigned a, unsigned b)
{
    if (a == b)
        return true;
    while (((a ^ b) & 1) == 0) {
        a >>= 1;
        b >>= 1;
    }
    return b == 0;
}

// std::unique on vector<entity*>::iterator (0x4269e0): the original keeps it out of line
namespace std {
// @ 0x4269e0
template<> __declspec(noinline) vector<rgl::entity*>::iterator unique(vector<rgl::entity*>::iterator _First, vector<rgl::entity*>::iterator _Last)
{
    for (vector<rgl::entity*>::iterator _Firstb; (_Firstb = _First) != _Last && ++_First != _Last; )
        if (*_Firstb == *_First) {
            for (; ++_First != _Last; )
                if (!(*_Firstb == *_First))
                    *++_Firstb = *_First;
            return (++_Firstb);
        }
    return (_Last);
}
}

// @ 0x426750
void world::query_entities(entity* e, unsigned mask, bool ordered, std::vector<entity*>* out)
{
    out->clear();
    for (cellref* r = e->refs; r != 0; r = r->next) {
        for (cellnode* nd = cells[r->x][r->y].head; nd != 0; nd = nd->next) {
            if (nd->obj != e) {
                if (type_ok(nd->obj->type_id(), mask)) {
                  if (!ordered || less(e, nd->obj)) {
                    if (box3_inl(e->get_v9c(), e->get_vac(), nd->obj->get_v9c(), nd->obj->get_vac()))
                        out->push_back(nd->obj);
                  }
                }
            }
        }
    }
    sort_range(out->begin(), out->end(), out->end() - out->begin());
    std::vector<entity*>::iterator u;
    u = std::unique(out->begin(), out->end());
    out->erase(u, out->end());
}

// @ 0x426960
bool world::less(entity* a, entity* b)
{
    int k = 0;
    unsigned ta = a->type_id();
    unsigned tb = b->type_id();
    if (ta == tb)
        return a->id > b->id;
    bool found = false;
    do {
        if (found) break;
        if ((((unsigned char)(tb >> (char)k) ^ (unsigned char)(ta >> (char)k)) & 1) == 0)
            k++;
        else
            found = true;
    } while (k < 32);
    return (ta >> (char)k) & 1;
}

// ---- the introsort instances behind std::sort of the query result -------------------------------------------------
// The VC7.1 <algorithm> instantiations for std::vector<entity*>::iterator (the class std::vector<T>::iterator is what
// puts the address-taken iterator temporaries in _Rotate / _Unguarded_partition). The slices instantiated them for
// std::vector<unsigned> (identical code; the linker folds them). _Adjust_heap at 0x426df0 is the plain-pointer instance.
typedef entity* u32;                                    // the element type (a pointer; the code is the same for any 4-byte element)
typedef std::vector<u32>::iterator It;

// @ 0x426a30
// std::_Sort<It,int> with sort_heap/pop_heap spelled out: in the binary they are inlined into the heap-sort tail (only
// _Pop_heap_0 is called), while cl 13.10 keeps std::sort_heap out of line here. std::_Pop_heap_0 (0x426e90) seen from
// its caller (pop_heap_0_ext) is kept out of line like the original.
void sort_range(It first, It last, int ideal)
{
    int count;
    for (; 32 < (count = last - first) && 0 < ideal; ) {
        std::pair<It, It> mid = std::_Unguarded_partition(first, last);
        ideal /= 2, ideal += ideal / 2;
        if (mid.first - first < last - mid.second)
            sort_range(first, mid.first, ideal), first = mid.second;
        else
            sort_range(mid.second, last, ideal), last = mid.first;
    }
    if (32 < count) {
        std::make_heap(first, last);
        for (; 1 < last - first; --last) {
            if (1 < last - first)
                std::_Pop_heap_0(first, last, (u32*)0);
        }
    } else if (1 < count)
        std::_Insertion_sort(first, last);
}
// @ 0x426b20
template std::pair<It, It> std::_Unguarded_partition<It>(It, It);
// @ 0x426c70
template void std::_Median<It>(It, It, It);
// @ 0x426db0
template void std::_Make_heap<It, int, u32>(It, It, int*, u32*);
// @ 0x426df0
template void std::_Adjust_heap<u32*, int, u32>(u32*, int, int, u32);  // the pointer instance is what lives at 0x426df0
// @ 0x426e50
template void std::_Push_heap<It, int, u32>(It, int, int, u32);
// @ 0x426e90
template void std::_Pop_heap_0<It, u32>(It, It, u32*);
// @ 0x426ec0
template void std::_Insertion_sort<It>(It, It);
// @ 0x426f40
template void std::_Rotate<It, int, u32>(It, It, It, int*, u32*);

// ---- dynamic_entity::on_entity / on_shape --------------------------------------------------------------------
static __forceinline float vdot(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
// collide_entity's dots sum z first: (a.z*b.z + a.y*b.y) + a.x*b.x (0x4271ee, 0x427287, 0x42735d)
static __forceinline float vdot_zyx(const vector4& a, const vector4& b) { return a.z * b.z + a.y * b.y + a.x * b.x; }

// @ 0x427000
bool dynamic_entity::on_entity(entity* e)
{
    rigid_base* o = static_cast<rigid_base*>(e);
    if (shape == 0)
        return false;
    if (o->shape == 0)
        return false;
    contact c[4];
    int n = ::shape_contact_count(shape, o->shape, 4, c);
    if (n == 0)
        return false;
    for (int i = 0; i < n; ++i) {
        vector4 nrm = c[i].normal;
        vector4 pt = c[i].point;
        vector4 tb, ta;
        const vector4* vb = o->point_velocity(&tb, &pt);
        const vector4* va = point_velocity(&ta, &pt);
        vector4 rel(va->x - vb->x, va->y - vb->y, va->z - vb->z);
        Contact rec;
        rec.p = pt;
        rec.n = nrm;
        ((contact_inl&)contacts).push(rec);
        vector4 neg(-nrm.x, -nrm.y, -nrm.z);
        Contact rec2;
        rec2.p = pt;
        rec2.n = neg;
        o->contacts.push_back(rec2);
        float vn = vdot_zyx(nrm, rel);
        vector4 t = rel - nrm * vn;
        vector4 td;
        normalize(td, t);
        if (!(vn >= 0.0f)) {                    // true for a NaN vn too (0x42727e: test ah,1 = C0, set when unordered)
            double j = -(vdot_zyx(nrm, rel) * ((o->restitution + restitution) * 0.5f + 1.0));
            float rb = o->impulse_response(&pt, &nrm);
            double mag = j / (impulse_response(&pt, &nrm) + rb);
            vector4 imp(nrm.x * mag, nrm.y * mag, nrm.z * mag);
            apply_impulse(&pt, &imp);
            vector4 imp2(-imp.x, -imp.y, -imp.z);
            o->apply_impulse(&pt, &imp2);
        }
        double j = vdot_zyx(td, rel) * -0.5;
        float rb = o->impulse_response(&pt, &td);
        double mag = j / (impulse_response(&pt, &td) + rb);
        vector4 imp = td * mag;
        apply_impulse(&pt, &imp);
        vector4 imp2(-imp.x, -imp.y, -imp.z);
        o->apply_impulse(&pt, &imp2);
    }
    return true;
}

// ---- std::vector<Contact> (collision_entity::contacts): the members the original keeps out of line ------------------------------
// @ 0x4274c0
template void std::_Uninit_fill_n(Contact*, unsigned, const Contact&, std::allocator<Contact>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x427540
template std::vector<Contact>::iterator std::vector<Contact>::insert(std::vector<Contact>::iterator, const Contact&);
// @ 0x427590
template void std::vector<Contact>::_Insert_n(std::vector<Contact>::iterator, size_t, const Contact&);
// @ 0x427880
template Contact* std::_Uninit_copy(Contact*, Contact*, Contact*, std::allocator<Contact>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x427ca0
template void std::_Uninitialized_fill_n(Contact*, unsigned, const Contact&, std::allocator<Contact>&);

// @ 0x427900
bool dynamic_entity::on_shape(shape_base* g)
{
    if (shape == 0)
        return false;
    contact c[4];
    int n = ::shape_contact_count(shape, g, 4, c);
    if (n == 0)
        return false;
    for (int i = 0; i < n; ++i) {
        vector4 nrm = c[i].normal;
        vector4 pt = c[i].point;
        Contact rec;
        vector4 v;
        point_velocity(&v, &pt);
        rec.p = pt;
        rec.n = nrm;
        ((contact_inl&)contacts).push(rec);
        float vn = nrm.x * v.x + nrm.y * v.y + v.z * nrm.z;
        vector4 t = v - nrm * vn;
        if (!(vn >= 0.0f)) {
            double j = -((restitution + 1.0) * (nrm.x * v.x + nrm.y * v.y + v.z * nrm.z));
            double mag = j / impulse_response(&pt, &nrm);
            vector4 imp(nrm.x * mag, nrm.y * mag, nrm.z * mag);
            apply_impulse(&pt, &imp);
        }
        if (vdot(t, t) > 0.01f) {
            vector4 td;
            normalize(td, t);
            double j = vdot(td, v) * -0.5;
            double mag = j / impulse_response(&pt, &td);
            vector4 imp(td.x * mag, td.y * mag, td.z * mag);
            apply_impulse(&pt, &imp);
        }
        if (c[i].depth > 0.01f) {
            float d = c[i].depth * 0.5f;
            vector4 corr = c[i].normal * d;
            shift.x += corr.x;
            shift.y += corr.y;
            shift.z += corr.z;
        }
    }
    return true;
}
