//-----------------------------------------------------------------------------
// File: agent_hud.cpp  (0x4f7d70..0x501a20)
//
// Desc: the mission's agent (re)initialisation and look refresh, the mission HUD (begin-of-frame refresh, the
//       status widget, the camera tick, the sun / flare update), the item mesh builder, the game-log and game
//       HUD builders with the map-text list widget, the std::vector instantiations kept from this object, the
//       editor's terrain dialog and the inventory panel builder. "T regime" region (docs/tu_map.md): cl emitted
//       this object in source order, so the file keeps the binary order and the helpers that follow their
//       caller are __declspec(noinline). Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40; listed directly for the resolution audit)
#include "../../include/image_addrs.h"
#include "../env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "../config/config_block.h"
#include "../strat/gui_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include "./agent_hud.h"
#include "../../rgl/mesh.h"
#include "../../rgl/scene/world.h"      // rgl::world (m30c / m31c: the light listener)
#include "../../rgl/mesh.h"      // mesh::set_part 0x495ba0 (listed directly: the resolution audit reads only the source's own includes)
#include "../agent_obj.h"      // rgl::mat3::rotate_about_u 0x40e770, Agent::set_slot 0x470d80 (listed directly)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../terrain_b/scene_file.h"     // SceneFile: GameData::scenes' record (flags)
#include "../../include/sound_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/fx.h"
#include "../../rgl/resource_manager.h"
#include "../../include/renderer.h"
#include "../skill_cap.h"
#include "../../include/item_kind.h"   // g_item_kind_count 0x5e29d4
#include "../menu_rec.h"   // g_menus 0x5e29b0
#include "../mode_rec.h"   // g_mission_templates 0x5e2a00


// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----




#include "../key_names.h"
#include "../../include/render_flags.h"
#include "../../include/wl_marker.h"
#pragma intrinsic(tan, atan2, atan, sqrt)

// ---- the mission's agent (re)initialisation ------------------------------------------------------------------
struct ModeFlag {
    // 0x435840 as the caller inlines it (Mission::mode_flag1 is out of line elsewhere)
    static __forceinline char of(const Mission* m) { return ((g_mission_templates[m->mode].flags >> 1) & 1) != 0; }
};

// @ 0x4f7d70
void Mission::init_agent(Agent* a)
{
    pick_slots(a);
    refresh_agent(a);
    a->wield[0] = -1;
    a->wield[1] = -1;
    a->on_wield_reset();
    if (a->pool_id == player_id && ((flags & 1) || ModeFlag::of(this))) {
        a->cycle_slot(0);
        a->cycle_slot(1);
    }
    a->set_slot(0, 0x14);
    a->set_slot(1, 0x14);
}

// @ 0x4f7e00
void Mission::refresh_agent(Agent* a)
{
    a->body->fx->clear_defs();
    rgl::lod_mesh** look = a->slot_mesh;                      // the 22 pointers at +0x13c: held-item meshes, node lists, display meshes, meters, fx
    rgl::lod_mesh* zero = 0;
    for (int p = 0; p < 0x16; p++)
        look[p] = zero;
    build_appearance(a->body, a->kind, a->troop, a->seed, &a->item, look, a->armor,
                     a->lods.extra, &a->lods.extra_count, &a->lods.body, (EquipSet*)a->pairs, a->pool_id);
    setup_item_meshes(a->body, a->kind, a->troop, look, &a->weapon_flags, (ItemRef*)a->pairs, a->pool_id);
    a->equip_value = a->equipment_value();
    a->set_slot(0, 0x14);
    a->set_slot(1, 0x14);
}

// new NodeA calls ContainerBase's constructor 0x4931a0 while new ContainerBase expands it in place (0x4f7f00, 0x4fbfa0, ...). One
// definition cannot give both, so the expanded form needs a second, inline constructor in ui/container.h (reported to the orchestrator).
#ifdef CONTAINER_HAS_INL_CTOR
#define NEW_CONTAINER new ContainerBase(ContainerBase::inl_tag())
#else
#define NEW_CONTAINER new ContainerBase
#endif
static __forceinline NodeA* hud_frame() { return new NodeA(0); }
// ContainerBase::ContainerBase stays out of line: new NodeA calls 0x4931a0, but new ContainerBase expands it (0x4f7f00, 0x4fbfa0). One definition cannot give both.
inline UiD::UiD() { f178 = 0; f17c = 0.1f; v168 = 1.0f; f180 = 0; f184 = 0; }
static __forceinline void t_back(Timer& t) { t.start = g_ticks[t.clock] - 10000000; }

static __forceinline rgl::mesh* res_clone(const tstring& n)
{
    return g_resmgr.get_object(&n)->clone();
}

// @ 0x4f7f00
void MissionBlocker::init_screen()
{
    snap = 0;
    if ((float)(rand() % 15817) * 6.322312e-05f < 0.05f)
        g_scene_dirty = 1;
    g_gui.reset();
    g_gui.hud_flag = 1;
    g_game->editor.sel_index = -1;
    g_game->editor.sel.resize(0, 0);

    if (refresh_slots) {
        int id = g_mission->player_id;
        if (id >= 0)
            g_mission->init_agent(agent_raw(id));
        refresh_slots = 0;
    } else if (grow_health) {
        Troop* troop = g_game->troops;
        int lvl = troop->attributes[0];
        int n = troop->nib_b[0x12] & 0xf;
        if (n < 0)
            n = 0;
        else if (n > (*(int*)&g_skill_tbl[36].max_rank))
            n = (*(int*)&g_skill_tbl[36].max_rank);
        int h = lvl + n * 2 + 0x23;
        if (h > status_height) {
            int id = g_mission->player_id;
            if (id >= 0) {
                Agent* a = agent_raw(id);
                float& hp = a->health;
                hp += (float)(h - status_height);
                agent_raw(g_mission->player_id)->max_health = (float)h;
            }
        }
    }

    WL_MARKER(12)
    t_back(timer_b);
    t_back(timer_a);
    sel_mode = -1;
    sel_valid = 0;
    f2e4 = 0;
    f2e8 = 0;
    scene_dirty = 0;
    f2d8 = -1;
    leave_answer = -1;
    g_pending_action = -1;
    g_game->editor.dirty = 0;
    shown_health = -1;
    shield_max = -2;
    ammo_count = -2;
    shield_cur = -1;
    ammo_stage = -1;
    g_game->editor.active = 0;

    frame = hud_frame();
    {
        frame->move_a(V2(0.0f, 0.0f));
        frame->set_xy(V2(1.0f, 0.75f));
    }
    {
        NodeA* root = frame;
        g_gui.dirty = 1;
        g_gui.objs.push_back(*(NodeA**)&root);
    }
    for (int i = 0; i < 3; i++) {
        w[i] = new MissionUi;
        w[i]->move_a(V2(0.5f, 0.375f));
        {
            w[i]->set_xy(V2(0.03f, 0.04f));
        }
        {
            tstring n(i == 0 ? "crosshair_up" : (i == 1 ? "crosshair_left" : "crosshair_right"));
            marks[i] = res_clone(n);
        }
        w[i]->set_track(0, marks[i]);
        frame->add_child(w[i]);
    }
    WL_MARKER(13)
    for (int i = 0; i < 4; i++) {
        w[3 + i] = new MissionUi;
        w[3 + i]->move_a(V2(0.5f, 0.35f));
        {
            w[3 + i]->set_xy(V2(0.03f, 0.04f));
        }
        {
            tstring n(i == 0 ? "arrow_up" : (i == 1 ? "arrow_right" : (i == 2 ? "arrow_down" : "arrow_left")));
            marks[3 + i] = res_clone(n);
        }
        w[3 + i]->set_track(0, marks[3 + i]);
        frame->add_child(w[3 + i]);
    }
    {
        w[3 + 0]->move_a(V2(0.5f, 0.45f));
        w[3 + 1]->move_a(V2(0.65f, 0.35f));
        w[3 + 2]->move_a(V2(0.5f, 0.3f));
        w[3 + 3]->move_a(V2(0.35f, 0.35f));
    }
    color[0] = 0.0f;
    color[1] = 0.0f;
    color[2] = 0.0f;
    color[3] = 0.0f;
    color[4] = 0.0f;
    color[5] = 0.0f;
    color[6] = 0.0f;

    // the status background group, its health bar and bar, the two text widgets and the shield icon
    status_group = NEW_CONTAINER;
    {
        status_group->move_a(V2(0.876249969f, 0.0243749972f));
        status_group->set_xy(V2(0.17f, 0.1175f));
    }
    {
        rgl::mesh* m;
        {
            tstring n("status_background");
            m = res_clone(n);
        }
        status_group->set_track(0, m);
    }
    health_bar = new MissionUi;
    {
        health_bar->move_a(V2(0.0f, 0.0f));
        health_bar->set_xy(V2(0.106500007f, 0.00500000035f));
    }
    {
        {
            tstring n("status_health_bar");
            health_mesh = res_clone(n);
        }
        health_bar->set_track(0, health_mesh);
    }
    status_bar = new MissionUi;
    {
        status_bar->move_a(V2(0.0725000203f, 0.05075f));
        status_bar->set_xy(V2(0.0400000028f, 0.05f));
    }
    shadow_mesh = new rgl::mesh;
    status_bar->set_track(0, shadow_mesh);
    shield_mesh = new rgl::mesh;
    status_bar->set_track(2, shield_mesh);
    shield_icon = new MissionUi;
    {
        shield_icon->move_a(V2(0.0249999985f, 0.0500000082f));
        shield_icon->set_xy(V2(0.05f, 0.0600000024f));
    }
    ui_font = new MissionUi;
    {
        ui_font->move_a(V2(0.0718750283f, 0.045f));
        ui_font->set_xy(V2(0.120000005f, 0.024f));
        ui_font->set148(2);
    }
    {
        tstring n("0");
        ui_font->set_text(n);
    }
    {
        tstring n("ui_font");
        ui_font->tracks[4]->res = g_resmgr.get_material(n);
    }
    ui_font->set_hilight_color(0xffffffff);
    ui_font_bg = new MissionUi;
    {
        ui_font_bg->move_a(V2(0.0718750283f, 0.045f));
        ui_font_bg->set_xy(V2(0.120000005f, 0.024f));
        ui_font_bg->set148(2);
    }
    {
        tstring n("0");
        ui_font_bg->set_text(n);
    }
    {
        tstring n("ui_font_background");
        ui_font_bg->tracks[4]->res = g_resmgr.get_material(n);
    }
    ui_font_bg->set_hilight_color(0xff000000);
    status_group->add_child(status_bar);
    status_group->add_child(health_bar);
    status_group->add_child(shield_icon);
    status_group->add_child(ui_font);
    status_group->add_child(ui_font_bg);
    frame->kids.push_back(status_group);

    name_widget = new MapWidget;
    name_widget->world = g_mission->scene;
    {
        name_widget->move_a(V2(0.0f, 0.0f));
        name_widget->set_xy(V2(1.0f, 0.75f));
    }
    if (g_options.i2c)
        name_widget->f354 = 1;
    {
        rgl::frustum* f = &fr;
        float t = (float)tan(0.8726646304130554);
        f->perspective = 1;
        f->nearz = 0.1f;
        f->farz = 800.0f;
        float b = 0.1f * t;
        f->left = -b;
        float r = t;
        r = r * 0.1f;
        float& rr = r;
        f->right = rr;
        f->bottom = b * -0.75f;
        f->top = rr * 0.75f;
        f->build_planes();
        name_widget->scene3d.fr = *f;
    }
    frame->kids.push_back(name_widget);

    update_status();
    {
        int id = g_mission->player_id;
        if (id >= 0 && agent_raw(id)->horse >= 0) {
            cam_back = 0.1f;
            cam_height = 2.4f;
        } else {
            cam_back = 0.0f;
            cam_height = 1.65f;
        }
    }
    yaw_rate = 0;
    pitch_rate = 0;
    {
        int id = g_mission->player_id;
        if (id >= 0)
        {
            const vector4& vel = agent_raw(id)->vel;
            yaw = (float)atan2(-vel.x, vel.y);
        }
        else
            yaw = 0.0f;
    }
    update_camera(0.001f);
    update_equipment();
}

