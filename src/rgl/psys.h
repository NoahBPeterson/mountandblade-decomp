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

// one live particle: rgl::particle (0x64 bytes, ./particles/mesh_obj.h)

// The object at scene+0x1c4: a pool of particle meshes (one per material) plus the camera axis and eye.

} // namespace rgl
#endif
