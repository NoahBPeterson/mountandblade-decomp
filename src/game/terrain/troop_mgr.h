// troop_mgr.h: the skin record's point-map builders. Skin::apply_group (0x470db0, the slices' SkinOwner::build_map) writes one slot of an
// ArrSet (count, array); Skin::copy_troop (0x473570) copies a lod mesh resource's name and applies every non-empty group (the five mesh
// lists of a rgl::lod_mesh, src/engine/resource/resource_manager.h, which terrain_scene_renderer.cpp includes itself). The skin record itself is src/engine/render/skin.h.
#ifndef TERRAIN_B_TROOP_MGR_H
#define TERRAIN_B_TROOP_MGR_H
#include <vector>
#include "engine/render/mesh.h"
#include "game/terrain/records.h"
#include "engine/render/skin.h"

#endif // TERRAIN_B_TROOP_MGR_H