// @ 0x4f8b40
void MissionBlocker::update_equipment()
{
    if (g_mission->player_id < 0)
        return;
    agent_by_id(g_mission->player_id)->update_equipment_display();
    if (g_game->f139e4 != 0)
        agent_by_id(g_mission->player_id)->show();
    else
        agent_by_id(g_mission->player_id)->hide();
}

// @ 0x4f8bc0
void Agent::hide()
{
    if (slot_mesh[0]) slot_mesh[0]->mask = 0xffff;
    if (slot_mesh[1]) slot_mesh[1]->mask = 0xffff;
    if (slot_mesh[2]) slot_mesh[2]->mask = 0xffff;
    if (slot_mesh[3]) slot_mesh[3]->mask = 0xffff;
    if (slot_mesh4) slot_mesh4->mask = 0xffff;
}

// @ 0x4f8c20
void Agent::show()
{
    if (slot_mesh[0]) slot_mesh[0]->mask = 0x2000;
    if (slot_mesh[1]) slot_mesh[1]->mask = 0x2000;
    if (slot_mesh[2]) slot_mesh[2]->mask = 0x2000;
    if (slot_mesh[3]) slot_mesh[3]->mask = 0x2000;
    if (slot_mesh4) slot_mesh4->mask = 0x2000;
}

