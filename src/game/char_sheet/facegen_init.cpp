#include "../../include/image_addrs.h"
#include "./facegen_init.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/skin_set.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../engvec.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)

// @ 0x4f49d0
void FaceGen::init()
{
    f360 = 0.5f;
    f364 = 0.5f;
    f354 = 0;
    f350 = 0;
    f358 = 0;
    f35c = 0;
    skin = 0;
    if (troop >= 0)
        skin = *(int*)(g_game->chars_c + troop * 0x508 + 0x30) & 0xf;
    int keys = g_skins_begin[skin].n_elems - 1;
    ((gui_manager*)(void*)&g_msglog)->reset();   // 0x42d1b0 (the 0x5d7080 object is the gui manager)
    g_game->f1a090 = 0;
    overlay* o = (overlay*)new_container(0x190, vtbl_592848);
    if (o != 0)
        o->f188 = 0;
    ui = o;
    float f2[2];
    f2[0] = 0.0f;
    f2[1] = 0.0f;
    o->set_pos(f2);
    f2[0] = 1.0f;
    f2[1] = 0.75f;
    o->set_size(f2);
    {
        tstring n("face_gen_window");
        rgl::mesh* m = g_alloc.get_object((const rgl::string*)&n);
        mesh_x* c = ((mesh_x*)m)->clone();
        o->add_item2(0, (rgl::mesh*)c);
    }
    {
        widget* w = ui;
        g_msg_tick = 1;
        ((NodeVec*)(void*)&g_gui)->push_back((NodeA* const&)w);   // 0x49b120
    }
    float deform_k = config_float("face_deform_key_h");
    float ab[2];
    {
        elem* d = (elem*)new_container(0x188, vtbl_597050);
        deform = d;
        label_refresh((MissionUi*)d, tstring("face_bo_deform"));
        ab[0] = d->x1;
        ab[1] = (float)keys * deform_k + d->y1;
        d->set_size(ab);
    }
    {
        elem* b = (elem*)new_container(0x188, vtbl_597050);
        buttons = b;
        label_refresh((MissionUi*)b, tstring("face_bo_button"));
        push_item(ui, (widget*)b);
    }
    tstring btn_text;
    {
        tstring empty("");
        buttons->set_text(&btn_text, &empty);
    }
    for (int i = 0; i < keys; i++) {
        tstring row_name("face_sl_deform_key_base");
        elem* row = (elem*)make_slider(deform, &row_name);
        deform_rows[i] = row;
        float y = (float)i * deform_k + row->ey;
        ab[0] = row->ex;
        ab[1] = y;
        row->set_pos(ab);
        tstring kt;
        ((TextPart*)&g_anim)->get_text(skin, i, &kt);   // 0x4f5d90
        widget* lab = (widget*)new_widget_base(0x178);
        key_labels[i] = lab;
        label_refresh((MissionUi*)lab, tstring("face_t_deform_key_base"));
        ab[0] = lab->ex;
        ab[1] = y + lab->ey;
        lab->set_pos(ab);
        lab->set_text(&btn_text, &kt);
        deform->add_child(lab);
    }
    {
        tstring t("face_s_deform");
        elem* sc = (elem*)make_scrollbar(ui, &t);
        tstring t2("face_sc_deform");
        elem* sl = (elem*)make_editor_list(deform, &t2, (int)sc);
        slider = sl;
        ((MissionPanel*)sl)->draw();   // 0x488d50
        push_item(ui, sl);
    }
    cam_btn = 0;
    if (g_no_cam) {
        tstring t("key");
        cam_btn = (elem*)new_button((ContainerBase*)ui, t, 0.0f, 0.72f, 0.5f, 0.028f);   // 0x4db810
        cam_btn->sb0();
    }
    float arrow_h = config_float("face_button_h");
    float arrow_h2 = config_float("face_button_arrow_h");
    float arrow_w = config_float("arrow_button_height");
    float arrow_r = config_float("arrow_button_width_ratio");
    b_next_face = (widget*)make_right_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh((MissionUi*)b_next_face, tstring("face_b_next_face"));
    b_next_beard = (widget*)make_right_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh((MissionUi*)b_next_beard, tstring("face_b_next_beard"));
    b_next_hair = (widget*)make_right_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh((MissionUi*)b_next_hair, tstring("face_b_next_hair"));
    b_prev_face = (widget*)make_left_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh((MissionUi*)b_prev_face, tstring("face_b_prev_face"));
    b_prev_beard = (widget*)make_left_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh((MissionUi*)b_prev_beard, tstring("face_b_prev_beard"));
    b_prev_hair = (widget*)make_left_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh((MissionUi*)b_prev_hair, tstring("face_b_prev_hair"));
    {
        tstring t("face_t_change_beard");
        tstring v("ui_change_beard");
        tstring* kv = g_strtable.lookup(v);
        t_change_beard = (widget*)new_key_label(buttons, kv, &t);
    }
    {
        tstring t("face_t_change_face");
        tstring v("ui_change_skin");
        tstring* kv = g_strtable.lookup(v);
        t_change_face = (widget*)new_key_label(buttons, kv, &t);
    }
    {
        tstring t("face_t_change_hair");
        tstring v("ui_change_hair");
        tstring* kv = g_strtable.lookup(v);
        t_change_hair = (widget*)new_key_label(buttons, kv, &t);
    }
    {
        tstring n("face_sl_age");
        sl_age = (elem*)make_slider(buttons, &n);
    }
    {
        tstring t("face_t_age");
        tstring v("ui_age");
        tstring* kv = g_strtable.lookup(v);
        t_age = (widget*)new_key_label(buttons, kv, &t);
    }
    {
        tstring n("face_sl_hair_color");
        sl_hair_color = (elem*)make_slider(buttons, &n);
    }
    {
        tstring t("face_t_hair_color");
        tstring v("ui_change_hair_color");
        tstring* kv = g_strtable.lookup(v);
        t_hair_color = (widget*)new_key_label(buttons, kv, &t);
    }
    if (g_skins_begin[skin].first_face == 0) {
        b_next_beard->sb0();
        b_prev_beard->sb0();
        t_change_beard->sb0();
    }
    {
        tstring v("ui_randomize");
        tstring* kv = g_strtable.lookup(v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = (widget*)new_sized_button((MissionUi*)buttons, *kv, 0.0f, 0.0f, arrow_h, b2, a, 3.7f, 0.072f, 0.085f);   // 0x4dcc50
        b_randomize = btn;
        label_refresh((MissionUi*)btn, tstring("face_b_randomize"));
    }
    {
        tstring v("ui_reset");
        tstring* kv = g_strtable.lookup(v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = (widget*)new_sized_button((MissionUi*)buttons, *kv, 0.0f, 0.0f, arrow_h, b2, a, 3.7f, 0.072f, 0.085f);   // 0x4dcc50
        b_reset = btn;
        label_refresh((MissionUi*)btn, tstring("face_b_reset"));
    }
    {
        tstring v("ui_done");
        tstring* kv = g_strtable.lookup(v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = (widget*)new_sized_button((MissionUi*)buttons, *kv, 0.0f, 0.0f, arrow_h, b2, a, 3.7f, 0.072f, 0.085f);   // 0x4dcc50
        b_done = btn;
        label_refresh((MissionUi*)btn, tstring("face_b_done"));
    }

    // ---- face body, world and wind tracks ----------------------------------------------------------
    face_body* b = (face_body*)::operator new(0x358);
    if (b)
        ctor_4ed630((MapWidget*)b);   // 0x4ed630
    body = b;
    float lo[4];
    lo[0] = -80.0f;
    lo[1] = -80.0f;
    lo[2] = 0.0f;
    lo[3] = 0.0f;
    float hi[4];
    hi[0] = 80.0f;
    hi[1] = 80.0f;
    hi[2] = 0.0f;
    hi[3] = 0.0f;
    int zero = 0;
    World* w = (World*)::operator new(sizeof(World));
    if (w)
        new (w, raw_place()) CellGrid(lo, hi, &zero, &zero, &zero);   // 0x478ad0 (World::World names it)
    world = w;
    w->color.x = 0.3f;
    w->color.y = 0.3f;
    w->color.z = 0.3f;
    w->color.w = 1.0f;
    {
        wind* wd = (wind*)::operator new(sizeof(wind));
        if (wd)
            wd->init();   // 0x461480
        push_track(w->tracks, (int)wd);
        wd->v04.x = 0.1f;
        wd->v04.y = 0.15f;
        wd->v04.z = 0.2f;
        wd->v04.w = 0.0f;
        wd->v14.x = 0.2f;
        wd->v14.y = 0.2f;
        wd->v14.z = 0.2f;
        wd->v14.w = 0.0f;
        wd->v24.x = -2.0f;
        wd->v24.y = -2.0f;
        wd->v24.z = 12.0f;
        wd->v24.w = 0.0f;
        wd->type = 2;
        wd->f54 = 20.0f;
        wd->v44.x = -0.5f;
        wd->v44.y = 0.0f;
        wd->v44.z = -1.0f;
        wd->v44.w = 0.0f;
    }
    {
        wind* wd = (wind*)::operator new(sizeof(wind));
        if (wd)
            wd->init();   // 0x461480
        push_track(w->tracks, (int)wd);
        wd->v04.x = 1.0f;
        wd->v04.y = 1.0f;
        wd->v04.z = 1.0f;
        wd->v04.w = 0.0f;
        wd->v14.x = 1.0f;
        wd->v14.y = 1.0f;
        wd->v14.z = 1.0f;
        wd->v14.w = 0.0f;
        wd->v24.x = -2.0f;
        wd->v24.y = -2.0f;
        wd->v24.z = 2.0f;
        wd->v24.w = 0.0f;
        wd->type = 2;
        wd->f54 = 20.0f;
        wd->v44.x = 1.0f;
        wd->v44.y = 1.0f;
        wd->v44.z = -1.0f;
        wd->v44.w = 0.0f;
    }
    ent = (Entity*)((rgl::world*)w)->create_entity(9);   // 0x428440
    b->world = w;

    float p2[2];
    p2[0] = 0.0513f;
    p2[1] = 0.2669f;
    b->set_pos(p2);
    float s2[2];
    s2[0] = 0.4585f;
    s2[1] = 0.44f;
    b->set_scale(s2);
    b->flags |= 0x300;
    b->clear_color = 0xff201815;

    // ---- the camera frustum (16:9 fit around the body) ---------------------------------------------
    float ratio = b->size_y / b->size_x * g_fov_k;
    float tanv = (float)tan(g_tan_ang);
    fr.perspective = 1;
    fr.nearz = 0.01f;
    fr.farz = 20.0f;
    fr.left = -(0.01f * tanv);
    fr.right = 0.01f * tanv;
    fr.bottom = -(ratio * (0.01f * tanv));
    fr.top = ratio * (0.01f * tanv);
    fr.build_planes();

    push_item(ui, (widget*)b);

    fr.m.o.x = 0.0f;
    fr.m.o.y = 0.0f;
    fr.m.o.z = 10.0f;
    fr.m.o.w = 0.0f;
    v154[0] = 0.0f;
    v154[1] = 0.0f;
    v154[2] = 0.0f;
    v154[3] = 0.0f;
    f164 = -0.3f;
    f168 = -0.05f;
    f16c = 0.7f;
    f170 = 0;
    b->view = fr;

    // ---- hook the face up to the troop's anim table ------------------------------------------------
    if (troop >= 0) {
        char* rec = g_game->chars_c + troop * 0x508;
        int sk = *(int*)(rec + 0x30) & 0xf;
        int flags = 0;
        g_anim.cur = clamp_index(&sk, &flags, &g_char_count);   // 0x432510
        ((FaceGenome*)&g_anim)->unpack((const unsigned __int64*)(rec + 0x3c0));   // 0x473800
        f354 = *(int*)((char*)&g_anim + 0x04);
        f350 = *(int*)((char*)&g_anim + 0x00);
        f35c = *(int*)((char*)&g_anim + 0x14);
        f358 = *(int*)((char*)&g_anim + 0x0c);
        f360 = g_dummy0;
        f364 = g_dummy1;
        f354++;
        f350++;
        for (int i = 0; i < keys; i++)
            deform_rows[i]->s34((&g_dummy_mat)[i]);
        sl_age->s34(g_dummy0);
        sl_hair_color->s34(g_dummy1);
    }
    refresh();

}
