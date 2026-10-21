// Slice s00408110: render-state helpers, and the shape classes served by vtables 0x54a4b8 (sphere), 0x54a540 (capsule).
// Class names are provisional ("capsule" is the string returned by 0x408630).
#include "rgl.h"
#include <math.h>
#include <new>
using namespace rgl;

// Stub of IDirect3DDevice9: only the vtable slot positions matter (d3d9.h does not compile here: sal.h missing).
struct d3d_device {
    virtual void __stdcall pad0();
    virtual void __stdcall pad1();
    virtual void __stdcall pad2();
    virtual void __stdcall pad3();
    virtual void __stdcall pad4();
    virtual void __stdcall pad5();
    virtual void __stdcall pad6();
    virtual void __stdcall pad7();
    virtual void __stdcall pad8();
    virtual void __stdcall pad9();
    virtual void __stdcall pad10();
    virtual void __stdcall pad11();
    virtual void __stdcall pad12();
    virtual void __stdcall pad13();
    virtual void __stdcall pad14();
    virtual void __stdcall pad15();
    virtual void __stdcall pad16();
    virtual void __stdcall pad17();
    virtual void __stdcall pad18();
    virtual void __stdcall pad19();
    virtual void __stdcall pad20();
    virtual void __stdcall pad21();
    virtual void __stdcall pad22();
    virtual void __stdcall pad23();
    virtual void __stdcall pad24();
    virtual void __stdcall pad25();
    virtual void __stdcall pad26();
    virtual void __stdcall pad27();
    virtual void __stdcall pad28();
    virtual void __stdcall pad29();
    virtual void __stdcall pad30();
    virtual void __stdcall pad31();
    virtual void __stdcall pad32();
    virtual void __stdcall pad33();
    virtual void __stdcall pad34();
    virtual void __stdcall pad35();
    virtual void __stdcall pad36();
    virtual void __stdcall pad37();
    virtual void __stdcall pad38();
    virtual void __stdcall pad39();
    virtual void __stdcall pad40();
    virtual void __stdcall pad41();
    virtual void __stdcall pad42();
    virtual void __stdcall pad43();
    virtual long __stdcall SetTransform(int state, const rgl::matrix* m);
    virtual void __stdcall pad45();
    virtual void __stdcall pad46();
    virtual void __stdcall pad47();
    virtual void __stdcall pad48();
    virtual void __stdcall pad49();
    virtual void __stdcall pad50();
    virtual void __stdcall pad51();
    virtual void __stdcall pad52();
    virtual void __stdcall pad53();
    virtual void __stdcall pad54();
    virtual void __stdcall pad55();
    virtual void __stdcall pad56();
    virtual long __stdcall SetRenderState(int state, unsigned value);
    virtual void __stdcall pad58();
    virtual void __stdcall pad59();
    virtual void __stdcall pad60();
    virtual void __stdcall pad61();
    virtual void __stdcall pad62();
    virtual void __stdcall pad63();
    virtual void __stdcall pad64();
    virtual long __stdcall SetTexture(unsigned stage, void* tex);
    virtual void __stdcall pad66();
    virtual void __stdcall pad67();
    virtual void __stdcall pad68();
    virtual void __stdcall pad69();
    virtual void __stdcall pad70();
    virtual void __stdcall pad71();
    virtual void __stdcall pad72();
    virtual void __stdcall pad73();
    virtual void __stdcall pad74();
    virtual void __stdcall pad75();
    virtual void __stdcall pad76();
    virtual void __stdcall pad77();
    virtual void __stdcall pad78();
    virtual void __stdcall pad79();
    virtual void __stdcall pad80();
    virtual void __stdcall pad81();
    virtual void __stdcall pad82();
    virtual long __stdcall DrawPrimitiveUP(int type, unsigned count, const void* data, unsigned stride);
    virtual void __stdcall pad84();
    virtual void __stdcall pad85();
    virtual void __stdcall pad86();
    virtual void __stdcall pad87();
    virtual void __stdcall pad88();
    virtual void __stdcall pad89();
    virtual void __stdcall pad90();
    virtual void __stdcall pad91();
    virtual long __stdcall SetVertexShader(void* shader);
};

// Archive stream (reader/writer share one vtable shape). Slots: 3 read float*, 8 read int*, 14 write float*, 19 write int*.
struct stream {
    virtual void s0(); virtual void s1(); virtual void s2();
    virtual stream* read_f(float* p);                                   // slot 3
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void read_i(void* p);                                       // slot 8
    virtual void s9(); virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13();
    virtual stream* write_f(float* p);                                  // slot 14
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual void write_i(void* p);                                      // slot 19
};

