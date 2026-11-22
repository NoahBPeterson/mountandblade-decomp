//-----------------------------------------------------------------------------
// File: mission_frame_update.cpp  (0x525160..0x5284a0)
//
// Desc: game-loop event handler, dialogue / conversation update, speaker and listener finders, the mission screen frame tick (0x5261e0), scene .sco save path
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
__forceinline bin_writer::bin_writer() {}   // inline here (resource_manager.cpp load_pack calls the out-of-line 0x417c10)
// the mission template's bit 1 (Mission::mode_flag1 0x435840's test, inline here: shr; test r8,1)
static inline bool template_flag1(int id)
{
    return ((g_mission_templates[id].flags >> 1) & 1) != 0;
}
// The agent table's walkers: the original's bodies were visible here (cl keeps g_mission in a register across them,
// as they write no global), though it left them out of line. agent2/agent_ai.cpp owns the rows at these addresses.
inline __declspec(noinline) void AgentTable::next_used(unsigned* i)
{
    ++*i;
    while ((int)*i < count) {
        unsigned v = *i;
        if (pages[v >> 4][v & 0xf].in_use != 0) break;
        ++*i;
    }
}
inline __declspec(noinline) int AgentTable::first_used()
{
    unsigned v = 0;
    if (0 < count) {
        Agent** pg = &pages[0];
        do {
            if (pg[v >> 4][v & 0xf].in_use != 0) break;
            v++;
        } while ((int)v < count);
    }
    return v;
}
// GameKeys::clicked 0x48ac90 (game/ui/widgets/widget_base.cpp owns the row): its body was visible here too; the screen tick keeps
// g_mission and g_game in registers across its calls.
inline __declspec(noinline) bool GameKeys::clicked(int i)
{
    bool r = false;
    int a = entry[i][0];
    if (a >= 0 && g_keyboard.down[a] != 0 && g_keyboard.prev[a] == 0)
        r = true;
    int b = entry[i][1];
    if (b >= 0 && g_keyboard.down[b] != 0 && g_keyboard.prev[b] == 0)
        r = true;
    return r;
}
inline __declspec(noinline) Blow* Blow::init()      // 0x4353a0: visible too (game/agent/agent_queries.cpp owns the row)
{
    vector4 zero(0.0f, 0.0f, 0.0f);
    vector4 unit(1.0f, 0.0f, 0.0f);
    pos = zero;
    dir = unit;
    dir2 = unit;
    flags = 0;
    attacker = -1;
    bone = 0;
    dmg_in = 0;
    kind = 0;
    item = -1;
    missile = 0;
    dmg_out = 0;
    absorbed = 0;
    return this;
}

