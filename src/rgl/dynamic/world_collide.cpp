// world_collide.cpp (0x426490..0x427d80): split from dynamic_entity.cpp. The world's per-cell shape and entity
// queries with their box tests and the group-mask ordering predicate, std::unique and the introsort instances behind
// the query's sort, dynamic_entity::on_entity / on_shape, the std::vector<Contact> members and the collision entity's
// destructor. Boundary at 0x426490: dynamic_entity::collide 0x4263d0 re-reads its address-taken `count` after every
// on_shape call, i.e. it was compiled without world::query_shapes' body (with it visible cl keeps count in ebx;
// docs/codegen.md 16, "a same-unit callee body can hide a translation-unit boundary"). The functions from 0x426490 on
// are one contiguous range, so they stay together here.
#include <math.h>
#include "./dynamic_entity.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

// std::vector<point20>::iterator::operator+ stays out of line (0x40f880, a COMDAT in mesh_obj.cpp), like the original's callers here
namespace std {
template<> vector<rgl::point20, allocator<rgl::point20> >::iterator vector<rgl::point20, allocator<rgl::point20> >::iterator::operator+(int) const;
}
using namespace rgl;

void sort_range(std::vector<entity*>::iterator first, std::vector<entity*>::iterator last, int ideal);          // 0x426a30

