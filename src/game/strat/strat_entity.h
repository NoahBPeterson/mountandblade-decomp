// strat_entity.h: the lights and shared declarations of the strategic entity's ground-shadow code. The entity is the real
// rgl::strategic_entity (rgl/scene/scene_entity.h: update_shadows / update_ground_cache / build_shadow_frame / create_shadow_mesh /
// shadow_tick are its members); its functions are in src/game/strat/strat_entity.cpp (0x42a090..0x42bac0) and strat_ctx.cpp.
#ifndef STRAT_ENTITY_H
#define STRAT_ENTITY_H
#include "./strat_ext.h"

// ---- scene lights (record 0x70+ bytes). type 2 = directional (direction at +0x44), else a point light at +0x24.
#include "../light.h"                   // Light (shared with agent2 / script)
OFFSET_CHECK3(Light, dir, 0x44);
OFFSET_CHECK3(Light, flags, 0x6c);

struct Psys;

// the point transform of a frame (0x408460), as callers inline it: rgl::tp (rgl/scene/scene_math.h)
using rgl::tp;

#endif // STRAT_ENTITY_H
