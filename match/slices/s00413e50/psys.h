// psys.h: the particle system whose per-particle geometry builder is 0x413e50 (slice s00413e50) and whose
// per-frame driver is 0x414960 (slice s00414960). Names are provisional; offsets are verified by the matches.
#ifndef MB_PSYS_H
#define MB_PSYS_H
#include "mesh.h"

using namespace rgl;

// one live particle, 0x64 bytes (psys.begin..end, stride 0x64)
struct particle {
    vector4 s, f, u;     // 0x00 orientation rows (copied wholesale into the output frame in the default mode)
    vector4 pos;         // 0x30
    vector4 dir;         // 0x40
    float t;             // 0x50 normalised age, 0..1
    float f54;
    float rot;           // 0x58 spin angle
    float f5c;
    float scale;         // 0x60
};

// camera-ish object reached through scene+0x1c4: a pool plus two vectors
struct view_t : mesh_pool {
    char padv[0x30c - sizeof(mesh_pool)];
    vector4 axis;        // 0x30c
    vector4 eye;         // 0x31c
};
struct scene2 {          // the object passed to 0x414960 (scene in mesh.h; only +0x1c4 is used here)
    char pad[0x1c4];
    view_t* view;        // 0x1c4
};

// piecewise-linear key: value v0 until t0, v1 after t1, linear in between
struct curve4 { float t0, v0, t1, v1; };

struct psys {
    mesh* cache;         // 0x00 mesh looked up from `res` on first use
    res_t* res;          // 0x04
    mesh* src;           // 0x08 template mesh (null: emit camera-facing quads)
    int f0c;
    particle* begin;     // 0x10
    particle* end;       // 0x14
    char pad18[0x28 - 0x18];
    curve4 a;            // 0x28 alpha curve (rise, hold, fall)
    curve4 r;            // 0x38
    curve4 g;            // 0x48
    curve4 b;            // 0x58
    curve4 s;            // 0x68 size curve
    float half_w;        // 0x78
    float half_h;        // 0x7c
    float uv[4][2];      // 0x80 uv of the four quad corners
    char pad_a0[0xb4 - 0xa0];
    vector4 axis;        // 0xb4
    char pad_c4[0xd8 - 0xc4];
    float brightness;    // 0xd8
    char pad_dc[0xe8 - 0xdc];
    unsigned flags;      // 0xe8 (mask 0xf00 selects how the frame is built)

    void emit(particle* p, mat4* m, mesh* out, const float* ref);   // 0x413e50
    void update(scene2* sc);                                         // 0x414960
};
#endif
