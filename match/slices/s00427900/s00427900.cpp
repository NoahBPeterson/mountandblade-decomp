// Slice s00427900: rigid_base::collide_shape (shape contacts -> impulses), the std::_Uninitialized_fill_n wrapper
// for the 32-byte contact record, the collision / skeleton / static entity destructors and constructors
// (vtables 0x54b548 / 0x54b730 / 0x54b7b8), the world's emitter pool and per-frame step, and the strategic
// entity destructor (vtable 0x54b5d0). Names and layouts are provisional; build flags /O2 /MT /Gy /TP /EHa.
#include <vector>
#include <memory>
#include <new>
#include <stdexcept>
#include "rgl.h"

using rgl::vector4;

struct coll_shape;
struct contact_rec {                 // 0x24 bytes, filled by shape_contact_count (s0040bcb0)
    vector4 normal;
    vector4 point;
    float depth;
};
int shape_contact_count(coll_shape* a, coll_shape* b, int max, contact_rec* out);       // 0x40bfe0

struct Contact { vector4 p, n; };        // 0x20 stored contact (point, normal)

// 0x427ca0: std::_Uninitialized_fill_n for Contact, called out of line from push_back's fast path
// @ 0x427ca0
template void std::_Uninitialized_fill_n<Contact*, unsigned int, Contact, std::allocator<Contact> >(Contact*, unsigned int, const Contact&, std::allocator<Contact>&);

// the same function seen from its callers: kept out of line (the instantiation above would be inlined)
void __cdecl ufill_ext(Contact* p, unsigned int n, const Contact& v, std::allocator<Contact>& al);     // 0x427ca0

// std::vector<Contact> written out (VC7.1 layout: empty allocator slot, first, last, end)
struct contact_vec {
    std::allocator<Contact> _Alval;
    Contact *_Myfirst, *_Mylast, *_Myend;
    typedef unsigned size_type;
    struct iterator {
        Contact* _Ptr;
        iterator() {}
        iterator(Contact* p) : _Ptr(p) {}
        iterator operator+(int off) const;                      // 0x40f880
    };
    size_type capacity() const { return _Myfirst == 0 ? 0 : _Myend - _Myfirst; }
    size_type size() const { return _Myfirst == 0 ? 0 : _Mylast - _Myfirst; }
    iterator begin() { return iterator(_Myfirst); }
    iterator end() { return iterator(_Mylast); }
    Contact* _Ufill(Contact* p, size_type n, const Contact& v) { ufill_ext(p, n, v, _Alval); return p + n; }
    void _Insert_n(Contact* _Where, size_type _Count, const Contact& _Val);            // 0x427590 (s00427000)
    __forceinline iterator insert(iterator _Where, const Contact& _Val)
    {
        size_type _Off = size() == 0 ? 0 : _Where._Ptr - _Myfirst;
        _Insert_n(_Where._Ptr, (size_type)1, _Val);
        return begin() + _Off;
    }
    __forceinline void push_back(const Contact& _Val)
    {
        if (size() < capacity())
            _Mylast = _Ufill(_Mylast, 1, _Val);
        else
            insert(end(), _Val);
    }
    void _Tidy() { if (_Myfirst != 0) operator delete(_Myfirst); _Myfirst = 0; _Mylast = 0; _Myend = 0; }
};

// ---- the entity family (vtable 0x54b460 and derived) ------------------------------------------------------------
struct entity {
    entity();                                                       // 0x41fba0
    virtual ~entity();                                              // 0x4213f0 (slot 0 holds the deleting dtor 0x4214e0)
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
    char pad4[0x58 - 4];
    rgl::matrix local;                                              // 0x58
    coll_shape* shape;                                              // 0x98
    char pad9c[0x15c - 0x9c];
};

// base of the rigid bodies: restitution, contact list, accumulated position shift; slots 25..27 are the
// point-velocity / impulse virtuals (stubs in the collision entity, real in dynamic_entity)
struct rigid_base : entity {
    float restitution;                                              // 0x15c (0.9)
    contact_vec contacts;                                           // 0x160
    char pad170[0x1c4 - 0x170];
    vector4 shift;                                                  // 0x1c4
    virtual vector4* point_velocity(vector4* out, const vector4* p);         // 25 (0x64)
    virtual void apply_impulse(const vector4* p, const vector4* j);          // 26 (0x68)
    virtual float impulse_response(const vector4* p, const vector4* n);      // 27 (0x6c)
    bool collide_shape(coll_shape* s);                              // 0x427900
};

