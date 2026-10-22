// Slice s0052cf60: the world-map mesh editor's per-frame update (0x52cf60, 2749 bytes).
//
// One thiscall member of the map editor.  Each frame it
//   * mirrors the game's selected sub-option into the editable mesh (g_scene->mesh, a gui_manager
//     living at 0x5e292c+0x6698),
//   * picks the mesh element under the cursor (vertex / edge / polygon, depending on the mesh mode),
//   * handles the three mode-switch keys,
//   * applies the translate (0x17124) and the three rotation (0x1715c / 0x17188 / 0x17128 / 0x17184)
//     keys, building the transform with identity3 + matrix_rotate_fu/su/sf,
//   * sets the per-mode highlight colour (set_colour) - the original shares that code between several
//     entry points, hence the goto labels L59f/L5c6/... mirroring the basic blocks,
//   * runs the two drag keys, and
//   * rebuilds whichever of the three rgl meshes were flagged dirty.
//
// Names are the repo's where an earlier slice already resolved them (select_mode 0x4c53f0,
// update_terrain 0x4c7f90, pick_vert/edge/poly 0x52da20/0x52dc00/0x52df40, build_tetra/ribbon/tri
// 0x4e65e0/0x4e6a70/0x4e75a0, cell_rect 0x4bd2f0, identity3 0x40cd60, matrix_rotate_fu/su/sf
// 0x40e570/0x40e670/0x40e770, rotate_2x2 0x4f3b40, update_option_checks 0x4bf640).  The remaining
// names are provisional; for those the address is the evidence.
#include <windows.h>
#include "rgl.h"

using rgl::vector4;

extern "C" int _fltused = 0;

namespace rgl {
struct mesh;                                        // only used through pointers here
}

// ---- gui_manager: the editable mesh (layout from slices s004c5a20 and s0052da20) ----------------
// vertices/polys/edges vectors at +0x18/+0x28/+0x38 (0x18/0x64/0x34-byte elements), +0x78 mode,
// +0x7c current element.  The members past +0x80 are only seen in this run of functions.
struct gui_manager {
    char pad0[0x78];
    int  mode;                                      // +0x78  0 vertex, 1 edge, 2 polygon
    int  cur;                                       // +0x7c  current element index
    char pad80[0x94 - 0x80];
    rgl::mesh* mesh94;                              // +0x94  rebuilt with build_tri
    rgl::mesh* mesh98;                              // +0x98  rebuilt with build_tetra
    rgl::mesh* mesh9c;                              // +0x9c  rebuilt with build_ribbon
    char padA0[0xac - 0xa0];
    char need_redraw[3];                            // +0xac..+0xae  one rebuild flag per mode

    unsigned pick_vert(const vector4& p, const vector4& d);     // 0x52da20
    unsigned pick_edge(const vector4& p, const vector4& d);     // 0x52dc00
    unsigned pick_poly(const vector4& p, const vector4& d);     // 0x52df40
    void select_mode(int mode, char on);                        // 0x4c53f0
    void select_current(int mode, int index);                   // 0x52e510 (provisional)
    void set_colour(int mode, const vector4& c);                // 0x52e590 (provisional)
    void set_basis(int mode, const float* m);                   // 0x52e6d0 (provisional)
    void rebuild_all();                                         // 0x52e870 (provisional)
    void drag_start(void* state);                               // 0x52e960 (provisional)
    void drag_step(void* state);                                // 0x52edb0 (provisional)
    void update_terrain(int mode, const vector4& dir);          // 0x4c7f90
    void build_tetra(rgl::mesh* m);                             // 0x4e65e0
    void build_ribbon(rgl::mesh* m);                            // 0x4e6a70
    void build_tri(rgl::mesh* m);                               // 0x4e75a0
};

// ---- engine globals ---------------------------------------------------------------------------
struct Game {
    char pad0[0x1a1cc];
    int dirty;                                      // +0x1a1cc  view dirty, cleared by the renderer
    int sub_option;                                 // +0x1a1d0  selected map-edit sub-option (0..2)
};
extern Game* g_game;                                // 0x5e2930

