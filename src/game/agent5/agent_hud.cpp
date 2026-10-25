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
#include <stdlib.h>
#include <stdio.h>
#include "./agent_hud.h"
#include "../gui_mgr.h"           // (listed directly: the resolution audit reads only the source's own includes)
#include "../../rgl/mesh_x.h"
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/sound_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/fx.h"
#include "../../rgl/resource_manager.h"
#include "../../include/renderer.h"
#include "../skill_cap.h"
#pragma intrinsic(tan, atan2, atan, sqrt)

// ---- the mission's agent (re)initialisation ------------------------------------------------------------------
struct ModeFlag {
    // 0x435840 as the caller inlines it (Mission::mode_flag1 is out of line elsewhere)
    static __forceinline char of(const Mission* m) { return ((g_modes2[m->mode].flags >> 1) & 1) != 0; }
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
    build_appearance(a->body, a->f10, a->troop, *(int*)((char*)a + 0x94), &a->f8c, look, (int*)((char*)a + 0x240),
                     (int*)((char*)a + 0x194), (int*)((char*)a + 0x1a4), (int*)((char*)a + 0x1a8), (int*)a->pairs, a->f8);
    setup_item_meshes((SkelBodyV*)a->body, a->f10, a->troop, (char*)look, (int)&a->f130, (ItemRef*)a->pairs, a->f8);
    a->f24c = a->equipment_value();
    a->set_slot(0, 0x14);
    a->set_slot(1, 0x14);
}

static __forceinline rgl::mesh* res_clone(const tstring& n)
{
    return g_resmgr_full.get_object((const rgl::string*)&n)->clone();
}

// @ 0x4f7f00
void MissionHud::refresh()
{
    float v[2];
    f2fc = 0;
    if ((float)(rand() % 15817) * 6.322312e-05f < 0.05f)
        g_scene_dirty = 1;
    g_gui.reset();
    g_hud_flag = 1;
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
        else if (n > (*(int*)&g_skill_tbl[36].cap))
            n = (*(int*)&g_skill_tbl[36].cap);
        int h = lvl + n * 2 + 0x23;
        if (h > status_height) {
            int id = g_mission->player_id;
            if (id >= 0) {
                Agent* a = agent_raw(id);
                a->health += (float)(h - status_height);
                agent_raw(g_mission->player_id)->max_health = (float)h;
            }
        }
    }

    stamp_b = g_ticks[clock_b] - 10000000u;
    stamp_a = g_ticks[clock_a] - 10000000u;
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

    FrameWidget* fw = new_frame_widget();
    frame = fw;
    {
        v[0] = 0.0f;
        v[1] = 0.0f;
        UI(fw)->set_size(v);
        v[0] = 1.0f;
        v[1] = 0.75f;
        UI(fw)->set_pos(v);
    }
    {
        MissionUi* root = frame;
        g_gui_dirty = 1;
        ((NodeVec*)&g_gui)->push_back(*(NodeA**)&root);
    }
    for (int i = 0; i < 3; i++) {
        MissionUi* w = new MissionUi;
        crosshair[i] = w;
        {
            v[0] = 0.5f;
        v[1] = 0.375f;
            UI(w)->set_size(v);
        }
        {
            v[0] = 0.03f;
        v[1] = 0.04f;
            UI(crosshair[i])->set_pos(v);
        }
        crosshair_mesh[i] = res_clone(tstring(i == 0 ? "crosshair_up" : (i == 1 ? "crosshair_left" : "crosshair_right")));
        UI(crosshair[i])->add_image(0, crosshair_mesh[i]);
        UI(frame)->add_child(UI(crosshair[i]));
    }
    for (int i = 0; i < 4; i++) {
        MissionUi* w = new MissionUi;
        arrow[i] = w;
        {
            v[0] = 0.5f;
        v[1] = 0.35f;
            UI(w)->set_size(v);
        }
        {
            v[0] = 0.03f;
        v[1] = 0.04f;
            UI(arrow[i])->set_pos(v);
        }
        arrow_mesh[i] = res_clone(tstring(i == 0 ? "arrow_up" : (i == 1 ? "arrow_right" : (i == 2 ? "arrow_down" : "arrow_left"))));
        UI(arrow[i])->add_image(0, arrow_mesh[i]);
        UI(frame)->add_child(UI(arrow[i]));
    }
    {
        v[0] = 0.5f;
        v[1] = 0.45f;
        UI(arrow[0])->set_size(v);
        v[0] = 0.65f;
        v[1] = 0.35f;
        UI(arrow[1])->set_size(v);
        v[0] = 0.5f;
        v[1] = 0.3f;
        UI(arrow[2])->set_size(v);
        v[0] = 0.35f;
        v[1] = 0.35f;
        UI(arrow[3])->set_size(v);
    }
    f84[0] = 0;
    f84[1] = 0;
    f84[2] = 0;
    f84[3] = 0;
    f84[4] = 0;
    f84[5] = 0;
    f84[6] = 0;

    // the status background group, its health bar and bar, the two text widgets and the shield icon
    status_group = new GroupWidget;
    {
        v[0] = 0.87625f;
        v[1] = 0.024375f;
        UI(status_group)->set_size(v);
        v[0] = 0.17f;
        v[1] = 0.1175f;
        UI(status_group)->set_pos(v);
    }
    {
        rgl::mesh* m = res_clone(tstring("status_background"));
        UI(status_group)->add_image(0, m);
    }
    health_bar = new MissionUi;
    {
        v[0] = 0.0f;
        v[1] = 0.0f;
        UI(health_bar)->set_size(v);
        v[0] = 0.1065f;
        v[1] = 0.005f;
        UI(health_bar)->set_pos(v);
    }
    {
        health_mesh = res_clone(tstring("status_health_bar"));
        UI(health_bar)->add_image(0, health_mesh);
    }
    status_bar = new MissionUi;
    {
        v[0] = 0.0725f;
        v[1] = 0.05075f;
        UI(status_bar)->set_size(v);
        v[0] = 0.04f;
        v[1] = 0.05f;
        UI(status_bar)->set_pos(v);
    }
    shadow_mesh = new rgl::mesh;
    UI(status_bar)->add_image(0, shadow_mesh);
    shield_mesh = new rgl::mesh;
    UI(status_bar)->add_image(2, shield_mesh);
    shield_icon = new MissionUi;
    {
        v[0] = 0.025f;
        v[1] = 0.05f;
        UI(shield_icon)->set_size(v);
        v[0] = 0.05f;
        v[1] = 0.06f;
        UI(shield_icon)->set_pos(v);
    }
    ui_font = new MissionUi;
    {
        v[0] = 0.071875f;
        v[1] = 0.045f;
        UI(ui_font)->set_size(v);
        v[0] = 0.12f;
        v[1] = 0.024f;
        UI(ui_font)->set_pos(v);
        int two = 2;
        UI(ui_font)->set_int(&two);
    }
    {
        tstring n("0");
                UI(ui_font)->set_text(v, &n);
    }
    {
        tstring n("ui_font");
        ui_font->tracks[4]->res = g_resmgr_full.base().get_material(*(rgl::string*)&n);
    }
    {
        unsigned col = 0xffffffff;
        UI(ui_font)->set_color(&col);
    }
    ui_font_bg = new MissionUi;
    {
        v[0] = 0.071875f;
        v[1] = 0.045f;
        UI(ui_font_bg)->set_size(v);
        v[0] = 0.12f;
        v[1] = 0.024f;
        UI(ui_font_bg)->set_pos(v);
        int two = 2;
        UI(ui_font_bg)->set_int(&two);
    }
    {
        tstring n("0");
                UI(ui_font_bg)->set_text(v, &n);
    }
    {
        tstring n("ui_font_background");
        ui_font_bg->tracks[4]->res = g_resmgr_full.base().get_material(*(rgl::string*)&n);
    }
    {
        unsigned col = 0xff000000;
        UI(ui_font_bg)->set_color(&col);
    }
    UI(status_group)->add_child(UI(status_bar));
    UI(status_group)->add_child(UI(health_bar));
    UI(status_group)->add_child(UI(shield_icon));
    UI(status_group)->add_child(UI(ui_font));
    UI(status_group)->add_child(UI(ui_font_bg));
    frame->kids.push_back(status_group);

    {
        void* mem = operator new(sizeof(NameWidget));
        name_widget = mem ? (NameWidget*)ctor_4ed630((MapWidget*)mem) : 0;
    }
    name_widget->mission_scene = g_mission->scene;
    {
        v[0] = 0.0f;
        v[1] = 0.0f;
        UI(name_widget)->set_size(v);
        v[0] = 1.0f;
        v[1] = 0.75f;
        UI(name_widget)->set_pos(v);
    }
    if (g_options.i2c)
        name_widget->f354 = 1;
    {
        double t = tan(0.8726646304130554);
        fr.perspective = 1;
        fr.nearz = 0.1f;
        fr.farz = 800.0f;
        fr.left = (float)-(0.1f * t);
        float r = (float)(t * 0.1f);
        fr.right = r;
        fr.bottom = (float)(0.1f * t * -0.75f);
        fr.top = r * 0.75f;
        fr.build_planes();
    }
    *(rgl::frustum*)name_widget->frustum = fr;
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
            yaw = (float)atan2(-agent_raw(id)->vel.x, agent_raw(id)->vel.y);
        else
            yaw = 0.0f;
    }
    update_camera(0.001f);
    update_equipment();
}

