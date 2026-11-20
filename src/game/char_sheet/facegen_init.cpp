#include "../../include/image_addrs.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "./facegen_init.h"
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../str_table.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/skin.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../engvec.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui_globals.h"      // UI / core globals
#include "../terrain_b/cell_grid.h"      // CellGrid (ctor 0x478ad0, dtor 0x475d50, spawn_wind 0x478f30): the one definition

// the medium button of the panel (0x4dcc50 with the two skin names): the label is the caller's temporary, the skin names this helper's
static inline MissionUi* medium_button(MissionUi* parent, const tstring& label, float x, float y, float h)
{
    return (MissionUi*)new_sized_button(parent, label, x, y, h, "medium_button", "medium_button_down", 3.7f, 0.072f, 0.085f);
}
// the scene bounds, passed by address as temporaries (0x478ad0)
struct face_pair { float x, y; face_pair(float a, float b) : x(a), y(b) {} };
// the widget's child list: a real inlined push_back; the slow path is 0x4927d0
static __forceinline void add_kid(ContainerBase* c, MissionUi* w)
{
    c->kids.push_back(w);
}
// the light list's out-of-line fill (0x40f8a0: ICF-folded with mesh_obj.cpp's _Uninit_fill_n<int*>; cdecl, no tag argument)
namespace std { template<> void _Uninitialized_fill_n(Light** first, unsigned n, Light* const& v, allocator<Light*>& al); }   // 0x40f8a0
// a new light in the scene's list (0x461480; push_back expanded in place: fill 0x40f8a0, the slow path _Insert_n 0x410610)
static __forceinline Light* add_light(rgl::world* w)
{
    Light* l = new Light;
    w->lights.push_back(l);
    return l;
}
// rgl::frustum::init (0x4f25a0), as this caller expands it
struct face_frustum : rgl::frustum {
    inline void init_inl(float fov, float aspect, float znear, float zfar)
    {
        double t = tan(fov * 0.5f);
        perspective = 1;
        nearz = znear;
        farz = zfar;
        float tf = (float)t;
        left = -(znear * tf);
        right = (float)t * znear;
        bottom = -(aspect * (znear * tf));
        top = aspect * ((float)t * znear);
        build_planes();
    }
};

// the arrow factories take the width ratio as the float it is (the shared declaration says int)

