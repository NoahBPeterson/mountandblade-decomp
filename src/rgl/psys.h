// psys.h: the particle system (src/rgl/psys.cpp, 0x413b60..0x414adf): per-particle geometry builder 0x413e50,
// per-frame driver 0x414960, the particle mesh pool 0x413d30 and the mesh vertex append 0x413c50.
// Names are provisional; offsets are the ones the matches of s00413c50/s00413e50/s00414960 verified.
#ifndef RGL_PSYS_H
#define RGL_PSYS_H
#include <vector>
#include "./rgl_math.h"
#include "./resource.h"
#include "./mesh.h"
#include "./scene/emitter.h"   // psys, curve4 (one definition for the scene modules and psys.cpp)

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

} // namespace rgl
#endif
