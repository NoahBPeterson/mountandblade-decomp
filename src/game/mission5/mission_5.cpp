//-----------------------------------------------------------------------------
// File: mission_5.cpp  (0x525160..0x52cf60)
//
// Desc: the game-loop event handler and the dialogue / conversation update (0x525160, 0x5257f0, speaker and listener
//       finders), the mission screen's per-frame tick (0x5261e0) with the scene save path (.sco writer, 0x527c90..0x528470),
//       the view controller (0x5284a0), the mission HUD (markers, party heroes, 0x529130), the agent bookkeeping helpers
//       (0x529840..0x529d80), the interaction probe (0x529ec0), the order markers and the order menu (0x52ae70..0x52b3c0),
//       Mission::order_agent / update_ui / update_cursor / find_position (0x52b5c0..0x52bdb0), the editor-side terrain
//       update and the scene brush (0x52bf40, 0x52c2b0) and the small grid element helpers (0x52ce00..0x52cef0).
//       Names provisional. "T regime" region (docs/tu_map.md): the file keeps the binary order; every function that
//       follows its caller is a separate (noinline) function in the original. Flags: /O2 /MT /Gy /TP.
//
// Boundary: starts at 0x525160 (Game::handle_event, a class change after the record readers of game_2.cpp); ends where the
//       map editor starts (0x52cf60, class Mesh | Elem). The map's boundary 0x5249f0 is not kept (see game_2.cpp).
//-----------------------------------------------------------------------------
#define PLACEDOBJ_CTOR_OUT_OF_LINE   // fn_529d80 0x529d80 calls PlacedObj::PlacedObj (0x437830) for the resize(0) temporary
#include "../../include/image_addrs.h"
#include "../skill_cap.h"      // SkillCap g_skill_tbl (0x8ee3100): [36].cap is the clamp at 0x8ee3ac4
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "./mission_5.h"
#include "../../include/troop.h"   // Troop::unequip (0x4630d0)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../../rgl/stream_inline.h"   // the file writer built on save_scene's stack: constructor inline
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
__forceinline bin_writer::bin_writer() {}   // inline here (resmgr.cpp load_pack calls the out-of-line 0x417c10)
#include "../../rgl/mesh.h"
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)
#include "../conv_screen.h"      // Rec1208 (ConvScreen): the conversation screen record, whose slot 3 is handle_event 0x525160
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../../include/item_kind.h"   // g_items 0x5e29d0
#include "../menu_rec.h"   // g_menus 0x5e29b0
#include "../mode_rec.h"   // g_mission_templates 0x5e2a00
struct Presentation;
extern Presentation* g_presentation;                        // 0x5e299c
#include "../key_names.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/render_flags.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
extern const RangePair g_aim_range_tbl[7] = {{-0.5f, 3.1415927f}, {-2.0f, 2.0f}, {-1.5f, 2.5f}, {-1.0f, 3.0f}, {-1.3f, 2.8f}, {-3.0f, 1.0f}, {5.0f, 4.0f}};  // 0x59390c (rdata: the aim arc per Agent::variant)

#include "../ui_globals.h"      // UI / core globals
#include "../map_flags.h"      // MapFlags::commit 0x513710
#include "../config/core.h"     // GameApp g_app_obj (0x8ee3c48): the map editor dialog's members

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
// GameKeys::clicked 0x48ac90 (game/ui/mission_ui.cpp owns the row): its body was visible here too; the screen tick keeps
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
inline Agent* AgentTable::get(unsigned id)
{
    return &pages[id >> 4][id & 0xf];
}

