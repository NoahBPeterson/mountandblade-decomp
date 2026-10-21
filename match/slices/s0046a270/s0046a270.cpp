// Slice s00468fc0: world-map terrain object, third part: a weighted random walk across the cell grid
// (0x468fc0), averaging of neighbouring flow vectors (0x469a70), and ambient particle spawning from the
// ground-type scatter records (0x46a270). Names provisional.
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include "s0046a270.h"

struct ivec2 { int dx, dy; };
extern ivec2 g_nbr[];                                   // 0x591340 eight neighbour offsets
extern int g_mask_a;                               // 0x5a8ea0
extern int g_mask_b;                               // 0x5a8e78
extern int g_terrain_gate;                              // 0x5aa03c
ptype* g_ptypes;                                        // 0x5e28f8 (defined elsewhere; declared here)

void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0
struct mat3 { rgl::vector4 s, f, u; };
__declspec(noinline) mat3* identity3(mat3* out);                             // 0x40cd60
struct pos2 : rgl::matrix {
    void rotate_sf(const float* angle);     // 0x40e770
};

static const int kStride = 251;

// inline vec2 helpers (scalar by const reference)
static __forceinline vec2 v2sub(const vec2& a, const vec2& b) { vec2 r; r.x = a.x - b.x; r.y = a.y - b.y; return r; }
static __forceinline vec2 v2add(const vec2& a, const vec2& b) { vec2 r; r.x = a.x + b.x; r.y = a.y + b.y; return r; }
static __forceinline vec2 v2mul(const vec2& a, const float& k) { vec2 r; r.x = a.x * k; r.y = a.y * k; return r; }
// rotate e by the unit vector d
static __forceinline vec2 v2rot(const vec2& e, const vec2& d) { vec2 r; r.x = e.x * d.y - e.y * d.x; r.y = e.y * d.y + e.x * d.x; return r; }

static __forceinline rgl::vector4 vmul(const rgl::vector4& a, const float& s) { return rgl::vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline rgl::vector4 vadd(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline rgl::vector4 vsub(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline rgl::vector4 vcross(const rgl::vector4& a, const rgl::vector4& b)
{
    return rgl::vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static __forceinline void vnormalize(rgl::vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv; v.y = v.y * inv; v.z = v.z * inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
}
#define RND() ((float)(rand() % 0x3dc9) * 6.322312e-05f)

// @ 0x46a270
void terrain_obj::spawn_particles(particle_list* list, int unused)
{
    list->count = 0;
    unsigned lv = (flags10 >> 0x15) & (g_mask_a - 1);
    float lv_scale = (float)lv * 0.01f;
    unsigned sel = (flags14 >> 2) & (g_mask_b - 1);
    int maxn = (int)(spawn_scale * spawn_scale * 0.8f);
    float k1 = (float)pow(spawn_scale, 1.7f) * 0.3f;
    float k2 = (float)g_terrain_gate * 0.02f;
    for (int y = 0; y < this->h; y++) {
        for (int x = 0; x < this->w; x++) {
            if (!(at(x, y).pz > -0.3f && at(x + 1, y).pz > -0.3f && at(x, y + 1).pz > -0.3f && at(x + 1, y + 1).pz > -0.3f))
                continue;
            terrain_quad& q = quad(x, y);
            for (int t = 0; t < 2; t++) {
                q.slot[t] = -1;
                if (list->nsrc != 0 && (q.tri[t] == 2 || q.tri[t] == 1) && list->count < 0x40000) {
                    float fx = (float)x;
                    float fy = (float)y;
                    for (int j = 0; j < list->nsrc; j++) {
                        scatter_src& s = list->src[j];
                        int kind = s.kind;
                        float p[3];
                        p[0] = fx + s.v.x;
                        p[1] = fy + s.v.y;
                        p[2] = s.v.z;
                        float nz[3];
                        noise_fractal(s.freq, 0.6f, 3, 0, p, nz);
                        float v = (nz[2] + nz[1]) + nz[0];
                        if (!(v < 0.5f)) v = 0.5f;
                        ptype* pt = &g_ptypes[kind];
                        float rate = ((v - s.thresh) * s.rate) * k1;
                        if ((pt->flags & 0x80000) == 0) {
                            rate = rate * lv_scale;
                        } else {
                            rate = rate * k2;
                            if (sel != 0) rate = 0.0f;
                        }
                        int n = 0;
                        if (RND() < rate) {
                            do {
                                if (maxn <= n) break;
                                if (!(RND() < q.f34) && (pt->flags & 0x80000) != 0) break;
                                rate = rate - 1.0f;
                                particle* e = &list->items[list->count];
                                e->type = kind;
                                e->cx = x;
                                e->cy = y;
                                e->f9c = 0;
                                e->f0c = t;
                                n++;
                                int a = 0;
                                if (1 < pt->nframes)
                                    a = rand() % pt->nframes;
                                e->anim = a;
                                mat3 id;
                                rgl::matrix m;
                                {
                                    mat3* ip = identity3(&id);
                                    memcpy(&m, ip, 0x30);
                                }
                                int cr[6];
                                int* ca = &cr[0]; int* cb = &cr[2]; int* cc = &cr[4];
                                triangle_corners(x, y, t, ca, cb, cc);
                                rand(); rand(); rand();
                                const rgl::vector4& PA = (const rgl::vector4&)at(ca[0], ca[1]);
                                const rgl::vector4& PB = (const rgl::vector4&)at(cb[0], cb[1]);
                                const rgl::vector4& PC = (const rgl::vector4&)at(cc[0], cc[1]);
                                m.o = vadd(vadd(vmul(PA, 0.3333f), vmul(PB, 0.3333f)), vmul(PC, 0.3333f));
                                float r1 = RND();
                                rgl::vector4 d1 = vmul(vsub(PA, m.o), r1);
                                float r2 = RND();
                                rgl::vector4 d2 = vmul(vsub(PB, m.o), r2);
                                float r3 = RND();
                                rgl::vector4 d3 = vmul(vsub(PC, m.o), r3);
                                m.o = vadd(vadd(vadd(d2, d1), d3), m.o);
                                if ((pt->flags & 0x40000) != 0) {
                                    m.u = q.nrm[t];
                                    m.f = vcross(m.u, m.s);
                                    vnormalize(m.f);
                                    m.s = vcross(m.f, m.u);
                                }
                                float ang = (float)(rand() % 0x3dc9) * 0.00039724255f;
                                ((pos2*)&m)->rotate_sf(&ang);
                                e->frame = m;
                                float sc = (float)(rand() % 0x3dc9) * 4.425618e-05f + 0.65f;
                                e->dir = rgl::vector4(((float)(rand() % 0x3dc9) * 1.2644623e-05f + 0.9f) * sc,
                                                      ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 0.9f) * sc,
                                                      ((float)(rand() % 0x3dc9) * 1.8966935e-05f + 0.85f) * sc);
                                list->count++;
                            } while (RND() < rate);
                        }
                    }
                }
            }
        }
    }
}
