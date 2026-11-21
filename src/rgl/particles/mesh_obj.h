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
// (vector4 + vector4 is rgl_math.h's member operator+: x, y, z added, w left unset)

vector4 unpack_color(const unsigned* argb);                         // 0x40e4e0

} // namespace rgl
namespace rgl { struct matrix; } typedef rgl::matrix pos;                                                             // include/mat4.h: the Module System position (an rgl::matrix)
namespace rgl {

// the matrix operations that rgl_math.h does not declare (callers: psys::spawn 0x40ec90)
struct frame_ops : matrix {
    vector4 transform_point(const vector4& p) const;                     // 0x408460
    vector4 rot(const vector4& p) const;                                 // 0x40a9c0 (rotates by the s, f, u rows)
};

// ---- the particle emitter (hangs off mesh+0xb0): rgl::psys, ../scene/emitter.h -----------------------------------
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
struct particle {                // 0x64 bytes: one live particle of a psys (std::vector<particle> psys::particles)
    matrix frame;                // 0x00 orientation rows s, f, u (psys::spawn 0x40ec90 copies the emitter frame here) and the position o (0x30)
    vector4 vel;                 // 0x40 (psys::emit 0x413e50: the facing direction of the velocity-aligned modes)
    float age;                   // 0x50 0..1, dies at >= 1 (psys::update 0x412150)
    float f54;
    float angle;                 // 0x58 spin angle
    float angvel;                // 0x5c
    float scale;                 // 0x60
};

} // namespace rgl

#endif
