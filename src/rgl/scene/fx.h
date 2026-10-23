// fx.h: the effect definition ("skeleton") and its elements. FxElem (0x1dc bytes, vtable 0x54b454: three virtual
// methods, NO virtual destructor) is one bone of the definition with its psys / lod lists; FxDef (0x7c bytes) owns an
// array of them plus its own lists and the envelope records. Global classes, like src/game/entity.cpp's FxElem
// (whose destructor 0x41f1d0 and psys-list helpers 0x41f250 / 0x41f2a0 live there).
// Offsets cite the function that reads or writes them. The slices' names: Slot, NodeOwner, slotarr (FxElem, FxDef).
#ifndef SCENE_FX_H
#define SCENE_FX_H
#include <vector>
#include "../rgl_math.h"
#include "./scene_math.h"
#include "./scene.h"
#include "./emitter.h"

using rgl::vector4;

#include "../../include/fx.h"   // quat4, FxRec, FxRes, mesh_ptr_vec, FxElem, EnvRec, FxDef (shared with src/game)

// the 0x30-byte skinning record: four weights, four bone ids, the same ids as floats (mesh::v60 seen as skin data)
struct SkinVert {
    float w[4];                 // 0x00
    int bone[4];                // 0x10
    float bonef[4];             // 0x20
};
// pointer into the skinning records; operator+ stays out of line in FxDef::skin
struct skin_iter {
    SkinVert* cur;
    skin_iter() : cur(0) {}
    skin_iter(SkinVert* p) : cur(p) {}
    skin_iter operator+(int n) const;                           // 0x4296d0
};

#endif
