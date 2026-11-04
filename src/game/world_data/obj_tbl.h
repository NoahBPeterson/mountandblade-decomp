// obj_tbl.h: the editor's scene-record table (MeshTable::placed, 0x499050), the tableau table spawners (TableauTbl, tableau.h) and the
// mesh-widget factories (WidgetMgrW, widget_mgr_w.h) of translation unit 87 (obj_tbl) of the TU map: resource record -> mesh ->
// entity helpers. Names provisional; offsets cite the function that proves them.
#ifndef GAME_WORLD_DATA_OBJ_TBL_H
#define GAME_WORLD_DATA_OBJ_TBL_H
#include <math.h>
#include "./mesh_hud.h"
#include "./raw_place.h"
#include "../../include/mat4.h"   // rgl::mat3
#include "../game_globals.h"      // canonical g_map_icons
#include "./tableau.h"
#include "./widget_mgr_w.h"
#include "../../include/render_flags.h"

// ---- a 3x3 frame (the rotation rows of a matrix): identity3 writes 0x30 bytes -------------------------------------------------------------
// (rgl::mat3 of include/mat4.h; identity3 is the global-scope writer 0x40cd60 declared in world_data.h; the axis rotations
// 0x40e570 / 0x40e670 / 0x40e770 are rgl::matrix's own, applied to the frame as the matrix it starts)
struct Frame3 : rgl::mat3 {};
// 0x442110 is agent3's Poser::add_extra (the entity takes the extra mesh as an int); 0x428440 is rgl::world::create_entity
// (Poser::add_extra is declared in mesh_hud.h)
#include "../../rgl/scene/world.h"            // rgl::world::create_entity 0x428440

// ---- the scene a tableau renders into and the entity it creates -----------------------------------------------------------------------------
struct TableauEntity {
    void* vptr;                                                 // +0x00
    char pad4[0x58 - 4];
    rgl::matrix local;                                          // +0x58 (0x4992b0: the frame copied into the new entity)
};
struct TableauWorld {
    // create_entity (0x428440) is rgl::world's: callers use ((rgl::world*)w)->create_entity(id)
};

// ---- the editor's 0x48-byte scene record ----------------------------------------------------------------------------------------------------
struct SceneRec48 {
    rgl::string name;                                           // +0x00 resource name
    unsigned char flags;                                        // +0x10 bit 0: the mesh gets the 0x2000 flag
    char pad1[0x24 - 0x11];
    float px, py, pz;                                           // +0x24
    float rx, ry, rz;                                           // +0x30 rotations in degrees
    float sx, sy, sz;                                           // +0x3c scale
};

extern float g_one;                                             // 0x549cdc (1.0f)
extern float g_zero;                                            // 0x549ca8 (0.0f)
extern int g_use_shaders;                                        // 0x5e28fc (non-zero: gamma on colours)
extern float g_zero;                                            // 0x549ca8 (0.0f)

#endif // GAME_WORLD_DATA_OBJ_TBL_H
