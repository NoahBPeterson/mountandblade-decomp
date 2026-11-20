#include "../../include/image_addrs.h"
#include "./pick_entity.h"
#include "../../rgl/scene/scene_entity.h"      // rgl::entity: the mission's pick marker (Mission +0x65dc)
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../view_rect.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../placed_obj.h"      // Mission::props is a vector<PlacedObj>: complete here so the harness types the scene vector
#include "../shared_vars.h"
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


// @ 0x4ed8d0
// ------------------------------------------------------------------------------------------------------------
void MissionBlocker::pick_entity()
{
    int set_junk = 0;
    Mission* sc = g_mission;
    sc->sel_marker->visible = 0;
    if (g_game->f1a20c) {
        g_gui.set_overlay(0, true);   // 0x42cdb0
        vector2 pp(0.5f, 0.23f);
        g_gui.set_view((const float*)&pp);   // 0x4c5190
        set_junk = 0;
        return;
    }
    float rect0[4], rect1[4];
    name_widget->screen_to_world((rgl::vector4*)rect0, (rgl::vector4*)rect1);   // 0x4bd2f0
    vector4 lo, hi;
    lo.x = rect0[0];
    lo.y = rect0[1];
    hi.x = rect1[0];
    lo.z = rect0[2];
    lo.w = rect0[3];
    hi.y = rect1[1];
    hi.z = rect1[2];
    hi.w = rect1[3];

    float capbuf[0x64 / 4];
    capsule* cap = new (capbuf, raw_place()) capsule(g_3ac49ba6, &lo, &hi);   // 0x433310
    float idpos[16];
    cap->capsule::transform(identity_pos((pos*)idpos));   // 0x408770
    sphere sph;                                            // the probe sphere (vtable 0x54a4f0); its centre and radius are set per row

    float best = g_4e6e6b28;
    int best_i = -1;
    int i = 0;
    for (;;) {
        if ((unsigned)i >= g_mission->props.size())
            break;
        PlacedObj* row = &g_mission->props[i];
        shape_base* hit = 0;
        sph.local_center = vector4(0.0f, 0.0f, 0.0f);
        sph.radius = g_3f000000;
        sph.sphere::transform(&row->frame);   // 0x408420
        int kind = row->kind;
        if (kind == 2) {
            sph.radius = row->scale.length() * 0.5f;
        } else if (kind == 1) {
            sph.radius = g_3f000000;
        } else if (kind == 4) {
            sph.local_center = vector4(0.0f, 0.0f, 1.0f);
            sph.local_center.w = lo.w;
            sph.radius = row->scale.length();
            int map = row->id;
            if (map >= 0 && map < g_ptype_count) {
                ptype& pt = g_ptypes[map];
                if (row->value < 0 || pt.nframes <= row->value)
                    row->value = row->value % pt.nframes;
                AnimBox& box = pt.boxes[row->value];
                if (box.res) {
                    hit = box.body;
                    smat m2 = *(smat*)&row->frame;
                    m2.scale_rows(&row->scale);
                    hit->transform((const pos*)&m2);
                    vector4 mx, mn;
                    hit->get_max(&mx);
                    hit->get_min(&mn);
                    sph.local_center = vector4((mn.x + mx.x) * 0.5f, (mx.y + mn.y) * 0.5f, (mn.z + mx.z) * 0.5f);
                    sph.local_center.w = hi.w;
                    vector4 d = vector4(mx.x - sph.local_center.x, mx.y - sph.local_center.y, mx.z - sph.local_center.z);
                    sph.radius = d.out_length();
                }
            }
        } else if (kind == 5) {
            sph.radius = (row->scale.y + row->scale.x) * 0.5f;
        } else if (kind == 0) {
            MissionObj* pr = &g_propkinds[row->id];
            sph.local_center = pr->center;
            sph.radius = row->scale.length() * pr->radius;
            hit = pr->body;
            if (hit) {
                smat m2 = *(smat*)&row->frame;
                m2.scale_rows(&row->scale);
                hit->transform((const pos*)&m2);
            }
        } else {
            i++;
            continue;
        }
        contact c;
        if (shape_contact_count(&sph, cap, 1, &c) && (hit == 0 || shape_contact_count(hit, cap, 1, &c))) {
            vector4 d = vector4(lo.x - c.point.x, lo.y - c.point.y, lo.z - c.point.z);
            float len = d.normalize();
            if (len < best) {
                best = len;
                best_i = i;
            }
        }
        i++;
    }
    sel_target = best_i;
    if (g_gui.overlay) {
        ((OverlayItf*)g_gui.overlay)->release(1);
    }
    g_gui.panel_color = -1;
    g_gui.panel_bg = 0xff101010;
    g_gui.overlay = 0;
    for (tstring* s = g_gui.lines; s < g_gui.lines + 0x50; s++)
        s->assign("");
    g_gui.line_x = -0.03f;
    g_gui.nlines = 0;
    *(int*)&g_gui.widest = 0;          // an integer zero store (the zero register), as the original; a float 0 is an immediate
    g_gui.sel_ent = 0;
    vector2 pair(0.1f, 0.13f);
    g_gui.set_view((const float*)&pair);   // 0x4c5190
    if (sel_target == -1) {
        sel_mode = -1;
        return;
    }
    int color_a = -1;
    int color_b = 0xffffdf4f;
    if (sel_target == g_game->editor.sel_index) {
        color_b = 0xffaaff22;
        color_a = 0xffaaaaff;
    }
    tstring s_kind, s_info, s_info2;
    PlacedObj* sel = &sc->props[sel_target];
    switch (sel->kind) {
    case 4:
        s_kind.assign("FLORA");
        s_info = sel->name;
        break;
    case 2:
        s_kind.assign("ITEM");
        s_info = sel->name;
        break;
    case 0:
        s_kind.assign("OBJECT");
        s_info = sel->name;
        break;
    case 3:
        s_info = sel->name;
        break;
    case 1: {
        s_kind.assign("ENTRY");
        char buf[512];
        sprintf(buf, "point no: %d", sel->value);
        s_info.assign(buf);
        break;
    }
    case 5: {
        s_kind.assign("PASSAGE");
        char pbuf[512], pbuf2[512];
        sprintf(pbuf, "Menu item no: %d", sel->m1a4);
        sprintf(pbuf2, "Target Entry Point: %d", sel->value);
        s_info.assign(pbuf);
        s_info2.assign(pbuf2);
        break;
    }
    case 6:
        s_kind.assign("SPAWNED ITEM");
        s_info = sel->name;
        break;
    case 7:
        s_kind.assign("SHIELD");
        break;
    }
    if (s_kind.len) g_gui.draw_string(&s_kind, 0.03f, color_a, 1);
    if (s_info.len) g_gui.draw_string(&s_info, 0.027f, color_b, 1);
    if (s_info2.len) g_gui.draw_string(&s_info2, 0.027f, color_b, 1);
    memcpy(&sc->sel_marker->local, &sel->frame, 64);
    select_entity(sc->sel_mesh, sel, 0x80229933);   // 0x4ea0c0
    if (!placement_adjusted && !g_game->f1a20c)
        sc->sel_marker->visible = 1;
    sel_mode = -1;
}