// Bodies the original's object saw (header inlines left out of line). cl uses them to keep globals across the call, and an
// object whose address reaches a visible body shares its stack slot (docs/codegen.md section 7). The owner of each row is
// shapes.cpp / game_util.cpp; the build links those.
inline __declspec(noinline) float rgl::vector4::normalize()
{
    float len = (float)sqrt(x * x + y * y + z * z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
static __forceinline vector4 vis_mul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vis_add(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vis_sub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
inline __declspec(noinline) void rgl::mat3::rotate_about_s(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 nf, nu;
    nf = vis_add(vis_mul(f, cos((double)*angle)), vis_mul(u, sn));
    nu = vis_sub(vis_mul(u, cos((double)*angle)), vis_mul(f, sn));
    u = nu;
    f = nf;
}
inline __declspec(noinline) void rgl::mat3::rotate_about_f(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nu;
    ns = vis_sub(vis_mul(s, cos((double)*angle)), vis_mul(u, sn));
    nu = vis_add(vis_mul(u, cos((double)*angle)), vis_mul(s, sn));
    s = ns;
    u = nu;
}
inline __declspec(noinline) float __cdecl clamp(const float* x, const float* lo, const float* hi)
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}
inline __declspec(noinline) float __cdecl fmax_ref(const float* a, const float* b) { return *a > *b ? *a : *b; }
inline __declspec(noinline) rgl::mat3* __cdecl rgl::identity3(rgl::mat3* out)
{
    vector4 a(1.0f, 0.0f, 0.0f), b(0.0f, 1.0f, 0.0f), c(0.0f, 0.0f, 1.0f);
    out->s = a;
    out->f = b;
    out->u = c;
    return out;
}
inline __declspec(noinline) matrix* __cdecl rgl::identity_pos(matrix* out)
{
    vector4 o(0.0f, 0.0f, 0.0f);
    rgl::mat3 m;
    *(rgl::mat3*)out = *identity3(&m);
    out->o = o;
    return out;
}
inline __declspec(noinline) float rgl::vector4::out_length() const { return length(); }
inline __declspec(noinline) vector4 rgl::matrix::rot(const vector4& p) const
{
    return vector4(p.x * s.x + (f.x * p.y + u.x * p.z),
                   f.y * p.y + (s.y * p.x + u.y * p.z),
                   f.z * p.y + (s.z * p.x + u.z * p.z));
}
inline __declspec(noinline) rgl::mat3 rgl::matrix::rot3(const matrix& m) const { return rgl::mat3(this->rot(m.s), this->rot(m.f), this->rot(m.u)); }
inline __declspec(noinline) matrix rgl::matrix::operator*(const matrix& m) const
{
    struct { rgl::mat3 r; vector4 o; } t;
    t.o = transform_point_inl(m.o);
    t.r = rot3(m);
    return *(matrix*)&t;
}
// lanes through three float locals: same instructions as the one-expression body, but the optimiser's address
// temporaries take other keys and one more product of camera_to's first sun cross comes out in the original's order
inline vector4 rgl::cross(const vector4& a, const vector4& b)
{
    float x = a.y * b.z - a.z * b.y;
    float y = a.z * b.x - a.x * b.z;
    float z = a.x * b.y - a.y * b.x;
    return vector4(x, y, z);
}
inline __declspec(noinline) capsule::capsule(float r, const vector4* a, const vector4* b) : shape_base(inline_init())
{
    radius = r;
    l0 = *a;
    l1 = *b;
    p0 = l0;
    p1 = l1;
}
// Inline vector helpers as the original's TU had them (scalar by const reference: one float store per use).
// The fourth lane of every result is left unset (the original copies whole vector4s with an unset w).
static __forceinline vector4 hv_scaled(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline void hv_addr(vector4& a, const vector4& b) { a.x = b.x + a.x; a.y = b.y + a.y; a.z = b.z + a.z; }
static __forceinline vector4 hv_mulv(const vector4& a, float s) { return vector4(a.x * s, a.y * s, a.z * s); }
struct HvS { double v; __forceinline HvS(double s) : v(s) {} };
static __forceinline vector4 hv_m6(const vector4& a, HvS s) { return vector4(a.x * s.v, a.y * s.v, a.z * s.v); }
// a scale by double: the float lanes load first, the scale stays live on the x87 stack and is dropped by fstp st(0)
static __forceinline vector4 hv_add(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 hv_neg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }
static __forceinline void hv_add_to(vector4& a, const vector4& b) { a.x = a.x + b.x; a.y = a.y + b.y; a.z = a.z + b.z; }
static __forceinline void hv_sub_to(vector4& a, const vector4& b) { a.x = a.x - b.x; a.y = a.y - b.y; a.z = a.z - b.z; }
#define AG(i) agent_by_id(i)
// stand-in: the source operand comes first in the formals only because that order gives the original's x lane (`fld head.x` first)
static __forceinline void hv_add_mix(const vector4& b, vector4& a) { a.x = a.x + b.x; a.y = b.y + a.y; a.z = b.z + a.z; }
static __forceinline float fmax_i(const float& a, const float& b) { return a > b ? a : b; }
static __forceinline void hv_view(rgl::frustum& f, const matrix& m) { f.m = m; f.build_planes(); }
static __forceinline float clamp_i(const float& x, const float& lo, const float& hi) { return x < lo ? lo : (x >= hi ? hi : x); }
static __forceinline void hv_sub_of(vector4& r, const vector4& a, const vector4& b) { r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; }
static __forceinline void cam_to(const matrix* e, const float& far_plane) { g_mission->camera_to(e, &far_plane); }
static __forceinline bool hud_day(float t) { return 4.0f <= t && t < 20.0f; }
static __forceinline float cl01(const float& e) { return e < 0.0f ? 0.0f : (e >= 1.0f ? 1.0f : e); }
static __forceinline void rot_s(matrix& m, const float& a) { m.rotate_about_s(&a); }
static __forceinline void rot_u(matrix& m, const float& a) { m.rotate_about_u(&a); }
static __forceinline void rot_f(matrix& m, const float& a) { m.rotate_about_f(&a); }
static __forceinline float clamp_v(const float& x, const float& lo, const float& hi) { return clamp(&x, &lo, &hi); }
static __forceinline float fmax_v(const float& a, const float& b) { return fmax_ref(&a, &b); }

static inline float timer_secs(const Timer* t) { return (float)(g_ticks[t->clock] - t->start) * 1e-5f; }   // Timer::elapsed written inline
#define gcam g_game->editor.cam_pos
#define geye g_game->editor.sel_copy.o
// @ 0x4f8c80
void MissionBlocker::update_camera(float dt)
{
    float radius, zoomL;

    if (g_game->editor.active != 0)
        dt = g_step_tbl[0];
    {
        matrix home;
        *(matrix*)&k = *(matrix*)identity_pos((pos*)&home);
    }
    radius = 0.03f;
    if (g_cfg.use_wbuffer != 0)
        radius = 0.01f;
    zoomL = zoom;
    if (g_game->editor.active != 0) {
        if ((float)(g_ticks[g_game->editor.timer.clock] - g_game->editor.timer.start) * 1e-5f < 0.0f) {
            vector4 c;
            c.x = gcam.x - geye.x;
            c.y = gcam.y - geye.y;
            c.z = gcam.z - geye.z;
            vector4 a = hv_add(hv_m6(c, (1.0f - dt) * c.normalize()), geye);
            float ya = atan2(-(-c.x), -c.y);
            if (yaw > 6.2831855f)
                yaw = yaw - 6.2831855f;
            else if (yaw < 0.0f)
                yaw = yaw + 6.2831855f;
            if (ya > 6.2831855f)
                ya = ya - 6.2831855f;
            else if (ya < 0.0f)
                ya = ya + 6.2831855f;
            ya = ya - yaw;
            if (ya > 3.1415927f)
                ya = -(6.2831855f - ya);
            if (ya < -3.1415927f)
                ya = ya + 6.2831855f;
            yaw = ya * dt + yaw;
            gcam = a;
        }
    }
    if (g_game->editor.active != 0) {
        // free camera: keys move the game's camera position along the camera's own axes
        float t, u;
        rot_s(k, 1.5707964f);
        k.rotate_about_f(&yaw);
        k.rotate_about_s(&pitch);
        k.rotate_about_f(&roll);
        free_cam_speed = (1.0f - (dt + dt)) * free_cam_speed;
        if (free_cam_speed < 0.0f)
            free_cam_speed = 0.0f;
        else if (free_cam_speed > 20.0f)
            free_cam_speed = 20.0f;
        t = free_cam_speed;
        if (g_keyboard.down[0x11] == 0 && g_keyboard.prev[0x11] != 0) {
            int now = (int)g_ticks[0];
            if (now - g_game->editor.cam_step_time < 35000) {
                int step = g_game->editor.cam_step * 2;
                if (step >= 0x20)
                    step = 0x20;
                g_game->editor.cam_step = step;
            } else {
                g_game->editor.cam_step = 1;
            }
            g_game->editor.cam_step_time = now;
        }
        if (g_keyboard.down[0x11] != 0) {
            u = (float)g_game->editor.cam_step * t;
            hv_sub_to(gcam, ((k.u * u) * dt));
            free_cam_speed = dt * 25.0f + free_cam_speed;
        }
        if (g_keyboard.down[0x1f] != 0) {
            hv_add_to(gcam, ((k.u * t) * dt));
            free_cam_speed = dt * 25.0f + free_cam_speed;
        }
        if (g_keyboard.down[0x1e] != 0) {
            hv_sub_to(gcam, ((k.s * t) * dt));
            free_cam_speed = dt * 17.5f + free_cam_speed;
        }
        if (g_keyboard.down[0x20] != 0) {
            hv_add_to(gcam, ((k.s * t) * dt));
            free_cam_speed = dt * 17.5f + free_cam_speed;
        }
        if (g_keyboard.down[0x21] != 0) {
            gcam.z = t * dt * 0.5f + gcam.z;
            free_cam_speed = dt * 17.5f + free_cam_speed;
        }
        if (g_keyboard.down[0x2e] != 0 && g_keyboard.down[0x1d] == 0 && g_keyboard.down[0x9d] == 0) {
            gcam.z = gcam.z - t * dt * 0.5f;
            free_cam_speed = dt * 17.5f + free_cam_speed;
        }
        k.o = gcam;
    } else if (g_mission->cam_mode != 0) {
        // the scripted camera slot: cur -> sway over value seconds
        if (g_mission->blend_time > 0.0f) {
            float t = (((float)(g_ticks[g_mission->blend_timer.clock] - g_mission->blend_timer.start) * 1e-5f) / g_mission->blend_time);
            if (g_mission->blend_n > 0) {
                t = atan((t - 0.5f) * (float)g_mission->blend_n * 3.1415927f) / (atan((float)g_mission->blend_n * 1.5707964f) * 2.0f) + 0.5f;
            }
            if (t >= 1.0f) {
                g_mission->frame_a = g_mission->frame_b;
                g_mission->blend_from = g_mission->blend_to;
                k.u = hv_neg(g_mission->frame_a.f);
                k.f = g_mission->frame_a.u;
                k.s = g_mission->frame_a.s;
                k.o = g_mission->frame_a.o;
                zoomL = g_mission->blend_to;
                g_mission->blend_time = 0.0f;
            } else {
k.o = hv_add(hv_mulv(g_mission->frame_b.o, t), hv_mulv(g_mission->frame_a.o, 1.0f - t));
                k.u = hv_neg(hv_add(hv_mulv(g_mission->frame_b.f, t), hv_mulv(g_mission->frame_a.f, 1.0f - t)));
                k.f = hv_add(hv_mulv(g_mission->frame_b.u, t), hv_mulv(g_mission->frame_a.u, 1.0f - t));
                k.s = hv_add(hv_mulv(g_mission->frame_b.s, t), hv_mulv(g_mission->frame_a.s, 1.0f - t));
                zoomL = g_mission->blend_from * (1.0f - t) + g_mission->blend_to * t;
            }
        } else {
            k.u = hv_neg(g_mission->frame_a.f);
            k.f = g_mission->frame_a.u;
            k.s = g_mission->frame_a.s;
            k.o = g_mission->frame_a.o;
            zoomL = g_mission->blend_from;
        }
        if (g_mission->cam_target_agent >= 0) {
            if (g_mission->cam_use_agent_rotation != 0)
                *(matrix*)&k = AG(g_mission->cam_target_agent)->body->local * k;
            else
                hv_addr(k.o, AG(g_mission->cam_target_agent)->body->local.o);
        }
        k.orthonormalize();
    } else {
      {
        if (g_mission->player_id >= 0 && g_game->f139e4 != 0 && AG(g_mission->player_id)->kind == 0) {
            // spectating: look through the agent this one is bound to
            Agent* ag = AG(g_mission->player_id);
            rgl::strategic_entity* body = AG(g_mission->player_id)->body;
            FxDef* sk;
            if (AG(g_mission->player_id)->horse >= 0)
                yaw = dt * AG(ag->horse)->turn_rate + yaw;
            sk = body->fx;
            if ((ag->defs->data[ag->anim[0].cur].flags & 0x100000) != 0) {
                if (!sk->pose_valid)
                    sk->ensure_updated();
                *(matrix*)&k = body->local * sk->elems[9].m84;
                rot_f(k, 3.1415927f);
                hv_add_to(k.o, (k.f * 0.15f));
            } else {
                if (!sk->pose_valid)
                    sk->ensure_updated();
                vector4 n = sk->elems[9].m84.o;
                vector4 c = n - lag;
                float hi = 1.0f;
                float lo = 0.0f;
                float l = c.out_length();
                float x = l * dt * 200.0f;
                hv_add_to(lag, hv_m6(c, clamp_v(x, lo, hi)));
                { vector4 tp = body->local.transform_point(lag);
                k.o = tp; }
                rot_s(k, 1.5707964f);
                k.rotate_about_f(&yaw);
                k.rotate_about_s(&pitch);
                hv_add_to(k.o, hv_add((k.f * 0.15f), (k.u * 0.11f)));
            }
        } else if (g_mission->player_id >= 0) {
            // third person: the player's own agent
            Agent* ag = AG(g_mission->player_id);
            if (g_game_keys.is_down(0xd)) {
                float fa = 0.8f;
                float fb = 0.5f;
                if (AG(g_mission->player_id)->horse >= 0) {
                    fa = 1.1f;
                    fb = 1.0f;
                }
                cam_height = (fa - cam_height) * dt * 3.0f + cam_height;
                cam_back = (fb - cam_back) * dt * 3.0f + cam_back;
            } else {
                float h;
                if (AG(g_mission->player_id)->horse >= 0) {
                    cam_back = (0.1f - cam_back) * dt + cam_back;
                    h = 2.4f;
                } else {
                    cam_back = (0.0f - cam_back) * dt + cam_back;
                    h = 1.65f;
                }
                cam_height = (h - cam_height) * dt + cam_height;
            }
            head = ag->pos;
            head.z += cam_height;
            hv_add_mix(ag->v98, head);
            rot_s(k, 1.5707964f);
            k.rotate_about_f(&yaw);
            k.rotate_about_s(&pitch);
            k.rotate_about_f(&roll);
            k.o = head;
            float d = clamp_i(pitch, -3.0f, 3.0f) * 0.0f + cam_dist + cam_back;
            hv_add_to(k.o, (k.u * d));
            hv_add_to(k.o, ((k.f * 0.37f) * d));
            if (g_game->editor.active == 0) {
                // keep the camera out of the walls: sweep a thin capsule from the head to the camera
                shape_base* list[0x2000];
                contact ct;
                int count, i;
                float t;
                vector4 q, c;
                vector4 p0 = k.o;
                hv_sub_of(q, k.o, (k.u * d));
                hv_sub_of(c, q, p0);
                c.normalize();
                vector4 a = (p0 - (c * 0.4f));
                capsule cap(1e-5f, &q, &a);
                g_mission->scene->query_segment(&p0, &q, 0x110, list, &count);
                float d2 = d;
                t = 1000.0f;
                for (i = 0; i < count; i++) {
                    shape_base* sh;
                    if (list[i]->shape_type() == 2 && (sh = list[i], shape_contact_count(sh, &cap, 1, &ct) != 0)) {
                        vector4 n = ((face*)list[i])->normal;
                        float dx = ct.point.x - q.x, dy = ct.point.y - q.y, dz = ct.point.z - q.z;
                        float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
                        float dot = c.x * n.x + c.y * n.y + c.z * n.z;
                        if (dot > 0.0f && dist < t) {
                            t = dist;
                            d2 = dist - (radius * 2.5f / fmax_v(dot, 0.1f) + 0.6f);
                            if (d2 < 0.25f)
                                d2 = 0.25f;
                        }
                    }
                }
                cam_pull = (d2 - cam_pull) * dt * 11.0f + cam_pull;
                { vector4 r = k.u * cam_pull; k.o = vector4(r.x + q.x, r.y + q.y, r.z + q.z); }
            }
        }
      }
    }
    if (g_mission->cam_mode == 0) {
        if (g_mission->blend_time > 0.0f) {
            float t = ((float)(g_ticks[g_mission->blend_timer.clock] - g_mission->blend_timer.start) * 1e-5f) / g_mission->blend_time;
            if (g_mission->blend_n > 0) {
                t = atan((t - 0.5f) * (float)g_mission->blend_n * 3.1415927f) / (atan((float)g_mission->blend_n * 1.5707964f) * 2.0f) + 0.5f;
            }
            if (t >= 1.0f) {
                g_mission->blend_time = 0.0f;
            } else {
                matrix m;
                m.u = hv_neg(g_mission->frame_b.f);
                m.f = g_mission->frame_b.u;
                m.s = g_mission->frame_b.s;
                m.o = g_mission->frame_b.o;
                if (g_mission->cam_target_agent >= 0) {
                    if (g_mission->cam_use_agent_rotation != 0) {
                        m = AG(g_mission->cam_target_agent)->body->local * m;
                    } else {
                        vector4* bo = &AG(g_mission->cam_target_agent)->body->local.o;
                        m.o.x = m.o.x + bo->x;
                        m.o.y = m.o.y + bo->y;
                        m.o.z = m.o.z + bo->z;
                    }
                }
                k.o = hv_add(hv_mulv(k.o, t), hv_scaled(m.o, 1.0f - t));
                k.u = hv_add(hv_mulv(k.u, t), hv_scaled(m.u, 1.0f - t));
                k.f = hv_add(hv_mulv(k.f, t), hv_scaled(m.f, 1.0f - t));
                k.s = hv_add(hv_mulv(k.s, t), hv_scaled(m.s, 1.0f - t));
                zoomL = (1.0f - t) * g_mission->blend_to + t * zoomL;
            }
            k.orthonormalize();
        }
        if (g_mission->cam_mode == 0) {
            g_mission->frame_a.f = hv_neg(k.u);
            g_mission->frame_a.u = k.f;
            g_mission->frame_a.s = hv_neg(k.s);
            g_mission->frame_a.o = k.o;
            g_mission->blend_from = zoomL;
        }
    }
    // the frustum
    float aspect = g_gui.view_b / g_gui.view_a;
    hv_view(fr, k);
    float v = (float)sqrt(1.0f / aspect);
    v = 1.17f < v ? 1.17f : v;
    v = v * zoomL * 0.86f;
    v = 102.0f < v ? 102.0f : v;
    fr.init(v * 0.017453292f, aspect, radius, 800.0f);
    name_widget->scene3d.fr = fr;
    cam_to(&k, 97.0f / zoomL);
}


#undef gcam
#undef geye

static __forceinline float dot3(const vector4& a, const vector4& b) { return a.y * b.y + a.z * b.z + a.x * b.x; }
struct MatrixProbe { float r[12]; };
// @ 0x4fa790
void Mission::camera_to(const matrix* eye, const float* far_plane)
{
    view = *(const rgl::matrix*)eye;
    apply_sun_light(&eye->o, far_plane);
    ((rgl::world*)scene)->cam_dir = hv_neg(eye->u);
    ((rgl::world*)scene)->cam_pos = eye->o;
    matrix lis = *eye;
    hv_sub_to(lis.o, 0.4f * lis.u);
    if (g_cfg.disable_sound == 0) {
        g_sound_mgr.frame = lis;
        // FMOD is y-up: y and z swap (0x4fa887..0x4fa8ae push f.x, f.z, f.y, u.x, u.z, u.y)
        FSOUND_3D_Listener_SetAttributes(&g_sound_mgr.frame.o.x, 0, g_sound_mgr.frame.f.x, g_sound_mgr.frame.f.z, g_sound_mgr.frame.f.y, g_sound_mgr.frame.u.x,
                                         g_sound_mgr.frame.u.z, g_sound_mgr.frame.u.y);
    }
    // the rain / snow emitter follows the eye
    if (precip_fx) {
        precip_fx->local.o = eye->o;
        precip_fx->local.f = hv_neg(eye->u);
        precip_fx->local.u = vector4(0.0f, 0.0f, 1.0f);
        // per-statement references: their keys set the fld/fmul operand order of each product (globopt re-sorts
        // the CSE'd products by base keys); with the inline formals as bases the x lane of the first cross flipped
        { const vector4& f = precip_fx->local.f; const vector4& u = precip_fx->local.u; precip_fx->local.s = cross(f, u); }
        precip_fx->local.s.normalize();
        { const vector4& u = precip_fx->local.u; const vector4& s = precip_fx->local.s; precip_fx->local.f = cross(u, s); }
        if (rain_snd < 0 && precip_mode == 1)
            rain_snd = g_sound_mgr.play_slot(8, 3, &g_sound_mgr.frame.o, 1.0f, 0);
    }
    // the sun billboard, 500 units towards the sun
#define CL01(E) ((E) < 0.0f ? 0.0f : ((E) >= 1.0f ? 1.0f : (E)))
    if (sun_fx) {          // (no flare: the original jumps to the particle update at 0x4faea5, it does not return)
    float tod = g_game->time_of_day();
    vector4 dir = g_env_rows[env_row].get_dir((int)timer_secs(&g_game->game_clock) % 24, hud_day(tod));
    sun_fx->local.o = eye->o - dir * 500.0f;
    sun_fx->local.f = dir;
    sun_fx->local.u = vector4(0.0f, 0.0f, 1.0f);
    sun_fx->local.orthonormalize();
    float vis;
    if (!g_shadow_map.sample(&eye->o, &vis)) {
        vector4 to = eye->o - dir * 500.0f;
        int hit = ((rgl::world*)scene)->trace_ray(&eye->o, &to, 500.0f, 0, 0);
        if (hit)
            flare_amt = g_step_tbl[2] * 3.5f + flare_amt;
        else
            flare_amt = flare_amt - g_step_tbl[2] * 3.5f;
        flare_amt = cl01(flare_amt);
    } else {
        vis += vis;
        flare_amt = (vis - flare_amt) * g_step_tbl[2] * 10.0f + flare_amt;
        flare_amt = cl01(flare_amt);
    }
    sun_fx_mesh->clear_geometry();
    {
        tstring name("sample_flare");
        matrix id;
        const rgl::mat4* idm = (const rgl::mat4*)identity_pos((pos*)&id);
        rgl::mesh* src = g_resmgr.get_object(&name);
        sun_fx_mesh->append_skinned(src, idm, -1, -1, 0);
    }
    {
        tstring name("flare");
        sun_fx_mesh->res = g_resmgr.get_material(name);
    }
    float sc2;
    {
        float k = (dot3(dir, eye->u) - 0.3f) * 1.4285715f;
        k = CL01(k);
        sc2 = k * k;
    }
    float in = sc2 * (cl01(1.0f - 150.0f / fog_far) * g_env_rows[env_row].size * flare_amt);
    {
        vector4 col(in * 1.1f, in, in * 0.8f);
        sun_fx_mesh->color = rgl::pack_color(&col);
    }
    sun_fx_mesh->color |= 0xff000000;
    { float a = sc2*sc2; sun_fx_mesh->scale((a * 5.0f + 2.0f) * g_env_rows[env_row].size * 50.0f); }
    }
#undef CL01
    if (g_game->scenes[scene_idx].flags & 0x100)
        g_terrain.update_particles(eye);
}

// @ 0x4faee0
bool ShadowMap::sample(const vector4* p, float* out)
{
    // (dx, dy) tap offsets: the original builds two int[9] tables on the stack (x then y)
    volatile bool ret = true;           // the original keeps the result byte in memory
    *out = 1.0f;
    int tx[9] = { 0, 1, 1, -1, -1, 2, -2, 0, 0 };
    int ty[9] = { 0, 1, -1, 1, -1, 0, 0, 2, -2 };
    float w[9] = { 1.0f, 0.9f, 0.9f, 0.9f, 0.9f, 0.7f, 0.7f, 0.7f, 0.7f };
    if (!cam || !g_use_shaders || !surf) {
        ret = false;
        return false;
    }
    rgl::frustum* lf = &cam->view.fr;
    vector4 clip = lf->to_clip(*p);
    if (!(0.0f <= clip.x && clip.x < 1.0f && 0.0f <= clip.y && clip.y < 1.0f))
        return true;
    int zero_x = 0;
    struct { float x, y; } fp = { (float)(int)scn.vp_local.Width * clip.x, (1.0f - clip.y) * (float)(int)scn.vp_local.Height };
    *out = 0.0f;
    int px = (int)fp.x;
    int zero_y = 0;
    int py = (int)fp.y;
    float depth = clip.z - 0.001f; float*& oref = out;
    for (int i = 0; i < 9; i++) {
        int xv = tx[i] + px;
        int yv = ty[i] + py;
        float a, b;
        read_pixel(g_render.dev, clamp_index(&xv, &zero_x, (const int*)&scn.vp_local.Width),
                   clamp_index(&yv, &zero_y, (const int*)&scn.vp_local.Height), (unsigned*)&a, (unsigned*)&b);
        if (!(a < depth))
            *out += w[i];
    }
    *oref = *oref * 0.13513514f;
    return ret;
}

// @ 0x4fb180
void Mission::apply_sun_light(const vector4* pos, const float* scale)
{
    float px = (float)(g_gui.screen_w + g_gui.screen_h);
    float area = (px * 0.5f) * 0.00086956524f;
    float size_v = ((g_options.scale_y * 0.5f + 0.75f) * *scale) * area;
    float& size = *(float*)&scale;       // the original stores the result back into the (dead) parameter slot
    size = size_v;
    if (g_keyboard.down[0x0d] && (g_keyboard.down[0x1d] || g_keyboard.down[0x9d]))
        size = 1000.0f;
    float range = g_options.scale_z * area;
    Mission* vl = this;
    AgentTable* tbl = &agents;
    int n = *(int*)tbl;
    unsigned i = 0;
    if (0 < n) {
        do {
            if (tbl->get(i)->in_use)
                break;
            i++;
        } while ((int)i < n);
    }
    if (i < *(unsigned*)tbl) {
        do {
            Agent* a = agents.pages[i >> 4] + (i & 0xf);
            if (a->body) {
                float intensity;
                const vector4& o = a->body->local.o;
                float dx = o.x - pos->x;
                float dy = o.y - pos->y;
                float dz = o.z - pos->z;
                float d = (float)sqrt(dy * dy + (dx * dx + dz * dz));
                float dist = 3.5f / (d > 3.5f ? d : 3.5f);
                if (!(d > 1.0f))
                    d = 1.0f;
                intensity = (1.0f / d) * size;
                a->body->set_f144(&intensity);
                if (a->pool_id == vl->player_id || a->rider == vl->player_id) {
                    float one = 1.0f;
                    a->body->set_fade(&one);
                } else {
                    float v = dist * range;
                    a->body->set_fade(&v);
                }
            }
            int n2 = *(int*)tbl;
            for (i++; (int)i < n2; i++) {
                if (tbl->get(i)->in_use)
                    break;
            }
        } while (i < *(unsigned*)tbl);
    }
}


static __forceinline rgl::mesh* item_mesh_v(const ItemRef* r, const float& s) { return make_item_mesh(r, &s); }
// @ 0x4fb350
void MissionBlocker::update_status()
{
    int id = g_mission->player_id;
    if (id < 0)
        return;
    Agent* ag = agent_by_id(id);
    if (g_game->f1a20c)
        status_group->hide();
    else
        status_group->show();
    int hp = (int)ag->health;
    if (hp != shown_health) {
        shown_health = hp;
        health_mesh->clear_geometry();
        {
            matrix frame;
            tstring name("status_health_bar");
            const rgl::mat4* m = (const rgl::mat4*)identity_pos((pos*)&frame);
            rgl::mesh* obj = g_resmgr.get_object(&name);
            health_mesh->append_skinned(obj, m, -1, -1, 0);
        }
        vector4 col((float)shown_health / ag->max_health, 1.0f, 1.0f);
        health_mesh->set_color(col);
    }

    // shield: slot_pair(0) says whether the agent has one; best_ammo then gives (max, ?, ?) and the (cur, flags) pair
    int la = -1, lb = -1, lc;
    ItemRef pair = ag->slot_pair(0);
    ItemRef res;
    res.item = -1;
    if (pair.item >= 0)
        res = ag->best_ammo(-1, &la, &lb, &lc);
    if (la != shield_max || res.item != shield_cur) {
        shield_max = la;
        { char buf[512];
        sprintf(buf, "%d", la);
        {
            tstring t1(buf);
            ui_font->set_text(t1);
        }
        {
            tstring t2(buf);
            ui_font_bg->set_text(t2);
        }
        if (shield_max < 0) {
            status_bar->hide();
            ui_font->hide();
            ui_font_bg->hide();
        } else {
            status_bar->show();
            ui_font->show();
            ui_font_bg->show();
            if (shield_max > 0)
                ui_font->set_color(0xffffffff);
            else
                ui_font->set_color(0xffb02020);
        }
        }
        if (res.item != shield_cur) {
            shield_mesh->clear_geometry();
            shadow_mesh->clear_geometry();
            if (res.item >= 0) {
                float piece_scale = 0.055f; rgl::mesh* piece = make_item_mesh(&res, &piece_scale);
                if (piece) {
                    matrix frame;
                    vector4 offset;
                    offset.x = 0.0005f;
                    piece->set_offset(offset);
                    identity3((rgl::mat3*)&frame);
                    rot_u((rgl::matrix&)frame, -1.45f);
                    piece->set_part(frame);
                    identity_pos((pos*)&frame);
                    frame.o.z -= 10.0f;
                    shield_mesh->append_skinned(piece, (const rgl::mat4*)&frame, -1, -1, 0);
                    float nx = frame.o.x + 0.0012f;
                    shield_mesh->res = piece->res;
                    frame.o.x = nx;
                    frame.o.y -= 0.001f;
                    frame.o.z -= 10.0f;
                    piece->scale(1.05f);
                    shadow_mesh->append_skinned(piece, (const rgl::mat4*)&frame, -1, -1, 0);
                    {
                        tstring shadow("ui_item_shadow");
                        shadow_mesh->res = g_resmgr.get_material(shadow);
                    }
                    ((rgl::mesh*)shadow_mesh)->set_bone_index(-1);
                    shadow_mesh->color = 0xff777777;
                    delete piece;
                }
            }
            shield_cur = res.item;
            shield_aux = res.m4;
        }
    }

    // ammo count: (count, capacity) from weapon_info
    unsigned lo = -1;
    int hi = -1;
    ItemRef ammo_pair = ag->slot_pair(1);
    if (ammo_pair.item >= 0)
        ag->weapon_info(-1, &lo, &hi);
    if ((int)lo != ammo_count) {
        ammo_count = lo;
        if ((int)lo == -1) {
            ammo_stage = -1;
            shield_icon->hide();
            return;
        }
        int stage = (int)((float)(int)lo * 5.0f / (float)hi + 0.999f);
        if (ammo_stage != stage) {
            ammo_stage = stage;
            if (stage <= 0) {
                shield_icon->hide();
                return;
            }
            rgl::mesh* cl;
            if (stage == 1) {
                tstring icon("status_shield_20");
                cl = res_clone(icon);
            } else if (stage == 2) {
                tstring icon("status_shield_40");
                cl = res_clone(icon);
            } else if (stage == 3) {
                tstring icon("status_shield_60");
                cl = res_clone(icon);
            } else if (stage == 4) {
                tstring icon("status_shield_80");
                cl = res_clone(icon);
            } else {
                tstring icon("status_shield_100");
                cl = res_clone(icon);
            }
            shield_icon->set_track(0, cl);
            shield_icon->show();
        }
    }
}


static __forceinline vector4 hv_scaled2(const float& s, const vector4& a) { return vector4(s * a.x, s * a.y, s * a.z); }

// @ 0x4fb8e0: shift every point of the ammo mesh by the scaled vertex normal, and every morph
// key frame's vertex by its scaled normal.
void rgl::mesh::set_offset(const vector4& pos)
{
    ensure_local();
    for (unsigned i = 0; i < vertices.size(); i++) {
        int idx = vertices[i].index;
        hv_add_to(points[idx].a, hv_scaled(vertices[i].normal, pos.x));
        hv_add_to(points[idx].b, hv_scaled(vertices[i].normal, pos.x));
        for (int j = 0; j < kf.count; j++)
            hv_add_to(kf.frames[j].verts[idx], hv_scaled2(pos.x, kf.frames[j].norms[i]));
    }
    slots[0].f28 = 1;                       // +0x138 (dirty flag)
    ((rgl::mesh*)this)->compute_bounds();
}

static __forceinline const rgl::string* as_rgl_string(const tstring& t) { return &t; }

// @ 0x4fba40 -------------------------------------------------------------------------------
rgl::mesh* __cdecl make_item_mesh(const ItemRef* ref, const float* scale)
{ const float*& scr = scale;
    rgl::mesh* result;
    matrix m;
    if (ref->item >= 0 && ref->item < g_item_kind_count &&
        (result = (rgl::mesh*)g_items[ref->item].mesh_for(0x1000000000000000ULL, (int)ref->b.mod)) != 0) {
        result = result->clone();
    } else {
        // built in place into the local (0x4fbaa7 passes the local as the hidden return pointer); assigning a
        // temporary would copy it into a second buffer, one extra 4-byte allocation the original never makes. The
        // name temporary dies at the end of the statement (0x4fbab0).
        std::vector<rgl::mesh*> objects = g_resmgr.find_meshes(as_rgl_string(((ItemRef*)ref)->mesh_name()));
        if (objects.empty()) {
            mb_warning("Unable to find mesh %s", ((ItemRef*)ref)->mesh_name().c_str());
            result = new rgl::mesh;
            return result;
        }
        result = objects[0]->clone();
        for (unsigned i = 1; i < objects.size(); i++) {
            const rgl::mat4* idm = (const rgl::mat4*)identity_pos((pos*)&m);
            result->append_skinned(objects[i], idm, -1, -1, 0);
        }
    }

    unsigned type = g_items[ref->item].type_flags & 0xff;
    identity3((rgl::mat3*)&m);
    matrix& mr = *(matrix*)&m;
    if (type == 0xd) {
        rot_s(mr, -1.5707964f);
        rot_u(mr, 3.1415927f);
    } else if (type == 1) {
        rot_s(mr, -1.5707964f);
        rot_u(mr, 1.5707964f);
    } else if (type == 2 || type == 3 || type == 4) {
        rot_u(mr, -0.7853982f);
    } else if (type == 0xe) {
        rot_s(mr, -1.5707964f);
        rot_u(mr, 2.670354f);
        rot_s(mr, -0.31415927f);
    } else if (type == 7) {
        rot_u(mr, 1.5707964f);
    } else if (type == 0xc) {
        rot_f(mr, -0.47123894f);
        rot_s(mr, 0.62831855f);
    } else if (type == 9) {
        rot_f(mr, 1.5707964f);
        rot_s(mr, -2.3561945f);
    } else if (type == 0x11) {
    } else if (type == 8) {
        rot_u(mr, 2.3561945f);
    } else if (type == 0xa || type == 5 || type == 6) {
        rot_u(mr, 0.7853982f);
    } else if (type == 0xb) {
        rot_s(mr, -1.2566371f);
        rot_u(mr, 0.7853982f);
    } else if (type == 0x14) {
        rot_s(mr, -0.62831855f);
        rot_u(mr, -0.47123894f);
    }
    result->set_part(m);

    float extent, size; vector4 c; {
    vector4 bmin, bmax;
    result->calc_bounds(&bmin, &bmax);

    vector4 sum = hv_add(bmin, bmax);
    c.x = sum.x * 0.5f; c.y = sum.y * 0.5f; c.z = sum.z * 0.5f;

    unsigned t2 = g_items[ref->item].type_flags & 0xff;
    size = 1.0f;
    if (t2 == 2 || t2 == 3 || t2 == 4) {
        size = (float)g_items[ref->item].weapon_length * 0.00625f + 0.3f;
        if (size < 0.5f)
            size = 0.5f;
        else if (size > 1.0f)
            size = 1.0f;
    }

    extent = fmax_i(bmax.x - bmin.x, bmax.y - bmin.y); }

    { vector4 tr = hv_neg(c);
    result->translate(&tr); }
    result->scale(((*scr / extent) * size) * 0.95f);
    result->darken_vertices();
    return result;
}

// @ 0x4fbef0 -------------------------------------------------------------------------------
void rgl::mesh::darken_vertices()
{
    ensure_local();
    for (unsigned i = 0; i < vertices.size(); i++) {
        rgl::vertex60& v = vertices[i];
        float f = (v.normal.z + 0.25f) * 0.8f;
        vector4 rgb = rgl::unpack_color(&vertices[i].color);
        rgb.x *= f;
        rgb.y *= f;
        rgb.z *= f;
        v.color = rgl::pack_color(&rgb);
    }
    slots[0].f28 = 1;
}

// @ 0x4fbfa0
void ItemA::init_screen()
{
    g_gui.reset();
    g_sound_mgr.stop_all();

    MenuRec* rec = &g_menus[g_game->current_menu];
    bool ok = true;
    int color = (int)(rec->flags >> 32) | 0xff000000;
    g_game->mode = 0;
    g_game->encounter_flags = 0;
    rec = &g_menus[g_game->current_menu];
    g_game->background_mesh = rec->mesh;                   // the game menu's mesh override (+0x1a210)
    g_game->menu_tableau = -1;
    g_menus[g_game->current_menu].ops.play(8);
    if (g_game->mode != 0)
        ok = false;

    for (int i = 0; i < 16; i++)
        kids[i] = 0;

    win = hud_frame();
    frame()->move_a(V2(-0.001f, -0.001f));
    frame()->set_xy(V2(1.002f, 0.752f));

    g_game->background_presentation = -1;

    fill = new MissionUi;
    fill->move_a(V2(0.1f, 0.1f));
    fill->set_xy(V2(1e-05f, 1e-05f));
    fill->hide();
    frame()->add_child(fill);

    if (ok) {
        if (g_game->background_mesh.differs("none")) {
            rgl::mesh* src = g_resmgr.get_object((const rgl::string*)&g_game->background_mesh);
            if (src) {
                rgl::mesh* m = src->clone();
                rec = &g_menus[g_game->current_menu];
                if ((rec->flags & 0x1000) != 0) {
                    float ar = g_gui.view_a / g_gui.view_b;
                    if (ar > 1.35f) {
                        float f = 1.33f / ar;
                        vector4 c(f, 1.0f, 1.0f);
                        m->set_color(c);
                        vector4 tr(1.0f - f, 0.0f, 0.0f);
                        m->translate(&tr);
                    }
                }
                frame()->set_track(2, m);
            }
        }
        if (g_game->menu_tableau >= 0) {
            rgl::mesh* m = g_tableaus.make_mesh_from(&g_game->menu_tableau, &g_game->menu_tableau_value);
            if (g_game->menu_tableau_pos.z != 1.0f)
                m->scale(g_game->menu_tableau_pos.z);
            fill->set_track(0, m);
            V2 at;
            at.x = g_game->menu_tableau_pos.x;
            at.y = g_game->menu_tableau_pos.y;
            fill->move_a(at);
            fill->show();
        }
    }

    frame()->set_color(0xffffffff);
    count = 0;

    panel = new MissionUi;
    panel->move_a(V2(0.0f, 0.0f));
    panel->set148(0x710);
    panel->set_xy(V2(1.0f, 0.75f));
    panel->set_color(color);
    {
        tstring name("");
        panel->set_text(name);
    }

    UiD* flag = new UiD;
    flag->f178 = 1;
    flag->move_a(V2(0.0f, 0.0f));
    frame()->add_child(flag);

    tree = new Camera;
    tree->f104 |= 0x20;
    tree->follow[1] = (UiD*)flag;           // +0x484

    ContainerBase* bar = NEW_CONTAINER;
    bar->move_a(V2(0.0f, 0.0f));
    bar->set_xy(panel->rect_max);
    bar->add_child(panel);
    tree->target = (ContainerBase*)bar;                // +0x490
    tree->add_child(bar);
    tree->move_a(V2(0.0f, 0.0f));
    {
        Camera* t = tree;
        V2 at(0.01f, 0.38f);
        t->move_a(at);
        t->update();
    }
    tree->scroll_refresh(V2(0.65f, 0.31f));
    frame()->add_child(tree);

    ContainerBase* value = NEW_CONTAINER;
    value->move_a(V2(0.0f, 0.0f));
    value->set_xy(V2(0.75f, 0.36f));
    UiD* track_made = make_scroll_track(win, 0.7f, 0.02f, 0.01f, 0.36f);
    Camera* made;
    made = make_quest_list((ContainerBase*)value, V2(0.01f, 0.02f), V2(0.75f, 0.36f), track_made);
    slider = made;
    frame()->add_child(made);

    for (int i = 0; i < 16; i++) {
        UiA* child = new UiA;
        kids[i] = child;
        kids[i]->set_hilight_color(0xff55ff50);
        kids[i]->set_xy(V2(0.64f, 0.04f));
        kids[i]->set_color(color);
        kids[i]->hide();
        value->add_child(kids[i]);
    }

    if (ok) {
        s_last();                                   // slot 9: fill the rows (a virtual call, as the original)
        pres.init(win, g_game->background_presentation);
        if (pres.kind < 0)
        {
            tstring n("meeting_window");
            win->set_mesh(0, res_clone(n));
        }
    }
    {
        NodeA* root = frame();
        g_gui.dirty = 1;
        g_gui.objs.push_back(*(NodeA**)&root);
    }
}

// @ 0x4fc810
void Rec58::init_screen()
{
    build(w04 != 0);
}

// the parameter table lookup every tab button position goes through: a missing key yields 0 (inlined at each of its four uses)
static __forceinline float param_value(const char* key)
{
    tstring k(key);
    int found = -1;
    float v = 0.0f;
    for (int i = 0; found == -1; i++) {
        if (i >= g_game_vars.count)
            break;
        if (g_game_vars.entries[i].key.equals(k))
            found = i;
    }
    if (found >= 0) {
        v = (float)atof(g_game_vars.entries[found].value.c_str());
    }
    return v;
}
#define MAKE_TAB(field, name) { tstring n(name); field = (MissionUi*)make_short_button(frame, g_strtable.lookup(n), x, yy, h); }

enum { LOG_HIST = 3 };
static __forceinline int hist_n() { if (g_gui.history.begin() == 0) return 0; return (int)g_gui.history.size(); }
static __forceinline void slider_sync(UiD* flag)
{
    float* pf = &flag->rect_max[flag->f178];
    float value = (*pf - flag->f17c) * ((flag->v168 - flag->f16c) / (flag->f170 - flag->f16c)) - (flag->f17c / *pf) * 5000.0f;
    flag->set_value(value);
}
// @ 0x4fc820
void Rec58::build(bool show)
{
    int count;
    int i;
    g_gui.reset();

    frame = hud_frame();
    frame->move_a(V2(0.0f, 0.0f));
    frame->set_xy(V2(1.0f, 0.75f));
    {
        tstring n("game_log_window");
        frame->set_track(0, res_clone(n));
    }

    log = new MWidget;
    log->set148(0x700);
    log->move_a(V2(0.0f, 0.0f));
    log->set_size(V2(0.02f, 0.02f));
    log->set_color(0xff000000);
    log->set_xy(V2(0.915f, 0.1f));
    log->name.assign("");

    if (show) {
        char delim = '|';
        tstring* arr = g_game->message_log.split(delim, count);
        if (count > 2) {
            int start = count - 300;
            start = start < 0 ? 0 : start;
            for (int i = start; i < count; i += 3) {
                log->name.append(arr[i]);
                log->name.push_back('\t');
                log->name.append(arr[i + 2]);
                log->name.push_back('\n');
            }
            log->name.erase(log->name.len - 1, 1);
        }
        if (arr)
            delete[] arr;
    } else {
        count = hist_n() * 3;
        for (i = 0; i < count; i += 3) {
            log->name.append(g_gui.history[i / 3]);
            log->name.push_back('\n');
        }
        if (log->name.len > 0)
            log->name.erase(log->name.len - 1, 1);
    }
    log->set_text(log->name);

    float y = log->rect_max.y + 0.001f;

    bar = new text_box;              // vptr 0x597050 (0x4fcb33)
    bar->move_a(V2(0.0f, 0.0f));
    bar->set_xy(V2(0.915f, y));
    bar->add_child(log);

    UiD* track = make_scroll_track((ContainerBase*)frame, 0.95f, 0.08f, 0.02f, 0.64f);
    {
        float ymin = y < 0.64f ? y : 0.64f;
        float d = fmax_i(0.64f - y, 0.0f);
        slider = make_quest_list((ContainerBase*)bar, V2(0.03f, d + 0.08f), V2(0.915f, ymin), track);
    }
    frame->add_child(slider);

    float x = param_value("note_button_x");
    float yy = param_value("note_button_y");
    float h = param_value("note_button_h");
    float dx = param_value("note_button_x_a");
    MAKE_TAB(tab_log, "ui_game_log");
    ((UiA*)tab_log)->f184 = 1;
    x += dx;
    MAKE_TAB(tab_recent, "ui_recent_messages");
    ((UiA*)tab_recent)->f184 = 1;
    x += dx;
    MAKE_TAB(tab_quests, "ui_quests");
    ((UiA*)tab_quests)->f184 = 1;
    x += dx;
    MAKE_TAB(tab_troops, "ui_troops2");
    ((UiA*)tab_troops)->f184 = 1;
    x += dx;
    MAKE_TAB(tab_locations, "ui_locations");
    ((UiA*)tab_locations)->f184 = 1;
    x += dx;
    MAKE_TAB(tab_factions, "ui_factions");
    ((UiA*)tab_factions)->f184 = 1;
    { tstring n("ui_return"); tab_return = (MissionUi*)make_short_button(frame, g_strtable.lookup(n), x + dx, yy, h); }

    if (show) {
        tab_log->set15c(1);
        tab_log->flag2_set();
    } else {
        tab_recent->set15c(1);
        tab_recent->flag2_set();
    }

    {
        NodeA* root = frame;
        g_gui.dirty = 1;
        g_gui.objs.push_back(*(NodeA**)&root);
    }

    // the slider's value follows the log's scroll state
    slider_sync((UiD*)((Camera*)slider)->follow[1]);
    ((UiD*)((Camera*)slider)->follow[1])->set_key_flags(0x10000);
}

// ---- the map-text list widget (the tstring array is freed by delete[]: its vector deleting destructor is 0x4fd0e0) -------------------------------------------------------
// @ 0x4fd150
MWidget::MWidget()
{
    cur = -1;
}

// @ 0x4fd1d0
char MWidget::key(unsigned ev)
{
    char r = 0;
    if ((ev & 7) != 0) {
        int old = cur;
        cur = -1;
        for (unsigned i = 0; i < rect_links.size(); i++) {
            const float left = position.x;
            if (rects_a[i].x + left <= g_gui.mouse[0] && position.x + rects_b[i].x > g_gui.mouse[0]
                && rects_a[i].y + position.y <= g_gui.mouse[1] && rects_b[i].y + position.y > g_gui.mouse[1]) {
                cur = rect_links[i];
                break;
            }
        }
        if (cur != old)
            get_xy();
    } else if ((ev & 8) != 0) {
        r = 1;
        for (unsigned i = 0; i < rect_links.size(); i++) {
            const float left = position.x;
            if (left + rects_a[i].x <= g_gui.mouse[0] && rects_b[i].x + position.x > g_gui.mouse[0]
                && rects_a[i].y + position.y <= g_gui.mouse[1] && rects_b[i].y + position.y > g_gui.mouse[1]) {
                set_key_flags(0x10000);
                sel = i;
                break;
            }
        }
    }
    set_key_flags(ev);
    refresh();
    return r;
}

// @ 0x4fd3d0
MWidget::~MWidget()
{
}

// ~vector<NotePage>: kept out of line (MWidget's destructor and Font::layout_map call it); <vector>'s body. The original's copy is
// folded into 0x41f4b0, the shared body of the 4-byte vectors' release (entity.cpp's std::vector<entity*>::_Tidy)
template<> std::vector<NotePage>::~vector()
{
    _Tidy();
}

// 0x4fd490 is std::vector<NotePage>::_Buy (agent5/agent_vec.cpp: the real vector's, instantiated there; MWidget's constructor calls it out of line)

// @ 0x4fd4f0
V2 MWidget::get_xy()
{
    MissionUi* u = this;
    if (u->tracks[4] == 0) {
        rgl::mesh* m = new rgl::mesh;
        {
            tstring n("ui_font");           // destroyed before add_image_a (0x4fd544)
            m->res = g_resmgr.get_material(n);
        }
        this->set_mesh(4, m);
    }
    vec2 r;
    if (u->font) {
        u->tracks[4]->clear_geometry();
        r = u->font->layout_map(&u->name, &(const vector4&)vector4(u->text_offset.x, u->text_offset.y, 0.0f), &u->text_scale.x, u->tracks[4], &(const unsigned&)0xff000000u, &(const unsigned&)0xff0000ffu, &(const unsigned&)0xffff0000u, &u->rect_min.x, &u->rect_max.x, u->f148, cur, &rects_a, &rects_b, &rect_links, &pages, &page_args);
    } else {
        u->tracks[4]->clear_geometry();
        r = g_font->layout_map(&u->name, &(const vector4&)vector4(u->text_offset.x, u->text_offset.y, 0.0f), &u->text_scale.x, u->tracks[4], &(const unsigned&)0xff000000u, &(const unsigned&)0xff0000ffu, &(const unsigned&)0xffff0000u, &u->rect_min.x, &u->rect_max.x, u->f148, cur, &rects_a, &rects_b, &rect_links, &pages, &page_args);
    }
    u->set_color(0xffffffffu);   // +0x98 (0x4fd6df)
    return r;
}

// 0x487b00: UTF-8 decoder (another object in the original; cl gave it a custom register convention: text in EDI, &out_len in EDX, the buffer and
// &capacity on the stack). A static noinline copy reproduces that convention.
static __declspec(noinline) void decode_utf8(const tstring* s, int* out_len, int* buf, const int* maxn)   // 0x487b00
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            int cnt = *out_len;
            if (cnt >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                ch = ((((((unsigned char)s->buf[i] & 7) << 6) | ((unsigned char)s->buf[i+1] & 0x3f)) << 6 | ((unsigned char)s->buf[i+2] & 0x3f)) << 6) | ((unsigned char)s->buf[i+3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                ch = (((((unsigned char)s->buf[i] & 0xf) << 6) | ((unsigned char)s->buf[i+1] & 0x3f)) << 6) | ((unsigned char)s->buf[i+2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                ch = ((((unsigned char)s->buf[i] & 0x1f) << 6) | ((unsigned char)s->buf[i+1] & 0x3f));
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[cnt] = ch;
            *out_len = cnt + 1;
        } while (i < n);
    }
}
static const int g_text_maxn = 0x2000;                             // the decode buffer's capacity, passed by address

static __forceinline bool name_is(tstring& s, const tstring& p) { return s.find(p, 0) == 0; }
// the metrics of character c: the ascii table below 0x200, else the glyph map (the default glyph when c is not in it)
static __forceinline const Glyph* glyph_of(Font& f, int c)
{
    if (c < 0x200)
        return &f.ascii[c];
    GlyphMap::iterator it = f.map.lower_bound(c);
    if (it != f.map.end())
        return &it->second;
    return &f.dflt;
}
// @ 0x4fd700  The map-text row renderer: `this` (the font) in ECX, 17 stack arguments (the first is the caller's 8-byte result block), ret 0x44.
// It lays the text out in rows, breaking at spaces / tabs / newlines, draws every glyph into the mesh and records, for each <tag=value> span,
// the rectangles (rects_a: where it opens, rects_b: where it closes) and the tag indices the widget's click test reads.
V2 Font::layout_map(const tstring* text, const vector4* p3, const float* scale, rgl::mesh* m, const void* col_text, const void* col_link,
                       const void* col_hot, const float* xmin, const float* xmax, unsigned flags, int cur, std::vector<vector4>* rects_a, std::vector<vector4>* rects_b,
                       std::vector<int>* links, std::vector<NotePage>* pages, std::vector<int>* args)
{
    // the five std::vectors are emptied and their blocks freed: VC7.1's clear() is _Tidy() (the original inlines it; the out-of-line
    // ~vector<NotePage>, 0x41f4b0, for the pages)
    rects_a->clear();
    rects_b->clear();
    links->clear();
    pages->clear();
    args->clear();

    vector4 pos = *p3;
    if (flags & 0x400)
        pos.x = *xmin;
    float start_y = pos.y;
    vec2 sc;
    sc[0] = scale[0] / x_ratio * font_scale;                      // the glyph advance scale
    sc[1] = font_scale / y_ratio * scale[1];

    int buf[0x2000];
    int total;
    decode_utf8(text, &total, buf, &g_text_maxn);

    if (!(flags & 0x100) && !(flags & 2)) {                       // shrink the text so the longest line fits
        int zero = 0;
        float w = measure(buf, total, &zero, &total) * sc[0] * 1.001f;
        float avail = *xmax - *xmin;
        if (w > avail && w > 0.0f) {
            float k = avail / w;
            sc[0] *= k;
            sc[1] *= k;
        }
    }

    float widest = *xmin;
    bool in_tag = false;
    int y = 0;
    int tag_index = -1;
    bool tag = false;
    int tag_start = 0;
    int n = total;
    if (n > 0) {
        int align_left = flags & 4;
        float inv_adv = 1.0f / sc[0];
        int search;
        do {
            search = y;
            bool first = true;
            float run_w = 0.0f;
            for (;;) {
                bool stop = false;
                int i = search;
                for (; i < n; i++) {
                    int ch = buf[i];
                    if (ch == 0x20 || ch == 10 || ch == 9) {
                        if (i < n) {
                            if (buf[i] == 10)
                                stop = true;
                            i++;
                        }
                        break;
                    }
                }
                int wend = i;
                float w = measure(buf, n, &y, &wend) * sc[0];
                if (first || w <= *xmax - *xmin) {
                    run_w = w;
                    search = i;
                } else
                    stop = true;
                if (search >= n)
                    stop = true;
                first = false;
                if (stop)
                    break;
            }

            if (align_left)
                pos.x = *xmin;
            else if (flags & 0x10)
                pos.x = (*xmin + *xmax) * 0.5f - run_w * 0.5f;
            else if (flags & 8)
                pos.x = *xmin + *xmax - run_w;

            float xpos = inv_adv * pos.x;
            for (int i2 = y; i2 < search; i2++) {
                if (tag) {
                    if (buf[i2] != 0x3e)                          // '>'
                        continue;
                    in_tag = !in_tag;
                    if (in_tag) {                                 // the opening tag: <name=value>
                        tag_index++;
                        tstring name;
                        for (int e = tag_start + 1; e < i2; e++)
                            name.push_back((char)buf[e]);
                        name.to_lower();
                        if (name_is(name, tstring("trp=")))
                            pages->push_back(NotePage(NotePage::troop));
                        else if (name_is(name, tstring("fac=")))
                            pages->push_back(NotePage(NotePage::faction));
                        else if (name_is(name, tstring("p=")))
                            pages->push_back(NotePage(NotePage::party));
                        else if (name_is(name, tstring("qst=")))
                            pages->push_back(NotePage(NotePage::quest));
                        char eq = '=';
                        int value = atoi(name.substr(name.find(eq, 0) + 1, name.len).c_str());
                        args->push_back(value);
                        rects_a->push_back(pos);
                        links->push_back(tag_index);
                    } else {                                      // the closing tag: the span's end rectangle
                        vector4 t;
                        t.x = pos.x;
                        t.y = font_scale * scale[1] * line_spacing * 0.9f + pos.y;
                        t.z = pos.z;
                        rects_b->push_back(t);
                    }
                    tag = false;
                    continue;
                }

                int ch = buf[i2];
                if (ch == 0x3c) {                                 // '<'
                    tag = true;
                    tag_start = i2;
                } else if (ch != 10) {
                    if (ch == 9) {
                        int tab = (int)((inv_adv * pos.x - xpos) * 4.0f);
                        pos.x = (((float)tab + 1.0f) * 0.25f + xpos) * sc[0];
                    } else if (in_tag) {
                        if (i2 == y) {                            // a span continues on a new row
                            pages->push_back((*pages)[pages->size() - 1]);
                            args->push_back((*args)[args->size() - 1]);
                            rects_a->push_back(pos);
                            links->push_back(tag_index);
                        }
                        const Glyph* g = glyph_of(*this, ch);
                        pos.x += sc[0] * g->f10;
                        draw_glyph(ch, &pos, &sc, m, (const unsigned*)(cur == tag_index ? col_hot : col_link));
                        pos.x += sc[0] * g->f18;
                    } else {
                        const Glyph* g = glyph_of(*this, ch);
                        pos.x += sc[0] * g->f10;
                        draw_glyph(ch, &pos, &sc, m, (const unsigned*)col_text);
                        pos.x += sc[0] * g->f18;
                    }
                }
            }

            if (in_tag) {
                vector4 t;
                t.x = pos.x;
                t.y = font_scale * scale[1] * line_spacing * 0.9f + pos.y;
                t.z = pos.z;
                rects_b->push_back(t);
            }
            if (widest < pos.x)
                widest = pos.x;
            float row = font_scale * scale[1] * line_spacing * 0.9f;
            y = search;
            pos.x = *xmin;
            pos.y -= row;
        } while (search < n);
    }

    m->compute_normals();
    return V2((widest - *xmin) + 0.0001f, start_y - pos.y);
}

// @ 0x4fdff0
template void std::vector<vector4>::push_back(const vector4&);

// @ 0x4fe060
template std::vector<vector4>::iterator std::vector<vector4>::insert(std::vector<vector4>::iterator, const vector4&);
