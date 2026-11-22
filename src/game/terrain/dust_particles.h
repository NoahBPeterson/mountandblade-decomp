#ifndef DUST_H
#define DUST_H
// Slice s004f3e10: the "dust grid" particle pass of the terrain object (g_terrain, 0xa76630: update_particles 0x4f3e10 is a
// thiscall on it, see match/slices/s004fa790; the slices called it Mission, which is the 0x5e292c object of mission.h and
// has nothing at these offsets): dust list at +0x5c3b8f4, cell scene objects at +0x843bd78, extents +0x845d97c / +0x845d980.
//
//   0x4f3e10  update_particles(view): for every grid cell (outer a < +0x845d980, inner b < +0x845d97c):
//             - the cell's 0x34-byte record holds (x, y) (+0/+4) and a reach (+0x10); when the cell has not been
//               emitted yet (word at +0x843e178 + (a * 0x30 + b) * 4) and the view origin is within reach + 36
//               in xy, emit_cell(b, a) re-emits its dust;
//             - the cell's scene object (when its +0x11c list is non-empty): hide it (byte +0x140 = 0) when its
//               animation length + 36 is below the distance to the view origin; otherwise show it and rewrite the
//               matrix at list[0] + 0x1d4: a copy of the view frame with the 4th column of side/forward/up cleared
//               and the 1/20-scaled axes, a (0.5, 0.5, 0.5, 1) colour at +0x204.
//   0x4f4060  emit_cell(inner, outer): mark the cell emitted, walk the dust list, clamp every entry's grid
//             position ((int)(world * 1/20), 0x54b4d0) into the grid, and for the entries of this cell grow the
//             cell object's three vectors (+0x50 0x60-byte frames, +0x70 / +0x90 0x20-byte records) to fit their
//             chain (object + its masters) sizes, then copy the objects into them: the frames get (x, y) from the
//             vertex table times 0.2 (0x54b4c8), the records the spawned particle frames and the object mesh
//             list; the cell object's matrix is rebuilt (set_matrix + terrain colour) and finally the dust
//             entries of the cell are relit (0x471b30) unless shaders are on in quality 2.
// Layouts: mission / dust entry from s0047a0c0, chain node + vector accessors from s0040ea00. Names provisional.
#include <math.h>
#include "engine/math/rgl_math.h"

using rgl::matrix;
using rgl::vector4;

extern int g_use_shaders;                       // 0x5e28fc: programmable pipeline in use

extern float g_k_0_2;                           // 0x54b4c8
extern float g_k_1_20;                          // 0x54b4d0
extern float g_k_reach;                         // 0x598e8c (36.0)

// ---- engine std::vector views (alloc, begin, end, cap): size() is inline, capacity()/resize are out of line ---
#include "engine/math/mat4_include.h"
#include "game/ui/screens/character_sheet/cs_common.h"
#include "game/terrain/terrain.h"                   // Terrain (g_terrain 0xa76630): the dust pass is Terrain::update_particles
#include "engine/render/mesh.h"                    // rgl::mesh (compute_bounds 0x419300, transform 0x47aa40, clone 0x41c6d0), rgl::mesh
#include <vector>
#include "engine/render/mesh_parts.h"             // rgl::vertex60
#include "engine/render/mesh_prims.h"             // rgl::point20, rgl::face20
using rgl::mat3;
struct mxo { mat3 r; vector4 o; };            // a matrix as three rows + origin
// the out-of-line std::vector<T>::capacity() instances (mesh_obj: 0x40f030 vertex60, 0x40f090 point20, face20) stay calls
namespace std {
template<> size_t vector<rgl::vertex60, allocator<rgl::vertex60> >::capacity() const;
template<> size_t vector<rgl::point20, allocator<rgl::point20> >::capacity() const;
template<> size_t vector<rgl::face20, allocator<rgl::face20> >::capacity() const;
// std::vector<T>::reserve of the three mesh lists: the out-of-line instances this TU emits after emit_cell (dust_particles.cpp:
// 0x4f46d0 vertex60, 0x4f4860 point20; the face20 instance is the same code, folded into 0x4f4860 by the original's ICF)
#ifndef DUST_RESERVE_INSTANTIATED_HERE   // dust_particles.cpp instantiates them
template<> void vector<rgl::vertex60, allocator<rgl::vertex60> >::reserve(size_type n);
template<> void vector<rgl::point20, allocator<rgl::point20> >::reserve(size_type n);
template<> void vector<rgl::face20, allocator<rgl::face20> >::reserve(size_type n);
#endif
}

#include "game/data/ptype.h"                      // AnimBox (meshes at +0x14) / ptype / g_ptypes (shared with terrain_a, terrain_b)
#include "engine/render/render_flags.h"

struct cell_rec {                               // 0x34 per cell; x stride 0x9c0
    float x, y;                                 // +0x00 / +0x04
    char pad08[8];
    float reach;                                // +0x10
    char pad14[0x20];
};

namespace rgl { unsigned pack_color(const vector4* c); }       // 0x40e440

#endif