static __forceinline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline float vdot(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
// @ 0x427900
bool rigid_base::collide_shape(coll_shape* s)
{
    if (shape == 0)
        return false;
    contact_rec c[4];
    int n = shape_contact_count(shape, s, 4, c);
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
        contacts.push_back(rec);
        float vn = nrm.x * v.x + nrm.y * v.y + nrm.z * v.z;
        vector4 t = vsub(v, vmul(nrm, vn));
        if (0.0f > vn) {
            double j = -((restitution + 1.0) * vn);
            double mag = j / impulse_response(&pt, &nrm);
            vector4 imp(nrm.x * mag, nrm.y * mag, nrm.z * mag);
            apply_impulse(&pt, &imp);
        }
        if (vdot(t, t) > 0.01f) {
            vector4 td;
            rgl::normalize(td, t);
            double j = vdot(td, v) * -0.5;
            double mag = j / impulse_response(&pt, &td);
            vector4 imp(td.x * mag, td.y * mag, td.z * mag);
            apply_impulse(&pt, &imp);
        }
        if (c[i].depth > 0.01f) {
            float d = c[i].depth * 0.5f;
            vector4 corr = vmul(c[i].normal, d);
            shift.x += corr.x;
            shift.y += corr.y;
            shift.z += corr.z;
        }
    }
    return true;
}

// ---- collision / skeleton / static entities -----------------------------------------------------------------------
// vtable 0x54b548 "collision_entity": a mesh entity with a restitution and a contact list; slots 25..27 are the
// default stubs of s00421110 (0x421520 / 0x421540 / 0x421550)
struct collision_entity : entity {
    float restitution;                                              // 0x15c
    contact_vec contacts;                                           // 0x160
    __forceinline collision_entity() { contacts._Myfirst = 0; contacts._Mylast = 0; contacts._Myend = 0; restitution = 0.9f; }
    virtual ~collision_entity();                                    // 0x427d20 deleting / 0x427cd0 body
    virtual vector4* point_velocity(vector4* out, const vector4* p);         // 25
    virtual void apply_impulse(const vector4* p, const vector4* j);          // 26
    virtual float impulse_response(const vector4* p, const vector4* n);      // 27
};
// @ 0x427cd0
collision_entity::~collision_entity()
{
    contacts._Tidy();
}

struct FxDef;
// vtable 0x54b730 "skeleton_entity": a collision entity carrying an effect/skeleton object at +0x170
struct skeleton_entity : collision_entity {
    FxDef* fx;                                                      // 0x170
    skeleton_entity();                                              // 0x427d80
    virtual ~skeleton_entity();                                     // 0x427e80 deleting / 0x427e10 body
    virtual int type_mask();                                        // 1: 0x427dc0
    virtual rgl::string type_name();                                // 2: 0x427dd0
    virtual void update(float dt);                                  // 7: 0x427df0
};
struct FxDef {
    ~FxDef();                                                       // 0x421680
    void update(float dt, const rgl::matrix* m);                    // 0x421df0
};

// @ 0x427d80
skeleton_entity::skeleton_entity() : fx(0) {}
// @ 0x427dc0
int skeleton_entity::type_mask() { return 0xb; }
// @ 0x427dd0
rgl::string skeleton_entity::type_name() { return rgl::string("skeleton_entity"); }
// @ 0x427df0
void skeleton_entity::update(float dt)
{
    if (fx)
        fx->update(dt, &local);
}
// @ 0x427e10
skeleton_entity::~skeleton_entity()
{
    delete fx;
}

// vtable 0x54b7b8 "static_entity": a plain mesh entity
struct static_entity : entity {
    virtual ~static_entity();                                       // 0x4280f0 (deleting)
    virtual rgl::string type_name();                                // 2: 0x4280c0
    virtual bool awake();                                           // 10: 0x4280e0
};
// @ 0x4280c0
rgl::string static_entity::type_name() { return rgl::string("static_entity"); }
// @ 0x4280e0
bool static_entity::awake() { return false; }
// @ 0x4280f0
static_entity::~static_entity() {}

