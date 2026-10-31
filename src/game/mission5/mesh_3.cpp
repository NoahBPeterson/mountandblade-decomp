//-----------------------------------------------------------------------------
// File: mesh_3.cpp  (0x52cf60..0x5305b0)
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
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../../rgl/mesh.h"

// ======================================================= slice s0052cf60: functions
#undef g_game_s2cf60
#define g_game_s2cf60 (*(Game_s2cf60**)&g_game)
#undef g_scene_s2cf60
#define g_scene_s2cf60 (*(Scene_s2cf60**)&(*(void**)((char*)&g_mission + 0x0)))
#undef g_editor_dialog_s2cf60
#define g_editor_dialog_s2cf60 (*(EditorPanel_s2cf60*)g_app_obj)
// @ 0x52cf60
void MapEditor_s2cf60::update()
{
    float dt;
    int   picked;

    g_scene_s2cf60->mesh_s2cf60.mode = g_game_s2cf60->sub_option;

    // --- pick the mesh_s2cf60 element under the cursor ------------------------------------------------
    if ((*(int*)((char*)&g_keyboard + 0x390)) != 0 && (*(int*)((char*)&g_keyboard + 0x790)) == 0) {
        vector4 from, to;
        this->view->screen_to_world(from, to);
        vector4 dir;
        dir.x = to.x - from.x;
        dir.y = to.y - from.y;
        dir.z = to.z - from.z;
        dir.normalize();
        if (g_scene_s2cf60->mesh_s2cf60.mode == 0)
            picked = (int)g_scene_s2cf60->mesh_s2cf60.pick_vert(from, dir);
        else if (g_scene_s2cf60->mesh_s2cf60.mode == 1)
            picked = (int)g_scene_s2cf60->mesh_s2cf60.pick_edge(from, dir);
        else
            picked = (int)g_scene_s2cf60->mesh_s2cf60.pick_poly(from, dir);
        if ((*(int*)((char*)&g_keyboard + 0x80)) == 0 && (*(int*)((char*)&g_keyboard + 0x280)) == 0) {
            g_scene_s2cf60->mesh_s2cf60.select_mode(g_scene_s2cf60->mesh_s2cf60.mode, 0);
            g_scene_s2cf60->mesh_s2cf60.need_redraw[g_scene_s2cf60->mesh_s2cf60.mode] = 1;
        }
        if (picked >= 0) {
            g_scene_s2cf60->mesh_s2cf60.toggle_flag(g_scene_s2cf60->mesh_s2cf60.mode, picked);
            g_scene_s2cf60->mesh_s2cf60.need_redraw[g_scene_s2cf60->mesh_s2cf60.mode] = 1;
        }
    }

    dt = 0.6f;
    if ((*(int*)((char*)&g_keyboard + 0x80)) != 0 || (*(int*)((char*)&g_keyboard + 0x280)) != 0)
        dt = 0.06f;

    // --- mode switch keys ----------------------------------------------------------------------
    if (g_a170dc != 0 && g_a174dc == 0) {
        g_game_s2cf60->sub_option = 0;
        g_scene_s2cf60->mesh_s2cf60.mode = g_game_s2cf60->sub_option;
        g_editor_dialog_s2cf60.update_checks();
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
    }
    if (g_a170e0 != 0 && g_a174e0 == 0) {
        g_game_s2cf60->sub_option = 1;
        g_scene_s2cf60->mesh_s2cf60.mode = g_game_s2cf60->sub_option;
        g_editor_dialog_s2cf60.update_checks();
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
    }
    if (g_a170e4 != 0 && g_a174e4 == 0) {
        g_game_s2cf60->sub_option = 2;
        g_scene_s2cf60->mesh_s2cf60.mode = g_game_s2cf60->sub_option;
        g_editor_dialog_s2cf60.update_checks();
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
    }

    // --- free drag: rotate the two 2x2 basis blocks by the editor's angle, then move the terrain
    //     along the rotated step ------------------------------------------------------------------
    if (g_a1715c != 0) {
        mat2_s2cf60 keep  = {{1.0f, 0.0f, 0.0f, 1.0f}};   // the original initialises a second identity block
        mat2_s2cf60 basis = {{1.0f, 0.0f, 0.0f, 1.0f}};
        basis.rotate_2x2(this->angle);
        float a = ((*(float*)((char*)&g_keyboard + 0x80c)) * dt) * g_percent;
        float b = ((*(float*)((char*)&g_keyboard + 0x810)) * dt) * g_percent;
        vector4 pos;
        pos.x = basis.m[0] * a - basis.m[2] * b;
        pos.y = basis.m[1] * a - basis.m[3] * b;
        pos.z = 0.0f;
        g_scene_s2cf60->mesh_s2cf60.update_terrain(g_scene_s2cf60->mesh_s2cf60.mode, pos);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

    // --- lift / lower (only while no modifier is held) ------------------------------------------
    if ((*(int*)((char*)&g_keyboard + 0x5c)) != 0) {
        vector4 pos;
        pos.x = 0.0f;
        pos.y = 0.0f;
        if (g_a171b4 == 0 && g_a173b4 == 0)
            pos.z = -((dt * g_percent) * (*(float*)((char*)&g_keyboard + 0x810)));
        else
            pos.z = 0.0f;
        g_scene_s2cf60->mesh_s2cf60.update_terrain(g_scene_s2cf60->mesh_s2cf60.mode, pos);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

    // --- the three per-mode rotate keys (blocked while a modifier is held) -----------------------
    if (g_a17188 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        mat3_s2cf60  m;
        float angle;
        identity3(&m);
        angle = (((*(float*)((char*)&g_keyboard + 0x810)) + (*(float*)((char*)&g_keyboard + 0x80c))) * dt) * g_rot_step;
        m.rotate_fu(angle);
        g_scene_s2cf60->mesh_s2cf60.transform_selected(g_scene_s2cf60->mesh_s2cf60.mode, m.m);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }
    if (g_a17128 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        mat3_s2cf60  m;
        float angle;
        identity3(&m);
        angle = (((*(float*)((char*)&g_keyboard + 0x810)) + (*(float*)((char*)&g_keyboard + 0x80c))) * dt) * g_rot_step;
        m.rotate_su(angle);
        g_scene_s2cf60->mesh_s2cf60.transform_selected(g_scene_s2cf60->mesh_s2cf60.mode, m.m);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }
    if (g_a17184 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        if (g_a17194 != 0)
            goto L5c6;
        {
            mat3_s2cf60  m;
            float angle;
            identity3(&m);
            angle = (((*(float*)((char*)&g_keyboard + 0x810)) + (*(float*)((char*)&g_keyboard + 0x80c))) * dt) * g_rot_step;
            m.rotate_sf(angle);
            g_scene_s2cf60->mesh_s2cf60.transform_selected(g_scene_s2cf60->mesh_s2cf60.mode, m.m);
            g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
            g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
            g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
            g_game_s2cf60->dirty = 1;
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
        float dim = g_one - ((((*(float*)((char*)&g_keyboard + 0x810)) + (*(float*)((char*)&g_keyboard + 0x80c))) * dt) * g_rot_step) * g_hilite_dim;
        vector4 c;
        c.x = dim;
        c.y = dim;
        c.z = dim;
        g_scene_s2cf60->mesh_s2cf60.scale_selected(g_scene_s2cf60->mesh_s2cf60.mode, c);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

L666:
    if (g_a17188 == 0)
        goto L71b;
L673:
    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float dim = g_one - ((((*(float*)((char*)&g_keyboard + 0x810)) + (*(float*)((char*)&g_keyboard + 0x80c))) * dt) * g_rot_step) * g_hilite_dim;
        vector4 c;
        c.x = dim;
        c.y = 1.0f;
        c.z = 1.0f;
        g_scene_s2cf60->mesh_s2cf60.scale_selected(g_scene_s2cf60->mesh_s2cf60.mode, c);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

L71b:
    if (g_a17128 == 0)
        goto L7d0;
L728:
    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float dim = g_one - ((((*(float*)((char*)&g_keyboard + 0x810)) + (*(float*)((char*)&g_keyboard + 0x80c))) * dt) * g_rot_step) * g_hilite_dim;
        vector4 c;
        c.x = 1.0f;
        c.y = dim;
        c.z = 1.0f;
        g_scene_s2cf60->mesh_s2cf60.scale_selected(g_scene_s2cf60->mesh_s2cf60.mode, c);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

L7d0:
    if (g_a17184 == 0)
        goto L885;
L7dd:
    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float dim = g_one - ((((*(float*)((char*)&g_keyboard + 0x810)) + (*(float*)((char*)&g_keyboard + 0x80c))) * dt) * g_rot_step) * g_hilite_dim;
        vector4 c;
        c.x = 1.0f;
        c.y = 1.0f;
        c.z = dim;
        g_scene_s2cf60->mesh_s2cf60.scale_selected(g_scene_s2cf60->mesh_s2cf60.mode, c);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

L885:
    // --- rebuild every per-mode table ----------------------------------------------------------
    if (g_a17420 != 0 && g_a17820 == 0) {
        g_scene_s2cf60->mesh_s2cf60.remove_selected();
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

    // --- drag the selection (needs a held code 0x52/0xd2 key) ----------------------------------
    if ((*(int*)((char*)&g_keyboard + 0xc4)) != 0 && g_a1758c == 0 && ((*(int*)((char*)&g_keyboard + 0x80)) != 0 || (*(int*)((char*)&g_keyboard + 0x280)) != 0))
        g_scene_s2cf60->mesh_s2cf60.make_copy(this->drag_state);

    if (g_a17190 != 0 && g_a17590 == 0 && ((*(int*)((char*)&g_keyboard + 0x80)) != 0 || (*(int*)((char*)&g_keyboard + 0x280)) != 0) &&
        g_a1717c == 0 && g_a171ac == 0) {
        g_scene_s2cf60->mesh_s2cf60.apply_copy(this->drag_state);
        g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 1;
        g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 1;
        g_game_s2cf60->dirty = 1;
    }

    // --- rebuild the three rgl meshes that were flagged ----------------------------------------
    if (g_scene_s2cf60->mesh_s2cf60.need_redraw[0] != 0)
        g_scene_s2cf60->mesh_s2cf60.build_spikes(g_scene_s2cf60->mesh_s2cf60.mesh98);
    if (g_scene_s2cf60->mesh_s2cf60.need_redraw[1] != 0)
        g_scene_s2cf60->mesh_s2cf60.build_ribbons(g_scene_s2cf60->mesh_s2cf60.mesh9c);
    if (g_scene_s2cf60->mesh_s2cf60.need_redraw[2] != 0)
        g_scene_s2cf60->mesh_s2cf60.polygon_to_mesh(g_scene_s2cf60->mesh_s2cf60.mesh94);
    g_scene_s2cf60->mesh_s2cf60.need_redraw[0] = 0;
    g_scene_s2cf60->mesh_s2cf60.need_redraw[1] = 0;
    g_scene_s2cf60->mesh_s2cf60.need_redraw[2] = 0;
}

// ======================================================= slice s0052da20: functions
// @ 0x52da20
unsigned Mesh_s2da20::pick_vert(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)verts.size(); i++) {
        float a, b;
        ((MeshImpl_s2da20*)this)->vert_ray(p, d, i, &a, &b);
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

// @ 0x52dae0
void Mesh_s2da20::vert_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                               float* a, float* b)
{
    const mface_s2da20& v = verts.first[index];
    float t = (v.pos.x - p.x) * d.x + (v.pos.y - p.y) * d.y + (v.pos.z - p.z) * d.z;
    rgl::vector4 q;
    if (t < 0.0f)
        q = p;
    else
        q = rgl::vector4(p.x + d.x * t, p.y + d.y * t, p.z + d.z * t);
    *a = (float)sqrt((q.x - v.pos.x) * (q.x - v.pos.x) +
                     (q.y - v.pos.y) * (q.y - v.pos.y) +
                     (q.z - v.pos.z) * (q.z - v.pos.z));
    *b = (float)sqrt((q.x - p.x) * (q.x - p.x) +
                     (q.y - p.y) * (q.y - p.y) +
                     (q.z - p.z) * (q.z - p.z));
}

// @ 0x52dc00
unsigned Mesh_s2da20::pick_edge(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)edges.size(); i++) {
        float a, b;
        ((MeshImpl_s2da20*)this)->edge_ray(p, d, i, &a, &b);
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

// @ 0x52dcc0
void Mesh_s2da20::edge_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                               float* a, float* b)
{
    const medge_s2da20& e = edges.first[index];
    const rgl::vector4& v1 = verts.first[e.v0].pos;
    const rgl::vector4& v2 = verts.first[e.v1].pos;

    float endx = d.x * 1000.0f + p.x;
    float endy = d.y * 1000.0f + p.y;
    float endz = d.z * 1000.0f + p.z;
    float ux = endx - p.x;
    float uy = endy - p.y;
    float uz = endz - p.z;

    float wx = v2.x - v1.x;
    float wy = v2.y - v1.y;
    float wz = v2.z - v1.z;

    float aw = p.x - v1.x;
    float bw = p.y - v1.y;
    float cw = p.z - v1.z;

    float e1 = wx * aw + wy * bw + wz * cw;             // v . w
    float b1 = ux * wx + uy * wy + uz * wz;             // u . v
    float d1 = ux * aw + uy * bw + uz * cw;             // u . w   (negated below)
    float c1 = wx * wx + wy * wy + wz * wz;             // v . v
    float a1 = ux * ux + uy * uy + uz * uz;             // u . u

    float den = a1 * c1 - b1 * b1;
    float s = (b1 * e1 - c1 * d1) / den;
    float t = (e1 + s * b1) / c1;                       // edge parameter from the UNCLAMPED ray parameter

    if (s < 0.0f)
        s = 0.0f;
    else if (s > 1.0f)
        s = 1.0f;
    if (t < 0.0f)
        t = 0.0f;
    else if (t > 1.0f)
        t = 1.0f;

    float rx = p.x + ux * s;
    float ry = p.y + uy * s;
    float rz = p.z + uz * s;
    float qx = v1.x + wx * t;
    float qy = v1.y + wy * t;
    float qz = v1.z + wz * t;

    *a = (float)sqrt((rx - qx) * (rx - qx) + (ry - qy) * (ry - qy) + (rz - qz) * (rz - qz));
    *b = (float)sqrt((qx - p.x) * (qx - p.x) + (qy - p.y) * (qy - p.y) +
                     (qz - p.z) * (qz - p.z));          // distance from the ray origin to the edge point
}

// @ 0x52df40
unsigned Mesh_s2da20::pick_poly(const rgl::vector4& p, const rgl::vector4& d)
{
    float best = 1e8f;
    unsigned besti = (unsigned)-1;
    for (unsigned i = 0; i < (unsigned)polys.size(); i++) {
        float dist;
        ((MeshImpl_s2da20*)this)->poly_ray(p, d, i, &dist);
        if (dist < best) {
            best = dist;
            besti = i;
        }
    }
    return besti;
}

// Ray / convex-polygon intersection.  The polygon's vertices (at most four) are gathered once,
// their mean gives the plane point, the first three give the normal, and the hit point is then
// tested against every edge (a hit outside the polygon reports 1e6, i.e. "no hit").
// @ 0x52dfd0
void Mesh_s2da20::poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                    float* dist)
{
    const mpoly_s2da20& poly = polys.first[index];
    int n = poly.count;
    rgl::vector4 pts[4];
    rgl::vector4 mean(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < n; i++) {
        pts[i] = verts.first[poly.idx[i]].pos;
        mean.x += pts[i].x;
        mean.y += pts[i].y;
        mean.z += pts[i].z;
    }
    float inv = 1.0f / (float)n;
    mean.x *= inv;
    mean.y *= inv;
    mean.z *= inv;

    rgl::vector4 e1, e2, normal;
    rgl::vec3_sub(e1, pts[1], pts[0]);
    rgl::vec3_sub(e2, pts[2], pts[0]);
    cross(normal, e1, e2);
    normal.normalize();

    // from here on the original calls nothing: the differences, dot and cross products are inline
    float mx = mean.x - p.x, my = mean.y - p.y, mz = mean.z - p.z;
    if (mx * d.x + my * d.y + mz * d.z < 0.0f) {   // face points away from the ray
        *dist = 1e6f;
        return;
    }
    float t = (normal.x * mx + normal.y * my + normal.z * mz) / (normal.x * d.x + normal.y * d.y + normal.z * d.z);
    rgl::vector4 hit(p.x + d.x * t, p.y + d.y * t, p.z + d.z * t);

    bool outside = false;
    for (int i = 0; i < n && !outside; i++) {
        const rgl::vector4& a = pts[i];
        const rgl::vector4& b = pts[(i + 1) % n];
        float ex = b.x - a.x, ey = b.y - a.y, ez = b.z - a.z;
        float len = (float)sqrt(ex * ex + ey * ey + ez * ez);
        if (len > 1e-10f) {
            float k = 1.0f / len;
            ex = ex * k;
            ey = ey * k;
            ez = ez * k;
        } else {
            ex = 0.0f;
            ey = 1.0f;
            ez = 0.0f;
        }
        float cx = ey * normal.z - ez * normal.y;
        float cy = ez * normal.x - ex * normal.z;
        float cz = ex * normal.y - ey * normal.x;
        if ((hit.x - a.x) * cx + (hit.y - a.y) * cy + (hit.z - a.z) * cz > 0.0f)
            outside = true;
    }
    if (outside) {
        *dist = 1e6f;
        return;
    }
    float qx = pts[0].x - p.x, qy = pts[0].y - p.y, qz = pts[0].z - p.z;
    *dist = (float)sqrt(qx * qx + qy * qy + qz * qz);
}

// ======================================================= slice s0052dfd0: functions
// @ 0x52dfd0
void Mesh_s2dfd0::poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                    float* dist)
{
    const mpoly_s2dfd0& poly = polys.first[index];
    int n = poly.count;
    rgl::vector4 pts[4];
    rgl::vector4 mean(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < n; i++) {
        pts[i] = verts.first[poly.idx[i]].pos;
        mean.x += pts[i].x;
        mean.y += pts[i].y;
        mean.z += pts[i].z;
    }
    float inv = 1.0f / (float)n;
    mean.x *= inv;
    mean.y *= inv;
    mean.z *= inv;

    rgl::vector4 e1, e2, normal;
    rgl::vec3_sub(e1, pts[1], pts[0]);
    rgl::vec3_sub(e2, pts[2], pts[0]);
    cross(normal, e1, e2);
    normal.normalize();

    // from here on the original calls nothing: the differences, dot and cross products are inline
    float mx = mean.x - p.x, my = mean.y - p.y, mz = mean.z - p.z;
    if (mx * d.x + my * d.y + mz * d.z < 0.0f) {   // face points away from the ray
        *dist = 1e6f;
        return;
    }
    float t = (normal.x * mx + normal.y * my + normal.z * mz) / (normal.x * d.x + normal.y * d.y + normal.z * d.z);
    rgl::vector4 hit(p.x + d.x * t, p.y + d.y * t, p.z + d.z * t);

    bool outside = false;
    for (int i = 0; i < n && !outside; i++) {
        const rgl::vector4& a = pts[i];
        const rgl::vector4& b = pts[(i + 1) % n];
        float ex = b.x - a.x, ey = b.y - a.y, ez = b.z - a.z;
        float len = (float)sqrt(ex * ex + ey * ey + ez * ez);
        if (len > 1e-10f) {
            float k = 1.0f / len;
            ex = ex * k;
            ey = ey * k;
            ez = ez * k;
        } else {
            ex = 0.0f;
            ey = 1.0f;
            ez = 0.0f;
        }
        float cx = ey * normal.z - ez * normal.y;
        float cy = ez * normal.x - ex * normal.z;
        float cz = ex * normal.y - ey * normal.x;
        if ((hit.x - a.x) * cx + (hit.y - a.y) * cy + (hit.z - a.z) * cz > 0.0f)
            outside = true;
    }
    if (outside) {
        *dist = 1e6f;
        return;
    }
    float qx = pts[0].x - p.x, qy = pts[0].y - p.y, qz = pts[0].z - p.z;
    *dist = (float)sqrt(qx * qx + qy * qy + qz * qz);
}

// Select/deselect one vertex, edge or polygon.  Only the vertex case updates `selected`.
// @ 0x52e510
void Mesh_s2dfd0::toggle_flag(int type, int index)
{
    if (type == 0) {
        bool flag = verts.first[index].flag;
        verts.first[index].flag = !flag;
        if (verts.first[index].flag)
            selected = index;
        else
            selected = -1;
    } else if (type == 1) {
        bool flag = edges.first[index].flag;
        edges.first[index].flag = !flag;
    } else {
        bool flag = polys.first[index].flag;
        polys.first[index].flag = !flag;
    }
}

// Scale the selected vertices about their centroid.  The +0x14 selection stamp is recomputed
// first (0x4c54f0); the scale factors are per axis.
// @ 0x52e590
void Mesh_s2dfd0::scale_selected(int dummy, const rgl::vector4* scale)
{
    calc_vertex_mode(dummy);
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    int n = 0;
    for (int i = 0; i < verts.size(); i++) {
        mface_s2dfd0& v = verts.first[i];
        if (v.index) {
            sx += v.pos.x;
            sy += v.pos.y;
            sz += v.pos.z;
            n++;
        }
    }
    float inv = 1.0f / (float)n;
    float mx = sx * inv;
    float my = sy * inv;
    float mz = sz * inv;
    for (int i = 0; i < verts.size(); i++) {
        mface_s2dfd0& v = verts.first[i];
        if (!v.index)
            continue;
        v.pos.x = (v.pos.x - mx) * scale->x + mx;
        v.pos.y = (v.pos.y - my) * scale->y + my;
        v.pos.z = (v.pos.z - mz) * scale->z + mz;
    }
}

// Rotate/scale the selected vertices by a 3x3 matrix about their centroid.
// @ 0x52e6d0
void Mesh_s2dfd0::transform_selected(int dummy, const float* m)
{
    calc_vertex_mode(dummy);
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    int n = 0;
    for (int i = 0; i < verts.size(); i++) {
        mface_s2dfd0& v = verts.first[i];
        if (v.index) {
            sx += v.pos.x;
            sy += v.pos.y;
            sz += v.pos.z;
            n++;
        }
    }
    float inv = 1.0f / (float)n;
    float mx = sx * inv;
    float my = sy * inv;
    float mz = sz * inv;
    for (int i = 0; i < verts.size(); i++) {
        mface_s2dfd0& v = verts.first[i];
        if (!v.index)
            continue;
        float dx = v.pos.x - mx;
        float dy = v.pos.y - my;
        float dz = v.pos.z - mz;
        v.pos = rgl::vector4(dz * m[8] + dy * m[4] + dx * m[0] + mx,
                             dz * m[9] + dy * m[5] + dx * m[1] + my,
                             dz * m[10] + dy * m[6] + dx * m[2] + mz);
    }
}

// Delete every flagged element of the current mode, then rebuild the derived data.  The
// index is held still after a deletion, since the array shrinks under it.
// @ 0x52e870
void Mesh_s2dfd0::remove_selected()
{
    if (mode == 0) {
        for (int i = 0; i < verts.size(); i++) {
            if (verts.first[i].flag) {
                remove_node(i);
                i--;
            }
        }
    }
    if (mode == 1) {
        for (int i = 0; i < edges.size(); i++) {
            if (edges.first[i].flag) {
                remove_edge(i);
                i--;
            }
        }
    } else {
        for (int i = 0; i < polys.size(); i++) {
            if (polys.first[i].flag) {
                remove_face(i);
                i--;
            }
        }
    }
    build_adjacency();
}

// ======================================================= slice s0052e960: functions
// @ 0x52e960
void Mesh_s2e960::make_copy(const rgl::matrix& m)
{
    rgl::matrix basis = m;
    basis.u = rgl::vector4(0.0f, 0.0f, 1.0f);
    basis.s = rgl::vcross_s2e960(basis.f, basis.u);
    basis.s.normalize();
    basis.f = rgl::vcross_s2e960(basis.u, basis.s);

    if (mode == 2) {
        edges2.clear();
        polys2.clear();
        verts2.clear();
        for (int i = 0; i < polys.size(); i++) {
            mpoly_s2e960& p = polys.first[i];
            if (!p.flag)
                continue;
            mpoly_s2e960 t;
            t.idx[0] = -100000;
            t.idx[1] = -100000;
            t.idx[2] = -100000;
            t.idx[3] = -100000;
            t.idx2[0] = -100000;
            t.idx2[1] = -100000;
            t.idx2[2] = -100000;
            t.idx2[3] = -100000;
            t.unk00 = 0;
            t.count = p.count;
            t.unk5c = 0;
            t.flag = 0;
            for (int j = 0; j < p.count; j++) {
                t.idx[j] = vertex_index(p.idx[j], basis);
                t.idx2[j] = edge_index(p.idx2[j], basis);
            }
            polys2.push_back(t);
        }
    }
}

// Return the copy index of source vertex `index`, creating it the first time it is asked for.
// @ 0x52ebc0
int Mesh_s2e960::vertex_index(int index, const rgl::matrix& m)
{
    for (unsigned i = 0; i < (unsigned)verts2.size(); i++) {
        if (verts2.first[i].index == index)
            return (int)i;
    }
    int n = verts2.size();
    mface_s2e960 f;
    f.pos = m.transform_point_to_local(verts.first[index].pos);
    f.flag = 0;
    f.index = index;
    verts2.push_back(f);
    return n;
}

// Same for edges: the copy keeps the two endpoint indices remapped and remembers where it came
// from in +0x20.
// @ 0x52eca0
int Mesh_s2e960::edge_index(int index, const rgl::matrix& m)
{
    for (unsigned i = 0; i < (unsigned)edges2.size(); i++) {
        if (edges2.first[i].index == index)
            return (int)i;
    }
    int n = edges2.size();
    medge_s2e960 e;
    e.v0 = vertex_index(edges.first[index].v0, m);
    e.v1 = vertex_index(edges.first[index].v1, m);
    e.unk08 = -10000;
    e.unk0c = -10000;
    e.unk10 = 0;
    e.flag = 0;
    e.t0 = 1.0f;
    e.t1 = 1.0f;
    e.index = index;
    edges2.push_back(e);
    return n;
}

// Flatten the copy buffers into the real ones: every copied vertex goes back to world space via
// the same basis, its edges/polygons are appended with the index ranges shifted, and the derived
// data is rebuilt.  All selection flags are cleared first.
// @ 0x52edb0
void Mesh_s2e960::apply_copy(const rgl::matrix& m)
{
    // The original keeps the basis matrix, the edge temporary and the polygon temporary in one
    // stack area (their lifetimes are disjoint), so the unwritten tail of each pushed element
    // holds leftover basis words; one shared buffer keeps that.
    char scratch[sizeof(mpoly_s2e960)];
    rgl::matrix& basis = *(rgl::matrix*)scratch;
    basis = m;
    basis.u = rgl::vector4(0.0f, 0.0f, 1.0f);
    basis.s = rgl::vcross_s2e960(basis.f, basis.u);
    basis.s.normalize();
    basis.f = rgl::vcross_s2e960(basis.u, basis.s);

    selected = -1;
    for (unsigned i = 0; i < (unsigned)verts.size(); i++)
        verts.first[i].flag = 0;
    selected = -1;
    for (unsigned i = 0; i < (unsigned)edges.size(); i++)
        edges.first[i].flag = 0;
    selected = -1;
    for (unsigned i = 0; i < (unsigned)polys.size(); i++)
        polys.first[i].flag = 0;

    if (polys2.size() == 0)
        return;

    int voff = verts.size();
    int eoff = edges.size();

    for (unsigned i = 0; i < (unsigned)verts2.size(); i++) {
        mface_s2e960 v;
        v.pos = rgl::to_world_s2e960(basis, verts2.first[i].pos);
        v.flag = 1;
        verts.push_back(v);
    }

    for (unsigned i = 0; i < (unsigned)edges2.size(); i++) {
        medge_s2e960& e = *(medge_s2e960*)scratch;
        e.v0 = edges2.first[i].v0 + voff;
        e.v1 = edges2.first[i].v1 + voff;
        e.unk08 = -10000;
        e.unk0c = -10000;
        e.unk10 = 0;
        e.flag = 1;
        e.t0 = 1.0f;
        e.t1 = 1.0f;
        edges.push_back(e);
    }

    for (unsigned i = 0; i < (unsigned)polys2.size(); i++) {
        mpoly_s2e960& p = polys2.first[i];
        mpoly_s2e960& t = *(mpoly_s2e960*)scratch;
        t.idx[0] = -100000;
        t.idx[1] = -100000;
        t.idx[2] = -100000;
        t.idx[3] = -100000;
        t.idx2[0] = -100000;
        t.idx2[1] = -100000;
        t.idx2[2] = -100000;
        t.idx2[3] = -100000;
        t.unk00 = 0;
        t.count = p.count;
        t.unk5c = 0;
        t.flag = 1;
        for (int j = 0; j < p.count; j++) {
            t.idx[j] = p.idx[j] + voff;
            t.idx2[j] = p.idx2[j] + eoff;
        }
        polys.push_back(t);
    }

    build_adjacency();
}

// ======================================================= slice s0052f250: functions
#undef g_map_s2f250
#define g_map_s2f250 (*(Map_s2f250**)&(*(void**)((char*)&g_mission + 0x0)))
#undef g_game_s2f250
#define g_game_s2f250 (*(Frame__s2f250**)&g_game)
#undef g_objrecs_s2f250
#define g_objrecs_s2f250 (*(ObjRec20_s2f250**)&g_ptypes)
#undef g_editor_view_s2f250
#define g_editor_view_s2f250 (*(EditorView_s2f250**)&(*(void**)((char*)&g_windows + 0x38)))
#undef g_editor_dialog_s2f250
#define g_editor_dialog_s2f250 (*(MapEditorDialog_s2f250*)g_app_obj)
// @ 0x52f250
void MapEditor_s2f250::update()
{
    g_map_s2f250->object->f140 = 0;

    // repeat the two scroll keys every 35000 ticks
    if ((*(int*)((char*)&g_keyboard + 0x34c)) != 0) {
        if (this->f318 < g_ticks[0] - 35000)
            set_list_scroll(1);
    }
    if ((*(int*)((char*)&g_keyboard + 0x32c)) != 0) {
        if (this->f314 < g_ticks[0] - 35000)
            set_list_scroll(-1);
    }
    if ((*(int*)((char*)&g_keyboard + 0x34c)) != 0 && g_key_inc_prev == 0)
        this->f318 = (int)g_ticks[0];
    if ((*(int*)((char*)&g_keyboard + 0x32c)) != 0 && g_key_dec_prev == 0)
        this->f314 = (int)g_ticks[0];
    if ((*(int*)((char*)&g_keyboard + 0x34c)) == 0 && g_key_inc_prev != 0)
        this->f318 = 0;
    if ((*(int*)((char*)&g_keyboard + 0x32c)) == 0 && g_key_dec_prev != 0)
        this->f314 = 0;

    // rebuild the preview when the kind / record / variant changed
    if (g_game_s2f250->mode != g_game_s2f250->prev_mode ||
        g_game_s2f250->id != g_game_s2f250->prev_id ||
        (g_game_s2f250->mode == 4 && this->f31c != this->f320))
    {
        g_map_s2f250->object->clear();

        PlacedObj_s2f250 rec;
        MeshVec_s2f250 list;
        int value = 0;
        if (g_game_s2f250->mode == 4)
            value = this->f31c;

        matrix m;
        identity(&m);
        g_map_s2f250->init_placed(&rec, &g_game_s2f250->mode, &g_game_s2f250->id, &value, &m);
        g_map_s2f250->build_mesh_name(&rec, &list);

        for (unsigned i = 0; i < list.size(); i++) {
            mesh_s2f250* c = list.first[i]->clone();
            g_map_s2f250->object->meshes.push_back(c);
            g_map_s2f250->object->recalc();
        }

        g_game_s2f250->prev_mode = g_game_s2f250->mode;
        g_game_s2f250->prev_id = g_game_s2f250->id;
        this->f320 = this->f31c;

        // preview colour / scale
        float v = 1.0f;
        if (g_game_s2f250->rand_scale != 0)
            v = (float)(rand() % 15817) * 4.741733573609963e-05f + 0.75f;
        g_game_s2f250->color = vector4(v, v, v);
        g_game_s2f250->f1a0ac = 0.0f;

        // frame: s = cross(-dir, up) normalised, f = cross(up, s), u = up
        mat3_s2f250 m3;
        m3.u = vector4(0.0f, 0.0f, 1.0f);
        vector4 d(-this->dir_x, -this->dir_y, -this->dir_z);
        vector4 s = cross_inline_s2f250(d, m3.u);
        m3.s = s;
        normalize_inline_s2f250(m3.s);
        vector4 f = cross_inline_s2f250(m3.u, m3.s);
        m3.f = f;
        *(mat3_s2f250*)&g_map_s2f250->object->frame = m3;

        if (g_game_s2f250->rand_rot != 0) {
            float angle = (float)(rand() % 15817) * 0.00039724254747852683f;
            g_map_s2f250->object->frame.rotate_sf(&angle);
        }
        if (g_game_s2f250->mode == 4) {
            int n = g_objrecs_s2f250[g_game_s2f250->id].variants;
            this->f31c = (n > 1) ? rand() % n : 0;
        }
    }

    // let the placement helper nudge the preview
    this->f2f8 = 0;
    vector4 off(0.0f, 0.0f, g_game_s2f250->f1a0ac);
    mat3_s2f250 m3 = *(mat3_s2f250*)&g_map_s2f250->object->frame;
    vector4 col = g_game_s2f250->color;
    char fA, fB, fC;
    this->apply(&fA, &off, &fB, &m3, &fC, &col, &this->f2f8);

    if (fA)
        g_game_s2f250->f1a0ac = off.z;
    if (fB)
        *(mat3_s2f250*)&g_map_s2f250->object->frame = m3;
    if (fC) {
        vector4 old = g_game_s2f250->color;
        g_game_s2f250->color = col;
        for (unsigned i = 0; i < g_map_s2f250->object->meshes.size(); i++) {
            mesh_s2f250* c = g_map_s2f250->object->meshes.first[i];
            c->set_color(vector4(1.0f / old.x, 1.0f / old.y, 1.0f / old.z));
            c->set_color(g_game_s2f250->color);
        }
    }
    if (fC != 0 || fB != 0 || fA != 0)
        goto set_cursor;

    {
        char ok;
        vector4 pos;
        this->find_position(&ok, &pos);
        if (ok) {
            this->f304 = pos;
            g_map_s2f250->object->frame.o = pos;
            g_map_s2f250->object->frame.o.z += g_game_s2f250->f1a0ac;
            g_map_s2f250->object->f140 = 1;
            if ((*(int*)((char*)&g_keyboard + 0x390)) != 0 && (*(int*)((char*)&g_keyboard + 0x790)) == 0)
                goto place;
        }
        if (g_game_s2f250->commit == 0)
            goto refresh;
place:
        if (g_game_s2f250->mode >= 0 && g_game_s2f250->id >= 0) {
                matrix cm;
                if (g_game_s2f250->commit != 0) {
                    g_game_s2f250->commit = 0;
                    cm = g_game_s2f250->cam;
                } else {
                    cm = g_map_s2f250->object->frame;
                }

                int pv = 0;
                if (g_game_s2f250->mode == 4)
                    pv = this->f31c;
                int spawn = -1;
                int idx = g_map_s2f250->add_placed(&g_game_s2f250->mode, &g_game_s2f250->id, &pv, &cm, &spawn);

                g_game_s2f250->selected = idx;
                g_game_s2f250->sel.resize(0, 0);
                g_game_s2f250->sel.push_back_out(g_game_s2f250->selected);

                PlacedObj_s2f250* p = &g_map_s2f250->records[g_game_s2f250->selected];
                p->v158 = g_game_s2f250->color;
                if (p->f1b4 != 0)
                    ((Prop_s2f250*)p->f1b4)->apply(&p->a18, &p->v158);

                g_game_s2f250->dirty = 1;
                g_editor_dialog_s2f250.update_objects();
                g_editor_view_s2f250->pick_entity();
                g_editor_view_s2f250->f2f9 = 1;
                SetFocus(g_editor_hwnd);

                if (g_game_s2f250->rand_scale != 0) {
                    float v2 = (float)(rand() % 15817) * 4.741733573609963e-05f + 0.75f;
                    g_game_s2f250->color = vector4(v2, v2, v2);
                }
                if (g_game_s2f250->rand_rot != 0) {
                    mat3_s2f250 im;
                    identity3(&im);
                    *(mat3_s2f250*)&g_map_s2f250->object->frame = im;
                    float angle = (float)(rand() % 15817) * 0.00039724254747852683f;
                    g_map_s2f250->object->frame.rotate_sf(&angle);
                }
                if (g_game_s2f250->mode == 4) {
                    int n = g_objrecs_s2f250[g_game_s2f250->id].variants;
                    this->f31c = (n > 1) ? rand() % n : 0;
                }
        }
    }
    goto refresh;

set_cursor:
    SetCursorPos(g_cursor_x, g_cursor_y);
    g_saved_x = g_cursor_x;
    g_saved_y = g_cursor_y;
    g_map_s2f250->object->frame.o = this->f304;
    g_map_s2f250->object->frame.o.z += g_game_s2f250->f1a0ac;
    g_map_s2f250->object->f140 = 1;

refresh:
    g_editor_dialog_s2f250.refresh();
}

// ======================================================= slice s0052fe40: functions
#undef g_items_s2fe40
#define g_items_s2fe40 (*(Item_s2fe40**)&g_items)
#undef g_map_pts_s2fe40
#define g_map_pts_s2fe40 (*(MapEntry_s2fe40**)&g_ptypes)
#undef g_map_points_s2fe40
#define g_map_points_s2fe40 (*(MapPoint_s2fe40**)&g_propkinds)
#undef g_resmgr_s2fe40
#define g_resmgr_s2fe40 (*(resmgr_s2fe40*)&g_resmgr)
// @ 0x52fe40 ---------------------------------------------------------------------------------------------
void ViewCtrl_s2fe40::apply(char* f1, float* pos, char* f2, float* out, char* f3, float* vec, char* f4)
{
    float k = 0.6f;
    if ((*(int*)((char*)&g_keyboard + 0x80)) != 0 || (*(int*)((char*)&g_keyboard + 0x280)) != 0)
        k = 0.6f * 0.1;
    // two statements: the original rounds (y+x)*k to 24 bit before multiplying by 0.005
    float angle = ((*(float*)((char*)&g_keyboard + 0x80c)) + (*(float*)((char*)&g_keyboard + 0x810))) * k;
    angle = angle * g_k005;

    *f1 = 0;
    *f2 = 0;
    *f3 = 0;

    vector2_s2fe40 m[2];

    if (g_a1715c != 0) {
        vector2_s2fe40 a = vector2_s2fe40(1.0f, 0.0f);
        vector2_s2fe40 b = vector2_s2fe40(0.0f, 1.0f);
        m[0] = a;
        m[1] = b;
        ((Mat2_s2fe40*)&m[0])->rotate_2x2(yaw);

        float dx = (*(float*)((char*)&g_keyboard + 0x80c)) * k * g_d001;
        pos[0] += m[0].x * dx;
        pos[1] += m[0].y * dx;
        float dy = (*(float*)((char*)&g_keyboard + 0x810)) * k * g_d001;
        pos[0] -= m[1].x * dy;
        pos[1] -= m[1].y * dy;
        *f1 = 1;
        *f4 = 1;
    }

    if ((*(int*)((char*)&g_keyboard + 0x5c)) != 0) {
        if (g_a171b4 == 0 && g_a173b4 == 0) {
            // (k*0.01) first, then *axis_y, to match the original's 24-bit rounding order
            double dz = k * g_d001;
            pos[2] = pos[2] - dz * (*(float*)((char*)&g_keyboard + 0x810));
        } else
            pos[2] = 0.0f;
        *f1 = 1;
        *f4 = 1;
    }

    if (g_a17120 != 0 && g_a17520 == 0) {
        Matrix12_s2fe40 tmp;
        Matrix12_s2fe40* id = identity3(&tmp);
        *(Matrix12_s2fe40*)out = *id;
        *f2 = 1;
    }

    if (g_a17188 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        ((Matrix12_s2fe40*)out)->rotate_fu(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_a17128 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        ((Matrix12_s2fe40*)out)->rotate_su(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_a17184 != 0 && g_a171b4 == 0 && g_a173b4 == 0 && g_a17194 == 0) {
        ((Matrix12_s2fe40*)out)->rotate_sf(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_a1712c != 0) {
        float c = cos(angle);
        float s = sin(angle);
        rotate_pair_s2fe40(out + 0, c, s);
        rotate_pair_s2fe40(out + 4, c, s);
        rotate_pair_s2fe40(out + 8, c, s);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0 &&
        g_a17188 == 0 && g_a17128 == 0 && g_a17184 == 0) {
        float s = g_one - angle * g_k005b;
        vec[0] = vec[0] * s;
        vec[1] = vec[1] * s;
        vec[2] = vec[2] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_a17188 != 0 && g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float s = g_one - angle * g_k005b;
        vec[0] = vec[0] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_a17128 != 0 && g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float s = g_one - angle * g_k005b;
        vec[1] = vec[1] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_a17184 != 0 && g_a17194 != 0 && g_a171b4 == 0 && g_a173b4 == 0) {
        float s = g_one - angle * g_k005b;
        vec[2] = vec[2] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_a17194 != 0 && g_a17594 == 0 && (g_a171b4 != 0 || g_a173b4 != 0)) {
        vec[0] = 1.0f;
        vec[1] = 1.0f;
        vec[2] = 1.0f;
        vec[3] = m[1].y;
        *f3 = 1;
        *f4 = 1;
    }
}

// @ 0x5302f0 ---------------------------------------------------------------------------------------------
void __stdcall build_mesh_name(MeshOwner_s2fe40* o, std::vector<rgl::mesh*>* out)
{
    out->resize(0, (rgl::mesh*)0);

    tstring name;
    int kind = o->f188;

    if (kind == 2) {
        int idx = find_item(&o->name);
        name = ((ItemKind*)&g_items_s2fe40[idx])->mesh_name_for(0);
    } else if (kind == 6) {
        int idx = find_item(&o->name);
        int var = o->f1a0;
        Item_s2fe40* it = &g_items_s2fe40[idx];
        int t = it->type_flags & 0xff;
        ItemRef_s2fe40 ref;
        ref.item = idx;
        ref.m4 = var;
        rgl::mesh* m;
        if (t == 2 || t == 3 || t == 4 || t == 6 || t == 5 || t == 0x12) {
            if (idx >= 0 && idx < g_item_kind_count)
                m = it->mesh_for(0, 0x30000000, var >> 24);
            else
                m = 0;
            if (m != 0) {
                out->push_back(m);
                goto done;
            }
        }
        {
            name = ((ItemRef*)&ref)->mesh_name();
        }
    } else if (kind == 4) {
        int i;
        for (i = 0; i < g_ptype_count; i++) {
            if (g_map_pts_s2fe40[i].name.equals(o->name) != 0)
                break;
        }
        if (i == g_ptype_count)
            i = 0;
        o->f1a0 = o->f1a0 % g_map_pts_s2fe40[i].count;
        name = *(tstring*)((char*)g_map_pts_s2fe40[i].names + o->f1a0 * 0x48);
    } else if (kind == 7) {
        name = ((ItemKind*)&g_items_s2fe40[o->f18c])->mesh_name_for(o->f1a0);
    } else if (kind == 0) {
        int pi = find_in_table78(&o->name);
        if (pi < 0)
            pi = o->f18c;
        if (pi >= 0 && pi < g_propkind_count) {
            MapPoint_s2fe40* p = &g_map_points_s2fe40[pi];
            if ((*(int*)((char*)&g_cfg + 0x18)) != 0 || (p->flags & 0x4000) == 0)
                *out = p->meshes;
        }
    } else if (kind == 1) {
        if ((*(int*)((char*)&g_cfg + 0x18)) != 0)
            name.assign("entry_arrow");
        else
            name.assign("");
    } else if (kind == 5) {
        if ((*(int*)((char*)&g_cfg + 0x18)) != 0)
            name.assign("_mesh_passage");
        else
            name.assign("");
    }

done:
    if (name.len > 0) {
        std::vector<rgl::mesh*> tmp = ((resmgr*)&g_resmgr_s2fe40)->find_meshes((const rgl::string*)&name);
        *out = tmp;
    }
}