// ======================================================= slice s00525160: functions
// @ 0x525160
void Rec1208::frame_update(float code_f)
{
    g_gui.flush();

    if (g_gui.wheel != 0)
    {
        // the wheel scrolls the choices: their scroll bar's value moves by the wheel over its axis extent (the bar's
        // on_value_changed 0x4d5940, inline here)
        UiD* o = question->follow[1];
        float* pa = &o->rect_max[o->f178];
        float v = (o->v168 - o->f16c)
                / (o->f170 - o->f16c)
                * (*pa - o->f17c)
                + (((float)g_gui.wheel / *pa) * o->f17c)
                  * 0.0005f;
        o->set_value(v);   // 0x48baa0: UiD::set_value, this = the object o (ecx is still o at the call)
        question->follow[1]->set_key_flags(0x10000);
    }

    if (speaker_id == g_game->player || g_game->talk_info_shown == 0)
        goto hide;
    if (!(g_gui.mouse[0] < window->rect_max.x))   // jp on mask 5: NaN hides too (0x525227)
        goto hide;
    if (!(window->position.y + 0.1f < g_gui.mouse[1]))
        goto hide;
    {
        msg_window->show();
        if (g_game->talk_relation_bar < -0x65 || g_game->talk_relation_bar > 0x65)
        {
            relation_bar->hide();
            reln_pointer->hide();
        }
        else
        {
            V2 pair((float)(g_game->talk_relation_bar + 0x64) * 0.00105f + 0.02f, 0.02f);
            reln_pointer->move_a(pair);
            relation_bar->show();
            reln_pointer->show();
        }
    }
    {
        for (int i = 0; i < 5; ++i)
        {
            if (g_game->talk_line_set[i] == 0)
                continue;
            lines[i]->set_text(g_game->talk_lines[i]);   // the extent it returns is not used (0x525301)
            lines[i]->get_xy();
            g_game->talk_line_set[i] = 0;   // the calls above may have swapped the global: 0x525322 re-reads it
        }
    }
    goto done;
hide:
    msg_window->hide();
done:

    g_mission->f6554 = 1;   // +0x6554

    {
    int res = -1;
    int v = res;
    if (g_dialogs.partner_agent >= 0)
    {
        g_mission->agents[g_dialogs.partner_agent].ai.target = g_mission->player_id;
        g_mission->agents[g_dialogs.partner_agent].ai.move_mode = 2;
        g_mission->agents[g_dialogs.partner_agent].ai.path_state = 0;
        g_mission->agents[g_dialogs.partner_agent].start_attack(0);
    }

    g_mission->frame(code_f, false);

    {
        if (g_mission->player_id >= 0)
        {
            g_mission->agents[g_mission->player_id].hide();
            g_mission->agents[g_mission->player_id].in_now.bits = 0;
        }
    }

    if (n_lines > 1)
    {
        int i;
        for (i = 0; i < 0x400; ++i)
        {
            if (buttons[i]->clear_mask(0x10000) && i < n_lines) {
                refresh(i, &res);
                v = res;
            }
        }
    }
    else if (g_keyboard.down[0xe0] == 0 && g_keyboard.prev[0xe0] != 0)
    {
        refresh(-2, &res);
        v = res;
    }

    update_camera();

    if (v != 1 && !(g_cfg.enable_edit_mode != 0 && g_keyboard.down[15] != 0 && g_keyboard.prev[15] == 0))
    {
    if (v == 0x10)
    {
        slot_as<head_rec>(26)->w18 = 0x10;
        g_windows.set_state(0x1a);
        return;
    }

    if (v == 4)
    {
        unsigned __int64 flags = g_game->troops[g_dialogs.fc].flags30;
        Troop& tr = g_game->troops[g_dialogs.fc];
        if ((flags & 0x10) == 0 || (flags & 0x1000) == 0)   // 0x525568..0x525574: both bits set skips the loop
        {
            for (int i = 0; i < 0xa; ++i)
                tr.unequip(i);
        }
        slot_as<head_rec>(8)->w18 = 0x10;
        slot_as<head_rec>(8)->w14 = g_dialogs.fc;
        slot_as<head_rec>(8)->w04 = 2;
        g_windows.set_state(8);
        return;
    }

    if (v != 3)
    {
        if (v != 2)
        {
            if (v != 5 && v != 7 && v != 6 && v != 9 && v != 0xa)
            {
                if (v == 0xb || v == 0xc)
                {
                    slot_as<head_rec>(15)->w18 = 0x10;
                    slot_as<head_rec>(15)->w14 = g_dialogs.fc;
                    slot_as<head_rec>(15)->w04 = (v != 0xb) + 1;
                    slot_as<head_rec>(15)->slot_empty_4();
                    g_windows.set_state(0xf);
                }
                return;
            }
            slot_as<head_rec>(12)->w18 = 0x10;
            if (v != 5 && v != 7 && v != 6)
            {
                slot_as<head_rec>(12)->w10 = -1;
                slot_as<head_rec>(12)->w14 = g_dialogs.fc;
            }
            else
                slot_as<head_rec>(12)->w10 = g_dialogs.f10;
            int mode = 0;
            if (v == 5)
                mode = 1;
            else if (v == 7)
                mode = v;
            else if (v == 6)
                mode = v;
            else if (v == 9)
                mode = 2;
            else if (v == 0xa)
                mode = 3;
            slot_as<head_rec>(12)->w04 = mode;
            g_windows.set_state(0xc);
            return;
        }
    }
    else
        target = g_dialogs.fc;
    {
        slot_as<head_rec>(8)->w18 = 0x10;
        int a = g_game->screen_troop;
        if (a == 0)
            a = g_dialogs.fc;
        slot_as<head_rec>(8)->w14 = a;
        slot_as<head_rec>(8)->w04 = ((v != 3) ? 0 : 3) + 1;
        g_windows.set_state(8);
        return;
    }
    }

    {
        int x = w18;
        if (x == 0x64)
            g_map_flags.commit((char*)&code_f);
        else
            g_windows.set_state(x);
    }
    }
}

// ======================================================= slice s005257f0: functions
// @ 0x526070 -------------------------------------------------------------------------------------------
int Rec1208::find_speaker_agent()
{
    int found = -1;
    int speaker = g_dialogs.speaker();
    unsigned __int64 flags = g_game->troops[speaker].flags30;
    bool any = false;
    if ((flags & 0x10) != 0 || g_dialogs.partner_agent == -1) {
        unsigned i = g_mission->agents.first_used();
        if (i < (unsigned)g_mission->agents.count) {
            do {
                if ((g_mission->agents.pages[i >> 4] + (i & 0xf))->troop == speaker) {
                    found = i;
                    any = true;
                }
                g_mission->agents.next_used(&i);
            } while (i < (unsigned)g_mission->agents.count);
        }
    }
    if (!any)
        return g_dialogs.partner_agent;
    return found;
}

