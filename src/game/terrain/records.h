// records.h: the small record types of the std::vector / std::stable_sort instantiations of terrain_scene_renderer.cpp, the
// name + five-array record (ArrSet), and the 0x1c8-byte polymorphic record (PlacedObj) that the same vectors hold.
// Only sizes, offsets and the constructors the instantiations expose are verified (cited per type); the object
// that owns each vector is elsewhere. Names provisional (the slices' names are kept where symbols/slices maps them).
#ifndef TERRAIN_B_RECORDS_H
#define TERRAIN_B_RECORDS_H
#include <vector>
#include <list>
#include <memory>
#include <algorithm>
#include "engine/core/tstring.h"
#include "game/terrain/poly_mesh.h"

#include "engine/core/ptr_tag.h"

// ---- plain records --------------------------------------------------------------------------------------------------
#include "engine/core/pod_elems.h"   // rec36 (0x24: vector inserted by 0x477410, _Insert_n 0x476370, push_back 0x477b80),
                                        // rec136 (0x88: reserve 0x476130, insert 0x4762b0, push_back 0x477380)
struct E36x { int v[9]; };      // 0x24: a second 36-byte record type: reserve 0x476020, insert 0x476240 (its _Insert_n is the earlier
                                // object's 0x424830), push_back 0x4772f0 (the slices called it rec36 / E36x)
// (the 0x38-byte physics record vector<> 0x479930 erases / 0x47a980 resizes, and copy_e56 0x477940 copies, is PhysRec: engine/physics/phys_set.h)
#include "engine/physics/phys_set.h"
// 8-byte sort record with a function-pointer predicate (the std::stable_sort instantiation 0x472870 .. 0x475f70)
struct P8 { int key, val; };
typedef bool (__cdecl *Pred8)(const P8&, const P8&);
typedef std::vector<P8>::iterator It8;

// (the _Ufill members of vector<rec36> / <MVert> / <MPoly> / <MEdge>, 0x4724c0 .. 0x472550, are poly_mesh.cpp's explicit
// instantiations; their _Uninit_fill_n are game/terrain/terrain_render.cpp's, 0x46b500 .. 0x46b650)
#include "engine/core/alloc_construct.h"   // alloc_rec136

// ---- ArrSet: name + five (count, int array) pairs (0x38 bytes) -----------------------------------------------------
#include "engine/core/arr_set.h"

// std::vector<ArrSet>'s range algorithms are real STL instances: _Destroy_range 0x4727a0, _Uninit_fill_n 0x4727e0 and the
// _Ucopy / _Destroy / _Ufill members 0x474350 / 0x475b60 / 0x475b80 (poly_mesh.cpp), _Copy_opt 0x468050 (game/terrain/terrain_particles.cpp),
// allocator::construct 0x46b6c0 and _Uninit_copy 0x46c1d0 (game/terrain/terrain_render.cpp)

// ---- the 0x1c8-byte polymorphic record (vtable 0x590b84; copy ctor 0x437bb0, operator= 0x437d60, reset 0x4352f0) -----
// The slices called it PlacedObj, MAgent, PropRec and PlacedObj (symbols/slices maps ??_7Agent@@, ??_7PlacedObj@@, ??_7PlacedObj@@).
#include "game/mission/placed_obj.h"      // PlacedObj, IfaceB, Quad (shared)

#endif // TERRAIN_B_RECORDS_H
