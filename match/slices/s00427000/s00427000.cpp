// Slice s00427000: rigid_base::collide_entity (two bodies' shape contacts -> impulses on both) and the
// std::vector<Contact> contact-list instantiations it uses (push_back, insert, _Insert_n, _Uninit_fill_n, _Uninit_copy,
// with their catch(...) funclets). Names and layouts are provisional. The vector members need /EHa (with /EHsc cl
// drops the EH frame the original keeps); collide_entity has no EH constructs and is identical either way, so the
// manifest rows carry /EHa for the STL helpers and the default flags for collide_entity.
//
// The stock <vector> instantiation does not reproduce these bodies (cl keeps allocator::construct out of line);
// the vector is written out as a small class with the members of the VC7.1 <vector>, and _Uninit_copy /
// _Uninit_fill_n are specialised with the placement-new inline (same trick as vec32 in s0040e060). The 9-byte stubs
// after _Uninit_* (0x42752b, 0x4278ed) and the cold tail of _Insert_n (0x427730) are catch(...) funclets that cl
// emits in the parent's COMDAT with no symbol of their own.
#include <vector>
#include <memory>
#include <new>
#include <stdexcept>
#include "rgl.h"

using rgl::vector4;

struct coll_shape;
struct vec4p { float x, y, z, w; };  // vector4 layout without the (empty) constructor: keeps contact_rec c[4] free of __vec_ctor
struct contact_rec {                 // 0x24 bytes, filled by shape_contact_count (s0040bcb0)
    vec4p normal;
    vec4p point;
    float depth;
};
int shape_contact_count(coll_shape* a, coll_shape* b, int max, contact_rec* out);       // 0x40bfe0

struct Contact { vector4 p, n; };        // 0x20 stored contact (point, normal)

struct contact_vec {
    std::allocator<Contact> _Alval;
    Contact *_Myfirst, *_Mylast, *_Myend;
    typedef unsigned size_type;
    struct iterator { Contact* _Ptr; iterator() {} iterator(Contact* p) : _Ptr(p) {} };
    size_type capacity() const { return _Myfirst == 0 ? 0 : _Myend - _Myfirst; }
    size_type size() const { return _Myfirst == 0 ? 0 : _Mylast - _Myfirst; }
    size_type max_size() const { return _Alval.max_size(); }
    iterator begin() { return iterator(_Myfirst); }
    iterator end() { return iterator(_Mylast); }
    __forceinline Contact* _Ucopy(Contact* f, Contact* l, Contact* p) { return std::_Uninitialized_copy(f, l, p, _Alval); }
    __forceinline Contact* _Ufill(Contact* p, size_type n, const Contact& v) { std::_Uninitialized_fill_n(p, n, v, _Alval); return p + n; }
    void _Destroy(Contact* f, Contact* l) { for (; f != l; ++f) std::_Destroy(f); }
    void _Xlen() const;                                             // 0x43e2a0 (length_error "vector<T> too long")
    void _Insert_n(Contact* _Where, size_type _Count, const Contact& _Val);
    __declspec(noinline) iterator insert(iterator _Where, const Contact& _Val);
    void push_back(const Contact& _Val);
};

// ---- entity family (reduced; see s00427900) ------------------------------------------------------------------------
struct entity {
    entity();                                                       // 0x41fba0
    virtual ~entity();                                              // 0
    virtual int type_mask();                                        // 1
    virtual rgl::string type_name();                                // 2
    virtual void sync();                                            // 3
    virtual void recalc();                                          // 4
    virtual float get_fbc();                                        // 5
    virtual void set_fbc(const float* p);                           // 6
    virtual void update(float dt);                                  // 7
    virtual void set_f144(const float* p);                          // 8
    virtual void render(void* sc, void* out);                       // 9
    virtual bool awake();                                           // 10
    virtual void s11();                                             // 11
    virtual const vector4* local_origin();                          // 12
    virtual const vector4* get_max();                               // 13
    virtual const vector4* get_min();                               // 14
    virtual int is_dirty();                                         // 15
    virtual void s16(); virtual int slot17();                       // 16, 17
    virtual void set_mode(unsigned m);                              // 18
    virtual void clear();                                           // 19
    virtual void set_local(const rgl::matrix* m);                   // 20
    virtual void set_local_dirty(const rgl::matrix* m);             // 21
    virtual void s22(); virtual void s23();                         // 22, 23
    virtual bool is_dynamic();                                      // 24
    char pad4[0x98 - 4];
    coll_shape* shape;                                              // 0x98
    char pad9c[0x15c - 0x9c];
};

