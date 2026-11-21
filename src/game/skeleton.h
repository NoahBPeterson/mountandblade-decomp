// skeleton.h: the agent's skeleton-side declarations (its display meshes are rgl::lod_mesh, rgl/mesh.h).
// The animation controller at the body's +0x304 is FxDef (include/fx.h: its bone records FxElem, its envelope records
// EnvRec); the body is rgl::strategic_entity (rgl/scene/scene_entity.h).
#ifndef GAME_SKELETON_H
#define GAME_SKELETON_H
#include <vector>
#include "../rgl/rgl_math.h"   // first: see agent4/agent_logic.h
#include "../include/rgl_base.h"
#include "./agent/item_kind.h"
#include "./mat4.h"                   // rgl::mat4

struct shape_base;

struct NewtonRagDollBone;
namespace rgl { struct mesh; }

#endif // GAME_SKELETON_H
