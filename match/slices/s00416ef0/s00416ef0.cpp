// Slice s00416ef0: bounding-box growth, effect technique lookup (ID3DXEffect), body-part record and its stream
// writer, a text-stream ctor/dtor pair, vector4 array (de)serialisers and the scalar/vector deleting
// destructors of the resource classes.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <new>
#include <vector>
#include "mbres.h"

void mb_error_noreturn_unused();               // (nothing: mb_error is declared in mbstream.h)

// @ 0x416ef0
void aabb::extend(const rgl::vector4* p)
{
    if (p->x < min.x)
        min.x = p->x;
    if (p->y < min.y)
        min.y = p->y;
    if (p->z < min.z)
        min.z = p->z;
    if (p->x > max.x)
        max.x = p->x;
    if (p->y > max.y)
        max.y = p->y;
    if (p->z > max.z)
        max.z = p->z;
}

// inline vector helpers the original used (add and scale by a constant)
static inline rgl::vector4 add(const rgl::vector4& a, const rgl::vector4& b)
{
    return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z);
}
static inline rgl::vector4 half(const rgl::vector4& a)
{
    return rgl::vector4(a.x * 0.5f, a.y * 0.5f, a.z * 0.5f);
}

// @ 0x416f70
void aabb::update()
{
    if (min.x <= max.x) {
        rgl::vector4* c = &center;
        *c = half(add(max, min));
        float dx = max.x - c->x;
        float dy = max.y - c->y;
        float dz = max.z - c->z;
        radius = (float)sqrt(dx * dx + dy * dy + dz * dz);
    }
}

// Effect technique set, vtable 0x54ac08, 0x2c bytes.
struct effect_set : res_base {
    char resolved;                 // +0x18
    D3DXHANDLE h[4];               // +0x1c  techniques: name, name_SHDW (falls back to [0]),
                                   //        name_SHDWNVIDIA (falls back to [1]), name_PRESHADED (falls back to [0])
    effect_set();                  // 0x417010
    virtual ~effect_set();         // 0x417fb0
    virtual int type();            // 0x417040
    int resolve(ID3DXEffect* fx);  // 0x417050
};

// @ 0x417010
effect_set::effect_set() : resolved(0)
{
    for (int i = 0; i < 4; i++)
        h[i] = 0;
}

// @ 0x417040
int effect_set::type() { return 10; }

// @ 0x417050
int effect_set::resolve(ID3DXEffect* fx)
{
    tstring tmp;
    if (!resolved) {
        tmp = name;
        h[0] = fx->GetTechniqueByName(tmp.c_str());
        if (!h[0])
            mb_error("Unable to get technique with name %s", tmp.c_str());
        tmp = name;
        {
            tstring suffix("_SHDW");
            tmp.append(suffix);
        }
        h[1] = fx->GetTechniqueByName(tmp.c_str());
        if (!h[1])
            h[1] = h[0];
        tmp = name;
        {
            tstring suffix("_SHDWNVIDIA");
            tmp.append(suffix);
        }
        h[2] = fx->GetTechniqueByName(tmp.c_str());
        if (!h[2])
            h[2] = h[1];
        tmp = name;
        {
            tstring suffix("_PRESHADED");
            tmp.append(suffix);
        }
        h[3] = fx->GetTechniqueByName(tmp.c_str());
        if (!h[3])
            h[3] = h[0];
        resolved = 1;
    }
    return 0;
}

// Collision-shape base (vtable 0x54a4b8 and relatives; see s0040cfe0 for the full interface).
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base();                              // slot 0
    virtual int shape_type();                           // slot 1 (0 sphere, 1 capsule, 2 face, 3 composite, 4 manifold)
    virtual void slot2();
    virtual void slot3(stream_base* s);
    virtual void save(stream_base* s);                  // slot 4
};

// Body part record, vtable 0x54ac58, 0x1c bytes: a name and its collision shape.
struct body_part : res_base {
    shape_base* shape;             // +0x18
    body_part();                   // 0x4171c0
    virtual ~body_part();          // 0x4177b0 (deleting)
    virtual int type();            // 0x4171f0
};

// @ 0x4171c0
body_part::body_part() : shape(0) {}

// @ 0x4171f0
int body_part::type() { return 6; }

// Deleting destructor 0x4177b0 is generated from this.
body_part::~body_part() { delete shape; }

// @ 0x417200
stream_base& w_body_part(stream_base& s, body_part* b)
{
    tstring unused;
    s.w_tstr(&b->name);
    if (b->shape->shape_type() == 3)
        s.w_cstr("composite");
    else if (b->shape->shape_type() == 0)
        s.w_cstr("sphere");
    else if (b->shape->shape_type() == 1)
        s.w_cstr("capsule");
    else if (b->shape->shape_type() == 2)
        s.w_cstr("face");
    else if (b->shape->shape_type() == 4)
        s.w_cstr("manifold");
    else
        mb_error("unrecognized body part");
    b->shape->save(&s);
    return s;
}

