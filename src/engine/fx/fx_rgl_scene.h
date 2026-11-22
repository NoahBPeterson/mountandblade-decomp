// fx.h: the effect definition ("skeleton") and its elements. FxElem (0x1dc bytes, vtable 0x54b454: three virtual
// methods, NO virtual destructor) is one bone of the definition with its psys / lod lists; FxDef (0x7c bytes) owns an
// array of them plus its own lists and the envelope records. Global classes, like src/engine/scene/fx_element_pool.cpp's FxElem
// (whose destructor 0x41f1d0 and psys-list helpers 0x41f250 / 0x41f2a0 live there).
// Offsets cite the function that reads or writes them. The slices' names: Slot, NodeOwner, slotarr (FxElem, FxDef).
#ifndef SCENE_FX_H
#define SCENE_FX_H
#include <vector>
#include "engine/math/rgl_math.h"
#include "engine/math/scene_math.h"
#include "engine/scene/scene.h"
#include "engine/scene/emitter.h"

using rgl::vector4;

#include "engine/fx/fx.h"   // quat4, FxElem, EnvRec, FxDef (shared with src/game)

#endif
