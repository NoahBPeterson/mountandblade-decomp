// Slice s00408a50: polygon shape ("face", vtable 0x54a588) scale / transform / save / load / draw.
// Layout and helper types as in s00408110 (declared locally).
#include "rgl.h"
using namespace rgl;

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

// vector4 with the (1e-10) normalize that callers inline (rgl::vector4::normalize at 0x407d60).
struct vec4i : vector4 {
    float normalize_inl()
    {
        float len = length();
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            x *= inv; y *= inv; z *= inv;
        } else {
            x = 0.0f; y = 1.0f; z = 0.0f;
        }
        return len;
    }
};

struct pos : matrix {
};

static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline void vcross(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}

// Global render state object at 0xa17910 (see s00408110); only draw_polygon is used here.
struct renderer {
    void draw_polygon(const void* v, int n); // 0x408e60
};
extern renderer g_render;   // 0xa17910

struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base();
};

// Polygon shape, vtable 0x54a588 (name "face" from 0x4089c0). Up to 4 vertices.
struct face : shape_base {
    vector4 w[4];                // 0x20 world-space vertices (transform output)
    vector4 v[4];                // 0x60 local vertices
    vector4 normal;              // 0xa0
    int count;                   // 0xb0
    virtual ~face();
    virtual void s1();
    virtual string type_name() const;                // slot 2   0x4089c0
    virtual void save(stream* s);                    // slot 3   0x408d60
    virtual void load(stream* s);                    // slot 4   0x408dd0
    virtual void s5();
    virtual void draw();                             // slot 6   0x408e40
    virtual void scale(const float* s);              // slot 7   0x408a50
    virtual void transform(const pos* m);            // slot 8   0x408bd0
};

// @ 0x408a50
void face::scale(const float* s)
{
    for (int i = 0; i < count; i++) {
        v[i].x *= s[0];
        v[i].y = v[i].y * s[1];
        v[i].z *= s[2];
    }
    int neg = 0;
    if (s[0] < 0.0f) neg = 1;
    if (s[1] < 0.0f) neg++;
    if (s[2] < 0.0f) neg++;
    if (neg == 1 || neg == 3) {
        normal = vector4(-normal.x, -normal.y, -normal.z);
        if (count == 3) {
            vector4 t = v[0];
            v[0] = v[2];
            v[2] = t;
        } else {
            vector4 t = v[1];
            v[1] = v[3];
            v[3] = t;
        }
    }
}
// @ 0x408bd0
void face::transform(const pos* m)
{
    for (int i = 0; i < count; i++) {
        const matrix& mm = *m;
        const vector4& s = mm.s, &f = mm.f, &u = mm.u, &o = mm.o;
        const vector4& p = v[i];
        w[i] = vector4((s.x * p.x + f.x * p.y) + u.x * p.z + o.x,
                       (s.y * p.x + f.y * p.y) + u.y * p.z + o.y,
                       (f.z * p.y + s.z * p.x) + u.z * p.z + o.z);
    }
    vector4 e1 = vsub(w[2], w[1]);
    vector4 e0 = vsub(w[1], w[0]);
    vcross(normal, e0, e1);
    ((vec4i&)normal).normalize_inl();
}
// @ 0x408d60
void face::save(stream* s)
{
    s->write_i(&count);
    int i = 0;
    if (count > 0) {
        float* p = &v[0].z;
        do {
            s->write_f(p - 2)->write_f(p - 1)->write_f(p);
            i++;
            p += 4;
        } while (i < count);
    }
    s->write_i(&m8);
}
// @ 0x408dd0
void face::load(stream* s)
{
    s->read_i(&count);
    int i = 0;
    if (count > 0) {
        float* p = &v[0].z;
        do {
            s->read_f(p - 2)->read_f(p - 1)->read_f(p);
            i++;
            p += 4;
        } while (i < count);
    }
    s->read_i(&m8);
}
// @ 0x408e40
void face::draw()
{
    g_render.draw_polygon(w, count);
}