// rgl::matrix plus the point transform (0x408460) that callers inline.
struct pos : matrix {
    __declspec(noinline) vector4 transform_point(const vector4& p) const;   // 0x408460
    __forceinline vector4 transform_point_inl(const vector4& p) const       // same body, as inlined by callers
    {
        return vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                       s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                       s.z * p.x + f.z * p.y + u.z * p.z + o.z);
    }
};

// The out-of-line 0x408460 as the callers outside this TU see it (no interprocedural register info).
struct pos_ext : matrix {
    vector4 transform_point(const vector4& p) const;                    // 0x408460
};

// Global render state object at 0xa17910: matrix stack, D3D device and current world matrix.
struct renderer {
    matrix stack[20];                // 0x000
    int sp;                          // 0x500
    char pad504[0x618 - 0x504];
    d3d_device* dev;                 // 0x618
    char pad61c[0x794 - 0x61c];
    matrix cur;                      // 0x794 (world matrix)
    __declspec(noinline) void translate(const vector4& t);         // 0x408170
    __declspec(noinline) void pop();                              // 0x4081b0
    __declspec(noinline) void draw_sphere(float r);               // 0x4081f0
    void draw_polygon(const void* v, int n); // 0x408e60
};
extern renderer g_render;   // 0xa17910

// Shape with vtable 0x54a4b8 (the pilot's "sphere"); only members used here.
struct sphere {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    vector4 v20, v30;
    float radius;
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4();
    virtual void s5(); virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16();
    virtual void save(stream* s);            // slot 17  0x4084c0
    virtual void load(stream* s);            // slot 18  0x408540
    virtual void s19();
    virtual void draw();                     // slot 20  0x408110
    virtual void scale(const float* s);      // slot 21  0x4083f0
    virtual void transform(const pos* m);    // slot 22  0x408420
    virtual void s23();
    virtual void get_min(float* out);        // slot 24  0x4085a0
    virtual void get_max(float* out);        // slot 25  0x4085d0
};

// Common header of the shapes served by vtables 0x54a540 / 0x54a588 (ctor inlined into the derived ctors).
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base();
};

// Shape with vtable 0x54a540.
struct capsule : shape_base {
    vector4 v20, v30, v40, v50;
    float radius;
    capsule();                                       // 0x408600
    virtual ~capsule();
    virtual void s1();
    virtual string type_name() const;                // slot 2   0x408630
    virtual void save(stream* s);                    // slot 3   0x408870
    virtual void load(stream* s);                    // slot 4   0x408910
    virtual void s5(); virtual void s6();
    virtual void scale(const float* s);              // slot 7   0x408720
    virtual void transform(const pos* m);            // slot 8   0x408770
    virtual void s9(); virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13();
    virtual capsule* create_new();                   // slot 14  0x408650
    virtual void copy_from(const capsule* o);        // slot 15  0x408690
};

// Polygon shape, vtable 0x54a588 (name "face" from 0x4089c0).
struct face : shape_base {
    vector4 v20, v30, v40, v50;
    vector4 v[4];                // 0x60
    vector4 normal;              // 0xa0
    int count;                   // 0xb0
    face();                                          // 0x408980
    virtual ~face();
    virtual void s1();
    virtual string type_name() const;                // slot 2   0x4089c0
    virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13();
    virtual void copy_from(const face* o);           // slot 14  0x4089e0
};

static __forceinline void write3_inl(stream* s, float* v) { s->write_f(v)->write_f(v + 1)->write_f(v + 2); }
static __forceinline void read3_inl(stream* s, float* v) { s->read_f(v)->read_f(v + 1)->read_f(v + 2); }
static inline void set3(float* o, const float& x, const float& y, const float& z) { o[0] = x; o[1] = y; o[2] = z; }
__declspec(noinline) stream* write_vec3(stream* s, float* v);   // 0x408510
__declspec(noinline) stream* read_vec3(stream* s, float* v);    // 0x408570


