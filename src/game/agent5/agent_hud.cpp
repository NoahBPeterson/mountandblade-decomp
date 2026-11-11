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
#include "../glyph_lookup.h"      // glyph_lower_bound (0x43b210)
#include "../../rgl/mesh.h"
#include "../../rgl/scene/world.h"      // rgl::world (m30c / m31c: the light listener)
#include "../../rgl/mesh.h"      // mesh::set_part 0x495ba0 (listed directly: the resolution audit reads only the source's own includes)
#include "../agent_obj.h"      // rgl::matrix::rotate_about_u 0x40e770, Agent::set_slot 0x470d80 (listed directly)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
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
    AgentEq_pick_slots((AgentEq*)a);
    refresh_agent(a);
    a->wield[0] = -1;
    a->wield[1] = -1;
    a->on_wield_reset();
    if (a->f8 == f5c && ((*((char*)this + 0x6528) & 1) || ModeFlag::of(this))) {
        a->cycle_slot(0);
        a->cycle_slot(1);
    }
    a->set_slot(0, 0x14);
    a->set_slot(1, 0x14);
}

// @ 0x4f7e00
void Mission::refresh_agent(Agent* a)
{
    ((FxDef*)a->body->skel)->clear_defs();
    int* look = (int*)a->slot_mesh;                         // the 22 dwords at +0x13c: held-item meshes, node lists, display meshes
    int zero = 0;
    for (int p = 0; p < 0x16; p++)
        look[p] = zero;
    build_appearance((rgl::strategic_entity*)a->body, a->f10, a->troop, *(int*)((char*)a + 0x94), &a->f8c, (rgl::lod_mesh2**)look, (int*)((char*)a + 0x240),
                     (rgl::lod_mesh2**)((char*)a + 0x194), (int*)((char*)a + 0x1a4), (rgl::lod_mesh2**)((char*)a + 0x1a8), (EquipSet*)a->pairs, a->f8);
    setup_item_meshes((SkelBodyV*)a->body, a->f10, a->troop, (char*)look, (int)&a->f130, (ItemRef*)a->pairs, a->f8);
    a->f24c = a->equipment_value();
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
    f2fc = 0;
    if ((float)(rand() % 15817) * 6.322312e-05f < 0.05f)
        g_scene_dirty = 1;
    g_gui.reset();
    g_gui.hud_flag = 1;
    GAME_AT(int, 0x1a1b4) = -1;
    ((std::vector<int>*)((char*)g_game + 0x1a1b8))->resize(0, 0);

    if (refresh_slots) {
        int id = g_mission->player_id;
        if (id >= 0)
            g_mission->init_agent(agent_raw(id));
        refresh_slots = 0;
    } else if (grow_health) {
        char* troop = (char*)GAME_AT(void*, 0x13a00);
        int lvl = *(int*)(troop + 0x58);
        int n = *(unsigned char*)(troop + 0x7a) & 0xf;
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
    f2c8 = -1;
    f2d4 = 0;
    f2e4 = 0;
    f2e8 = 0;
    f2f0 = 0;
    f2d8 = -1;
    f2dc = -1;
    g_pending_action = -1;
    GAME_AT(int, 0x1a1cc) = 0;
    shown_health = -1;
    shield_max = -2;
    ammo_count = -2;
    shield_cur = -1;
    ammo_stage = -1;
    GAME_AT(int, 0x1a090) = 0;

    frame = hud_frame();
    {
        { float v[2]; v[0] = 0.0f; v[1] = 0.0f; frame->move_a(v); }
        { float v[2]; v[0] = 1.0f; v[1] = 0.75f; frame->set_xy(v); }
    }
    {
        MissionUi* root = frame;
        g_gui.dirty = 1;
        g_gui.objs.push_back(*(MissionUi**)&root);
    }
    for (int i = 0; i < 3; i++) {
        w[i] = new MissionUi;
        { float v[2]; v[0] = 0.5f; v[1] = 0.375f; w[i]->move_a(v); }
        {
            { float v[2]; v[0] = 0.03f; v[1] = 0.04f; w[i]->set_xy(v); }
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
        { float v[2]; v[0] = 0.5f; v[1] = 0.35f; w[3 + i]->move_a(v); }
        {
            { float v[2]; v[0] = 0.03f; v[1] = 0.04f; w[3 + i]->set_xy(v); }
        }
        {
            tstring n(i == 0 ? "arrow_up" : (i == 1 ? "arrow_right" : (i == 2 ? "arrow_down" : "arrow_left")));
            marks[3 + i] = res_clone(n);
        }
        w[3 + i]->set_track(0, marks[3 + i]);
        frame->add_child(w[3 + i]);
    }
    {
        { float v[2]; v[0] = 0.5f; v[1] = 0.45f; w[3 + 0]->move_a(v); }
        { float v[2]; v[0] = 0.65f; v[1] = 0.35f; w[3 + 1]->move_a(v); }
        { float v[2]; v[0] = 0.5f; v[1] = 0.3f; w[3 + 2]->move_a(v); }
        { float v[2]; v[0] = 0.35f; v[1] = 0.35f; w[3 + 3]->move_a(v); }
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
        { float v[2]; v[0] = 0.876249969f; v[1] = 0.0243749972f; status_group->move_a(v); }
        { float v[2]; v[0] = 0.17f; v[1] = 0.1175f; status_group->set_xy(v); }
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
        { float v[2]; v[0] = 0.0f; v[1] = 0.0f; health_bar->move_a(v); }
        { float v[2]; v[0] = 0.106500007f; v[1] = 0.00500000035f; health_bar->set_xy(v); }
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
        { float v[2]; v[0] = 0.0725000203f; v[1] = 0.05075f; status_bar->move_a(v); }
        { float v[2]; v[0] = 0.0400000028f; v[1] = 0.05f; status_bar->set_xy(v); }
    }
    shadow_mesh = new rgl::mesh;
    status_bar->set_track(0, shadow_mesh);
    shield_mesh = new rgl::mesh;
    status_bar->set_track(2, shield_mesh);
    shield_icon = new MissionUi;
    {
        { float v[2]; v[0] = 0.0249999985f; v[1] = 0.0500000082f; shield_icon->move_a(v); }
        { float v[2]; v[0] = 0.05f; v[1] = 0.0600000024f; shield_icon->set_xy(v); }
    }
    ui_font = new MissionUi;
    {
        { float v[2]; v[0] = 0.0718750283f; v[1] = 0.045f; ui_font->move_a(v); }
        { float v[2]; v[0] = 0.120000005f; v[1] = 0.024f; ui_font->set_xy(v); }
        { int two = 2; ui_font->set148(&two); }
    }
    {
        tstring n("0");
                float out[2]; ui_font->set_text(out, &n);
    }
    {
        tstring n("ui_font");
        ui_font->tracks[4]->res = g_resmgr.get_material(n);
    }
    {
        unsigned col = 0xffffffff;
        ui_font->set_target(&col);
    }
    ui_font_bg = new MissionUi;
    {
        { float v[2]; v[0] = 0.0718750283f; v[1] = 0.045f; ui_font_bg->move_a(v); }
        { float v[2]; v[0] = 0.120000005f; v[1] = 0.024f; ui_font_bg->set_xy(v); }
        { int two = 2; ui_font_bg->set148(&two); }
    }
    {
        tstring n("0");
                float out[2]; ui_font_bg->set_text(out, &n);
    }
    {
        tstring n("ui_font_background");
        ui_font_bg->tracks[4]->res = g_resmgr.get_material(n);
    }
    {
        unsigned col = 0xff000000;
        ui_font_bg->set_target(&col);
    }
    status_group->add_child(status_bar);
    status_group->add_child(health_bar);
    status_group->add_child(shield_icon);
    status_group->add_child(ui_font);
    status_group->add_child(ui_font_bg);
    frame->kids.push_back(status_group);

    name_widget = new MapWidget;
    name_widget->world = g_mission->scene;
    {
        { float v[2]; v[0] = 0.0f; v[1] = 0.0f; name_widget->move_a(v); }
        { float v[2]; v[0] = 1.0f; v[1] = 0.75f; name_widget->set_xy(v); }
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
        if (id >= 0 && agent_raw(id)->f2c >= 0) {
            f27c = 0.1f;
            f280 = 2.4f;
        } else {
            f27c = 0.0f;
            f280 = 1.65f;
        }
    }
    f270 = 0;
    f274 = 0;
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
    if (slot_mesh[0]) ((rgl::lod_mesh*)slot_mesh[0])->mask = 0xffff;
    if (slot_mesh[1]) ((rgl::lod_mesh*)slot_mesh[1])->mask = 0xffff;
    if (slot_mesh[2]) ((rgl::lod_mesh*)slot_mesh[2])->mask = 0xffff;
    if (slot_mesh[3]) ((rgl::lod_mesh*)slot_mesh[3])->mask = 0xffff;
    if (slot_mesh4) ((rgl::lod_mesh*)slot_mesh4)->mask = 0xffff;
}

// @ 0x4f8c20
void Agent::show()
{
    if (slot_mesh[0]) ((rgl::lod_mesh*)slot_mesh[0])->mask = 0x2000;
    if (slot_mesh[1]) ((rgl::lod_mesh*)slot_mesh[1])->mask = 0x2000;
    if (slot_mesh[2]) ((rgl::lod_mesh*)slot_mesh[2])->mask = 0x2000;
    if (slot_mesh[3]) ((rgl::lod_mesh*)slot_mesh[3])->mask = 0x2000;
    if (slot_mesh4) ((rgl::lod_mesh*)slot_mesh4)->mask = 0x2000;
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
inline __declspec(noinline) void rgl::matrix::rotate_about_s(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 nf, nu;
    nf = vis_add(vis_mul(f, cos((double)*angle)), vis_mul(u, sn));
    nu = vis_sub(vis_mul(u, cos((double)*angle)), vis_mul(f, sn));
    u = nu;
    f = nf;
}
inline __declspec(noinline) void rgl::matrix::rotate_about_f(const float* angle)
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
inline vector4 rgl::cross(const vector4& a, const vector4& b) { return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
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
#define gcam GAME_AT(vector4, 0x1a1a4)
#define geye GAME_AT(vector4, 0x1a13c)
// @ 0x4f8c80
void MissionBlocker::update_camera(float dt)
{
    float radius, zoomL;

    if (GAME_AT(int, 0x1a090) != 0)
        dt = g_step_tbl[0];
    {
        matrix home;
        *(matrix*)&k = *(matrix*)identity_pos((pos*)&home);
    }
    radius = 0.03f;
    if (g_cfg.use_wbuffer != 0)
        radius = 0.01f;
    zoomL = zoom;
    if (GAME_AT(int, 0x1a090) != 0) {
        if ((float)(g_ticks[GAME_AT(int, 0x1a150)] - GAME_AT(__int64, 0x1a158)) * 1e-5f < 0.0f) {
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
    if (GAME_AT(int, 0x1a090) != 0) {
        // free camera: keys move the game's camera position along the camera's own axes
        float t, u;
        rot_s(k, 1.5707964f);
        k.rotate_about_f(&yaw);
        k.rotate_about_s(&pitch);
        k.rotate_about_f(&f284);
        f300 = (1.0f - (dt + dt)) * f300;
        if (f300 < 0.0f)
            f300 = 0.0f;
        else if (f300 > 20.0f)
            f300 = 20.0f;
        t = f300;
        if (g_keyboard.down[0x11] == 0 && g_keyboard.prev[0x11] != 0) {
            int now = (int)g_ticks[0];
            if (now - GAME_AT(int, 0x1a0b4) < 35000) {
                int step = GAME_AT(int, 0x1a0b0) * 2;
                if (step >= 0x20)
                    step = 0x20;
                GAME_AT(int, 0x1a0b0) = step;
            } else {
                GAME_AT(int, 0x1a0b0) = 1;
            }
            GAME_AT(int, 0x1a0b4) = now;
        }
        if (g_keyboard.down[0x11] != 0) {
            u = (float)GAME_AT(int, 0x1a0b0) * t;
            hv_sub_to(gcam, ((k.u * u) * dt));
            f300 = dt * 25.0f + f300;
        }
        if (g_keyboard.down[0x1f] != 0) {
            hv_add_to(gcam, ((k.u * t) * dt));
            f300 = dt * 25.0f + f300;
        }
        if (g_keyboard.down[0x1e] != 0) {
            hv_sub_to(gcam, ((k.s * t) * dt));
            f300 = dt * 17.5f + f300;
        }
        if (g_keyboard.down[0x20] != 0) {
            hv_add_to(gcam, ((k.s * t) * dt));
            f300 = dt * 17.5f + f300;
        }
        if (g_keyboard.down[0x21] != 0) {
            gcam.z = t * dt * 0.5f + gcam.z;
            f300 = dt * 17.5f + f300;
        }
        if (g_keyboard.down[0x2e] != 0 && g_keyboard.down[0x1d] == 0 && g_keyboard.down[0x9d] == 0) {
            gcam.z = gcam.z - t * dt * 0.5f;
            f300 = dt * 17.5f + f300;
        }
        k.o = gcam;
    } else if (CAMSLOT.rec != 0) {
        // the scripted camera slot: cur -> sway over value seconds
        if (CAMSLOT.value > 0.0f) {
            float t = (((float)(g_ticks[CAMSLOT.clock] - CAMSLOT.start) * 1e-5f) / CAMSLOT.value);
            if (CAMSLOT.num > 0) {
                t = atan((t - 0.5f) * (float)CAMSLOT.num * 3.1415927f) / (atan((float)CAMSLOT.num * 1.5707964f) * 2.0f) + 0.5f;
            }
            if (t >= 1.0f) {
                CAMSLOT.cur = CAMSLOT.sway;
                CAMSLOT.zoom_cur = CAMSLOT.zoom_to;
                k.u = hv_neg(CAMSLOT.cur.f);
                k.f = CAMSLOT.cur.u;
                k.s = CAMSLOT.cur.s;
                k.o = CAMSLOT.cur.o;
                zoomL = CAMSLOT.zoom_to;
                CAMSLOT.value = 0.0f;
            } else {
k.o = hv_add(hv_mulv(CAMSLOT.sway.o, t), hv_mulv(CAMSLOT.cur.o, 1.0f - t));
                k.u = hv_neg(hv_add(hv_mulv(CAMSLOT.sway.f, t), hv_mulv(CAMSLOT.cur.f, 1.0f - t)));
                k.f = hv_add(hv_mulv(CAMSLOT.sway.u, t), hv_mulv(CAMSLOT.cur.u, 1.0f - t));
                k.s = hv_add(hv_mulv(CAMSLOT.sway.s, t), hv_mulv(CAMSLOT.cur.s, 1.0f - t));
                zoomL = CAMSLOT.zoom_cur * (1.0f - t) + CAMSLOT.zoom_to * t;
            }
        } else {
            k.u = hv_neg(CAMSLOT.cur.f);
            k.f = CAMSLOT.cur.u;
            k.s = CAMSLOT.cur.s;
            k.o = CAMSLOT.cur.o;
            zoomL = CAMSLOT.zoom_cur;
        }
        if (CAMSLOT.agent >= 0) {
            if (CAMSLOT.attached != 0)
                *(matrix*)&k = AG(CAMSLOT.agent)->body->m * k;
            else
                hv_addr(k.o, AG(CAMSLOT.agent)->body->m.o);
        }
        k.orthonormalize();
    } else {
      {
        if (g_mission->player_id >= 0 && g_game->f139e4 != 0 && AG(g_mission->player_id)->f10 == 0) {
            // spectating: look through the agent this one is bound to
            Agent* ag = AG(g_mission->player_id);
            AgentBody* body = AG(g_mission->player_id)->body;
            Skeleton* sk;
            if (AG(g_mission->player_id)->f2c >= 0)
                yaw = dt * *(float*)((char*)AG(ag->f2c) + 0x56e8) + yaw;
            sk = (Skeleton*)body->skel;
            if ((ag->defs->defs[ag->anim[0].cur].flags & 0x100000) != 0) {
                if (!sk->ready)
                    sk->ensure_updated();
                *(matrix*)&k = body->m * *(matrix*)(sk->recs + 0x1140);
                rot_f(k, 3.1415927f);
                hv_add_to(k.o, (k.f * 0.15f));
            } else {
                if (!sk->ready)
                    sk->ensure_updated();
                vector4 n = *(vector4*)(sk->recs + 0x1170);
                vector4 c = n - lag;
                float hi = 1.0f;
                float lo = 0.0f;
                float l = c.out_length();
                float x = l * dt * 200.0f;
                hv_add_to(lag, hv_m6(c, clamp_v(x, lo, hi)));
                { vector4 tp = body->m.transform_point(lag);
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
                if (AG(g_mission->player_id)->f2c >= 0) {
                    fa = 1.1f;
                    fb = 1.0f;
                }
                f280 = (fa - f280) * dt * 3.0f + f280;
                f27c = (fb - f27c) * dt * 3.0f + f27c;
            } else {
                float h;
                if (AG(g_mission->player_id)->f2c >= 0) {
                    f27c = (0.1f - f27c) * dt + f27c;
                    h = 2.4f;
                } else {
                    f27c = (0.0f - f27c) * dt + f27c;
                    h = 1.65f;
                }
                f280 = (h - f280) * dt + f280;
            }
            head = ag->pos;
            head.z += f280;
            hv_add_mix(ag->v98, head);
            rot_s(k, 1.5707964f);
            k.rotate_about_f(&yaw);
            k.rotate_about_s(&pitch);
            k.rotate_about_f(&f284);
            k.o = head;
            float d = clamp_i(pitch, -3.0f, 3.0f) * 0.0f + f278 + f27c;
            hv_add_to(k.o, (k.u * d));
            hv_add_to(k.o, ((k.f * 0.37f) * d));
            if (GAME_AT(int, 0x1a090) == 0) {
                // keep the camera out of the walls: sweep a thin capsule from the head to the camera
                shape_base* list[0x2000];
                contact ct;
                int count, i;
                float t;
                vector4 q, c;
                char capbuf[sizeof(capsule)];
                capsule& cap = *(capsule*)capbuf;
                {
                vector4 p0 = k.o;
                hv_sub_of(q, k.o, (k.u * d));
                hv_sub_of(c, q, p0);
                c.normalize();
                vector4 a = (p0 - (c * 0.4f));
                cap.capsule::capsule(1e-5f, &q, &a);
                g_mission->scene->query_segment(&p0.x, &q.x, 0x110, list, &count);
                }
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
                f2ec = (d2 - f2ec) * dt * 11.0f + f2ec;
                { vector4 r = k.u * f2ec; k.o = vector4(r.x + q.x, r.y + q.y, r.z + q.z); }
            }
        }
      }
    }
    if (CAMSLOT.rec == 0) {
        if (CAMSLOT.value > 0.0f) {
            float t = ((float)(g_ticks[CAMSLOT.clock] - CAMSLOT.start) * 1e-5f) / CAMSLOT.value;
            if (CAMSLOT.num > 0) {
                t = atan((t - 0.5f) * (float)CAMSLOT.num * 3.1415927f) / (atan((float)CAMSLOT.num * 1.5707964f) * 2.0f) + 0.5f;
            }
            if (t >= 1.0f) {
                CAMSLOT.value = 0.0f;
            } else {
                matrix m;
                m.u = hv_neg(CAMSLOT.sway.f);
                m.f = CAMSLOT.sway.u;
                m.s = CAMSLOT.sway.s;
                m.o = CAMSLOT.sway.o;
                if (CAMSLOT.agent >= 0) {
                    if (CAMSLOT.attached != 0) {
                        m = AG(CAMSLOT.agent)->body->m * m;
                    } else {
                        vector4* bo = &AG(CAMSLOT.agent)->body->m.o;
                        m.o.x = m.o.x + bo->x;
                        m.o.y = m.o.y + bo->y;
                        m.o.z = m.o.z + bo->z;
                    }
                }
                k.o = hv_add(hv_mulv(k.o, t), hv_scaled(m.o, 1.0f - t));
                k.u = hv_add(hv_mulv(k.u, t), hv_scaled(m.u, 1.0f - t));
                k.f = hv_add(hv_mulv(k.f, t), hv_scaled(m.f, 1.0f - t));
                k.s = hv_add(hv_mulv(k.s, t), hv_scaled(m.s, 1.0f - t));
                zoomL = (1.0f - t) * CAMSLOT.zoom_to + t * zoomL;
            }
            k.orthonormalize();
        }
        if (CAMSLOT.rec == 0) {
            CAMSLOT.cur.f = hv_neg(k.u);
            CAMSLOT.cur.u = k.f;
            CAMSLOT.cur.s = hv_neg(k.s);
            CAMSLOT.cur.o = k.o;
            CAMSLOT.zoom_cur = zoomL;
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

#define flare_amt (*(float*)&f65cc)
#define flare_gain f6610
#define cam_row cam

#undef gcam
#undef geye

static __forceinline float dot3(const vector4& a, const vector4& b) { return a.y * b.y + a.z * b.z + a.x * b.x; }
// @ 0x4fa790
void Mission::camera_to(const matrix* eye, const float* far_plane)
{
    view = *(const rgl::matrix*)eye;
    apply_sun_light(&eye->o, far_plane);
    ((rgl::world*)scene)->m30c = hv_neg(eye->u);
    ((rgl::world*)scene)->m31c = eye->o;
    matrix lis = *eye;
    hv_sub_to(lis.o, 0.4f * lis.u);
    if (g_cfg.disable_sound == 0) {
        g_sound_mgr.frame = lis;
        // FMOD is y-up: y and z swap (0x4fa887..0x4fa8ae push f.x, f.z, f.y, u.x, u.z, u.y)
        FSOUND_3D_Listener_SetAttributes(&g_sound_mgr.frame.o.x, 0, g_sound_mgr.frame.f.x, g_sound_mgr.frame.f.z, g_sound_mgr.frame.f.y, g_sound_mgr.frame.u.x,
                                         g_sound_mgr.frame.u.z, g_sound_mgr.frame.u.y);
    }
#define sun ((FrameNode*)f65b8)
    if (f65b8) {
        sun->pos.o = eye->o;
        sun->pos.f = hv_neg(eye->u);
        sun->pos.u = vector4(0.0f, 0.0f, 1.0f);
        sun->pos.s = cross(sun->pos.f, sun->pos.u);
        sun->pos.s.normalize();
        sun->pos.f = cross(sun->pos.u, sun->pos.s);
#undef sun
        if (f6664 < 0 && fog_dense == 1)
            f6664 = g_sound_mgr.play_slot(8, 3, &g_sound_mgr.frame.o.x, 1.0f, 0);
    }
#define flare ((FrameNode*)f65c0)
#define fm ((rgl::mesh*)f65c4)
#define CL01(E) ((E) < 0.0f ? 0.0f : ((E) >= 1.0f ? 1.0f : (E)))
    if (f65c0) {          // (no flare: the original jumps to the particle update at 0x4faea5, it does not return)
    float tod = g_game->time_of_day();
    vector4 dir = g_env_rows[cam_row].get_dir((int)timer_secs((const Timer*)&g_game->f13b18) % 24, hud_day(tod));
    flare->pos.o = eye->o - dir * 500.0f;
    flare->pos.f = dir;
    flare->pos.u = vector4(0.0f, 0.0f, 1.0f);
    flare->pos.orthonormalize();
    float vis;
    if (!g_shadow_map.sample(&eye->o, &vis)) {
        vector4 to = eye->o - dir * 500.0f;
        int hit = ((rgl::world*)scene)->trace_ray(&eye->o.x, &to.x, 500.0f, 0, 0);
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
    fm->clear_geometry();
    {
        tstring name("sample_flare");
        matrix id;
        const rgl::mat4* idm = (const rgl::mat4*)identity_pos((pos*)&id);
        rgl::mesh* src = g_resmgr.get_object(&name);
        fm->append_skinned(src, idm, -1, -1, 0);
    }
    {
        tstring name("flare");
        fm->res = g_resmgr.get_material(name);
    }
    float sc2;
    {
        float k = (dot3(dir, eye->u) - 0.3f) * 1.4285715f;
        k = CL01(k);
        sc2 = k * k;
    }
    float in = sc2 * (cl01(1.0f - 150.0f / flare_gain) * g_env_rows[cam_row].size * flare_amt);
    {
        vector4 col(in * 1.1f, in, in * 0.8f);
        fm->color = rgl::pack_color(&col);
    }
    fm->color |= 0xff000000;
    { float a = sc2*sc2; fm->scale((a * 5.0f + 2.0f) * g_env_rows[cam_row].size * 50.0f); }
    }
#undef flare
#undef fm
#undef CL01
    if (g_game->levels[f5df4].flags & 0x100)
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
    rgl::frustum* lf = (rgl::frustum*)((char*)cam + 0x10c);
    vector4 clip = lf->to_clip(*p);
    if (!(0.0f <= clip.x && clip.x < 1.0f && 0.0f <= clip.y && clip.y < 1.0f))
        return true;
    int zero_x = 0;
    struct { float x, y; } fp = { (float)scn.f8 * clip.x, (1.0f - clip.y) * (float)scn.fc };
    *out = 0.0f;
    int px = (int)fp.x;
    int zero_y = 0;
    int py = (int)fp.y;
    float depth = clip.z - 0.001f; float*& oref = out;
    for (int i = 0; i < 9; i++) {
        int xv = tx[i] + px;
        int yv = ty[i] + py;
        float a, b;
        read_pixel((int)g_render.dev, clamp_index(&xv, &zero_x, &scn.f8),
                   clamp_index(&yv, &zero_y, &scn.fc), (unsigned*)&a, (unsigned*)&b);
        if (!(a < depth))
            *out += w[i];
    }
    *oref = *oref * 0.13513514f;
    return ret;
}

struct ViewLight { char pad0[4]; AgentTable tbl; char pad1c[0x5c - 4 - sizeof(AgentTable)]; int local_id; };
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
    ViewLight* vl = (ViewLight*)this;
    AgentTable* tbl = (AgentTable*)((char*)this + 4);
    int n = *(int*)tbl;
    unsigned i = 0;
    if (0 < n) {
        do {
            if (tbl->get(i)->used)
                break;
            i++;
        } while ((int)i < n);
    }
    if (i < *(unsigned*)tbl) {
        do {
            Agent* a = ((AgentTable*)((char*)this + 4))->pages[i >> 4] + (i & 0xf);
            if (a->body) {
                float intensity;
                const vector4& o = ((SunBody*)a->body)->origin;
                float dx = o.x - pos->x;
                float dy = o.y - pos->y;
                float dz = o.z - pos->z;
                float d = (float)sqrt(dy * dy + (dx * dx + dz * dz));
                float dist = 3.5f / (d > 3.5f ? d : 3.5f);
                if (!(d > 1.0f))
                    d = 1.0f;
                intensity = (1.0f / d) * size;
                ((SunBody*)a->body)->set_intensity(&intensity);
                if (a->f8 == vl->local_id || a->f30 == vl->local_id) {
                    float one = 1.0f;
                    ((PartyModel*)a->body)->set_fade(&one);
                } else {
                    float v = dist * range;
                    ((PartyModel*)a->body)->set_fade(&v);
                }
            }
            int n2 = *(int*)tbl;
            for (i++; (int)i < n2; i++) {
                if (tbl->get(i)->used)
                    break;
            }
        } while (i < *(unsigned*)tbl);
    }
}


static __forceinline rgl::mesh* item_mesh_v(const ItemRef* r, const float& s) { return make_item_mesh(r, &s); }
static __forceinline void offset_v(rgl::mesh* m, const float& v) { m->set_offset(*(const vector4*)&v); }
// @ 0x4fb350
void MissionBlocker::update_status()
{
    int id = g_mission->player_id;
    if (id < 0)
        return;
    Agent* ag = agent_by_id(id);
    if (GAME_AT(int, 0x1a20c))
        status_group->hide();
    else
        status_group->show();
    matrix frame;
    int hp = (int)ag->health;
    if (hp != shown_health) {
        shown_health = hp;
        health_mesh->clear_geometry();
        {
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
        float tsz[2];
        sprintf(buf, "%d", la);
        {
            tstring t1(buf);
            ui_font->set_text(tsz, &t1);
        }
        {
            tstring t2(buf);
            ui_font_bg->set_text(tsz, &t2);
        }
        if (shield_max < 0) {
            status_bar->hide();
            ui_font->hide();
            ui_font_bg->hide();
        } else {
            status_bar->show();
            ui_font->show();
            ui_font_bg->show();
            if (shield_max > 0) {
                unsigned c = 0xffffffff;
                ui_font->set_color(&c);
            } else {
                unsigned c = 0xffb02020;
                ui_font->set_color(&c);
            }
        }
        }
        if (res.item != shield_cur) {
            shield_mesh->clear_geometry();
            shadow_mesh->clear_geometry();
            if (res.item >= 0) {
                float piece_scale = 0.055f; rgl::mesh* piece = make_item_mesh(&res, &piece_scale);
                if (piece) {
                    offset_v(piece, 0.0005f);
                    identity3((rgl::mat3*)&frame);
                    rot_u((rgl::matrix&)frame, -1.45f);
                    piece->set_part(frame);
                    identity_pos((pos*)&frame);
                    frame.o.z -= 10.0f;
                    shield_mesh->append_skinned(piece, (const rgl::mat4*)&frame, -1, -1, 0);
                    frame.o.x += 0.0012f;
                    frame.o.y -= 0.001f;
                    frame.o.z -= 10.0f;
                    shield_mesh->res = piece->res;
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
    for (unsigned i = 0; i < v50.size(); i++) {
        int idx = v50[i].index;
        hv_add_to(v70[idx].a, hv_scaled(v50[i].normal, pos.x));
        hv_add_to(v70[idx].b, hv_scaled(v50[i].normal, pos.x));
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
        (result = (rgl::mesh*)g_items[ref->item].mesh_for(0x1000000000000000ULL, (int)*(signed char*)((const char*)ref + 7))) != 0) {
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
        size = (float)g_items[ref->item].weight * 0.00625f + 0.3f;
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
    for (unsigned i = 0; i < v50.size(); i++) {
        rgl::vertex60& v = v50[i];
        float f = (v.normal.z + 0.25f) * 0.8f;
        vector4 rgb = rgl::unpack_color(&v50[i].color);
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

    char* rec = (char*)g_menus + GAME_AT(int, 0x13aec) * 0x60;
    bool ok = true;
    int color = (int)(*(__int64*)(rec + 0x20) >> 32) | 0xff000000;
    GAME_AT(int, 0xc8) = 0;
    GAME_AT(int, 0xa8) = 0;
    rec = (char*)g_menus + GAME_AT(int, 0x13aec) * 0x60;
    struct MenuMesh { char pad0[0x28]; tstring mesh; };   // menu_rec.h's MenuRec, of which this path uses +0x28 only
    g_game->mesh1a210 = ((MenuMesh*)rec)->mesh;      // the game menu's mesh override (+0x1a210)
    GAME_AT(int, 0x1a220) = -1;
    ((DialogBlock*)((char*)g_menus + GAME_AT(int, 0x13aec) * 0x60 + 0x38))->play(8);
    if (GAME_AT(int, 0xc8) != 0)
        ok = false;

    for (int i = 0; i < 16; i++)
        kids[i] = 0;

    win = hud_frame();
    { float v[2]; v[0] = -0.001f; v[1] = -0.001f; frame()->move_a(v); }
    { float v[2]; v[0] = 1.002f; v[1] = 0.752f; frame()->set_xy(v); }

    GAME_AT(int, 0xac) = -1;

    fill = new MissionUi;
    { float v[2]; v[0] = 0.1f; v[1] = 0.1f; fill->move_a(v); }
    { float v[2]; v[0] = 1e-05f; v[1] = 1e-05f; fill->set_xy(v); }
    fill->hide();
    frame()->add_child(fill);

    if (ok) {
        if (((tstring*)((char*)g_game + 0x1a210))->differs("none")) {
            rgl::mesh* src = g_resmgr.get_object((const rgl::string*)((char*)g_game + 0x1a210));
            if (src) {
                rgl::mesh* m = src->clone();
                rec = (char*)g_menus + GAME_AT(int, 0x13aec) * 0x60;
                if ((*(__int64*)(rec + 0x20) & 0x1000) != 0) {
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
        if (GAME_AT(int, 0x1a220) >= 0) {
            rgl::mesh* m = g_tableaus.make_mesh_from(&GAME_AT(int, 0x1a220), &GAME_AT(int, 0x1a224));
            if (GAME_AT(float, 0x1a230) != 1.0f)
                m->scale(GAME_AT(float, 0x1a230));
            fill->set_track(0, m);
            { float v[2]; v[0] = GAME_AT(float, 0x1a228); v[1] = GAME_AT(float, 0x1a22c); fill->move_a(v); }
            fill->show();
        }
    }

    {
        unsigned c = 0xffffffff;
        frame()->set_color(&c);
    }
    count = 0;

    panel = new MissionUi;
    { float v[2]; v[0] = 0.0f; v[1] = 0.0f; panel->move_a(v); }
    {
        int two = 0x710;
        panel->set148(&two);
    }
    { float v[2]; v[0] = 1.0f; v[1] = 0.75f; panel->set_xy(v); }
    {
        unsigned c = color;
        panel->set_color(&c);
    }
    {
        tstring name("");
        float out[2];
        panel->set_text(out, &name);
    }

    UiD* flag = new UiD;
    flag->f178 = 1;
    { float v[2]; v[0] = 0.0f; v[1] = 0.0f; flag->move_a(v); }
    frame()->add_child(flag);

    tree = new Camera;
    tree->f104 |= 0x20;
    tree->follow[1] = (UiD*)flag;           // +0x484

    ContainerBase* bar = NEW_CONTAINER;
    { float v[2]; v[0] = 0.0f; v[1] = 0.0f; bar->move_a(v); }
    bar->set_xy(&panel->screen[0]);
    bar->add_child(panel);
    tree->target = (ContainerBase*)bar;                // +0x490
    tree->add_child(bar);
    { float v[2]; v[0] = 0.0f; v[1] = 0.0f; tree->move_a(v); }
    {
        float v[2];
        Camera* t = tree;
        v[0] = 0.01f;
        v[1] = 0.38f;
        t->move_a(v);
        ((FieldPanel*)t)->update();
    }
    { float v[2]; v[0] = 0.65f; v[1] = 0.31f; ((HudOuter*)tree)->scroll_refresh((vector4*)v); }
    frame()->add_child(tree);

    ContainerBase* value = NEW_CONTAINER;
    { float v[2]; v[0] = 0.0f; v[1] = 0.0f; value->move_a(v); }
    { float v[2]; v[0] = 0.75f; v[1] = 0.36f; value->set_xy(v); }
    UiD* track_made = make_scroll_track(win, 0.7f, 0.02f, 0.01f, 0.36f);
    Camera* made;
    { float a[2], b[2];
    b[0] = 0.75f;
    b[1] = 0.36f;
    a[0] = 0.01f;
    a[1] = 0.02f;
    made = make_quest_list((ContainerBase*)value, a, b, track_made); }
    slider = made;
    frame()->add_child(made);

    for (int i = 0; i < 16; i++) {
        UiA* child = new UiA;
        kids[i] = child;
        { unsigned c = 0xff55ff50; kids[i]->set_target(&c); }
        float pos[2] = { 0.64f, 0.04f };
        kids[i]->set_xy(pos);
        { unsigned c = color; kids[i]->set_color(&c); }
        kids[i]->hide();
        value->add_child(kids[i]);
    }

    if (ok) {
        ((ScreenOps*)this)->begin();
        ((BlockOwner*)((char*)this + 0x30))->init(win, GAME_AT(int, 0xac));
        if (f40 < 0)
        {
            tstring n("meeting_window");
            win->set_mesh(0, res_clone(n));
        }
    }
    {
        MissionUi* root = frame();
        g_gui.dirty = 1;
        g_gui.objs.push_back(*(MissionUi**)&root);
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

static __forceinline int hist_n() { if (g_gui.history.begin() == 0) return 0; return (int)g_gui.history.size(); }
static __forceinline void slider_sync(UiD* flag)
{
    float* pf = &flag->screen[flag->f178];
    float value = (*pf - flag->f17c) * ((flag->v168 - flag->f16c) / (flag->f170 - flag->f16c)) - (flag->f17c / *pf) * 5000.0f;
    flag->set_value(value);
}
// @ 0x4fc820
void Rec58::build(bool show)
{
    int count;
    g_gui.reset();

    frame = hud_frame();
    { float v[2]; v[0] = 0.0f; v[1] = 0.0f; frame->move_a(v); }
    { float v[2]; v[0] = 1.0f; v[1] = 0.75f; frame->set_xy(v); }
    {
        tstring n("game_log_window");
        frame->set_track(0, res_clone(n));
    }

    log = new MWidget;
    {
        int c700 = 0x700;
        log->set148(&c700);
    }
    { float v[2]; v[0] = 0.0f; v[1] = 0.0f; log->move_a(v); }
    { float v[2]; v[0] = 0.02f; v[1] = 0.02f; log->set_size(v); }
    {
        unsigned c = 0xff000000;
        log->set_color(&c);
    }
    { float v[2]; v[0] = 0.915f; v[1] = 0.1f; log->set_xy(v); }
    log->name.assign("");

    if (show) {
        char delim = '|';
        tstring* arr = ((tstring*)((char*)g_game + 0x13b50))->split(delim, count);
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
            ((TStringArray*)arr)->destroy(3);
    } else {
        count = hist_n() * 3;
        for (int i = 0; i < count; i += 3) {
            log->name.append(g_gui.history[i / 3]);
            log->name.push_back('\n');
        }
        if (log->name.len > 0)
            log->name.erase(log->name.len - 1, 1);
    }
    {
        float out[2];
        log->set_text(out, &log->name);
    }

    float y = log->screen[1] + 0.001f;

    bar = new text_box;              // vptr 0x597050 (0x4fcb33)
    { float v[2]; v[0] = 0.0f; v[1] = 0.0f; bar->move_a(v); }
    { float v[2]; v[0] = 0.915f; v[1] = y; bar->set_xy(v); }
    bar->add_child(log);

    UiD* track = make_scroll_track((ContainerBase*)frame, 0.95f, 0.08f, 0.02f, 0.64f);
    {
        float ymin = y < 0.64f ? y : 0.64f;
        float d = fmax_i(0.64f - y, 0.0f);
        float a[2], b[2];
        b[0] = 0.915f;
        b[1] = ymin;
        a[0] = 0.03f;
        a[1] = d + 0.08f;
        slider = make_quest_list((ContainerBase*)bar, a, b, track);
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
        MissionUi* root = frame;
        g_gui.dirty = 1;
        g_gui.objs.push_back(*(MissionUi**)&root);
    }

    // the slider's value follows the log's scroll state
    slider_sync((UiD*)((Camera*)slider)->follow[1]);
    ((UiD*)((Camera*)slider)->follow[1])->set_key_flags(0x10000);
}

// ---- the tstring array and the map-text list widget -------------------------------------------------------
// @ 0x4fd0e0
void* TStringArray::destroy(unsigned flags)
{
    tstring* first = (tstring*)this;
    if (flags & 2) {
        int n = *(int*)((char*)this - 4);
        tstring* p = first + n;
        for (int i = n - 1; i >= 0; i--) {
            --p;
            p->~tstring();
        }
        if (flags & 1)
            ::operator delete[]((char*)this - 4);
        return (char*)this - 4;
    }
    first[0].~tstring();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

static inline void v4list_zero(V4List& l) { l.first = 0; l.last = 0; l.end = 0; }
// @ 0x4fd150
MWidget::MWidget()
{
    v4list_zero(rects_a);
    v4list_zero(rects_b);
    list_c.buy(0);
    list_d.first = 0;
    list_d.last = 0;
    list_d.end = 0;
    list_e.first = 0;
    list_e.last = 0;
    list_e.end = 0;
    cur = -1;
}

// the three lists are VC7.1 vectors (map_list.h keeps the fields other rows name)
#define MW_IDS (*(std::vector<int>*)&list_e)
#define MW_A (*(std::vector<vector4>*)&rects_a)
#define MW_B (*(std::vector<vector4>*)&rects_b)
// @ 0x4fd1d0
char MWidget::key(unsigned ev)
{
    char r = 0;
    if ((ev & 7) != 0) {
        int id[2];                          // +0x94 writes its 8-byte result here (0x4fd2ba reuses the slot)
        id[0] = cur;
        cur = -1;
        for (unsigned i = 0; i < MW_IDS.size(); i++) {
            const float left = fE0;
            if (MW_A[i].x + left <= g_gui.mouse[0] && fE0 + MW_B[i].x > g_gui.mouse[0]
                && MW_A[i].y + fE4 <= g_gui.mouse[1] && MW_B[i].y + fE4 > g_gui.mouse[1]) {
                cur = MW_IDS[i];
                break;
            }
        }
        if (cur != id[0])
            get_xy((float*)id);
    } else if ((ev & 8) != 0) {
        r = 1;
        for (unsigned i = 0; i < MW_IDS.size(); i++) {
            const float left = fE0;
            if (left + MW_A[i].x <= g_gui.mouse[0] && MW_B[i].x + fE0 > g_gui.mouse[0]
                && MW_A[i].y + fE4 <= g_gui.mouse[1] && MW_B[i].y + fE4 > g_gui.mouse[1]) {
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
    list_e.free_block();
    list_d.free_block();
    VEC_CLEAR(&list_c);
    rects_b.free_block();
    rects_a.free_block();
}

// @ 0x4fd490
bool IntList::buy(unsigned n)
{
    first = 0;
    last = 0;
    end = 0;
    if (n == 0)
        return false;
    if (n > 0x3fffffff) {
        xlen();
        return true;
    }
    int* p = (int*)operator new(n * 4);
    end = p + n;
    first = p;
    last = p;
    return true;
}

// @ 0x4fd4f0
void MWidget::get_xy(float* result)
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
    MapText r;
    if (u->f124) {
        u->tracks[4]->clear_geometry();
        MapText t;
        r = *(MapText*)((Font*)u->f124)->layout_map(&t, &u->name, &(const vector4&)vector4(u->f138, u->f13c, 0.0f), &u->f140, u->tracks[4], &(const unsigned&)0xff000000u, &(const unsigned&)0xff0000ffu, &(const unsigned&)0xffff0000u, &u->screen[2], &u->screen[0], u->f148, cur, &rects_a, &rects_b, &list_e, &list_c, &list_d);
    } else {
        u->tracks[4]->clear_geometry();
        MapText t;
        r = *(MapText*)g_font->layout_map(&t, &u->name, &(const vector4&)vector4(u->f138, u->f13c, 0.0f), &u->f140, u->tracks[4], &(const unsigned&)0xff000000u, &(const unsigned&)0xff0000ffu, &(const unsigned&)0xffff0000u, &u->screen[2], &u->screen[0], u->f148, cur, &rects_a, &rects_b, &list_e, &list_c, &list_d);
    }
    u->set_color((unsigned*)&(const unsigned&)0xffffffffu);   // +0x98 (0x4fd6df)
    *(MapText*)result = r;
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

// the inlined vector<int>::push_back
static __forceinline void ints_push(IntList* v, const int& x)
{
    int* p = v->first;
    unsigned n = p == 0 ? 0 : (unsigned)(v->last - p);
    if (p != 0 && n < (unsigned)(v->end - p)) {
        *v->last = x;
        ++v->last;
    } else
        ((VecOps<int>*)v)->insert_n(std::vector<int>::iterator(v->last), 1, x);
}

static __forceinline bool name_is(tstring& s, const tstring& p) { return s.find(p, 0) == 0; }
// @ 0x4fd700  The map-text row renderer: `this` (the font) in ECX, 17 stack arguments (the first is the caller's 8-byte result block), ret 0x44.
// It lays the text out in rows, breaking at spaces / tabs / newlines, draws every glyph into the mesh and records, for each <tag=value> span,
// the rectangles (rects_a: where it opens, rects_b: where it closes) and the tag indices the widget's click test reads.
void* Font::layout_map(void* out, const tstring* text, const vector4* p3, const float* scale, rgl::mesh* m, const void* col_text, const void* col_link,
                       const void* col_hot, const float* xmin, const float* xmax, unsigned flags, int cur, void* v178, void* v188, void* v1b8, void* v198, void* v1a8)
{
    V4List* rects_a = (V4List*)v178;
    V4List* rects_b = (V4List*)v188;
    IntList* tags = (IntList*)v1b8;                               // the tag index of every rectangle
    IntList* spare = (IntList*)v198;
    IntList* codes = (IntList*)v1a8;                              // tag kind and value pairs
    VEC_CLEAR(rects_a);
    VEC_CLEAR(rects_b);
    tags->free_block();
    VEC_CLEAR(spare);
    codes->free_block();

    vector4 pos = *p3;
    if (flags & 0x400)
        pos.x = *xmin;
    float start_y = pos.y;
    float sc[2];
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
                        int code;
                        if (name_is(name, tstring("trp="))) {
                            code = 1;
                            VEC_INT_PUSH_BACK(codes, code);
                        } else if (name_is(name, tstring("fac="))) {
                            code = 2;
                            VEC_INT_PUSH_BACK(codes, code);
                        } else if (name_is(name, tstring("p="))) {
                            code = 3;
                            VEC_INT_PUSH_BACK(codes, code);
                        } else if (name_is(name, tstring("qst="))) {
                            code = 4;
                            VEC_INT_PUSH_BACK(codes, code);
                        }
                        char eq = '=';
                        int value = atoi(name.substr(name.find(eq, 0) + 1, name.len).c_str());
                        tag = false;
                        ints_push(codes, value);
                        rects_a->push_back(pos);
                        ints_push(tags, tag_index);
                    } else {                                      // the closing tag: the span's end rectangle
                        vector4 t;
                        t.x = pos.x;
                        t.y = font_scale * scale[1] * line_spacing * 0.9f + pos.y;
                        t.z = pos.z;
                        rects_b->push_back(t);
                        tag = false;
                    }
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
                            VEC_INT_PUSH_BACK(spare, spare->first[(spare->first == 0 ? 0u : (unsigned)(spare->last - spare->first)) - 1]);
                            VEC_INT_PUSH_BACK(codes, codes->first[(codes->first == 0 ? 0u : (unsigned)(codes->last - codes->first)) - 1]);
                            rects_b->push_back(pos);
                            VEC_INT_PUSH_BACK(tags, tag_index);
                        }
                        const Glyph* g;
                        if (ch >= 0x200) {
                            GlyphNode* node;
                            int key = ch;
                            glyph_lower_bound(map, &node, key);
                            if (node != map.head)
                                g = &node->val;
                            else
                                g = &dflt;
                        } else
                            g = &ascii[ch];
                        pos.x += sc[0] * g->f10;
                        draw_glyph(ch, &pos, sc, m, (int)(cur == tag_index ? col_hot : col_link));
                        pos.x += sc[0] * g->f18;
                    } else {
                        const Glyph* g;
                        if (ch >= 0x200) {
                            GlyphNode* node;
                            int key = ch;
                            glyph_lower_bound(map, &node, key);
                            if (node != map.head)
                                g = &node->val;
                            else
                                g = &dflt;
                        } else
                            g = &ascii[ch];
                        pos.x += sc[0] * g->f10;
                        draw_glyph(ch, &pos, sc, m, (int)col_text);
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
    ((float*)out)[0] = (widest - *xmin) + 0.0001f;
    ((float*)out)[1] = start_y - pos.y;
    return out;
}

static __forceinline unsigned v4_size(const V4List* v) { return v->first == 0 ? 0 : (unsigned)(v->last - v->first); }
static __forceinline unsigned v4_capacity(const V4List* v) { return v->first == 0 ? 0 : (unsigned)(v->end - v->first); }
static __forceinline vector4* v4_ufill(V4List* v, vector4* p, unsigned n, const vector4& val)
{
    uninit_fill_n<vector4>(p, n, val, *(std::allocator<vector4>*)v, std::_Ptr_cat(p, p));
    return p + n;
}
static __forceinline V4List::iterator v4_end(V4List* v) { V4List::iterator r; r.p = v->last; return r; }

// @ 0x4fdff0
void V4List::push_back(const vector4& v)
{
    if (v4_size(this) < v4_capacity(this))
        last = v4_ufill(this, last, 1, v);
    else
        insert(v4_end(this), v);
}

// @ 0x4fe060
V4List::iterator V4List::insert(iterator where, const vector4& val)
{
    unsigned idx = v4_size(this) == 0 ? 0 : (unsigned)(where.p - first);
    ((VecOps<vector4>*)this)->insert_n(std::vector<vector4>::iterator(where.p), 1, val);
    iterator r;
    r.p = first + idx;
    return r;
}