// ---- the world's queries ------------------------------------------------------------------------------------------
// @ 0x426490
void world::query_shapes(entity* e, shape_base** out, int* count, int* use3d, unsigned mask)
{
    *count = 0;
    for (cellref* r = (cellref*)e->refs; r != 0; r = r->next) {
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
    size_t cnt = a.size();
    while (i < cnt) {
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
    for (cellref* r = (cellref*)e->refs; r != 0; r = r->next) {
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
    rigid_base* o = (rigid_base*)e;
    if (shape == 0)
        return false;
    if (o->shape == 0)
        return false;
    contact_rec c[4];
    int n = ::shape_contact_count(shape, o->shape, 4, (contact*)c);
    if (n == 0)
        return false;
    for (int i = 0; i < n; ++i) {
        vector4 nrm = *(vector4*)&c[i].normal;
        vector4 pt = *(vector4*)&c[i].point;
        vector4 tb, ta;
        const vector4* vb = o->point_velocity(&tb, &pt);
        const vector4* va = point_velocity(&ta, &pt);
        vector4 rel(va->x - vb->x, va->y - vb->y, va->z - vb->z);
        Contact rec;
        rec.p = pt;
        rec.n = nrm;
        ((contact_ops&)contacts).push_back(rec);
        vector4 neg(-nrm.x, -nrm.y, -nrm.z);
        Contact rec2;
        rec2.p = pt;
        rec2.n = neg;
        ((contact_ops&)o->contacts).push_back(rec2);
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

// ---- std::vector<Contact> ----------------------------------------------------------------------------------------
namespace std {
// @ 0x427880
template<> Contact* _Uninit_copy<Contact*, Contact*, allocator<Contact> >(Contact* _First, Contact* _Last, Contact* _Dest, allocator<Contact>& _Al, _Nonscalar_ptr_iterator_tag)
{
    Contact* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            new ((void*)_Dest) Contact(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}
// @ 0x4274c0
template<> void _Uninit_fill_n<Contact*, unsigned int, Contact, allocator<Contact> >(Contact* _First, unsigned int _Count, const Contact& _Val, allocator<Contact>& _Al, _Nonscalar_ptr_iterator_tag)
{
    Contact* _Next = _First;
    try {
        for (; 0 < _Count; --_Count, ++_First)
            new ((void*)_First) Contact(_Val);
    } catch (...) {
        for (; _Next != _First; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
}
}

namespace std {
template<> __declspec(noinline) void _Uninitialized_fill_n<Contact*, unsigned int, Contact, allocator<Contact> >(Contact*, unsigned int, const Contact&, allocator<Contact>&);   // 0x427ca0, defined at the end of the file
}
// The body of std::_Uninitialized_fill_n (VC7.1 <memory>) as the callers of this vector inline it; the explicit specialization of the
// std function itself (0x427ca0) is the out-of-line copy that on_shape's push_back calls.
template<class _FI, class _Diff, class _Tval, class _Alloc> __forceinline void fill_n_inl(_FI _F, _Diff _N, const _Tval& _X, _Alloc& _Al)
{
    std::_Uninit_fill_n(_F, _N, _X, _Al, std::_Ptr_cat(_F, _F));
}
__forceinline Contact* contact_ops::_Ucopy(Contact* f, Contact* l, Contact* p) { return std::_Uninitialized_copy(f, l, p, *(std::allocator<Contact>*)this); }
__forceinline Contact* contact_ops::_Ufill(Contact* p, size_type n, const Contact& v) { fill_n_inl(p, n, v, *(std::allocator<Contact>*)this); return p + n; }
// @ 0x427450
void contact_ops::push_back(const Contact& _Val)
{
    if (size() < capacity())
        last = _Ufill(last, 1, _Val);
    else
        insert(end_(), _Val);
}
// @ 0x427540
contact_ops::iterator contact_ops::insert(iterator _Where, const Contact& _Val)
{
    size_type _Off = size() == 0 ? 0 : _Where._Ptr - first;
    _Insert_n(_Where._Ptr, (size_type)1, _Val);
    return iterator(first + _Off);
}
// @ 0x427590
void contact_ops::_Insert_n(Contact* _Where, size_type _Count, const Contact& _Val)
{
    Contact _Tmp = _Val;
    size_type _Capacity = capacity();
    if (_Count == 0)
        ;
    else if (max_size() - size() < _Count)
        _Xlen();
    else if (_Capacity < size() + _Count) {
        _Capacity = max_size() - _Capacity / 2 < _Capacity ? 0 : _Capacity + _Capacity / 2;
        if (_Capacity < size() + _Count)
            _Capacity = size() + _Count;
        std::allocator<Contact>& _Alval = *(std::allocator<Contact>*)this;
        Contact* _Newvec = _Alval.allocate(_Capacity);
        Contact* _Ptr = _Newvec;
        try {
            _Ptr = _Ucopy(first, _Where, _Newvec);
            _Ptr = _Ufill(_Ptr, _Count, _Tmp);
            _Ucopy(_Where, last, _Ptr);
        } catch (...) {
            _Destroy(_Newvec, _Ptr);
            _Alval.deallocate(_Newvec, _Capacity);
            throw;
        }
        _Count += size();
        if (first != 0) {
            _Destroy(first, last);
            _Alval.deallocate(first, end - first);
        }
        end = _Newvec + _Capacity;
        last = _Newvec + _Count;
        first = _Newvec;
    } else if ((size_type)(last - _Where) < _Count) {
        _Ucopy(_Where, last, _Where + _Count);
        try {
            _Ufill(last, _Count - (last - _Where), _Tmp);
        } catch (...) {
            _Destroy(_Where + _Count, last + _Count);
            throw;
        }
        last += _Count;
        {
            Contact* _Fe = last - _Count;
            for (Contact* _Q = _Where; _Q != _Fe; ++_Q)
                *_Q = _Tmp;
        }
    } else {
        Contact* _Oldend = last;
        last = _Ucopy(_Oldend - _Count, _Oldend, last);
        {
            Contact* _L = _Oldend - _Count;
            Contact* _D = _Oldend;
            while (_Where != _L)
                *--_D = *--_L;
        }
        {
            Contact* _Q = _Where;
            Contact* _Fe = _Where + _Count;
            for (; _Q != _Fe; ++_Q)
                *_Q = _Tmp;
        }
    }
}

// The std::vector<Contact> members as collide_shape has them: push_back and insert expanded inline, the fill through the
// out-of-line 0x427ca0 and the insertion through 0x427590 (the real members, contact_ops'
struct contact_inl : contact_ops {
    // iterator::operator+ is std::vector<point20>::iterator's (0x40f880; the Contact and point20 records are the same size, so the original has one instance)
    typedef std::vector<point20>::iterator piter;
    iterator begin() { return iterator(first); }
    iterator end_() { return iterator(last); }
    Contact* _Ufill(Contact* p, size_type n, const Contact& v) { std::_Uninitialized_fill_n(p, n, v, *(std::allocator<Contact>*)this); return p + n; }   // the out-of-line 0x427ca0
    __forceinline iterator insert(iterator _Where, const Contact& _Val)
    {
        size_type _Off = size() == 0 ? 0 : _Where._Ptr - first;
        _Insert_n(_Where._Ptr, (size_type)1, _Val);
        return iterator((Contact*)(piter((point20*)first) + (int)_Off)._Myptr);
    }
    __forceinline void push_back(const Contact& _Val)
    {
        if (size() < capacity())
            last = _Ufill(last, 1, _Val);
        else
            insert(end_(), _Val);
    }
};

// @ 0x427900
bool dynamic_entity::on_shape(shape_base* g)
{
    if (shape == 0)
        return false;
    contact_rec c[4];
    int n = ::shape_contact_count(shape, g, 4, (contact*)c);
    if (n == 0)
        return false;
    for (int i = 0; i < n; ++i) {
        vector4 nrm = *(vector4*)&c[i].normal;
        vector4 pt = *(vector4*)&c[i].point;
        Contact rec;
        vector4 v;
        point_velocity(&v, &pt);
        rec.p = pt;
        rec.n = nrm;
        ((contact_inl&)contacts).push_back(rec);
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
            vector4 corr = *(vector4*)&c[i].normal * d;
            shift.x += corr.x;
            shift.y += corr.y;
            shift.z += corr.z;
        }
    }
    return true;
}

namespace std {
// @ 0x427ca0
template<> __declspec(noinline) void _Uninitialized_fill_n<Contact*, unsigned int, Contact, allocator<Contact> >(Contact* _First, unsigned int _Count, const Contact& _Val, allocator<Contact>& _Al)
{
    _Nonscalar_ptr_iterator_tag _Cat;
    _Uninit_fill_n(_First, _Count, _Val, _Al, _Cat);
}
}
