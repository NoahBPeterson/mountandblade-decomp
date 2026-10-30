#include "../../include/image_addrs.h"
#include "./pick_entity.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../view_rect.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ed8d0
// ------------------------------------------------------------------------------------------------------------
void __fastcall pick_entity(GuiCtx* self)
{
    int set_junk = 0;
    Mission* sc = g_mission;
    *(char*)(sc->f65dc + 0x140) = 0;
    if (g_game->f1a20c) {
        ((gui_manager*)&orig::g_gui_mgr)->set_o568(0, true);   // 0x42cdb0
        vector2 pp(0.5f, 0.23f);
        ((gui_view*)&orig::g_gui_mgr)->set_view((const float*)&pp);   // 0x4c5190
        set_junk = 0;
        return;
    }
    float rect0[4], rect1[4];
    ((camera_ray*)(self->f60))->screen_to_world((rgl::vector4*)rect0, (rgl::vector4*)rect1);   // 0x4bd2f0
    vector4 lo, hi;
    lo.x = rect0[0];
    lo.y = rect0[1];
    lo.z = rect0[2];
    lo.w = rect0[3];
    hi.x = rect1[0];
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
        if (!g_mission->props._Myfirst)
            break;
        unsigned n = (unsigned)(((int)g_mission->props._Mylast - (int)g_mission->props._Myfirst) / 0x1c8);
        if ((unsigned)i >= n)
            break;
        scene_row* row = (scene_row*)((int)g_mission->props._Myfirst + i * 0x1c8);
        shape_base* hit = 0;
        sph.local_center = vector4(0.0f, 0.0f, 0.0f);
        sph.radius = g_3f000000;
        sph.sphere::transform((const pos*)&row->mat[0]);   // 0x408420
        int kind = row->kind;
        if (kind == 2) {
            sph.radius = row->scale.length() * 0.5f;
        } else if (kind == 1) {
            sph.radius = g_3f000000;
        } else if (kind == 4) {
            sph.local_center = vector4(0.0f, 0.0f, 1.0f);
            sph.local_center.w = lo.w;
            sph.radius = row->scale.length();
            int map = row->ref;
            if (map >= 0 && map < g_ptype_count) {
                int* maprow = (int*)((char*)g_ptypes + map * 0x20);
                if (row->point < 0 || maprow[6] <= row->point)
                    row->point = row->point % maprow[6];
                char* pr = (char*)maprow[7] + row->point * 0x48;
                if (*(int*)(pr + 0x20)) {
                    hit = *(shape_base**)(pr + 0x24);
                    smat m2 = *(smat*)&row->mat[0];
                    m2.scale_rows(&row->scale);
                    hit->transform((const pos*)&m2);
                    vector4 mx, mn;
                    hit->get_max(&mx);
                    hit->get_min(&mn);
                    sph.local_center = vector4((mn.x + mx.x) * 0.5f, (mn.y + mx.y) * 0.5f, (mn.z + mx.z) * 0.5f);
                    sph.local_center.w = hi.w;
                    vector4 d = vector4(mx.x - sph.local_center.x, mx.y - sph.local_center.y, mx.z - sph.local_center.z);
                    sph.radius = d.out_length();
                }
            }
        } else if (kind == 5) {
            sph.radius = (row->scale.y + row->scale.x) * 0.5f;
        } else if (kind == 0) {
            char* pr = (char*)g_propkinds + row->ref * 0x78;
            sph.local_center = *(vector4*)(pr + 0x58);
            sph.radius = row->scale.length() * *(float*)(pr + 0x68);
            hit = *(shape_base**)(pr + 0x54);
            if (hit) {
                smat m2 = *(smat*)&row->mat[0];
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
    self->f2cc = best_i;
    if (g_overlay) {
        ((OverlayItf*)g_overlay)->release(1);
    }
    g_cursor_id = -1;
    g_cursor_color = 0xff101010;
    g_overlay = 0;
    for (tstring* s = g_strs; s < g_strs + ((0x5d7d50 - 0x5d7850) / 0x10); s++)
        s->assign("");
    g_list2 = -0.03f;
    g_list0 = 0;
    g_list1 = 0;
    g_sel_ent = 0;
    vector2 pair(0.1f, 0.13f);
    ((gui_view*)&orig::g_gui_mgr)->set_view((const float*)&pair);   // 0x4c5190
    if (self->f2cc == -1) {
        self->f2c8 = -1;
        return;
    }
    int color_a = -1;
    int color_b = 0xffffdf4f;
    if (self->f2cc == g_game->f1a1b4) {
        color_b = 0xffaaff22;
        color_a = 0xffaaaaff;
    }
    scene_row* sel = (scene_row*)((int)sc->props._Myfirst + self->f2cc * 0x1c8);
    tstring s_kind, s_info, s_info2;
    switch (sel->kind) {
    case 4:
        s_kind.assign("FLORA");
        s_info = *(tstring*)sel->caption;
        break;
    case 2:
        s_kind.assign("ITEM");
        s_info = *(tstring*)sel->caption;
        break;
    case 0:
        s_kind.assign("OBJECT");
        s_info = *(tstring*)sel->caption;
        break;
    case 3:
        s_info = *(tstring*)sel->caption;
        break;
    case 1: {
        s_kind.assign("ENTRY");
        char buf[512];
        sprintf(buf, "point no: %d", sel->point);
        s_info.assign(buf);
        break;
    }
    case 5: {
        s_kind.assign("PASSAGE");
        char pbuf[512], pbuf2[512];
        sprintf(pbuf, "Menu item no: %d", sel->f1a4);
        sprintf(pbuf2, "Target Entry Point: %d", sel->point);
        s_info.assign(pbuf);
        s_info2.assign(pbuf2);
        break;
    }
    case 6:
        s_kind.assign("SPAWNED ITEM");
        s_info = *(tstring*)sel->caption;
        break;
    case 7:
        s_kind.assign("SHIELD");
        break;
    }
    if (s_kind.len) ((gui_manager*)&orig::g_gui_mgr)->draw_string(&s_kind, 0.03f, color_a, 1);
    if (s_info.len) ((gui_manager*)&orig::g_gui_mgr)->draw_string(&s_info, 0.027f, color_b, 1);
    if (s_info2.len) ((gui_manager*)&orig::g_gui_mgr)->draw_string(&s_info2, 0.027f, color_b, 1);
    memcpy((void*)(sc->f65dc + 0x58), &sel->mat[0], 64);
    select_entity((rgl::mesh*)sc->f65e0, (skin_owner*)sel, 0x80229933);   // 0x4ea0c0
    if (!self->f2f8 && !g_game->f1a20c)
        *(char*)(sc->f65dc + 0x140) = 1;
    self->f2c8 = -1;
}