struct Scene {
    char pad0[0x6698];
    gui_manager mesh;                               // +0x6698  (s0052f250 calls this object Map)
};
extern Scene* g_scene;                              // 0x5e292c

struct MapView {
    void cell_rect(vector4& p, vector4& q);         // 0x4bd2f0  picking ray (origin, target)
};

struct MapEditor {
    char      pad0[0x60];
    MapView*  view;                                 // +0x60
    char      pad64[0x228 - 0x64];
    char      drag_state[0x268 - 0x228];            // +0x228  handed to 0x52e960 / 0x52edb0
    float     angle;                                // +0x268  handed to rotate_2x2

    void update();                                  // 0x52cf60
};

struct EditorPanel {
    void update_option_checks();                    // 0x4bf640  (no arguments)
};
extern EditorPanel g_editor_dialog;                 // 0x8ee3c48  (object, not a pointer)

// ---- transform helpers ------------------------------------------------------------------------
struct mat3 {                                       // three basis rows, 0x30 bytes
    float m[12];
    void rotate_fu(const float& angle);             // 0x40e570  forward/up rows by *angle
    void rotate_su(const float& angle);             // 0x40e670  side/up rows by *angle
    void rotate_sf(const float& angle);             // 0x40e770  side/forward rows by *angle
};
void identity3(mat3* m);                            // 0x40cd60 (cdecl)

struct mat2 {                                       // 2x2 rotation matrix, 0x10 bytes
    float m[4];
    void rotate(float angle);                       // 0x4f3b40 (thiscall, ret 4)
};

// ---- per-key input state ----------------------------------------------------------------------
// Three 0x400-byte tables of per-scan-code ints at 0xa17000 / 0xa17400 / 0xa17800: held state,
// this frame's press, and the previous frame's press (s0052f250 names 0xa17458/0xa17858
// g_key_place/g_key_place_prev).  Only the codes read here are declared.
extern int g_a170dc;                                // 0xa170dc  held, code 0x37
extern int g_a170e0;                                // 0xa170e0  held, code 0x38
extern int g_a170e4;                                // 0xa170e4  held, code 0x39
extern int g_a17124;                                // 0xa17124  held, code 0x49
extern int g_a17128;                                // 0xa17128  held, code 0x4a
extern int g_a17148;                                // 0xa17148  held, code 0x52
extern int g_a1715c;                                // 0xa1715c  held, code 0x57
extern int g_a1717c;                                // 0xa1717c  held, code 0x5f
extern int g_a17184;                                // 0xa17184  held, code 0x61
extern int g_a17188;                                // 0xa17188  held, code 0x62
extern int g_a1718c;                                // 0xa1718c  held, code 0x63
extern int g_a17190;                                // 0xa17190  held, code 0x64
extern int g_a17194;                                // 0xa17194  held, code 0x65
extern int g_a171ac;                                // 0xa171ac  held, code 0x6b
extern int g_a171b4;                                // 0xa171b4  held, code 0x6d
extern int g_a17348;                                // 0xa17348  held, code 0xd2
extern int g_a173b4;                                // 0xa173b4  held, code 0xed
extern int g_a17420;                                // 0xa17420  press, code 0x08
extern int g_a17458;                                // 0xa17458  press, code 0x16
extern int g_a174dc;                                // 0xa174dc  press, code 0x37
extern int g_a174e0;                                // 0xa174e0  press, code 0x38
extern int g_a174e4;                                // 0xa174e4  press, code 0x39
extern int g_a1758c;                                // 0xa1758c  press, code 0x63
extern int g_a17590;                                // 0xa17590  press, code 0x64
extern int g_a17820;                                // 0xa17820  previous, code 0x08
extern int g_a17858;                                // 0xa17858  previous, code 0x16
extern float g_a178d4;                              // 0xa178d4  camera drag step
extern float g_a178d8;                              // 0xa178d8  camera drag step
extern const float g_one;                           // 0x549cdc = 1.0f
extern const float g_hilite_dim;                    // 0x54b4d0 = 0.05f
extern const float g_rot_step;                      // 0x590c60 = 0.005f
extern const double g_percent;                      // 0x59c480 = 0.01

