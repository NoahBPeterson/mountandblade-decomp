// troop_mgr.h: the skin record's point-map builders. Skin::apply_group (0x470db0, the slices' SkinOwner::build_map) writes one slot of an
// ArrSet (count, array); Skin::copy_troop (0x473570) copies a lod mesh resource's name and applies every non-empty group (the five mesh
// lists of a lod_group, src/rgl/core/resmgr.h, which terrain_b.cpp includes itself). The skin record itself is src/include/skin.h.
#ifndef TERRAIN_B_TROOP_MGR_H
#define TERRAIN_B_TROOP_MGR_H
#include <vector>
#include "../../rgl/mesh.h"
#include "./records.h"
#include "../../include/skin.h"

// copy_troop calls apply_group through the same address from outside its own definition: cl must not assume its
// register use (the slice compiled them apart).
struct SkinApplyExt {
    __forceinline void apply_group(ArrSet* dst, int idx, rgl::mesh* obj) { ((Skin*)this)->apply_group(dst, idx, obj); }   // 0x470db0
};

#endif // TERRAIN_B_TROOP_MGR_H
