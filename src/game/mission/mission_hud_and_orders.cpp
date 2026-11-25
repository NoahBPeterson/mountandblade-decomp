//-----------------------------------------------------------------------------
// File: mission_hud_and_orders.cpp  (0x5284a0..0x52bf40)
//
// Desc: view controller, mission HUD markers, party heroes, agent bookkeeping, interaction probe, order markers and menu, order_agent, update_ui, cursor, find_position
//       One part of what was game/mission5/mission_5.cpp (the object's functions in address order; the cut points and the evidence for
//       them are in docs/source_tree.md section 3). The prelude and the helper definitions below are the
//       parts of the old file's prelude and earlier sections this part's code uses.
//-----------------------------------------------------------------------------

#define PLACEDOBJ_CTOR_OUT_OF_LINE   // fn_529d80 0x529d80 calls PlacedObj::PlacedObj (0x437830) for the resize(0) temporary
#include "platform/image_addrs.h"
#include "game/data/skill_cap.h"      // SkillCap g_skill_tbl (0x8ee3100): [36].cap is the clamp at 0x8ee3ac4
#include "game/config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "game/mission/mission_frame_update.h"
#include "game/campaign/game_state_load.h"      // Game::more_start (g_game is the game constructor's Game)
#include "game/data/troop.h"   // Troop::unequip (0x4630d0)
#include "engine/core/tstring.h"
#include "engine/math/rgl_math.h"
#include "engine/io/stream.h"
#include "engine/io/stream_inline.h"   // the file writer built on save_scene's stack: constructor inline
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "engine/render/mesh.h"
#include "game/ui/gui_manager.h"      // g_gui (0x5d7080)
#include "game/ui/screens/conversation/conv_screen.h"      // Rec1208 (ConvScreen): the conversation screen record, whose slot 3 is handle_event 0x525160
#include "platform/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "game/data/item_kind_include.h"   // g_items 0x5e29d0
#include "game/data/menu_rec.h"   // g_menus 0x5e29b0
#include "game/data/mode_rec.h"   // g_mission_templates 0x5e2a00
#include "game/data/presentation.h"                               // widget_mgr, g_presentation (0x5e299c)
#include "game/input/key_names.h"
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "engine/render/render_flags.h"
#include "game/ui/ui_globals.h"      // UI / core globals
#include "game/campaign/map_flags.h"      // MapFlags::commit 0x513710
#include "game/config/core.h"     // GameApp g_app_obj (0x8ee3c48): the map editor dialog's members

// ---- definitions from the old file's prelude and earlier sections that this part's code uses (copies; each is a static
// or inline helper, a type or a macro: none of them emits code or data of its own)
// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
extern const RangePair g_aim_range_tbl[7] = {{-0.5f, 3.1415927f}, {-2.0f, 2.0f}, {-1.5f, 2.5f}, {-1.0f, 3.0f}, {-1.3f, 2.8f}, {-3.0f, 1.0f}, {5.0f, 4.0f}};  // 0x59390c (rdata: the aim arc per Agent::variant)
#include "game/mission/mission_shared.h"   // the definitions this file shares with its sibling parts

inline Agent* AgentTable::get(unsigned id)
{
    return &pages[id >> 4][id & 0xf];
}


