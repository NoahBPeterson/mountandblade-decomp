#include "./facegen_init.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
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
        skin = *(int*)(g_game->chars + troop * 0x508 + 0x30) & 0xf;
    int keys = g_skins_begin[skin].n1 - 1;
    g_msglog.reset();
    g_game->f1a090 = 0;
    overlay* o = new overlay;
    if (o != 0)
        o->f188 = 0;
    ui = o;
    float f2[2];
    f2[0] = 0.0f;
    f2[1] = 0.0f;
    o->v58(f2);
    f2[0] = 1.0f;
    f2[1] = 0.75f;
    o->v64(f2);
    {
        tstring n("face_gen_window");
        mesh* m = g_alloc.get_object(n);
        mesh* c = mesh_clone(m);
        o->v80(0, c);
    }
    {
        widget* w = ui;
        g_msg_tick = 1;
        g_gui.set_window(&w);
    }
    float deform_k = config_float("face_deform_key_h");
    float ab[2];
    {
        elem* d = new elem;
        deform = d;
        elem_fill(d, tstring("face_bo_deform"));
        ab[0] = d->pex;
        ab[1] = (float)keys * deform_k + d->pey;
        d->v64(ab);
    }
    {
        elem* b = new elem;
        buttons = b;
        elem_fill(b, tstring("face_bo_button"));
        push_item(ui, (widget*)b);
    }
    tstring btn_text;
    {
        tstring empty("");
        buttons->va4(&btn_text, &empty);
    }
    for (int i = 0; i < keys; i++) {
        tstring row_name("face_sl_deform_key_base");
        elem* row = new_label2(deform, &row_name);
        deform_rows[i] = row;
        float y = (float)i * deform_k + row->ey;
        ab[0] = row->ex;
        ab[1] = y;
        row->v58(ab);
        tstring kt;
        anim_track(&g_anim, skin, i, &kt);
        widget* lab = new widget;
        key_labels[i] = lab;
        elem_fill(lab, tstring("face_t_deform_key_base"));
        ab[0] = lab->ex;
        ab[1] = y + lab->ey;
        lab->v58(ab);
        lab->va4(&btn_text, &kt);
        deform->vc8(lab);
    }
    {
        tstring t("face_s_deform");
        elem* sc = new_scrollbar(ui, &t);
        tstring t2("face_sc_deform");
        elem* sl = new_slider(deform, &t2, sc);
        slider = sl;
        list_setup(sl);
        push_item(ui, sl);
    }
    cam_btn = 0;
    if (g_no_cam) {
        tstring t("key");
        tstring a("medium_button_down");
        tstring b2("medium_button");
        tstring t1("");
        cam_btn = (elem*)new_medium_button(ui, &t, 0, 0, &a, &b2, &t1, 0.014f, 0.5f, 0.72f);
        cam_btn->vb0();
    }
    float arrow_h = config_float("face_button_h");
    (void)arrow_h;
    float arrow_h2 = config_float("face_button_arrow_h");
    float arrow_w = config_float("arrow_button_height");
    float arrow_r = config_float("arrow_button_width_ratio");
    b_next_face = new_arrow(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_next_face, tstring("face_b_next_face"));
    b_next_beard = new_arrow(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_next_beard, tstring("face_b_next_beard"));
    b_next_hair = new_arrow(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_next_hair, tstring("face_b_next_hair"));
    b_prev_face = new_arrow_l(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_prev_face, tstring("face_b_prev_face"));
    b_prev_beard = new_arrow_l(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_prev_beard, tstring("face_b_prev_beard"));
    b_prev_hair = new_arrow_l(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_prev_hair, tstring("face_b_prev_hair"));
    {
        tstring t("face_t_change_beard");
        tstring v("ui_change_beard");
        tstring* kv = g_strtable.lookup(&v);
        t_change_beard = new_key_label(buttons, kv, &t);
    }
    {
        tstring t("face_t_change_face");
        tstring v("ui_change_skin");
        tstring* kv = g_strtable.lookup(&v);
        t_change_face = new_key_label(buttons, kv, &t);
    }
    {
        tstring t("face_t_change_hair");
        tstring v("ui_change_hair");
        tstring* kv = g_strtable.lookup(&v);
        t_change_hair = new_key_label(buttons, kv, &t);
    }
    {
        tstring n("face_sl_age");
        sl_age = new_label2(buttons, &n);
    }
    {
        tstring t("face_t_age");
        tstring v("ui_age");
        tstring* kv = g_strtable.lookup(&v);
        t_age = new_key_label(buttons, kv, &t);
    }
    {
        tstring n("face_sl_hair_color");
        sl_hair_color = new_label2(buttons, &n);
    }
    {
        tstring t("face_t_hair_color");
        tstring v("ui_change_hair_color");
        tstring* kv = g_strtable.lookup(&v);
        t_hair_color = new_key_label(buttons, kv, &t);
    }
    if (g_skins_begin[skin].first_face == 0) {
        b_next_beard->vb0();
        b_prev_beard->vb0();
        t_change_beard->vb0();
    }
    {
        tstring v("ui_randomize");
        tstring* kv = g_strtable.lookup(&v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = new_medium_button(buttons, kv, 0, 0, &a, &b2, &btn_text, 0.014f, 0.72f, 3.7f);
        b_randomize = btn;
        elem_fill(btn, tstring("face_b_randomize"));
    }
    {
        tstring v("ui_reset");
        tstring* kv = g_strtable.lookup(&v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = new_medium_button(buttons, kv, 0, 0, &a, &b2, &btn_text, 0.014f, 0.72f, 3.7f);
        b_reset = btn;
        elem_fill(btn, tstring("face_b_reset"));
    }
    {
        tstring v("ui_done");
        tstring* kv = g_strtable.lookup(&v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = new_medium_button(buttons, kv, 0, 0, &a, &b2, &btn_text, 0.014f, 0.72f, 3.7f);
        b_done = btn;
        elem_fill(btn, tstring("face_b_done"));
    }

    // ---- face body, world and wind tracks ----------------------------------------------------------
    face_body* b = new face_body;
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
    World* w = new World(lo, hi, &zero, &zero, &zero);
    world = w;
    w->f68 = 0.3f;
    w->f6c = 0.3f;
    w->f70 = 0.3f;
    w->f74 = 1.0f;
    {
        wind* wd = new wind;
        w->tracks.push_back((int)wd);
        wd->v04[0] = 0.1f;
        wd->v04[1] = 0.15f;
        wd->v04[2] = 0.2f;
        wd->v04[3] = 0.0f;
        wd->v14[0] = 0.2f;
        wd->v14[1] = 0.2f;
        wd->v14[2] = 0.2f;
        wd->v14[3] = 0.0f;
        wd->v24[0] = -2.0f;
        wd->v24[1] = -2.0f;
        wd->v24[2] = 12.0f;
        wd->v24[3] = 0.0f;
        wd->type = 2;
        wd->f54 = 20.0f;
        wd->v44[0] = -0.5f;
        wd->v44[1] = 0.0f;
        wd->v44[2] = -1.0f;
        wd->v44[3] = 0.0f;
    }
    {
        wind* wd = new wind;
        w->tracks.push_back((int)wd);
        wd->v04[0] = 1.0f;
        wd->v04[1] = 1.0f;
        wd->v04[2] = 1.0f;
        wd->v04[3] = 0.0f;
        wd->v14[0] = 1.0f;
        wd->v14[1] = 1.0f;
        wd->v14[2] = 1.0f;
        wd->v14[3] = 0.0f;
        wd->v24[0] = -2.0f;
        wd->v24[1] = -2.0f;
        wd->v24[2] = 2.0f;
        wd->v24[3] = 0.0f;
        wd->type = 2;
        wd->f54 = 20.0f;
        wd->v44[0] = 1.0f;
        wd->v44[1] = 1.0f;
        wd->v44[2] = -1.0f;
        wd->v44[3] = 0.0f;
    }
    ent = w->create_entity(9);
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
        char* rec = g_game->chars + troop * 0x508;
        int sk = *(int*)(rec + 0x30) & 0xf;
        int flags = 0;
        g_anim.cur = gen_face_seed(&sk, &flags, &g_char_count);
        g_anim.set_anim_a(rec + 0x3c0);
        f354 = *(int*)((char*)&g_anim + 0x04);
        f350 = *(int*)((char*)&g_anim + 0x00);
        f35c = *(int*)((char*)&g_anim + 0x14);
        f358 = *(int*)((char*)&g_anim + 0x0c);
        f360 = g_dummy0;
        f364 = g_dummy1;
        f354++;
        f350++;
        for (int i = 0; i < keys; i++)
            deform_rows[i]->v34((&g_dummy_mat)[i]);
        sl_age->v34(g_dummy0);
        sl_hair_color->v34(g_dummy1);
    }
    refresh();
}