// Root of the stream classes' own hierarchy as seen by the constructor at 0x4172d0: vtable 0x54a740, the
// name string at +4 and the text buffer at +0x20.
struct stream_root {
    virtual void reset();
    tstring name;                  // +4
    int state;                     // +0x14
    char autoflush;                // +0x18
    int f1c;                       // +0x1c
    tstring buf;                   // +0x20
    stream_root() {}
};

// Third text-stream class (vtable 0x54ac88; its slots 13/14 are 0x415cf0/0x415c80, the typed writers are no-ops).
struct tstream3 : stream_root {
    FILE* fp;                      // +0x30
    char failed;                   // +0x34
    tstream3();                    // 0x4172d0
    virtual void reset();
};

// @ 0x4172d0
tstream3::tstream3()
{
    autoflush = 0;
    f1c = 0;
    fp = 0;
    state = 0;
    failed = 1;
}

// The 0x54a890 stream class' destructor (the class itself is declared in s00414960 as text_stream2).
struct tstream2 : stream_root {
    FILE* fp;
    char failed;
    ~tstream2();                   // 0x417310
    virtual void reset();
};

// @ 0x417310
tstream2::~tstream2() {}

// keys[n] plus 16-byte records (provisional name): written/read as `int key; y z w x` (quaternion-like).
struct key_array {
    rgl::vector4* data;            // +0
    int* keys;                     // +4
    int n;                         // +8
};

// @ 0x417330
stream_base& w_key_array_yzwx(stream_base& s, key_array* a)
{
    s.w_int(&a->n);
    for (int i = 0; i < a->n; i++) {
        s.w_int(&a->keys[i]);
        rgl::vector4* q = &a->data[i];
        s.w_float(&q->y).w_float(&q->z).w_float(&q->w).w_float(&q->x);
    }
    return s;
}

// @ 0x4173b0
stream_base& r_key_array_yzwx(stream_base& s, key_array* a)
{
    s.r_int(&a->n);
    if (a->n != 0) {
        a->keys = new int[a->n];
        a->data = new rgl::vector4[a->n];
        for (int i = 0; i < a->n; i++) {
            s.r_int(&a->keys[i]);
            rgl::vector4* q = &a->data[i];
            s.r_float(&q->y).r_float(&q->z).r_float(&q->w).r_float(&q->x);
        }
    }
    return s;
}

struct f3 { float a, b, c; };

// @ 0x417480
int __fastcall vec12_size(std::vector<f3>* v) { return (int)v->size(); }

// @ 0x4174a0
stream_base& w_key_array_xyz(stream_base& s, key_array* a)
{
    s.w_int(&a->n);
    for (int i = 0; i < a->n; i++) {
        s.w_int(&a->keys[i]);
        rgl::vector4* q = &a->data[i];
        s.w_float(&q->x).w_float(&q->y).w_float(&q->z);
    }
    return s;
}

// @ 0x417520
stream_base& r_key_array_xyz(stream_base& s, key_array* a)
{
    s.r_int(&a->n);
    if (a->n != 0) {
        a->keys = new int[a->n];
        a->data = new rgl::vector4[a->n];
        for (int i = 0; i < a->n; i++) {
            s.r_int(&a->keys[i]);
            rgl::vector4* q = &a->data[i];
            s.r_float(&q->x).r_float(&q->y).r_float(&q->z);
        }
    }
    return s;
}

// @ 0x4175f0
void __stdcall clear_f14(res_base* r) { r->f14 = 0; }

// ---- scalar/vector deleting destructors (generated from the virtual destructors) ----
// 0x417600 (skin), 0x417690 (material; calls ~material 0x4168f0), 0x4177b0 (body_part, above).
skin::~skin() {}
material::~material() {}

// Particle system (0x260 bytes, vtable 0x54a6f4; full layout in s00412150): only its size and the external
// destructor 0x4127f0 matter here. The empty constructor is NOT the original; it exists so that cl emits
// the vtable and the scalar deleting destructor ??_GPsys (0x417730).
struct Psys {
    virtual ~Psys();               // 0x4127f0, defined in another TU (deleting: 0x417730)
    char pad[0x25c];
    Psys();
};
Psys::Psys() {}

// A delete[] expression on each class makes cl emit the vector deleting destructor (??_E, the flags & 2 path)
// as the vtable's slot 0 instead of ??_G; that is what 0x417600, 0x417690, 0x417730 and 0x4177b0 are.
// This function itself is not in the original.
void delete_arrays(skin* a, material* b, Psys* c, body_part* d)
{
    new skin[3];
    new material[3];
    new Psys[3];
    new body_part[3];
    delete[] a;
    delete[] b;
    delete[] c;
    delete[] d;
}