inline __declspec(noinline) Blow* Blow::init()      // 0x4353a0: visible too (agent/agent.cpp owns the row)
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
struct V2_s25160 { float x, y; V2_s25160(float a, float b) : x(a), y(b) {} };
// @ 0x525160
void Rec1208::frame_update(float code_f)
{
    g_gui.flush();

    if (g_gui.wheel != 0)
    {
        // camera/sky interpolation over the current site's passage table
        char* o = *(char**)(((char*)(void*)question) + 0x484);
        float* pa = (float*)(o + *(int*)(o + 0x178) * 4 + 0xf0);
        float v = (*(float*)(o + 0x168) - *(float*)(o + 0x16c))
                / (*(float*)(o + 0x170) - *(float*)(o + 0x16c))
                * (*pa - *(float*)(o + 0x17c))
                + (((float)g_gui.wheel / *pa) * *(float*)(o + 0x17c))
                  * 0.0005f;
        ((UiD*)o)->set_value(v);   // 0x48baa0: UiD::set_value, this = the object o (ecx is still o at the call)
        ((VObj_s25160*)*(char**)(((char*)(void*)question) + 0x484))->v18(0x10000);
    }

    if (speaker_id == g_game->player || g_game->talk_info_shown == 0)
        goto hide;
    if (!(g_gui.mouse[0] < *(float*)(((char*)(void*)window) + 0xf0)))   // jp on mask 5: NaN hides too (0x525227)
        goto hide;
    if (!(*(float*)(((char*)(void*)window) + 0xe4) + 0.1f < g_gui.mouse[1]))
        goto hide;
    {
        ((VObj_s25160*)(void*)msg_window)->v45();
        if (*(int*)(*(char**)&g_game + 0x1a238) < -0x65 || *(int*)(*(char**)&g_game + 0x1a238) > 0x65)
        {
            ((VObj_s25160*)(void*)relation_bar)->v44();
            ((VObj_s25160*)(void*)reln_pointer)->v44();
        }
        else
        {
            V2_s25160 pair((float)(*(int*)(*(char**)&g_game + 0x1a238) + 0x64) * 0.00105f + 0.02f, 0.02f);
            ((VObj_s25160*)(void*)reln_pointer)->v22((float*)&pair);
            ((VObj_s25160*)(void*)relation_bar)->v45();
            ((VObj_s25160*)(void*)reln_pointer)->v45();
        }
    }
    {
        for (int i = 0; i < 5; ++i)
        {
            char* b = *(char**)&g_game;
            if (*(char*)(b + 0x1a290 + i) == 0)
                continue;
            int out1[2];   // 8-byte results through hidden pointers (slots esp+0x10, esp+0x18)
            int out2[2];
            ((VObj_s25160*)(void*)lines[i])->v41(out1, (void*)(b + 0x1a240 + i * 0x10));   // out value first, the record second (0x525301)
            ((VObj_s25160*)(void*)lines[i])->v37(out2);
            *(char*)(*(char**)&g_game + 0x1a290 + i) = 0;   // the calls above may have swapped the global: 0x525322 re-reads it
        }
    }
    goto done;
hide:
    ((VObj_s25160*)(void*)msg_window)->v44();
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
            g_mission->agents[g_mission->player_id].in_flags = 0;
        }
    }

    if (n_lines > 1)
    {
        int i;
        for (i = 0; i < 0x400; ++i)
        {
            if (((VObj_s25160*)(void*)buttons[i])->v17(0x10000) && i < n_lines) {
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
// 0x407d60 (rgl/shapes/shapes.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
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
    G.z = ((AgentX*)(&g_mission->agents[target]))->speed_scale() + G.z;

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
                vector4 L(x.x + 0.01f, x.y + 0.01f, (((AgentX*)(&g_mission->agents[listener]))->speed_scale() - 0.1f) + x.z);
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
                g_game->editor.cam_pos = *(vector4*)((char*)this + 0x218);
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
            // the three class blocks' `use` flags, walked as ints with the ClassInfo stride: a ClassInfo* walk loses the
            // shared &cls[0].use that the original computes once for the loops below
            TeamRec* base = &g_mission->teams[team];

            if (g_game_keys.clicked(0x23)) {
                bool changed = false;
                int* p = &base->cls[0].use;
                { int m = 3;                              // three slots, counted down
                do {
                    if (!(*p == 0)) {
                        *p = 0;
                    } else {
                        *p = 1;
                        changed = true;
                    }
                    p = (int*)((char*)p + sizeof(ClassInfo));
                } while (--m != 0); }
                if (!changed) {
                    int* q = &base->cls[0].use;
                    { int m = 3;                              // three slots, counted down
                    do {
                        *q = 1;
                        q = (int*)((char*)q + sizeof(ClassInfo));
                    } while (--m != 0); }
                }
            } else if (g_game_keys.clicked(0x1f)) {
                int* q = &base->cls[0].use;
                { int m = 3;                              // three slots, counted down
                do {
                    *q = 1;
                    q = (int*)((char*)q + sizeof(ClassInfo));
                } while (--m != 0); }
            } else {
                if (!b3) {
                    int* q = &base->cls[0].use;
                    { int m = 3;                              // three slots, counted down
                    do {
                        *q = 0;
                        q = (int*)((char*)q + sizeof(ClassInfo));
                    } while (--m != 0); }
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
            int* ap = &base->cls[0].use;
            { int m = 3;                              // three slots, counted down
            do {
                if (*ap == 0)
                    all = false;
                ap = (int*)((char*)ap + sizeof(ClassInfo));
            } while (--m != 0); }
            if (all) {
                tstring t("ui_everyone_hear_me");
                g_gui.add(*g_strtable.lookup(t), 0xffddee88);
            } else {
                char label[512];
                ((ForceLabel*)base)->make_label(label);
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
            char* obj = *(char**)((char*)g_mission_templates + g_mission_ctx.passage_menu * 0x60 + 0x58);
            ((DialogBlock*)(obj + block * 0x68 + 0x4c))->play(0x10);
        }

        if (pressed_s261e0(g_game_keys.entry[16], 2)) {
            g_script.trigger_result.value = 0;
            int* slots = rec_base_s261e0(g_mission->mode);
            for (int i = 0; i < *slots; i++)
                ((snd_slot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-0x16);
            if (g_script.trigger_result.value != 0)
                flag13 = 1;
        }
    }

    if (g_cfg.enable_edit_mode && g_keyboard.down[56] && g_keyboard.down[38] == 0 && g_keyboard.prev[38] != 0)
        g_mission->light_entities(g_mission->interior, g_mission->num_interior);   // 0x47ce00: thiscall member, this = the mission (ecx at the call)

    if (f2d8 == 0) {
        b6 = false;
    } else if (leave_answer == 0) {
        int* slots = rec_base_s261e0(g_mission->mode);
        for (int i = 0; i < *slots; i++)
            ((snd_slot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-0x1e);
        b6 = false;
    }

    if (pressed_s261e0(g_game_keys.entry[11], 2) && g_keyboard.down[56] == 0 && g_keyboard.down[184] == 0) {
        g_script.trigger_result.value = 0;
        ((snd_bank*)rec_base_s261e0(g_mission->mode))->update_all(-0x15);
        if (g_script.trigger_result.value != 0)
            b6 = false;
    }

    if (g_game->question_answer >= 0) {
        int v = g_game->question_answer;
        g_script.param1 = v;
        int* slots = rec_base_s261e0(g_mission->mode);
        for (int i = 0; i < *slots; i++)
            ((snd_slot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-8);
        g_game->question_answer = -1;
    }

    // ---- "leave area" / agent selection ------------------------------------------------------------
    if ((g_mission->flags & 1) != 0) {
        if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 8)
            leave_answer = 0;
    } else if (pressed_s261e0(g_game_keys.entry[4], 2) && sel_mode == 8 &&
               (!template_bit1_s261e0(g_mission->mode) || g_mission->mission_result > 0)) {
        leave_answer = 0;
    } else if (template_bit1_s261e0(g_mission->mode) && g_mission->player_id >= 0 &&
               g_game_keys.clicked(4) && sel_mode == 8) {
        float best = 10000.0f;
        unsigned id = g_mission->agents.first_used();
        if (id < (unsigned)g_mission->agents.count) do {
            Agent* a = (g_mission->agents.pages[(unsigned)id >> 4] + (id & 0xf));
            if (a->state == 1 && a->kind == 0 && ((AgentX*)(g_mission->agents.pages[(unsigned)g_mission->player_id >> 4] + (g_mission->player_id & 0xf)))->is_enemy((Agent const*)a)) {
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
        ((BlockOwner*)((char*)this + 0x30))->init((ContainerBase*)frame, presentation);
        if ((bh.flags44 & 1) == 0) {
            if (g_render.dev != 0)
                g_render.dev->ShowCursor(TRUE);
            g_gui.show_cursor = 1;
        }
    }
    if (g_presentation != 0) {
        BlockHead& head = bh;
        ((MissionSource*)&head)->pick((int*)&PickArg_s261e0());
        __int64 d = g_ticks[bh.tick.clock] - bh.tick.start;
        if ((float)d * 1e-5f > bh.duration) {
            g_presentation = 0;
            if (head.owned.begin != 0)
                operator delete(head.owned.begin);
            head.owned.begin = 0;
            head.owned.end = 0;
            head.owned.cap = 0;
            ((SrcVtbl_s261e0*)head.menu_block)->v49();
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
            !template_bit1_s261e0(g_mission->mode)) {
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
    nav.save(rec->s444);                // 0x524f00 (game_2.cpp)
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
        if (g_presentation == 0 || (bh.flags44 & 1)) {
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
    if (g_presentation == 0 || (bh.flags44 & 1)) {
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
            a->facing = m.f;
    }

    if (snap < 5) {
        a->facing = a->vel;        // copied through `a` itself: the scheduler then knows the stores cannot alias the loads
        const vector4& d = a->vel;
        float nx = -d.x;
        float ang = (float)atan2(nx, d.y);
        int n = snap + 1;
        p_ref = 0.0f;
        snap = n;
        yaw = ang;
    }

    // ---- movement / action bits ---------------------------------------------------------------------
    a->in_flags = 0;
    if (!g_game->editor.active) {
        if (g_keyboard.down[42] == 0)
            a->in_flags = 0x200000;
        if (g_game_keys.clicked(5))
            a->in_flags |= 0x100000;

        if (a->horse >= 0) {
            if (g_game_keys.clicked(0))
                a->in_flags |= 0x80000;
            if (g_game_keys.is_down(0))
                a->in_flags |= 0x10;
            else if (g_game_keys.is_down(1))
                a->in_flags |= 0x20;
            if (g_game_keys.is_down(3))
                a->in_flags |= 0x40;
            else if (g_game_keys.is_down(2))
                a->in_flags |= 0x80;
            if (g_game_keys.clicked(4) && sel_mode == 3)
                a->in_flags |= 0x10000;
            if ((char)g_keyboard.is_ready(0x24) && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0))
                a->in_flags |= 0x40000;
        } else {
            if (g_game_keys.is_down(0))
                a->in_flags |= 1;
            else if (g_game_keys.is_down(1))
                a->in_flags |= 2;
            if (g_game_keys.is_down(3))
                a->in_flags |= 4;
            else if (g_game_keys.is_down(2))
                a->in_flags |= 8;
            if (g_game_keys.clicked(4) && sel_mode == 2 && sel_valid != 0) {
                a->in_flags |= 0x20000;
                a->ai.target = sel_target;
            }
        }

        if (g_presentation == 0 || (bh.flags44 & 1)) {
            if (g_game_keys.is_down(6))
                a->in_flags |= 0x100;
            if (g_game_keys.clicked(6))
                a->in_flags |= 0x400;
            if (g_game_keys.is_down(7))
                a->in_flags |= 0x200;
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
        (pages[id >> 4] + (id & 0xf))->f55d4 = (g_game->f139e4 != 0);
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
            if (o->state == 1 && o->kind == 0 && ((AgentX*)g_mission->agents.get(g_mission->player_id))->is_enemy(o)) {
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
    Agent* a = &g_mission->agents.pages._Myfirst[(unsigned)g_mission->player_id >> 4][g_mission->player_id & 0xf];

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
    float aim = signed_angle((const vec2*)&a->vel, (const vec2*)&a->facing.x);

    if (pair.item >= 0) {
        bool other = false;
        if (g_game_keys.entry[13][0] >= 0 && g_keyboard.down[g_game_keys.entry[13][0]]) other = true;
        if ((g_game_keys.entry[13][1] < 0 || g_keyboard.down[g_game_keys.entry[13][1]] == 0) && !other) {
            int type = g_items[pair.item].type_flags & 0xff;
            if ((type == 8 || type == 9 || type == 10 || type == 16 || type == 17) && g_options.i64) {
                if (!((AgentX*)cur_at_s29130())->f100_is_0x48_and_charged()) {
                    if (g_mission->agents.pages._Myfirst[(unsigned)g_mission->player_id >> 4][g_mission->player_id & 0xf].horse == -1 ||
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
    ((Game*)g_game)->more_start();
    if (party_battle_mode != 0 || (mission_template >= 0 && (((char*)g_mission_templates)[mission_template * 0x58 + 0x20] & 0x10))) {
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
        drop_stack_seed(&g_game->stack_seeds[0][0], a->party, a->party_stack, a->seed, 0);
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
void Mission::drop_stack_seed(int* hist, int party_id, int slot, int value, int flag)
{
    Party* p = game_parties()->at(game_parties()->lookup(party_id));
    int found = -1;
    int (*g)[32] = (int (*)[32])hist;
    for (int i = 0; i < p->stacks[slot].count; i++) {
        if (found != -1)
            break;
        if (value == g[slot][i])
            found = i;
    }
    if (found != -1) {
        for (int j = found; j < 0x1f; j++)
            hist[slot * 32 + j] = hist[slot * 32 + j + 1];
        g[slot][31] = rand();
        if (flag && p->stacks[slot].count <= 0x20)
            hist[slot * 32 + p->stacks[slot].count - 1] = value;
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
        drop_stack_seed(&g_game->stack_seeds[0][0], a->party, a->party_stack, a->seed, 1);
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
    pos.z = ((AgentX*)me)->speed_scale() + pos.z;
    vector4 front = pos + me->facing * 10.5f;
    pos = pos + me->facing * 0.05f;

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
    g_mission->scene->query_segment((const float*)&pos, (const float*)&front, 0x110, found.shapes, &count);
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
                if (e->game_id != g_mission->player_id && !((AgentX*)me)->is_enemy((Agent const*)a)) {
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
        } else if ((g_mission->max_x - 0.5f < me->pos.x && me->facing.x > 0.5f) ||
                   (g_mission->max_y - 0.5f < me->pos.y && me->facing.y > 0.5f) ||
                   (g_mission->min_x + 0.5f > me->pos.x && me->facing.x < -0.5f) ||
                   (g_mission->min_y + 0.5f > me->pos.y && me->facing.y < -0.5f)) {
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
    *(int*)&g_gui.widest = 0;          // an integer zero store (the zero register), as the original; a float 0 is an immediate
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
        if (((AgentX*)table_agent_s29ec0(g_mission->player_id))->is_enemy(table_agent_s29ec0(target)))
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
        name = *((MapObject*)o)->mesh_ptr();   // 0x4c6ca0 (rgl/mesh2/gui_edit.cpp: MapObject is the menu item there)
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

    ((c49c2c0::Mission*)m)->order(team, order);   // 0x49c2c0: defined as c49c2c0::Mission::order (script/script_interpreter.cpp)

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
            add_placed(&mode, &slot->item, (int*)&slot->m4, &m,
                       (int*)((char*)agent + 8));
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
    ((char*)agent + 0x220)[chosen] = 0;

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
        ((MapEditorDialog*)this)->update();      // 0x5305b0 (map_editor.cpp)
        goto L_bbc5;
    }

L_bb4b:
    g_mission->sel_marker->visible = 0;
    update_placement();                      // 0x52f250 (mesh_poly_ray.cpp)
    goto L_bbc5;
    } else if (m == 1 || m == 3 || m == 2) {
        g_mission->sel_marker->visible = 0;
        g_mission->edit_obj->visible = 0;
        this->update_cursor();
    } else if (m == 4) {
        g_mission->marker_entity->visible = 1;
        update_mesh_edit();                      // 0x52cf60 (mesh_3.cpp)
    }

L_bbc5:
    if (g_game->editor.game_mode == 0 && g_game->editor.rt_flag == 0) {
        if ((float)(g_ticks[this->timer_b.clock] - this->timer_b.start) * 1e-05f > 0.05f) {
            ::pick_entity((GuiCtx*)this);           // 0x4ed8d0
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
        ((MapEditorDialog*)this)->update();      // 0x5305b0 (map_editor.cpp)
        goto L_bbc5;
    }

L_bb4b:
    g_mission->sel_marker->visible = 0;
    update_placement();                      // 0x52f250 (mesh_poly_ray.cpp)
    goto L_bbc5;

    } else if (m == 1 || m == 3 || m == 2) {
        g_mission->sel_marker->visible = 0;
        g_mission->edit_obj->visible = 0;
        this->update_cursor();
    } else if (m == 4) {
        g_mission->marker_entity->visible = 1;
        update_mesh_edit();                      // 0x52cf60 (mesh_3.cpp)
    }

L_bbc5:
    if (g_game->editor.game_mode == 0 && g_game->editor.rt_flag == 0) {
        if ((float)(g_ticks[this->timer_b.clock] - this->timer_b.start) * 1e-05f > 0.05f) {
            ::pick_entity((GuiCtx*)this);           // 0x4ed8d0
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
// cell::collect_masked 0x4396b0: agent2/agent_stats.cpp; cell_grid::walk 0x437290: agent/agent.cpp): the end point b and the
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
inline __declspec(noinline) void rgl::world::query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count)
{
    int pairs[0x2000][2];
    int n;
    walk(a, b, pairs, &n);
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

    float b[2] = { far_pt.x, far_pt.y };
    vector4 p;
    p.x = near_pt.x;
    p.y = near_pt.y;

    float best = 1000000.0f;
    shape_base* shapes[8192];
    int n;
    {
        int count;
        g_mission->scene->query_segment((const float*)&p, b, 0x100, shapes, &count);
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



// ======================================================= slice s0052bf40: functions
// @ 0x52bf40
void Terrain::update(TerrainDesc* desc, vector4* pos, int sel, int mode, int flag)
{
    clear_brushed();

    if (mode == 1)
        sel = desc->n_minus2;
    else if (mode == 2)
        sel = desc->n_minus1;

    float speed;
    if (g_game->editor.game_mode == 1)
        speed = g_game->editor.mx;
    else if (g_game->editor.game_mode == 3)
        speed = g_game->editor.mz;
    else
        speed = g_game->editor.my;

    float r1 = speed * speed;
    float r2 = this->scale * 10.0f + speed;
    r2 = r2 * r2;

    int cell[2];   // brush() reads {col, row} through one pointer (0x52c0a1: [esp+0x1c], [esp+0x20])
    for (cell[1] = 0; cell[1] <= this->h; cell[1]++) {
        for (cell[0] = 0; cell[0] <= this->w; cell[0]++) {
            terrain_cell& tc = this->cell[cell[0]][cell[1]];
            const vector4 dv = *(const vector4*)&tc.px - *pos;
            float d2 = dv.x * dv.x + dv.z * dv.z + dv.y * dv.y;
            if ((mode == 0 && d2 <= r2) || d2 <= r1) {
                char place = 1;
                float d = (float)sqrt(d2);
                if (!(d2 <= r1))   // 0x52c065: fcomp vs the squared distance (the fld st(0) copy), not d
                    place = 0;
                if (sel < 0)
                    return;
                if (sel >= desc->n)
                    return;
                brush(desc, &sel, cell, &d, &mode, &flag, &place);
            }
        }
    }

    if (mode == 0) {
        int rowbase = 0;
        for (int i = 0; i < desc->n - 2; i++) {
            char found = 0;
            for (int j = 0; j <= nx; j++) {
                for (int k = 0; k <= ny; k++) {
                    if (i + 4 < 0xf && brushed[i + 4][j][k] != 0)
                        found = 1;
                }
            }
            if (found)
                this->load_layer(desc, i);
            rowbase += 5;
        }
    } else if (mode == 1) {
        this->load_layer(desc, desc->n_minus2);
    } else if (mode == 2) {
        this->load_layer(desc, desc->n_minus1);
    }

    float h;
    {
        vector4 v = *(vector4*)((char*)this->world + 0x68);
        h = (v.x + v.y + v.z) * 0.3f;
    }

    for (int bx = 0; bx < this->nx; bx++) {
        for (int by = 0; by < this->ny; by++) {
            for (int layer = 0; layer < 0xf; layer++) {
                if (brushed[layer][bx][by] != 0) {
                    int blend = -1;
                    if (g_use_shaders != 0 && this->layer[1].mat2 != 0) {
                        if (layer == 1)
                            blend = 2;
                        else if (layer == 2)
                            continue;
                    }
                    this->emit_particle(layer, blend, bx, by, h);
                }
            }
        }
    }
}

// =====================================================================================================
//  Terrain_s2bf40::clear
// =====================================================================================================
// @ 0x52c290
__declspec(noinline) void Terrain::clear_brushed()
{
    // written as a byte loop (not memset) so cl expands it exactly like the
    // original: lea edi,[ecx+0x845e030]; xor eax,eax; mov ecx,0x5d;
    // rep stosd; stosw; stosb.
    char* p = (char*)brushed;
    for (int i = 0; i < 0x177; i++)
        p[i] = 0;
}

// ======================================================= slice s0052c2b0: functions
#undef g_game_c
#define g_game_c ((char*)g_game)
// the element's lazy value block: read with a default, store with the allocate-on-positive rule (expanded at every use)
static __forceinline float elem_get_s2c2b0(TerrainDescEntry* E, const int* c)
{
    float r = 0.0f;
    if (E->data)
        r = E->data[c[0] * 0x1a4 + c[1]];
    return r;
}
static __forceinline float clamp0_s2c2b0(float v) { return 0.0f > v ? 0.0f : v; }
static __forceinline void elem_set_s2c2b0(TerrainDescEntry* E, const int* c, float val)
{
    if (E->data == 0) {
        if (!(val > 0.0f))
            return;
        E->data = (float*)operator new[](0xac440);
        E->clear();
    }
    E->data[c[0] * 0x1a4 + c[1]] = val;
}
// @ 0x52c2b0
void Terrain::brush(TerrainDesc* desc, int* pidx, int* cell, float* amt, int* op, int* fA, char* fB)
{
    // Stand-ins, no code: 0x52bf40 needs brush's cell, pidx, op and fA to count as escaping (without them it keeps its locals in shared slots).
    // An opaque brush gives 0x52bf40 the same bytes, so the original's escape was real or the callee was opaque; its real form is not known yet.
    int*& cell_r = cell; int*& pidx_r = pidx; int*& op_r = op; int*& fA_r = fA;
    int stampbase = 0;
#define MODE_S2C2B0 (*(int*)(g_game_c + 0x1a094))
    float sel;
    if (MODE_S2C2B0 == 1)      sel = *(float*)(g_game_c + 0x1a0d4);
    else if (MODE_S2C2B0 == 3) sel = *(float*)(g_game_c + 0x1a0dc);
    else                       sel = *(float*)(g_game_c + 0x1a0d8);

    float lo;
    if (MODE_S2C2B0 == 1)      lo = *(float*)(g_game_c + 0x1a0ec);
    else if (MODE_S2C2B0 == 3) lo = *(float*)(g_game_c + 0x1a0f4);
    else                       lo = *(float*)(g_game_c + 0x1a0f0);

    float t = *amt / sel;
    if (t < 0.0f) t = 0.0f;
    else if (t >= 1.0f) t = 1.0f;

    float q = 1.0f - (1.0f - lo) * t;
    float hi;
    if (MODE_S2C2B0 == 1)      hi = *(float*)(g_game_c + 0x1a0e0);
    else if (MODE_S2C2B0 == 3) hi = *(float*)(g_game_c + 0x1a0e8);
    else                       hi = *(float*)(g_game_c + 0x1a0e4);

    const float amount = hi * q;

    if (*(char*)fA != 0 && *fB != 0) {
        // ---- both flags set ------------------------------------------------------------------------
        if (*op == 0) {
            for (int i = 0; i < desc->n - 2; i++) {
                elem_set_s2c2b0(&desc->layers[i], cell,
                                clamp0_s2c2b0(elem_get_s2c2b0(&desc->layers[i], cell) - amount));
            }
            stampbase = 4;
        } else if (*op == 1) {
            TerrainDescEntry* E = &desc->layers[*pidx];
            elem_set_s2c2b0(E, cell, clamp0_s2c2b0(elem_get_s2c2b0(E, cell) * (1.0f - amount)));
        } else if (*op == 2) {
            TerrainDescEntry* E = &desc->layers[*pidx];
            unsigned argb = 0xffffffff;
            if (E->data) argb = *(unsigned*)&E->data[cell[0] * 0x1a4 + cell[1]];
            rgl::vector4 col = rgl::unpack_color(&argb);
            float k = 1.0f - amount;
            col = col * k + rgl::vector4(amount, amount, amount);   // the whole-struct assignment carries the temporary's unset w, as the original
            argb = rgl::pack_color(&col);
            E->put(cell, cell + 1, (int*)&argb);
        }
    } else {
        // ---- one of the two flags is clear: normal brush -------------------------------------------
        TerrainDescEntry* E = &desc->layers[*pidx];
        float v = elem_get_s2c2b0(E, cell);

        if (*op == 0) {
            float nv = v + amount;
            if (1.0f < nv) nv = 1.0f;

            int n = desc->n - 2;
            bool any = false;
            for (int i = *pidx + 1; i < n; i++) {
                if (elem_get_s2c2b0(&desc->layers[i], cell) > 0.98f)
                    any = true;
            }

            if (any) {
                elem_set_s2c2b0(E, cell, 1.0f);
            } else {
                if (*fB == 0) goto done;
                elem_set_s2c2b0(E, cell, nv);
            }

            if (*fB != 0) {
                for (int i = *pidx + 1; i < desc->n - 2; i++) {
                    if (desc->layers[i].data) {
                        float old = desc->layers[i].data[cell[0] * 0x1a4 + cell[1]];
                        if (old > 0.0f) {
                            float dv = old - amount;
                            if (dv < 0.0f) dv = 0.0f;
                            else if (dv >= old) dv = old;
                            elem_set_s2c2b0(&desc->layers[i], cell, dv);
                        }
                    }
                }
            }
        } else if (*op == 1) {
            if (*(int*)(g_game_c + 0x1a108) == 1) {
                v = v - amount;
            } else if (*(int*)(g_game_c + 0x1a108) == 0) {
                v = v + amount;
            } else {
                // response = average of the nine surrounding terrain tiles, blended by `amount`
                int x = cell[0];
                int cnt = 0;
                int yp1 = cell[1] + 1;
                float sum = 0.0f;
                if (x >= 0 && x < w && yp1 >= 0 && yp1 < h) {
                    sum = cells[x * 251 + yp1].f88
                        + cells[x * 251 + yp1].h2;
                    cnt = 1;
                }
                int xp1 = x + 1;
                if (xp1 >= 0) {
                    if (xp1 < w && yp1 >= 0 && yp1 < h) {
                        sum += cells[xp1 * 251 + yp1].f88
                             + cells[xp1 * 251 + yp1].h2;
                        cnt++;
                    }
                    if (xp1 >= 0 && xp1 < w && cell[1] >= 0 && h > cell[1]) {
                        cnt++;
                        sum += cells[xp1 * 251 + cell[1]].f88
                             + cells[xp1 * 251 + cell[1]].h2;
                    }
                }
                int ym1 = cell[1] - 1;
                if (xp1 >= 0 && xp1 < w && ym1 >= 0 && ym1 < h) {
                    sum += cells[xp1 * 251 + ym1].f88
                         + cells[xp1 * 251 + ym1].h2;
                    cnt++;
                }
                if (x >= 0 && x < w && ym1 >= 0 && ym1 < h) {
                    sum += cells[x * 251 + ym1].f88
                         + cells[x * 251 + ym1].h2;
                    cnt++;
                }
                int xm1 = x - 1;
                if (xm1 >= 0) {
                    if (xm1 < w && ym1 >= 0 && ym1 < h) {
                        cnt++;
                        sum += cells[xm1 * 251 + ym1].f88
                             + cells[xm1 * 251 + ym1].h2;
                    }
                    if (xm1 >= 0 && xm1 < w && cell[1] >= 0 && cell[1] < h) {
                        cnt++;
                        sum += cells[xm1 * 251 + cell[1]].f88
                             + cells[xm1 * 251 + cell[1]].h2;
                    }
                    if (xm1 >= 0 && xm1 < w && yp1 >= 0 && yp1 < h) {
                        sum += cells[xm1 * 251 + yp1].f88
                             + cells[xm1 * 251 + yp1].h2;
                        cnt++;
                    }
                }
                if (x >= 0 && x < w && cell[1] >= 0 && cell[1] < h) {
                    cnt++;
                    sum += cells[x * 251 + cell[1]].f88
                         + cells[x * 251 + cell[1]].h2;
                }
                float centre = cells[x * 251 + cell[1]].f88
                             + cells[x * 251 + cell[1]].h2;
                v = ((1.0f - amount) * centre + ((centre + sum) * amount) / (float)(cnt + 1)) + v - centre;
            }
            elem_set_s2c2b0(E, cell, v);
        } else if (*op == 2) {
            // blend the tile colour toward the brush colour
            unsigned argb = 0xffffffff;
            if (E->data) argb = *(unsigned*)&E->data[cell[0] * 0x1a4 + cell[1]];
            rgl::vector4 col = rgl::unpack_color(&argb);
            float k = 1.0f - amount;
            col = col * k + amount * *(rgl::vector4*)(g_game_c + 0x1a0f8);   // the whole-struct assignment carries the temporary's unset w, as the original
            argb = rgl::pack_color(&col);
            E->put(cell, cell + 1, (int*)&argb);
        }
    }

done:
    int P1[2], P2[2], P3[2];
    clamp_cell(P1, cell);
    int up[2]   = { cell[0] + 1, cell[1] + 1 };
    int down[2] = { cell[0] - 1, cell[1] - 1 };
    clamp_cell(P2, up);
    clamp_cell(P3, down);

    if (stampbase < 0xf) {
        int k = stampbase * 5;
        do {
            ((char*)brushed)[(P1[0] + k) * 5 + P1[1]] = 1;
            ((char*)brushed)[(P2[0] + k) * 5 + P2[1]] = 1;
            ((char*)brushed)[(P3[0] + k) * 5 + P3[1]] = 1;
            k += 5;
        } while (k < 0x4b);
    }
}

// ======================================================= slice s0052ce00: functions
// @ 0x52ce00
void TerrainDescEntry::clear()
{
    if (data != 0) {
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                data[x * 0x1a4 + y] = 0.0f;
    }
}

// @ 0x52ce70
void TerrainDescEntry::put(int* ix, int* iy, int* value)
{
    if (data == 0) {
        if (*value == -1)
            return;
        data = (float*)operator new[](0xac440);
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                ((int*)data)[x * 0x1a4 + y] = -1;
    }
    ((int*)data)[*ix * 0x1a4 + *iy] = *value;
}

// @ 0x52cef0
__declspec(noinline) int* Terrain::clamp_cell(int* out, int* in)
{
    int ox, oy;
    int ix = in[0] / block;
    if (ix < 0)
        ox = 0;
    else if (ix >= nx)
        ox = nx - 1;
    else
        ox = ix;
    int iy = in[1] / block;
    if (iy < 0)
        oy = 0;
    else if (iy >= ny)
        oy = ny - 1;
    else
        oy = iy;
    out[0] = ox;
    out[1] = oy;
    return out;
}
