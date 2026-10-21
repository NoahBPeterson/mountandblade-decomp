// Slice s004661e0: world-map terrain grid passes (height smoothing, flag clearing, vegetation/snow weights from
// noise, quad diagonal choice and per-quad triangle types). Names provisional.
#include <windows.h>
#include <stdlib.h>
#include "rgl.h"
#include "s004661e0.h"

static inline rgl::vector4 vsub(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline void vcross(rgl::vector4& r, const rgl::vector4& a, const rgl::vector4& b) { r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x; }
static inline float vdot(const rgl::vector4& a, const rgl::vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

struct ivec2 { int dx, dy; };
extern ivec2 g_nbr[];                                   // 0x591340 neighbour offsets
void __cdecl noise_fractal(float a, float b, int c, int d, const float* in, float* out);   // 0x414fc0

// Stand-in body for the cell classifier at 0x4646b0 (not part of this slice; it lives in another one): defining it in this
// translation unit lets cl see that the call does not write memory, which is what the original build knew
// (it keeps this->w cached in a register across the call). The harness maps calls to the real function.
__declspec(noinline) int terrain_cell::kind()
{
    float* w = &v4c;
    int r = 0;
    for (int i = 0; i < 14; i++)
        if (0.4f < w[i]) r = i + 1;
    return r;
}

// @ 0x4661e0
void terrain_obj::smooth_heights()
{
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            float sum = this->at(x, y).pz;
            float cnt = 1.0f;
            #define INR(a, b) ((a) >= 0 && (a) <= this->w && (b) >= 0 && (b) <= this->h)
            if (INR(x, y + 1))     { sum += this->at(x, y + 1).pz;     cnt += 1.0f; }
            if (INR(x + 1, y + 1)) { sum += this->at(x + 1, y + 1).pz; cnt += 1.0f; }
            if (INR(x + 1, y))     { sum += this->at(x + 1, y).pz;     cnt += 1.0f; }
            if (INR(x + 1, y - 1)) { sum += this->at(x + 1, y - 1).pz; cnt += 1.0f; }
            if (INR(x, y - 1))     { sum += this->at(x, y - 1).pz;     cnt += 1.0f; }
            if (INR(x - 1, y - 1)) { sum += this->at(x - 1, y - 1).pz; cnt += 1.0f; }
            if (INR(x - 1, y))     { sum += this->at(x - 1, y).pz;     cnt += 1.0f; }
            if (INR(x - 1, y + 1)) { sum += this->at(x - 1, y + 1).pz; cnt += 1.0f; }
            #undef INR
            float d = sum / cnt - this->at(x, y).pz;
            if (d < -10.0f) d = -10.0f;
            else if (d > 2.0f) d = 2.0f;
            float nh = ((float)(rand() % 0x3dc9) * 2.5289246e-05f + 0.2f) * d + this->at(x, y).pz;
            this->at(x, y).pz = nh;
            this->at(x, y).h2 = nh;
        }
    }
}

// @ 0x466480
void terrain_obj::clear_flags(unsigned mask)
{
    unsigned inv = ~mask;
    for (int y = 0; y <= this->h; y++)
        for (int x = 0; x <= this->w; x++)
            this->at(x, y).flags &= inv;
}

// @ 0x4664e0
void terrain_obj::compute_weights()
{
    struct { float lead; float big; float z[13]; } lim = { 0.0f, 10000.0f };
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            terrain_cell& c = this->at(x, y);
            if ((this->bit58 == -1 || (c.flags & (1 << this->bit58))) && c.pz <= lim.big) {
                float v = ((1.0f - c.f28) - 0.19f) * 12.0f;
                if (v < 0.0f) v = 0.0f;
                else if (v >= 1.0f) v = 1.0f;
                c.v4c = 1.0f - v;
            }
            if (this->f78 >= 0) {
                rgl::vector4 in, out;
                in.x = (float)x;
                in.y = (float)y;
                in.z = 0.0f;
                noise_fractal(15.0f, 0.6f, 3, 0, &in.x, &out.x);
                float v = (out.x + out.y + out.z) * 4.5f + 0.7f;
                if (v < 0.0f) v = 0.0f;
                else if (v >= 1.0f) v = 1.0f;
                c.v50 = v * c.v4c;
                if (v > 0.99f) c.v4c = 0.0f;
            }
            if (this->flag843bd75 && c.pz < 0.0f)
                c.v54 = 1.0f;
        }
    }
}

// @ 0x4666b0
void terrain_obj::pick_diagonals()
{
    for (int y = 0; y < this->h; y++) {
        if (this->w > 0) {
            int order[4] = { 8, 2, 1, 0 };
            for (int x = 0; x < this->w; x++) {
                int v[4];
                for (int k = 0; k < 4; k++) {
                    int nx = g_nbr[order[k]].dx + x;
                    int ny = g_nbr[order[k]].dy + y;
                    if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h)
                        v[k] = this->at(nx, ny).kind();
                    else
                        v[k] = -1;
                }
                int r = -1;
                if (v[1] != v[0] && v[3] != v[0]) r = 0;
                if (v[2] != v[1] && v[0] != v[1]) r = 1;
                if (v[3] != v[2] && v[1] != v[2]) r = 2;
                terrain_quad& q = this->quad(x, y);
                if (!(v[0] == v[3] || v[2] == v[3])) goto set1;
                switch (r) {
                case -1: {
                    rgl::vector4 e1 = vsub(this->at(x + 1, y + 1).pos(), this->at(x, y).pos());
                    rgl::vector4 e2 = vsub(this->at(x + 1, y).pos(), this->at(x, y).pos());
                    rgl::vector4 e3 = vsub(this->at(x, y + 1).pos(), this->at(x, y).pos());
                    rgl::vector4 cr;
                    vcross(cr, e2, e3);
                    if (vdot(e1, cr) > 0.0f) q.diag = 0; else q.diag = 1;
                    break; }
                case 0:
                case 2:
                    q.diag = 0;
                    break;
                default:
                set1:
                    q.diag = 1;
                    break;
                }
            }
        }
    }
}

// @ 0x466930
void terrain_obj::pick_triangle_types()
{
    for (int y = 0; y < this->h; y++) {
        if (this->w > 0) {
            int order[4] = { 8, 2, 1, 0 };
            for (int x = 0; x < this->w; x++) {
                int v[4];
                for (int k = 0; k < 4; k++) {
                    int nx = g_nbr[order[k]].dx + x;
                    int ny = g_nbr[order[k]].dy + y;
                    if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h)
                        v[k] = this->at(nx, ny).kind();
                    else
                        v[k] = -1;
                }
                terrain_quad& q = this->quad(x, y);
                int* pa = &q.tri_a;
                if (q.diag != 0) {
                    q.tri_b = (v[0] > 0 ? v[0] : (v[2] > 0 ? v[2] : v[3]));
                    *pa = (v[0] > 0 ? v[0] : (v[1] > 0 ? v[1] : v[2]));
                } else {
                    q.tri_b = (v[1] > 0 ? v[1] : (v[2] > 0 ? v[2] : v[3]));
                    *pa = (v[1] > 0 ? v[1] : (v[3] > 0 ? v[3] : v[0]));
                }
            }
        }
    }
}
