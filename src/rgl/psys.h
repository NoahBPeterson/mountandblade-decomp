// psys.h: the particle system (src/rgl/psys.cpp, 0x413b60..0x414adf): per-particle geometry builder 0x413e50,
// per-frame driver 0x414960, the particle mesh pool 0x413d30 and the mesh vertex append 0x413c50.
// Names are provisional; offsets are the ones the matches of s00413c50/s00413e50/s00414960 verified.
#ifndef RGL_PSYS_H
#define RGL_PSYS_H
#include <vector>
#include "./rgl_math.h"
#include "./resource.h"
#include "./mesh.h"

namespace rgl {

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

// The object at scene+0x1c4: a pool of particle meshes (one per material) plus the camera axis and eye.
struct mesh_pool {
    char pad0[0xc8];
    std::vector<mesh*> meshes;   // 0xc8
    char pad_d8[0x30c - 0xd8];
    vector4 axis;                // 0x30c
    vector4 eye;                 // 0x31c
    mesh* get_mesh(skin* r);                        // 0x413d30
};

// piecewise-linear key: value v0 until t0, v1 after t1, linear in between
struct curve4 { float t0, v0, t1, v1; };

struct psys {
    mesh* cache;         // 0x00 mesh looked up from `res` on first use
    skin* res;           // 0x04
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
    vec2 uv[4];          // 0x80 uv of the four quad corners
    char pad_a0[0xb4 - 0xa0];
    vector4 axis;        // 0xb4
    char pad_c4[0xd8 - 0xc4];
    float brightness;    // 0xd8
    char pad_dc[0xe8 - 0xdc];
    unsigned flags;      // 0xe8 (mask 0xf00 selects how the frame is built)

    void emit(particle* p, matrix* m, mesh* out, const float* ref);   // 0x413e50
    void update(scene* sc);                                          // 0x414960
};

} // namespace rgl
#endif
