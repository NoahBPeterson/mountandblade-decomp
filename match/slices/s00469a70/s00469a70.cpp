// Slice s00468fc0: world-map terrain object, third part: a weighted random walk across the cell grid
// (0x468fc0), averaging of neighbouring flow vectors (0x469a70), and ambient particle spawning from the
// ground-type scatter records (0x46a270). Names provisional.
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include "s00469a70.h"

struct ivec2 { int dx, dy; };
extern ivec2 g_nbr[];                                   // 0x591340 eight neighbour offsets
extern unsigned g_mask_a;                               // 0x5a8ea0
extern unsigned g_mask_b;                               // 0x5a8e78
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

// @ 0x469a70
void terrain_obj::spread_flow(unsigned mask, int mode, unsigned mark)
{
    int order[9];
    unsigned n;
    if (mode == 1) {
        n = 4;
        order[0] = 2; order[1] = 4; order[2] = 5; order[3] = 6;
    } else {
        n = 8;
        for (int i = 0; i < 8; i++) order[i] = i;
    }
    static int seen_dummy;
    int seen[kStride * kStride];
    for (int y = 0; y <= this->h; y++)
        for (int x = 0; x <= this->w; x++)
            seen[x * kStride + y] = 0;
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            terrain_cell& c = at(x, y);
            int cnt = 0;
            vec2 dsum, rsum;
            dsum.x = 0.0f; dsum.y = 0.0f; rsum.x = 0.0f; rsum.y = 0.0f;
            for (unsigned k = 0; k < n; k++) {
                int nx = g_nbr[order[k]].dx + x;
                int ny = g_nbr[order[k]].dy + y;
                if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h) {
                    terrain_cell& d = at(nx, ny);
                    if ((mask & d.flags) != 0) {
                        cnt++;
                        float e[2], r[2], q[2];
                        e[0] = c.px - d.px; e[1] = c.py - d.py;
                        e[0] *= 0.3f; e[1] *= 0.3f;
                        r[0] = e[0] * d.dir.y - e[1] * d.dir.x;
                        r[1] = e[1] * d.dir.y + e[0] * d.dir.x;
                        q[0] = r[0] + d.disp.x;
                        q[1] = r[1] + d.disp.y;
                        dsum.x = q[0] + dsum.x;
                        dsum.y = q[1] + dsum.y;
                        rsum.x = rsum.x + d.dir.x;
                        rsum.y = rsum.y + d.dir.y;
                    }
                }
            }
            if (cnt > 0 || (mark != 0 && cnt != 0)) {
                seen[x * kStride + y] = 1;
                float inv = 1.0f / (float)cnt;
                vec2 nd1, nd2;
                nd1.x = dsum.x * inv; nd1.y = dsum.y * inv;
                c.disp = nd1;
                nd2.x = rsum.x * inv; nd2.y = rsum.y * inv;
                c.dir = nd2;
                float l = (float)sqrt(c.dir.y * c.dir.y + c.dir.x * c.dir.x);
                if (l > 0.0f) {
                    l = 1.0f / l;
                    c.dir.x = l * c.dir.x;
                    c.dir.y = l * c.dir.y;
                } else {
                    c.dir.x = 0.0f;
                    c.dir.y = 1.0f;
                }
            }
        }
    }
    if (mark != 0) {
        for (int y = 0; y <= this->h; y++)
            for (int x = 0; x <= this->w; x++)
                if (seen[x * kStride + y] != 0)
                    at(x, y).flags |= mark;
    }
}

