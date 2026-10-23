// emitter.h: the particle emitter ("psys", 0x1bc bytes). rgl::psys in ../psys.h is the same class seen through the
// fields psys::emit / psys::update (0x413e50 / 0x414960) read; this header carries every field the scene modules
// touch. Layout evidence: psys::init 0x420b40 (defaults for most fields), world::alloc_emitter 0x427ea0 (ctor,
// noise_index), world::step 0x427f40, world::partition_items 0x41efd0, find_emitter 0x421a50.
//
// The particle vector's element is the 0x64-byte record E100 (the name the neighbouring objects registered for the
// std::vector<E100> instances 0x41fb00 / 0x410dc0); ../psys.h calls it rgl::particle.
#ifndef SCENE_EMITTER_H
#define SCENE_EMITTER_H
#include <vector>
#include "../rgl_math.h"
#include "./scene_math.h"

struct E100 { int v[25]; };                                     // 0x64-byte particle record
struct skin;                                                    // ../resource.h (global)

namespace rgl {

struct mesh;
struct scene;
struct particle;                                                // ../psys.h: the 0x64-byte record (E100 here)

// piecewise-linear key: value v0 until t0, v1 after t1, linear in between (psys::emit 0x413e50 reads the curves as these)
struct curve4 { float t0, v0, t1, v1; };

struct float_pair {                      // two floats built through a stack temporary (psys::init)
    float u, v;
    float_pair() {}
    float_pair(float a, float b) : u(a), v(b) {}
};

struct psys {
    mesh* cache;                    // 0x00 (zeroed by init 0x420b40)
    skin* res;                      // 0x04
    union {
        int f8;                     // 0x08
        mesh* src;                  // 0x08 template mesh (null: emit camera-facing quads; psys::emit 0x413e50)
    };
    std::vector<E100> particles;    // 0x0c (first at 0x10, last at 0x14)   0x41efd0, 0x420b40
    float f1c, f20, f24;            // 0x1c noise scale, 0x20 noise amount, 0x24 drag      0x420b40
    float_pair curve[10];                // 0x28 five (t0,v0,t1,v1) curves written as ten pairs  0x420b40; psys::emit 0x413e50 reads them as curve4: alpha, r, g, b, size
    float_pair half;                     // 0x78 (u = half width, v = half height; 0x413e50)
    float_pair quad[4];                  // 0x80 corner uvs (0x413e50 passes them as vec2)
    int fa0;                        // 0xa0
    vector4 va4, vb4, vc4;          // 0xa4 x axis, 0xb4 z axis (the particle axis of 0x414960), 0xc4 gravity   0x420b40
    float fd4, fd8, fdc, fe0;       // 0xd4..0xe0 (fd8 = brightness, 0x413e50)
    int fe4;                        // 0xe4
    union {
        int fe8;                    // 0xe8 (1)
        unsigned flags;             // 0xe8 (mask 0xf00 selects how the frame is built; 0x413e50)
    };
    int fec, ff0;                   // 0xec, 0xf0
    mat4 mf4, m134;                 // 0xf4, 0x134 (identity)
    vector4 v174;                   // 0x174
    vector4 v184;                   // 0x184 (0,0,1)
    float f194;                     // 0x194
    int f198;                       // 0x198
    int spawn_left;                 // 0x19c  0x41efd0 (<= 0 and no particles: the emitter is dead)
    int f1a0, f1a4;                 // 0x1a0, 0x1a4
    int tag;                        // 0x1a8 (id searched by find_emitter 0x421a50; init sets -1)
    float radius;                   // 0x1ac (render_scene 0x429890 and StrategicEntity::update 0x421b70 read it)
    int f1b0, f1b4;                 // 0x1b0, 0x1b4
    int noise_index;                // 0x1b8 (world::alloc_emitter 0x427ea0 stores the world's counter)
    psys(const int& idx) : particles() { noise_index = idx; init(); }   // inline in world::alloc_emitter
    void init();                                    // 0x420b40
    void update(const float* dt, const matrix& frame);      // 0x412150 (per-frame step)
    void update(scene* sc);                         // 0x414960 (render-time update, ../psys.cpp)
    void emit(particle* p, matrix* m, mesh* out, const float* ref);   // 0x413e50 (../psys.cpp)
};

} // namespace rgl
#endif