// @ 0x52cf60
void MapEditor::update()
{
    float dt;
    int   picked;

    g_scene->mesh.mode = g_game->sub_option;

    // --- pick the mesh element under the cursor ------------------------------------------------
    if (g_a17458 != 0 && g_a17858 == 0) {
        vector4 from, to;
        this->view->cell_rect(from, to);
        vector4 dir;
        dir.x = to.x - from.x;
        dir.y = to.y - from.y;
        dir.z = to.z - from.z;
        dir.normalize();
        if (g_scene->mesh.mode == 0)
            picked = (int)g_scene->mesh.pick_vert(from, dir);
        else if (g_scene->mesh.mode == 1)
            picked = (int)g_scene->mesh.pick_edge(from, dir);
        else
            picked = (int)g_scene->mesh.pick_poly(from, dir);
        if (g_a17148 == 0 && g_a17348 == 0) {
            g_scene->mesh.select_mode(g_scene->mesh.mode, 0);
            g_scene->mesh.need_redraw[g_scene->mesh.mode] = 1;
        }
        if (picked >= 0) {
            g_scene->mesh.select_current(g_scene->mesh.mode, picked);
            g_scene->mesh.need_redraw[g_scene->mesh.mode] = 1;
        }
    }

    dt = 0.6f;
    if (g_a17148 != 0 || g_a17348 != 0)
        dt = 0.06f;

    // --- mode switch keys ----------------------------------------------------------------------
    if (g_a170dc != 0 && g_a174dc == 0) {
        g_game->sub_option = 0;
        g_scene->mesh.mode = g_game->sub_option;
        g_editor_dialog.update_option_checks();
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
    }
    if (g_a170e0 != 0 && g_a174e0 == 0) {
        g_game->sub_option = 1;
        g_scene->mesh.mode = g_game->sub_option;
        g_editor_dialog.update_option_checks();
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
    }
    if (g_a170e4 != 0 && g_a174e4 == 0) {
        g_game->sub_option = 2;
        g_scene->mesh.mode = g_game->sub_option;
        g_editor_dialog.update_option_checks();
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
    }

    // --- free drag: rotate the two 2x2 basis blocks by the editor's angle, then move the terrain
    //     along the rotated step ------------------------------------------------------------------
    if (g_a1715c != 0) {
        mat2 keep  = {{1.0f, 0.0f, 0.0f, 1.0f}};   // the original initialises a second identity block
        mat2 basis = {{1.0f, 0.0f, 0.0f, 1.0f}};
        basis.rotate(this->angle);
        float a = (g_a178d4 * dt) * g_percent;
        float b = (g_a178d8 * dt) * g_percent;
        vector4 pos;
        pos.x = basis.m[0] * a - basis.m[2] * b;
        pos.y = basis.m[1] * a - basis.m[3] * b;
        pos.z = 0.0f;
        g_scene->mesh.update_terrain(g_scene->mesh.mode, pos);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }

    // --- lift / lower (only while no modifier is held) ------------------------------------------
    if (g_a17124 != 0) {
        vector4 pos;
        pos.x = 0.0f;
        pos.y = 0.0f;
        if (g_a171b4 == 0 && g_a173b4 == 0)
            pos.z = -((dt * g_percent) * g_a178d8);
        else
            pos.z = 0.0f;
        g_scene->mesh.update_terrain(g_scene->mesh.mode, pos);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }

    // --- the three per-mode rotate keys (blocked while a modifier is held) -----------------------
    if (g_a17188 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        mat3  m;
        float angle;
        identity3(&m);
        angle = ((g_a178d8 + g_a178d4) * dt) * g_rot_step;
        m.rotate_fu(angle);
        g_scene->mesh.set_basis(g_scene->mesh.mode, m.m);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }
    if (g_a17128 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        mat3  m;
        float angle;
        identity3(&m);
        angle = ((g_a178d8 + g_a178d4) * dt) * g_rot_step;
        m.rotate_su(angle);
        g_scene->mesh.set_basis(g_scene->mesh.mode, m.m);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }
    if (g_a17184 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        if (g_a17194 != 0)
            goto L5c6;
        {
            mat3  m;
            float angle;
            identity3(&m);
            angle = ((g_a178d8 + g_a178d4) * dt) * g_rot_step;
            m.rotate_sf(angle);
            g_scene->mesh.set_basis(g_scene->mesh.mode, m.m);
            g_scene->mesh.need_redraw[0] = 1;
            g_scene->mesh.need_redraw[1] = 1;
            g_scene->mesh.need_redraw[2] = 1;
            g_game->dirty = 1;
        }
    }

    // --- highlight colour: the "no key" colour dims all three lanes, a held rotate key dims one -
L59f:
    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0)
        goto L5c6;
    goto L666;

