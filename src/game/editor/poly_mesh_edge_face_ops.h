// poly_mesh_edge_face_ops.h: classes of poly_mesh_edge_face_ops.cpp (0x4ca600..0x4ce720, TU map 104 "mesh_2"): the screen-mode queue, the graph
// editor's edge/face operations (PolyMesh, game/terrain/poly_mesh.h), the big editor records, the std::vector instantiations the linker placed
// in this object, the skeleton fix-up, the language tables' constructor and loader, the csv row and the scene
// object readers. Merged from the slices s004ca550 (0x4ca600..0x4ca9b0), s004caa40, s004cb3a0, s004cbcd0, s004cc680,
// s004cd010, s004cd850 and s004cdfd0 (up to 0x4ce680). Names are provisional unless a vtable maps them.
#ifndef MESH2_MESH_2_H
#define MESH2_MESH_2_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <io.h>
#include <new>
#include <vector>
#include <memory>
#include <stdexcept>
#include "engine/math/rgl_math.h"
#include "engine/io/stream.h"
#include "engine/core/tstring.h"
#include "engine/physics/rag_model.h"
#include "game/ui/gui_manager.h"
using rgl::mesh;                 // the loaded-mesh class: engine/render/mesh.h (gui_manager.h includes it)
#include "game/editor/mesh2_common.h"
#include "game/terrain/poly_mesh.h"   // PolyMesh: the map editor's polygon mesh, whose members 0x4c53f0..0x4cb940 this module defines
#include "game/editor/screen_cfg.h"

using rgl::vector4;
using rgl::matrix;

#include "engine/core/crt.h"   // canonical: operator delete[] (0x53570d)

// ============================================================================================================
// 0x4cb610..0x4cbc80: the big editor record's releasers and vector helpers
// ============================================================================================================
// 0x4cb6b0 is std::vector<ArrSet>::_Tidy (from the explicit instantiation in poly_mesh_edge_face_ops.cpp; its _Destroy_range call is
// the out-of-line 0x4727a0, terrain_b)

#include "engine/core/glyph_hash.h"    // GlyphList (_Incsize 0x4cb6f0), GlyphIt


#include "engine/core/vec_xlen.h"   // std::vector<int>::_Xlen 0x43e2a0 (out of line: vector<int>'s members here call it)
// 0x4cbb90 / 0x4cbbe0 / 0x4cbc30 / 0x4cbc80 are the _Buy members of std::vector<RagdollBone> / <ArrSet> / <Constraint> /
// <FaceTexture> (the explicit instantiations in poly_mesh_edge_face_ops.cpp)

// ============================================================================================================
// 0x4cbcd0..0x4cd850: std::vector instantiations and the element types of this object
// ============================================================================================================
struct E44  { int v[11]; ~E44() {} }; // 0x2c-byte record (empty, user-declared dtor: the original's
                                      // _Destroy loop survives, the call inlines away)
#include "engine/core/vec_elems.h"   // FaceTexture (shared with game/ui/screens)


#include "game/data/e68.h"           // E68 (shared with game/ui/widgets/mission.h)

// _Nonscalar_ptr_iterator_tag: an empty tag that cl 13.10 still passes as one 4-byte stack slot
// (which is where the last `mov eax, [esp+8]` in 0x4cc260 comes from).
struct nonscalar_tag {};
// 0x4ca7e0: std::_Destroy_range for the 0xb0-byte record FaceTexture (frees +0x20, then destroys the string at +0), defined in this object


#include "engine/physics/bone.h"   // RagdollBone (the ragdoll bone record, ctor 0x4cd7a0), Bone / CapDef (fix_skeletons 0x4cd850)

// ============================================================================================================
// 0x4cd850..0x4cdf20: skeleton fix-up and the csv row (the translation tables are LangTables, mesh2_common.h)
// ============================================================================================================
// the skeleton fix_skeletons looks up is resource.h's frame_set (get_object70 0x418cf0: nbones +0x18, bones +0x1c, engine/physics/bone.h's Bone)
struct named_object;
namespace rgl { struct mesh; }
#include "engine/resource/resource.h"   // resmgr (global at 0x5d6fa0): the one class; find_meshes 0x41b960 (hidden return pointer first, ret 8)
namespace std { template<> vector<rgl::mesh*, allocator<rgl::mesh*> >& vector<rgl::mesh*, allocator<rgl::mesh*> >::operator=(const vector<rgl::mesh*, allocator<rgl::mesh*> >&); }   // 0x43f0a0 (the out-of-line copy-assignment; the linker folded every pointer vector's onto vector<int>'s)

#include "game/data/item_kind_include.h"   // ItemKind (item_kinds.txt, 0x238 bytes): its constructor 0x4cdf20 is defined in poly_mesh_edge_face_ops.cpp

extern SkeletonBody g_skeletons;                      // 0xa180e8 (the registered skeletons, filled from skeleton_bodies.xml)
#include "engine/core/alloc_construct.h"   // alloc_tstring 0x436c90
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v, alloc_tstring& al, ptr_tag tag);   // 0x438f00 (agent_stats.cpp)
void __fastcall fix_skeletons(std::vector<RagModel>* list);               // 0x4cd850


// ============================================================================================================
// 0x4cdfd0..0x4ce680: the scene object tables
// ============================================================================================================
// (`mesh` is the opaque loaded-mesh class; gui_manager.h declares a sized stand-in)
#include "engine/resource/resource.h"                                    // body_part (the body resource: shape at +0x18, copy_shape 0x439100), shape_base
inline void mesh_node_bounds(mesh* node, vector4* mn, vector4* mx) { node->calc_bounds(mn, mx); }   // 0x438d80
void vmin(vector4* r, const vector4* a, const vector4* b);  // 0x40af80
void vmax(vector4* r, const vector4* a, const vector4* b);  // 0x40b080

#include "game/data/ptype.h"                              // ptype / AnimBox: the flora kind record 0x4cdfd0 reads

#include "game/mission/mission_obj.h"      // MissionObj: one scene prop kind (ctor 0x4ce2a0, read 0x4ce360 below)

// FaceTexture (0xb0, src/engine/core/vec_elems.h): its default constructor 0x4ce680 is defined in poly_mesh_edge_face_ops.cpp.




#endif