// ---- particle emitter pool and per-frame step of the world (layouts shared with s00421110 / s0041ee20) -----------
struct Particle { int a[25]; };       // 100 bytes
// 0x1bc-byte particle emitter (s00412150 / s00421110 `Emitter`): particle vector at +0xc, noise index at +0x1b8
struct Emitter {
    char pad0[0xc];
    std::vector<Particle> particles;                                // 0x0c (first +0x10)
    char pad1[0xf4 - 0x1c];
    rgl::matrix world;                                              // 0xf4 (64 bytes)
    char pad2[0x1b8 - 0x134];
    int noise_index;                                                // 0x1b8
    Emitter(const int& idx) : particles() { noise_index = idx; init(); }
    void init();                                                    // 0x420b40
    void update(const float* dt, const rgl::matrix& frame);         // 0x412150
};
template<class T> struct ptr_vec {                                  // std::vector<T*> with push_back out of line
    std::allocator<T*> al;
    T** first; T** last; T** end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    struct iter {                                                   // std::_Ptrit
        T** cur;
        iter(T** p) : cur(p) {}
        T*& operator*() const { return *cur; }
        iter& operator+=(int n) { cur += n; return *this; }
        iter operator+(int n) const { iter t = *this; return t += n; }
    };
    iter begin() { return iter(first); }
    T*& operator[](unsigned i) { return first[i]; }
    T*& at(unsigned i) { return *(begin() + i); }
    void push_back(T* const& v);                                    // 0x420650
    void tidy() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }
    __forceinline void push(T* v) { push_back(v); }
};

struct world {
    char pad0[0x28];
    char pad28[0x7c - 0x28];
    std::vector<entity*> ents;                                      // 0x7c (first at 0x80)
    std::vector<entity*> ents2;                                     // 0x8c (first at 0x90)
    char pad9c[4];
    ptr_vec<int> removed;                                           // 0xa0 (first at 0xa4)
    char padb0[0x32c - 0xb0];
    ptr_vec<Emitter> emitters;                                      // 0x32c (first at 0x330)
    unsigned live;                                                  // 0x33c
    int noise_counter;                                              // 0x340
    Emitter* alloc_emitter();                                       // 0x427ea0
    void step(float dt);                                            // 0x427f40
    void release_links(entity* e);                                  // 0x41ee20
    void partition_items();                                         // 0x41efd0
    void remove_entity(int* id);                                    // 0x41f0d0
    void insert_obj(entity* e);                                     // 0x421600
};

// @ 0x427ea0
Emitter* world::alloc_emitter()
{
    if (live < emitters.size()) {
        Emitter* e = emitters[live];
        live++;
        return e;
    }
    Emitter* e = new Emitter(noise_counter);
    emitters.push(e);
    ++live;
    return e;
}

// @ 0x427f40
void world::step(float dt)
{
    for (std::vector<entity*>::iterator it = ents2.begin(); it < ents2.end(); ++it) {
        (*it)->sync();
        ents.push_back(*it);
    }
    ents2.resize(0, 0);
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it)
        (*it)->update(dt);
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it) {
        if ((*it)->awake()) {
            release_links(*it);
            insert_obj(*it);
        }
    }
    for (Emitter** it = emitters.first; it < emitters.last; ++it)
        (*it)->update(&dt, (*it)->world);
    partition_items();
    for (int** it = removed.first; it < removed.last; ++it)
        remove_entity(*it);
    removed.tidy();
}

// ---- strategic entity destructor (vtable 0x54b5d0; layout in s00421110's fx.h) ---------------------------------------
struct Mgr { void release(void* p); };                              // 0x4175f0 (this = 0x5d6fa0)
extern Mgr g_mgr;                                                   // 0x5d6fa0
struct Stash {                                                      // object at +0x28c
    ~Stash();                                                       // 0x41f4e0
};
struct StrategicEntity : entity {
    char pad15c[0x288 - 0x15c];
    void* p288;                                                     // 0x288 (released through the manager)
    Stash* f28c;                                                    // 0x28c
    int* f290;                                                      // 0x290 (plain array)
    char pad294[0x304 - 0x294];
    FxDef* fx;                                                      // 0x304
    ptr_vec<Emitter> emitters;                                      // 0x308 (first at 0x30c)
    virtual ~StrategicEntity();                                    // 0x428220 deleting / 0x428120 body
};
// @ 0x428120
StrategicEntity::~StrategicEntity()
{
    if (fx != 0) {
        delete fx;
        fx = 0;
    }
    if (f28c != 0) {
        delete f28c;
        f28c = 0;
    }
    if (f290 != 0) {
        delete f290;
        f290 = 0;
    }
    if (p288 != 0) {
        g_mgr.release(p288);
        p288 = 0;
    }
    for (unsigned i = 0; i < emitters.size(); i++) {
        Emitter* e = emitters.at(i);
        delete e;
    }
    emitters.tidy();
}
