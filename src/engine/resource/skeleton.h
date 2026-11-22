// skeleton.h: the agent's skeleton-side declarations (its display meshes are rgl::lod_mesh, engine/render/mesh.h).
// The animation controller at the body's +0x304 is FxDef (engine/fx/fx.h: its bone records FxElem, its envelope records
// EnvRec); the body is rgl::strategic_entity (engine/scene/scene_entity.h).
#ifndef GAME_SKELETON_H
#define GAME_SKELETON_H
#include <vector>
#include "engine/math/rgl_math.h"   // first: see game/agent/agent_logic.h
#include "engine/core/rgl_base.h"
#include "game/data/item_kind.h"
#include "engine/math/mat4.h"                   // rgl::mat4

struct shape_base;

struct NewtonRagDollBone;
namespace rgl { struct mesh; }

#endif // GAME_SKELETON_H