// cross product as the view update inlines it: three sequential stores into a temporary
static __forceinline vector4 cross_s257f0(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - b.z * a.x;
    r.z = b.y * a.x - a.y * b.x;
    return r;
}
static __forceinline vector4 vsub_s257f0(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}
static __forceinline void vsub_to_s257f0(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
}
static __forceinline void vadd_s257f0(vector4& a, const vector4& b) { a.x += b.x; a.y += b.y; a.z += b.z; }
static __forceinline float vdot_neg_s257f0(const vector4& a, const vector4& b) { return -a.x * b.x + -a.z * b.z + -a.y * b.y; }
// 0x407d60 (engine/physics/collision_shapes.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
namespace rgl {
inline __declspec(noinline) float vector4::normalize()      // 0x407d60
{
    float len = length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
}

// @ 0x5257f0 -------------------------------------------------------------------------------------------
__declspec(noinline) void Rec1208::update_camera()
{
    unsigned target = find_speaker_agent();
    int listener = find_listener_agent();
    if (target == (unsigned)-1)
        return;

    vector4 G = g_mission->agents[target].body->local.o;
    vector4 F;
    G.z = g_mission->agents[target].speed_scale() + G.z;

    if (target == (unsigned)(g_dialogs.partner_agent)) {
        vector4 x;
        vsub_to_s257f0(x, g_mission->agents[g_mission->player_id].pos, g_mission->agents[target].pos);
        float len = x.normalize();
        F = x;                                // the look direction (0x5258e1..0x5258fa copy all four dwords)
        if (1.0f < len)
            len = 1.0f;
        vadd_s257f0(G, x * len);
    } else {
        F = g_mission->agents[target].body->local.f;
        vadd_s257f0(G, F);
        if (listener != -1) {
            vector4 x = g_mission->agents[listener].body->local.o;
            {
                vector4 L(x.x + 0.01f, x.y + 0.01f, (g_mission->agents[listener].speed_scale() - 0.1f) + x.z);
                vsub_to_s257f0(x, L, G);
            }
            float dist = x.normalize();
            float t = vdot_neg_s257f0(F, x);
            if (dist < 1.4f) {
                vector4 b = x + F * t;
                b.normalize();
                vsub_to_s257f0(x, G, F);
                vector4 half = b * 0.5f;
                ((Vec3*)&G)->sub_assign(*(const Vec3*)&half);
                F = G - x;
                F.normalize();
            }
        }
    }

    // orthonormal frame with up = (0,0,1); cross() expanded inline
    vector4 D;
    {
        vector4 up;
        up = vector4(0.0f, 0.0f, 1.0f);
        D = cross_s257f0(up, F);
        D.normalize();
    }
    {
    vector4 E;
    E = cross_s257f0(F, D);

    if ((world.o - G).length() > 0.3f || (world.u - F).length() > 0.1f)
        blend = blend - g_step_tbl[2] * 4.0f;

    float t = clamp01(1.0f - blend);
    if (!started) {
        started = 1;
        t = t * g_step_tbl[2] * 1.5f;
    }
    t = clamp01(t);
    float inv = 1.0f - t;

    world.o = G * t + world.o * inv;
    world.s = D * t + world.s * inv;
    world.f = E * t + world.f * inv;
    world.u = F * t + world.u * inv;

    world.orthonormalize();
    fr.m = world;
    float far_plane = 2.0f;
    g_mission->camera_to((const rgl::matrix*)&world, &far_plane);

    window->scene3d.fr = fr;

    blend = (g_step_tbl[2] + g_step_tbl[2]) + blend;
    blend = blend < 0.0f ? 0.0f : (blend >= 1.0f ? 1.0f : blend);
    }
}

// ======================================================= slice s00526130: functions
// @ 0x526130 -------------------------------------------------------------------------------------------
int Rec1208::find_listener_agent()
{
    int found = -1;
    int listener = g_dialogs.listener();
    unsigned __int64 flags = g_game->troops[listener].flags30;
    if ((flags & 0x10) != 0 || g_dialogs.partner_agent == -1) {
        unsigned i = g_mission->agents.first_used();
        unsigned count = g_mission->agents.count;
        if (i < count) {
            do {
                if ((g_mission->agents.pages[i >> 4] + (i & 0xf))->troop == listener)
                    found = i;
                g_mission->agents.next_used(&i);
            } while (i < count);
        }
    }
    return found;
}

// ======================================================= slice s005261e0: functions
#undef g_strtable_s261e0
#define g_strtable_s261e0 g_strtable
#undef g_clamp_max
#define g_clamp_max g_skill_tbl[36].max_rank             // 0x8ee3ac4

// Timer::elapsed through a forced-inline call: cl then keeps g_mission in ecx and adds the member offset to it (add ecx,
// 0x6328), as the original; the direct g_mission->clock.elapsed() picks eax and a lea.
// An agent of the mission through a forced-inline call: the conversation start reads the partner's troop three times,
// and cl orders the third load's address operands as the original ([page + index]) only with the call boundary; the
// plain g_mission->agents[id] there gives [index + page].
static __forceinline Agent* mission_agent(Mission* m, unsigned id) { return &m->agents[id]; }
static __forceinline float timer_elapsed(const Timer* t) { return t->elapsed(); }
// A key or its alternate went down this frame (the bindings' two codes). The loops of this function are do-while loops:
// with the `for` forms (this walk, the three-slot group loops, the agent scan) cl folds every vector operator[] page
// load of the function into its use, where the original keeps `lea r,[first+i*4]; mov r,[r]` (docs/codegen.md 6.6).
static __forceinline bool pressed_s261e0(const int* keys, int n)
{
    bool r = false;
    const int* p = keys;
    do {
        int k = *p;
        if (k >= 0 && g_keyboard.down[k] != 0 && g_keyboard.prev[k] == 0)
            r = true;
        p++;
    } while ((intptr_t)p < (intptr_t)(keys + n));
    return r;
}
// MapFlags::commit 0x513710 and MissionSource::pick 0x4d5410 ignore their argument (commit overwrites the slot for its tail
// call), and the callers pass the address of an unnamed temporary: here a byte that shares flag13's slot and a dword, in
// Rec1208::frame_update a byte in the parameter's home. So the parameters are `const T&` of an empty class (commit) and of a
// 4-byte class with an empty constructor (pick); these are the temporaries, passed through the headers' pointer types.
struct CommitArg_s261e0 { CommitArg_s261e0() {} };
struct PickArg_s261e0 { int v; PickArg_s261e0() {} };
// @ 0x5261e0
void MissionBlocker::frame_update(float dt)
{
    if (g_mt_inited)
        EnterCriticalSection(&g_mt_cs);

    {
    float sdt = dt;

    // ---- key toggles (slow-motion / pause) ----------------------------------------------------------
    if (g_cfg.cheat_mode != 0 && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) && g_keyboard.down[67] != 0 && g_keyboard.prev[67] == 0)
        cheat_slowmo = (unsigned char)!cheat_slowmo;
    if (g_cfg.cheat_mode != 0 && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) && g_keyboard.down[87] != 0 && g_keyboard.prev[87] == 0)
        cheat_freeze = (unsigned char)!cheat_freeze;
    if (cheat_freeze)
        sdt = 0.0f;
    else if (cheat_slowmo)
        sdt = dt * 0.2f;

