// mesh_obj.h: the classes of mesh_obj.cpp (0x40e940..0x413b5f) are defined in ../mesh.h (the mesh object, its element
// types, the particle emitter, the scene it is rendered into, the scene registry and the lod mesh). This header is
// kept so that the module's include list does not change.
#ifndef RGL_MESH_OBJ_H
#define RGL_MESH_OBJ_H
#include "../mesh.h"
#include "../rgl_math.h"
namespace rgl {

// inline vector helpers of the mesh and particle code (rgl_math.h declares the out-of-line operators
// 0x407cd0 / 0x407d00; these are what the callers inline). v * s and s * v take s by reference.
inline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
inline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
inline vector4 vmul(const float& s, const vector4& a) { return vector4(s * a.x, s * a.y, s * a.z); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
__forceinline vector4 operator+(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

// 0x40e4e0 as setup_material calls it: the hidden return pointer form of rgl_math.h's unpack_color(out, argb)
// (same cdecl call, but the result is copied out of a temporary)
vector4 unpack_color(const unsigned* argb);                         // 0x40e4e0

matrix* identity(matrix* out);                                          // 0x40cd00 (cdecl, returns out)

// the matrix operations that rgl_math.h does not declare (callers: emitter::spawn 0x40ec90)
struct frame_ops : matrix {
    vector4 transform_point(const vector4& p) const;                     // 0x408460
    vector4 rot(const vector4& p) const;                                 // 0x40a9c0 (rotates by the s, f, u rows)
};

// ---- the particle emitter (hangs off mesh+0xb0) ----------------------------------------------------------------
// The same object as src/rgl/psys.h's `psys` (flags +0xe8; psys::update 0x414960 and psys::emit 0x413e50 are
// its render-time methods): this class has the simulation fields (update 0x412150, spawn 0x40ec90) and the
// particle vector psys.h holds as raw pointers.
// Module System particle-system flags (header_particle_systems.py)
enum {
    psf_always_emit         = 0x0000000002,
    psf_global_emit_dir     = 0x0000000010,
    psf_emit_at_water_level = 0x0000000020,
    psf_billboard_2d        = 0x0000000100,
    psf_billboard_3d        = 0x0000000200,
    psf_turn_to_velocity    = 0x0000000400,
    psf_randomize_rotation  = 0x0000001000,
    psf_randomize_size      = 0x0000002000
};
struct emitter_particle {        // 0x64 bytes (same record as psys.h's `particle`)
    vector4 s, f, u;             // 0x00 orientation rows (spawn 0x40ec90 copies the emitter frame here)
    vector4 pos;                 // 0x30
    vector4 vel;                 // 0x40 (psys.h: dir)
    float age;                   // 0x50 0..1, dies at >= 1 (update 0x412150)
    float f54;
    float angle;                 // 0x58
    float angvel;                // 0x5c
    float scale;                 // 0x60
};
struct emitter {
    char pad0[0xc];
    std::vector<emitter_particle> particles;   // +0x0c (first +0x10)
    float noise_scale;           // +0x1c
    float noise_amount;          // +0x20
    float drag;                  // +0x24
    char pad28[0xa0 - 0x28];
    float lifetime;              // +0xa0
    char pada4[0xc4 - 0xa4];
    vector4 accel;               // +0xc4
    float accel_scale;           // +0xd4
    char padd8[0xe8 - 0xd8];
    unsigned flags;              // +0xe8 psf_*
    float water_z;               // +0xec
    int disabled;                // +0xf0
    matrix world;                // +0xf4
    matrix local;                // +0x134
    float box[3];                // +0x174 emission box half-extents
    char pad180[4];
    vector4 vel_base;            // +0x184
    float vel_scale;             // +0x194
    float f198;                  // +0x198 spawn rate
    int spawn_left;              // +0x19c
    int spawn_total;             // +0x1a0
    float spawn_accum;           // +0x1a4
    char pad1a8[4];
    float radius;                // +0x1ac
    float spin_scale;            // +0x1b0
    float spin_drag;             // +0x1b4
    int noise_index;             // +0x1b8 (indexes g_ticks)
    void update(const float* dt, const matrix& frame);      // 0x412150
    void spawn(emitter_particle* out);                      // 0x40ec90
    void update(scene* sc);                          // 0x414960 (src/rgl/psys.cpp: psys::update)
};
extern __int64 g_ticks[];                             // 0x5e2948

} // namespace rgl

#endif