// ======================================================= slice s005284a0: functions
static __forceinline Agent& ag_s284a0(unsigned id) { return g_mission->agents.pages[id >> 4][id & 0xf]; }
static __forceinline Agent& agr_s284a0(unsigned id) { Agent** pages = &g_mission->agents.pages[0]; return *(pages[id >> 4] + (id & 0xf)); }
static __forceinline float cl0_s284a0(float x) { if (0.0f > x) x = 0.0f; return x; }
// @ 0x5284a0 ------------------------------------------------------------------------------------------
void MissionBlocker::update_view(float dt)
{
    if (g_game->editor.active && (g_keyboard.down[45] || g_keyboard.down[21] || g_keyboard.down[44] || g_keyboard.down[22] ||
                   g_keyboard.down[47] || g_keyboard.down[34] || g_keyboard.down[20]))
        return;
    if (g_game->editor.active)
        dt = g_step_tbl[0];
    float k = expf(g_options.exp_arg * 4.0f) * 0.04f;

    // ---- aiming at an agent while a melee weapon (or the fists) is out is slower ---------------------
    if (!g_game->editor.active) {
        if (g_presentation == 0 || (pres.flags & 1)) {
            if (g_mission->player_id >= 0) {
                Agent* a = &agr_s284a0(g_mission->player_id);
                ItemRef w0 = a->slot_pair(0);
                ItemRef w1 = a->slot_pair(1);
                bool hand0;
                if (w0.item < 0) {
                    hand0 = true;
                } else {
                    int t = g_items[w0.item].type_flags & 0xff;
                    hand0 = (t == 2 || t == 3 || t == 4);
                }
                if (((hand0 && a->active == 1 && g_options.ctl_mode > 0 && a->anim[1].cur == a->snd_a) ||
                     (w1.item < 0 && a->f5610 > 0 && g_options.stick_mode > 0 && a->anim[1].cur == a->block_anim_a)) &&
                    a->anim[1].scale < 0.6f)
                    k *= 0.1f;
            }
        }
    }
    // ---- look input ---------------------------------------------------------------------------------
    float look_y = 0.0f;
    float look_x = 0.0f;
    if (g_presentation == 0 || (pres.flags & 1)) {
        look_x = g_keyboard.axis[0];
        look_y = g_keyboard.axis[1];
    } else {
        if (g_gui.mouse[0] <= 0.01f)
            look_x = dt * -400.0f;
        else if (g_gui.mouse[0] >= 0.99f)
            look_x = dt * 400.0f;
        if (g_gui.mouse[1] <= 0.01f)
            look_y = dt * 400.0f;
        if (g_gui.mouse[1] >= 0.74f)
            look_y = dt * -400.0f;
    }

    // ---- damp the view rates and integrate them into yaw / pitch ------------------------------------
    if (!g_game->editor.active || g_keyboard.down[0xe0] != 0) {
        { float r = 1.0f - dt * 20.0f; if (0.0f > r) r = 0.0f; yaw_rate = r * yaw_rate; }
        { float r = 1.0f - dt * 20.0f; if (0.0f > r) r = 0.0f; pitch_rate = r * pitch_rate; }
        yaw_rate = look_x * zoom * 0.0018f + yaw_rate;
        pitch_rate = (g_options.invert ? -look_y : look_y) * zoom * 0.0018f + pitch_rate;
        yaw = yaw - yaw_rate * k * dt;
        pitch = pitch - pitch_rate * k * dt;
    }

    // ---- clamps -------------------------------------------------------------------------------------
    float& p_ref = pitch;
    if (p_ref < -0.9519978165626526f)
        p_ref = -0.9519978165626526f;
    else if (p_ref > 0.9519978165626526f)
        p_ref = 0.9519978165626526f;
    if (cam_dist < 0.7f)
        cam_dist = 0.7f;
    else if (cam_dist > 10.0f)
        cam_dist = 10.0f;

    if (g_mission->player_id < 0)
        return;

    // ---- build the view frame and hand its forward row to the local agent ----------------------------
    Agent* a = &agr_s284a0(g_mission->player_id);
    {
        matrix m;
        identity3((rgl::mat3*)&m);
        m.rotate_about_u(&yaw);
        m.rotate_about_s(&p_ref);

        bool looking = false;
        if (g_game_keys.entry[13][0] >= 0 && g_keyboard.down[g_game_keys.entry[13][0]] != 0)
            looking = true;
        if ((g_game_keys.entry[13][1] < 0 || g_keyboard.down[g_game_keys.entry[13][1]] == 0) && !looking)
            a->in_now.move = m.f;
    }

    if (snap < 5) {
        a->in_now.move = a->vel;        // copied through `a` itself: the scheduler then knows the stores cannot alias the loads
        const vector4& d = a->vel;
        float nx = -d.x;
        float ang = (float)atan2(nx, d.y);
        int n = snap + 1;
        p_ref = 0.0f;
        snap = n;
        yaw = ang;
    }

    // ---- movement / action bits ---------------------------------------------------------------------
    a->in_now.bits = 0;
    if (!g_game->editor.active) {
        if (g_keyboard.down[42] == 0)
            a->in_now.bits = 0x200000;
        if (g_game_keys.clicked(5))
            a->in_now.bits |= 0x100000;

        if (a->horse >= 0) {
            if (g_game_keys.clicked(0))
                a->in_now.bits |= 0x80000;
            if (g_game_keys.is_down(0))
                a->in_now.bits |= 0x10;
            else if (g_game_keys.is_down(1))
                a->in_now.bits |= 0x20;
            if (g_game_keys.is_down(3))
                a->in_now.bits |= 0x40;
            else if (g_game_keys.is_down(2))
                a->in_now.bits |= 0x80;
            if (g_game_keys.clicked(4) && sel_mode == 3)
                a->in_now.bits |= 0x10000;
            if ((char)g_keyboard.is_ready(0x24) && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0))
                a->in_now.bits |= 0x40000;
        } else {
            if (g_game_keys.is_down(0))
                a->in_now.bits |= 1;
            else if (g_game_keys.is_down(1))
                a->in_now.bits |= 2;
            if (g_game_keys.is_down(3))
                a->in_now.bits |= 4;
            else if (g_game_keys.is_down(2))
                a->in_now.bits |= 8;
            if (g_game_keys.clicked(4) && sel_mode == 2 && sel_valid != 0) {
                a->in_now.bits |= 0x20000;
                a->ai.target = sel_target;
            }
        }

        if (g_presentation == 0 || (pres.flags & 1)) {
            if (g_game_keys.is_down(6))
                a->in_now.bits |= 0x100;
            if (g_game_keys.clicked(6))
                a->in_now.bits |= 0x400;
            if (g_game_keys.is_down(7))
                a->in_now.bits |= 0x200;
        }

        if (g_game_keys.clicked(8)) {
            a->cycle_slot(0);
        } else if (g_game_keys.clicked(9)) {
            a->cycle_slot(1);
        } else if (g_game_keys.clicked(10)) {
            a->wield[0] = -1;
            a->on_wield_reset();
        }
    }

    if (g_game_keys.clicked(0xe)) {
        g_game->f139e4 = (g_game->f139e4 == 0);
        update_equipment();
    }
    {
        unsigned id = g_mission->player_id;
        Agent** pages = &g_mission->agents.pages[0];
        (pages[id >> 4] + (id & 0xf))->in_now.f4 = (g_game->f139e4 != 0);
    }

    // ---- field of view ------------------------------------------------------------------------------
    bool zoom_key = false;
    if (g_game_keys.entry[12][0] >= 0 && g_keyboard.down[g_game_keys.entry[12][0]] != 0)
        zoom_key = true;
    if ((g_game_keys.entry[12][1] >= 0 && g_keyboard.down[g_game_keys.entry[12][1]] != 0) || zoom_key)
        zoom = zoom - dt * 350.0f;
    else
        zoom = zoom + dt * 350.0f;

    float limit = 97.0f;
    if (g_game->f139e4 != 0)
        limit = 90.0f;
    if (zoom < 37.0f)
        zoom = 37.0f;
    else if (zoom > limit)
        zoom = limit;

    // ---- the "use / attack" keys walk the agent table and hit the agent under the crosshair ---------
    if (g_cfg.cheat_mode != 0 && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) && g_keyboard.down[63] != 0 && g_keyboard.prev[63] == 0) {
        if (agr_s284a0(g_mission->player_id).f7c == 2)
            g_mission->agents.get(g_mission->player_id)->f7c = 1;
        else
            g_mission->agents.get(g_mission->player_id)->f7c = 2;
    }
    if (g_cfg.cheat_mode != 0 && g_keyboard.down[29] != 0 && (char)g_keyboard.is_ready(0x3e)) {
        bool done = false;
        for (unsigned i = g_mission->agents.first_used(); i < (unsigned)g_mission->agents.count && !done; g_mission->agents.next_used(&i)) {
            Agent* o = &agr_s284a0(i);
            if (o->state == 1 && o->kind == 0 && g_mission->agents.get(g_mission->player_id)->is_enemy(o)) {
                Blow b;
                b.init();
                b.kind = 2;
                b.bone = 0;
                b.attacker = g_mission->player_id;
                b.pos = g_mission->agents.get(i)->pos;
                b.dmg_in = 100.0f;
                g_mission->agents.get(i)->note_blow(&b);
                if (g_keyboard.down[42] == 0)
                    done = true;
            }
        }
    }

    if (g_cfg.cheat_mode != 0 && g_keyboard.down[29] != 0 && g_keyboard.down[64] != 0 && g_keyboard.prev[64] == 0) {
        bool done = false;
        for (unsigned i = g_mission->agents.first_used(); i < (unsigned)g_mission->agents.count && !done; g_mission->agents.next_used(&i)) {
            Agent* o = &agr_s284a0(i);
            if (i != (unsigned)g_mission->player_id && o->state == 1 && o->kind == 0 && g_mission->agents.get(g_mission->player_id)->is_friend(o)) {
                Blow b;
                b.init();
                b.kind = 2;
                b.bone = 0;
                b.attacker = g_mission->player_id;
                b.pos = g_mission->agents.get(i)->pos;
                b.dmg_in = 100.0f;
                g_mission->agents.get(i)->note_blow(&b);
                if (g_keyboard.down[42] == 0)
                    done = true;
            }
        }
    }

    if (g_cfg.cheat_mode != 0 && g_keyboard.down[29] != 0 && g_keyboard.down[61] != 0 && g_keyboard.prev[61] == 0) {
        Blow b;
        b.init();
        b.kind = 2;
        b.bone = 0;
        b.attacker = g_mission->player_id;
        b.pos = ag_s284a0(g_mission->player_id).pos;
        b.dmg_in = 200.0f;
        agr_s284a0(g_mission->player_id).note_blow(&b);
    }

    if ((g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) && g_keyboard.down[35] != 0 && g_keyboard.prev[35] == 0 && g_cfg.cheat_mode != 0)
        agr_s284a0(g_mission->player_id).health = agr_s284a0(g_mission->player_id).max_health;
}