    // ---- scene time-scale blend --------------------------------------------------------------------
    if (g_cfg.cheat_mode) {
        if (g_mission->time_speed_anim_time > 0.0f) {
            __int64 d = g_ticks[g_mission->time_speed_timer.clock] - g_mission->time_speed_timer.start;
            float r = (float)d * 1e-5f;
            r = r / g_mission->time_speed_anim_time;      // 0x52630b..0x526311: fmul then fdiv (the fused expression compiles to fdiv then fmul)
            if (r >= 1.0f) {
                g_mission->time_speed = g_mission->target_time_speed;
                sdt = sdt * g_mission->time_speed;
                g_mission->time_speed_anim_time = 0.0f;
            } else {
                sdt = (r * g_mission->target_time_speed + (1.0f - r) * g_mission->time_speed) * sdt;
            }
        } else {
            sdt = sdt * g_mission->time_speed;
        }
    }

    // ---- renderer "ready" rate keys ----------------------------------------------------------------
    {
        char c7 = g_keyboard.is_ready(0xd0);
        unsigned char c8 = g_keyboard.b840;
        if (c7 || (g_keyboard.down[0xd0] != 0 && c8 != 0))
            rgl::g_vert_budget -= 5;
        c7 = g_keyboard.is_ready(0xc8);
        if (c7 || (g_keyboard.down[0xc8] != 0 && c8 != 0))
            rgl::g_vert_budget += 5;
    }

