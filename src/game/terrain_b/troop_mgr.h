// troop_mgr.h: the object that builds the per-slot point maps of a troop's mesh groups (TroopMgr), and the troop
// record's group vectors it reads. apply_group (0x470db0, the slices' SkinOwner::build_map) writes one slot of an
// ArrSet (count, array); copy_troop (0x473570) copies a troop record's name and applies every non-empty group.
#ifndef TERRAIN_B_TROOP_MGR_H
#define TERRAIN_B_TROOP_MGR_H
#include <vector>
#include "../../rgl/mesh.h"
#include "./records.h"

struct MeshRef { char pad[0x38]; rgl::mesh** base; };       // base: pointer to the base mesh pointer at +0x38 (0x470db0)
struct MeshGroupRec {                                       // reduced: the record copy_troop reads (NOT the 0x508-byte troop record, Troop in src/include/troop.h: +0x34.. holds five vectors where Troop has f50 / f58 / nib)
    char pad0[0x34];
    std::vector<int> groups[5];                             // +0x34 (first at +0x38): mesh handles per group (0x473570)
    int ngroups;                                            // +0x84
    char pad88[0xd0 - 0x88];
    tstring name;                                           // +0xd0
};
struct TroopMgr {
    char pad0[0x38];
    MeshRef* ref;                                           // +0x38 (0x470db0)
    void apply_group(ArrSet* dst, int idx, rgl::mesh* obj); // 0x470db0 (thiscall, ret 0xc): nearest base point per point of obj
    void copy_troop(ArrSet* dst, const MeshGroupRec* r);        // 0x473570
};
// copy_troop calls apply_group through the same address from outside its own definition: cl must not assume its
// register use (the slice compiled them apart).
struct TroopMgrExt {
    __forceinline void apply_group(ArrSet* dst, int idx, rgl::mesh* obj) { ((TroopMgr*)this)->apply_group(dst, idx, obj); }   // 0x470db0
};

#endif // TERRAIN_B_TROOP_MGR_H
