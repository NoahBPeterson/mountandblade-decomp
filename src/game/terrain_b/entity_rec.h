// entity_rec.h: the scene entity created by CellGrid::create_entity (0x428440; type 9 is a mesh object) as far as the
// mission's interior and dust-grid builders use it: a vtable with refresh (slot 4) and a distance setter (slot 6), and
// the pointer vector of meshes at +0x118.
#ifndef TERRAIN_B_ENTITY_REC_H
#define TERRAIN_B_ENTITY_REC_H
#include "./tmesh.h"

struct Entity {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3();
    virtual void refresh();                     // slot 4 (+0x10): rebuild from the owned meshes
    virtual void v5();
    virtual void set_distance(float* dist);     // slot 6 (+0x18)
    virtual void v7();
    char pad4[0x118 - 4];
    MeshVec meshes;                             // +0x118 (first at +0x11c)
};

#endif // TERRAIN_B_ENTITY_REC_H
