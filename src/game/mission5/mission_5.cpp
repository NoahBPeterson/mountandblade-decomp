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
extern const float g_aim_range_tbl[14] = {-0.5f, 3.1415927f, -2.0f, 2.0f, -1.5f, 2.5f, -1.0f, 3.0f, -1.3f, 2.8f, -3.0f, 1.0f, 5.0f, 4.0f};  // 0x59390c (rdata: {lo, hi} pairs)

#include "../ui_globals.h"      // UI / core globals
#include "../map_flags.h"      // MapFlags::commit 0x513710

// The agent table's walkers: the original's bodies were visible here (cl keeps g_mission in a register across them,
// as they write no global), though it left them out of line. agent2/agent_ai.cpp owns the rows at these addresses.
inline __declspec(noinline) void AgentTable::next_used(unsigned* i)
{
    ++*i;
    while ((int)*i < count) {
        unsigned v = *i;
        if (pages[v >> 4][v & 0xf].used != 0) break;
        ++*i;
    }
}
inline __declspec(noinline) int AgentTable::first_used()
{
    unsigned v = 0;
    if (0 < count) {
        Agent** pg = &pages[0];
        do {
            if (pg[v >> 4][v & 0xf].used != 0) break;
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
#undef g_big
#define g_big       (*(Big5e292c_s25160**)&g_mission)
// agent record i of the mission (pages of 16, 0x5928 bytes each)
struct AgentRow_s25160 { char b[0x5928]; };
static __forceinline char* rec_s25160(unsigned i)
{
    std::vector<AgentRow_s25160*>& pages = *(std::vector<AgentRow_s25160*>*)((char*)g_big + 0x18);
    return (char*)(pages[i >> 4] + (i & 0xf));
}
#define REC_S25160(i) rec_s25160((unsigned)(i))
struct Widget_s12 { char pad0[0x10]; int f10; };
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

    if (speaker_id == *(int*)(*(char**)&g_game + 0x8c) || *(int*)(*(char**)&g_game + 0x1a23c) == 0)
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

    *(char*)(g_big + 0x6554) = 1;

    {
    int res = -1;
    int v = res;
    if (g_dialogs.partner_agent >= 0)
    {
        *(int*)(REC_S25160(g_dialogs.partner_agent) + 0x31c) = *(int*)((char*)g_big + 0x5c);
        *(int*)(REC_S25160(g_dialogs.partner_agent) + 0x533c) = 2;
        *(int*)(REC_S25160(g_dialogs.partner_agent) + 0x5338) = 0;
        ((Agent*)REC_S25160(g_dialogs.partner_agent))->start_attack(0);
    }

    ((Mission*)g_big)->frame(code_f, false);

    {
        if (*(int*)((char*)g_big + 0x5c) >= 0)
        {
            ((Agent*)REC_S25160(*(int*)((char*)g_big + 0x5c)))->hide();
            *(int*)(REC_S25160(*(int*)((char*)g_big + 0x5c)) + 0x55d0) = 0;
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

    ((ViewObj_s257f0*)(void*)this)->update();

    if (v != 1 && !(g_cfg.enable_edit_mode != 0 && g_keyboard.down[15] != 0 && g_keyboard.prev[15] == 0))
    {
    if (v == 0x10)
    {
        slot_as<Widget_s261e0>(26)->f18 = 0x10;
        g_windows.set_state(0x1a);
        return;
    }

    if (v == 4)
    {
        unsigned __int64 flags = ((Troop*)(*(char**)(*(char**)&g_game + 0x13a00)))[g_dialogs.fc].flags30;
        Troop& tr = ((Troop*)(*(char**)(*(char**)&g_game + 0x13a00)))[g_dialogs.fc];
        if ((flags & 0x10) == 0 || (flags & 0x1000) == 0)   // 0x525568..0x525574: both bits set skips the loop
        {
            for (int i = 0; i < 0xa; ++i)
                tr.unequip(i);
        }
        slot_as<Widget_s261e0>(8)->f18 = 0x10;
        slot_as<Widget_s261e0>(8)->f14 = g_dialogs.fc;
        slot_as<Widget_s261e0>(8)->f4 = 2;
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
                    slot_as<Widget_s261e0>(15)->f18 = 0x10;
                    slot_as<Widget_s261e0>(15)->f14 = g_dialogs.fc;
                    slot_as<Widget_s261e0>(15)->f4 = (v != 0xb) + 1;
                    ((VObj_s25160*)slot_as<Widget_s261e0>(15))->v4();
                    g_windows.set_state(0xf);
                }
                return;
            }
            slot_as<Widget_s261e0>(12)->f18 = 0x10;
            if (v != 5 && v != 7 && v != 6)
            {
                slot_as<Widget_s12>(12)->f10 = -1;
                slot_as<Widget_s261e0>(12)->f14 = g_dialogs.fc;
            }
            else
                slot_as<Widget_s12>(12)->f10 = g_dialogs.f10;
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
            slot_as<Widget_s261e0>(12)->f4 = mode;
            g_windows.set_state(0xc);
            return;
        }
    }
    else
        target = g_dialogs.fc;
    {
        slot_as<Widget_s261e0>(8)->f18 = 0x10;
        int a = *(int*)(*(char**)&g_game + 0x13af0);
        if (a == 0)
            a = g_dialogs.fc;
        slot_as<Widget_s261e0>(8)->f14 = a;
        slot_as<Widget_s261e0>(8)->f4 = ((v != 3) ? 0 : 3) + 1;
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
#undef g_mission_s257f0
#define g_mission_s257f0 (*(Mission_s257f0**)&g_mission)
#undef g_game_s257f0
#define g_game_s257f0 (*(Game_s257f0**)&g_game)
#undef g_n_dialogs_s257f0
#define g_n_dialogs_s257f0 (*(DialogMgr_s257f0*)&g_dialogs)
#undef AGENT
#define AGENT(id) (g_mission_s257f0->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))
// @ 0x526070 -------------------------------------------------------------------------------------------
int ViewObj_s257f0::find_speaker_agent()
{
    int found = -1;
    int speaker = g_n_dialogs_s257f0.speaker();
    unsigned __int64 flags = *(unsigned __int64*)&g_game_s257f0->chars[speaker].flags;    // 64-bit troop flags
    bool any = false;
    if ((flags & 0x10) != 0 || *(int*)((char*)&g_dialogs + 0x14) == -1) {
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
        return *(int*)((char*)&g_dialogs + 0x14);
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
#undef AG_S257F0
#define AG_S257F0(id) ((*(std::vector<Agent_s257f0*>*)((char*)g_mission_s257f0 + 0x18))[(unsigned)(id) >> 4] + ((id) & 0xf))
static __forceinline void vadd_s257f0(vector4& a, const vector4& b) { a.x += b.x; a.y += b.y; a.z += b.z; }
static __forceinline float vdot_neg_s257f0(const vector4& a, const vector4& b) { return -a.x * b.x + -a.z * b.z + -a.y * b.y; }
// @ 0x5257f0 -------------------------------------------------------------------------------------------
void ViewObj_s257f0::update()
{
    unsigned target = find_speaker_agent();
    int listener = find_listener_agent();
    if (target == (unsigned)-1)
        return;

    vector4 G = AG_S257F0(target)->body->m.o;
    vector4 F;
    G.z = ((AgentX*)AG_S257F0(target))->speed_scale() + G.z;

    if (target == (unsigned)(*(int*)((char*)&g_dialogs + 0x14))) {
        vector4 x;
        vsub_to_s257f0(x, AG_S257F0(g_mission_s257f0->selected)->pos, AG_S257F0(target)->pos);
        float len = x.normalize();
        F = x;                                // the look direction (0x5258e1..0x5258fa copy all four dwords)
        if (1.0f < len)
            len = 1.0f;
        vadd_s257f0(G, x * len);
    } else {
        F = AG_S257F0(target)->body->m.f;
        vadd_s257f0(G, F);
        if (listener != -1) {
            vector4 x = AG_S257F0(listener)->body->m.o;
            {
                vector4 L(x.x + 0.01f, x.y + 0.01f, (((AgentX*)AG_S257F0(listener))->speed_scale() - 0.1f) + x.z);
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

    if ((m.o - G).length() > 0.3f || (m.u - F).length() > 0.1f)
        blend = blend - g_step_tbl[2] * 4.0f;

    float t = clamp01_s257f0(1.0f - blend);
    if (!started) {
        started = 1;
        t = t * g_step_tbl[2] * 1.5f;
    }
    t = clamp01_s257f0(t);
    float inv = 1.0f - t;

    m.o = G * t + m.o * inv;
    m.s = D * t + m.s * inv;
    m.f = E * t + m.f * inv;
    m.u = F * t + m.u * inv;

    m.orthonormalize();
    m_fc = m;
    float far_plane = 2.0f;
    ((Mission*)g_mission_s257f0)->camera_to((const rgl::matrix*)&m, &far_plane);

    struct Pose { char b[0x120]; };
    *(Pose*)((char*)f30 + 0x1ac) = *(Pose*)((char*)this + 0x34);

    blend = (g_step_tbl[2] + g_step_tbl[2]) + blend;
    blend = blend < 0.0f ? 0.0f : (blend >= 1.0f ? 1.0f : blend);
    }
}

// ======================================================= slice s00526130: functions
#undef g_mission_s26130
#define g_mission_s26130 (*(Mission_s26130**)&g_mission)
#undef g_game_s26130
#define g_game_s26130 (*(Game_s26130**)&g_game)
#undef g_n_dialogs_s26130
#define g_n_dialogs_s26130 (*(DialogMgr_s26130*)&g_dialogs)
// @ 0x526130 -------------------------------------------------------------------------------------------
int ViewObj_s257f0::find_listener_agent()
{
    int found = -1;
    int listener = g_n_dialogs_s26130.listener();
    unsigned __int64 flags = *(unsigned __int64*)&g_game_s26130->chars[listener].flags;    // 64-bit troop flags
    if ((flags & 0x10) != 0 || *(int*)((char*)&g_dialogs + 0x14) == -1) {
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
#undef g_keyready_s261e0
#define g_keyready_s261e0 (*(KeyReady_s261e0*)&g_keyboard)
#undef g_mission_s261e0
#define g_mission_s261e0 (*(Mission_s261e0**)&g_mission)
#undef g_game_s261e0
#define g_game_s261e0 (*(GameData_s261e0**)&g_game)
#undef g_flags_s261e0
#define g_flags_s261e0 (*(Flags5e291c_s261e0*)&g_map_flags)
#undef g_clamp_max
#define g_clamp_max g_skill_tbl[36].max_rank             // 0x8ee3ac4

// @ 0x5261e0
void MissionBlocker::frame_update(float dt)
{
    float v[2];
    if (g_mt_inited)
        EnterCriticalSection(&g_mt_cs);

    {
    float sdt = dt;

    // ---- key toggles (slow-motion / pause) ----------------------------------------------------------
    if (g_cfg.cheat_mode != 0 && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) && g_keyboard.down[67] != 0 && g_keyboard.prev[67] == 0)
        f2e1 = (unsigned char)!f2e1;
    if (g_cfg.cheat_mode != 0 && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) && g_keyboard.down[87] != 0 && g_keyboard.prev[87] == 0)
        f2e0 = (unsigned char)!f2e0;
    if (f2e0)
        sdt = 0.0f;
    else if (f2e1)
        sdt = dt * 0.2f;

    // ---- scene time-scale blend --------------------------------------------------------------------
    if (g_cfg.cheat_mode) {
        if (g_mission_s261e0->f6810 > 0.0f) {
            __int64 d = g_ticks[g_mission_s261e0->f6800] - g_mission_s261e0->f6808;
            float r = (float)d * 1e-5f;
            r = r / g_mission_s261e0->f6810;      // 0x52630b..0x526311: fmul then fdiv (the fused expression compiles to fdiv then fmul)
            if (r >= 1.0f) {
                g_mission_s261e0->f67f4 = g_mission_s261e0->f67f8;
                sdt = sdt * g_mission_s261e0->f67f4;
                g_mission_s261e0->f6810 = 0.0f;
            } else {
                sdt = (r * g_mission_s261e0->f67f8 + (1.0f - r) * g_mission_s261e0->f67f4) * sdt;
            }
        } else {
            sdt = sdt * g_mission_s261e0->f67f4;
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

    ((ViewCtrl_s284a0*)this)->update(sdt);
    g_mission_s261e0->b6554 = 0;
    ((Mission*)g_mission_s261e0)->frame(sdt, true);
    if (g_mission_s261e0->local_id >= 0) {
        update_status();
        update_marks();
    }
    update_camera(sdt);
    g_gui.flush();
    }

    // ---- no-camera position save -------------------------------------------------------------------
    if (g_cfg.enable_edit_mode && (g_keyboard.down[29] || g_keyboard.down[157]) && g_keyboard.down[18] && !g_keyboard.prev[18]) {
        if (g_game_s261e0->f1a090 == 0) {
            if (g_mission_s261e0->local_id >= 0) {
                *(vector4*)((char*)g_game_s261e0 + 0x1a1a4) = *(vector4*)((char*)page_agent_s261e0(g_mission_s261e0, g_mission_s261e0->local_id) + 0x50);
                *(float*)((char*)g_game_s261e0 + 0x1a1ac) += 1.5f;
            } else {
                *(vector4*)((char*)g_game_s261e0 + 0x1a1a4) = *(vector4*)((char*)this + 0x218);
            }
            g_game_s261e0->f139e4 = 0;
            g_game_s261e0->f1a090 = 1;
            *(int*)((char*)g_game_s261e0 + 0x1a20c) = 0;
            g_game_s261e0->f1a1b4 = -1;
            ((std::vector<int>*)((char*)g_game_s261e0 + 0x1a1b8))->resize(0, 0);
            *(int*)((char*)g_game_s261e0 + 0x1a1cc) = 0;
            ((Screen_s2ae70*)g_mission_s261e0)->init_order_markers();   // 0x52653e: ecx = g_mission (0x5e292c), not the screen
        } else {
            restore_defaults();
            if (g_mt_inited) {
                LeaveCriticalSection(&g_mt_cs);
                SetEvent(g_mt_event);
            }
            return;
        }
    }
    if (g_game_s261e0->f1a090 != 0) {
        ((Mission_s2b5c0*)this)->update_ui();
        if (g_mt_inited) {
            LeaveCriticalSection(&g_mt_cs);
            SetEvent(g_mt_event);
        }
        return;
    }

    {
    // ---- "mission started" pending item ------------------------------------------------------------
    bool  b6;               // local at esp+0x12
    if (g_mission_s261e0->b6338 == 0) {
        b6 = true;
    } else {
        float e = ((Timer*)((char*)g_mission_s261e0 + 0x6328))->elapsed();
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
            ((PolyMesh*)((char*)g_mission_s261e0 + 0x6698))->build_geometry();
            ((Mission_s27c90*)g_mission_s261e0)->save_scene_data();
            ((Mission_s29840*)g_mission_s261e0)->fn_529d80();
            ((GameData_s27c90*)g_game_s261e0)->save_scene(g_mission_s261e0->f5df4);
        } else {
            b6 = false;
        }
        g_pending_action = -1;
    }

    // ---- mission-editor key block ------------------------------------------------------------------
    if (g_mission_s261e0->local_id >= 0) {
        if (g_keyboard.down[29] == 0 && g_keyboard.down[157] == 0) {
            if (g_game_keys.clicked(0x15)) {
                f2a0 = 0;
                ((Screen_s2ae70*)this)->order_tick();
            } else if (g_game_keys.clicked(0x16)) {
                f2a0 = 1;
                ((Screen_s2ae70*)this)->order_tick();
            } else if (g_game_keys.clicked(0x17)) {
                f2a0 = 2;
                ((Screen_s2ae70*)this)->order_tick();
            } else if (g_game_keys.clicked(0x1a)) {
                f2a0 = 5;
                ((Screen_s2ae70*)this)->order_tick();
            } else if (g_game_keys.clicked(0x1b)) {
                f2a0 = 6;
                ((Screen_s2ae70*)this)->order_tick();
            } else if (g_game_keys.clicked(0x1c)) {
                f2a0 = 7;
                ((Screen_s2ae70*)this)->order_tick();
            } else if (g_game_keys.clicked(0x1d)) {
                f2a0 = 8;
                ((Screen_s2ae70*)this)->order_tick();
            } else if (g_game_keys.clicked(0x18)) {
                WL_MARKER(12)
                if (g_mission_s261e0->f6674 == 0) {
                    f2a0 = 4;
                    g_mission_s261e0->f6674 = 1;
                } else {
                    f2a0 = 3;
                    g_mission_s261e0->f6674 = 0;
                }
                ((Screen_s2ae70*)this)->order_tick();
                WL_MARKER(13)
            } else if (g_game_keys.clicked(0x1e)) {
                WL_MARKER(12)
                if (g_mission_s261e0->f666c == 0) {
                    f2a0 = 9;
                    g_mission_s261e0->f666c = 1;
                } else {
                    f2a0 = 10;
                    g_mission_s261e0->f666c = 0;
                }
                ((Screen_s2ae70*)this)->order_tick();
                WL_MARKER(13)
            } else if (g_game_keys.clicked(0x19)) {
                WL_MARKER(12)
                if (g_mission_s261e0->f6670 == 0) {
                    f2a0 = 0xc;
                    g_mission_s261e0->f6670 = 1;
                } else {
                    f2a0 = 0xd;
                    g_mission_s261e0->f6670 = 0;
                }
                ((Screen_s2ae70*)this)->order_tick();
                WL_MARKER(13)
            }
        }
        if (g_game_keys.clicked(0x1f) || g_game_keys.clicked(0x20) || g_game_keys.clicked(0x21) ||
            g_game_keys.clicked(0x22) || g_game_keys.clicked(0x23)) {
            bool mod = (g_keyboard.down[29] || g_keyboard.down[157] || g_keyboard.down[42] || g_keyboard.down[54]);
            bool b3 = false;
            if (!mod)
                *(int*)((char*)g_game_s261e0 + 0x1a1d4) = 0;
            else {
                b3 = true;
                *(int*)((char*)g_game_s261e0 + 0x1a1d4) = 1;
            }

            int kind = page_agent_s261e0(g_mission_s261e0, g_mission_s261e0->local_id)->f1c4;
            if (kind < 0 || 8 <= kind)
                kind = 7;
            char* base = (char*)g_mission_s261e0 + 0x527c + kind * 0x16c;

            if (g_game_keys.clicked(0x23)) {
                bool changed = false;
                int* p = (int*)(base + 0x4c);
                for (int m = 3; m != 0; m--) {                // the original counts down (cl unrolls an up-counting constant loop)
                    if (!(*p == 0)) {
                        *p = 0;
                    } else {
                        *p = 1;
                        changed = true;
                    }
                    p = (int*)((char*)p + 0x6c);
                }
                if (!changed) {
                    int* q = (int*)(base + 0x4c);
                    for (int m = 3; m != 0; m--) {
                        *q = 1;
                        q = (int*)((char*)q + 0x6c);
                    }
                }
            } else if (g_game_keys.clicked(0x1f)) {
                int* q = (int*)(base + 0x4c);
                for (int m = 3; m != 0; m--) {
                    *q = 1;
                    q = (int*)((char*)q + 0x6c);
                }
            } else {
                if (!b3) {
                    int* q = (int*)(base + 0x4c);
                    for (int m = 3; m != 0; m--) {
                        *q = 0;
                        q = (int*)((char*)q + 0x6c);
                    }
                }
                int idx = 0;
                if (g_game_keys.clicked(0x20))
                    idx = 0;
                else if (g_game_keys.clicked(0x21))
                    idx = 1;
                else if (g_game_keys.clicked(0x22))
                    idx = 2;
                *(int*)(base + 0x4c + idx * 0x6c) = 1;
            }

            bool all = true;
            int* ap = (int*)(base + 0x4c);
            for (int m = 3; m != 0; m--) {
                if (*ap == 0)
                    all = false;
                ap = (int*)((char*)ap + 0x6c);
            }
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
        if ((float)d * 1e-5f > g_f590b38 && g_mission_s261e0->local_id >= 0) {
            f29c = 0;
            __int64 d2 = g_ticks[timer_b.clock] - timer_b.start;
            if ((float)d2 * 1e-5f > 0.1f) {
                ((Screen_s29ec0*)this)->f_529ec0();
                timer_b.start = g_ticks[timer_b.clock];
            }
            if (f2c8 == -1) {
                g_gui.set_overlay(0, 1);
                v[0] = 0.5f;
                v[1] = 0.5f;
                g_gui.set_view(v);
            }
        }
    }

    // ---- item-use keys + sound slots ---------------------------------------------------------------
    int val14 = -1;             // esp+0x14, the slot sdt had
    unsigned char flag13 = 0;   // esp+0x13 ("value valid")
    if (g_mission_s261e0->local_id >= 0) {
        if (any_pressed_s261e0(g_game_keys.entry[4], 2) && f2c8 == 4 && f2d4 != 0) {
            WL_MARKER(12)
            int v = *(int*)((char*)g_game_s261e0->f13a50 + 0x7c +
                            (g_mission_s261e0->f5df4 * 0x125 + f2cc) * 4);
            if (v == 100000)
                b6 = false;
            else {
                g_game_s261e0->f13af4 = v;
                g_game_s261e0->f13af8 = 0;
            }
            WL_MARKER(13)
        } else if (any_pressed_s261e0(g_game_keys.entry[4], 2) && f2c8 == 1 && f2d4 != 0) {
            WL_MARKER(12)
            g_mission_s261e0->f6620 = f2cc;
            WL_MARKER(13)
        } else if (any_pressed_s261e0(g_game_keys.entry[4], 2) && f2c8 == 7 && f2d4 != 0) {
            WL_MARKER(12)
            MissionItem_s261e0& it = item_vec_s261e0()[f2cc];
            int pair[2];
            pair[0] = it.f18c;
            pair[1] = it.f1a0;
            if (((Mission_s2b5c0*)g_mission_s261e0)->order_agent(g_mission_s261e0->local_id, (ItemRef_s2b5c0*)pair)) {   // 0x52b5c0 is the thiscall member; this = the mission
                ((Mission_s27c90*)g_mission_s261e0)->release_item(f2cc);
                update_equipment();
            }
            WL_MARKER(13)
        } else if (any_pressed_s261e0(g_game_keys.entry[4], 2) && f2c8 == 5 && f2d4 != 0) {
            MissionItem_s261e0& rec = item_vec_s261e0()[f2cc];
            WL_MARKER(12)
            int v = rec.f1a0;
            if (v >= *(int*)((char*)g_game_s261e0->f13a50 + g_mission_s261e0->f5df4 * 0x494 + 0x100) || v < 0)
                val14 = -1;
            else
                val14 = *(int*)((char*)g_game_s261e0->f13a50 + 0xc0 +
                                (g_mission_s261e0->f5df4 * 0x125 + v) * 4);
            flag13 = 1;
            WL_MARKER(13)
        } else if (any_pressed_s261e0(g_game_keys.entry[4], 2) && f2c8 == 6 && f2d4 != 0) {
            WL_MARKER(12)
            int idx = item_vec_s261e0()[f2cc].f1a0;
            g_game_s261e0->f13af8 = idx;
            int block = item_vec_s261e0()[f2cc].f1a4;
            WL_MARKER(13)
            char* obj = *(char**)((char*)g_mission_templates + g_mission_ctx.passage_menu * 0x60 + 0x58);
            ((DialogBlock*)(obj + block * 0x68 + 0x4c))->play(0x10);
        }

        if (any_pressed_s261e0(g_game_keys.entry[16], 2)) {
            g_script.trigger_result.value = 0;
            int* slots = rec_base_s261e0(g_mission_s261e0->cur_rec);
            for (int i = 0; i < *slots; i++)
                ((snd_slot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-0x16);
            if (0 != g_script.trigger_result.value)
                flag13 = 1;
        }
    }

    if (g_cfg.enable_edit_mode && g_keyboard.down[56] && g_keyboard.down[38] == 0 && g_keyboard.prev[38] != 0)
        ((Mission*)g_mission_s261e0)->light_entities((rgl::mesh**)((char*)g_mission_s261e0 + 0x6340), g_mission_s261e0->f633c);   // 0x47ce00: thiscall member, this = the mission (ecx at the call)

    if (!(f2d8 == 0)) { if (f2dc == 0) {
        int* slots = rec_base_s261e0(g_mission_s261e0->cur_rec);
        for (int i = 0; i < *slots; i++)
            ((snd_slot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-0x1e);
        b6 = false;
    } } else {
        b6 = false;
    }

    if (any_pressed_s261e0(g_game_keys.entry[11], 2) && g_keyboard.down[56] == 0 && g_keyboard.down[184] == 0) {
        g_script.trigger_result.value = 0;
        ((snd_bank*)rec_base_s261e0(g_mission_s261e0->cur_rec))->update_all(-0x15);
        if (g_script.trigger_result.value != 0)
            b6 = false;
    }

    if (g_game_s261e0->f1a208 >= 0) {
        int v = g_game_s261e0->f1a208;
        g_script.param1 = v;
        int* slots = rec_base_s261e0(g_mission_s261e0->cur_rec);
        for (int i = 0; i < *slots; i++)
            ((snd_slot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-8);
        g_game_s261e0->f1a208 = -1;
    }

    // ---- "leave area" / agent selection ------------------------------------------------------------
    if ((g_mission_s261e0->f6528 & 1) != 0) {
        if (any_pressed_s261e0(g_game_keys.entry[4], 2) && f2c8 == 8)
            f2dc = 0;
    } else if (any_pressed_s261e0(g_game_keys.entry[4], 2) && f2c8 == 8 &&
               (!template_bit1_s261e0(g_mission_s261e0->cur_rec) || g_mission_s261e0->f6524 > 0)) {
        f2dc = 0;
    } else if (template_bit1_s261e0(g_mission_s261e0->cur_rec) && g_mission_s261e0->local_id >= 0 &&
               g_game_keys.clicked(4) && f2c8 == 8) {
        float best = 10000.0f;
        for (unsigned id = g_mission->agents.first_used(); id < (unsigned)g_mission->agents.count; g_mission->agents.next_used(&id)) {
            Agent_s261e0* a = page_agent_s261e0(g_mission_s261e0, id);
            if (a->f14 == 1 && a->f10 == 0 && ((AgentX*)page_agent_s261e0(g_mission_s261e0, g_mission_s261e0->local_id))->is_enemy((Agent const*)a)) {
                vector4 d = *(vector4*)&a->px - *(vector4*)&page_agent_s261e0(g_mission_s261e0, g_mission_s261e0->local_id)->px;
                float dist = (float)sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
                if (dist < best)
                    best = dist;
            }
        }
        if (g_game_keys.clicked(4) && f2c8 == 8) {
            if (best > 8.0f) {
                int* result = &f2dc;
                *result = -1;
                {
                    tstring no("ui_no");
                    tstring yes("ui_yes");
                    tstring question("ui_question_leave_area");
                    // 0x527591: ecx = [0xa73980] (the dialog box the menu points to), unlike set_state/open (ecx = 0xa73980)
                    g_windows.box()->setup2(g_strtable.lookup(question), g_strtable.lookup(yes), g_strtable.lookup(no), (int)result);
                }
                g_windows.open(0);
            } else {
                tstring t("ui_cant_retreat_there_are_enemies_nearby");
                g_gui.add(*g_strtable.lookup(t), 0xffff3344);
            }
        }
    }

    // ---- conversation / mission setup --------------------------------------------------------------
    if (g_game_s261e0->fc8 == 0xf) {
        g_game_s261e0->fc8 = 0;
        f324 = g_game_s261e0->f13b04;
    }
    if (f324 >= 0 && g_presentation == 0) {
        ((BlockOwner*)((char*)this + 0x30))->init((ContainerBase*)frame, f324);
        if ((bh.flags44 & 1) == 0) {
            if (g_render.dev != 0)
                ((void (__stdcall*)(void*, int))*(void**)(*(int*)g_render.dev + 0x30))(g_render.dev, 1);
            g_gui.show_cursor = 1;
        }
    }
    if (g_presentation != 0) {
        ((MissionSource*)((char*)this + 0x30))->pick((int*)&val14);
        __int64 d = g_ticks[bh.tick.clock] - bh.tick.start;
        if ((float)d * 1e-5f > bh.f58) {
            g_presentation = 0;
            if (bh.owned.begin != 0)
                operator delete(bh.owned.begin);
            bh.owned.begin = 0;
            bh.owned.end = 0;
            bh.owned.cap = 0;
            ((SrcVtbl_s261e0*)bh.f5c)->v49();
            if (g_render.dev != 0)
                ((void (__stdcall*)(void*, int))*(void**)(*(int*)g_render.dev + 0x30))(g_render.dev, 0);
            g_gui.show_cursor = 0;
            f324 = -1;
        }
    }

    if (!b6) {
        // ---- boolean-false branch ------------------------------------------------------------------
        g_mission_ctx.tick();
        if (w18 == 100) {
            if (0 < g_mission_ctx.passage_menu) {
                if ((*(unsigned __int64*)((char*)g_menus + g_mission_ctx.passage_menu * 0x60 + 0x20) & 0x10) != 0)
                    g_game_s261e0->fa8 |= 1;
            }
        } else {
            if (g_game_s261e0->f13ad0 > 0.0f && g_game_s261e0->f13ad4 != 0)
                g_windows.set_state(7);
            else
                g_windows.set_state(w18);
            goto unlock;
        }
    } else {
        // ---- boolean-true branch -------------------------------------------------------------------
        if (flag13 != 0 && g_mission_s261e0->local_id >= 0 && g_game_s261e0->b18 == 0) {
            int v = val14;
            refresh_slots = 1;
            slot_as<Widget_s261e0>(8)->f18 = 0xe;
            if (v < 0) {
                slot_as<Widget_s261e0>(8)->f4 = 0;
                g_windows.set_state(8);
            } else {
                slot_as<Widget_s261e0>(8)->f4 = 3;
                slot_as<Widget_s261e0>(8)->f14 = v;
                if (slot_as<Widget_s261e0>(8)->f14 < 0) {
                    slot_as<Widget_s261e0>(8)->f14 = 0;
                    g_windows.set_state(8);
                } else {
                    int lim = g_game_s261e0->f13a04 - 1;
                    if (slot_as<Widget_s261e0>(8)->f14 > lim)
                        slot_as<Widget_s261e0>(8)->f14 = lim;
                    g_windows.set_state(8);
                }
            }
            goto after;
        }

        if (any_pressed_s261e0(g_game_keys.entry[15], 2) && g_mission_s261e0->local_id >= 0 && g_game_s261e0->b18 == 0) {
            grow_health = 1;
            char* rec = (char*)g_game_s261e0->chars;
            int base = *(int*)(rec + 0x58);
            int v = (unsigned char)rec[0x7a] & 0xf;
            if (v < 0)                                    // (always false after the mask; the original tests it: and eax,0xf / jge)
                v = 0;
            else if (v > g_clamp_max)
                v = g_clamp_max;
            status_height = base + 0x23 + v * 2;
            slot_as<Widget_s261e0>(15)->f18 = 0xe;
            slot_as<Widget_s261e0>(15)->f4 = 0;
            slot_as<Widget_s261e0>(15)->f14 = g_game_s261e0->f8c;
            g_windows.set_state(0xf);
            goto after;
        }

        if (any_pressed_s261e0(g_game_keys.entry[17], 2) && g_game_s261e0->b18 == 0 &&
            !template_bit1_s261e0(g_mission_s261e0->cur_rec)) {
            slot_as<Widget_s261e0>(12)->f18 = 0xe;
            slot_as<Widget_s261e0>(12)->f4 = 5;
            g_windows.set_state(0xc);
            goto after;
        }

        if (any_pressed_s261e0(g_game_keys.entry[18], 2) && g_game_s261e0->b18 == 0) {
            if (g_module_settings.show_quest_notes != 0) {
                slot_as<Widget_s261e0>(26)->f18 = 0xe;
                *(int*)((char*)g_game_s261e0 + 0x13b08) = 4;
                *(int*)((char*)g_game_s261e0 + 0x13b0c) = 0;
                g_windows.set_state(0x1a);
            } else {
                slot_as<Widget_s261e0>(13)->f18 = 0xe;
                slot_as<Widget_s261e0>(13)->f4 = 0;
                g_windows.set_state(0xd);
            }
            goto after;
        }

        if (any_pressed_s261e0(g_game_keys.entry[19], 2) && g_game_s261e0->b18 == 0) {
            slot_as<Widget_s261e0>(25)->f18 = 0xe;
            slot_as<Widget_s261e0>(25)->f4 = 0;
            g_windows.set_state(0x19);
            goto after;
        }

        if (g_keyboard.down[0x1] != 0 && g_keyboard.prev[0x1] == 0) {
            if (((Mission*)g_mission_s261e0)->mode_flag1()) {
                slot_as<Widget_s261e0>(18)->f18 = 0xe;
                g_windows.set_state(0x12);
            } else {
                slot_as<Widget_s261e0>(5)->f18 = 0xe;
                g_windows.set_state(5);
            }
            goto after;
        }

        if (g_game_s261e0->b18 == 0 && g_game_keys.clicked(0x24)) {
            if (!((Mission*)g_mission_s261e0)->mode_flag1() && g_game_s261e0->b18 == 0) {
                quick_save_msg();
            } else {
                tstring t("ui_cant_quick_save");
                g_gui.add(*g_strtable.lookup(t), 0xffffffff);
            }
            goto after;
        }

        if (g_mission_s261e0->f6620 != -1) {
            if (g_mission_s261e0->f6620 < 0)
                goto after;
            refresh_slots = 1;
            unsigned id = (unsigned)g_mission_s261e0->f6620;
            g_mission_s261e0->f6620 = -1;
            int dialog_id = page_agent_s261e0(g_mission_s261e0, id)->f88;
            slot_as<Widget_s261e0>(16)->f18 = 0xe;
            g_dialogs.start(dialog_id, (int)id, -1, 0, 0);
            g_mission_s261e0->f6558 = page_agent_s261e0(g_mission_s261e0, id)->f88;
            g_game_s261e0->f13a08 = page_agent_s261e0(g_mission_s261e0, id)->f88;
            g_windows.set_state(0x10);
            goto after;
        }

        if (g_game_s261e0->f13af4 < 0)
            goto after;
        g_mission_ctx.tick();
        g_mission_ctx.f3c |= 8;
    }
    g_map_flags.commit((char*)&flag13);

    }
after:
unlock:
    if (g_mt_inited) {
        LeaveCriticalSection(&g_mt_cs);
        SetEvent(g_mt_event);
    }
}

// ======================================================= slice s00527c90: functions
#undef g_game_s27c90
#define g_game_s27c90 (*(GameData_s27c90**)&g_game)
#undef g_mission_s27c90
#define g_mission_s27c90 (*(Mission_s27c90**)&g_mission)
// @ 0x527c90
void GameData_s27c90::save_scene(int idx)
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
void SceneObjRec_s27c90::save(stream_base* s, int arg)
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
    for (unsigned i = 0; i < objs.size(); i++) {
        if (objs[i].f10 == arg)
            count++;
    }
    s->w_int(&count);
    for (unsigned j = 0; j < objs.size(); j++) {
        if (objs[j].f10 == arg)
            objs[j].save(s);
    }
    stream_b.rewind();
    s->w_stream(&stream_b);
    if (!stream_a.at_end()) {
        int marker = 0xff4ad1a6;
        s->w_int(&marker);
        stream_a.rewind();
        s->w_stream(&stream_a);
    }
}

// @ 0x527ef0
void SceneObj_s27c90::save(stream_base* s)
{
    s->w_int(&f188);
    s->w_int(&f18c);
    int tmp;
    s->w_int(&tmp);
    w_matrix(*s, (rgl::matrix*)&mat[0]);
    s->w_tstr(&name);
    s->w_int(&f1a0);
    s->w_int(&f1a4);
    float* p = &f158;
    s->w_float(p).w_float(p + 1).w_float(p + 2);
}

// @ 0x527f90
void Mission_s27c90::save_scene_data()
{
    SceneObjRec_s27c90* rec = g_game_s27c90->scenes + cur_scene;
    if (rec->flags30 & 0x100) {
        rec->stream_a.reset();
        grids.save(&rec->stream_a);
    }
    rec->stream_b.reset();
    other.read(&rec->stream_b);
}

// @ 0x528000
void GridSet_s27c90::save(stream_base* s)
{
    s->w_int(&count);
    s->w_int(&width);
    s->w_int(&height);
    int i = 0;
    if (count > 0) {
        Grid_s27c90* g = grids;
        do {
            g->save(s, &width);
            i++;
            g++;
        } while (i < count);
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
void Grid_s27c90::save(stream_base* s, int* dims)
{
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
void Mission_s27c90::release_item(int idx)
{
    std::vector<MissionItem_s27c90>& items = *(std::vector<MissionItem_s27c90>*)((char*)this + 0x6688);
    if (items[idx].owner) {
        items[idx].owner->release();
        ((MissionHdr_s27c90*)this)->world->remove_entity(*(int*)((char*)items[idx].owner + 8));
        items[idx].owner = 0;
    }
}

// @ 0x528470
void ResourceOwner_s27c90::release()
{
    Resource_s27c90** p = first;
    if (p < last) {
        do {
            (*p)->flags |= 0x8000;
            p++;
        } while (p < last);
    }
}

// ======================================================= slice s005284a0: functions
#undef g_items_s284a0
#define g_items_s284a0 (*(ItemKind_s284a0**)&g_items)
#undef g_mission_s284a0
#define g_mission_s284a0 (*(Mission_s284a0**)&g_mission)
#undef g_game_s284a0
#define g_game_s284a0 (*(Game_s284a0**)&g_game)
#undef g_key_names_s284a0
#define g_key_names_s284a0 (*(KeyNames_s284a0*)&g_keyboard)
#undef AGENT
#define AGENT(id) (g_mission_s284a0->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))
static __forceinline Agent& ag_s284a0(unsigned id) { return g_mission->agents.pages[id >> 4][id & 0xf]; }
static __forceinline Agent& agr_s284a0(unsigned id) { Agent** pages = *(Agent***)((char*)g_mission + 0x1c); return *(pages[id >> 4] + (id & 0xf)); }
static __forceinline float cl0_s284a0(float x) { if (0.0f > x) x = 0.0f; return x; }
// @ 0x5284a0 ------------------------------------------------------------------------------------------
void ViewCtrl_s284a0::update(float dt)
{
    if (g_game_s284a0->f1a090 && (g_keyboard.down[45] || g_keyboard.down[21] || g_keyboard.down[44] || g_keyboard.down[22] ||
                   g_keyboard.down[47] || g_keyboard.down[34] || g_keyboard.down[20]))
        return;
    if (g_game_s284a0->f1a090)
        dt = g_step_tbl[0];
    float k = expf(g_options.exp_arg * 4.0f) * 0.04f;

    // ---- aiming at an agent while a melee weapon (or the fists) is out is slower ---------------------
    if (!g_game_s284a0->f1a090) {
        if (g_presentation == 0 || (flags44 & 1)) {
            if (g_mission->f5c >= 0) {
                Agent_s284a0* a = (Agent_s284a0*)&agr_s284a0(g_mission->f5c);
                ItemRef_s284a0 w0 = a->slot_pair(0);
                ItemRef_s284a0 w1 = a->slot_pair(1);
                bool hand0;
                if (w0.item < 0) {
                    hand0 = true;
                } else {
                    int t = g_items_s284a0[w0.item].type_flags & 0xff;
                    hand0 = (t == 2 || t == 3 || t == 4);
                }
                if (((hand0 && a->f560c == 1 && g_options.ctl_mode > 0 && a->f100 == a->f55ec) ||
                     (w1.item < 0 && a->f5610 > 0 && g_options.stick_mode > 0 && a->f100 == a->f5614)) &&
                    a->f118 < 0.6f)
                    k *= 0.1f;
            }
        }
    }

    // ---- look input ---------------------------------------------------------------------------------
    float look_y = 0.0f;
    float look_x = 0.0f;
    if (g_presentation == 0 || (flags44 & 1)) {
        look_x = (*(float*)((char*)&g_keyboard + 0x80c));
        look_y = (*(float*)((char*)&g_keyboard + 0x810));
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
    if (!g_game_s284a0->f1a090 || (*(int*)((char*)&g_keyboard + 0x38c)) != 0) {
        { float r = 1.0f - dt * 20.0f; if (0.0f > r) r = 0.0f; yaw_rate = r * yaw_rate; }
        { float r = 1.0f - dt * 20.0f; if (0.0f > r) r = 0.0f; pitch_rate = r * pitch_rate; }
        yaw_rate = look_x * fov * 0.0018f + yaw_rate;
        pitch_rate = (g_options.invert ? -look_y : look_y) * fov * 0.0018f + pitch_rate;
        yaw = yaw - yaw_rate * k * dt;
        pitch = pitch - pitch_rate * k * dt;
    }

    // ---- clamps -------------------------------------------------------------------------------------
    float& p_ref = pitch;
    if (p_ref < -0.9519978165626526f)
        p_ref = -0.9519978165626526f;
    else if (p_ref > 0.9519978165626526f)
        p_ref = 0.9519978165626526f;
    if (look_speed < 0.7f)
        look_speed = 0.7f;
    else if (look_speed > 10.0f)
        look_speed = 10.0f;

    if (g_mission->f5c < 0)
        return;

    // ---- build the view frame and hand its forward row to the local agent ----------------------------
    Agent_s284a0* a = (Agent_s284a0*)&agr_s284a0(g_mission->f5c);
    {
        matrix m;
        identity3((rgl::mat3*)&m);
        ((MatrixExt_s284a0*)&m)->rotate_sf(&yaw);
        ((MatrixExt_s284a0*)&m)->rotate_fu(&p_ref);

        bool looking = false;
        if (g_game_keys.entry[13][0] >= 0 && g_keyboard.down[g_game_keys.entry[13][0]] != 0)
            looking = true;
        if ((g_game_keys.entry[13][1] < 0 || g_keyboard.down[g_game_keys.entry[13][1]] == 0) && !looking)
            a->dir55d8 = m.f;
    }

    if (snap < 5) {
        a->dir55d8 = a->dir;        // copied through `a` itself: the scheduler then knows the stores cannot alias the loads
        const vector4& d = a->dir;
        float nx = -d.x;
        float ang = (float)atan2(nx, d.y);
        int n = snap + 1;
        p_ref = 0.0f;
        snap = n;
        yaw = ang;
    }

    // ---- movement / action bits ---------------------------------------------------------------------
    a->in_flags = 0;
    if (!g_game_s284a0->f1a090) {
        if (g_keyboard.down[42] == 0)
            a->in_flags = 0x200000;
        if (g_game_keys.clicked(5))
            a->in_flags |= 0x100000;

        if (a->mount_id >= 0) {
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
            if (g_game_keys.clicked(4) && mode == 3)
                a->in_flags |= 0x10000;
            if (g_key_names_s284a0.is_ready(0x24) && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0))
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
            if (g_game_keys.clicked(4) && mode == 2 && aim_valid != 0) {
                a->in_flags |= 0x20000;
                a->target_id = target;
            }
        }

        if (g_presentation == 0 || (flags44 & 1)) {
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
            a->wield_a = -1;
            a->on_wield_reset();
        }
    }

    if (g_game_keys.clicked(0xe)) {
        g_game_s284a0->f139e4 = (g_game_s284a0->f139e4 == 0);
        update_equipment();
    }
    {
        unsigned id = g_mission->f5c;
        Agent** pages = *(Agent***)((char*)g_mission + 0x1c);
        (pages[id >> 4] + (id & 0xf))->f55d4 = (g_game_s284a0->f139e4 != 0);
    }

    // ---- field of view ------------------------------------------------------------------------------
    bool zoom_key = false;
    if (g_game_keys.entry[12][0] >= 0 && g_keyboard.down[g_game_keys.entry[12][0]] != 0)
        zoom_key = true;
    if ((g_game_keys.entry[12][1] >= 0 && g_keyboard.down[g_game_keys.entry[12][1]] != 0) || zoom_key)
        fov = fov - dt * 350.0f;
    else
        fov = fov + dt * 350.0f;

    float limit = 97.0f;
    if (g_game_s284a0->f139e4 != 0)
        limit = 90.0f;
    if (fov < 37.0f)
        fov = 37.0f;
    else if (fov > limit)
        fov = limit;

    // ---- the "use / attack" keys walk the agent table and hit the agent under the crosshair ---------
    if (g_cfg.cheat_mode != 0 && (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) && g_keyboard.down[63] != 0 && g_keyboard.prev[63] == 0) {
        if (agr_s284a0(g_mission->f5c).f7c == 2)
            g_mission->agents.get(g_mission->f5c)->f7c = 1;
        else
            g_mission->agents.get(g_mission->f5c)->f7c = 2;
    }

    if (g_cfg.cheat_mode != 0 && g_keyboard.down[29] != 0 && g_key_names_s284a0.is_ready(0x3e)) {
        bool done = false;
        for (unsigned i = g_mission->agents.first_used(); i < (unsigned)g_mission->agents.count && !done; g_mission->agents.next_used(&i)) {
            Agent* o = &agr_s284a0(i);
            if (o->f14 == 1 && o->f10 == 0 && ((AgentX*)g_mission->agents.get(g_mission->f5c))->is_enemy(o)) {
                Blow b;
                b.init();
                b.kind = 2;
                b.bone = 0;
                b.attacker = g_mission->f5c;
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
            if (i != (unsigned)g_mission->f5c && o->f14 == 1 && o->f10 == 0 && g_mission->agents.get(g_mission->f5c)->is_friend(o)) {
                Blow b;
                b.init();
                b.kind = 2;
                b.bone = 0;
                b.attacker = g_mission->f5c;
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
        b.attacker = g_mission->f5c;
        b.pos = ag_s284a0(g_mission->f5c).pos;
        b.dmg_in = 200.0f;
        agr_s284a0(g_mission->f5c).note_blow(&b);
    }

    if ((g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0) && g_keyboard.down[35] != 0 && g_keyboard.prev[35] == 0 && g_cfg.cheat_mode != 0)
        agr_s284a0(g_mission->f5c).health = agr_s284a0(g_mission->f5c).max_health;
}

// ======================================================= slice s00529130: functions
#undef g_scene_s29130
#define g_scene_s29130 (*(Mission_s29130**)&(*(void**)((char*)&g_mission + 0x0)))
#undef g_game_s29130
#define g_game_s29130 (*(GameData_s29130**)&g_game)
#undef g_items_s29130
#define g_items_s29130 (*(ItemKind_s29130**)&g_items)
static __forceinline Agent_s29130* cur_at_s29130()
{
    std::vector<Agent_s29130*>& pages = *(std::vector<Agent_s29130*>*)((char*)g_scene_s29130 + 0x18);
    return pages[g_scene_s29130->cur_agent_s29130 >> 4] + (g_scene_s29130->cur_agent_s29130 & 0xf);
}
struct AimRange_s29130 { float lo, hi; };
struct V2_s29130 {
    float x, y;
    V2_s29130() {}
    __forceinline V2_s29130(float a, float b) : x(a), y(b) {}
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
    Agent_s29130* a = &g_scene_s29130->agents.pages[g_scene_s29130->cur_agent_s29130 >> 4][g_scene_s29130->cur_agent_s29130 & 0xf];

    V2_s29130 base;
    float h = 0.5f / tanf(0.5f * (zoom * 0.017453292f));
    base.y = h * tanf(a->aim_error);
    base.x = 0.0f;
    {
        V2_s29130 p(0.5f, base.y + 0.375f);
        w[0]->move_a((float*)&p);
    }
    {
        V2_s29130 p = base;
        p.rotate(2.268928f);
        p.x = p.x + 0.5f;
        p.y = p.y + 0.375f;
        w[1]->move_a((float*)&p);
    }
    {
        V2_s29130 p = base;
        p.rotate(-2.268928f);
        p.x = p.x + 0.5f;
        p.y = p.y + 0.375f;
        w[2]->move_a((float*)&p);
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

    ItemRef_s29130 pair = a->slot_pair(0);
    float aim = signed_angle((const vec2*)a->blk60, (const vec2*)&a->facing);

    if (pair.item >= 0) {
        bool other = false;
        if (g_game_keys.entry[13][0] >= 0 && g_keyboard.down[g_game_keys.entry[13][0]]) other = true;
        if ((g_game_keys.entry[13][1] < 0 || g_keyboard.down[g_game_keys.entry[13][1]] == 0) && !other) {
            int type = g_items_s29130[pair.item].type & 0xff;
            if ((type == 8 || type == 9 || type == 10 || type == 16 || type == 17) && g_options.i64) {
                if (!((AgentX*)cur_at_s29130())->f100_is_0x48_and_charged()) {
                    if (g_scene_s29130->agents.pages[g_scene_s29130->cur_agent_s29130 >> 4][g_scene_s29130->cur_agent_s29130 & 0xf].mount == -1 ||
                        (((const AimRange_s29130*)g_aim_range_tbl)[a->f56c4].lo <= aim && aim < ((const AimRange_s29130*)g_aim_range_tbl)[a->f56c4].hi)) {
                        target[0] = 0.9f;
                        target[1] = 0.9f;
                        target[2] = 0.9f;
                    }
                }
            } else if (type == 2 || type == 3 || type == 4) {
                if (g_options.i68 || !g_options.ctl_mode) {
                    int t = a->pick_front();
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
    g_game_s29130->more_start();
    if (f2c8 != 0 || (f154 >= 0 && (((char*)g_mission_templates)[f154 * 0x58 + 0x20] & 0x10))) {
        refresh_markers();
    }
    WL_MARKER(13)
}

// @ 0x529600
void MissionCtx::refresh_markers()
{
    int n = 1;
    if (g_scene_s29130->f6524 > 0)
        n = 0x701;
    else if (g_scene_s29130->f6524 < 0)
        n = 0x7031;
    g_scene_s29130->fn_529840(n);
    if (mode == 3 || mode == 4 || mode == 1) {
        reset_party_heroes(g_game_s29130->cur_party);
        if (party[0] >= 0) reset_party_heroes(party[0]);
        if (party[1] >= 0) reset_party_heroes(party[1]);
    }
}

// @ 0x529670
void MissionCtx::reset_party_heroes(int party_id)
{
    PartyMgr* mgr = party_mgr_s29130();
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
        Troop_s29130* t = (Troop_s29130*)(*(int*)((char*)g_game_s29130 + 0x13a00) + party->stacks[j].troop * 0x508);
        if (t->flags & 0x10) {
            if (t->f_a0 < 0.0f) t->f_a0 = 0.0f;
        }
    }
}

// ======================================================= slice s00529840: functions
#undef g_scene_s29840
#define g_scene_s29840 (*(Mission_s29840**)&(*(void**)((char*)&g_mission + 0x0)))
#undef g_game_s29840
#define g_game_s29840 (*(GameData_s29840**)&g_game)
// @ 0x529840
void Mission_s29840::fn_529840(int)
{
    unsigned i = agents.first_used();
    while (i < (unsigned)agents.count) {
        std::vector<Agent_s29840*>& pgs = *(std::vector<Agent_s29840*>*)&agents.pad4[0x14 - 4];
        Agent_s29840* a = pgs[i >> 4] + (i & 0xf);
        if (a->f10 != 1 && a->f10 == 0) {
            if (a->f1bc >= 0 && a->f1c0 >= 0) {
                Troop_s29840* t = &troop_tbl_s29840()[a->troop];
                if (!(t->flags & 0x10) && a->f1bc == g_game_s29840->cur_party && t->f50 > 0) {
                    // the xp stays an exact integer / x87 value until the _ftol
                    float xp = (float)a->f5730;
                    int lv = t->b0;
                    int cur = troop_tbl_s29840()[g_game_s29840->cur_troop].b0;
                    if (lv > cur)
                        xp = xp / ((lv - cur) + 7.0f) * 7.0f;
                    party_mgr_s29840()->get(a->f1bc)->add_xp_to_stack(a->f1c0, (int)xp);
                }
                t = &troop_tbl_s29840()[a->troop];
                if (t->flags & 0x10) {
                    t->f_a0 = a->f5724;
                    Party* p = party_mgr_s29840()->at(party_mgr_s29840()->lookup(a->f1bc));
                    int skill = p->skill_level(9);
                    float v = a->f571c - a->f5724;
                    if (0.0f > v) v = 0.0f;
                    troop_tbl_s29840()[a->troop].f_a0 += (skill * 0.05f + 0.1f) * v;
                }
                if (a->f14 != 1) {
                    if (a->f14 == 5)
                        fn_529b70(a);
                    else if (a->f14 == 4)
                        fn_529d00(a);
                }
            } else {
                if (troop_tbl_s29840()[a->troop].flags & 0x10) {
                    troop_tbl_s29840()[a->troop].f_a0 = a->f5724;
                    if (a->troop == g_game_s29840->cur_troop) {
                        Party* p = party_mgr_s29840()->at(party_mgr_s29840()->lookup(g_game_s29840->cur_party));
                        int skill = p->skill_level(9);
                        float v = a->f571c - a->f5724;
                        if (0.0f > v) v = 0.0f;
                        troop_tbl_s29840()[a->troop].f_a0 += (skill * 0.05f + 0.1f) * v;
                    }
                }
            }
        }
        agents.next_used(&i);
    }
}

// @ 0x529b70
void Mission_s29840::fn_529b70(Agent_s29840* a)
{
    fn_529bc0(a, 1);
    if (a->f1bc == g_game_s29840->cur_party)
        fn_529c30((int*)((char*)g_game_s29840 + 0x1405c), a->f1bc, a->f1c0, a->f94, 0);
}

// @ 0x529bc0
__declspec(noinline) void __stdcall fn_529bc0(Agent_s29840* a, int add)
{
    if (a->f1bc < 0) return;
    int slot = a->f1c0;
    if (slot < 0) return;
    if (add)
        party_mgr_s29840()->get(a->f1bc)->stacks[slot].count--;
    else
        party_mgr_s29840()->get(a->f1bc)->stacks[slot].f10++;
}

// @ 0x529c30
void Mission_s29840::fn_529c30(int* hist, int party_id, int slot, int value, int flag)
{
    Party* p = party_mgr_s29840()->at(party_mgr_s29840()->lookup(party_id));
    int count = p->stacks[slot].count;
    int found = -1;
    int (*g)[32] = (int (*)[32])hist;
    for (int i = 0; i < count; i++) {
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
void Mission_s29840::fn_529d00(Agent_s29840* a)
{
    if (a->f1bc >= 0) {
        int slot = a->f1c0;
        if (slot >= 0) {
            Party* p = party_mgr_s29840()->get(a->f1bc);
            int* v = (int*)p->stacks + slot * 8 + 4;
            (*v)++;
        }
    }
    if (a->f1bc == g_game_s29840->cur_party)
        fn_529c30((int*)((char*)g_game_s29840 + 0x1405c), a->f1bc, a->f1c0, a->f94, 1);
}

// @ 0x529d80
void Mission_s29840::fn_529d80()
{
    Scene_s29840* sc = (Scene_s29840*)(*(char**)((char*)g_game_s29840 + 0x13a50) + f5df4 * 0x494);
    for (unsigned i = 0; i < records.size(); i++) {
        if (records[i].m10 == 0 && records[i].m14 >= 0) {
            PlacedObj& q = sc->recs[records[i].m14];
            records[i].id = q.id;
            records[i].name = q.name;
        }
    }
    std::vector<PlacedObj>& recs = sc->recs;
    recs.resize(0);
    for (unsigned i = 0; i < records.size(); i++) {
        PlacedObj* r = &records[0] + i;
        if (r->m10 == 0) {
            recs.push_back(*r);
            r->m14 = (int)recs.size() - 1;
        }
    }
}

// ======================================================= slice s00529ec0: functions
#undef g_strtable_s29ec0
#define g_strtable_s29ec0 g_strtable
#undef g_items_s29ec0
#define g_items_s29ec0 (*(ItemKind_s29ec0**)&g_items)
#undef g_game_s29ec0
#define g_game_s29ec0 (*(GameData_s29ec0**)&g_game)
#undef g_prop_kinds_s29ec0
#define g_prop_kinds_s29ec0 (*(PropKind_s29ec0**)&g_propkinds)
#undef g_mission_s29ec0
#define g_mission_s29ec0 (*(Mission_s29ec0**)&g_mission)
#undef g_mapobjs_s29ec0
#define g_mapobjs_s29ec0 (*(MapObjList_s29ec0**)&g_menus)
#undef g_areas_s29ec0
#define g_areas_s29ec0 (*(Area_s29ec0**)&g_mission_templates)
// the length the original inlines in this function: x*x + z*z + y*y (0x52a2af, 0x52a337, 0x52a786)
static inline float len_xzy_s29ec0(const vector4& v)
{
    return (float)sqrt(v.x * v.x + v.z * v.z + v.y * v.y);
}
// the mission's agent table: pages vector at +0x18 (first at +0x1c)
static __forceinline Agent_s29ec0* table_agent_s29ec0(unsigned id)
{
    std::vector<Agent_s29ec0*>& pages = *(std::vector<Agent_s29ec0*>*)((char*)g_mission_s29ec0 + 0x18);
    return pages[id >> 4] + (id & 0x0f);
}
static __forceinline std::vector<PlacedObj>& props_s29ec0()
{
    return *(std::vector<PlacedObj>*)((char*)g_mission_s29ec0 + 0x6688);
}
static inline bool area_bit1_s29ec0()
{
    return ((g_areas_s29ec0[g_mission_s29ec0->f5df0].flags >> 1) & 1) != 0;
}
// @ 0x529ec0
void Screen_s29ec0::f_529ec0()
{
    contact c;
    Agent_s29ec0* me = table_agent_s29ec0(g_mission_s29ec0->local_id);
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

    capsule_s29ec0 cap(1e-4f, &pos, &front);
    matrix ident;
    cap.transform((const ::pos*)identity_pos(&ident));

    shape_base_s29ec0* found[0x2000];
    int count;

    best = 1e9f;
    second = 1e9f;
    g_mission_s29ec0->scene->query_segment((const float*)&pos, (const float*)&front, 0x110, (shape_base**)found, &count);
    {
        int n = count;
        for (int i = 0; i < n; i++) {
            if (found[i]->shape_type() == 2) {
                if (shape_contact_count((shape_base*)found[i], &cap, 1, &c)) {
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

    g_mission_s29ec0->scene->collect(9, (rgl::entity**)found, &count);
    for (int i = 0; i < count; i++) {
        SceneEntity_s29ec0* e = (SceneEntity_s29ec0*)found[i];
        if (!e->shape)
            continue;
        contact c;
        if (!shape_contact_count((shape_base*)e->shape, &cap, 1, &c))
            continue;

        vector4 d = c.point - pos;
        float len = len_xzy_s29ec0(d);
        if (!(len <= best))        // 0x52a2d1: test ah,0x41 / jp -- skips on len > best and on NaN
            continue;

        if (e->type == 0x0b) {
            Agent_s29ec0* a = table_agent_s29ec0(e->id);
            if (a->f10 == 0) {
                // a human: talk target, unless it is the player or an enemy
                if (e->id != g_mission_s29ec0->local_id && !((AgentX*)me)->is_enemy((Agent const*)a)) {
                    mode = 1;
                    target = e->id;
                    best = len;
                    vector4 r = e->pos - pos;
                    second = len_xzy_s29ec0(r);
                }
            } else if (a->f10 == 1) {
                // a ridden mount
                if (g_items_s29ec0[a->item.item].type_flags == 1 && a->f30 == -1 && a->f14 == 1 && a->f5830 == 2) {
                    target = e->id;
                    mode = 2;
                    best = d.out_length();
                    vector4 r = e->pos - pos;
                    second = r.out_length();   // 0x52a420: call 0x407c50 on the second vector as well
                }
            }
        } else if (e->type == 0x0c) {
            int id = e->id;
            PlacedObj& p = props_s29ec0()[id];
            if (p.kind == 0) {
                int flags = g_prop_kinds_s29ec0[p.id].flags & 0xff;
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
                if (g_mission_ctx.passage_menu > 0 && p.m1a4 >= 0 && p.m1a4 < g_mapobjs_s29ec0[g_mission_ctx.passage_menu].count) {
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
        if (me->mount != -1 && f26c < -0.5519978f && me->f9c < 1.0f) {
            mode = 3;
        } else if ((g_mission_s29ec0->f6544 - 0.5f < me->pos.x && me->facing.x > 0.5f) ||
                   (g_mission_s29ec0->f6548 - 0.5f < me->pos.y && me->facing.y > 0.5f) ||
                   (g_mission_s29ec0->f653c + 0.5f > me->pos.x && me->facing.x < -0.5f) ||
                   (g_mission_s29ec0->f6540 + 0.5f > me->pos.y && me->facing.y < -0.5f)) {
            mode = 8;
        }
    }
    }

    {
    // ---- phase D: validate --------------------------------------------------------------------------
    int ok = 0;
    if (mode == 1) {
        if (area_bit1_s29ec0() || (g_mission_s29ec0->f6528 & 1)) {
            if (!g_game_s29ec0->f13b10)
                goto no_selection;
        }
        if (g_game_s29ec0->f13b11)
            goto no_selection;
        if (second < 3.5f)
            ok = 1;
    } else if (mode == 2) {
        if (me->mount != -1)
            goto no_selection;
        {
            ItemRef_s29ec0& ir = table_agent_s29ec0(target)->item;
            int stat;
            if (ir.item < 0)
                stat = 0;
            else {
                ItemKind_s29ec0* k = &g_items_s29ec0[ir.item];
                stat = k->f1ec;
                if (stat != 0) {
                    int mod = ((signed char*)&ir.m4)[3];
                    if (k->type_flags == 1) {
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
            if (stat > ((GameData*)g_game_s29ec0)->skill_level(g_game_s29ec0->f8c, 0x18))
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
    g_gui.fa4 = 0;
    g_gui.fa8 = -0.03f;
    g_gui.fa0 = 0;
    g_gui.overlay = 0;
    g_gui.sel_ent = 0;
    float view[2];
    view[0] = 0.5f;
    view[1] = 0.23f;
    g_gui.set_view(view);

    // ---- phase F: name the selection -----------------------------------------------------------------
    if (mode == 1) {
        char* rec = g_game_s29ec0->chars + table_agent_s29ec0(target)->troop * 0x508;
        tstring* tr = g_strtable_s29ec0.find((tstring*)(rec + 0x20));
        if (!tr)
            tr = (tstring*)rec;
        name = *tr;
        if (ok) {
            tstring k("ui_talk");
            action = *g_strtable_s29ec0.lookup(k);
        }
        if (((AgentX*)table_agent_s29ec0(g_mission_s29ec0->local_id))->is_enemy((Agent const*)table_agent_s29ec0(target)))
            color = (int)0xffd06050;
        else if (((Agent*)table_agent_s29ec0(g_mission_s29ec0->local_id))->is_friend((Agent const*)table_agent_s29ec0(target)))
            color = (int)0xff60d050;
    } else if (mode == 2) {
        char buf[0x40];
        ((ItemRef*)&table_agent_s29ec0(target)->item)->name(buf);
        name.assign(buf);
        if (ok) {
            tstring k("ui_mount");
            action = *g_strtable_s29ec0.lookup(k);
        } else if (me->mount == -1) {
            int skill = ((GameData*)g_game_s29ec0)->skill_level(g_game_s29ec0->f8c, 0x18);
            if (((ItemRef*)&table_agent_s29ec0(target)->item)->stat_1ec() > skill) {
                tstring k("ui_riding_skill_not_adequate_to_mount");
                action = *g_strtable_s29ec0.lookup(k);
            }
        }
    } else if (mode == 3) {
        tstring k("ui_dismount");
        action = *g_strtable_s29ec0.lookup(k);
    } else if (mode == 4) {
        char* rec = g_game_s29ec0->map_records + g_mission_s29ec0->f5df4 * 0x494;
        int dest = *(int*)(rec + 0x7c + target * 4);
        if (dest > g_game_s29ec0->f13a54) {
            tstring k("ui_exit");
            action = *g_strtable_s29ec0.lookup(k);
        } else if (dest >= 0) {
            {
                tstring k("ui_door_to");
                name = *g_strtable_s29ec0.lookup(k);
            }
            name.append(*(tstring*)(g_game_s29ec0->map_records + *(int*)(rec + 0x7c + target * 4) * 0x494 + 0x20));
            name.underscores_to_spaces();
            if (ok) {
                tstring k2("ui_open");
                action = *g_strtable_s29ec0.lookup(k2);
            }
        }
    } else if (mode == 5) {
        int inv = props_s29ec0()[target].value;
        char* rec = g_game_s29ec0->map_records + g_mission_s29ec0->f5df4 * 0x494;
        if (inv >= *(int*)(rec + 0x100) || inv < 0) {
            {
                tstring k("ui_baggage");
                name = *g_strtable_s29ec0.lookup(k);
            }
            if (ok) {
                tstring k2("ui_access_inventory");
                action = *g_strtable_s29ec0.lookup(k2);
            }
        } else {
            {
                tstring k("ui_chest");
                name = *g_strtable_s29ec0.lookup(k);
            }
            if (ok) {
                tstring k2("ui_open");
                action = *g_strtable_s29ec0.lookup(k2);
            }
        }
    } else if (mode == 6) {
        MapObject_s29ec0* o = (MapObject_s29ec0*)((char*)g_mapobjs_s29ec0[g_mission_ctx.passage_menu].objs + props_s29ec0()[target].m1a4 * 0x68);
        name = *((MapObject*)o)->mesh_ptr();
        if (ok) {
            tstring k("ui_go");
            action = *g_strtable_s29ec0.lookup(k);
        }
    } else if (mode == 7) {
        PlacedObj& p = props_s29ec0()[target];
        ItemRef_s29ec0 ref;
        ref.item = p.id;
        ref.m4 = p.value;
        char buf[0x40];
        ((ItemRef*)&ref)->name(buf);
        name.assign(buf);
        if (ok) {
            tstring k("ui_equip");
            action = *g_strtable_s29ec0.lookup(k);
        }
    } else if (mode == 8) {
        name.assign("");
        if (area_bit1_s29ec0()) {
            tstring k("ui_retreat_battle");
            action = *g_strtable_s29ec0.lookup(k);
        } else {
            tstring k("ui_leave_area");
            action = *g_strtable_s29ec0.lookup(k);
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
    f2c8 = mode;
    f2cc = target;
    f2d4 = ok;
    }
}

// ======================================================= slice s0052ae70: functions
#undef g_strtable_s2ae70
#define g_strtable_s2ae70 g_strtable
#undef g_mission_s2ae70
#define g_mission_s2ae70 (*(Mission_s2ae70**)&g_mission)
// add_point / add_vertex take their arguments by const reference in the original (temporaries built after the pushes)
__forceinline int Screen_s2ae70::vtx(int p, const rgl::vec2_s2ae70& uv, const unsigned& color, const rgl::vector4& n, const int& zero)
{
    return ((rgl::mesh*)f673c)->add_vertex(p, (const ::vec2*)&uv, &color, &n, &zero);
}
// @ 0x52ae70
void Screen_s2ae70::init_order_markers()
{
    if (f6738 == 0) {
        f6738 = (Entity_s2ae70*)world->create_entity(9);
        float scale = 100000.0f;
        f6738->set_scale(&scale);

        f6730 = (rgl::mesh_s2ae70*)new rgl::mesh;
        f6734 = (rgl::mesh_s2ae70*)new rgl::mesh;
        f672c = (rgl::mesh_s2ae70*)new rgl::mesh;

        {
            tstring name("vertex_color_blend_mat");
            f6730->res = (material_s2ae70*)g_resmgr.get_material(name);
        }
        f6734->res = f6730->res;
        f672c->res = f6730->res;

        f6738->add_extra(f6730);
        f6738->add_extra(f6734);
        f6738->add_extra(f672c);

        Shape_s2ae70* s = &shape;
        s->polygon_to_mesh(f672c);
        s->build_ribbons(f6734);
        s->build_spikes(f6730);

        scale = 100000.0f;
        f6738->set_scale(&scale);
        f6738->f140 = 0;
    }

    if (f6740 == 0) {
        f6740 = (Entity_s2ae70*)world->create_entity(9);

        f673c = (rgl::mesh_s2ae70*)new rgl::mesh;
        {
            tstring name("vertex_color_blend_mat");
            f673c->res = (material_s2ae70*)g_resmgr.get_material(name);
        }
        f6740->add_extra(f673c);

        int p0 = ((rgl::mesh*)f673c)->add_point(&rgl::vector4(-1000.0f, -1000.0f, f654c));
        int p1 = ((rgl::mesh*)f673c)->add_point(&rgl::vector4(1000.0f, -1000.0f, f654c));
        int p2 = ((rgl::mesh*)f673c)->add_point(&rgl::vector4(1000.0f, 1000.0f, f654c));
        int p3 = ((rgl::mesh*)f673c)->add_point(&rgl::vector4(-1000.0f, 1000.0f, f654c));

        int v0 = vtx(p0, rgl::vec2_s2ae70(0.0f, 0.0f), 0x663333ff, rgl::vector4(0.0f, 0.0f, 1.0f), 0);
        int v1 = vtx(p1, rgl::vec2_s2ae70(0.0f, 0.0f), 0x663333ff, rgl::vector4(0.0f, 0.0f, 1.0f), 0);
        int v2 = vtx(p2, rgl::vec2_s2ae70(0.0f, 0.0f), 0x663333ff, rgl::vector4(0.0f, 0.0f, 1.0f), 0);
        int v3 = vtx(p3, rgl::vec2_s2ae70(0.0f, 0.0f), 0x663333ff, rgl::vector4(0.0f, 0.0f, 1.0f), 0);

        f673c->add_face(v0, v1, v2);
        f673c->add_face(v0, v2, v3);

        f673c->f164 = 1;
        f673c->f138 = 1;

        {
            float scale = 100000.0f;
            f6740->set_scale(&scale);
        }
        f6740->f140 = 1;
    }
}

// @ 0x52b300
void Screen_s2ae70::order_tick()
{
    unsigned id = g_mission_s2ae70->local_id;
    if ((int)id < 0)
        return;

    Mission_s2ae70* m = g_mission_s2ae70;
    Agent_s2ae70* a = m->pages[id >> 4] + (id & 0xf);
    int kind = a->kind;
    if (kind < 0 || kind >= 8)
        kind = 7;
    if (g_cfg.cheat_mode && g_keyboard.down[42] && (*(int*)((char*)&g_keyboard + 0x80)))
        kind = 1;

    m->order(kind, f2a0);

    if (f29c == 1) {
        f2b0 = g_ticks[f2a8] - 750000;
        f29c = 2;
        draw_order_menu();
    }
}

// @ 0x52b3c0
void Screen_s2ae70::draw_order_menu()
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

    unsigned base = (f29c == 1) ? 0xffffffff : 0xff50ff60;

    unsigned c;
    c = (f2a0 == 0) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_hold_position");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 1) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_follow");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 2) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_charge");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 3) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_mount");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }

    c = (f2a0 == 4) ? base : 0xffd0d0d0;
    {
        tstring s("ui_order_dismount");
        g_gui.draw_string(g_strtable.lookup(s), 0.03f, c, 1);
    }
}

// ======================================================= slice s0052b5c0: functions
#undef g_strtable_s2b5c0
#define g_strtable_s2b5c0 g_strtable
#undef g_items_s2b5c0
#define g_items_s2b5c0 (*(ItemKind_s2b5c0**)&g_items)
#undef g_game_s2b5c0
#define g_game_s2b5c0 (*(CampaignGame_s2b5c0**)&g_game)
#undef g_mission_s2b5c0
#define g_mission_s2b5c0 (*(Mission_s2b5c0**)&g_mission)
#undef g_editor_s2b5c0
#define g_editor_s2b5c0 (*(Editor_s2b5c0*)&g_app_obj)
static __forceinline int room_s2b5c0(ItemRef* r) { int n = r->m4 & 0xff; return r->max_amount() - n; }   // ItemRef::room_left (0x434c30), inlined here
// @ 0x52b5c0
bool Mission_s2b5c0::order_agent(unsigned agent_id, ItemRef_s2b5c0* order_)
{
    ItemRef* order = (ItemRef*)order_;
    Agent* agent = ((Mission*)this)->agents.pages[agent_id >> 4] + (agent_id & 0xf);
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
            sprintf(buf, g_strtable_s2b5c0.lookup(key)->c_str(), oldname, newname);
        }
        g_gui.add(tstring(buf), -1);

        unsigned k = g_items[slot->item].type_flags & 0xff;
        // 0x52b808..0x52b814: kind 7 with count <= 1 does not stop here, it falls into the generic
        // max_amount test below (0x52b816..0x52b84d)
        if ((k == 7 && (slot->m4 & 0xffff) > 1) ||
            k == 2 || k == 3 || k == 4 || k == 8 || k == 9 || k == 0x10 || k == 0x11 ||
            room_s2b5c0(slot) > 0) {
            matrix m = agent->body->m;
            m.o.z += 0.1f;   // 0x52b86f..0x52b875: [esp+0x64] is matrix +0x38 (o.z), not o.y
            int mode = 6;
            ((Mission*)this)->add_placed(&mode, &slot->item, (int*)&slot->m4, &m,
                       (int*)((char*)agent + 8));
        }
    } else {
        order->name(newname);
        {
            tstring key("ui_picking_up");
            sprintf(buf, g_strtable_s2b5c0.lookup(key)->c_str(), newname);
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

    ((Mission*)this)->refresh_agent(agent);   // 0x4f7e00: Mission::refresh_agent(Agent*), this = the saved this
    agent->on_wield_reset();
    return true;
    }

    {
        tstring key("ui_unable_to_take");
        g_gui.add(*g_strtable_s2b5c0.lookup(key), -1);
    }
    return false;
}

// =====================================================================================================
//  Mission_s2b5c0::update_ui
// =====================================================================================================
// @ 0x52ba10
void Mission_s2b5c0::update_ui()
{
    if (g_keyboard.down[35] != 0 && g_keyboard.prev[35] == 0)
        g_game_s2b5c0->f1a20c = (g_game_s2b5c0->f1a20c == 0);

    g_mission_s2b5c0->f65d0->f140 = 0;
    g_mission_s2b5c0->f6738->f140 = 0;

    int m = g_game_s2b5c0->game_mode;
    if (m == 0) {
    if (g_game_s2b5c0->f1a20c != 0)
        return;

    if (g_keyboard.down[57] == 0 || g_keyboard.prev[57] != 0)
        goto L_bb21;

    g_game_s2b5c0->f1a098 = 1 - g_game_s2b5c0->f1a098;
    {
        vector4 v;
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        g_game_s2b5c0->color = v;
    }
    g_game_s2b5c0->f1a0ac = 0;
    g_editor_s2b5c0.update_checks();
    if (g_game_s2b5c0->f1a098 == 0) {
        g_editor_s2b5c0.refresh();
    } else {
        goto L_bb4b;
    }

L_bb21:
    if (g_game_s2b5c0->f1a098 == 0 && g_game_s2b5c0->f1a0c8 == 0) {
        g_mission_s2b5c0->f65e4->f140 = 0;
        this->update_dialog();
        goto L_bbc5;
    }

L_bb4b:
    g_mission_s2b5c0->f65dc->f140 = 0;
    this->update_map();
    goto L_bbc5;
    } else if (m == 1 || m == 3 || m == 2) {
        g_mission_s2b5c0->f65dc->f140 = 0;
        g_mission_s2b5c0->f65e4->f140 = 0;
        this->update_cursor();
    } else if (m == 4) {
        g_mission_s2b5c0->f6738->f140 = 1;
        this->update_mode4();
    }

L_bbc5:
    if (g_game_s2b5c0->game_mode == 0 && g_game_s2b5c0->f1a098 == 0) {
        if ((float)(g_ticks[this->f2b8] - this->f2c0) * 1e-05f > 0.05f) {
            this->pick_entity();
            this->f2c0 = g_ticks[this->f2b8];
        }
    }
    if (this->f2cc == -1) {
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
// epilogue); the whole state machine is emitted once at 0x52ba10 in Mission_s2b5c0::update_ui.
void Mission_s2b5c0::update_ui_continue()
{
    if (g_keyboard.down[35] != 0 && g_keyboard.prev[35] == 0)
        g_game_s2b5c0->f1a20c = (g_game_s2b5c0->f1a20c == 0);

    g_mission_s2b5c0->f65d0->f140 = 0;
    g_mission_s2b5c0->f6738->f140 = 0;

    int m = g_game_s2b5c0->game_mode;
    if (m != 0) {
        if (m == 1 || m == 3 || m == 2) {
            g_mission_s2b5c0->f65dc->f140 = 0;
            this->update_cursor();
            g_mission_s2b5c0->f65e4->f140 = 0;
        } else if (m == 4) {
            g_mission_s2b5c0->f6738->f140 = 1;
            this->update_mode4();
        }
        goto L_bbc5;
    }

    if (g_game_s2b5c0->f1a20c != 0)
        return;

    if (g_keyboard.down[57] == 0 || g_keyboard.prev[57] != 0)
        goto L_bb21;

    g_game_s2b5c0->f1a098 = 1 - g_game_s2b5c0->f1a098;
    {
        vector4 v;
        v.x = 1.0f;
        v.y = 1.0f;
        v.z = 1.0f;
        g_game_s2b5c0->color = v;
    }
    g_game_s2b5c0->f1a0ac = 0;
    g_editor_s2b5c0.update_checks();
    if (g_game_s2b5c0->f1a098 == 0) {
        g_editor_s2b5c0.refresh();
        goto L_bb21;
    }

L_bb4b:
    g_mission_s2b5c0->f65dc->f140 = 0;
    this->update_map();
    goto L_bbc5;

L_bb21:
    if (g_game_s2b5c0->f1a098 == 0 && g_game_s2b5c0->f1a0c8 == 0) {
        this->update_dialog();
        g_mission_s2b5c0->f65e4->f140 = 0;
        goto L_bbc5;
    }
    goto L_bb4b;

L_bbc5:
    if (g_game_s2b5c0->game_mode == 0 && g_game_s2b5c0->f1a098 == 0) {
        long long now = g_ticks[this->f2b8];
        long long prev = this->f2c0;
        if ((float)(now - prev) * 1e-05f > 0.05f) {
            this->pick_entity();
            this->f2c0 = g_ticks[this->f2b8];
        }
    }
    if (this->f2cc == -1) {
        g_gui.set_overlay(0, 1);
        float v[2] = { 0.5f, 0.5f };
        g_gui.set_view(v);
    }
}

// =====================================================================================================
//  Mission_s2b5c0::update_cursor
// =====================================================================================================
// @ 0x52bc80
void Mission_s2b5c0::update_cursor()
{
    if (g_mission_s2b5c0->f0->hf == 0)
        return;

    vector4 pos;
    char ok;
    // the original stores this flag as a byte (setne / mov [esp+8],cl) but passes
    // the whole 4-byte stack slot to terrain.update (mov ecx,[esp+8]); a bool/int
    // union reproduces both the byte store and the dword load.
    union { bool b; int i; } f;

    if ((*(int*)((char*)&g_keyboard + 0x390)) == 0 && g_keyboard.down[226] == 0)
        goto L_bd6f;

    if ((*(float*)((char*)&g_keyboard + 0x810)) != 0.0f || (*(float*)((char*)&g_keyboard + 0x80c)) != 0.0f) {
        this->find_position(&ok, &pos);
        f.b = (g_keyboard.down[226] != 0);
        if (ok) {
            int mode = 0;
            if (g_game_s2b5c0->game_mode == 2)
                mode = 1;
            else if (g_game_s2b5c0->game_mode == 3)
                mode = 2;
            if (mode != 0)
                this->f2f0 = 1;
            g_terrain.update((int*)((char*)g_mission_s2b5c0 + 0x63d0), &pos,
                             g_mission_s2b5c0->f651c, mode, f.i);
            g_game_s2b5c0->f1a1cc = 1;
        }
    }

    if ((*(int*)((char*)&g_keyboard + 0x390)) != 0)
        goto L_bd78;

L_bd6f:
    if ((*(int*)((char*)&g_keyboard + 0x790)) != 0)
        goto L_bd8a;
L_bd78:
    if (g_keyboard.down[226] != 0)
        return;
    if (g_keyboard.prev[226] == 0)
        return;
L_bd8a:
    if (this->f2f0 != 0) {
        g_terrain.conv_build_scene();
        this->f2f0 = 0;
    }
}

// =====================================================================================================
//  Mission_s2b5c0::find_position
// =====================================================================================================
// @ 0x52bdb0
void Mission_s2b5c0::find_position(char* ok, vector4* pos)
{
    *ok = 0;

    vector4 near_pt, far_pt;
    ((Camera_s2b5c0*)this->view)->screen_to_world(&near_pt, &far_pt);

    capsule_s2b5c0 cap(0.0f, &near_pt, &far_pt);
    matrix m;
    cap.transform((const ::pos*)identity_pos(&m));

    float b[2] = { far_pt.x, far_pt.y };
    vector4 p;
    p.x = near_pt.x;
    p.y = near_pt.y;

    float best = 1000000.0f;
    shape_base_s2b5c0* shapes[8192];
    int n;
    {
        int count;
        g_mission_s2b5c0->f0->query_segment((const float*)&p, b, 0x100, (shape_base**)shapes, &count);
        n = count;
    }
    for (int i = 0; i < n; i++) {
        if (shapes[i]->shape_type() == 2) {
            shape_base_s2b5c0* s = shapes[i];
            if (shape_contact_count((shape_base*)s, &cap, 0, 0)) {
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
#undef g_game_s2bf40
#define g_game_s2bf40 (*(CampaignGame_s2bf40**)&g_game)
// @ 0x52bf40
void Terrain::update(int* objs, vector4* pos, int sel, int mode, int flag)
{
    clear_brushed();

    if (mode == 1)
        sel = objs[1];
    else if (mode == 2)
        sel = objs[2];

    float speed;
    if (g_game_s2bf40->game_mode == 1)
        speed = g_game_s2bf40->f1a0d4;
    else if (g_game_s2bf40->game_mode == 3)
        speed = g_game_s2bf40->f1a0dc;
    else
        speed = g_game_s2bf40->f1a0d8;

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
                if (sel >= objs[0])
                    return;
                brush(objs, &sel, cell, &d, &mode, &flag, &place);
            }
        }
    }

    if (mode == 0) {
        int rowbase = 0;
        for (int i = 0; i < objs[0] - 2; i++) {
            char found = 0;
            for (int j = 0; j <= nx; j++) {
                for (int k = 0; k <= ny; k++) {
                    if (i + 4 < 0xf && brushed[i + 4][j][k] != 0)
                        found = 1;
                }
            }
            if (found)
                this->load_layer(objs, i);
            rowbase += 5;
        }
    } else if (mode == 1) {
        this->load_layer(objs, objs[1]);
    } else if (mode == 2) {
        this->load_layer(objs, objs[2]);
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
                    if (g_use_shaders != 0 && this->layer[1].f10 != 0) {
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
static __forceinline float elem_get_s2c2b0(BrushElem_s2c2b0* E, const int* c)
{
    float r = 0.0f;
    if (E->v)
        r = E->v[c[0] * 0x1a4 + c[1]];
    return r;
}
static __forceinline float clamp0_s2c2b0(float v) { return 0.0f > v ? 0.0f : v; }
static __forceinline void elem_set_s2c2b0(BrushElem_s2c2b0* E, const int* c, float val)
{
    if (E->v == 0) {
        if (!(val > 0.0f))
            return;
        E->v = (float*)operator new[](0xac440);
        E->clear();
    }
    E->v[c[0] * 0x1a4 + c[1]] = val;
}
// @ 0x52c2b0
void Terrain::brush(int* elems, int* pidx, int* cell, float* amt, int* op, int* fA, char* fB)
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
            for (int i = 0; i < *elems - 2; i++) {
                elem_set_s2c2b0(&((BrushElem_s2c2b0*)((char*)elems + 0xc))[i], cell,
                                clamp0_s2c2b0(elem_get_s2c2b0(&((BrushElem_s2c2b0*)((char*)elems + 0xc))[i], cell) - amount));
            }
            stampbase = 4;
        } else if (*op == 1) {
            BrushElem_s2c2b0* E = &((BrushElem_s2c2b0*)((char*)elems + 0xc))[*pidx];
            elem_set_s2c2b0(E, cell, clamp0_s2c2b0(elem_get_s2c2b0(E, cell) * (1.0f - amount)));
        } else if (*op == 2) {
            BrushElem_s2c2b0* E = &((BrushElem_s2c2b0*)((char*)elems + 0xc))[*pidx];
            unsigned argb = 0xffffffff;
            if (E->v) argb = *(unsigned*)&E->v[cell[0] * 0x1a4 + cell[1]];
            rgl::vector4 col = rgl::unpack_color(&argb);
            float k = 1.0f - amount;
            col = col * k + rgl::vector4(amount, amount, amount);   // the whole-struct assignment carries the temporary's unset w, as the original
            argb = rgl::pack_color(&col);
            E->put(cell, cell + 1, (int*)&argb);
        }
    } else {
        // ---- one of the two flags is clear: normal brush -------------------------------------------
        BrushElem_s2c2b0* E = &((BrushElem_s2c2b0*)((char*)elems + 0xc))[*pidx];
        float v = elem_get_s2c2b0(E, cell);

        if (*op == 0) {
            float nv = v + amount;
            if (1.0f < nv) nv = 1.0f;

            int n = *elems - 2;
            bool any = false;
            for (int i = *pidx + 1; i < n; i++) {
                if (elem_get_s2c2b0(&((BrushElem_s2c2b0*)((char*)elems + 0xc))[i], cell) > 0.98f)
                    any = true;
            }

            if (any) {
                elem_set_s2c2b0(E, cell, 1.0f);
            } else {
                if (*fB == 0) goto done;
                elem_set_s2c2b0(E, cell, nv);
            }

            if (*fB != 0) {
                for (int i = *pidx + 1; i < *elems - 2; i++) {
                    if (((BrushElem_s2c2b0*)((char*)elems + 0xc))[i].v) {
                        float old = ((BrushElem_s2c2b0*)((char*)elems + 0xc))[i].v[cell[0] * 0x1a4 + cell[1]];
                        if (old > 0.0f) {
                            float dv = old - amount;
                            if (dv < 0.0f) dv = 0.0f;
                            else if (dv >= old) dv = old;
                            elem_set_s2c2b0(&((BrushElem_s2c2b0*)((char*)elems + 0xc))[i], cell, dv);
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
            if (E->v) argb = *(unsigned*)&E->v[cell[0] * 0x1a4 + cell[1]];
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
void Elem_s2ce00::clear()
{
    if (v != 0) {
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                v[x * 0x1a4 + y] = 0.0f;
    }
}

// @ 0x52ce70
void Elem_s2ce00::put(int* ix, int* iy, int* value)
{
    if (v == 0) {
        if (*value == -1)
            return;
        v = (float*)operator new[](0xac440);
        for (int x = 0; x < 0x1a4; x++)
            for (int y = 0; y < 0x1a4; y++)
                ((int*)v)[x * 0x1a4 + y] = -1;
    }
    ((int*)v)[*ix * 0x1a4 + *iy] = *value;
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