L5c6:
    if (g_a17188 != 0)
        goto L673;
    if (g_a17128 != 0)
        goto L728;
    if (g_a17184 != 0)
        goto L7dd;
    {
        float dim = g_one - (((g_a178d8 + g_a178d4) * dt) * g_rot_step) * g_hilite_dim;
        vector4 c;
        c.x = dim;
        c.y = dim;
        c.z = dim;
        g_scene->mesh.set_colour(g_scene->mesh.mode, c);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }

L666:
    if (g_a17188 == 0)
        goto L71b;
L673:
    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float dim = g_one - (((g_a178d8 + g_a178d4) * dt) * g_rot_step) * g_hilite_dim;
        vector4 c;
        c.x = dim;
        c.y = 1.0f;
        c.z = 1.0f;
        g_scene->mesh.set_colour(g_scene->mesh.mode, c);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }

L71b:
    if (g_a17128 == 0)
        goto L7d0;
L728:
    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float dim = g_one - (((g_a178d8 + g_a178d4) * dt) * g_rot_step) * g_hilite_dim;
        vector4 c;
        c.x = 1.0f;
        c.y = dim;
        c.z = 1.0f;
        g_scene->mesh.set_colour(g_scene->mesh.mode, c);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }

L7d0:
    if (g_a17184 == 0)
        goto L885;
L7dd:
    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float dim = g_one - (((g_a178d8 + g_a178d4) * dt) * g_rot_step) * g_hilite_dim;
        vector4 c;
        c.x = 1.0f;
        c.y = 1.0f;
        c.z = dim;
        g_scene->mesh.set_colour(g_scene->mesh.mode, c);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }

L885:
    // --- rebuild every per-mode table ----------------------------------------------------------
    if (g_a17420 != 0 && g_a17820 == 0) {
        g_scene->mesh.rebuild_all();
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }

    // --- drag the selection (needs a held code 0x52/0xd2 key) ----------------------------------
    if (g_a1718c != 0 && g_a1758c == 0 && (g_a17148 != 0 || g_a17348 != 0))
        g_scene->mesh.drag_start(this->drag_state);

    if (g_a17190 != 0 && g_a17590 == 0 && (g_a17148 != 0 || g_a17348 != 0) &&
        g_a1717c == 0 && g_a171ac == 0) {
        g_scene->mesh.drag_step(this->drag_state);
        g_scene->mesh.need_redraw[0] = 1;
        g_scene->mesh.need_redraw[1] = 1;
        g_scene->mesh.need_redraw[2] = 1;
        g_game->dirty = 1;
    }

    // --- rebuild the three rgl meshes that were flagged ----------------------------------------
    if (g_scene->mesh.need_redraw[0] != 0)
        g_scene->mesh.build_tetra(g_scene->mesh.mesh98);
    if (g_scene->mesh.need_redraw[1] != 0)
        g_scene->mesh.build_ribbon(g_scene->mesh.mesh9c);
    if (g_scene->mesh.need_redraw[2] != 0)
        g_scene->mesh.build_tri(g_scene->mesh.mesh94);
    g_scene->mesh.need_redraw[0] = 0;
    g_scene->mesh.need_redraw[1] = 0;
    g_scene->mesh.need_redraw[2] = 0;
}
