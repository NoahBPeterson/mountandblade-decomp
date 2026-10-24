// res_mesh.h: a resource mesh handed out by the resource manager (global 0x5d6fa0) and the image table (global 0xa75f40), as the
// game modules that do not share rgl_mesh.h's rgl::mesh call it: clone 0x41c6d0, translate 0x44f240 (ret 4). ONE definition of the
// stub triple for src/game/party (party_window.h: MeshOps, ResMgr, ObjTable) and src/game/inventory_panel (char_sheet.cpp: Mesh,
// ResMgr; party_screen.h: ObjTable, inv_common.h: RMesh). The vector parameter of translate is untyped (the party window passes a
// rgl_math vector4, other modules a rgl.h one).
//   ResMgr::get_object 0x419540 (thiscall, ret 4)   a mesh resource by name (callers clone it)
//   ObjTable::make_mesh_from 0x4991d0 (ret 8)      a display mesh for (selected, slot)
#ifndef GAME_RES_MESH_H
#define GAME_RES_MESH_H
#include "./tstring.h"

struct ResMesh {
    ResMesh* clone();                       // 0x41c6d0
    void translate(const void* v);          // 0x44f240 (ret 4): a vector4
};
struct ResMgr {
    ResMesh* get_object(const tstring* name);   // 0x419540 (ret 4)
    ResMesh* get_object(const tstring& name);   // 0x419540 (ret 4): the same call with the key as an object
};
struct ObjTable {
    ResMesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0 (ret 8): a display mesh for (selected, slot)
};

#endif // GAME_RES_MESH_H
