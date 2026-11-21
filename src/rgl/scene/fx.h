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

#include "../../include/fx.h"   // quat4, FxElem, EnvRec, FxDef (shared with src/game)

#endif