// @ 0x4f49d0
void FaceGen::init_screen()
{
    f354 = 0;
    f350 = 0;
    f358 = 0;
    f35c = 0;
    f360 = 0.5f;
    f364 = 0.5f;
    skin = 0;
    int id = w14;
    if (id >= 0)
        skin = (int)g_game->troops[id].flags30 & 0xf;
    int keys = g_anim.skin_vec[skin].n_face_keys - 1;
    g_gui.reset();   // 0x42d1b0 (the 0x5d7080 object is the gui manager)
    g_game->editor.active = 0;
    ui = new NodeA(0);
    ui->move_a(V2(0.0f, 0.0f));
    ui->set_xy(V2(1.0f, 0.75f));
    {
        rgl::mesh* m;
        {
            tstring n("face_gen_window");
            m = g_resmgr.get_object(&n)->clone();
        }
        ui->set_track(0, m);
    }
    {
        NodeA* w = ui;
        g_gui.dirty = 1;
        g_gui.objs.push_back((NodeA* const&)w);   // 0x49b120
    }
    float deform_k = config_float_called("face_deform_key_h");
    deform = new text_box;
    label_refresh(deform, "face_bo_deform");
    deform->set_xy(V2(deform->rect_max.x, (float)keys * deform_k + deform->rect_max.y));
    buttons = new text_box;
    label_refresh(buttons, "face_bo_button");
    add_kid(ui, buttons);
    {
        tstring empty("");
        deform->set_text(empty);   // 0x4f4cab: [ebp+0x180] (the deform panel), not the buttons panel
    }
    int i;
    for (i = 0; i < keys; i++) {
        tstring kt;
        { tstring n("face_sl_deform_key_base"); deform_rows[i] = make_slider(deform, &n); }
        deform_rows[i]->move_a(V2(deform_rows[i]->rel_position.x, (float)i * deform_k + deform_rows[i]->rel_position.y));
        g_anim.get_text(skin, i, &kt);   // 0x4f5d90
        key_labels[i] = new MissionUi;
        label_refresh(key_labels[i], "face_t_deform_key_base");
        key_labels[i]->move_a(V2(key_labels[i]->rel_position.x, (float)i * deform_k + key_labels[i]->rel_position.y));
        key_labels[i]->set_text(kt);
        deform->add_child(key_labels[i]);
    }
    {
        UiD* sc;
        { tstring n("face_s_deform"); sc = make_scrollbar(ui, &n); }
        { tstring n("face_sc_deform"); slider = make_editor_list(deform, &n, (UiD*)sc); }
    }
    slider->draw();   // 0x488d50
    add_kid(ui, slider);
    cam_btn = 0;
    if (g_cfg.enable_edit_mode) {
        cam_btn = new_button(ui, "key", 0.0f, 0.72f, 0.5f, 0.028f);   // 0x4db810
        cam_btn->hide();
    }
    float arrow_h = config_float("face_button_h");
    float arrow_h2 = config_float("face_button_arrow_h");
    float arrow_w = config_float("arrow_button_height");
    float arrow_r = config_float("arrow_button_width_ratio");
    b_next_face = (MissionUi*)make_right_arrow(buttons, 0.0f, 0.0f, arrow_h2, arrow_r, arrow_w);
    label_refresh(b_next_face, "face_b_next_face");
    b_next_beard = (MissionUi*)make_right_arrow(buttons, 0.0f, 0.0f, arrow_h2, arrow_r, arrow_w);
    label_refresh(b_next_beard, "face_b_next_beard");
    b_next_hair = (MissionUi*)make_right_arrow(buttons, 0.0f, 0.0f, arrow_h2, arrow_r, arrow_w);
    label_refresh(b_next_hair, "face_b_next_hair");
    b_prev_face = (MissionUi*)make_left_arrow(buttons, 0.0f, 0.0f, arrow_h2, arrow_r, arrow_w);
    label_refresh(b_prev_face, "face_b_prev_face");
    b_prev_beard = (MissionUi*)make_left_arrow(buttons, 0.0f, 0.0f, arrow_h2, arrow_r, arrow_w);
    label_refresh(b_prev_beard, "face_b_prev_beard");
    b_prev_hair = (MissionUi*)make_left_arrow(buttons, 0.0f, 0.0f, arrow_h2, arrow_r, arrow_w);
    label_refresh(b_prev_hair, "face_b_prev_hair");
    { tstring t("face_t_change_beard"); tstring v("ui_change_beard"); t_change_beard = (MissionUi*)new_key_label(buttons, g_strtable.lookup(v), &t); }
    { tstring t("face_t_change_face"); tstring v("ui_change_skin"); t_change_face = (MissionUi*)new_key_label(buttons, g_strtable.lookup(v), &t); }
    { tstring t("face_t_change_hair"); tstring v("ui_change_hair"); t_change_hair = (MissionUi*)new_key_label(buttons, g_strtable.lookup(v), &t); }
    { tstring n("face_sl_age"); sl_age = make_slider(buttons, &n); }
    { tstring t("face_t_age"); tstring v("ui_age"); t_age = (MissionUi*)new_key_label(buttons, g_strtable.lookup(v), &t); }
    { tstring n("face_sl_hair_color"); sl_hair_color = make_slider(buttons, &n); }
    { tstring t("face_t_hair_color"); tstring v("ui_change_hair_color"); t_hair_color = (MissionUi*)new_key_label(buttons, g_strtable.lookup(v), &t); }
    if (g_anim.skin_vec[skin].n_beard == 0) {
        b_next_beard->hide();
        b_prev_beard->hide();
        t_change_beard->hide();
    }
    { b_randomize = medium_button(buttons, *g_strtable.lookup("ui_randomize"), 0.0f, 0.0f, arrow_h); }
    label_refresh(b_randomize, "face_b_randomize");
    { b_reset = medium_button(buttons, *g_strtable.lookup("ui_reset"), 0.0f, 0.0f, arrow_h); }
    label_refresh(b_reset, "face_b_reset");
    { b_done = medium_button(buttons, *g_strtable.lookup("ui_done"), 0.0f, 0.0f, arrow_h); }
    label_refresh(b_done, "face_b_done");

    // ---- face body, world and lights ----------------------------------------------------------
    body = new MapWidget;   // 0x4ed630
    {
        world = new rgl::world((const float*)&face_pair(-80.0f, -80.0f), (const float*)&face_pair(80.0f, 80.0f),
                               &(const int&)0, &(const int&)0, &(const int&)0);
        rgl::world* w = world;
        w->ambient = rgl::vector4(0.3f, 0.3f, 0.3f);
        w->ambient.w = 1.0f;
    }
    {
        Light* wd = add_light(world);
        wd->type = 2;
        wd->range = 20.0f;
        wd->color = rgl::vector4(0.1f, 0.15f, 0.2f);
        wd->base_color = rgl::vector4(0.2f, 0.2f, 0.2f);
        wd->pos = rgl::vector4(-2.0f, -2.0f, 12.0f);
        wd->dir = rgl::vector4(-0.5f, 0.0f, -1.0f);
    }
    {
        Light* wd = add_light(world);
        wd->type = 2;
        wd->range = 20.0f;
        wd->color = rgl::vector4(1.0f, 1.0f, 1.0f);
        wd->base_color = rgl::vector4(1.0f, 1.0f, 1.0f);
        wd->pos = rgl::vector4(-2.0f, -2.0f, 2.0f);
        wd->dir = rgl::vector4(1.0f, 1.0f, -1.0f);
    }
    ent = (rgl::strategic_entity*)world->create_entity(9);   // 0x428440
    body->world = world;
    body->move_a(V2(0.05125f, 0.266875f));
    body->set_xy(V2(0.45875f, 0.44f));
    body->scene3d.flags |= 0x300;
    body->scene3d.clear_color = 0xff201815;

    // ---- the camera frustum (35 degrees, fitted to the body) ---------------------------------------------
    {
        float aspect = body->rect_max.y / body->rect_max.x;
        aspect *= g_gui.fov_k;
        ((face_frustum*)&fr)->init_inl(0.61086524f, aspect, 0.01f, 20.0f);
    }
    add_kid(ui, body);

    { fr.m.o = rgl::vector4(0.0f, 0.0f, 10.0f); }
    { *(rgl::vector4*)v154 = rgl::vector4(0.0f, 0.0f, 0.0f); }
    yaw = -0.3f;
    pitch = -0.05f;
    zoom = 0.7f;
    follow = 0;
    body->scene3d.fr = fr;

    // ---- hook the face up to the w14's anim table ------------------------------------------------
    if (w14 >= 0) {
        Troop* rec = &g_game->troops[w14];
        int sk = rec->flags32 & 0xf;
        g_anim.cur = clamp_index(&sk, &(const int&)0, &g_anim.char_count);   // 0x432510
        g_anim.unpack(rec->face_keys);   // 0x473800
        f354 = g_anim.item;
        f350 = g_anim.set;
        f35c = g_anim.material;
        f358 = g_anim.variant;
        f360 = g_anim.scale;
        f364 = g_anim.time;
        f354++;
        f350++;
        for (i = 0; i < keys; i++)
            deform_rows[i]->set168(g_anim.weights[i]);
        sl_age->set168(g_anim.scale);
        sl_hair_color->set168(g_anim.time);
    }
    refresh();
}
