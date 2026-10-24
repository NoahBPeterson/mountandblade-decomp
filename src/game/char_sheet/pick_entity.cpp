#include "./pick_entity.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4ed8d0
// ------------------------------------------------------------------------------------------------------------
void __fastcall pick_entity(Ctx* self)
{
    int set_junk = 0;
    scene* sc = (scene*)g_tasks;
    *(char*)(sc->f65dc + 0x140) = 0;
    if (g_menu_mode) {
        self->reset_all(1);
        vector2 pp = { 0.5f, 0.23f };
        self->set_pair(&pp);
        set_junk = 0;
        return;
    }
    int rect0[4], rect1[4];
    ((ViewRect*)(self->f60))->cell_rect(rect0, rect1);
    vector4 lo, hi;
    lo.x = *(float*)&rect0[0];
    lo.y = *(float*)&rect0[1];
    lo.z = *(float*)&rect0[2];
    lo.w = *(float*)&rect0[3];
    hi.x = *(float*)&rect1[0];
    hi.y = *(float*)&rect1[1];
    hi.z = *(float*)&rect1[2];
    hi.w = *(float*)&rect1[3];

    float capbuf[0x64 / 4];
    capsule_ctor_inline((capsule*)capbuf, g_3ac49ba6, &lo, &hi);
    float idpos[16];
    identity_pos(idpos);
    smat probe = *(smat*)idpos;
    float sphbuf[0x44 / 4];
    sphere_shape* sphere = (sphere_shape*)sphbuf;
    sphere->v20 = lo;
    sphere->radius = g_4e6e6b28;
    sphere->v30 = lo;

    float best = g_4e6e6b28;
    int best_i = -1;
    int i = 0;
    int n = (sc->f6690 - sc->f668c) / 0x1c8;
    while (i < n) {
        scene_row* row = (scene_row*)(sc->f668c + i * 0x1c8);
        vector4 o = row->origin;
        shape_base* hit = 0;
        vector4 centre = lo;
        centre.w = 0.0f;
        float rad = g_3f000000;
        make_contact_box(&row->mat[0]);
        if (row->kind == 2) {
            rad = row->mat[3].length() * 0.5f;
        } else if (row->kind == 1) {
            rad = g_3f000000;
        } else if (row->kind == 4) {
            centre.x = 0.0f; centre.y = 0.0f; centre.z = 1.0f; centre.w = o.w;
            rad = row->mat[3].length();
            int map = row->ref;
            if (map >= 0 && map < g_map_count) {
                int* maprow = (int*)((char*)g_map_rows + map * 0x20);
                if (row->point < 0 || maprow[6] <= row->point)
                    row->point = row->point % maprow[6];
                char* pr = (char*)maprow[7] + row->point * 0x48;
                if (*(int*)(pr + 0x20)) {
                    hit = *(shape_base**)(pr + 0x24);
                    smat m2 = probe;
                    m2.scale_rows(&row->mat[0]);
                    hit->s2(&m2);
                }
            }
        } else if (row->kind == 5) {
            rad = (row->mat[1].x + row->mat[0].x) * 0.5f;
        } else if (row->kind == 0) {
            char* pr = (char*)g_point_rows + row->ref * 0x78;
            centre = *(vector4*)(pr + 0x58);
            rad = row->mat[3].length() * *(float*)(pr + 0x68);
            hit = *(shape_base**)(pr + 0x54);
            if (hit) {
                smat m2 = probe;
                m2.scale_rows(&row->mat[0]);
                hit->s2(&m2);
            }
        }
        if (shape_contact_count((shape_base*)capbuf, sphere, 1, &row->mat[1])) {
            if (hit == 0 || shape_contact_count(hit, sphere, 1, &row->mat[1])) {
                vector4 d;
                d.x = lo.x - centre.x;
                d.y = lo.y - centre.y;
                d.z = lo.z - centre.z;
                float len = d.length();
                if (len < rad * g_3f8ccccd + best) {
                    best = len;
                    best_i = i;
                }
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
    vector2 pair = { 0.1f, 0.13f };
    self->set_pair(&pair);
    if (self->f2cc == -1) {
        self->f2c8 = -1;
        return;
    }
    int color = 0xffdf4f;
    if (self->f2cc == g_sel_entity) {
        color = 0xffaaff22;
    }
    scene_row* sel = (scene_row*)(sc->f668c + self->f2cc * 0x1c8);
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
        s_info = *(tstring*)sel->caption;
        break;
    }
    if (s_kind.len) self->draw_string(&s_kind, 0.03f, color, 1);
    if (s_info.len) self->draw_string(&s_info, 0.0269f, 0xffdf4f, 1);
    if (s_info2.len) self->draw_string(&s_info2, 0.0269f, 0xffdf4f, 1);
    memcpy((void*)(sc->f65dc + 0x58), &sel->origin, 16);
    self->select_entity(sel, sc->f65e0, 0x80229933);
    if (!self->f2f8 && !g_menu_mode)
        *(char*)(sc->f65dc + 0x140) = 1;
    self->f2c8 = -1;
}