// @ 0x4f8b40
void MissionHud::update_equipment()
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

// Inline vector helpers as the original's TU had them (scalar by const reference: one float store per use).
// The fourth lane of every result is left unset (the original copies whole vector4s with an unset w).
static __forceinline vector4 hv_mul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 hv_mulv(const vector4& a, float s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 hv_add(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 hv_sub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 hv_neg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }
static __forceinline void hv_add_to(vector4& a, const vector4& b) { a.x = a.x + b.x; a.y = a.y + b.y; a.z = a.z + b.z; }
static __forceinline void hv_sub_to(vector4& a, const vector4& b) { a.x = a.x - b.x; a.y = a.y - b.y; a.z = a.z - b.z; }
#define AG(i) agent_by_id(i)

// @ 0x4f8c80
void MissionHud::update_camera(float dt)
{
    float radius, zoomL;

    if (GAME_AT(int, 0x1a090) != 0)
        dt = g_step_tbl[0];
    {
        matrix home;
        *(matrix*)&k = *(matrix*)identity_pos((pos*)&home);
    }
    radius = 0.03f;
    if (g_cam_sens != 0)
        radius = 0.01f;
    zoomL = zoom;
    if (GAME_AT(int, 0x1a090) != 0) {
        if ((float)(g_ticks[GAME_AT(int, 0x1a150)] - GAME_AT(__int64, 0x1a158)) * 1e-5f < 0.0f) {
            vector4 c;
            vector4& gcam = GAME_AT(vector4, 0x1a1a4);
            vector4& geye = GAME_AT(vector4, 0x1a13c);
            c.x = gcam.x - geye.x;
            c.y = gcam.y - geye.y;
            c.z = gcam.z - geye.z;
            vector4 a = hv_add(geye, hv_mulv(c, (1.0f - dt) * c.normalize()));
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
        float ang, t, u;
        vector4& gcam = GAME_AT(vector4, 0x1a1a4);
        ang = 1.5707964f;
        k.rotate_about_s(ang);
        k.rotate_about_f(&yaw);
        k.rotate_about_s(pitch);
        k.rotate_about_f(&f284);
        f300 = (1.0f - (dt + dt)) * f300;
        if (f300 < 0.0f)
            f300 = 0.0f;
        else if (f300 > 20.0f)
            f300 = 20.0f;
        t = f300;
        if (g_cam_version == 0 && g_cam_down != 0) {
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
        if (g_cam_version != 0) {
            u = (float)GAME_AT(int, 0x1a0b0) * t;
            hv_sub_to(gcam, hv_mul(hv_mul(k.u, u), dt));
            f300 = dt * 25.0f + f300;
        }
        if (g_cam_left != 0) {
            hv_add_to(gcam, hv_mul(hv_mul(k.u, t), dt));
            f300 = dt * 25.0f + f300;
        }
        if (g_cam_right != 0) {
            hv_sub_to(gcam, hv_mul(hv_mul(k.s, t), dt));
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_in != 0) {
            hv_add_to(gcam, hv_mul(hv_mul(k.s, t), dt));
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_out != 0) {
            gcam.z = t * dt * 0.5f + gcam.z;
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_roll != 0 && g_cam_lock == 0 && g_cam_lock2 == 0) {
            gcam.z = gcam.z - t * dt * 0.5f;
            f300 = dt * 17.5f + f300;
        }
        k.o = gcam;
    } else if (CAMSLOT.rec != 0) {
        // the scripted camera slot: cur -> sway over value seconds
        if (CAMSLOT.value > 0.0f) {
            float t = (float)(g_ticks[CAMSLOT.clock] - CAMSLOT.start) * 1e-5f / CAMSLOT.value;
            if (CAMSLOT.num > 0) {
                float nn = (float)CAMSLOT.num;
                t = atan((t - 0.5f) * nn * 3.1415927f) / (atan(nn * 1.5707964f) * 2.0f) + 0.5f;
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
                float u = 1.0f - t;
                k.o = hv_add(hv_mulv(CAMSLOT.sway.o, t), hv_mulv(CAMSLOT.cur.o, u));
                k.u = hv_neg(hv_add(hv_mulv(CAMSLOT.sway.f, t), hv_mulv(CAMSLOT.cur.f, u)));
                k.f = hv_add(hv_mulv(CAMSLOT.sway.u, t), hv_mulv(CAMSLOT.cur.u, u));
                k.s = hv_add(hv_mulv(CAMSLOT.sway.s, t), hv_mulv(CAMSLOT.cur.s, u));
                zoomL = CAMSLOT.zoom_to * t + CAMSLOT.zoom_cur * u;
            }
        } else {
            k.u = hv_neg(CAMSLOT.cur.f);
            k.f = CAMSLOT.cur.u;
            k.s = CAMSLOT.cur.s;
            k.o = CAMSLOT.cur.o;
            zoomL = CAMSLOT.zoom_cur;
        }
        if (CAMSLOT.agent >= 0) {
            if (CAMSLOT.attached != 0) {
                *(matrix*)&k = AG(CAMSLOT.agent)->body->m.mul(k);
                k.orthonormalize();
                goto pass2;
            }
            vector4* bo = &AG(CAMSLOT.agent)->body->m.o;
            k.o.x = bo->x + k.o.x;
            k.o.y = bo->y + k.o.y;
            k.o.z = bo->z + k.o.z;
        }
        k.orthonormalize();
    } else {
      int id = g_mission->player_id;
      if (id >= 0) {
        if (g_game->f139e4 != 0 && AG(g_mission->player_id)->f10 == 0) {
            // spectating: look through the agent this one is bound to
            Agent* ag = AG(g_mission->player_id);
            AgentBody* body = AG(g_mission->player_id)->body;
            Skeleton* sk;
            if (AG(g_mission->player_id)->f2c >= 0)
                yaw = dt * *(float*)((char*)AG(ag->f2c) + 0x56e8) + yaw;
            sk = (Skeleton*)body->skel;
            if ((ag->defs->defs[ag->anim[0].cur].flags & 0x100000) != 0) {
                float ang;
                if (!sk->ready)
                    sk->ensure_updated();
                *(matrix*)&k = body->m.mul(*(matrix*)(sk->recs + 0x1140));
                ang = 3.1415927f;
                k.rotate_about_f(&ang);
                hv_add_to(k.o, hv_mul(k.f, 0.15f));
            } else {
                float lo = 0.0f, hi = 1.0f, ang, radial;
                vector4 c, n;
                if (!sk->ready)
                    sk->ensure_updated();
                n = *(vector4*)(sk->recs + 0x1170);
                c.x = n.x - lag.x;
                c.y = n.y - lag.y;
                c.z = n.z - lag.z;
                radial = c.out_length() * dt * 200.0f;
                hv_add_to(lag, hv_mulv(c, clamp(&radial, &lo, &hi)));
                k.o = body->m.transform_point(lag);
                ang = 1.5707964f;
                k.rotate_about_s(ang);
                k.rotate_about_f(&yaw);
                k.rotate_about_s(pitch);
                hv_add_to(k.o, hv_add(hv_mul(k.f, 0.15f), hv_mul(k.u, 0.11f)));
            }
        } else if (id >= 0) {
            // third person: the player's own agent
            Agent* ag = AG(g_mission->player_id);
            float ang, d;
            if (g_game_keys.is_down(0xd)) {
                float fa = 0.8f, fb = 0.5f;
                if (AG(g_mission->player_id)->f2c >= 0) {
                    fb = 1.0f;
                    fa = 1.1f;
                }
                f280 = (fa - f280) * dt * 3.0f + f280;
                f27c = (fb - f27c) * dt * 3.0f + f27c;
            } else {
                if (AG(g_mission->player_id)->f2c >= 0) {
                    f27c = (0.1f - f27c) * dt + f27c;
                    d = 2.4f;
                } else {
                    f27c = (0.0f - f27c) * dt + f27c;
                    d = 1.65f;
                }
                f280 = (d - f280) * dt + f280;
            }
            head = ag->pos;
            head.z = head.z + f280;
            ang = 1.5707964f;
            head.x = head.x + ag->v98.x;
            head.y = ag->v98.y + head.y;
            head.z = ag->v98.z + head.z;
            k.rotate_about_s(ang);
            k.rotate_about_f(&yaw);
            k.rotate_about_s(pitch);
            k.rotate_about_f(&f284);
            k.o = head;
            d = (pitch < -3.0f ? -3.0f : (pitch < 3.0f ? pitch : 3.0f)) * 0.0f + f278 + f27c;
            hv_add_to(k.o, hv_mul(k.u, d));
            hv_add_to(k.o, hv_mul(hv_mul(k.f, 0.37f), d));
            if (GAME_AT(int, 0x1a090) == 0) {
                // keep the camera out of the walls: sweep a thin capsule from the head to the camera
                shape_base* list[0x2000];
                contact ct;
                int count, i;
                float t;
                vector4 p0 = k.o;
                vector4 q = hv_sub(k.o, hv_mul(k.u, d));
                vector4 c = hv_sub(q, p0);
                c.normalize();
                vector4 a = hv_sub(p0, hv_mul(c, 0.4f));
                capsule cap(1e-5f, &q, &a);
                ((level*)g_mission->scene)->query_segment(&p0.x, &q.x, 0x110, list, &count);
                t = 1000.0f;
                for (i = 0; i < count; i++) {
                    if (list[i]->shape_type() == 2 && shape_contact_count(list[i], &cap, 1, &ct) != 0) {
                        vector4 n = ((face*)list[i])->normal;
                        float dx = ct.point.x - q.x, dy = ct.point.y - q.y, dz = ct.point.z - q.z;
                        float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
                        float dot = n.z * c.z + n.y * c.y + n.x * c.x;
                        if (dot > 0.0f && dist < t) {
                            float lo = 0.1f;
                            t = dist;
                            d = dist - (radius * 2.5f / fmax_ref(&dot, &lo) + 0.6f);
                            if (d < 0.25f)
                                d = 0.25f;
                        }
                    }
                }
                f2ec = (d - f2ec) * dt * 11.0f + f2ec;
                k.o = hv_add(hv_mul(k.u, f2ec), q);
            }
        }
      }
    }
pass2:
    if (CAMSLOT.rec == 0) {
        if (CAMSLOT.value > 0.0f) {
            float t = (float)(g_ticks[CAMSLOT.clock] - CAMSLOT.start) * 1e-5f / CAMSLOT.value;
            if (CAMSLOT.num > 0) {
                float nn = (float)CAMSLOT.num;
                t = atan((t - 0.5f) * nn * 3.1415927f) / (atan(nn * 1.5707964f) * 2.0f) + 0.5f;
            }
            if (t >= 1.0f) {
                CAMSLOT.value = 0.0f;
            } else {
                matrix m;
                float u;
                m.u = hv_neg(CAMSLOT.sway.f);
                m.f = CAMSLOT.sway.u;
                m.s = CAMSLOT.sway.s;
                m.o = CAMSLOT.sway.o;
                if (CAMSLOT.agent >= 0) {
                    if (CAMSLOT.attached != 0) {
                        m = AG(CAMSLOT.agent)->body->m.mul(m);
                    } else {
                        vector4* bo = &AG(CAMSLOT.agent)->body->m.o;
                        m.o.x = m.o.x + bo->x;
                        m.o.y = m.o.y + bo->y;
                        m.o.z = m.o.z + bo->z;
                    }
                }
                u = 1.0f - t;
                k.o = hv_add(hv_mul(k.o, t), hv_mul(m.o, u));
                k.u = hv_add(hv_mul(k.u, t), hv_mul(m.u, u));
                k.f = hv_add(hv_mul(k.f, t), hv_mul(m.f, u));
                k.s = hv_add(hv_mul(k.s, t), hv_mul(m.s, u));
                zoomL = u * CAMSLOT.zoom_to + t * zoomL;
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
    float aspect = g_screen_a / g_screen_b;
    fr.m = k;
    fr.build_planes();
    float v = (float)sqrt(1.0f / aspect);
    v = 1.17f < v ? 1.17f : v;
    v = v * zoomL * 0.86f;
    v = 102.0f < v ? 102.0f : v;
    fr.init(v * 0.017453292f, aspect, radius, 800.0f);
    *(rgl::frustum*)name_widget->frustum = fr;
    float far_plane = 97.0f / zoomL;
    g_mission->camera_to(&k, &far_plane);
}

#define flare_amt (*(float*)&f65cc)
#define flare_gain f6610
#define cam_row cam

// @ 0x4fa790
void Mission::camera_to(const matrix* eye, const float* far_plane)
{
    view = *(const posm*)eye;
    apply_sun_light(&eye->o, far_plane);
    CellGrid* sc = (CellGrid*)scene;
    sc->m30c = vector4(-eye->u.x, -eye->u.y, -eye->u.z);
    sc->m31c = eye->o;
    matrix lis = *eye;
    vector4 back(lis.u.x * 0.4f, lis.u.y * 0.4f, lis.u.z * 0.4f);
    lis.o.x = lis.o.x - back.x;
    lis.o.y = lis.o.y - back.y;
    lis.o.z = lis.o.z - back.z;
    if (g_listener_off == 0) {
        g_listener = lis;
        FSOUND_3D_Listener_SetAttributes(&g_listener.o.x, 0, g_listener.f.x, g_listener.f.y, g_listener.f.z, g_listener.u.x,
                                         g_listener.u.y, g_listener.u.z);
    }
    FrameNode* sun = (FrameNode*)f65b8;
    if (sun) {
        sun->pos.o = eye->o;
        sun->pos.f = vector4(-eye->u.x, -eye->u.y, -eye->u.z);
        sun->pos.u = vector4(0.0f, 0.0f, 1.0f);
        sun->pos.s = cross(sun->pos.f, sun->pos.u);
        sun->pos.s.normalize();
        sun->pos.f = cross(sun->pos.u, sun->pos.s);
        if (f6664 < 0 && fog_dense == 1)
            f6664 = g_sound_mgr.play_slot(8, 3, &g_listener.o.x, 1.0f, 0);
    }
    FrameNode* flare = (FrameNode*)f65c0;
    if (!flare)
        return;
    float tod = time_of_day();
    bool day = tod >= 4.0f && tod < 20.0f;
    int step = (int)((g_ticks[g_game->f13b18] - *(__int64*)&g_game->f13b20) * 1e-5f);
    vector4 dir = g_env_rows[cam_row].get_dir(step % 24, day);
    flare->pos.o = vector4(eye->o.x - dir.x * 500.0f, eye->o.y - dir.y * 500.0f, eye->o.z - dir.z * 500.0f);
    flare->pos.f = dir;
    flare->pos.u = vector4(0.0f, 0.0f, 1.0f);
    flare->pos.orthonormalize();
    float vis;
    if (g_shadow_map.sample(&eye->o, &vis)) {
        vis = vis + vis;
        flare_amt = (vis - flare_amt) * g_step_tbl[2] * 10.0f + flare_amt;
    } else {
        vector4 to(eye->o.x - dir.x * 500.0f, eye->o.y - dir.y * 500.0f, eye->o.z - dir.z * 500.0f);
        if (((level*)scene)->trace_ray(&eye->o.x, &to.x, 1000.0f, 0, 0))
            flare_amt = flare_amt + g_step_tbl[2] * 3.5f;
        else
            flare_amt = flare_amt - g_step_tbl[2] * 3.5f;
    }
    if (flare_amt < 0.0f)
        flare_amt = 0.0f;
    else if (flare_amt >= 1.0f)
        flare_amt = 1.0f;
    rgl::mesh* fm = (rgl::mesh*)f65c4;
    fm->clear_geometry();
    {
        tstring name("sample_flare");
        matrix id;
        fm->append_skinned(g_resmgr_full.get_object((const rgl::string*)&name), (const rgl::mat4*)identity_pos((pos*)&id), -1, -1, 0);
    }
    {
        tstring name("flare");
        fm->res = g_resmgr_full.base().get_material(*(rgl::string*)&name);
    }
    float k = dir.y * eye->u.y + dir.z * eye->u.z + dir.x * eye->u.x - 0.3f;
    k = k * 1.4285715f;
    if (k < 0.0f)
        k = 0.0f;
    else if (k >= 1.0f)
        k = 1.0f;
    float sc2 = k * k;
    float d = 1.0f - 150.0f / flare_gain;
    if (d < 0.0f)
        d = 0.0f;
    else if (d >= 1.0f)
        d = 1.0f;
    float size = *(float*)((char*)&g_env_rows[cam_row] + 0x1c);
    float in = d * size * flare_amt * sc2;
    vector4 col(in * 1.1f, in, in * 0.8f);
    fm->color = rgl::pack_color(&col);
    fm->color |= 0xff000000;
    fm->scale((sc2 * sc2 * 5.0f + 2.0f) * size * 50.0f);
    if (g_game->levels[f5df4].flags & 0x100)
        g_terrain.update_particles(eye);
}

// @ 0x4faee0
bool ShadowMap::sample(const vector4* p, float* out)
{
    // (dx, dy) tap offsets: the original builds two int[9] tables on the stack (x then y)
    int tx[9] = { 0, 1, 1, -1, -1, 2, -2, 0, 0 };
    int ty[9] = { 0, 1, -1, 1, -1, 0, 0, 2, -2 };
    float w[9] = { 1.0f, 0.9f, 0.9f, 0.9f, 0.9f, 0.7f, 0.7f, 0.7f, 0.7f };

    bool ret = true;
    *out = 1.0f;
    if (!cam || !g_use_shaders || !surf) {
        ret = false;
        return ret;
    }
    rgl::frustum* lf = (rgl::frustum*)((char*)cam + 0x10c);
    vector4 clip = lf->to_clip(*p);
    if (!(0.0f <= clip.x && clip.x < 1.0f && 0.0f <= clip.y && clip.y < 1.0f))
        return ret;
    int zero_x = 0;
    *out = 0.0f;
    int px = (int)((float)w2 * clip.x);
    int zero_y = 0;
    int py = (int)((1.0f - clip.y) * (float)h2);
    float depth = clip.z - 0.001f;
    for (int i = 0; i < 9; i++) {
        int xv = tx[i] + px;
        int yv = ty[i] + py;
        float a, b;
        int cy = clamp_index(&yv, &zero_y, &h2);
        int cx = clamp_index(&xv, &zero_x, &w2);
        read_pixel((int)g_render.dev, cx, cy, (unsigned*)&a, (unsigned*)&b);
        if (!(a < depth))
            *out += w[i];
    }
    *out = *out * 0.13513514f;
    return ret;
}

struct ViewLight { char pad0[4]; AgentTable tbl; char pad1c[0x5c - 4 - sizeof(AgentTable)]; int local_id; };
// @ 0x4fb180
void Mission::apply_sun_light(const vector4* pos, const float* scale)
{
    float px = (float)(g_screen_w + g_screen_h);
    float area = (px * 0.5f) * 0.00086956524f;
    float size_v = ((g_scale_a * 0.5f + 0.75f) * *scale) * area;
    float& size = *(float*)&scale;       // the original stores the result back into the (dead) parameter slot
    size = size_v;
    if (g_flag_a17108 && (g_flag_a17148 || g_flag_a17348))
        size = 1000.0f;
    float range = g_scale_b * area;
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
            char** pp = &(*(char***)((char*)this + 0x1c))[i >> 4];
            Agent* a = (Agent*)(*pp + (i & 0xf) * 0x5928);
            SunBody* sb = (SunBody*)a->body;
            if (sb) {
                float one = 1.0f;
                float intensity;
                float dx = sb->origin.x - pos->x;
                float dy = sb->origin.y - pos->y;
                float dz = sb->origin.z - pos->z;
                float d = (float)sqrt(dy * dy + (dx * dx + dz * dz));
                float dist = 3.5f / (d > 3.5f ? d : 3.5f);
                if (!(d > 1.0f))
                    d = 1.0f;
                intensity = (1.0f / d) * size;
                sb->set_intensity(&intensity);
                const float* v;
                if (a->f8 == vl->local_id || a->f30 == vl->local_id)
                    v = &one;
                else {
                    dist = dist * range;
                    v = &dist;
                }
                ((PartyModel*)sb)->set_fade(v);
            }
            int n2 = *(int*)tbl;
            for (i++; (int)i < n2; i++) {
                if (tbl->get(i)->used)
                    break;
            }
        } while (i < *(unsigned*)tbl);
    }
}


// @ 0x4fb350
void MissionHud::update_status()
{
    int id = g_mission->player_id;
    if (id < 0)
        return;
    Agent* ag = agent_raw(id);
    if (GAME_AT(int, 0x1a20c))
        UI(status_group)->hide();
    else
        UI(status_group)->show();
    matrix frame;
    int hp = (int)ag->health;
    if (hp != shown_health) {
        shown_health = hp;
        health_mesh->clear_geometry();
        {
            tstring name("status_health_bar");
            const rgl::mat4* m = (const rgl::mat4*)identity_pos((pos*)&frame);
            rgl::mesh* obj = g_resmgr_full.get_object((const rgl::string*)&name);
            health_mesh->append_skinned(obj, m, -1, -1, 0);
        }
        vector4 col((float)shown_health / ag->max_health, 1.0f, 1.0f);
        health_mesh->set_color(col);
    }

    // shield: slot_pair(0) says whether the agent has one; best_ammo then gives (max, ?, ?) and the (cur, flags) pair
    ItemRef pair;
    int la = -1, lb = -1, lc;
    ItemRef res;
    res.item = -1;
    pair = ag->slot_pair(0);
    if (pair.item >= 0)
        res = ag->best_ammo(-1, &la, &lb, &lc);
    if (la != shield_max || res.item != shield_cur) {
        shield_max = la;
        char buf[512];
        float tsz[2];
        sprintf(buf, "%d", la);
        {
            tstring t1(buf);
            UI(ui_font)->set_text(tsz, &t1);
        }
        {
            tstring t2(buf);
            UI(ui_font_bg)->set_text(tsz, &t2);
        }
        if (shield_max < 0) {
            UI(status_bar)->hide();
            UI(ui_font)->hide();
            UI(ui_font_bg)->hide();
        } else {
            UI(status_bar)->show();
            UI(ui_font)->show();
            UI(ui_font_bg)->show();
            unsigned color = 0xffffffff;
            if (shield_max <= 0)
                color = 0xffb02020;
            UI(ui_font)->set_color98(&color);
        }
        if (res.item != shield_cur) {
            shield_mesh->clear_geometry();
            shadow_mesh->clear_geometry();
            if (res.item >= 0) {
                vector4 at;
                at.x = 0.055f;
                rgl::mesh* piece = make_item_mesh(&res, &at.x);
                if (piece) {
                    float v;
                    v = 0.0005f;
                    piece->set_offset(*(const vector4*)&v);
                    identity3((rgl::mat3*)&frame);
                    float ang = -1.45f;
                    ((posm*)&frame)->rotate_about_u(&ang);
                    piece->set_part(&frame);
                    identity_pos((pos*)&frame);
                    frame.o.z -= 10.0f;
                    shield_mesh->append_skinned(piece, (const rgl::mat4*)&frame, -1, -1, 0);
                    shield_mesh->res = piece->res;
                    frame.o.x += 0.0012f;
                    frame.o.y -= 0.001f;
                    frame.o.z -= 10.0f;
                    piece->scale(1.05f);
                    shadow_mesh->append_skinned(piece, (const rgl::mat4*)&frame, -1, -1, 0);
                    {
                        tstring shadow("ui_item_shadow");
                        shadow_mesh->res = g_resmgr_full.base().get_material(*(rgl::string*)&shadow);
                    }
                    ((mesh_x*)shadow_mesh)->set_bone_index(-1);
                    shadow_mesh->color = 0xff777777;
                    delete piece;
                }
            }
            shield_cur = res.item;
            shield_aux = res.m4;
        }
    }

    // ammo count: (count, capacity) from weapon_info
    ItemRef ammo_pair;
    unsigned lo = -1;
    int hi = -1;
    ammo_pair = ag->slot_pair(1);
    if (ammo_pair.item >= 0)
        ag->weapon_info(-1, &lo, &hi);
    if ((int)lo != ammo_count) {
        ammo_count = lo;
        if ((int)lo == -1) {
            ammo_stage = -1;
            UI(shield_icon)->hide();
            return;
        }
        int stage = (int)((float)(int)lo * 5.0f / (float)hi + 0.999f);
        if (stage != ammo_stage) {
            ammo_stage = stage;
            if (stage <= 0) {
                UI(shield_icon)->hide();
                return;
            }
            const char* nm;
            if (stage == 1)
                nm = "status_shield_20";
            else if (stage == 2)
                nm = "status_shield_40";
            else if (stage == 3)
                nm = "status_shield_60";
            else if (stage == 4)
                nm = "status_shield_80";
            else
                nm = "status_shield_100";
            rgl::mesh* cl;
            {
                tstring icon(nm);
                rgl::mesh* obj = g_resmgr_full.get_object((const rgl::string*)&icon);
                cl = obj->clone();
            }
            UI(shield_icon)->add_image(0, cl);
            UI(shield_icon)->show();
        }
    }
}


// @ 0x4fb8e0: shift every point of the ammo mesh by the scaled vertex normal, and every morph
// key frame's vertex by its scaled normal.
void rgl::mesh::set_offset(const vector4& pos)
{
    ensure_local();
    for (unsigned i = 0; v50.begin() != NULL && i < v50.size(); i++) {
        int idx = v50[i].index;
        rgl::point20& p = v70[idx];
        p.a.x += v50[i].normal.x * pos.x;
        p.a.y += v50[i].normal.y * pos.x;
        p.a.z += v50[i].normal.z * pos.x;
        p.b.x += v50[i].normal.x * pos.x;
        p.b.y += v50[i].normal.y * pos.x;
        p.b.z += v50[i].normal.z * pos.x;
        for (int j = 0; j < kf.count; j++) {
            rgl::frame_t& f = kf.frames[j];
            f.verts[idx].x += f.norms[i].x * pos.x;
            f.verts[idx].y += f.norms[i].y * pos.x;
            f.verts[idx].z += f.norms[i].z * pos.x;
        }
    }
    slots[0].f28 = 1;                       // +0x138 (dirty flag)
    ((mesh_x*)this)->compute_bounds();
}

// @ 0x4fba40 -------------------------------------------------------------------------------
rgl::mesh* __cdecl make_item_mesh(const ItemRef* ref, const float* scale)
{
    rgl::mesh* result;
    matrix m;
    if (ref->item >= 0 && ref->item < g_item_kind_count &&
        (result = (rgl::mesh*)g_items[ref->item].mesh_for(0x1000000000000000ULL, (int)*(signed char*)((const char*)ref + 7))) != 0) {
        result = result->clone();
    } else {
        std::vector<rgl::mesh*> objects;
        {
            tstring name = ((ItemRef*)ref)->mesh_name();
            objects = g_resmgr_full.find_meshes((const rgl::string*)&name);
        }
        if (objects.empty()) {
            {
                tstring name = ((ItemRef*)ref)->mesh_name();
                mb_warning("Unable to find mesh %s", name.c_str());
            }
            result = new rgl::mesh;
            return result;
        }
        result = objects[0]->clone();
        for (unsigned i = 1; i < objects.size(); i++) {
            identity_pos((pos*)&m);
            result->append_skinned(objects[i], (const rgl::mat4*)&m, -1, -1, 0);
        }
    }

    unsigned type = g_items[ref->item].type_flags & 0xff;
    identity3((rgl::mat3*)&m);
    float angle;
    if (type == 0xd) {
        angle = -1.5707964f;
        ((posm*)&m)->rotate_about_s(angle);
        angle = 3.1415927f;
        goto sf;
    } else if (type == 1) {
        angle = -1.5707964f;
        ((posm*)&m)->rotate_about_s(angle);
        angle = 1.5707964f;
        goto sf;
    } else if (type == 2 || type == 3 || type == 4) {
        angle = -0.7853982f;
        goto sf;
    } else if (type == 0xe) {
        angle = -1.5707964f;
        ((posm*)&m)->rotate_about_s(angle);
        angle = 2.670354f;
        ((posm*)&m)->rotate_about_u(&angle);
        angle = -0.31415927f;
        ((posm*)&m)->rotate_about_s(angle);
        goto done;
    } else if (type == 7) {
        angle = 1.5707964f;
        goto sf;
    } else if (type == 0xc) {
        angle = -0.47123894f;
        ((posm*)&m)->rotate_about_f(&angle);
        angle = 0.62831855f;
        ((posm*)&m)->rotate_about_s(angle);
        goto done;
    } else if (type == 9) {
        angle = 1.5707964f;
        ((posm*)&m)->rotate_about_f(&angle);
        angle = -2.3561945f;
        ((posm*)&m)->rotate_about_s(angle);
        goto done;
    } else if (type == 0x11) {
        goto done;
    } else if (type == 8) {
        angle = 2.3561945f;
        goto sf;
    } else if (type == 0xa || type == 5 || type == 6) {
        angle = 0.7853982f;
        goto sf;
    } else if (type == 0xb) {
        angle = -1.2566371f;
        ((posm*)&m)->rotate_about_s(angle);
        angle = 0.7853982f;
        goto sf;
    } else if (type == 0x14) {
        angle = -0.62831855f;
        ((posm*)&m)->rotate_about_s(angle);
        angle = -0.47123894f;
        goto sf;
    } else {
        goto done;
    }
sf:
    ((posm*)&m)->rotate_about_u(&angle);
done:
    result->set_part(&m);

    vector4 bmin, bmax;
    result->calc_bounds(&bmin, &bmax);

    float sumz = bmax.z + bmin.z;
    float cx = (bmin.x + bmax.x) * 0.5f;
    float cy = (bmin.y + bmax.y) * 0.5f;

    unsigned t2 = g_items[ref->item].type_flags & 0xff;
    float size = 1.0f;
    if (t2 == 2 || t2 == 3 || t2 == 4) {
        size = (float)g_items[ref->item].weight * 0.00625f + 0.3f;
        if (size < 0.5f)
            size = 0.5f;
        else if (size > 1.0f)
            size = 1.0f;
    }

    float extent = bmax.y - bmin.y;
    if (extent < bmax.x - bmin.x)
        extent = bmax.x - bmin.x;

    vector4 tr;
    tr.x = -cx;
    tr.y = -cy;
    tr.z = -(sumz * 0.5f);
    tr.w = 0.0f;
    result->translate(&tr);
    result->scale(((*scale / extent) * size) * 0.95f);
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
        vector4 rgb = rgl::unpack_color(&v.color);
        rgb.x *= f;
        rgb.y *= f;
        rgb.z *= f;
        v.color = rgl::pack_color(&rgb);
    }
    slots[0].f28 = 1;
}

// @ 0x4fbfa0
void GameScreen::build()
{
    float v[2];
    g_gui.reset();
    ((SoundW*)&g_sound_mgr)->stop_all();

    char* rec = g_menus + GAME_AT(int, 0x13aec) * 0x60;
    bool ok = true;
    int color = (int)(*(__int64*)(rec + 0x20) >> 32) | 0xff000000;
    GAME_AT(int, 0xc8) = 0;
    GAME_AT(int, 0xa8) = 0;
    rec = g_menus + GAME_AT(int, 0x13aec) * 0x60;
    *(tstring*)((char*)g_game + 0x1a210) = *(tstring*)(rec + 0x28);
    GAME_AT(int, 0x1a220) = -1;
    ((DialogBlock*)(g_menus + GAME_AT(int, 0x13aec) * 0x60 + 0x38))->play(8);
    if (GAME_AT(int, 0xc8) != 0)
        ok = false;

    for (int i = 0; i < 16; i++)
        kids[i] = 0;

    frame = new_frame_widget();
    v[0] = -0.001f;
    v[1] = -0.001f;
    UI(frame)->set_size(v);
    v[0] = 1.002f;
    v[1] = 0.752f;
    UI(frame)->set_pos(v);

    GAME_AT(int, 0xac) = -1;

    fill = new MissionUi;
    v[0] = 0.1f;
    v[1] = 0.1f;
    UI(fill)->set_size(v);
    v[0] = 1e-05f;
    v[1] = 1e-05f;
    UI(fill)->set_pos(v);
    UI(fill)->hide();
    UI(frame)->add_child(UI(fill));

    if (ok) {
        if (((tstring*)((char*)g_game + 0x1a210))->differs("none")) {
            rgl::mesh* src = g_resmgr_full.get_object((const rgl::string*)((char*)g_game + 0x1a210));
            if (src) {
                rgl::mesh* m = src->clone();
                rec = g_menus + GAME_AT(int, 0x13aec) * 0x60;
                if ((*(unsigned*)(rec + 0x20) & 0x1000) && 1.35f < g_screen_b / g_screen_a) {
                    float f = 1.33f / (g_screen_b / g_screen_a);
                    vector4 c;
                    c.x = f; c.y = 1.0f; c.z = 1.0f; c.w = 1.0f;
                    m->set_color(c);
                    vector4 tr;
                    tr.x = 1.0f - f; tr.y = 0.0f; tr.z = 0.0f; tr.w = 0.0f;
                    m->translate(&tr);
                }
                UI(frame)->add_image(2, m);
            }
        }
        if (GAME_AT(int, 0x1a220) >= 0) {
            rgl::mesh* m = g_tableaus.make_mesh_from(&GAME_AT(int, 0x1a220), &GAME_AT(int, 0x1a224));
            if (GAME_AT(float, 0x1a230) != 1.0f)
                m->scale(GAME_AT(float, 0x1a230));
            UI(fill)->add_image(0, m);
            v[0] = GAME_AT(float, 0x1a228);
            v[1] = GAME_AT(float, 0x1a22c);
            UI(fill)->set_size(v);
            UI(fill)->show();
        }
    }

    {
        unsigned c = 0xffffffff;
        UI(frame)->set_color98(&c);
    }
    fa0 = 0;

    panel = new MissionUi;
    v[0] = 0.0f;
    v[1] = 0.0f;
    UI(panel)->set_size(v);
    {
        int two = 0x710;
        UI(panel)->set_int(&two);
    }
    v[0] = 1.0f;
    v[1] = 0.75f;
    UI(panel)->set_pos(v);
    {
        unsigned c = color;
        UI(panel)->set_color98(&c);
    }
    {
        tstring name("");
        float out[2];
        UI(panel)->set_text(out, &name);
    }

    FlagWidget* flag = new FlagWidget;
    flag->f178 = 1;
    v[0] = 0.0f;
    v[1] = 0.0f;
    UI(flag)->set_size(v);
    UI(frame)->add_child(UI(flag));

    tree = new ListWidget;
    tree->f104 |= 0x20;
    tree->flag = flag;

    GroupWidget* bar = new GroupWidget;
    v[0] = 0.0f;
    v[1] = 0.0f;
    UI(bar)->set_size(v);
    UI(bar)->set_pos(&panel->screen[0]);
    UI(bar)->add_child(UI(panel));
    tree->bar = bar;
    UI(tree)->add_child(UI(bar));
    v[0] = 0.0f;
    v[1] = 0.0f;
    UI(tree)->set_size(v);
    v[0] = 0.01f;
    v[1] = 0.38f;
    UI(tree)->set_size(v);
    ((FieldPanel*)tree)->update();
    v[0] = 0.65f;
    v[1] = 0.31f;
    ((HudOuter*)tree)->scroll_refresh((vector4*)v);
    UI(frame)->add_child(UI(tree));

    GroupWidget* value = new GroupWidget;
    v[0] = 0.0f;
    v[1] = 0.0f;
    UI(value)->set_size(v);
    v[0] = 0.75f;
    v[1] = 0.36f;
    UI(value)->set_pos(v);
    MissionUi* made = (MissionUi*)make_scroll_track((widget*)frame, 0.7f, 0.02f, 0.01f, 0.36f);
    {
        float a[2] = { 0.01f, 0.02f };
        float b[2] = { 0.75f, 0.36f };
        made = (MissionUi*)make_quest_list((widget*)value, a, b, (int)made);
    }
    slider = made;
    UI(frame)->add_child(UI(made));

    MissionUi** kp = kids;
    for (int i = 16; i != 0; i--, kp++) {
        ToggleWidget* child = new ToggleWidget;
        *kp = child;
        unsigned c9 = 0xff55ff50;
        UI(*kp)->set_color(&c9);
        float pos[2] = { 0.64f, 0.04f };
        UI(*kp)->set_pos(pos);
        unsigned c8 = color;
        UI(*kp)->set_color98(&c8);
        UI(*kp)->hide();
        UI(value)->add_child(UI(*kp));
    }

    if (ok) {
        ((ScreenOps*)this)->begin();
        ((BlockOwner*)((char*)this + 0x30))->init((ContainerBase*)frame, GAME_AT(int, 0xac));
        if (f40 < 0)
            UI(frame)->add_image_a(0, g_resmgr_full.get_object((const rgl::string*)&tstring("meeting_window"))->clone());
    }
    g_gui_dirty = 1;
    {
        MissionUi* root = frame;
        ((NodeVec*)&g_gui)->push_back(*(NodeA**)&root);
    }
}

// @ 0x4fc810
void LogScreen::build_thunk()
{
    build(show_log != 0);
}

// the parameter table lookup every tab button position goes through: a missing key yields 0 (inlined at each of its four uses)
static __forceinline float param_value(const char* key)
{
    tstring k(key);
    int found = -1;
    for (int i = 0; i < g_param_count; i++) {
        if (g_game_vars[i].name.equals(k)) {
            found = i;
            break;
        }
    }
    float v = 0.0f;
    if (found >= 0) {
        char* s = g_game_vars[found].value;
        if (s)
            v = (float)atof(s ? s : (char*)&g_game_vars[found].value);
    }
    return v;
}
#define MAKE_TAB(name) make_tab(frame, name, x, yy, h)
static MissionUi* make_tab(FrameWidget* frame, const char* name, float x, float y, float h)
{
    return (MissionUi*)make_short_button(frame, g_strtable.lookup(tstring(name)), x, y, h);
}

// @ 0x4fc820
void LogScreen::build(bool show)
{
    float v[2];
    g_gui.reset();

    frame = new_frame_widget();
    v[0] = 0.0f;
    v[1] = 0.0f;
    UI(frame)->set_size(v);
    v[0] = 1.0f;
    v[1] = 0.75f;
    UI(frame)->set_pos(v);
    UI(frame)->add_image(0, res_clone(tstring("game_log_window")));

    log = new MWidget;
    {
        int c700 = 0x700;
        UI(log)->set_int(&c700);
    }
    v[0] = 0.0f;
    v[1] = 0.0f;
    UI(log)->set_size(v);
    v[0] = 0.02f;
    v[1] = 0.02f;
    UI(log)->set_inset(v);
    {
        unsigned c = 0xff000000;
        UI(log)->set_color98(&c);
    }
    v[0] = 0.915f;
    v[1] = 0.1f;
    UI(log)->set_pos(v);
    tstring* text = &log->U()->name;
    text->assign("");

    if (show) {
        char delim = '|';
        int count;
        tstring* arr = ((tstring*)((char*)g_game + 0x13b50))->split(delim, count);
        if (count > 2) {
            int start = count - 300;
            if (start < 0)
                start = 0;
            for (int i = start; i < count; i += 3) {
                text->append(arr[i]);
                text->push_back('\t');
                text->append(arr[i + 2]);
                text->push_back('\n');
            }
            text->erase(text->len - 1, 1);
        }
        if (arr)
            ((TStringArray*)arr)->destroy(3);
    } else {
        int n = 0;
        if (g_msg_begin)
            n = ((int)(g_msg_end - g_msg_begin) / 0x10) * 3;
        for (int i = 0; i < n; i += 3) {
            text->append(g_msg_begin[i / 3]);
            text->push_back('\n');
        }
        if (text->len > 0)
            text->erase(text->len - 1, 1);
    }
    {
        float out[2];
        UI(log)->set_text(out, text);
    }

    float y = log->U()->screen[1] + 0.001f;

    bar = new GroupWidget;
    v[0] = 0.0f;
    v[1] = 0.0f;
    UI(bar)->set_size(v);
    v[0] = 0.915f;
    v[1] = y;
    UI(bar)->set_pos(v);
    UI(bar)->add_child(UI(log));

    MissionUi* track = (MissionUi*)make_scroll_track((widget*)frame, 0.95f, 0.08f, 0.02f, 0.64f);
    {
        float ymin = y < 0.64f ? y : 0.64f;
        float d = 0.64f - y;
        if (!(d > 0.0f))
            d = 0.0f;
        float a[2] = { 0.03f, d + 0.08f };
        float b[2] = { 0.915f, ymin };
        slider = (MissionUi*)make_quest_list((widget*)bar, a, b, (int)track);
    }
    UI(frame)->add_child(UI(slider));

    float x = param_value("note_button_x");
    float yy = param_value("note_button_y");
    float h = param_value("note_button_h");
    float dy = param_value("note_button_x_a");
    tab_log = MAKE_TAB("ui_game_log");
    ((ToggleWidget*)tab_log)->f184 = 1;
    yy += dy;
    tab_recent = MAKE_TAB("ui_recent_messages");
    ((ToggleWidget*)tab_recent)->f184 = 1;
    yy += dy;
    tab_quests = MAKE_TAB("ui_quests");
    ((ToggleWidget*)tab_quests)->f184 = 1;
    yy += dy;
    tab_troops = MAKE_TAB("ui_troops2");
    ((ToggleWidget*)tab_troops)->f184 = 1;
    yy += dy;
    tab_locations = MAKE_TAB("ui_locations");
    ((ToggleWidget*)tab_locations)->f184 = 1;
    yy += dy;
    tab_factions = MAKE_TAB("ui_factions");
    ((ToggleWidget*)tab_factions)->f184 = 1;
    yy += dy;
    tab_return = MAKE_TAB("ui_return");
    ((ToggleWidget*)tab_return)->f184 = 1;

    MissionUi* active;
    if (show) {
        UI(tab_log)->select(1);
        active = tab_log;
    } else {
        UI(tab_recent)->select(1);
        active = tab_recent;
    }
    UI(active)->refresh();

    g_gui_dirty = 1;
    {
        MissionUi* root = frame;
        ((NodeVec*)&g_gui)->push_back(*(NodeA**)&root);
    }

    // the slider's value follows the log's scroll state
    FlagWidget* flag = (FlagWidget*)((ListWidget*)slider)->flag;
    float* pf = &flag->screen[flag->f178];
    float value = (*pf - flag->f17c) * ((flag->v168 - flag->f16c) / (flag->f170 - flag->f16c)) - (flag->f17c / *pf) * 5000.0f;
    ((UiD*)flag)->set_value(value);
    UI(flag)->flag_cmd(0x10000);
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

// @ 0x4fd150
MWidget::MWidget()
{
    ((MissionUi*)this)->MissionUi::MissionUi();
    *(void**)this = (void*)0x5995c0;
    rects_a.first = 0;
    rects_a.last = 0;
    rects_a.end = 0;
    rects_b.first = 0;
    rects_b.last = 0;
    rects_b.end = 0;
    list_c.buy(0);
    list_d.first = 0;
    list_d.last = 0;
    list_d.end = 0;
    list_e.first = 0;
    list_e.last = 0;
    list_e.end = 0;
    cur = -1;
}

// @ 0x4fd1d0
char MWidget::on_event(unsigned ev)
{
    char r = 0;
    if ((ev & 7) != 0) {
        int id = cur;
        cur = -1;
        int* ids = list_e.first;
        unsigned i = 0, off = 0;
        for (; ids != 0 && i < (unsigned)(list_e.last - ids); i++, off += 0x10) {
            vector4* a = (vector4*)((char*)rects_a.first + off);
            vector4* b = (vector4*)((char*)rects_b.first + off);
            if (a->x + U()->fE0 <= g_cursor_x)
                if (U()->fE0 + b->x > g_cursor_x)
                    if (a->y + U()->fE4 <= g_cursor_y)
                        if (b->y + U()->fE4 > g_cursor_y) {
                            cur = list_e.first[i];
                            break;
                        }
        }
        if (cur != id)
            U()->get_xy((float*)&id);
    } else if ((ev & 8) != 0) {
        int* ids = list_e.first;
        r = 1;
        unsigned i = 0, off = 0;
        for (; ids != 0 && i < (unsigned)(list_e.last - ids); i++, off += 0x10) {
            vector4* a = (vector4*)((char*)rects_a.first + off);
            vector4* b = (vector4*)((char*)rects_b.first + off);
            if (U()->fE0 + a->x <= g_cursor_x)
                if (b->x + U()->fE0 > g_cursor_x)
                    if (a->y + U()->fE4 <= g_cursor_y)
                        if (b->y + U()->fE4 > g_cursor_y) {
                            U()->set_key_flags(0x10000);
                            sel = i;
                            break;
                        }
        }
    }
    U()->set_key_flags(ev);
    U()->refresh();
    return r;
}

// @ 0x4fd3b0
void* MWidget::destroy(unsigned flags)
{
    this->~MWidget();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

// @ 0x4fd3d0
MWidget::~MWidget()
{
    *(void**)this = (void*)0x5995c0;
    list_e.free_block();
    list_d.free_block();
    VEC_CLEAR(&list_c);
    rects_b.free_block();
    rects_a.free_block();
    ((Buffer*)this)->Buffer::~Buffer();
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
MapText MWidget::text_rect()
{
    MissionUi* u = U();
    if (u->tracks[4] == 0) {
        rgl::mesh* m = new rgl::mesh;
        tstring n("ui_font");
        m->res = g_resmgr_full.base().get_material(*(rgl::string*)&n);
        UI(this)->add_image_a(4, m);
    }
    u->tracks[4]->clear_geometry();

    MapText out;
    vector4 rc;
    rc.x = u->f138;
    rc.y = u->f13c;
    rc.z = 0.0f;
    unsigned col[3] = { 0xffff0000, 0xff0000ff, 0xff000000 };
    Font* f = (Font*)u->f124;
    if (f)
        f->layout_map(&out, &u->name, &rc, &u->f140, u->tracks[4], &col[2], &col[1], &col[0], &u->screen[2], &u->screen[0], u->f148, cur, &rects_a, &rects_b, &list_e, &list_c, &list_d);
    else
        g_font->layout_map(&out, &u->name, &rc, &u->f140, u->tracks[4], &col[2], &col[1], &col[0], &u->screen[2], &u->screen[0], u->f148, cur, &rects_a, &rects_b, &list_e, &list_c, &list_d);
    int idx = -1;
    u->get_xy((float*)&idx);
    return out;
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
extern const int g_text_maxn;                                     // 0x5937ac (0x2000: the decode buffer's capacity)

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
    float widest = *xmin;

    int buf[0x2000];
    int total;
    decode_utf8(text, &total, buf, &g_text_maxn);

    if (!(flags & 0x100) && !(flags & 2)) {                       // shrink the text so the longest line fits
        int zero = 0;
        float w = measure(buf, total, &zero, &total) * sc[0] * 1.001f;
        float avail = *xmax - *xmin;
        if (avail < w && 0.0f < w) {
            float k = avail / w;
            sc[0] *= k;
            sc[1] *= k;
        }
    }

    int y = 0;
    char in_tag = 0;
    char tag = 0;
    int tag_start = 0;
    int tag_index = -1;
    if (total > 0) {
        int align_left = flags & 4;
        float inv_adv = 1.0f / sc[0];
        int search;
        do {
            search = y;
            float run_w = 0.0f;
            char first = 1;
            for (;;) {
                char stop = 0;
                int i = search;
                for (; i < total; i++) {
                    int ch = buf[i];
                    if (ch == 0x20 || ch == 10 || ch == 9) {
                        if (i < total) {
                            if (buf[i] == 10)
                                stop = 1;
                            i++;
                        }
                        break;
                    }
                }
                int wend = i;
                float w = measure(buf, total, &y, &wend) * sc[0];
                if (first || w <= *xmax - *xmin) {
                    run_w = w;
                    search = wend;
                } else
                    stop = 1;
                if (search >= total)
                    stop = 1;
                first = 0;
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
                        if (name.find(tstring("color"), 0) == 0) {
                            code = 1;
                            VEC_INT_PUSH_BACK(codes, code);
                        } else if (name.find(tstring("size"), 0) == 0) {
                            code = 2;
                            VEC_INT_PUSH_BACK(codes, code);
                        } else if (name.find(tstring("font"), 0) == 0) {
                            code = 3;
                            VEC_INT_PUSH_BACK(codes, code);
                        } else if (name.find(tstring("alpha"), 0) == 0) {
                            code = 4;
                            VEC_INT_PUSH_BACK(codes, code);
                        }
                        char eq = '=';
                        int value = atol(name.substr(name.find(eq, 0) + 1, name.len).c_str());
                        ints_push(codes, value);
                        rects_a->push_back(pos);
                        ints_push(tags, tag_index);
                        tag = 0;
                    } else {                                      // the closing tag: the span's end rectangle
                        vector4 t;
                        t.x = pos.x;
                        t.y = font_scale * scale[1] * line_spacing * 0.9f + pos.y;
                        t.z = pos.z;
                        rects_b->push_back(t);
                        tag = 0;
                    }
                    continue;
                }

                int ch = buf[i2];
                if (ch == 0x3c) {                                 // '<'
                    tag = 1;
                    tag_start = i2;
                } else if (ch != 10) {
                    if (ch == 9) {
                        int tab = (int)((inv_adv * pos.x - xpos) * 4.0f);
                        pos.x = (((float)tab + 1.0f) * 0.25f + xpos) * sc[0];
                    } else {
                        if (in_tag) {
                            if (i2 == y) {                        // a span continues on a new row
                                VEC_INT_PUSH_BACK(spare, spare->first[(spare->first ? (unsigned)(spare->last - spare->first) : 0) - 1]);
                                VEC_INT_PUSH_BACK(codes, codes->first[(codes->first ? (unsigned)(codes->last - codes->first) : 0) - 1]);
                                rects_b->push_back(pos);
                                VEC_INT_PUSH_BACK(tags, tag_index);
                            }
                        }
                        const Glyph* g;
                        if (ch < 0x200)
                            g = &ascii[ch];
                        else {
                            GlyphNode* node;
                            int key = ch;
                            map.lower_bound(&node, &key);
                            g = node == map.head ? &dflt : &node->val;
                        }
                        pos.x += sc[0] * g->f10;
                        draw_glyph(ch, &pos, sc, m, (int)(in_tag ? (cur == tag_index ? col_hot : col_link) : col_text));
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
            pos.x = *xmin;
            y = search;
            pos.y -= row;
        } while (search < total);
    }

    m->compute_normals();
    ((float*)out)[0] = (widest - *xmin) + 0.0001f;
    ((float*)out)[1] = start_y - pos.y;
    return out;
}

// @ 0x4fdff0
void V4List::push_back(const vector4& v)
{
    unsigned size = first == 0 ? 0 : (unsigned)(last - first);
    if (first != 0 && size < (unsigned)(end - first)) {
        vector4* l = last;
        uninit_fill_n<vector4>(l, 1, v, *(std::allocator<vector4>*)this, std::_Nonscalar_ptr_iterator_tag());
        last = l + 1;
    } else {
        iterator w;
        w.p = last;
        insert(w, v);
    }
}

// @ 0x4fe060
V4List::iterator V4List::insert(iterator where, const vector4& val)
{
    unsigned idx;
    if (first == 0 || last - first == 0)
        idx = 0;
    else
        idx = where.p - first;
    ((VecOps<vector4>*)this)->insert_n(std::vector<vector4>::iterator(where.p), 1, val);
    iterator r;
    r.p = first + idx;
    return r;
}