// @ 0x408110
void sphere::draw()
{
    g_render.stack[g_render.sp] = g_render.cur;
    g_render.sp = g_render.sp + 1;
    g_render.translate(v30);
    g_render.draw_sphere(radius);
    g_render.pop();
}
// @ 0x408170
void renderer::translate(const vector4& t)
{
    vector4* o = &cur.o;
    o->x += t.x;
    o->y = t.y + o->y;
    o->z = t.z + o->z;
    dev->SetTransform(0x100, &cur);
}
// @ 0x4081b0
void renderer::pop()
{
    sp = sp - 1;
    cur = stack[sp];
    dev->SetTransform(0x100, &cur);
}
// @ 0x4081f0
void renderer::draw_sphere(float r)
{
    float pts[12288];
    int n = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            vector4 c[4];
            for (int k = 0; k < 4; k++) {
                int lat = i, lon = j;
                if (k == 1 || k == 2) lat = i + 1;
                if (k == 2 || k == 3) lon = j + 1;
                float a = (float)lon * 0.7853982f;
                float b = (float)lat * 0.62831855f - 1.5707964f;
                c[k].x = (float)(cos(a) * cos(b) * r);
                c[k].y = (float)(sin(a) * cos(b) * r);
                c[k].z = (float)(sin(b) * r);
            }
            pts[n] = c[0].x; pts[n + 1] = c[0].y; pts[n + 2] = c[0].z; n += 3;
            pts[n] = c[1].x; pts[n + 1] = c[1].y; pts[n + 2] = c[1].z; n += 3;
            pts[n] = c[1].x; pts[n + 1] = c[1].y; pts[n + 2] = c[1].z; n += 3;
            pts[n] = c[2].x; pts[n + 1] = c[2].y; pts[n + 2] = c[2].z; n += 3;
            pts[n] = c[2].x; pts[n + 1] = c[2].y; pts[n + 2] = c[2].z; n += 3;
            pts[n] = c[3].x; pts[n + 1] = c[3].y; pts[n + 2] = c[3].z; n += 3;
            pts[n] = c[3].x; pts[n + 1] = c[3].y; pts[n + 2] = c[3].z; n += 3;
            pts[n] = c[0].x; pts[n + 1] = c[0].y; pts[n + 2] = c[0].z; n += 3;
        }
    }
    dev->SetTexture(0, 0);
    dev->SetRenderState(0x8b, 0xffbffff2);
    dev->SetVertexShader(0);
    dev->DrawPrimitiveUP(2, 0xa0, pts, 12);
}
// @ 0x4083f0
void sphere::scale(const float* s)
{
    v20.x = s[0] * v20.x;
    v20.y = s[1] * v20.y;
    v20.z = s[2] * v20.z;
    radius = (float)fabs(s[0]) * radius;
}
// @ 0x408420
void sphere::transform(const pos* m)
{
    v30 = ((const pos_ext*)m)->transform_point(v20);
}
// @ 0x4084c0
void sphere::save(stream* s)
{
    write_vec3(s->write_f(&radius), &v20.x);
    v30 = v20;
    s->write_i(&m8);
}
// @ 0x408510
stream* write_vec3(stream* s, float* v)
{
    s->write_f(v)->write_f(v + 1)->write_f(v + 2);
    return s;
}
// @ 0x408540
void sphere::load(stream* s)
{
    read_vec3(s->read_f(&radius), &v20.x);
    s->read_i(&m8);
}
// @ 0x408570
stream* read_vec3(stream* s, float* v)
{
    s->read_f(v)->read_f(v + 1)->read_f(v + 2);
    return s;
}
// @ 0x4085a0
void sphere::get_min(float* out)
{
    set3(out, v30.x - radius, v30.y - radius, v30.z - radius);
}
// @ 0x4085d0
void sphere::get_max(float* out)
{
    set3(out, v30.x + radius, v30.y + radius, v30.z + radius);
}
// @ 0x408600
capsule::capsule() {}
// @ 0x408630
string capsule::type_name() const { return "capsule"; }
// @ 0x408650
capsule* capsule::create_new() { return new capsule; }
// @ 0x408690
void capsule::copy_from(const capsule* o)
{
    m4 = o->m4;
    v40 = o->v40;
    v50 = o->v50;
    v20 = v40;
    v30 = v50;
    radius = o->radius;
    m8 = o->m8;
}
// @ 0x408720
void capsule::scale(const float* s)
{
    v40.x = s[0] * v40.x;
    v40.y = s[1] * v40.y;
    v40.z = s[2] * v40.z;
    v50.x = s[0] * v50.x;
    v50.y = s[1] * v50.y;
    v50.z = s[2] * v50.z;
    radius = (float)fabs(s[0]) * radius;
}
// @ 0x408770
void capsule::transform(const pos* m)
{
    v20 = m->transform_point_inl(v40);
    v30 = m->transform_point_inl(v50);
}
// @ 0x408870
void capsule::save(stream* s)
{
    float* a = &v40.x;
    stream* t = s->write_f(&radius);
    write3_inl(t, a);
    write3_inl(t, &v50.x);
    v20 = v40;
    v30 = v50;
    s->write_i(&m8);
}
// @ 0x408910
void capsule::load(stream* s)
{
    float* a = &v40.x;
    stream* t = s->read_f(&radius);
    read3_inl(t, a);
    read3_inl(t, &v50.x);
    s->read_i(&m8);
}
// @ 0x408980
face::face() : count(0) {}
// @ 0x4089c0
string face::type_name() const { return "face"; }
// @ 0x4089e0
void face::copy_from(const face* o)
{
    m4 = o->m4;
    count = o->count;
    for (int i = 0; i < count; i++) v[i] = o->v[i];
    m8 = o->m8;
}
// @ 0x408460
vector4 pos::transform_point(const vector4& p) const
{
    return vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                   s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                   s.z * p.x + f.z * p.y + u.z * p.z + o.z);
}