// ======================================================= slice s00529130: functions
static __forceinline Agent* cur_at_s29130()
{
    std::vector<Agent*>& pages = g_mission->agents.pages;
    return pages[(unsigned)g_mission->player_id >> 4] + (g_mission->player_id & 0xf);
}
struct V2_s29130 : V2 {                 // a V2 with the rotation the aim marks use
    V2_s29130() {}
    __forceinline V2_s29130(float a, float b) : V2(a, b) {}
    __forceinline void rotate(double a)
    {
        float c = (float)cos(a);
        float s = (float)sin(a);
        float nx = x * c - y * s;
        y = x * s + y * c;
        x = nx;
    }
};
// @ 0x529130
void MissionBlocker::update_marks()
{
    Agent* a = &(&*g_mission->agents.pages.begin())[(unsigned)g_mission->player_id >> 4][g_mission->player_id & 0xf];

    V2_s29130 base;
    float h = 0.5f / tanf(0.5f * (zoom * 0.017453292f));
    base.y = h * tanf(a->aim_error);
    base.x = 0.0f;
    {
        V2_s29130 p(0.5f, base.y + 0.375f);
        w[0]->move_a(p);
    }
    {
        V2_s29130 p = base;
        p.rotate(2.268928f);
        p.x = p.x + 0.5f;
        p.y = p.y + 0.375f;
        w[1]->move_a(p);
    }
    {
        V2_s29130 p = base;
        p.rotate(-2.268928f);
        p.x = p.x + 0.5f;
        p.y = p.y + 0.375f;
        w[2]->move_a(p);
    }

    float target[8];   // eight slots, seven used (the frame's top dword has no reference)
    target[0] = 0.0f;
    target[1] = 0.0f;
    target[2] = 0.0f;
    target[3] = 0.0f;
    target[4] = 0.0f;
    target[5] = 0.0f;
    target[6] = 0.0f;

    w[0]->hide();
    w[1]->hide();
    w[2]->hide();
    w[3]->hide();
    w[4]->hide();
    w[5]->hide();
    w[6]->hide();

    ItemRef pair = a->slot_pair(0);
    float aim = signed_angle(a->vel, a->in_now.move);

    if (pair.item >= 0) {
        bool other = false;
        if (g_game_keys.entry[13][0] >= 0 && g_keyboard.down[g_game_keys.entry[13][0]]) other = true;
        if ((g_game_keys.entry[13][1] < 0 || g_keyboard.down[g_game_keys.entry[13][1]] == 0) && !other) {
            int type = g_items[pair.item].type_flags & 0xff;
            if ((type == 8 || type == 9 || type == 10 || type == 16 || type == 17) && g_options.i64) {
                if (!cur_at_s29130()->f100_is_0x48_and_charged()) {
                    if ((&*g_mission->agents.pages.begin())[(unsigned)g_mission->player_id >> 4][g_mission->player_id & 0xf].horse == -1 ||
                        (g_aim_range_tbl[a->variant].lo <= aim && aim < g_aim_range_tbl[a->variant].hi)) {
                        target[0] = 0.9f;
                        target[1] = 0.9f;
                        target[2] = 0.9f;
                    }
                }
            } else if (type == 2 || type == 3 || type == 4) {
                if (g_options.i68 || !g_options.ctl_mode) {
                    int t = (int)a->pick_front();
                    if (t >= 0) {
                        int dir = a->pick_attack_dir(t);
                        if (dir >= 0 && dir < 4) {
                            if (dir == 3)
                                target[3] = 0.7f;
                            else if (dir == 1)
                                target[4] = 0.7f;
                            else if (dir == 0)
                                target[5] = 0.7f;
                            else if (dir == 2)
                                target[6] = 0.7f;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < 7; i++) {
        float step = i < 3 ? g_step_tbl[0] * 0.5f : g_step_tbl[0] * 2.5f;
        if (target[i] > color[i]) {
            color[i] += step * 1.2f;
            if (color[i] > target[i]) color[i] = target[i];
        } else if (target[i] < color[i]) {
            color[i] -= step;
            if (color[i] < target[i]) color[i] = target[i];
        }
        if (i < 3)
            marks[i]->color = 0x00ffffff;
        else
            marks[i]->color = 0xffbb11;
        int alpha = (int)(color[i] * 255.0f);
        if (alpha) {
            if (i < 3)
                w[i]->show();
            else
                w[i]->show();
        }
        marks[i]->color |= (unsigned)alpha << 24;
    }
}

// @ 0x5295a0
void MissionCtx::tick()
{
    WL_MARKER(12)
    static_cast<Game*>(g_game)->more_start();
    if (party_battle_mode != 0 || (mission_template >= 0 && (g_mission_templates[mission_template].flags & 0x10))) {
        refresh_markers();
    }
    WL_MARKER(13)
}

// @ 0x529600
void MissionCtx::refresh_markers()
{
    int n = 1;
    if (g_mission->mission_result > 0)
        n = 0x701;
    else if (g_mission->mission_result < 0)
        n = 0x7031;
    g_mission->apply_agent_results(n);
    if (mode == 3 || mode == 4 || mode == 1) {
        reset_party_heroes(g_game->cur_party);
        if (party[0] >= 0) reset_party_heroes(party[0]);
        if (party[1] >= 0) reset_party_heroes(party[1]);
    }
}

// @ 0x529670
void MissionCtx::reset_party_heroes(int party_id)
{
    PartyMgr* mgr = &g_game->parties;
    int idx = mgr->lookup(party_id);
    Party* party = &mgr->pages[idx >> 4][idx & 0xf];

    int n = 0;
    int i = 0;
    while (i < party->nstacks) {
        if (party->stacks[i].flags & 1) break;
        n++;
        i++;
    }

    for (int j = 0; j < n; j++) {
        Troop* t = &g_game->troops[party->stacks[j].troop];
        if (t->flags & 0x10) {
            if (t->hit_points < 0.0f) t->hit_points = 0.0f;
        }
    }
}

// ======================================================= slice s00529840: functions
// the game's party manager and troop table through inline accessors: the original keeps the address in a register across the
// lookup (a direct g_game->parties is reloaded after each call)
static inline PartyMgr* game_parties() { return &g_game->parties; }
static inline Troop* game_troops() { return g_game->troops; }
// @ 0x529840
void Mission::apply_agent_results(int)
{
    unsigned i = agents.first_used();
    while (i < (unsigned)agents.count) {
        Agent* a = agents.pages[i >> 4] + (i & 0xf);
        if (a->kind != 1 && a->kind == 0) {
            if (a->party >= 0 && a->party_stack >= 0) {
                Troop* t = &game_troops()[a->troop];
                if (!(t->flags & 0x10) && a->party == g_game->cur_party && t->upgrade_troop[0] > 0) {
                    // the xp stays an exact integer / x87 value until the _ftol
                    float xp = (float)a->xp_pool;
                    int lv = t->level;
                    int cur = game_troops()[g_game->cur_troop].level;
                    if (lv > cur)
                        xp = xp / ((lv - cur) + 7.0f) * 7.0f;
                    game_parties()->get(a->party)->add_xp_to_stack(a->party_stack, (int)xp);
                }
                t = &game_troops()[a->troop];
                if (t->flags & 0x10) {
                    t->hit_points = a->health;
                    Party* p = game_parties()->at(game_parties()->lookup(a->party));
                    int skill = p->skill_level(9);
                    float v = a->start_health - a->health;
                    if (0.0f > v) v = 0.0f;
                    game_troops()[a->troop].hit_points += (skill * 0.05f + 0.1f) * v;
                }
                if (a->state != 1) {
                    if (a->state == 5)
                        record_killed(a);
                    else if (a->state == 4)
                        record_wounded(a);
                }
            } else {
                if (game_troops()[a->troop].flags & 0x10) {
                    game_troops()[a->troop].hit_points = a->health;
                    if (a->troop == g_game->cur_troop) {
                        Party* p = game_parties()->at(game_parties()->lookup(g_game->cur_party));
                        int skill = p->skill_level(9);
                        float v = a->start_health - a->health;
                        if (0.0f > v) v = 0.0f;
                        game_troops()[a->troop].hit_points += (skill * 0.05f + 0.1f) * v;
                    }
                }
            }
        }
        agents.next_used(&i);
    }
}

// @ 0x529b70
void Mission::record_killed(Agent* a)
{
    remove_from_stack(a, 1);
    if (a->party == g_game->cur_party)
        drop_stack_seed(g_game->stack_seeds, a->party, a->party_stack, a->seed, 0);
}

// @ 0x529bc0
__declspec(noinline) void __stdcall remove_from_stack(Agent* a, int killed)
{
    if (a->party < 0) return;
    int slot = a->party_stack;
    if (slot < 0) return;
    if (killed)
        game_parties()->get(a->party)->stacks[slot].count--;
    else
        game_parties()->get(a->party)->stacks[slot].wounded++;
}

// @ 0x529c30
void Mission::drop_stack_seed(SeedRow* hist, int party_id, int slot, int value, int flag)
{
    Party* p = game_parties()->at(game_parties()->lookup(party_id));
    int found = -1;
    for (int i = 0; i < p->stacks[slot].count; i++) {
        if (found != -1)
            break;
        if (value == hist[slot][i])
            found = i;
    }
    if (found != -1) {
        for (int j = found; j < 0x1f; j++)
            hist[slot][j] = hist[slot][j + 1];
        hist[slot][31] = rand();
        if (flag && p->stacks[slot].count <= 0x20)
            hist[slot][p->stacks[slot].count - 1] = value;
    }
}

// @ 0x529d00
void Mission::record_wounded(Agent* a)
{
    if (a->party >= 0) {
        int slot = a->party_stack;
        if (slot >= 0) {
            Party* p = game_parties()->get(a->party);
            int& wounded = p->stacks[slot].wounded;   // through a reference: the original adds the stack offset first
            wounded++;
        }
    }
    if (a->party == g_game->cur_party)
        drop_stack_seed(g_game->stack_seeds, a->party, a->party_stack, a->seed, 1);
}

// @ 0x529d80
void Mission::sync_scene_props()
{
    SceneFile* sc = &g_game->scenes[scene_idx];
    for (unsigned i = 0; i < props.size(); i++) {
        if (props[i].m10 == 0 && props[i].m14 >= 0) {
            PlacedObj& q = sc->props[props[i].m14];
            props[i].id = q.id;
            props[i].name = q.name;
        }
    }
    std::vector<PlacedObj>& recs = sc->props;
    recs.resize(0);
    for (unsigned i = 0, off = 0; i < props.size(); i++, off += sizeof(PlacedObj)) {
        PlacedObj* r = (PlacedObj*)((char*)&props[0] + off);
        if (r->m10 == 0) {
            recs.push_back(*r);
            r->m14 = (int)recs.size() - 1;
        }
    }
}

// ======================================================= slice s00529ec0: functions
// the length the original inlines in this function: x*x + z*z + y*y (0x52a2af, 0x52a337, 0x52a786)
static inline float len_xzy_s29ec0(const vector4& v)
{
    return (float)sqrt(v.x * v.x + v.z * v.z + v.y * v.y);
}
// an agent of the mission's table (the pages vector indexed through a reference, as the probe's original reads it)
static __forceinline Agent* table_agent_s29ec0(unsigned id)
{
    std::vector<Agent*>& pages = g_mission->agents.pages;
    return pages[id >> 4] + (id & 0x0f);
}
static inline bool area_bit1_s29ec0()
{
    return ((g_mission_templates[g_mission->mode].flags >> 1) & 1) != 0;
}
// @ 0x529ec0
void MissionBlocker::probe_interaction()
{
    contact c;
    Agent* me = table_agent_s29ec0(g_mission->player_id);
    int mode = -1;
    int target = -1;
    float second;

    {
    vector4 pos = me->pos;
    float best;

    // ---- phase A: cast the thin probe 10.5 m ahead and find the nearest face hit ----------------------
    pos.z = me->speed_scale() + pos.z;
    vector4 front = pos + me->in_now.move * 10.5f;
    pos = pos + me->in_now.move * 0.05f;

    capsule cap(1e-4f, &pos, &front);
    matrix ident;
    cap.transform((const ::pos*)identity_pos(&ident));

    union {                                     // one buffer: the face query's shapes, then the entity query's entities
        shape_base* shapes[0x2000];
        rgl::entity* ents[0x2000];
    } found;
    int count;

    best = 1e9f;
    second = 1e9f;
    g_mission->scene->query_segment(&pos, &front, 0x110, found.shapes, &count);
    {
        int n = count;
        for (int i = 0; i < n; i++) {
            if (found.shapes[i]->shape_type() == 2) {
                if (shape_contact_count(found.shapes[i], &cap, 1, &c)) {
                    vector4 hit = c.point;
                    vector4 d = hit - pos;
                    float len = d.normalize();
                    if (len < best) {
                        best = len;
                        mode = -1;
                        second = len;
                    }
                }
            }
        }
    }

    // ---- phase B: the same probe as a 0.1 m capsule_s29ec0, 0.1 m further out, against the entities ---------
    // 0x52a124..0x52a1fa: Vector::normalize inlined, the squares summed x, z, y (not the outlined 0x407d60's x, y, z)
    cap.radius = 0.1f;
    {
        vector4 t = cap.p1 - cap.p0;
        float al = (float)sqrt(t.y * t.y + t.z * t.z + t.x * t.x);
        if (al > 1e-10f) {
            float inv = 1.0f / al;
            t.x = inv * t.x;
            t.y = inv * t.y;
            t.z = inv * t.z;
        } else {
            t.x = 0.0f;
            t.y = 1.0f;
            t.z = 0.0f;
        }
        vector4 s = t * 0.1f;
        cap.p0.x = cap.p0.x + s.x;
        cap.p0.y = cap.p0.y + s.y;
        cap.p0.z = cap.p0.z + s.z;
    }
    best += 1.0f;

    g_mission->scene->collect(9, found.ents, &count);
    for (int i = 0; i < count; i++) {
        rgl::entity* e = found.ents[i];
        if (!e->shape)
            continue;
        contact c;
        if (!shape_contact_count(e->shape, &cap, 1, &c))
            continue;

        vector4 d = c.point - pos;
        float len = len_xzy_s29ec0(d);
        if (!(len <= best))        // 0x52a2d1: test ah,0x41 / jp -- skips on len > best and on NaN
            continue;

        if (e->game_kind == 0x0b) {
            Agent* a = table_agent_s29ec0(e->game_id);
            if (a->kind == 0) {
                // a human: talk target, unless it is the player or an enemy
                if (e->game_id != g_mission->player_id && !me->is_enemy(a)) {
                    mode = 1;
                    target = e->game_id;
                    best = len;
                    vector4 r = e->local.o - pos;
                    second = len_xzy_s29ec0(r);
                }
            } else if (a->kind == 1) {
                // a ridden mount
                if ((unsigned char)g_items[a->item.item].type_flags == 1 && a->rider == -1 && a->state == 1 && a->f5830 == 2) {
                    target = e->game_id;
                    mode = 2;
                    best = d.out_length();
                    vector4 r = e->local.o - pos;
                    second = r.out_length();   // 0x52a420: call 0x407c50 on the second vector as well
                }
            }
        } else if (e->game_kind == 0x0c) {
            int id = e->game_id;
            PlacedObj& p = g_mission->props[id];
            if (p.kind == 0) {
                int flags = g_propkinds[p.id].flags & 0xff;
                if (flags == 5) {
                    mode = flags;
                    best = len;
                    target = id;
                    second = len;
                    continue;
                }
                if (flags == 10 && len < 4.0f) {
                    target = id;
                    mode = 8;
                    best = len;
                    second = len;
                    continue;
                }
            } else if (p.kind == 5) {
                if (g_mission_ctx.passage_menu > 0 && p.m1a4 >= 0 && p.m1a4 < g_menus[g_mission_ctx.passage_menu].count) {
                    target = id;
                    mode = 6;
                    best = len;
                    second = len;
                    continue;
                }
            } else if (p.kind == 6) {
                mode = 7;
                best = len;
                target = id;
                second = len;
                continue;
            }
            if (mode >= 0) {
                best = len;
                second = len;
            }
        }
    }

    // ---- phase C: nothing picked -> mount/dismount, then the four mission borders ---------------------
    if (mode == -1) {
        if (me->horse != -1 && pitch < -0.5519978f && me->vel_xy.y < 1.0f) {
            mode = 3;
        } else if ((g_mission->max_x - 0.5f < me->pos.x && me->in_now.move.x > 0.5f) ||
                   (g_mission->max_y - 0.5f < me->pos.y && me->in_now.move.y > 0.5f) ||
                   (g_mission->min_x + 0.5f > me->pos.x && me->in_now.move.x < -0.5f) ||
                   (g_mission->min_y + 0.5f > me->pos.y && me->in_now.move.y < -0.5f)) {
            mode = 8;
        }
    }
    }

    {
    // ---- phase D: validate --------------------------------------------------------------------------
    int ok = 0;
    if (mode == 1) {
        if (area_bit1_s29ec0() || (g_mission->flags & 1)) {
            if (!g_game->talk_enabled)
                goto no_selection;
        }
        if (g_game->talk_disabled)
            goto no_selection;
        if (second < 3.5f)
            ok = 1;
    } else if (mode == 2) {
        if (me->horse != -1)
            goto no_selection;
        {
            ItemRef& ir = table_agent_s29ec0(target)->item;
            int stat;
            if (ir.item < 0)
                stat = 0;
            else {
                ItemKind* k = &g_items[ir.item];
                stat = k->difficulty;
                if (stat != 0) {
                    int mod = ir.b.mod;
                    if ((unsigned char)k->type_flags == 1) {
                        if (mod == 0x21) stat -= 1;
                        else if (mod == 0x20) stat += 1;
                        else if (mod == 0x24) stat += 2;
                    } else {
                        if (mod == 0x12) stat += 1;
                        else if (mod == 0x13) stat += 2;
                        else if (mod == 0x11) stat += 4;
                    }
                }
            }
            if (stat > g_game->skill_level(g_game->player, 0x18))
                goto no_selection;
            vector4 r = table_agent_s29ec0(target)->pos - me->pos;
            if (len_xzy_s29ec0(r) < 1.3f)
                ok = 1;
        }
    } else if (mode == 4) {
        if (second < 2.5f)
            ok = 1;
    } else if (mode == 5) {
        if (second < 5.0f)
            ok = 1;
    } else if (mode == 6 || mode == 7) {
        if (second < 2.7f)
            ok = 1;
    }
no_selection:

    // ---- phase E: reset the HUD cache ----------------------------------------------------------------
    tstring name;
    int color = -1;
    tstring action;

    if (g_gui.overlay)
        delete g_gui.overlay;
    g_gui.panel_color = -1;
    g_gui.panel_bg = 0xff101010;
    for (int i = 0; i < 80; i++)
        g_gui.lines[i].assign("");
    g_gui.widest = 0.0f;
    g_gui.line_x = -0.03f;
    g_gui.nlines = 0;
    g_gui.overlay = 0;
    g_gui.sel_ent = 0;
    float view[2];
    view[0] = 0.5f;
    view[1] = 0.23f;
    g_gui.set_view(view);

    // ---- phase F: name the selection -----------------------------------------------------------------
    if (mode == 1) {
        Troop* rec = &g_game->troops[table_agent_s29ec0(target)->troop];
        tstring* tr = g_strtable.find(&rec->key);
        if (!tr)
            tr = &rec->name;
        name = *tr;
        if (ok) {
            tstring k("ui_talk");
            action = *g_strtable.lookup(k);
        }
        if (table_agent_s29ec0(g_mission->player_id)->is_enemy(table_agent_s29ec0(target)))
            color = (int)0xffd06050;
        else if (table_agent_s29ec0(g_mission->player_id)->is_friend(table_agent_s29ec0(target)))
            color = (int)0xff60d050;
    } else if (mode == 2) {
        char buf[0x40];
        table_agent_s29ec0(target)->item.name(buf);
        name.assign(buf);
        if (ok) {
            tstring k("ui_mount");
            action = *g_strtable.lookup(k);
        } else if (me->horse == -1) {
            int skill = g_game->skill_level(g_game->player, 0x18);
            if (table_agent_s29ec0(target)->item.difficulty() > skill) {
                tstring k("ui_riding_skill_not_adequate_to_mount");
                action = *g_strtable.lookup(k);
            }
        }
    } else if (mode == 3) {
        tstring k("ui_dismount");
        action = *g_strtable.lookup(k);
    } else if (mode == 4) {
        SceneFile* rec = &g_game->scenes[g_mission->scene_idx];
        int dest = rec->f7c[target];
        if (dest > g_game->num_scenes) {
            tstring k("ui_exit");
            action = *g_strtable.lookup(k);
        } else if (dest >= 0) {
            {
                tstring k("ui_door_to");
                name = *g_strtable.lookup(k);
            }
            name.append(g_game->scenes[rec->f7c[target]].name);
            name.underscores_to_spaces();
            if (ok) {
                tstring k2("ui_open");
                action = *g_strtable.lookup(k2);
            }
        }
    } else if (mode == 5) {
        int inv = g_mission->props[target].value;
        SceneFile* rec = &g_game->scenes[g_mission->scene_idx];
        if (inv >= rec->f100 || inv < 0) {
            {
                tstring k("ui_baggage");
                name = *g_strtable.lookup(k);
            }
            if (ok) {
                tstring k2("ui_access_inventory");
                action = *g_strtable.lookup(k2);
            }
        } else {
            {
                tstring k("ui_chest");
                name = *g_strtable.lookup(k);
            }
            if (ok) {
                tstring k2("ui_open");
                action = *g_strtable.lookup(k2);
            }
        }
    } else if (mode == 6) {
        menu_item* o = &g_menus[g_mission_ctx.passage_menu].items[g_mission->props[target].m1a4];
        name = *o->door_text();   // 0x4c6ca0
        if (ok) {
            tstring k("ui_go");
            action = *g_strtable.lookup(k);
        }
    } else if (mode == 7) {
        PlacedObj& p = g_mission->props[target];
        ItemRef ref;
        ref.item = p.id;
        ref.m4 = p.value;
        char buf[0x40];
        ref.name(buf);
        name.assign(buf);
        if (ok) {
            tstring k("ui_equip");
            action = *g_strtable.lookup(k);
        }
    } else if (mode == 8) {
        name.assign("");
        if (area_bit1_s29ec0()) {
            tstring k("ui_retreat_battle");
            action = *g_strtable.lookup(k);
        } else {
            tstring k("ui_leave_area");
            action = *g_strtable.lookup(k);
        }
    } else if (mode == -1)
        goto publish;

    // ---- phase G: draw -------------------------------------------------------------------------------
    if (name.len)
        g_gui.draw_string(&name, 0.027f, (unsigned)color, 1);
    if (action.len) {
        {
            tstring sep(" ");
            g_gui.draw_string(&sep, 0.01f, 0xffff9d10, 1);
        }
        g_gui.draw_string(&action, 0.035f, 0xffff9d10, 1);
    }

publish:
    // ---- phase H: publish ----------------------------------------------------------------------------
    sel_mode = mode;
    sel_target = target;
    sel_valid = ok;
    }
}

// ======================================================= slice s0052ae70: functions
// a vertex of the ground marker, its arguments as const references: the original builds the temporaries after the pushes and
// loads marker_ground last
static __forceinline int add_ground_vertex(Mission* m, int p, const V2& uv, const unsigned& color, const rgl::vector4& n, const int& zero)
{
    return m->marker_ground->add_vertex(p, &uv, &color, &n, &zero);
}
// @ 0x52ae70
void Mission::init_order_markers()
{
    if (marker_entity == 0) {
        marker_entity = scene->create_entity(9);
        float scale = 100000.0f;
        marker_entity->set_fbc(&scale);

        marker_spikes = new rgl::mesh;
        marker_ribbons = new rgl::mesh;
        marker_poly = new rgl::mesh;

        {
            tstring name("vertex_color_blend_mat");
            marker_spikes->res = g_resmgr.get_material(name);
        }
        marker_ribbons->res = marker_spikes->res;
        marker_poly->res = marker_spikes->res;

        marker_entity->add_mesh(marker_spikes);
        marker_entity->add_mesh(marker_ribbons);
        marker_entity->add_mesh(marker_poly);

        PolyMesh* s = &nav;
        s->polygon_to_mesh(marker_poly);
        s->build_ribbons(marker_ribbons);
        s->build_spikes(marker_spikes);

        scale = 100000.0f;
        marker_entity->set_fbc(&scale);
        marker_entity->visible = 0;
    }

    if (special == 0) {
        special = scene->create_entity(9);

        marker_ground = new rgl::mesh;
        {
            tstring name("vertex_color_blend_mat");
            marker_ground->res = g_resmgr.get_material(name);
        }
        special->add_mesh(marker_ground);

        int p0 = marker_ground->add_point(&rgl::vector4(-1000.0f, -1000.0f, base_z));
        int p1 = marker_ground->add_point(&rgl::vector4(1000.0f, -1000.0f, base_z));
        int p2 = marker_ground->add_point(&rgl::vector4(1000.0f, 1000.0f, base_z));
        int p3 = marker_ground->add_point(&rgl::vector4(-1000.0f, 1000.0f, base_z));

        int v0 = add_ground_vertex(this, p0, V2(0.0f, 0.0f), 0x663333ff, rgl::vector4(0.0f, 0.0f, 1.0f), 0);
        int v1 = add_ground_vertex(this, p1, V2(0.0f, 0.0f), 0x663333ff, rgl::vector4(0.0f, 0.0f, 1.0f), 0);
        int v2 = add_ground_vertex(this, p2, V2(0.0f, 0.0f), 0x663333ff, rgl::vector4(0.0f, 0.0f, 1.0f), 0);
        int v3 = add_ground_vertex(this, p3, V2(0.0f, 0.0f), 0x663333ff, rgl::vector4(0.0f, 0.0f, 1.0f), 0);

        marker_ground->add_face(v0, v1, v2);
        marker_ground->add_face(v0, v2, v3);

        marker_ground->slots[1].f28 = 1;   // the two dirty flags
        marker_ground->slots[0].f28 = 1;

        {
            float scale = 100000.0f;
            special->set_fbc(&scale);
        }
        special->visible = 1;
    }
}

// @ 0x52b300
void MissionBlocker::order_tick()
{
    unsigned id = g_mission->player_id;
    if ((int)id < 0)
        return;

    Mission* m = g_mission;
    Agent* a = m->agents.pages[id >> 4] + (id & 0xf);
    int team = a->team;
    if (team < 0 || team >= 8)
        team = 7;
    if (g_cfg.cheat_mode && g_keyboard.down[42] && g_keyboard.down[29])
        team = 1;

    m->order(team, order);   // 0x49c2c0 (script/script_interpreter.cpp)

    if (order_state == 1) {
        timer_a.start = g_ticks[timer_a.clock] - 750000;   // the order menu's 7.5 s timeout
        order_state = 2;
        draw_order_menu();
    }
}

// @ 0x52b3c0
void MissionBlocker::draw_order_menu()
{
    {
        float view[2];
        view[0] = 0.5f;
        view[1] = 0.4f;
        g_gui.set_overlay(0, 1);
        g_gui.set_view(view);
    }

    {
        tstring s("ui_order_group");
        g_gui.draw_string(g_strtable.lookup(s), 0.035f, 0xfff0e0d0, 1);
    }

    unsigned base = (order_state == 1) ? 0xffffffff : 0xff50ff60;

    unsigned c;
    c = (order == 0) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_hold_position");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (order == 1) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_follow");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (order == 2) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_charge");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (order == 3) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_mount");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (order == 4) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_dismount");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }
}

// ======================================================= slice s0052b5c0: functions
static __forceinline int room_s2b5c0(ItemRef* r) { int n = r->m4 & 0xff; return r->max_amount() - n; }   // ItemRef::room_left (0x434c30), inlined here
// @ 0x52b5c0
bool Mission::order_agent(unsigned agent_id, ItemRef* order)
{
    Agent* agent = agents.pages[agent_id >> 4] + (agent_id & 0xf);
    ItemKind* ok = &g_items[order->item];
    int chosen = -1;

    for (int i = 0; chosen == -1 && i < 4; i++) {
        ItemRef* r = &agent->pairs[i];
        ItemRef slot;
        slot.item = r->item;
        int c = r->m4;
        slot.m4 = c;
        if (slot.item >= 0) {
            unsigned k = g_items[slot.item].type_flags & 0xff;
            if (k == 5 || k == 6 || k == 0x12 || k == 0xa) {
                if (slot.max_amount() - (c & 0xff) > 0)
                    continue;
            } else if (k != 7 || (c & 0xffff) > 0)
                continue;
        }
        chosen = i;
    }

    if (chosen == -1) {
        unsigned k = ok->type_flags & 0xff;
        int cur;
        if (k == 2 || k == 3 || k == 4 || k == 8 || k == 9 || k == 0xa ||
            k == 0x10 || k == 0x11)
            cur = agent->slots[0];
        else if (k == 7)
            cur = agent->slots[1];
        else
            goto L_rot;
        if (cur >= 0) {
            chosen = cur;
            if (chosen != -1)
                goto L_done;
        }
    L_rot:
        {
            int d = agent->slots[0];
            int e = (d + 1) % 4;
            int f = agent->slots[1];
            if (e == f)
                e = (f + 1) % 4;
            if (e == d)
                e = (f + 2) % 4;
            chosen = e;
        }
    L_done: ;
    }

    if (chosen >= 0) {
    ItemRef* slot = &agent->pairs[chosen];
    char newname[128];
    char oldname[128];
    char buf[512];

    if (slot->item >= 0) {
        slot->name(oldname);
        order->name(newname);
        {
            tstring key("ui_dropping_picking_up");
            sprintf(buf, g_strtable.lookup(key)->c_str(), oldname, newname);
        }
        g_gui.add(tstring(buf), -1);

        unsigned k = g_items[slot->item].type_flags & 0xff;
        // 0x52b808..0x52b814: kind 7 with count <= 1 does not stop here, it falls into the generic
        // max_amount test below (0x52b816..0x52b84d)
        if ((k == 7 && (slot->m4 & 0xffff) > 1) ||
            k == 2 || k == 3 || k == 4 || k == 8 || k == 9 || k == 0x10 || k == 0x11 ||
            room_s2b5c0(slot) > 0) {
            matrix m = agent->body->local;
            m.o.z += 0.1f;   // 0x52b86f..0x52b875: [esp+0x64] is matrix +0x38 (o.z), not o.y
            int mode = 6;
            add_placed(&mode, &slot->item, &slot->word, &m, &agent->pool_id);
        }
    } else {
        order->name(newname);
        {
            tstring key("ui_picking_up");
            sprintf(buf, g_strtable.lookup(key)->c_str(), newname);
        }
        g_gui.add(tstring(buf), -1);
    }

    slot->item = order->item;
    slot->m4 = order->m4;
    agent->slot_flag[chosen] = 0;

    unsigned k = g_items[order->item].type_flags & 0xff;
    if (k == 2 || k == 3 || k == 4 || k == 8 || k == 9 || k == 0xa ||
        k == 0x10 || k == 0x11)
        agent->slots[0] = chosen;
    else if (k == 7)
        agent->slots[1] = chosen;

    refresh_agent(agent);   // 0x4f7e00: Mission::refresh_agent(Agent*), this = the saved this
    agent->on_wield_reset();
    return true;
    }

    {
        tstring key("ui_unable_to_take");
        g_gui.add(*g_strtable.lookup(key), -1);
    }
    return false;
}

// =====================================================================================================
//  MissionBlocker::update_ui
// =====================================================================================================
// @ 0x52ba10
void MissionBlocker::update_ui()
{
    if (g_keyboard.down[35] != 0 && g_keyboard.prev[35] == 0)
        g_game->f1a20c = (g_game->f1a20c == 0);

    g_mission->cursor->visible = 0;
    g_mission->marker_entity->visible = 0;

    int m = g_game->editor.game_mode;
    if (m == 0) {
    if (g_game->f1a20c != 0)
        return;

    if (g_keyboard.down[57] == 0 || g_keyboard.prev[57] != 0)
        goto L_bb21;

    g_game->editor.rt_flag = 1 - g_game->editor.rt_flag;
    {
        vector4 v;
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        g_game->editor.rt = v;
    }
    g_game->editor.rt_extra = 0;
    g_app_obj.update_editor_checks();
    if (g_game->editor.rt_flag == 0) {
        g_app_obj.refresh_editor_dialog();
    } else {
        goto L_bb4b;
    }

L_bb21:
    if (g_game->editor.rt_flag == 0 && g_game->editor.flag_c8 == 0) {
        g_mission->edit_obj->visible = 0;
        update_editor();      // 0x5305b0 (map_editor_update.cpp)
        goto L_bbc5;
    }

L_bb4b:
    g_mission->sel_marker->visible = 0;
    update_placement();                      // 0x52f250 (poly_mesh_poly_ray_edit.cpp)
    goto L_bbc5;
    } else if (m == 1 || m == 3 || m == 2) {
        g_mission->sel_marker->visible = 0;
        g_mission->edit_obj->visible = 0;
        this->update_cursor();
    } else if (m == 4) {
        g_mission->marker_entity->visible = 1;
        update_mesh_edit();                      // 0x52cf60 (poly_mesh_pick.cpp)
    }

L_bbc5:
    if (g_game->editor.game_mode == 0 && g_game->editor.rt_flag == 0) {
        if ((float)(g_ticks[this->timer_b.clock] - this->timer_b.start) * 1e-05f > 0.05f) {
            pick_entity();                          // 0x4ed8d0
            this->timer_b.start = g_ticks[this->timer_b.clock];
        }
    }
    if (this->sel_target == -1) {
        g_gui.set_overlay(0, 1);
        float v[2] = { 0.5f, 0.5f };
        g_gui.set_view(v);
    }
}

// @ 0x52ba1d
// The original 0x52ba1d is the 601-byte interior continuation of update_ui: it begins
// at `push esi` with EBX=0 and the ZF from the `cmp eax,ebx` in the 13-byte prologue at
// 0x52ba10, and returns through the caller's epilogue -- it is not a callable function
// on its own.  A standalone C definition cannot reproduce an interior entry point, so
// this is the continuation body translated in full as its own function (own prologue /
// epilogue); the whole state machine is emitted once at 0x52ba10 in MissionBlocker::update_ui.
void MissionBlocker::update_ui_continue()
{
    if (g_keyboard.down[35] != 0 && g_keyboard.prev[35] == 0)
        g_game->f1a20c = (g_game->f1a20c == 0);

    g_mission->cursor->visible = 0;
    g_mission->marker_entity->visible = 0;

    int m = g_game->editor.game_mode;
    if (m == 0) {
    L_bmode0:
    if (g_game->f1a20c != 0)
        return;

    if (g_keyboard.down[57] == 0 || g_keyboard.prev[57] != 0)
        goto L_bb21;

    g_game->editor.rt_flag = 1 - g_game->editor.rt_flag;
    {
        vector4 v;
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        g_game->editor.rt = v;
    }
    g_game->editor.rt_extra = 0;
    g_app_obj.update_editor_checks();
    if (g_game->editor.rt_flag == 0) {
        g_app_obj.refresh_editor_dialog();
    } else {
        goto L_bb4b;
    }

L_bb21:
    if (g_game->editor.rt_flag == 0 && g_game->editor.flag_c8 == 0) {
        g_mission->edit_obj->visible = 0;
        update_editor();      // 0x5305b0 (map_editor_update.cpp)
        goto L_bbc5;
    }

L_bb4b:
    g_mission->sel_marker->visible = 0;
    update_placement();                      // 0x52f250 (poly_mesh_poly_ray_edit.cpp)
    goto L_bbc5;

    } else if (m == 1 || m == 3 || m == 2) {
        g_mission->sel_marker->visible = 0;
        g_mission->edit_obj->visible = 0;
        this->update_cursor();
    } else if (m == 4) {
        g_mission->marker_entity->visible = 1;
        update_mesh_edit();                      // 0x52cf60 (poly_mesh_pick.cpp)
    }

L_bbc5:
    if (g_game->editor.game_mode == 0 && g_game->editor.rt_flag == 0) {
        if ((float)(g_ticks[this->timer_b.clock] - this->timer_b.start) * 1e-05f > 0.05f) {
            pick_entity();                          // 0x4ed8d0
            this->timer_b.start = g_ticks[this->timer_b.clock];
        }
    }
    if (this->sel_target == -1) {
        g_gui.set_overlay(0, 1);
        float v[2] = { 0.5f, 0.5f };
        g_gui.set_view(v);
    }
}

// =====================================================================================================
//  MissionBlocker::update_cursor
// =====================================================================================================
// @ 0x52bc80
void MissionBlocker::update_cursor()
{
    if (g_mission->scene->hf == 0)
        return;

    vector4 pos;
    char ok;
    // the original stores this flag as a byte (setne / mov [esp+8],cl) but passes
    // the whole 4-byte stack slot to terrain.update (mov ecx,[esp+8]); a bool/int
    // union reproduces both the byte store and the dword load.
    union { bool b; int i; } f;

    if (g_keyboard.down[0xe1] == 0 && g_keyboard.down[226] == 0)
        goto L_bd6f;

    if (g_keyboard.axis[1] != 0.0f || g_keyboard.axis[0] != 0.0f) {
        this->find_position(&ok, &pos);
        f.b = (g_keyboard.down[226] != 0);
        if (ok) {
            int mode = 0;
            if (g_game->editor.game_mode == 2)
                mode = 1;
            else if (g_game->editor.game_mode == 3)
                mode = 2;
            if (mode != 0)
                this->scene_dirty = 1;
            g_terrain.update(&g_mission->terrain, &pos,
                             g_mission->object_kind, mode, f.i);
            g_game->editor.dirty = 1;
        }
    }

    if (g_keyboard.down[0xe1] != 0)
        goto L_bd78;

L_bd6f:
    if (g_keyboard.prev[0xe1] != 0)
        goto L_bd8a;
L_bd78:
    if (g_keyboard.down[226] != 0)
        return;
    if (g_keyboard.prev[226] == 0)
        return;
L_bd8a:
    if (this->scene_dirty != 0) {
        g_terrain.conv_build_scene();
        this->scene_dirty = 0;
    }
}

// =====================================================================================================
//  MissionBlocker::find_position
// =====================================================================================================
// The segment query chain with its bodies visible, as the original's unit saw it (world::query_segment 0x439870,
// cell::collect_masked 0x4396b0: game/agent/agent_stats.cpp; cell_grid::walk 0x437290: game/agent/agent_queries.cpp): the end point b and the
// count do not escape, so b[0] is stored after the &count push as in the original
inline __declspec(noinline) void rgl::cell_grid::walk(const float* a, const float* b, int (*cells)[2], int* count)
{
    vec2 d;
    d.x = b[0] - a[0];
    d.y = b[1] - a[1];
    vec2 dir = d;
    float l = (float)sqrt(dir.x * dir.x + dir.y * dir.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        dir.x = inv * dir.x;
        dir.y = inv * dir.y;
    } else {
        dir.x = 0.0f;
        dir.y = 1.0f;
    }
    int step[2], cur[2];
    float tdelta[2], tmax[2], fpos[2];
    float* dd = &dir.x;
    for (int i = 0; i < 2; i++) {
        if (dd[i] > 0.0f)
            step[i] = 1;
        else
            step[i] = -1;
        float inv = 1.0f / dd[i];
        tdelta[i] = (float)fabs(inv * cell_size[i]);
        fpos[i] = (a[i] - lo[i]) / cell_size[i];
        cur[i] = (int)floor(fpos[i]) + 1;
        if (cur[i] < 0)
            cur[i] = 0;
        else if (cur[i] > n[i] - 1)
            cur[i] = n[i] - 1;
        if (a[i] < lo[i]) {
            if (step[i] < 0)
                tmax[i] = 1e10f;
            else
                tmax[i] = (float)fabs((lo[i] - a[i]) * inv);
        } else if (a[i] > hi[i]) {
            if (step[i] > 0)
                tmax[i] = 1e10f;
            else
                tmax[i] = (float)fabs((a[i] - hi[i]) * inv);
        } else {
            float fr = (float)fmod(a[i] - lo[i], cell_size[i]);
            if (step[i] > 0)
                fr = cell_size[i] - fr;
            tmax[i] = (float)fabs((double)(fr * inv));
        }
    }
    int* pc = cur;
    *count = 0;
    cells[0][0] = cur[0];
    cells[0][1] = cur[1];
    float acc = 0.0f;
    (*count)++;
    int n = *count;     // the incremented count is a temporary copied into n (mov edi,eax)
    float total = (float)sqrt(d.y * d.y + d.x * d.x);
    while (total > tmax[0] + acc || total > tmax[1] + acc) {
        int idx;
        if (tmax[0] < tmax[1])
            idx = 0;
        else
            idx = 1;
        cur[idx] += step[idx];
        cells[n][0] = cur[0];
        cells[n][1] = cur[1];
        n = *count + 1;
        *count = n;
        float* tp = &tmax[idx];
        float* to = &tmax[1 - idx];
        *to -= *tp;
        acc += *tp;
        if (pc[idx] == 0)
            *tp = 1e10f;
        else if (pc[idx] == this->n[idx] - 1)
            *tp = 1e10f;
        else
            *tp = tdelta[idx];
    }
}

// max by value over const references (the shape of the engine's own helper: std::max returns a reference and leaves
// stack temporaries, the windows.h macro tests v > 0)
static inline int max_of(const int& a, const int& b) { return a < b ? b : a; }
inline __declspec(noinline) void rgl::cell::collect_masked(unsigned mask, shape_base** out, int* count)
{
    int n = 0;
    shape_base* buf[0x2000];
    unsigned i = 0;
    int k = 0;
    while (i < a.size()) {
        shape_base* sh = a[i];
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        if (k < *count) {
            shape_base* o = out[k];
            if (sh >= o) {
                if (sh == o) {
                    i++;
                    buf[n++] = sh;
                } else {
                    buf[n++] = o;
                }
                k++;
                continue;
            }
        }
        buf[n++] = sh;
        i++;
    }
    if (k < *count) {
        int m = *count - k;
        memcpy(&buf[n], &out[k], m * sizeof(shape_base*));
        n += m;
    }
    *count = n;
    for (int j = 0; j < *count; j++)
        out[j] = buf[j];
}

// shapes of this cell containing the point p (xy box, or the 3-D box via in_box when *use3d != 0)
inline __declspec(noinline) void rgl::world::query_segment(const vector4* a, const vector4* b, unsigned mask, shape_base** out, int* count)
{
    int pairs[0x2000][2];
    int n;
    walk(&a->x, &b->x, pairs, &n);
    *count = 0;
    for (int i = 0; i < n; i++)
        cells[pairs[i][0]][pairs[i][1]].collect_masked(mask, out, count);
}

// @ 0x52bdb0
void MissionBlocker::find_position(char* ok, vector4* pos)
{
    *ok = 0;

    vector4 near_pt, far_pt;
    name_widget->screen_to_world(&near_pt, &far_pt);

    capsule cap(0.0f, &near_pt, &far_pt);
    matrix m;
    cap.transform((const ::pos*)identity_pos(&m));

    vec2 b;                                 // 8 bytes: query_segment reads only the segment's x/y (the original's
    b.x = far_pt.x;                         // frame has far_pt immediately after it, and no b.z / b.w slot)
    b.y = far_pt.y;
    vector4 p;
    p.x = near_pt.x;
    p.y = near_pt.y;

    float best = 1000000.0f;
    shape_base* shapes[8192];
    int n;
    {
        int count;
        g_mission->scene->query_segment(&p, (const vector4*)&b, 0x100, shapes, &count);
        n = count;
    }
    for (int i = 0; i < n; i++) {
        if (shapes[i]->shape_type() == 2) {
            face* s = static_cast<face*>(shapes[i]);   // shape_type 2: a face
            if (shape_contact_count(s, &cap, 0, 0)) {
                s->intersect(&p, &near_pt, &far_pt);
                float dx = p.x - near_pt.x;
                float dy = p.y - near_pt.y;
                float dz = p.z - near_pt.z;
                float d = (float)sqrt(dx * dx + dy * dy + dz * dz);
                if (d < best) {
                    best = d;
                    *pos = p;
                    *ok = 1;
                }
            }
        }
    }
}