    update_view(sdt);
    g_mission->f6554 = 0;
    g_mission->frame(sdt, true);
    if (g_mission->player_id >= 0) {
        update_status();
        update_marks();
    }
    update_camera(sdt);
    g_gui.flush();
    }

    // ---- no-camera position save -------------------------------------------------------------------
    if (g_cfg.enable_edit_mode && (g_keyboard.down[29] || g_keyboard.down[157]) && g_keyboard.down[18] && !g_keyboard.prev[18]) {
        if (g_game->editor.active == 0) {
            if (g_mission->player_id >= 0) {
                g_game->editor.cam_pos = (g_mission->agents.pages[(unsigned)g_mission->player_id >> 4] + (g_mission->player_id & 0xf))->pos;
                g_game->editor.cam_pos.z += 1.5f;
            } else {
                g_game->editor.cam_pos = head;
            }
            g_game->f139e4 = 0;
            g_game->editor.active = 1;
            g_game->f1a20c = 0;
            g_game->editor.sel_index = -1;
            g_game->editor.sel.resize(0, 0);
            g_game->editor.dirty = 0;
            g_mission->init_order_markers();
        } else {
            restore_defaults();
            if (g_mt_inited) {
                LeaveCriticalSection(&g_mt_cs);
                SetEvent(g_mt_event);
            }
            return;
        }
    }
    if (g_game->editor.active != 0) {
        update_ui();
        if (g_mt_inited) {
            LeaveCriticalSection(&g_mt_cs);
            SetEvent(g_mt_event);
        }
        return;
    }

    {
    // ---- "mission started" pending item ------------------------------------------------------------
    bool  b6;               // local at esp+0x12
    if (g_mission->clock_set == 0) {
        b6 = true;
    } else {
        float e = timer_elapsed(&g_mission->clock);
        if (!(0.0f <= e)) {
            b6 = true;
        } else {
            b6 = false;
            if (*(g_windows.dq.end() - 1) == 0)
                b6 = true;
        }
    }
    if (g_pending_action != -1) {
        if (g_pending_action == 0) {
            g_mission->nav.build_geometry();
            g_mission->save_scene_data();
            g_mission->sync_scene_props();
            g_game->save_scene(g_mission->scene_idx);
        } else {
            b6 = false;
        }
        g_pending_action = -1;
    }

    // ---- mission-editor key block ------------------------------------------------------------------
    if (g_mission->player_id >= 0) {
        if (g_keyboard.down[29] == 0 && g_keyboard.down[157] == 0) {
            if (g_game_keys.clicked(0x15)) {
                order = 0;
                order_tick();
            } else if (g_game_keys.clicked(0x16)) {
                order = 1;
                order_tick();
            } else if (g_game_keys.clicked(0x17)) {
                order = 2;
                order_tick();
            } else if (g_game_keys.clicked(0x1a)) {
                order = 5;
                order_tick();
            } else if (g_game_keys.clicked(0x1b)) {
                order = 6;
                order_tick();
            } else if (g_game_keys.clicked(0x1c)) {
                order = 7;
                order_tick();
            } else if (g_game_keys.clicked(0x1d)) {
                order = 8;
                order_tick();
            } else if (g_game_keys.clicked(0x18)) {
                WL_MARKER(12)
                if (g_mission->ordered_dismount == 0) {
                    order = 4;
                    g_mission->ordered_dismount = 1;
                } else {
                    order = 3;
                    g_mission->ordered_dismount = 0;
                }
                order_tick();
                WL_MARKER(13)
            } else if (g_game_keys.clicked(0x1e)) {
                WL_MARKER(12)
                if (g_mission->ordered_blunt == 0) {
                    order = 9;
                    g_mission->ordered_blunt = 1;
                } else {
                    order = 10;
                    g_mission->ordered_blunt = 0;
                }
                order_tick();
                WL_MARKER(13)
            } else if (g_game_keys.clicked(0x19)) {
                WL_MARKER(12)
                if (g_mission->ordered_hold_fire == 0) {
                    order = 0xc;
                    g_mission->ordered_hold_fire = 1;
                } else {
                    order = 0xd;
                    g_mission->ordered_hold_fire = 0;
                }
                order_tick();
                WL_MARKER(13)
            }
        }
        if (g_game_keys.clicked(0x1f) || g_game_keys.clicked(0x20) || g_game_keys.clicked(0x21) ||
            g_game_keys.clicked(0x22) || g_game_keys.clicked(0x23)) {
            bool mod = (g_keyboard.down[29] || g_keyboard.down[157] || g_keyboard.down[42] || g_keyboard.down[54]);
            bool b3 = false;
            if (!mod)
                g_game->editor.f1a1d4 = 0;
            else {
                b3 = true;
                g_game->editor.f1a1d4 = 1;
            }

            int team = (g_mission->agents.pages[(unsigned)g_mission->player_id >> 4] + (g_mission->player_id & 0xf))->team;
            if (team < 0 || 8 <= team)
                team = 7;
            // the three class blocks' `use` flags
            TeamRec* base = &g_mission->teams[team];

            if (g_game_keys.clicked(0x23)) {
                bool changed = false;
                for (int k = 0; k < 3; k++) {
                    if (!(base->cls[k].use == 0)) {
                        base->cls[k].use = 0;
                    } else {
                        base->cls[k].use = 1;
                        changed = true;
                    }
                }
                if (!changed) {
                    for (int k = 0; k < 3; k++) {
                        base->cls[k].use = 1;
                    }
                }
            } else if (g_game_keys.clicked(0x1f)) {
                for (int k = 0; k < 3; k++) {
                    base->cls[k].use = 1;
                }
            } else {
                if (!b3) {
                    for (int k = 0; k < 3; k++) {
                        base->cls[k].use = 0;
                    }
                }
                int idx = 0;
                if (g_game_keys.clicked(0x20))
                    idx = 0;
                else if (g_game_keys.clicked(0x21))
                    idx = 1;
                else if (g_game_keys.clicked(0x22))
                    idx = 2;
                base->cls[idx].use = 1;
            }

            bool all = true;
            for (int k = 0; k < 3; k++) {
                                if (base->cls[k].use == 0)
                    all = false;
                }
            if (all) {
                tstring t("ui_everyone_hear_me");
                g_gui.add(*g_strtable.lookup(t), 0xffddee88);
            } else {
                char label[512];
                base->make_label(label);
                char buf[512];
                {
                    tstring t("ui_str_hear_me");
                    sprintf(buf, g_strtable.lookup(t)->c_str(), label, buf);
                }
                g_gui.add(tstring(buf), 0xffddee88);
            }
        }
    }

    // ---- region-change / texture timers ------------------------------------------------------------
    {
        __int64 d = g_ticks[timer_a.clock] - timer_a.start;
        if ((float)d * 1e-5f > g_f590b38 && g_mission->player_id >= 0) {
            order_state = 0;
            __int64 d2 = g_ticks[timer_b.clock] - timer_b.start;
            if ((float)d2 * 1e-5f > 0.1f) {
                probe_interaction();
                timer_b.start = g_ticks[timer_b.clock];
            }
            if (sel_mode == -1) {
                g_gui.set_overlay(0, 1);
                float v[2];
                v[0] = 0.5f;
                v[1] = 0.5f;
                g_gui.set_view(v);
            }
        }
    }

    // ---- item-use keys + sound slots ---------------------------------------------------------------
    unsigned char flag13 = 0;   // esp+0x13 ("value valid")
    int val14 = -1;             // esp+0x14, the slot sdt had
    if (g_mission->player_id >= 0) {
        if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 4 && sel_valid != 0) {
            WL_MARKER(12)
            int v = g_game->scenes[g_mission->scene_idx].f7c[sel_target];
            if (v == 100000)
                b6 = false;
            else {
                g_game->jump_scene = v;
                g_game->jump_entry = 0;
            }
            WL_MARKER(13)
        } else if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 1 && sel_valid != 0) {
            WL_MARKER(12)
            g_mission->conversation = sel_target;
            WL_MARKER(13)
        } else if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 7 && sel_valid != 0) {
            WL_MARKER(12)
            PlacedObj& it = g_mission->props[sel_target];
            ItemRef pair;
            pair.item = it.id;
            pair.m4 = it.value;
            if (g_mission->order_agent(g_mission->player_id, &pair)) {   // 0x52b5c0
                g_mission->release_item(sel_target);
                update_equipment();
            }
            WL_MARKER(13)
        } else if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 5 && sel_valid != 0) {
            PlacedObj& rec = g_mission->props[sel_target];
            WL_MARKER(12)
            int v = rec.value;
            if (v >= g_game->scenes[g_mission->scene_idx].f100 || v < 0)
                val14 = -1;
            else
                val14 = g_game->scenes[g_mission->scene_idx].fc0[v];
            flag13 = 1;
            WL_MARKER(13)
        } else if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 6 && sel_valid != 0) {
            WL_MARKER(12)
            int idx = g_mission->props[sel_target].value;
            g_game->jump_entry = idx;
            int block = g_mission->props[sel_target].m1a4;
            WL_MARKER(13)
            g_menus[g_mission_ctx.passage_menu].items[block].conseq.play(0x10);
        }

        if (pressed_s261e0(g_game_keys.entry[16], 2)) {
            g_script.trigger_result.value = 0;
            TriggerVec& tv = g_mission_templates[g_mission->mode].triggers;
            for (int i = 0; i < tv.count; i++)
                tv.data[i].update(-0x16);
            if (g_script.trigger_result.value != 0)
                flag13 = 1;
        }
    }

    if (g_cfg.enable_edit_mode && g_keyboard.down[56] && g_keyboard.down[38] == 0 && g_keyboard.prev[38] != 0)
        g_mission->light_entities(g_mission->interior, g_mission->num_interior);   // 0x47ce00: thiscall member, this = the mission (ecx at the call)

    if (f2d8 == 0) {
        b6 = false;
    } else if (leave_answer == 0) {
        TriggerVec& tv = g_mission_templates[g_mission->mode].triggers;
        for (int i = 0; i < tv.count; i++)
            tv.data[i].update(-0x1e);
        b6 = false;
    }

    if (pressed_s261e0(g_game_keys.entry[11], 2) && g_keyboard.down[56] == 0 && g_keyboard.down[184] == 0) {
        g_script.trigger_result.value = 0;
        g_mission_templates[g_mission->mode].triggers.update_all(-0x15);
        if (g_script.trigger_result.value != 0)
            b6 = false;
    }

    if (g_game->question_answer >= 0) {
        int v = g_game->question_answer;
        g_script.param1 = v;
        TriggerVec& tv = g_mission_templates[g_mission->mode].triggers;
        for (int i = 0; i < tv.count; i++)
            tv.data[i].update(-8);
        g_game->question_answer = -1;
    }

    // ---- "leave area" / agent selection ------------------------------------------------------------
    if ((g_mission->flags & 1) != 0) {
        if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 8)
            leave_answer = 0;
    } else if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 8 &&
               (!template_flag1(g_mission->mode) || g_mission->mission_result > 0)) {
        leave_answer = 0;
    } else if (template_flag1(g_mission->mode) && g_mission->player_id >= 0 &&
               g_game_keys.clicked(4) && sel_mode == 8) {
        float best = 10000.0f;
        unsigned id = g_mission->agents.first_used();
        if (id < (unsigned)g_mission->agents.count) do {
            Agent* a = (g_mission->agents.pages[(unsigned)id >> 4] + (id & 0xf));
            if (a->state == 1 && a->kind == 0 && (g_mission->agents.pages[(unsigned)g_mission->player_id >> 4] + (g_mission->player_id & 0xf))->is_enemy(a)) {
                vector4 d = a->pos - (g_mission->agents.pages[(unsigned)g_mission->player_id >> 4] + (g_mission->player_id & 0xf))->pos;
                float dist = (float)sqrt(d.y * d.y + d.z * d.z + d.x * d.x);   // this term order gives the original x87 schedule (x*x in place, + z*z, + y*y)
                if (dist < best)
                    best = dist;
            }
            g_mission->agents.next_used(&id);
        } while (id < (unsigned)g_mission->agents.count);
        if (g_game_keys.clicked(4) && sel_mode == 8) {
            if (best > 8.0f) {
                int* result = &leave_answer;
                *result = -1;
                {
                    tstring no("ui_no");
                    tstring yes("ui_yes");
                    tstring question("ui_question_leave_area");
                    // 0x527591: ecx = [0xa73980] (the dialog box the menu points to), unlike set_state/open (ecx = 0xa73980)
                    g_windows.box()->setup2(g_strtable.lookup(question), g_strtable.lookup(yes), g_strtable.lookup(no), result);
                }
                g_windows.open(0);
            } else {
                tstring t("ui_cant_retreat_there_are_enemies_nearby");
                g_gui.add(*g_strtable.lookup(t), 0xffff3344);
            }
        }
    }

    // ---- conversation / mission setup --------------------------------------------------------------
    if (g_game->mode == 0xf) {
        g_game->mode = 0;
        presentation = g_game->presentation;
    }
    if (presentation >= 0 && g_presentation == 0) {
        pres.init((ContainerBase*)frame, presentation);
        if ((pres.flags & 1) == 0) {
            if (g_render.dev != 0)
                g_render.dev->ShowCursor(TRUE);
            g_gui.show_cursor = 1;
        }
    }
    if (g_presentation != 0) {
        pres.pick((int*)&PickArg_s261e0());
        __int64 d = g_ticks[pres.tick.clock] - pres.tick.start;
        if ((float)d * 1e-5f > pres.duration) {
            pres.release();
            pres.scene->update_children();          // +0xc4: deletes the overlays
            if (g_render.dev != 0)
                g_render.dev->ShowCursor(FALSE);
            g_gui.show_cursor = 0;
            presentation = -1;
        }
    }

    if (!b6) {
        // ---- boolean-false branch ------------------------------------------------------------------
        g_mission_ctx.tick();
        if (w18 == 100) {
            if (0 < g_mission_ctx.passage_menu) {
                if ((g_menus[g_mission_ctx.passage_menu].flags & 0x10) != 0)
                    g_game->encounter_flags |= 1;
            }
        } else {
            if (g_game->rest_period > 0.0f && g_game->rest_attackable != 0)
                g_windows.set_state(7);
            else
                g_windows.set_state(w18);
            goto unlock;
        }
    } else {
        // ---- boolean-true branch -------------------------------------------------------------------
        if (flag13 != 0 && g_mission->player_id >= 0 && g_game->flag18 == 0) {
            int v = val14;
            refresh_slots = 1;
            slot_as<head_rec>(8)->w18 = 0xe;
            if (v < 0) {
                slot_as<head_rec>(8)->w04 = 0;
                g_windows.set_state(8);
            } else {
                slot_as<head_rec>(8)->w04 = 3;
                slot_as<head_rec>(8)->w14 = v;
                if (slot_as<head_rec>(8)->w14 < 0) {
                    slot_as<head_rec>(8)->w14 = 0;
                    g_windows.set_state(8);
                } else {
                    int lim = g_game->ntroops - 1;
                    if (slot_as<head_rec>(8)->w14 > lim)
                        slot_as<head_rec>(8)->w14 = lim;
                    g_windows.set_state(8);
                }
            }
            goto after;
        }

        if (pressed_s261e0(g_game_keys.entry[15], 2) && g_mission->player_id >= 0 && g_game->flag18 == 0) {
            grow_health = 1;
            Troop* rec = g_game->troops;   // the player troop (record 0): hit points 35 + strength + 2 * ironflesh
            int base = rec->attributes[0];
            int v = rec->nib_b[0x12] & 0xf;               // +0x7a
            if (v < 0)                                    // (always false after the mask; the original tests it: and eax,0xf / jge)
                v = 0;
            else if (v > g_clamp_max)
                v = g_clamp_max;
            status_height = base + 0x23 + v * 2;
            slot_as<head_rec>(15)->w18 = 0xe;
            slot_as<head_rec>(15)->w04 = 0;
            slot_as<head_rec>(15)->w14 = g_game->player;
            g_windows.set_state(0xf);
            goto after;
        }

        if (pressed_s261e0(g_game_keys.entry[17], 2) && g_game->flag18 == 0 &&
            !template_flag1(g_mission->mode)) {
            slot_as<head_rec>(12)->w18 = 0xe;
            slot_as<head_rec>(12)->w04 = 5;
            g_windows.set_state(0xc);
            goto after;
        }

        if (pressed_s261e0(g_game_keys.entry[18], 2) && g_game->flag18 == 0) {
            if (g_module_settings.show_quest_notes != 0) {
                slot_as<head_rec>(26)->w18 = 0xe;
                g_game->notes_type = 4;
                g_game->notes_object = 0;
                g_windows.set_state(0x1a);
            } else {
                slot_as<head_rec>(13)->w18 = 0xe;
                slot_as<head_rec>(13)->w04 = 0;
                g_windows.set_state(0xd);
            }
            goto after;
        }

        if (pressed_s261e0(g_game_keys.entry[19], 2) && g_game->flag18 == 0) {
            slot_as<head_rec>(25)->w18 = 0xe;
            slot_as<head_rec>(25)->w04 = 0;
            g_windows.set_state(0x19);
            goto after;
        }

        if (g_keyboard.down[0x1] != 0 && g_keyboard.prev[0x1] == 0) {
            if (g_mission->mode_flag1()) {
                slot_as<head_rec>(18)->w18 = 0xe;
                g_windows.set_state(0x12);
            } else {
                slot_as<head_rec>(5)->w18 = 0xe;
                g_windows.set_state(5);
            }
            goto after;
        }

        if (g_game->flag18 == 0 && g_game_keys.clicked(0x24)) {
            if (!g_mission->mode_flag1() && g_game->flag18 == 0) {
                quick_save_msg();
            } else {
                tstring t("ui_cant_quick_save");
                g_gui.add(*g_strtable.lookup(t), 0xffffffff);
            }
            goto after;
        }

        if (g_mission->conversation != -1) {
            if (g_mission->conversation < 0)
                goto after;
            refresh_slots = 1;
            unsigned id = (unsigned)g_mission->conversation;
            g_mission->conversation = -1;
            int dialog_id = mission_agent(g_mission, id)->troop;
            slot_as<head_rec>(16)->w18 = 0xe;
            g_dialogs.start(dialog_id, (int)id, -1, 0, 0);
            g_mission->talk_troop = mission_agent(g_mission, id)->troop;
            g_game->f13a08 = mission_agent(g_mission, id)->troop;
            g_windows.set_state(0x10);
            goto after;
        }

        if (g_game->jump_scene < 0)
            goto after;
        g_mission_ctx.tick();
        g_mission_ctx.key_mask |= 8;
    }
    g_map_flags.commit((char*)&CommitArg_s261e0());

    }
