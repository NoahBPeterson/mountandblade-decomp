//-----------------------------------------------------------------------------
// File: mesh_3.cpp  (0x52cf60..0x52dae0)
//
// Desc: the map editor's per-frame update (0x52cf60), the polygon-mesh picking and editing members (0x52da20..0x52edb0:
//       pick_vert / vert_ray / pick_edge / edge_ray / pick_poly / poly_ray, toggle_flag, scale / transform / remove the
//       selection, make_copy / vertex_index / edge_index / apply_copy), the editor panel update (0x52f250), the view
//       controller's apply (0x52fe40) and the mesh-name chooser (0x5302f0). Names provisional. "T regime" region
//       (docs/tu_map.md): the file keeps the binary order. Flags: /O2 /MT /Gy /TP.
//
// Boundary: starts at 0x52cf60 (class change Elem | Mesh) and ends at 0x5305b0 where the map editor dialog starts
//       (class change Mesh | MapEditorDialog); no forward call proves either (T regime: Mission::update_ui calls 0x52cf60,
//       0x52f250 and 0x5305b0 in turn, so all three may belong to the caller's object).
//-----------------------------------------------------------------------------
#include "./mesh_3.h"
#include "../strat/gui_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../../rgl/mesh.h"
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)

#include "../key_names.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----