struct rigid_base : entity {
    float restitution;                                              // 0x15c
    contact_vec contacts;                                           // 0x160
    char pad170[0x1c4 - 0x170];
    vector4 shift;                                                  // 0x1c4
    virtual vector4* point_velocity(vector4* out, const vector4* p);         // 25 (0x64)
    virtual void apply_impulse(const vector4* p, const vector4* j);          // 26 (0x68)
    virtual float impulse_response(const vector4* p, const vector4* n);      // 27 (0x6c)
    bool collide_entity(rigid_base* o);                             // 0x427000
};

static __forceinline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline float vdot(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// @ 0x427000
bool rigid_base::collide_entity(rigid_base* o)
{
    if (shape == 0)
        return false;
    if (o->shape == 0)
        return false;
    contact_rec c[4];
    int n = shape_contact_count(shape, o->shape, 4, c);
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
        contacts.push_back(rec);
        vector4 neg(-nrm.x, -nrm.y, -nrm.z);
        Contact rec2;
        rec2.p = pt;
        rec2.n = neg;
        o->contacts.push_back(rec2);
        float vn = vdot(nrm, rel);
        vector4 t = vsub(rel, vmul(nrm, vn));
        vector4 td;
        rgl::normalize(td, t);
        if (vn < 0.0f) {
            double j = -(vdot(nrm, rel) * ((o->restitution + restitution) * 0.5f + 1.0));
            float rb = o->impulse_response(&pt, &nrm);
            double mag = j / (impulse_response(&pt, &nrm) + rb);
            vector4 imp(nrm.x * mag, nrm.y * mag, nrm.z * mag);
            apply_impulse(&pt, &imp);
            vector4 imp2(-imp.x, -imp.y, -imp.z);
            o->apply_impulse(&pt, &imp2);
        }
        double j = vdot(td, rel) * -0.5;
        float rb = o->impulse_response(&pt, &td);
        double mag = j / (impulse_response(&pt, &td) + rb);
        vector4 imp(td.x * mag, td.y * mag, td.z * mag);
        apply_impulse(&pt, &imp);
        vector4 imp2(-imp.x, -imp.y, -imp.z);
        o->apply_impulse(&pt, &imp2);
    }
    return true;
}

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

void contact_vec::_Xlen() const { throw std::length_error("vector<T> too long"); }
// @ 0x427450
void contact_vec::push_back(const Contact& _Val)
{
    if (size() < capacity())
        _Mylast = _Ufill(_Mylast, 1, _Val);
    else
        insert(end(), _Val);
}
// @ 0x427540
contact_vec::iterator contact_vec::insert(iterator _Where, const Contact& _Val)
{
    size_type _Off = size() == 0 ? 0 : _Where._Ptr - _Myfirst;
    _Insert_n(_Where._Ptr, (size_type)1, _Val);
    return iterator(_Myfirst + _Off);
}
// @ 0x427590
void contact_vec::_Insert_n(Contact* _Where, size_type _Count, const Contact& _Val)
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
        Contact* _Newvec = _Alval.allocate(_Capacity);
        Contact* _Ptr = _Newvec;
        try {
            _Ptr = _Ucopy(_Myfirst, _Where, _Newvec);
            _Ptr = _Ufill(_Ptr, _Count, _Tmp);
            _Ucopy(_Where, _Mylast, _Ptr);
        } catch (...) {
            _Destroy(_Newvec, _Ptr);
            _Alval.deallocate(_Newvec, _Capacity);
            throw;
        }
        _Count += size();
        if (_Myfirst != 0) {
            _Destroy(_Myfirst, _Mylast);
            _Alval.deallocate(_Myfirst, _Myend - _Myfirst);
        }
        _Myend = _Newvec + _Capacity;
        _Mylast = _Newvec + _Count;
        _Myfirst = _Newvec;
    } else if ((size_type)(_Mylast - _Where) < _Count) {
        _Ucopy(_Where, _Mylast, _Where + _Count);
        try {
            _Ufill(_Mylast, _Count - (_Mylast - _Where), _Tmp);
        } catch (...) {
            _Destroy(_Where + _Count, _Mylast + _Count);
            throw;
        }
        _Mylast += _Count;
        {
            Contact* _Fe = _Mylast - _Count;
            for (Contact* _Q = _Where; _Q != _Fe; ++_Q)
                *_Q = _Tmp;
        }
    } else {
        Contact* _Oldend = _Mylast;
        _Mylast = _Ucopy(_Oldend - _Count, _Oldend, _Mylast);
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