after:
unlock:
    if (g_mt_inited) {
        LeaveCriticalSection(&g_mt_cs);
        SetEvent(g_mt_event);
    }
}

// ======================================================= slice s00527c90: functions
// @ 0x527c90
void GameData::save_scene(int idx)
{
    tstring path = scene_path(idx);
    bin_writer w;
    const char* p = module_path(path.c_str());
    (&w.name)->assign(p);
    w.is_open = 0;
    w.fp = fopen(p, "wb");
    if (w.fp) {
        w.failed = 0;
        w.is_open = 1;
    } else {
        w.failed = 1;
    }
    scenes[idx].save(&w, 0);
    if (w.is_open) {
        stream_base& s = w;
        if (s.flush() == 0)
            fclose(w.fp);
        w.failed = 1;
        w.is_open = 0;
    }
}

// @ 0x527da0
void SceneFile::save(stream_base* s, int group)
{
    {
        int version = -0x2cd;
        s->w_int(&version);
    }
    {
        int four = 4;
        s->w_int(&four);
    }
    int count = 0;
    for (unsigned i = 0; i < props.size(); i++) {
        if (props[i].m10 == group)
            count++;
    }
    s->w_int(&count);
    for (unsigned j = 0; j < props.size(); j++) {
        if (props[j].m10 == group)
            props[j].save(s);
    }
    s444.rewind();
    s->w_stream(&s444);
    if (!s414.at_end()) {
        int marker = 0xff4ad1a6;
        s->w_int(&marker);
        s414.rewind();
        s->w_stream(&s414);
    }
}