// ======================================================= slice s0052cf60: functions
#undef g_game_s2cf60
#define g_game_s2cf60 (*(Game_s2cf60**)&g_game)
#undef g_scene_s2cf60
#define g_scene_s2cf60 (*(Scene_s2cf60**)&(*(void**)((char*)&g_mission + 0x0)))
#undef g_editor_dialog_s2cf60
#define g_editor_dialog_s2cf60 (*(EditorPanel_s2cf60*)&g_app_obj)
// the 2x2 identity as the original expands it here (the body of identity_2x2 0x4f3be0: two row temporaries, copied)
static inline void rot2_identity(rot2* m) { rot2_row t(1.0f, 0.0f), u(0.0f, 1.0f); m->r[0] = t; m->r[1] = u; }
// @ 0x52cf60
void MapEditor_s2cf60::update()
{
    float dt;
    int   picked;

    g_scene_s2cf60->nav.mode = g_game_s2cf60->sub_option;

    // --- pick the mesh_s2cf60 element under the cursor ------------------------------------------------
    if (g_keyboard.down[225] != 0 && g_keyboard.prev[225] == 0) {
        vector4 from, to;
        this->view->screen_to_world(from, to);
        vector4 dir(to.x - from.x, to.y - from.y, to.z - from.z);
        dir.normalize();
        gui_manager_s2cf60& ms = g_scene_s2cf60->nav;   // one this pointer for the three arms
        if (ms.mode == 0)
            picked = (int)ms.pick_vert(from, dir);
        else if (ms.mode == 1)
            picked = (int)ms.pick_edge(from, dir);
        else
            picked = (int)ms.pick_poly(from, dir);
        if (g_keyboard.down[29] == 0 && g_keyboard.down[157] == 0) {
            g_scene_s2cf60->nav.select_mode(g_scene_s2cf60->nav.mode, 0);
            g_scene_s2cf60->nav.need_redraw[g_scene_s2cf60->nav.mode] = 1;
        }
        if (picked >= 0) {
            g_scene_s2cf60->nav.toggle_flag(g_scene_s2cf60->nav.mode, picked);
            g_scene_s2cf60->nav.need_redraw[g_scene_s2cf60->nav.mode] = 1;
        }
    }

    dt = 0.6f;
    if (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0)
        dt *= 0.1f;                        // folded to 0x3d75c290 (0.06f is 0x3d75c28f)

    // --- mode switch keys ----------------------------------------------------------------------
    if (g_keyboard.down[2] != 0 && g_keyboard.prev[2] == 0) {
        g_game_s2cf60->sub_option = 0;
        g_scene_s2cf60->nav.mode = g_game_s2cf60->sub_option;
        g_editor_dialog_s2cf60.update_checks();
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
    }
    if (g_keyboard.down[3] != 0 && g_keyboard.prev[3] == 0) {
        g_game_s2cf60->sub_option = 1;
        g_scene_s2cf60->nav.mode = g_game_s2cf60->sub_option;
        g_editor_dialog_s2cf60.update_checks();
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
    }
    if (g_keyboard.down[4] != 0 && g_keyboard.prev[4] == 0) {
        g_game_s2cf60->sub_option = 2;
        g_scene_s2cf60->nav.mode = g_game_s2cf60->sub_option;
        g_editor_dialog_s2cf60.update_checks();
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
    }

    // --- free drag: rotate the two 2x2 basis blocks by the editor's angle, then move the terrain
    //     along the rotated step ------------------------------------------------------------------
    if (g_keyboard.down[34] != 0) {
        rot2 basis;
        rot2_identity(&basis);
        basis.rotate_2x2(this->yaw);
        float a = g_keyboard.axis[1] * 0.01;
        float b = g_keyboard.axis[0] * 0.01;
        a *= dt;
        b *= dt;
        rot2_row t = basis.r[0] * a;
        rot2_row u = basis.r[1] * b;
        rot2_row d = t - u;
        g_scene_s2cf60->nav.update_terrain(g_scene_s2cf60->nav.mode, vector4(d.x, d.y, 0.0f));
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

    // --- lift / lower (only while no modifier is held) ------------------------------------------
    if (g_keyboard.down[20] != 0) {
        float z;
        if (g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0)
            z = -((dt * 0.01) * g_keyboard.axis[1]);
        else
            z = 0.0f;
        g_scene_s2cf60->nav.update_terrain(g_scene_s2cf60->nav.mode, vector4(0.0f, 0.0f, z));
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

    // --- the three per-mode rotate keys (blocked while a modifier is held) -----------------------
    if (g_keyboard.down[45] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 && g_keyboard.down[48] == 0) {
        mat3_s2cf60  m;
        float angle;
        identity3(&m);
        angle = ((g_keyboard.axis[1] + g_keyboard.axis[0]) * dt) * 0.005f;
        m.rotate_fu(angle);
        g_scene_s2cf60->nav.transform_selected(g_scene_s2cf60->nav.mode, m.m);
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }
    if (g_keyboard.down[21] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 && g_keyboard.down[48] == 0) {
        mat3_s2cf60  m;
        float angle;
        identity3(&m);
        angle = ((g_keyboard.axis[1] + g_keyboard.axis[0]) * dt) * 0.005f;
        m.rotate_su(angle);
        g_scene_s2cf60->nav.transform_selected(g_scene_s2cf60->nav.mode, m.m);
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }
    if (g_keyboard.down[44] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
        if (g_keyboard.down[48] != 0)
            goto L5c6;
        {
            mat3_s2cf60  m;
            float angle;
            identity3(&m);
            angle = ((g_keyboard.axis[1] + g_keyboard.axis[0]) * dt) * 0.005f;
            m.rotate_sf(angle);
            g_scene_s2cf60->nav.transform_selected(g_scene_s2cf60->nav.mode, m.m);
            g_scene_s2cf60->nav.need_redraw[0] = 1;
            g_scene_s2cf60->nav.need_redraw[1] = 1;
            g_scene_s2cf60->nav.need_redraw[2] = 1;
            g_game_s2cf60->dirty = 1;
        }
    }

    // --- highlight colour: the "no key" colour dims all three lanes, a held rotate key dims one -
L59f:
    if (g_keyboard.down[48] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0)
        goto L5c6;
    goto L666;

L5c6:
    if (g_keyboard.down[45] != 0)
        goto L673;
    if (g_keyboard.down[21] != 0)
        goto L728;
    if (g_keyboard.down[44] != 0)
        goto L7dd;
    {
        vector4 c;
        float dim = g_one - (((g_keyboard.axis[1] + g_keyboard.axis[0]) * dt) * 0.005f) * g_hilite_dim;
        c.x = dim;
        c.y = dim;
        c.z = dim;
        g_scene_s2cf60->nav.scale_selected(g_scene_s2cf60->nav.mode, c);
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

L666:
    if (g_keyboard.down[45] == 0)
        goto L71b;
L673:
    if (g_keyboard.down[48] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
        float dim = g_one - (((g_keyboard.axis[1] + g_keyboard.axis[0]) * dt) * 0.005f) * g_hilite_dim;
        vector4 c;
        c.x = dim;
        c.y = 1.0f;
        c.z = 1.0f;
        g_scene_s2cf60->nav.scale_selected(g_scene_s2cf60->nav.mode, c);
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

L71b:
    if (g_keyboard.down[21] == 0)
        goto L7d0;
L728:
    if (g_keyboard.down[48] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
        float dim = g_one - (((g_keyboard.axis[1] + g_keyboard.axis[0]) * dt) * 0.005f) * g_hilite_dim;
        vector4 c;
        c.x = 1.0f;
        c.y = dim;
        c.z = 1.0f;
        g_scene_s2cf60->nav.scale_selected(g_scene_s2cf60->nav.mode, c);
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

L7d0:
    if (g_keyboard.down[44] == 0)
        goto L885;
L7dd:
    if (g_keyboard.down[48] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
        float dim = g_one - (((g_keyboard.axis[1] + g_keyboard.axis[0]) * dt) * 0.005f) * g_hilite_dim;
        vector4 c;
        c.x = 1.0f;
        c.y = 1.0f;
        c.z = dim;
        g_scene_s2cf60->nav.scale_selected(g_scene_s2cf60->nav.mode, c);
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

L885:
    // --- rebuild every per-mode table ----------------------------------------------------------
    if (g_keyboard.down[0xd3] != 0 && g_keyboard.prev[0xd3] == 0) {
        g_scene_s2cf60->nav.remove_selected();
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

    // --- drag the selection (needs a held code 0x52/0xd2 key) ----------------------------------
    if (g_keyboard.down[46] != 0 && g_keyboard.prev[46] == 0 && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0))
        g_scene_s2cf60->nav.make_copy(&this->drag_frame);

    if (g_keyboard.down[47] != 0 && g_keyboard.prev[47] == 0 && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) &&
        g_keyboard.down[42] == 0 && g_keyboard.down[0x36] == 0) {
        g_scene_s2cf60->nav.apply_copy(&this->drag_frame);
        g_scene_s2cf60->nav.need_redraw[0] = 1;
        g_scene_s2cf60->nav.need_redraw[1] = 1;
        g_scene_s2cf60->nav.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

    // --- rebuild the three rgl meshes that were flagged ----------------------------------------
    if (g_scene_s2cf60->nav.need_redraw[0] != 0)
        g_scene_s2cf60->nav.build_spikes(g_scene_s2cf60->nav.marker_spikes);
    if (g_scene_s2cf60->nav.need_redraw[1] != 0)
        g_scene_s2cf60->nav.build_ribbons(g_scene_s2cf60->nav.marker_ribbons);
    if (g_scene_s2cf60->nav.need_redraw[2] != 0)
        g_scene_s2cf60->nav.polygon_to_mesh(g_scene_s2cf60->nav.marker_poly);
    g_scene_s2cf60->nav.need_redraw[0] = 0;
    g_scene_s2cf60->nav.need_redraw[1] = 0;
    g_scene_s2cf60->nav.need_redraw[2] = 0;
}

// ======================================================= slice s0052da20: functions
// @ 0x52da20
unsigned Mesh_s2da20::pick_vert(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)verts.size(); i++) {
        float a, b;
        vert_ray(p, d, i, &a, &b);
        if (b < 2.0f)
            b = 2.0f;
        float ratio = a / b;
        if (ratio < 0.1f && ratio < best) {
            besti = i;
            best = ratio;
        }
    }
    return besti;
}
