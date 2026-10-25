// res_mgr.h: the resource manager (global 0x5d6fa0, full layout in src/rgl/core/resmgr.h) as the game-side modules of the rgl.h
// universe call it: the meshes it hands out are rgl::mesh of rgl_mesh.h. ONE definition for src/game/char_sheet (editor_ui.h,
// facegen_win.h, quest_log.h), src/game/inventory_panel (view_open.cpp, char_window.cpp, map_picker.cpp, game_app.cpp). The modules
// whose mesh class is their own (conv_open.h / conv_refresh.h base_mesh, facegen_init.h, entity.h, strat_ext.h, agent/item_kind.h,
// party_window.h MeshOps, mesh_start.cpp Mesh, inventory_panel/char_sheet.cpp Mesh) cannot share it: the return type of get_object
// is part of the declaration.
//   get_object 0x419540 (thiscall, ret 4)   a mesh resource by name (callers clone it, 0x41c6d0)
//   get_material 0x418a70                    a material by name
//   load_pack 0x41e150                       a resource pack by name
//   unload 0x418ff0                          (game_app.cpp: called with the id the main loop saved in 0x5e293c)
#ifndef GAME_RES_MGR_H
#define GAME_RES_MGR_H
#include "./tstring.h"
#include "./rgl_mesh.h"

namespace rgl { struct string; }
struct resmgr {
    // (added: the definition's own signatures, core/resmgr.cpp; the const tstring& views below name no symbol that exists)
    rgl::mesh* get_object(const rgl::string* name);         // 0x419540 (ret 4)
    void load_pack(const rgl::string& name, int user);      // 0x41e150
    rgl::mesh* get_object(const tstring& name);             // 0x419540 (ret 4)
    rgl::mesh* get_object(tstring* name);                   // 0x419540 (ret 4)
    void* get_material(const tstring& name);                // 0x418a70
    void load_pack(const tstring& name, int user);          // 0x41e150
    void resolve_effect_sets(int id);                                    // 0x418ff0
};

#endif // GAME_RES_MGR_H
