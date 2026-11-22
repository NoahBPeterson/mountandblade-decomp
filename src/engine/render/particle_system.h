// particle_system.h: the particle system (src/engine/render/particle_system.cpp, 0x413b60..0x414adf): per-particle geometry builder 0x413e50,
// per-frame driver 0x414960, the particle mesh pool 0x413d30 and the mesh vertex append 0x413c50.
// Names are provisional; offsets are the ones the matches of s00413c50/s00413e50/s00414960 verified.
#ifndef RGL_PSYS_H
#define RGL_PSYS_H
#include <vector>
#include "engine/math/rgl_math.h"
#include "engine/resource/resource.h"
#include "engine/render/mesh.h"
#include "engine/scene/emitter.h"   // psys, curve4 (one definition for the scene modules and particle_system.cpp)

namespace rgl {

// one live particle: rgl::particle (0x64 bytes, ./engine/render/mesh_rgl_particles.h)

// The object at scene+0x1c4: a pool of particle meshes (one per material) plus the camera axis and eye.

} // namespace rgl
#endif
