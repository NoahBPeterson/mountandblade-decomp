// obj_tbl.h: the editor's scene-record table (scene_tbl, 0x499050), the tableau table spawners (TableauTbl, tableau.h) and the
// mesh-widget factories (WidgetMgrW, widget_mgr_w.h) of translation unit 87 (obj_tbl) of the TU map: resource record -> mesh ->
// entity helpers. Names provisional; offsets cite the function that proves them.
#ifndef GAME_WORLD_DATA_OBJ_TBL_H
#define GAME_WORLD_DATA_OBJ_TBL_H
#include <math.h>
#include "./mesh_hud.h"
#include "./tableau.h"
#include "./widget_mgr_w.h"

// ---- mesh members the shared headers lack (same object as rgl::mesh) ---------------------------------------------------------------------
struct MeshY : MeshX {
    void translate(const vector4* d);                           // 0x44f240
    void set_color(unsigned c);                                 // 0x47b360
    void add_quad(const vector4* a, const vector4* b, const vec2* uv0, const vec2* uv1, const unsigned* color);   // 0x44e970
};

// ---- a 3x3 frame (the rotation rows of a matrix): identity3 writes 0x30 bytes -------------------------------------------------------------
struct Frame3 {
    rgl::vector4 s, f, u;
    void rotate_about_s(const float* angle);                    // 0x40e570
    void rotate_about_f(const float* angle);                    // 0x40e670
    void rotate_about_u(const float* angle);                    // 0x40e770
};
Frame3* identity3(Frame3* out);                                 // 0x40cd60 (cdecl; returns out)

// ---- the scene a tableau renders into and the entity it creates -----------------------------------------------------------------------------
struct TableauEntity {
    void* vptr;                                                 // +0x00
    char pad4[0x58 - 4];
    rgl::matrix local;                                          // +0x58 (0x4992b0: the frame copied into the new entity)
    void add_mesh(rgl::mesh* m);                                // 0x442110 (thiscall, ret 4)
};
struct TableauWorld {
    TableauEntity* create_entity(int type_id);                  // 0x428440 (thiscall, ret 4)
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
extern int g_gamma_flag;                                        // 0x5e28fc (non-zero: gamma on colours)
extern void* g_mesh_recs;                                       // 0x5e29f0 (the 0x48-stride mesh records: name at +0x14)

#endif // GAME_WORLD_DATA_OBJ_TBL_H