// @ 0x527ef0
void PlacedObj::save(stream_base* s)
{
    s->w_int(&kind);
    s->w_int(&id);
    int tmp;
    s->w_int(&tmp);
    w_matrix(*s, &start_frame);
    s->w_tstr(&name);
    s->w_int(&value);
    s->w_int(&m1a4);
    float* p = &scale.x;
    s->w_float(p).w_float(p + 1).w_float(p + 2);
}

// @ 0x527f90
void Mission::save_scene_data()
{
    SceneFile* rec = g_game->scenes + scene_idx;
    if (rec->flags & 0x100) {
        rec->s414.reset();
        terrain.save(&rec->s414);
    }
    rec->s444.reset();
    nav.save(rec->s444);                // 0x524f00 (map_builder_and_data_load.cpp)
}

// @ 0x528000
void TerrainDesc::save(stream_base* s)
{
    s->w_int(&n);
    s->w_int(&dims.w);
    s->w_int(&dims.h);
    int i = 0;
    if (n > 0) {
        TerrainDescEntry* g = layers;
        do {
            g->save(s, &dims);
            i++;
            g++;
        } while (i < n);
    }
}

// the value write of Grid::save: floats as they are for a negative type, else a byte rounded from 0..255
static inline int round_s28060(float y)
{
    float f = (float)floor((double)y);
    int ci = (int)f;
    return (y - f <= 0.5f) ? ci : ci + 1;
}
// @ 0x528060
void TerrainDescEntry::save(stream_base* s, Dims* d)
{
    const int* dims = &d->w;   // w, h
    s->w_int(&type);
    s->w_tstr(&name);
    if (data == 0) {
        int zero = 0;
        s->w_int(&zero);
        return;
    }
    {
        int one = 1;
        s->w_int(&one);
    }

    bool run = false;
    int gap = 0;
    int count = 0;
    int saved_col = 0;
    int saved_row = 0;
    for (int row = 0; row < dims[1]; row++) {
        {
            for (int col = 0; col < dims[0]; col++) {
                float v = data[col * 0x1a4 + row];
                bool transparent;
                if (type == -0x3115)
                    transparent = ((*(int*)&v & 0xffffff) == 0xffffff);
                else
                    transparent = (-0.001f < v && v < 0.001f);
                if (transparent) {
                    if (run) {
                        s->w_int(&count);
                        int r = saved_row;
                        int c = saved_col;
                        for (int i = 0; i < count; i++) {
                            if (type >= 0) {
                                float y = data[c * 0x1a4 + r] * 255.0f;
                                int ci = round_s28060(y);
                                if (ci < 0) {
                                    unsigned char b = 0;
                                    s->w_byte(&b);
                                } else {
                                    unsigned char b = 0xff;
                                    if (ci < 0x100)
                                        b = (unsigned char)ci;
                                    s->w_byte(&b);
                                }
                            } else
                                s->w_float(&data[c * 0x1a4 + r]);
                            c++;
                            if (c >= dims[0]) {
                                c = 0;
                                r++;
                            }
                        }
                        count = 0;
                        run = false;
                        gap = 1;
                    } else {
                        gap++;
                    }
                } else {
                    if (run) {
                        count++;
                    } else {
                        run = true;
                        s->w_int(&gap);
                        gap = 0;
                        count = 1;
                        saved_col = col;
                        saved_row = row;
                    }
                }
            }
        }
    }
    if (run) {
        s->w_int(&count);
        int r = saved_row;
        int c = saved_col;
        for (int i = 0; i < count; i++) {
            if (type >= 0) {
                float y = data[c * 0x1a4 + r] * 255.0f;
                int ci = round_s28060(y);
                if (ci < 0) {
                    unsigned char b = 0;
                    s->w_byte(&b);
                } else {
                    unsigned char b = 0xff;
                    if (ci < 0x100)
                        b = (unsigned char)ci;
                    s->w_byte(&b);
                }
            } else
                s->w_float(&data[c * 0x1a4 + r]);
            c++;
            if (c >= dims[0]) {
                c = 0;
                r++;
            }
        }
    } else {
        s->w_int(&gap);
    }
}

// @ 0x528410
void Mission::release_item(int idx)
{
    if (props[idx].ent) {
        props[idx].ent->release_items();
        scene->remove_entity(props[idx].ent->id);
        props[idx].ent = 0;
    }
}

// @ 0x528470
__declspec(noinline) void rgl::entity::release_items()
{
    for (std::vector<shape_base*>::iterator it = items.begin(); it < items.end(); ++it)
        (*it)->m8 |= 0x8000;
}
