// mem_stream.cpp (0x417010..0x417baf): resource-record helpers (effect technique set, body part), stream
// ctor/dtor of two text stream classes, key-array serialisers, the deleting destructors of four resource
// classes, key_array assignment and the in-memory binary stream's slots.
// Migrated from slices s00416ef0 (0x417010..0x4177b0) and s00417850 (0x417850..0x417b50).
// Boundaries: content only (no call crosses either side); see src/rgl/MIGRATION.md.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <string.h>
#include <new>
#include <vector>
#include "./rgl_string.h"
#include "./rgl_math.h"
#include "./stream.h"
#include "./resource.h"
#include "./mesh.h"
#include "./mem_stream.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../include/keyframes.h"
#include "../include/mat4.h"
#include "../include/mesh_prims.h"
#include "../include/vec.h"

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
    rgl::string tmp;
    if (!resolved) {
        tmp = name;
        h[0] = fx->GetTechniqueByName(tmp.c_str());
        if (!h[0])
            mb_error("Unable to get technique with name %s", tmp.c_str());
        tmp = name;
        {
            rgl::string suffix("_SHDW");
            tmp.append(suffix);
        }
        h[1] = fx->GetTechniqueByName(tmp.c_str());
        if (!h[1])
            h[1] = h[0];
        tmp = name;
        {
            rgl::string suffix("_SHDWNVIDIA");
            tmp.append(suffix);
        }
        h[2] = fx->GetTechniqueByName(tmp.c_str());
        if (!h[2])
            h[2] = h[1];
        tmp = name;
        {
            rgl::string suffix("_PRESHADED");
            tmp.append(suffix);
        }
        h[3] = fx->GetTechniqueByName(tmp.c_str());
        if (!h[3])
            h[3] = h[0];
        resolved = 1;
    }
    return 0;
}

// @ 0x4171c0
body_part::body_part() : shape(0) {}

// @ 0x4171f0
int body_part::type() { return 6; }

// @ 0x417200
stream_base& w_body_part(stream_base& s, body_part* b)
{
    rgl::string unused;
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

// @ 0x4172d0
tstream3::tstream3() {}

// @ 0x417310
text_stream2::~text_stream2() {}

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

// Out-of-line std::vector<T>::size() for a 12-byte T (the callers 0x41a6b0, 0x43edf0, 0x4b5790 pass the vector
// in ECX). T = key_array is a guess: any 12-byte element gives these bytes (the slice had a fastcall stand-in).
// 0x417480 vector::size
template std::vector<key_array>::size_type std::vector<key_array>::size() const;

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
void __stdcall clear_f14(named_object* r) { r->f14 = 0; }

// ---- vector deleting destructors (compiler-generated) ----------------------------------------------
// 0x417600 skin, 0x417690 material, 0x417730 rgl::mesh, 0x4177b0 body_part. cl emits a real ??_E (the
// flags & 2 array path; otherwise ??_E is only a weak alias of ??_G) when the object contains an array new
// of the class. The original's array news are not in the code that survived /OPT:REF, so this stand-in
// supplies them; it is not in the binary. material's array path inlines its (inline) destructor, the scalar
// path calls the out-of-line copy 0x4168f0; rgl::mesh's destructor 0x4127f0 is out of line in mesh.obj.
void deleting_dtor_standin(skin* a, material* b, rgl::mesh* c, body_part* d)
{
    new skin[3];
    new material[3];
    new rgl::mesh[3];
    new body_part[3];
    delete[] a;
    delete[] b;
    delete[] c;
    delete[] d;
}

// @ 0x417850
void key_array::operator=(const key_array& o)
{
    n = o.n;
    if (keys) { delete[] keys; keys = 0; }
    if (data) { delete[] data; data = 0; }
    if (o.n) {
        keys = new int[n];
        data = new rgl::vector4[n];
        for (int i = 0; i < n; i++)
            data[i] = o.data[i];
        memcpy(keys, o.keys, n * 4);
    }
}

// ---- in-memory binary stream slots ---------------------------------------------------------
// @ 0x417930
stream_base& mem_stream::w_stream(stream_base* o)
{
    int n = o->buf.len - o->buf.pos;
    buf.write(&n, 4);
    if (n > 0) {
        const char* p;
        if (o->buf.buf)
            p = o->buf.pos + o->buf.buf;
        else
            p = (const char*)&o->buf.buf;
        buf.write(p, n);
    }
    return *this;
}
// @ 0x417990
stream_base& mem_stream::w_cstr(const char* s)
{
    int n = strlen(s);
    buf.write(&n, 4);
    buf.write(s, n);
    return *this;
}
// @ 0x4179e0
stream_base& mem_stream::w_int(int* v) { buf.write(v, 4); return *this; }
// @ 0x417a00
stream_base& mem_stream::w_i64(__int64* v) { buf.write(v, 8); return *this; }
// @ 0x417a20
stream_base& mem_stream::w_tstr(const rgl::string* s)
{
    int n = s->len;
    buf.write(&n, 4);
    buf.write(s->c_str(), n);
    return *this;
}
// @ 0x417a70
stream_base& mem_stream::w_byte(unsigned char* v) { buf.write(v, 1); return *this; }
// @ 0x417a90
stream_base& mem_stream::r_tstr(rgl::string* v)
{
    int n;
    r_int(&n);
    v->assign("");
    for (int i = 0; i < n; i++) {
        char c;
        buf.read(&c, 1);
        v->push_back(c);
    }
    return *this;
}
// @ 0x417af0
stream_base& mem_stream::r_byte(char* v) { buf.read(v, 1); return *this; }
// @ 0x417b10
stream_base& mem_stream::r_int(int* v) { buf.read(v, 4); return *this; }
// @ 0x417b30
stream_base& mem_stream::r_i64(__int64* v) { buf.read(v, 8); return *this; }
// @ 0x417b50
stream_base& mem_stream::r_stream(stream_base* o)
{
    int n;
    r_int(&n);
    o->reset();
    for (int i = 0; i < n; i++) {
        char c;
        buf.read(&c, 1);
        o->buf.push_back(c);
    }
    return *this;
}
