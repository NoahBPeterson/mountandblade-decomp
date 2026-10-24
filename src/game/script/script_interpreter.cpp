//-----------------------------------------------------------------------------
// File: script_interpreter.cpp  (0x49fa10..0x4ae458, plus the validators 0x49cc60..0x49d4b0 it calls)
//
// Desc: the Module System operation interpreter: Operation::execute runs one operation of a script / trigger /
//       dialog block: it fetches the (up to 16) operand values, dispatches on the opcode (header_operations.py
//       numbering, `switch (op)` over 30..2385 with a 2356-entry jump table at 0x4ae458; 591 opcodes are
//       implemented, the rest report "Unrecognized opcode"), and applies the neg / this_or_next flags to the
//       condition result. Control opcodes below 30 (call_script, try_*, store_script_param_*) are run by the
//       block runner (0x4b09e0) and never reach this function.
//
//       The argument validators (0x49cc60..0x49d4b0) belong to the same object: the original passes their value
//       pointer in ECX (most), EAX (check_party, warn_tableau), EDI (warn_scene_prop_instance), EDX (warn_agent) or
//       ECX+ESI (check_string_arg), i.e. cl's register conventions for functions it can see. They are defined here,
//       ahead of their caller as cl emitted them (bottom-up); report and the four game-table checks are byte-exact
//       (__fastcall, external), the rest are statics (cl picks the registers). Bodies as in match/slices/s0049cc60.
//
//       Case bodies are in opcode order (the original emits them in that order, 581 of 591 monotone). See NOTES.md.
//       Flags: /O2 /MT /Gy /TP. Names provisional except the Module System ones.
//-----------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include "./script_interpreter.h"
// the shared headers it builds on (included again here so their address annotations are read with this file)
#include "../dialog_manager.h"
#include "../agent/agent.h"
#include "../tstring.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../../include/mat4.h"
#include "../../include/quaternion.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../light.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../slot_array.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_recs.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// ==== error reporting and argument validators (static, same object) =========================================

// @ 0x49cc60
// "%s; LINE NO: %d:" to the on-screen message log and the text log, then raise the error flag.
static __declspec(noinline) void report(const char* msg)
{
    char buf[1024];
    sprintf(buf, "%s; LINE NO: %d:", msg, g_lineno);
    g_msglog.add(tstring(buf), 0xffff5555);
    stream_base& lg = *(stream_base*)g_log_obj;
    lg.w_cstr(buf);
    g_script_error = 1;
}

// 0x49ccc0 check_script_param: used by the block runner only (s0049cc60).

// @ 0x49cd00
static __declspec(noinline) char check_register(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= 0x80) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Register ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cd50
static __declspec(noinline) char check_presentation(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_presentation_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Presentation ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49cda0
// String operand: its type selects the table: 3 = string (s_*), 0x16 = quick string, 0 = string register.
static __declspec(noinline) char check_string_arg(const int& type, const int& value)
{
    char buf[512];
    int t = type;
    char r = 1;
    if (t == 3) {
        int v = value;
        if (v < 0 || v >= g_strings.count) {
            sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid String ID: %d", g_cur_opcode, v);
            report(buf);
            r = 0;
        }
    } else if (t == 0x16) {
        int v = value;
        if (v < 0 || v >= g_strings.quick_count) {
            sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Quick String ID: %d", g_cur_opcode, v);
            report(buf);
            r = 0;
        }
    } else if (t == 0) {
        int v = value;
        if (v < 0 || v >= 0x80) {
            sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid String Register ID: %d", g_cur_opcode, v);
            report(buf);
            r = 0;
        }
    }
    return r;
}

// Range validators: the value pointer in ECX (__fastcall with external linkage keeps cl from picking another register)
// @ 0x49ce40
static __declspec(noinline) char check_mesh(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_mesh_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Mesh ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49ce90
static __declspec(noinline) char check_menu(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_menu_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Menu ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49cee0
__declspec(noinline) char __fastcall check_troop(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= game()->num_troops) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Troop ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49cf30
__declspec(noinline) char __fastcall check_quest(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= game()->num_quests) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Quest ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49cf80
__declspec(noinline) char __fastcall check_faction(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= game()->num_factions) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Faction ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49cfd0
static __declspec(noinline) char check_item_kind(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_item_kind_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Item Kind ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d020
__declspec(noinline) char __fastcall check_scene(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= game()->num_scenes) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Scene ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d070
static __declspec(noinline) char check_mission_template(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_mission_template_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Mission Template ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d0c0
static __declspec(noinline) char check_party_template(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_party_template_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Party Template ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d110
static __declspec(noinline) char check_map_icon(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_map_icon_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Map Icon ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d160
// Party id: must be a live entry of the party table (record +4 == 1). The original takes the pointer in EAX.
static __declspec(noinline) char check_party(const int& value)
{
    char buf[512];
    PartyMgr* t = &g_game->parties;
    unsigned id = value;
    if (id < (unsigned)t->deflt) {
        int idx = t->index_of(id);
        if (idx >= 0) {
            if (t->pages[idx >> 4][idx & 0xf].type == 1)
                return 1;
        }
    }
    sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Party ID: %d", g_cur_opcode, id);
    report(buf);
    return 0;
}

// @ 0x49d1f0
static __declspec(noinline) char warn_particle_system(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_particle_system_count) {
        sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid particle_system ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}
// @ 0x49d240
static __declspec(noinline) char warn_scene_prop(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_scene_prop_count) {
        sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Scene_prop ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// @ 0x49d290
// Scene prop instance: the mission must exist and the id must index its 0x1c8-byte prop records (pointer in EDI).
static __declspec(noinline) char warn_scene_prop_instance(const int& value)
{
    char buf[512];
    int id = value;
    if (g_mission && id >= 0) {
        if (mission()->props.first && id < mission()->props.size())
            return 1;
    }
    sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Scene_prop Instance ID: %d", g_cur_opcode, id);
    report(buf);
    return 0;
}

// @ 0x49d310
static __declspec(noinline) char warn_sound(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_sound_count) {
        if (g_warnings_off_b == 0) {
            sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Sound ID: %d", g_cur_opcode, v);
            report(buf);
        }
        r = 0;
    }
    return r;
}

// @ 0x49d360
// Tableau: the id must index the 0x68-byte tableau records (pointer in EAX).
static __declspec(noinline) char warn_tableau(const int& value)
{
    char buf[512];
    int id = value;
    if (id >= 0 && g_tableaus.first != 0) {
        if (id < g_tableaus.size())
            return 1;
    }
    sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Tableau ID: %d", g_cur_opcode, id);
    report(buf);
    return 0;
}

// @ 0x49d3d0
static __declspec(noinline) char warn_track(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_track_count) {
        if (g_warnings_off_a == 0 && g_warnings_off_b == 0) {
            sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Track ID: %d", g_cur_opcode, v);
            report(buf);
        }
        r = 0;
    }
    return r;
}

// @ 0x49d430
// Agent: the mission must exist and the id must be a live entry of its agent table (pointer in EDX).
static __declspec(noinline) char warn_agent(const int& value)
{
    char buf[512];
    char r = 0;
    if (g_mission) {
        int id = value;
        if (id >= 0 && id < mission()->agents.count) {
            if (mission()->agents.pages[id >> 4][id & 0xf].live != 0)
                return 1;
        }
    }
    sprintf(buf, "SCRIPT WARNING ON OPCODE %d: Invalid Agent ID: %d", g_cur_opcode, value);
    report(buf);
    return r;
}

// @ 0x49d4b0
static __declspec(noinline) char check_animation(const int& value)
{
    char buf[512];
    int v = value;
    char r = 1;
    if (v < 0 || v >= g_animation_count) {
        sprintf(buf, "SCRIPT ERROR ON OPCODE %d: Invalid Animation ID: %d", g_cur_opcode, v);
        report(buf);
        r = 0;
    }
    return r;
}

// ==== the interpreter ===========================================================================================

// 64-bit operand range test as the original spells it: lo <= v < hi on the full 64-bit value
#define IN_RANGE(v, lo, hi) ((v) >= (lo) && (v) < (hi))
#define SLOT_OK(v) IN_RANGE(v, 0, 0x100000)
// store a value into the lvalue operand through one named temporary (the original's single 64-bit slot at
// esp+0x18: a by-value temporary per call site would get a frame slot of its own)
#define SET_LV(e) do { lv_ = (e); set_lvalue(locals, lv_); } while (0)

// @ 0x49fa10
char Operation::execute(__int64* locals, int* context, int* error)
{
    char buf[0x400];
    __int64 lv_;
    g_cur_opcode = id;
    unsigned op_word = id;
    unsigned neg = op_word & op::neg;
    unsigned or_next = op_word & op::this_or_next;
    g_script_error = 0;
    char result = 1;
    if (g_or_latch) {
        if (!or_next)
            g_or_latch = 0;
        return 1;
    }
    int opcode = op_word & 0xfffffff;
    __int64 params[16];
    int types[16];
    params[0] = 0;
    params[1] = 0;
    params[2] = 0;
    int i;
    for (i = 0; i < n; i++)
        params[i] = get_operand(locals, &i, &types[i]);

    switch (opcode) {

    // ---- conditions: comparisons --------------------------------------------------------------------------
    case op::ge:
        result = params[0] >= params[1];
        break;
    case op::eq:
        result = params[0] == params[1];
        break;
    case op::gt:
        result = params[0] > params[1];
        break;
    case op::is_between:
        result = params[0] >= params[1] && params[0] < params[2];
        break;

    case op::entering_town: {
        // the main party is travelling to the town (ai_bhvr_attack_party with the town as object) and is within
        // 2.3 times its radius of it
        GameX* game = ::game();
        result = 0;
        if ((__int64)party(game->main_party)->ai_object == params[0] &&
            party(game->main_party)->ai_behavior == ai_bhvr_attack_party) {
            float dist = (party(party(game->main_party)->ai_object)->pos - party(game->main_party)->pos).out_length();
            if (party(game->main_party)->radius * 2.3f >= dist)
                result = 1;
        }
        break;
    }
    case op::map_free:
        result = (*context & 1) && !(*context & 0x30000);
        break;
    case op::encountered_party_is_attacker:
        result = game()->encountered_party_is_attacker != 0;
        break;
    case op::conversation_screen_is_active:
        result = g_screen_stack.back() == 0x10;
        break;
    case op::in_meta_mission:
        result = g_in_meta_mission;
        break;
    case op::set_player_troop:
        if (check_troop((int)params[0]))
            game()->player_troop = (int)params[0];
        break;
    case op::store_repeat_object:
        SET_LV((__int64)game()->repeat_object);
        break;
    case op::set_result_string:
        if (check_string_arg(types[0], (int)params[0]))
            g_result_string = g_script.expand(g_strings.get((int)params[0], types[0]));
        break;
    case op::key_is_down:
        if (IN_RANGE(params[0], 0, 0x100))
            result = g_keyboard.down[(int)params[0]] != 0;
        break;
    case op::key_clicked:
        if (IN_RANGE(params[0], 0, 0x100))
            result = g_keyboard.clicked((int)params[0]);
        break;
    case op::game_key_is_down:
        if (IN_RANGE(params[0], 0, 0x25))
            result = g_game_keys.is_down((int)params[0]);
        break;
    case op::game_key_clicked:
        if (IN_RANGE(params[0], 0, 0x25))
            result = g_game_keys.clicked((int)params[0]);
        break;
    case op::mouse_get_position:
        if (check_register((int)params[0])) {
            float zero = 0;
            vector4 v;
            script_pos((int)params[0]).o = v.set(g_mouse_x, g_mouse_y, zero);
        }
        break;
    case op::get_global_cloud_amount:
        SET_LV((__int64)(int)(game()->cloud_amount * 100.0));
        break;
    case op::set_global_cloud_amount: {
        game()->cloud_amount = (float)clamp_index((int)params[0], 0, 100) * 0.01f;
        break;
    }
    case op::get_global_haze_amount:
        SET_LV((__int64)(int)(game()->haze_amount * 100.0));
        break;
    case op::set_global_haze_amount: {
        game()->haze_amount = (float)clamp_index((int)params[0], 0, 100) * 0.01f;
        break;
    }

    // ---- conditions: parties joining -----------------------------------------------------------------------
    case op::hero_can_join:
        // a free stack slot and room under the party size limit ([party_id], default the main party)
        if (check_party((int)params[0])) {
            if (params[0] == 0) {
                int main = game()->main_party;
                if (party(main)->max_stacks() - party(main)->num_regular_stacks() < 1 ||
                    party(game()->main_party)->num_regular_members() >= party(main)->party_size_limit())
                    result = 0;
                else
                    result = 1;
            } else {
                result = party((int)params[0])->max_stacks() > party((int)params[0])->num_regular_stacks();
            }
        }
        break;
    case op::hero_can_join_as_prisoner:
        if (check_party((int)params[0])) {
            if (params[0] == 0) {
                int main = game()->main_party;
                if (party(main)->max_stacks() - party(main)->num_prisoner_stacks() < 1 ||
                    party(game()->main_party)->num_prisoner_members() >= party(main)->prisoner_limit())
                    result = 0;
                else
                    result = 1;
            } else {
                result = party((int)params[0])->max_stacks() > party((int)params[0])->num_prisoner_stacks();
            }
        }
        break;
    case op::party_can_join:
        if (g_encountered_party < 0 || !game()->party_can_join_party(g_encountered_party, game()->main_party, 0))
            result = 0;
        else
            result = 1;
        break;
    case op::party_can_join_as_prisoner:
        if (g_encountered_party < 0 || !game()->party_can_join_party(g_encountered_party, game()->main_party, 1))
            result = 0;
        else
            result = 1;
        break;
    case op::troops_can_join:
        if (params[0] > 0) {
            int main = game()->main_party;
            if (party(main)->max_stacks() - party(main)->num_regular_stacks() < 1 ||
                (__int64)party(main)->num_regular_members() + params[0] > (__int64)party(main)->party_size_limit())
                result = 0;
            else
                result = 1;
        }
        break;
    case op::troops_can_join_as_prisoner:
        if (params[0] > 0) {
            int main = game()->main_party;
            if (party(main)->max_stacks() - party(main)->num_prisoner_stacks() < 1 ||
                (__int64)party(main)->num_prisoner_members() + params[0] > (__int64)party(main)->prisoner_limit())
                result = 0;
            else
                result = 1;
        }
        break;
    case op::party_can_join_party:
        result = 0;
        if (check_party((int)params[0]) && check_party((int)params[1]))
            result = game()->party_can_join_party((int)params[0], (int)params[1], params[2] != 0);
        break;
    case op::main_party_has_troop: {
        int main = game()->main_party;
        result = 0;
        for (i = 0; i < party(main)->num_stacks; i++)
            if ((__int64)party(main)->stacks[i].troop == params[0])
                result = 1;
        break;
    }
    case op::party_is_in_town:
        if (check_party((int)params[0])) {
            int p = (int)params[0];
            if (party(p)->ai_behavior == ai_bhvr_in_town && (__int64)party(p)->ai_object == params[1])
                result = 1;
            else
                result = 0;
        }
        break;
    case op::party_is_in_any_town:
        if (check_party((int)params[0]))
            result = party((int)params[0])->ai_behavior == ai_bhvr_in_town;
        break;
    case op::party_is_active:
        if (game()->parties.is_party((int)params[0]) &&
            (party((int)params[0])->flags & 0x1080100) == 0)   // pf_disabled | 0x80000 | 0x1000000
            result = 1;
        else
            result = 0;
        break;

    // ---- conditions: items and quests ------------------------------------------------------------------------
    case op::player_has_item: {
        TroopX* t = troop(game()->player_troop);
        result = 0;
        for (int k = 0; k < t->num_inventory_slots(); k++)
            if ((__int64)t->items[k].item == params[0])
                result = 1;
        break;
    }
    case op::troop_has_item_equipped:
        result = 0;
        if (check_troop((int)params[0])) {
            ItemRef* e = troop((int)params[0])->equipment;
            for (int k = 0; k < 10; k++)
                if ((__int64)e[k].item == params[1])
                    result = 1;
        }
        break;
    case op::troop_is_mounted:
        result = 0;
        if (check_troop((int)params[0]))
            result = troop((int)params[0])->equipment[ek_horse].item >= 0;
        break;
    case op::troop_is_guarantee_ranged:
        result = 0;
        if (check_troop((int)params[0])) {
            if ((troop((int)params[0])->flags & tf_guarantee_ranged) == 0)
                result = 0;
            else
                result = 1;
        }
        break;
    case op::troop_is_guarantee_horse:
        result = 0;
        if (check_troop((int)params[0])) {
            if ((troop((int)params[0])->flags & tf_guarantee_horse) == 0)
                result = 0;
            else
                result = 1;
        }
        break;
    case op::check_quest_active:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 8) & 1;
        break;
    case op::check_quest_finished:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 9) & 1;
        break;
    case op::check_quest_succeeded:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 10) & 1;
        break;
    case op::check_quest_failed:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 11) & 1;
        break;
    case op::check_quest_concluded:
        if (check_quest((int)params[0]))
            result = (quest((int)params[0])->flags >> 12) & 1;
        break;

    // ---- slots ---------------------------------------------------------------------------------------------------
    case op::troop_set_slot:
        if (check_troop((int)params[0]) && SLOT_OK(params[1]))
            troop((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::party_set_slot:
        if (check_party((int)params[0]) && SLOT_OK(params[1]))
            party((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::faction_set_slot:
        if (check_faction((int)params[0]) && SLOT_OK(params[1]))
            faction((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::scene_set_slot:
        if (check_scene((int)params[0]) && SLOT_OK(params[1]))
            scene((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::party_template_set_slot:
        // the original bounds the slot number by the party template count here (not by 0x100000)
        if (check_party_template((int)params[0]) && params[1] >= 0 && params[1] < g_party_template_count)
            g_party_templates[(int)params[0]].slots.set((int)params[1], params[2]);
        break;
    case op::agent_set_slot:
        if (warn_agent((int)params[0]) && SLOT_OK(params[1]))
            agent((int)params[0])->slot_vars.set((int)params[1], params[2]);
        break;
    case op::quest_set_slot:
        if (check_quest((int)params[0]) && SLOT_OK(params[1]))
            quest((int)params[0])->slots.set((int)params[1], params[2]);
        break;
    case op::item_set_slot:
        if (check_item_kind((int)params[0]) && SLOT_OK(params[1]))
            item_kind((int)params[0])->slots.set((int)params[1], params[2]);
        break;

    case op::troop_get_slot:
        if (check_troop((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(troop((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::party_get_slot:
        if (check_party((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(party((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::faction_get_slot:
        if (check_faction((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(faction((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::scene_get_slot:
        if (check_scene((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(scene((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::party_template_get_slot:
        if (check_party_template((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(g_party_templates[(int)params[1]].slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::agent_get_slot:
        if (warn_agent((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(agent((int)params[1])->slot_vars.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::quest_get_slot:
        if (check_quest((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(quest((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;
    case op::item_get_slot:
        if (check_item_kind((int)params[1]) && SLOT_OK(params[2]))
            SET_LV(item_kind((int)params[1])->slots.get((int)params[2]));
        else
            SET_LV(0);
        break;

    case op::troop_slot_eq:
        result = 0;
        if (check_troop((int)params[0]) && SLOT_OK(params[1]))
            result = troop((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::party_slot_eq:
        result = 0;
        if (check_party((int)params[0]) && SLOT_OK(params[1]))
            result = party((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::faction_slot_eq:
        result = 0;
        if (check_faction((int)params[0]) && SLOT_OK(params[1]))
            result = faction((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::scene_slot_eq:
        result = 0;
        if (check_scene((int)params[0]) && SLOT_OK(params[1]))
            result = scene((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::party_template_slot_eq:
        result = 0;
        if (check_party_template((int)params[0]) && SLOT_OK(params[1]))
            result = g_party_templates[(int)params[0]].slots.get((int)params[1]) == params[2];
        break;
    case op::agent_slot_eq:
        result = 0;
        if (warn_agent((int)params[0]) && SLOT_OK(params[1]))
            result = agent((int)params[0])->slot_vars.get((int)params[1]) == params[2];
        break;
    case op::quest_slot_eq:
        result = 0;
        if (check_quest((int)params[0]) && SLOT_OK(params[1]))
            result = quest((int)params[0])->slots.get((int)params[1]) == params[2];
        break;
    case op::item_slot_eq:
        result = 0;
        if (check_item_kind((int)params[0]) && SLOT_OK(params[1]))
            result = item_kind((int)params[0])->slots.get((int)params[1]) == params[2];
        break;

    case op::troop_slot_ge:
        result = 0;
        if (check_troop((int)params[0]) && SLOT_OK(params[1]))
            result = troop((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::party_slot_ge:
        result = 0;
        if (check_party((int)params[0]) && SLOT_OK(params[1]))
            result = party((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::faction_slot_ge:
        result = 0;
        if (check_faction((int)params[0]) && SLOT_OK(params[1]))
            result = faction((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::scene_slot_ge:
        result = 0;
        if (check_scene((int)params[0]) && SLOT_OK(params[1]))
            result = scene((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::party_template_slot_ge:
        result = 0;
        if (check_party_template((int)params[0]) && SLOT_OK(params[1]))
            result = g_party_templates[(int)params[0]].slots.get((int)params[1]) >= params[2];
        break;
    case op::agent_slot_ge:
        result = 0;
        if (warn_agent((int)params[0]) && SLOT_OK(params[1]))
            result = agent((int)params[0])->slot_vars.get((int)params[1]) >= params[2];
        break;
    case op::quest_slot_ge:
        result = 0;
        if (check_quest((int)params[0]) && SLOT_OK(params[1]))
            result = quest((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;
    case op::item_slot_ge:
        result = 0;
        if (check_item_kind((int)params[0]) && SLOT_OK(params[1]))
            result = item_kind((int)params[0])->slots.get((int)params[1]) >= params[2];
        break;

    // ---- sound and music -------------------------------------------------------------------------------------------
    case op::play_sound:
        if (warn_sound((int)params[0])) {
            int ent = trigger_entity(*context);
            if (ent)
                g_sound.play_at((int)params[0], (int)params[1], (rgl::vector4*)(ent + 0x88), 1.0f, 0);
            else
                g_sound.play((int)params[0], (int)params[1]);
        }
        break;
    case op::play_track:
        if (warn_track((int)params[0]) && IN_RANGE(params[1], 0, 3)) {
            g_next_track = (int)params[0];
            g_next_track_options = (int)params[1];
        }
        break;
    case op::play_cue_track:
        if (warn_track((int)params[0]))
            g_cue_track = (int)params[0];
        break;
    case op::music_set_situation:
        g_music_situation = (int)params[0];
        break;
    case op::music_set_culture:
        g_music_culture = (int)params[0];
        break;
    case op::stop_all_sounds:
        g_sound.stop_all();
        break;

    // ---- positions -------------------------------------------------------------------------------------------------
    case op::copy_position:
        if (check_register((int)params[0]) && check_register((int)params[1]))
            script_pos((int)params[0]) = script_pos((int)params[1]);
        break;
    case op::init_position:
        if (check_register((int)params[0]))
            script_pos((int)params[0]) = identity_frame();
        break;
    case op::get_trigger_object_position:
        if (check_register((int)params[0]))
            script_pos((int)params[0]) = game()->trigger_object_pos;
        break;
    case op::get_distance_between_positions:
        if (check_register((int)params[1]) && check_register((int)params[2]))
            SET_LV((__int64)(int)(rgl::length(script_pos((int)params[1]).o - script_pos((int)params[2]).o) * 100.0f));
        else
            SET_LV(0);
        break;
    case op::position_is_behind_position:
        if (check_register((int)params[0]) && check_register((int)params[1]) &&
            rgl::dot(script_pos((int)params[1]).f, script_pos((int)params[0]).o - script_pos((int)params[1]).o) < 0.0f)
            result = 1;
        else
            result = 0;
        break;
    case op::position_transform_position_to_parent:
        if (check_register((int)params[0]) && check_register((int)params[1]) && check_register((int)params[2]))
            script_pos((int)params[0]) = script_pos((int)params[1]) * script_pos((int)params[2]);
        break;
    case op::position_transform_position_to_local:
        if (check_register((int)params[0]) && check_register((int)params[1]) && check_register((int)params[2]))
            script_pos((int)params[0]) = script_pos((int)params[1]).to_local(script_pos((int)params[2]));
        break;
    case op::position_copy_rotation:
        if (check_register((int)params[0]) && check_register((int)params[1]))
            *(Rot*)&script_pos((int)params[0]) = *(Rot*)&script_pos((int)params[1]);
        break;
    case op::position_copy_origin:
        if (check_register((int)params[0]) && check_register((int)params[1]))
            script_pos((int)params[0]).o = script_pos((int)params[1]).o;
        break;
    case op::position_move_x:
        // movement in cm; [value] 0 = along the local axis, else along the global one
        if (check_register((int)params[0])) {
            float d = (float)params[1] * 0.01f;
            if (params[2])
                script_pos((int)params[0]).o.x += d;
            else
                script_pos((int)params[0]).o += d * script_pos((int)params[0]).s;
        }
        break;
    case op::position_move_y:
        if (check_register((int)params[0])) {
            float d = (float)params[1] * 0.01f;
            if (params[2])
                script_pos((int)params[0]).o.y += d;
            else
                script_pos((int)params[0]).o += d * script_pos((int)params[0]).f;
        }
        break;
    case op::position_move_z:
        if (check_register((int)params[0])) {
            float d = (float)params[1] * 0.01f;
            if (params[2])
                script_pos((int)params[0]).o.z += d;
            else
                script_pos((int)params[0]).o += d * script_pos((int)params[0]).u;
        }
        break;
    case op::position_rotate_x:
        if (check_register((int)params[0])) {
            float a = (float)params[1] * 0.017453292f;
            script_pos((int)params[0]).rotate_x(&a);
        }
        break;
    case op::position_rotate_y:
        if (check_register((int)params[0])) {
            float a = (float)params[1] * 0.017453292f;
            script_pos((int)params[0]).rotate_y(&a);
        }
        break;
    case op::position_rotate_z:
        // [use_global_z_axis]: rotate every row about the global z axis instead of the local one
        if (check_register((int)params[0])) {
            float a = (float)params[1] * 0.017453292f;
            if (params[2]) {
                Pos& p = script_pos((int)params[0]);
                p.s.rotate_z(a);
                p.f.rotate_z(a);
                p.u.rotate_z(a);
            } else
                script_pos((int)params[0]).rotate_z(&a);
        }
        break;
    case op::position_get_x:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_fixed_point_multiplier * script_pos((int)params[1]).o.x));
        }
        break;
    case op::position_get_y:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_fixed_point_multiplier * script_pos((int)params[1]).o.y));
        }
        break;
    case op::position_get_z:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_fixed_point_multiplier * script_pos((int)params[1]).o.z));
        }
        break;
    case op::position_set_x:
        if (check_register((int)params[0]))
            script_pos((int)params[0]).o.x = (float)params[1] / g_fixed_point_multiplier;
        break;
    case op::position_set_y:
        if (check_register((int)params[0]))
            script_pos((int)params[0]).o.y = (float)params[1] / g_fixed_point_multiplier;
        break;
    case op::position_set_z:
        if (check_register((int)params[0]))
            script_pos((int)params[0]).o.z = (float)params[1] / g_fixed_point_multiplier;
        break;
    case op::position_get_scale_x:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_fixed_point_multiplier * script_pos((int)params[1]).s.x));
        }
        break;
    case op::position_get_scale_y:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_fixed_point_multiplier * script_pos((int)params[1]).f.y));
        }
        break;
    case op::position_get_scale_z:
        if (check_register((int)params[1])) {
            SET_LV(round_i64(g_fixed_point_multiplier * script_pos((int)params[1]).u.z));
        }
        break;
    case op::position_get_rotation_around_z:
        // degrees, 0..359
        if (check_register((int)params[1])) {
            Pos& p = script_pos((int)params[1]);
            SET_LV((round_i64((float)atan2((double)-p.f.x, (double)p.f.y) * 57.29578f) + 360) % 360);
        }
        break;
    case op::position_normalize_origin:
        // normalizes the origin; the destination gets its old length (fixed point)
        if (check_register((int)params[1])) {
            float len = script_pos((int)params[1]).o.normalize();
            SET_LV(round_i64(g_fixed_point_multiplier * len));
        }
        break;
    case op::position_set_z_to_ground_level:
        if (check_register((int)params[0])) {
            MissionX* m = mission();
            vec4& o = script_pos((int)params[0]).o;
            if (m->min_x <= o.x && o.x < m->max_x && m->min_y <= o.y && o.y < m->max_y)
                script_pos((int)params[0]).o.z = m->scene->ground_height(&o);
        }
        break;

    // ---- presentations and overlays ---------------------------------------------------------------------------------
    case op::start_presentation:
        if (check_presentation((int)params[0])) {
            game()->next_window = 0xf;
            game()->presentation = (int)params[0];
        }
        break;
    case op::start_background_presentation:
        // only from game menus (context 8)
        if (*context == 8 && check_presentation((int)params[0]))
            game()->background_presentation = (int)params[0];
        break;
    case op::presentation_set_duration:
        if (g_presentation)
            g_presentation->duration = (float)params[0] * 0.01f;
        break;
    case op::create_text_overlay:
        if (g_presentation && check_string_arg(types[1], (int)params[1]))
            SET_LV((__int64)g_presentation->create_text_overlay(
                g_script.expand(g_strings.get((int)params[1], types[1])), (int)params[2]));
        break;
    case op::create_mesh_overlay:
        if (g_presentation && check_mesh((int)params[1]))
            SET_LV((__int64)g_presentation->create_mesh_overlay((int)params[1]));
        break;
    case op::create_button_overlay:
        if (g_presentation && check_string_arg(types[1], (int)params[1]))
            SET_LV((__int64)g_presentation->create_button_overlay(
                g_script.expand(g_strings.get((int)params[1], types[1])), (int)params[2]));
        break;
    case op::create_image_button_overlay:
        if (g_presentation && check_mesh((int)params[1]) && check_mesh((int)params[2]))
            SET_LV((__int64)g_presentation->create_image_button_overlay((int)params[1], (int)params[2]));
        break;
    case op::create_slider_overlay:
        if (g_presentation)
            SET_LV((__int64)g_presentation->create_slider_overlay((int)params[1], (int)params[2]));
        break;
    case op::create_progress_overlay:
        if (g_presentation)
            SET_LV((__int64)g_presentation->create_progress_overlay((int)params[1], (int)params[2]));
        break;
    case op::create_combo_button_overlay:
        if (g_presentation)
            SET_LV((__int64)g_presentation->create_combo_button_overlay());
        break;
    case op::create_text_box_overlay:
        if (g_presentation)
            SET_LV((__int64)g_presentation->create_text_box_overlay());
        break;
    case op::create_check_box_overlay:
        if (g_presentation && check_mesh((int)params[1]) && check_mesh((int)params[2]))
            SET_LV((__int64)g_presentation->create_check_box_overlay((int)params[1], (int)params[2]));
        break;
    case op::overlay_set_text:
        if (g_presentation && check_string_arg(types[1], (int)params[1]))
            g_presentation->set_text((int)params[0], g_script.expand(g_strings.get((int)params[1], types[1])));
        break;
    case op::overlay_set_color:
        if (g_presentation)
            g_presentation->set_color((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_alpha:
        if (g_presentation)
            g_presentation->set_alpha((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_hilight_color:
        if (g_presentation)
            g_presentation->set_hilight_color((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_hilight_alpha:
        if (g_presentation)
            g_presentation->set_hilight_alpha((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_size:
        if (g_presentation && check_register((int)params[1]))
            g_presentation->set_size((int)params[0], &script_pos((int)params[1]).o);
        break;
    case op::overlay_set_position:
        if (g_presentation && check_register((int)params[1]))
            g_presentation->set_position((int)params[0], &script_pos((int)params[1]).o);
        break;
    case op::overlay_set_val:
        if (g_presentation)
            g_presentation->set_val((int)params[0], (int)params[1]);
        break;
    case op::overlay_set_boundaries:
        if (g_presentation)
            g_presentation->set_boundaries((int)params[0], (int)params[1], (int)params[2]);
        break;
    case op::overlay_set_area_size:
        // the position register is not validated here
        if (g_presentation)
            g_presentation->set_area_size((int)params[0], &script_pos((int)params[1]).o);
        break;
    case op::overlay_set_mesh_rotation:
        if (g_presentation)
            g_presentation->set_mesh_rotation((int)params[0], &script_pos((int)params[1]));
        break;
    case op::overlay_add_item:
        if (g_presentation && check_string_arg(types[1], (int)params[1]))
            g_presentation->add_item((int)params[0], g_script.expand(g_strings.get((int)params[1], types[1])));
        break;
    case op::overlay_animate_to_color:
        // duration in 1/1000 seconds
        if (g_presentation)
            g_presentation->animate_to_color((int)params[0], (float)params[1] * 0.001f, (int)params[2]);
        break;
    case op::overlay_animate_to_alpha:
        if (g_presentation)
            g_presentation->animate_to_alpha((int)params[0], (float)params[1] * 0.001f, (int)params[2]);
        break;
    case op::overlay_animate_to_highlight_color:
        if (g_presentation)
            g_presentation->animate_to_highlight_color((int)params[0], (float)params[1] * 0.001f, (int)params[2]);
        break;
    case op::overlay_animate_to_highlight_alpha:
        if (g_presentation)
            g_presentation->animate_to_highlight_alpha((int)params[0], (float)params[1] * 0.001f, (int)params[2]);
        break;
    case op::overlay_animate_to_size:
        if (g_presentation && check_register((int)params[2]))
            g_presentation->animate_to_size((int)params[0], (float)params[1] * 0.001f, &script_pos((int)params[2]).o);
        break;
    case op::overlay_animate_to_position:
        if (g_presentation && check_register((int)params[2]))
            g_presentation->animate_to_position((int)params[0], (float)params[1] * 0.001f, &script_pos((int)params[2]).o);
        break;
    case op::create_image_button_overlay_with_tableau_material:
        // mesh -1 = none (not validated)
        if (g_presentation && (params[1] == -1 || check_mesh((int)params[1])) && warn_tableau((int)params[2]))
            SET_LV((__int64)g_presentation->create_image_button_overlay_with_tableau(
                (int)params[1], (int)params[2], (int)params[3]));
        break;
    case op::create_mesh_overlay_with_tableau_material:
        if (g_presentation && (params[1] == -1 || check_mesh((int)params[1])) && warn_tableau((int)params[2]))
            SET_LV((__int64)g_presentation->create_mesh_overlay_with_tableau(
                (int)params[1], (int)params[2], (int)params[3]));
        break;
    case op::create_game_button_overlay:
        if (g_presentation && check_string_arg(types[1], (int)params[1]))
            SET_LV((__int64)g_presentation->create_game_button_overlay(
                g_script.expand(g_strings.get((int)params[1], types[1]))));
        break;
    case op::context_menu_add_item:
        if (check_string_arg(types[0], (int)params[0])) {
            game()->context_menu_texts[game()->num_context_menu_items] =
                g_script.expand(g_strings.get((int)params[0], types[0]));
            game()->context_menu_values[game()->num_context_menu_items] = (int)params[1];
            game()->num_context_menu_items++;
        }
        break;
    case op::get_average_game_difficulty:
        game()->update_average_difficulty();
        SET_LV((__int64)(int)game()->average_difficulty);
        break;

    // ---- missions, map, resting, experience ------------------------------------------------------------------------
    case op::all_enemies_defeated:
        result = mission()->all_enemies_defeated((int)params[0]);
        break;
    case op::race_completed_by_player:
        break;
    case op::num_active_teams_le:
        if ((__int64)mission()->num_active_teams() > params[0])
            result = 0;
        else
            result = 1;
        break;
    case op::main_hero_fallen:
        result = mission()->main_hero_fallen();
        break;
    case op::set_party_battle_mode:
        g_party_battle_mode = 1;
        break;
    case op::set_camera_follow_party:
        if (check_party((int)params[0])) {
            game()->camera_follow_party = (int)params[0];
            game()->camera_follow = 1;
        }
        break;
    case op::start_map_conversation:
        game()->map_conversation_troop = (int)params[0];
        game()->map_conversation_mode = 5;
        game()->start_map_conversation = 1;
        break;
    case op::rest_for_hours:
        // (rest_for_hours, <rest_period>, [time_speed], [remain_attackable])
        game()->rest_period = (float)params[0];
        game()->rest_time_speed = (int)params[1];
        game()->rest_interactive = 0;
        if (params[0] > 0) {
            if (params[2]) {
                game()->rest_attackable = 0;
                party(game()->main_party)->ai_behavior = ai_bhvr_hold;
            } else {
                game()->rest_attackable = 1;
                party(game()->main_party)->ai_behavior = ai_bhvr_in_town;
            }
        }
        break;
    case op::rest_for_hours_interactive:
        game()->rest_period = (float)params[0];
        game()->rest_time_speed = (int)params[1];
        game()->rest_interactive = 1;
        if (params[0] > 0) {
            if (params[2]) {
                game()->rest_attackable = 0;
                party(game()->main_party)->ai_behavior = ai_bhvr_hold;
            } else {
                game()->rest_attackable = 1;
                party(game()->main_party)->ai_behavior = ai_bhvr_in_town;
            }
        }
        break;
    case op::add_xp_to_troop:
        // (add_xp_to_troop, <value>, [troop_id])
        if (check_troop((int)params[1]))
            game()->add_xp_to_troop((int)params[1], (int)params[0]);
        break;
    case op::add_gold_as_xp:
        game()->add_gold_as_xp((int)params[0]);
        break;
    case op::add_xp_as_reward:
        game()->add_xp_as_reward((int)params[0]);
        break;
    case op::add_gold_to_party:
        // (add_gold_to_party, <value>, <party_id>): 1.011 only clears the party's carried-gold byte
        if (check_party((int)params[1]))
            party((int)params[1])->carry_gold = 0;
        break;
    case op::set_party_creation_random_limits:
        // both limits clamped to 0..101
        if (params[0] < 0)
            params[0] = 0;
        else if (params[0] > 101)
            params[0] = 101;
        if (params[1] < 0)
            params[1] = 0;
        else if (params[1] > 101)
            params[1] = 101;
        if (params[0] <= params[1]) {
            game()->party_creation_min_random = (int)params[0];
            game()->party_creation_max_random = (int)params[1];
        }
        break;

    // ---- spawning, messages, notes ---------------------------------------------------------------------------------
    case op::spawn_around_party:
        // (spawn_around_party, <party_id>, <party_template_id>): the new party's id goes to reg0
        if (check_party((int)params[0]) && check_party_template((int)params[1])) {
            PartyX* p = party((int)params[0]);
            g_script.reg[0] = game()->spawn_around(p->pos.x, p->pos.y, (int)params[1], g_spawn_radius);
        }
        break;
    case op::set_spawn_radius:
        g_spawn_radius = (float)params[0];
        break;
    case op::display_log_message:
        if (check_string_arg(types[0], (int)params[0])) {
            int color = -1;
            if (params[1])
                color = (int)params[1];
            tstring text = g_script.expand(g_strings.get((int)params[0], types[0]));
            g_msglog.add(text, color);
            char num[0x200];
            _itoa(color, num, 10);
            GameX* g = game();
            if (g->message_log.len > 2)
                g->message_log.push_back('|');
            game()->message_log.append(g->format_time((int)game()->clock.elapsed()));
            game()->message_log.push_back('|');
            game()->message_log.append(tstring(num));
            game()->message_log.push_back('|');
            game()->message_log.append(text);
            if (g_text_log_enabled) {
                text.push_back('\n');
                ((stream_base*)g_log_obj)->w_tstr(&text);
            }
        }
        break;
    case op::display_message:
        if (check_string_arg(types[0], (int)params[0])) {
            int color = -1;
            if (params[1])
                color = (int)params[1];
            tstring text = g_script.expand(g_strings.get((int)params[0], types[0]));
            g_msglog.add(text, color);
            if (g_text_log_enabled) {
                text.push_back('\n');
                ((stream_base*)g_log_obj)->w_tstr(&text);
            }
        }
        break;
    case op::set_show_messages:
        g_show_messages = params[0] != 0;
        break;
    case op::add_troop_note_tableau_mesh:
        if (check_troop((int)params[0]) && warn_tableau((int)params[1]))
            troop((int)params[0])->note_tableau = (int)params[1];
        break;
    case op::add_faction_note_tableau_mesh:
        if (check_faction((int)params[0]) && warn_tableau((int)params[1]))
            faction((int)params[0])->note_tableau = (int)params[1];
        break;
    case op::add_party_note_tableau_mesh:
        if (check_party((int)params[0]) && warn_tableau((int)params[1]))
            party((int)params[0])->note_tableau = (int)params[1];
        break;
    case op::add_quest_note_tableau_mesh:
        if (check_quest((int)params[0]) && warn_tableau((int)params[1]))
            quest((int)params[0])->note_tableau = (int)params[1];
        break;
    // (add_*_note_from_dialog, <id>, <note_slot_no>, <value>): in a conversation only (context 4); the note is
    // "<speaker name>: <current sentence>"; value 1 = time-stamp it (else -1 = hidden). The slot number has only an
    // upper bound. add_quest_note_from_dialog validates its quest id as a troop id.
    case op::add_quest_note_from_dialog:
        if (*context == 4 && check_troop((int)params[0]) && params[1] < 8) {
            quest((int)params[0])->notes[(int)params[1]] = *troop(g_dialogs.speaker())->display_name();
            quest((int)params[0])->notes[(int)params[1]].append(tstring(": "));
            quest((int)params[0])->notes[(int)params[1]].append(g_dialogs.current_text());
            quest((int)params[0])->note_times[(int)params[1]] = params[2] ? game()->note_time : -1;
        }
        break;
    case op::add_quest_note_from_sreg:
        // (add_quest_note_from_sreg, <quest_id>, <note_slot_no>, <string_id>, <value>)
        if (check_string_arg(types[2], (int)params[2]) && check_quest((int)params[0]) && params[1] < 8) {
            quest((int)params[0])->notes[(int)params[1]] = g_script.expand(g_strings.get((int)params[2], types[2]));
            quest((int)params[0])->note_times[(int)params[1]] = params[3] ? game()->note_time : -1;
        }
        break;
    case op::add_troop_note_from_dialog:
        if (*context == 4 && check_troop((int)params[0]) && params[1] < 8) {
            troop((int)params[0])->notes[(int)params[1]] = *troop(g_dialogs.speaker())->display_name();
            troop((int)params[0])->notes[(int)params[1]].append(tstring(": "));
            troop((int)params[0])->notes[(int)params[1]].append(g_dialogs.current_text());
            troop((int)params[0])->note_times[(int)params[1]] = params[2] ? game()->note_time : -1;
        }
        break;
    case op::add_faction_note_from_dialog:
        if (*context == 4 && check_faction((int)params[0]) && params[1] < 8) {
            faction((int)params[0])->notes[(int)params[1]] = *troop(g_dialogs.speaker())->display_name();
            faction((int)params[0])->notes[(int)params[1]].append(tstring(": "));
            faction((int)params[0])->notes[(int)params[1]].append(g_dialogs.current_text());
            faction((int)params[0])->note_times[(int)params[1]] = params[2] ? game()->note_time : -1;
        }
        break;
    case op::add_party_note_from_dialog:
        if (*context == 4 && check_party((int)params[0]) && params[1] < 8) {
            party((int)params[0])->notes[(int)params[1]] = *troop(g_dialogs.speaker())->display_name();
            party((int)params[0])->notes[(int)params[1]].append(tstring(": "));
            party((int)params[0])->notes[(int)params[1]].append(g_dialogs.current_text());
            party((int)params[0])->note_times[(int)params[1]] = params[2] ? game()->note_time : -1;
        }
        break;
    case op::add_troop_note_from_sreg:
        if (check_string_arg(types[2], (int)params[2]) && check_troop((int)params[0]) && params[1] < 8) {
            troop((int)params[0])->notes[(int)params[1]] = g_script.expand(g_strings.get((int)params[2], types[2]));
            troop((int)params[0])->note_times[(int)params[1]] = params[3] ? game()->note_time : -1;
        }
        break;
    case op::add_faction_note_from_sreg:
        if (check_string_arg(types[2], (int)params[2]) && check_faction((int)params[0]) && params[1] < 8) {
            faction((int)params[0])->notes[(int)params[1]] = g_script.expand(g_strings.get((int)params[2], types[2]));
            faction((int)params[0])->note_times[(int)params[1]] = params[3] ? game()->note_time : -1;
        }
        break;
    case op::add_party_note_from_sreg:
        if (check_string_arg(types[2], (int)params[2]) && check_party((int)params[0]) && params[1] < 8) {
            party((int)params[0])->notes[(int)params[1]] = g_script.expand(g_strings.get((int)params[2], types[2]));
            party((int)params[0])->note_times[(int)params[1]] = params[3] ? game()->note_time : -1;
        }
        break;
    case op::tutorial_box:
        // (tutorial_box / dialog_box, <text_string_id>, [title_string_id]); title defaults to string 1
        if (check_string_arg(types[0], (int)params[0]) && g_screen_stack.back() != 0) {
            int title = 1, title_type = 3;
            if (params[1] > 0 && check_string_arg(types[1], (int)params[1])) {
                title = (int)params[1];
                title_type = types[1];
            }
            g_windows.dialog_box->show(g_script.expand(g_strings.get(title, title_type)),
                                       g_script.expand(g_strings.get((int)params[0], types[0])));
            g_windows.open(0);
        }
        break;
    case op::question_box:
        // (question_box, <string_id>, [yes_string_id], [no_string_id]); defaults strings 2 / 3; answer in game +0x1a208
        if (check_string_arg(types[0], (int)params[0]) && g_screen_stack.back() != 0) {
            int yes = 2, yes_type = 3, no = 3, no_type = 3;
            if (params[1] > 0 && check_string_arg(types[1], (int)params[1])) {
                yes = (int)params[1];
                yes_type = types[1];
            }
            if (params[2] > 0 && check_string_arg(types[2], (int)params[2])) {
                no = (int)params[2];
                no_type = types[2];
            }
            g_windows.dialog_box->ask(g_script.expand(g_strings.get((int)params[0], types[0])),
                                      g_script.expand(g_strings.get(yes, yes_type)),
                                      g_script.expand(g_strings.get(no, no_type)), &game()->question_answer);
            g_windows.open(0);
        }
        break;
    case op::tutorial_message:
        // (tutorial_message, <string_id>, [color]); an invalid string (-1) clears the message
        if (check_string_arg(types[0], (int)params[0])) {
            int color = -1;
            if (params[1])
                color = (int)params[1];
            g_msglog.tutorial_message(g_script.expand(g_strings.get((int)params[0], types[0])), color);
        } else
            g_msglog.tutorial_message(tstring(""), (int)params[1]);
        break;

    // ---- prices, joining and leaving -----------------------------------------------------------------------------
    case op::reset_price_rates:
        game()->reset_price_rates();
        break;
    case op::set_price_rate_for_item:
        if (check_item_kind((int)params[0]))
            item_kind((int)params[0])->price_rate = (float)params[1] * 0.01f;
        break;
    case op::set_price_rate_for_item_type:
        for (int k = 0; k < g_item_kind_count; k++)
            if ((__int64)(item_kind(k)->type_flags & 0xff) == params[0])
                item_kind(k)->price_rate = (float)params[1] * 0.01f;
        break;
    case op::party_join:
        sprintf(buf, UI("ui_value_joined_your_party"), CS(party(g_encountered_party)->display_name()));
        g_msglog.push(tstring(buf), -1);
        game()->transfer_stacks(g_encountered_party, game()->main_party, 0);
        break;
    case op::party_join_as_prisoner:
        sprintf(buf, UI("ui_value_joined_party_as_prisoner"), CS(party(g_encountered_party)->display_name()));
        g_msglog.push(tstring(buf), -1);
        game()->transfer_stacks(g_encountered_party, game()->main_party, 1);
        break;
    case op::troop_join:
        if (check_troop((int)params[0])) {
            sprintf(buf, UI("ui_value_has_joined_party"), CS(troop((int)params[0])->display_name()));
            g_msglog.push(tstring(buf), -1);
            party(game()->main_party)->add_members((int)params[0], 1, 0, 0, 0);
        }
        break;
    case op::troop_join_as_prisoner:
        if (check_troop((int)params[0])) {
            sprintf(buf, UI("ui_value_has_been_taken_prisoner"), CS(troop((int)params[0])->display_name()));
            g_msglog.push(tstring(buf), -1);
            party(game()->main_party)->add_members((int)params[0], 1, 1, 0, 0);
        }
        break;
    case op::remove_member_from_party:
        // (remove_member_from_party, <troop_id>, [party_id]): prisoners first, then the companions
        if (check_troop((int)params[0]) && check_party((int)params[1])) {
            int p = (int)params[1];
            if (!params[1])
                p = game()->main_party;
            if (!game()->remove_troops(p, (int)params[0], 1, 1, 0))
                game()->remove_troops(p, (int)params[0], 1, 0, 0);
            if (!params[1]) {
                sprintf(buf, UI("ui_value_left_the_party"), CS(troop((int)params[0])->display_name()));
                g_msglog.push(tstring(buf), -1);
            }
        }
        break;
    case op::remove_regular_prisoners:
        // the party argument is ignored: the main party's non-hero prisoner stacks are released
        for (int k = party(game()->main_party)->num_stacks - 1; k >= 0; k--) {
            int main = game()->main_party;
            if ((party(main)->stacks[k].flags & 1) && !troop(party(main)->stacks[k].troop)->is_hero()) {
                sprintf(buf, UI("ui_number_values_left_the_party"), party(game()->main_party)->stacks[k].count,
                        CS(troop(party(game()->main_party)->stacks[k].troop)->display_name()));
                g_msglog.push(tstring(buf), -1);
                party(game()->main_party)->remove_stack(k, game()->stack_rows);
            }
        }
        break;
    case op::remove_troops_from_companions:
        if (check_troop((int)params[0]) && params[1] > 0) {
            game()->remove_troops(game()->main_party, (int)params[0], (int)params[1], 0, 0);
            if (troop((int)params[0])->is_hero())
                sprintf(buf, UI("ui_value_left_the_party"), CS(troop((int)params[0])->display_name()));
            else
                sprintf(buf, UI("ui_number_value_left_the_party"), params[1], CS(troop((int)params[0])->display_name()));
            g_msglog.push(tstring(buf), -1);
        }
        break;
    case op::remove_troops_from_prisoners:
        // formats the message like remove_troops_from_companions but never shows it
        if (check_troop((int)params[0]) && params[1] > 0) {
            game()->remove_troops(game()->main_party, (int)params[0], (int)params[1], 1, 0);
            if (troop((int)params[0])->is_hero())
                sprintf(buf, UI("ui_value_left_the_party"), CS(troop((int)params[0])->display_name()));
            else
                sprintf(buf, UI("ui_number_value_left_the_party"), params[1], CS(troop((int)params[0])->display_name()));
        }
        break;

    case op::heal_party:
        // the party argument is ignored: the main party's regular stacks are healed, and the player agent
        {
            GameX* g = game();
            int main = g->main_party;
            PartyMgr* pm = &g->parties;
            for (i = 0; i < ((PartyX*)pm->get(main))->num_stacks; i++) {
                if (!(((PartyX*)pm->get(main))->stacks[i].flags & 1)) {
                    TroopX* troops = g->troops;
                    int t = ((PartyX*)pm->get(main))->stacks[i].troop;
                    if (troops[t].is_hero()) {
                        float hp = (float)troops[((PartyX*)pm->get(main))->stacks[i].troop].max_hit_points();
                        troops[((PartyX*)pm->get(main))->stacks[i].troop].hit_points = hp;
                    } else
                        ((PartyX*)pm->get(main))->stacks[i].num_wounded = 0;
                    g = game();
                }
                main = g->main_party;
                pm = &g->parties;
            }
            if (g_mission && mission()->player_agent >= 0)
                agent(mission()->player_agent)->health = agent(mission()->player_agent)->max_health;
        }
        break;
    case op::disable_party:
        if (check_party((int)params[0]))
            party((int)params[0])->flags |= 0x100;          // pf_disabled
        break;
    case op::enable_party:
        if (check_party((int)params[0]))
            party((int)params[0])->flags &= ~(unsigned __int64)0x100;
        break;
    case op::remove_party:
        if (check_party((int)params[0]))
            game()->remove_party((int)params[0]);
        break;
    case op::add_companion_party:
        // (add_companion_party, <troop_id_hero>)
        if (check_troop((int)params[0])) {
            int np = game()->create_companion_party((int)params[0], party(game()->main_party)->pos);
            party(np)->faction = 0;
            party(np)->flags |= 0x100000;                   // pf_limit_members
            party(np)->f8e4 = 6;
            party(np)->morale = party(game()->main_party)->morale;
            party(np)->f80 = party(game()->main_party)->f80;
        }
        break;

    // ---- sites and visitors --------------------------------------------------------------------------------------
    case op::add_troop_to_site:
        // (add_troop_to_site, <troop_id>, <scene_id>, <entry_no>)
        if (check_troop((int)params[0]))
            troop((int)params[0])->set_site((int)params[1], (int)params[2]);
        break;
    case op::remove_troop_from_site:
        if (check_troop((int)params[0]))
            troop((int)params[0])->site = 0;
        break;
    case op::modify_visitors_at_site:
        // the old scene is validated, not the new one
        if (check_scene(g_visitor_scene))
            g_visitor_scene = (int)params[0];
        break;
    case op::reset_visitors:
        if (check_scene(g_visitor_scene))
            for (int k = 0; k < 64; k++) {
                scene(g_visitor_scene)->visitors[k].troop = -1;
                scene(g_visitor_scene)->visitors[k].dna = 0xffff2f33;
            }
        break;
    case op::set_visitor:
        // (set_visitor, <entry_no>, <troop_id>, [<dna>]); troop -1 clears the entry
        if (IN_RANGE(params[0], 0, 0x40) && (params[1] == -1 || check_troop((int)params[1]))) {
            scene(g_visitor_scene)->visitors[(int)params[0]].troop = (int)params[1];
            if (params[1] == -1) {
                scene(g_visitor_scene)->visitor_counts[(int)params[0]] = 0;
                break;
            }
            scene(g_visitor_scene)->visitor_counts[(int)params[0]] = 1;
            if (params[2])
                scene(g_visitor_scene)->visitors[(int)params[0]].dna = (int)params[2];
            if (g_mission)
                mission()->visitors_changed();
        }
        break;
    case op::set_visitors:
        // (set_visitors, <entry_no>, <troop_id>, <number_of_troops>)
        if (IN_RANGE(params[0], 0, 0x40) && check_troop((int)params[1]) && params[2] > 0) {
            scene(g_visitor_scene)->visitors[(int)params[0]].troop = (int)params[1];
            scene(g_visitor_scene)->visitors[(int)params[0]].dna = 0xffff2f33;
            scene(g_visitor_scene)->visitor_counts[(int)params[0]] = (int)params[2];
            if (g_mission)
                mission()->visitors_changed();
        }
        break;
    case op::add_visitors_to_current_scene:
        // (add_visitors_to_current_scene, <entry_no>, <troop_id>, <number_of_troops>): only during a mission
        if (g_mission && IN_RANGE(params[0], 0, 0x40) && check_troop((int)params[1]) && params[2] > 0) {
            scene(g_visitor_scene)->visitors[(int)params[0]].troop = (int)params[1];
            scene(g_visitor_scene)->visitors[(int)params[0]].dna = 0xffff2f33;
            scene(g_visitor_scene)->visitor_counts[(int)params[0]] = (int)params[2];
            mission()->visitors_changed();
            mission()->add_visitors((int)params[0], (int)params[2]);
        }
        break;

    // ---- factions and quests -----------------------------------------------------------------------------------
    case op::set_relation:
        // (set_relation, <faction_id>, <faction_id>, <value>): value -100..100, symmetric; the player is told when
        // one side is faction 0
        if (check_faction((int)params[0]) && check_faction((int)params[1])) {
            float v = (float)params[2] * 0.01f;
            int old_rel = round_float(faction((int)params[0])->relations[(int)params[1]] * 100.0f);
            float lo = -1.0f, hi = 1.0f;
            float c = clamp_float(&v, &lo, &hi);
            faction((int)params[0])->relations[(int)params[1]] = c;
            faction((int)params[1])->relations[(int)params[0]] = c;
            int new_rel = round_float(faction((int)params[0])->relations[(int)params[1]] * 100.0f);
            if (params[0] == 0 || params[1] == 0) {
                int f1 = (int)params[1], f0 = (int)params[0];
                if (old_rel < new_rel) {
                    sprintf(buf, UI("ui_your_relations_with_value_has_improved_from_number_to_number"),
                            CS(faction(imax_ref(&f0, &f1))->display_name()), old_rel, new_rel);
                    g_msglog.push(tstring(buf), 0xff6fff28);
                } else if (old_rel > new_rel) {
                    sprintf(buf, UI("ui_your_relations_with_value_has_deteriorated_from_number_to_number"),
                            CS(faction(imax_ref(&f0, &f1))->display_name()), old_rel, new_rel);
                    g_msglog.push(tstring(buf), 0xffff7028);
                }
                if (params[0] == 0)
                    game()->update_faction_parties((int)params[1]);
                else if (params[1] == 0)
                    game()->update_faction_parties((int)params[0]);
            }
        }
        break;
    case op::faction_set_name:
        if (check_faction((int)params[0]) && check_string_arg(types[1], (int)params[1]))
            faction((int)params[0])->set_name(g_script.expand(g_strings.get((int)params[1], types[1])));
        break;
    case op::faction_set_color:
        if (check_faction((int)params[0])) {
            FactionRec* f = faction((int)params[0]);
            f->color = (int)params[1];
            f->color = (int)params[1] | 0xff000000;
        }
        break;
    case op::start_quest:
        // (start_quest, <quest_id>, [giver_troop_id]); the giver defaults to the conversation troop
        if (check_quest((int)params[0])) {
            __int64 giver;
            if (params[1] > 0 && check_troop((int)params[1]))
                giver = params[1];
            else
                giver = g_conversation_troop;
            game()->start_quest((int)params[0], (int)giver);
        }
        break;
    case op::complete_quest:
        if (check_quest((int)params[0]))
            game()->complete_quest((int)params[0]);
        break;
    case op::succeed_quest:
        if (check_quest((int)params[0]))
            game()->succeed_quest((int)params[0]);
        break;
    case op::fail_quest:
        if (check_quest((int)params[0]))
            game()->fail_quest((int)params[0]);
        break;
    case op::cancel_quest:
        if (check_quest((int)params[0]))
            game()->cancel_quest((int)params[0]);
        break;
    case op::set_quest_progression:
        if (check_quest((int)params[0]))
            quest((int)params[0])->set_progression((int)params[1]);
        break;
    case op::conclude_quest:
        if (check_quest((int)params[0]))
            game()->conclude_quest((int)params[0]);
        break;
    case op::setup_quest_text:
        if (check_quest((int)params[0])) {
            quest((int)params[0])->title = g_script.expand(*quest((int)params[0])->raw_title());
            quest((int)params[0])->text = g_script.expand(*quest((int)params[0])->raw_text());
        }
        break;
    case op::setup_quest_giver:
        // (setup_quest_giver, <quest_id>, <string_id>)
        if (check_quest((int)params[0]) && check_string_arg(types[1], (int)params[1]))
            quest((int)params[0])->giver = g_script.expand(g_strings.get((int)params[1], types[1]));
        break;

    // ---- encounters and merchandise ------------------------------------------------------------------------------
    case op::start_encounter:
        if (check_party((int)params[0]))
            game()->start_encounter(game()->main_party, (int)params[0]);
        break;
    case op::leave_encounter:
        game()->encounter_flags |= 1;
        break;
    case op::encounter_attack:
        game()->encounter_flags2 |= 0x40;
        break;
    case op::select_enemy:
        if (params[0] == 0 || params[0] == 1)
            g_mission_ui.select_enemy(1 - (int)params[0]);
        break;
    case op::set_passage_menu:
        if (check_menu((int)params[0]))
            g_passage_menu = (int)params[0];
        break;
    case op::end_current_battle:
        g_battle_flags |= 1;
        break;
    case op::set_mercenary_source_party:
        if (check_party((int)params[0]))
            game()->mercenary_source_party = (int)params[0];
        break;
    case op::set_merchandise_modifier_quality: {
        // percentage, clamped to 0..1000 (as a factor 0..10)
        float v = (float)params[0] * 0.01f, lo = 0, hi = 1000.0f;
        game()->merchandise_modifier_quality = clamp_float(&v, &lo, &hi);
        break;
    }
    case op::reset_item_probabilities: {
        // (reset_item_probabilities, [value]): default 100; only the low word of the operand is tested
        int v = (int)params[0];
        if (!v)
            v = 100;
        game()->set_item_probabilities((float)v * 0.01f);
        break;
    }
    case op::set_item_probability_in_merchandise:
        if (IN_RANGE(params[0], 0, g_item_kind_count))
            item_kind((int)params[0])->merchandise_probability = (float)params[1] * 0.01f;
        break;

    // ---- troops ------------------------------------------------------------------------------------------------
    case op::troop_set_type:
        if (check_troop((int)params[0])) {
            troop((int)params[0])->set_type(clamp_index((int)params[1], 0, g_skin_count));
        }
        break;
    case op::troop_get_type:
        if (check_troop((int)params[1]))
            SET_LV((__int64)(int)(troop((int)params[1])->flags & 0xf));
        break;
    case op::troop_is_hero:
        if (check_troop((int)params[0]))
            result = troop((int)params[0])->is_hero();
        break;
    case op::troop_is_wounded:
        if (check_troop((int)params[0]))
            result = game()->troop_is_wounded((int)params[0]);
        break;
    case op::troop_set_auto_equip:
        // value 0 disables auto-equipping (tf_is_merchant)
        if (check_troop((int)params[0])) {
            if (params[1])
                troop((int)params[0])->flags &= ~(unsigned __int64)0x1000;
            else
                troop((int)params[0])->flags |= 0x1000;
        }
        break;
    case op::troop_ensure_inventory_space:
        if (check_troop((int)params[0]))
            game()->ensure_inventory_space((int)params[0], (int)params[1]);
        break;
    case op::troop_sort_inventory:
        if (check_troop((int)params[0]))
            troop((int)params[0])->sort_inventory();
        break;
    case op::troop_add_merchandise:
        // (troop_add_merchandise, <troop_id>, <item_type_id>, <value>)
        if (check_troop((int)params[0]))
            game()->add_merchandise((int)params[1], (int)params[2], (int)params[0]);
        break;
    case op::troop_get_xp:
        if (check_troop((int)params[1]))
            SET_LV((__int64)troop((int)params[1])->xp);
        break;
    case op::troop_raise_attribute:
        if (check_troop((int)params[0]) && params[2] && IN_RANGE(params[1], 0, 4))
            troop((int)params[0])->raise_attribute((int)params[1], (int)params[2]);
        break;
    case op::troop_raise_skill:
        if (check_troop((int)params[0]) && params[2] && IN_RANGE(params[1], 0, 0x2a))
            troop((int)params[0])->raise_skill((int)params[1], (int)params[2]);
        break;
    case op::troop_raise_proficiency:
        if (check_troop((int)params[0]) && params[2] > 0 && IN_RANGE(params[1], 0, 7))
            troop((int)params[0])->raise_proficiency((int)params[1], (float)params[2]);
        break;
    case op::troop_raise_proficiency_linear:
        if (check_troop((int)params[0]) && params[2] && IN_RANGE(params[1], 0, 7))
            troop((int)params[0])->raise_proficiency_linear((int)params[1], (int)params[2]);
        break;
    case op::troop_add_proficiency_points:
        // at most 99 points at a time
        if (check_troop((int)params[0]) && params[1] > 0) {
            int* points = &troop((int)params[0])->proficiency_points;
            *points += clamp_index((int)params[1], 0, 100);
        }
        break;
    case op::troop_add_gold:
        if (check_troop((int)params[0]) && params[1])
            game()->add_gold((int)params[1], (int)params[0]);
        break;
    case op::troop_remove_gold:
        if (params[1] > 0) {
            if (params[0] == 0)
                game()->remove_player_gold((int)params[1], 0);
            else if (check_troop((int)params[0]))
                troop((int)params[0])->remove_gold((int)params[1]);
        }
        break;
    case op::troop_add_item:
        // (troop_add_item, <troop_id>, <item_id>, [modifier])
        if (check_troop((int)params[0]) && IN_RANGE(params[1], 0, g_item_kind_count)) {
            if (params[0] == (__int64)game()->player_troop)
                game()->player_add_item((int)params[1], (int)params[2]);
            else {
                TroopX* t = troop((int)params[0]);
                int k = t->first_free_inventory_slot();
                if (k >= 0)
                    t->set_item((int)params[1], k, (int)params[2]);
            }
        }
        break;
    case op::troop_remove_item:
        if (check_troop((int)params[0])) {
            int n = game()->remove_items((int)params[0], (int)params[1], 1, 1);
            if (params[0] == 0 && n > 0) {
                sprintf(buf, UI("ui_lost_item_value"), CS(item_kind((int)params[1])->display_name()));
                g_msglog.push(tstring(buf), -1);
            }
        }
        break;
    case op::troop_clear_inventory:
        if (check_troop((int)params[0]))
            clear_inventory_items(troop((int)params[0])->items);
        break;
    case op::troop_equip_items:
        if (check_troop((int)params[0]))
            troop((int)params[0])->auto_equip(-1, 0, 0, 0);
        break;
    case op::troop_inventory_slot_set_item_amount:
        // (troop_inventory_slot_set_item_amount, <troop_id>, <inventory_slot_no>, <value>); 0 empties the slot.
        // The low byte of the packed word holds the missing amount (max - value).
        if (check_troop((int)params[0]) && params[1] >= 0 &&
            params[1] < (__int64)(troop((int)params[0])->num_inventory_slots() + 10)) {
            if (params[1] < 10) {
                if (params[2] > 0) {
                    int mx = troop((int)params[0])->equipment[(int)params[1]].max_amount();
                    ItemRef& r = troop((int)params[0])->equipment[(int)params[1]];
                    r.b.amount = 0;
                    r.m4 |= mx - (int)params[2];
                } else
                    troop((int)params[0])->equipment[(int)params[1]].item = -1;
            } else {
                if (params[2] > 0) {
                    int mx = troop((int)params[0])->items[(int)params[1] - 10].max_amount();
                    ItemRef& r = troop((int)params[0])->items[(int)params[1] - 10];
                    r.b.amount = 0;
                    r.m4 |= mx - (int)params[2];
                } else
                    troop((int)params[0])->items[(int)params[1] - 10].item = -1;
            }
        }
        break;
    case op::troop_add_items:
        // (troop_add_items, <troop_id>, <item_id>, <number>)
        if (check_troop((int)params[0]) && check_item_kind((int)params[1])) {
            game()->add_items((int)params[0], (int)params[1], (int)params[2]);
            if (params[0] == 0) {
                sprintf(buf, UI("ui_got_number_value"), (int)params[2], CS(item_kind((int)params[1])->display_name()));
                g_msglog.push(tstring(buf), -1);
            }
        }
        break;
    case op::troop_remove_items:
        // puts the value of the removed items into reg0
        if (check_troop((int)params[0]) && check_item_kind((int)params[1])) {
            g_script.reg[0] = game()->remove_items((int)params[0], (int)params[1], (int)params[2], 1);
            if (params[0] == 0) {
                sprintf(buf, UI("ui_lost_number_value"), (int)params[2], CS(item_kind((int)params[1])->display_name()));
                g_msglog.push(tstring(buf), -1);
            }
        }
        break;
    case op::troop_inventory_slot_get_item_amount:
        // (troop_inventory_slot_get_item_amount, <destination>, <troop_id>, <inventory_slot_no>)
        if (check_troop((int)params[1]) && params[2] >= 0 &&
            params[2] < (__int64)(troop((int)params[1])->num_inventory_slots() + 10)) {
            if (params[2] < 10)
                SET_LV((__int64)troop((int)params[1])->equipment[(int)params[2]].room_left());
            else
                SET_LV((__int64)troop((int)params[1])->items[(int)params[2] - 10].room_left());
        }
        break;
    case op::troop_inventory_slot_get_item_max_amount:
        if (check_troop((int)params[1]) && params[2] >= 0 &&
            params[2] < (__int64)(troop((int)params[1])->num_inventory_slots() + 10)) {
            if (params[2] < 10)
                SET_LV((__int64)troop((int)params[1])->equipment[(int)params[2]].max_amount());
            else
                SET_LV((__int64)troop((int)params[1])->items[(int)params[2] - 10].max_amount());
        }
        break;

    case op::troop_loot_troop:
        // (troop_loot_troop, <target_troop>, <source_troop_id>, <probability>): every itp_always_loot item, then ten
        // random draws weighted by the source's level against the item value
        if (params[2] > 0 && check_troop((int)params[0]) && check_troop((int)params[1])) {
            i = troop((int)params[1])->num_inventory_slots();
            TroopX* t = troop((int)params[0]);
            int dst = ((ItemList*)t->items)->first_free(t->num_inventory_slots());
            for (int k = 0; k < i; k++) {
                TroopX* troops = game()->troops;
                if (dst < troops[(int)params[0]].num_inventory_slots()) {
                    ItemRef* src = &troops[(int)params[1]].items[k];
                    if (src->item >= 0 && (item_kind(src->item)->type_flags & itp_always_loot)) {
                        troops[(int)params[0]].items[dst] = *src;
                        dst++;
                    }
                }
            }
            for (int n = 10; n; n--) {
                int k = rand_below(&i);
                TroopX* troops = game()->troops;
                if (dst < troops[(int)params[0]].num_inventory_slots() && troops[(int)params[1]].items[k].item >= 0) {
                    float one = 1.0f;
                    float x = ((float)troops[(int)params[1]].level + 10.0f) * 10.0f /
                              item_kind(troops[(int)params[1]].items[k].item)->value;
                    float chance = fmin_ref(&one, &x) * ((float)params[2] * 0.01f);
                    if (frand() < chance) {
                        ItemRef* src = &troop((int)params[1])->items[k];
                        if (!(item_kind(src->item)->type_flags & (itp_unique | itp_always_loot))) {
                            troop((int)params[0])->items[dst] = *src;
                            IR(troop((int)params[0])->items[dst]).randomize_modifier(0.1f);
                            dst++;
                        }
                    }
                }
            }
        }
        break;
    case op::troop_get_inventory_capacity:
        if (check_troop((int)params[1]))
            SET_LV((__int64)(troop((int)params[1])->num_inventory_slots() + 10));
        else
            SET_LV(0);
        break;
    case op::troop_get_inventory_slot:
        // (troop_get_inventory_slot, <destination>, <troop_id>, <inventory_slot_no>); -1 when invalid
        if (check_troop((int)params[1]) && params[2] >= 0 &&
            params[2] < (__int64)(troop((int)params[1])->num_inventory_slots() + 10)) {
            if (params[2] < 10)
                SET_LV((__int64)troop((int)params[1])->equipment[(int)params[2]].item);
            else
                SET_LV((__int64)troop((int)params[1])->items[(int)params[2] - 10].item);
        } else
            SET_LV(-1);
        break;
    case op::troop_get_inventory_slot_modifier:
        if (check_troop((int)params[1]) && params[2] >= 0 &&
            params[2] < (__int64)(troop((int)params[1])->num_inventory_slots() + 10)) {
            if (params[2] < 10)
                SET_LV((__int64)troop((int)params[1])->equipment[(int)params[2]].b.mod);
            else
                SET_LV((__int64)troop((int)params[1])->items[(int)params[2] - 10].b.mod);
        } else
            SET_LV(-1);
        break;
    case op::troop_set_inventory_slot:
        if (check_troop((int)params[0]) && params[1] >= 0 &&
            params[1] < (__int64)(troop((int)params[0])->num_inventory_slots() + 10)) {
            if (params[1] < 10)
                IR(troop((int)params[0])->equipment[(int)params[1]]).set_item((int)params[2]);
            else
                IR(troop((int)params[0])->items[(int)params[1] - 10]).set_item((int)params[2]);
        }
        break;
    case op::troop_set_inventory_slot_modifier:
        if (check_troop((int)params[0]) && params[1] >= 0 &&
            params[1] < (__int64)(troop((int)params[0])->num_inventory_slots() + 10)) {
            if (params[1] < 10)
                IR(troop((int)params[0])->equipment[(int)params[1]]).set_modifier((int)params[2]);
            else
                IR(troop((int)params[0])->items[(int)params[1] - 10]).set_modifier((int)params[2]);
        }
        break;
    case op::troop_set_faction:
        if (check_troop((int)params[0]) && check_faction((int)params[1]))
            troop((int)params[0])->faction = (int)params[1];
        break;
    case op::troop_set_health:
        // (troop_set_health, <troop_id>, <relative health (0-100)>, [absolute])
        if (check_troop((int)params[0])) {
            TroopX* t = troop((int)params[0]);
            if (params[2]) {
                t->hit_points = (float)clamp_index((int)params[1], 0, t->max_hit_points() + 1);
            } else {
                t->hit_points = (float)clamp_index((int)((__int64)t->max_hit_points() * params[1] / 100), 0,
                                                   t->max_hit_points() + 1);
            }
        }
        break;
    case op::troop_get_upgrade_troop:
        // (troop_get_upgrade_troop, <destination>, <troop_id>, <upgrade_path>): path 0 or 1
        if (check_troop((int)params[1]) && IN_RANGE(params[2], 0, 2)) {
            if (params[2] == 0)
                SET_LV((__int64)(unsigned)troop((int)params[1])->upgrade_troop[0]);
            else
                SET_LV((__int64)(unsigned)troop((int)params[1])->upgrade_troop[1]);
        }
        break;
    case op::item_get_type:
        if (check_item_kind((int)params[1]))
            SET_LV((__int64)(item_kind((int)params[1])->type_flags & 0xff));
        break;

    // ---- parties -----------------------------------------------------------------------------------------------
    case op::party_get_num_companions:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->num_regular_members());
        break;
    case op::party_get_num_prisoners:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->num_prisoner_members());
        break;
    case op::party_set_flags:
        // (party_set_flags, <party_id>, <flag>, <clear_or_set>)
        if (check_party((int)params[0])) {
            if (params[2])
                party((int)params[0])->flags |= params[1];
            else
                party((int)params[0])->flags &= ~params[1];
            party((int)params[0])->update_flags();
            party((int)params[0])->dirty = 1;
        }
        break;
    case op::party_set_extra_text:
        if (check_party((int)params[0]) && check_string_arg(types[1], (int)params[1]))
            party((int)params[0])->extra_text = g_script.expand(g_strings.get((int)params[1], types[1]));
        break;
    case op::party_get_current_terrain:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->current_terrain);
        break;
    case op::party_get_template_id:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->template_id);
        break;
    case op::party_add_members:
        // stores the number added to reg0
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = game()->add_members((int)params[0], (int)params[1], (int)params[2], 0);
        break;
    case op::party_add_prisoners:
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = game()->add_members((int)params[0], (int)params[1], (int)params[2], 1);
        break;
    case op::party_add_leader:
        // (party_add_leader, <party_id>, <troop_id>, [<number>]): inserted as the first stack (not into party 0)
        if (check_party((int)params[0]) && params[0] != 0 && check_troop((int)params[1])) {
            int max_index = 0x3f;
            int n = min_int(party((int)params[0])->num_stacks, max_index);
            party((int)params[0])->num_stacks = n + 1;
            int k;
            for (k = n; k > 0; k--)
                party((int)params[0])->stacks[k] = party((int)params[0])->stacks[k - 1];
            party((int)params[0])->stacks[k].clear();
            party((int)params[0])->stacks[k].troop = (int)params[1];
            int count = (int)params[2], one = 1;
            party((int)params[0])->stacks[k].count = imax_ref(&one, &count);
        }
        break;
    case op::party_force_add_members:
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            party((int)params[0])->force_add_members((int)params[1], (int)params[2], 0, 0);
        break;
    case op::party_force_add_prisoners:
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            party((int)params[0])->force_add_members((int)params[1], (int)params[2], 1, 0);
        break;
    case op::party_remove_members:
        // stores the number removed to reg0
        g_script.reg[0] = 0;
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = game()->remove_troops((int)params[0], (int)params[1], (int)params[2], 0, 0);
        break;
    case op::party_remove_prisoners:
        g_script.reg[0] = 0;
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = game()->remove_troops((int)params[0], (int)params[1], (int)params[2], 1, 0);
        break;
    case op::party_clear:
        // the main party keeps its first stack (the player)
        if (check_party((int)params[0])) {
            party((int)params[0])->num_stacks = 0;
            if (params[0] == (__int64)game()->main_party)
                party((int)params[0])->num_stacks = 1;
        }
        break;
    case op::party_wound_members:
        // (party_wound_members, <party_id>, <troop_id>, <number>): the first regular stack of the troop
        if (check_party((int)params[0])) {
            PartyMgr* pm = &game()->parties;
            int p = (int)params[0];
            int found = -1;
            for (i = 0; found == -1 && i < ((PartyX*)pm->get(p))->num_stacks; i++)
                if ((__int64)((PartyX*)pm->get(p))->stacks[i].troop == params[1] &&
                    !(((PartyX*)pm->get(p))->stacks[i].flags & 1))
                    found = i;
            if (found >= 0) {
                int w = ((PartyX*)pm->get(p))->stacks[found].num_wounded + (int)params[2];
                int v = min_int(w, ((PartyX*)pm->get(p))->stacks[found].count);
                if (v < 0)
                    v = 0;
                ((PartyX*)pm->get(p))->stacks[found].num_wounded = v;
            }
        }
        break;
    case op::party_remove_members_wounded_first:
        g_script.reg[0] = 0;
        if (check_party((int)params[0]) && check_troop((int)params[1]) && params[2] > 0)
            g_script.reg[0] = game()->remove_troops((int)params[0], (int)params[1], (int)params[2], 0, 1);
        break;
    case op::party_set_faction:
        if (check_party((int)params[0]) && check_faction((int)params[1])) {
            party((int)params[0])->faction = (int)params[1];
            party((int)params[0])->dirty = 1;
        }
        break;

    case op::party_relocate_near_party:
        // (party_relocate_near_party, <party_id>, <target_party_id>, <value_spawn_radius>)
        if (check_party((int)params[0]) && check_party((int)params[1])) {
            GameX* g = game();
            PartyX* target = party((int)params[1]);
            g->relocate_near(target->pos.x, target->pos.y, (int)params[0], (float)params[2]);
        }
        break;
    case op::party_get_position:
        if (check_register((int)params[0]) && check_party((int)params[1])) {
            *(Rot*)&script_pos((int)params[0]) = identity_rot();
            script_pos((int)params[0]).o = party((int)params[1])->pos;
        }
        break;
    case op::party_set_position:
        if (check_party((int)params[0]) && check_register((int)params[1])) {
            party((int)params[0])->pos = script_pos((int)params[1]).o;
            party((int)params[0])->position_changed();
        }
        break;
    case op::map_get_random_position_around_position:
        // (map_get_random_position_around_position, <dest_position_no>, <position_no>, <radius>)
        if (check_register((int)params[0]) && check_register((int)params[1]) && params[2] >= 0)
            game()->random_position_around(script_pos((int)params[1]).o.x, script_pos((int)params[1]).o.y,
                                           (float)params[2], &script_pos((int)params[0]).o);
        break;
    case op::map_get_land_position_around_position:
        result = 0;
        if (check_register((int)params[0]) && check_register((int)params[1]) && params[2] >= 0)
            result = game()->position_around(script_pos((int)params[1]).o.x, script_pos((int)params[1]).o.y,
                                             (float)params[2], &script_pos((int)params[0]).o, 0);
        break;
    case op::map_get_water_position_around_position:
        result = 0;
        if (check_register((int)params[0]) && check_register((int)params[1]) && params[2] >= 0)
            result = game()->position_around(script_pos((int)params[1]).o.x, script_pos((int)params[1]).o.y,
                                             (float)params[2], &script_pos((int)params[0]).o, 1);
        break;
    case op::party_count_members_of_type:
        // (party_count_members_of_type, <destination>, <party_id>, <troop_id>)
        if (check_party((int)params[1]) && check_troop((int)params[2])) {
            int t = (int)params[2], p = (int)params[1];
            int prisoners = game()->count_troops(t, p, 1);
            SET_LV((__int64)(game()->count_troops(t, p, 0) + prisoners));
        }
        break;
    case op::party_count_companions_of_type:
    case op::party_count_prisoners_of_type:
        if (check_party((int)params[1]) && check_troop((int)params[2]))
            SET_LV((__int64)game()->count_troops((int)params[2], (int)params[1],
                                                              opcode == op::party_count_prisoners_of_type));
        break;
    case op::party_get_free_companions_capacity:
        if (check_party((int)params[1])) {
            if (party((int)params[1])->max_stacks() - party((int)params[1])->num_regular_stacks() < 1)
                SET_LV(0);
            else {
                int members = party((int)params[1])->num_regular_members();
                SET_LV((__int64)(party((int)params[1])->party_size_limit() - members));
            }
        }
        break;
    case op::party_get_free_prisoners_capacity:
        if (check_party((int)params[1])) {
            if (party((int)params[1])->max_stacks() - party((int)params[1])->num_prisoner_stacks() < 1)
                SET_LV(0);
            else {
                int members = party((int)params[1])->num_prisoner_members();
                SET_LV((__int64)(party((int)params[1])->prisoner_limit() - members));
            }
        }
        break;
    case op::party_set_ai_initiative:
        // (party_set_ai_initiative, <party_id>, <value>): the original clamps the party id (not the value) to
        // 0..100 and stores it as the initiative
        if (check_party((int)params[0])) {
            float initiative = (float)clamp_index((int)params[0], 0, 101) * 0.01f;
            party((int)params[0])->ai_initiative = initiative;
        }
        break;
    case op::party_set_ai_behavior:
        if (check_party((int)params[0])) {
            PartyMgr* pm = &game()->parties;
            int p = (int)params[0];
            if (((PartyX*)pm->get(p))->ai_behavior == ai_bhvr_in_town ||
                ((PartyX*)pm->get(p))->ai_behavior == ai_bhvr_negotiate_party)
                ((PartyX*)pm->get(p))->ai_behavior = ai_bhvr_hold;
            party((int)params[0])->f8e4 = (int)params[1];
            party((int)params[0])->ai_state = 0;
        }
        break;
    case op::party_set_ai_object:
        if (check_party((int)params[0]) && check_party((int)params[1]))
            party((int)params[0])->ai_target_object = (int)params[1];
        break;
    case op::party_set_ai_target_position:
        if (check_party((int)params[0]) && check_register((int)params[1])) {
            PartyX* p = party((int)params[0]);
            p->ai_target_x = script_pos((int)params[1]).o.x;
            p->ai_target_y = script_pos((int)params[1]).o.y;
        }
        break;
    case op::party_set_ai_patrol_radius:
        if (check_party((int)params[0])) {
            float r = (float)params[1];
            party((int)params[0])->ai_patrol_radius = r;
        }
        break;
    case op::party_ignore_player:
        // (party_ignore_player, <party_id>, <duration_in_hours>)
        if (check_party((int)params[0])) {
            int h = (int)params[1];
            if (h < 0)
                h = 0;
            party((int)params[0])->ignore_player_timer.set_delay((float)-h);
        }
        break;
    case op::party_set_bandit_attraction:
        if (check_party((int)params[0])) {
            float a = (float)params[1] * 0.01f;
            party((int)params[0])->bandit_attraction = a;
        }
        break;
    case op::party_get_num_companion_stacks:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->num_regular_stacks());
        break;
    case op::party_get_num_prisoner_stacks:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->num_prisoner_stacks());
        break;
    case op::party_stack_get_troop_id:
        if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x40))
            SET_LV((__int64)party((int)params[1])->stacks[(int)params[2]].troop);
        break;
    case op::party_stack_get_size:
        if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x40))
            SET_LV((__int64)party((int)params[1])->stacks[(int)params[2]].count);
        break;
    case op::party_stack_get_num_wounded:
        if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x40))
            SET_LV((__int64)party((int)params[1])->stacks[(int)params[2]].num_wounded);
        break;
    case op::party_stack_get_troop_dna:
        // the main party's stacks keep their dna in the game's stack rows
        if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x40)) {
            if (params[1] != (__int64)game()->main_party)
                SET_LV((__int64)game()->stack_dna((int)params[1], (int)params[2], 0, 0));
            else
                SET_LV((__int64)*(int*)(game()->stack_rows + (int)params[2] * 0x80));
        }
        break;
    case op::party_prisoner_stack_get_troop_id:
        if (check_party((int)params[1])) {
            PartyMgr* pm = &game()->parties;
            int k = ((PartyX*)pm->get((int)params[1]))->num_regular_stacks() + (int)params[2];
            if (k >= 0 && k < 0x40)
                SET_LV((__int64)((PartyX*)pm->get((int)params[1]))->stacks[k].troop);
        }
        break;
    case op::party_prisoner_stack_get_size:
        if (check_party((int)params[1])) {
            int k = party((int)params[1])->num_regular_stacks() + (int)params[2];
            if (k >= 0 && k < 0x40)
                SET_LV((__int64)party((int)params[1])->stacks[k].count);
        }
        break;
    case op::party_prisoner_stack_get_troop_dna:
        if (check_party((int)params[1])) {
            GameX* g = game();
            int k = party((int)params[1])->num_regular_stacks() + (int)params[2];
            if (k >= 0 && k < 0x40) {
                if (params[1] != (__int64)g->main_party)
                    SET_LV((__int64)g->stack_dna((int)params[1], k, 0, 0));
                else
                    SET_LV((__int64)*(int*)(g->stack_rows + k * 0x80));
            }
        }
        break;

    case op::party_attach_to_party:
        if (check_party((int)params[0]) && check_party((int)params[1])) {
            game()->attach_party((int)params[0], (int)params[1]);
            party((int)params[0])->f8cc = 0;
        }
        break;
    case op::party_detach:
        if (check_party((int)params[0]))
            game()->detach_party((int)params[0]);
        break;
    case op::party_collect_attachments_to_party:
        if (check_party((int)params[0]) && check_party((int)params[1])) {
            GameX* g = game();
            g->collect_attachments((int)params[0], (PartyX*)g->parties.get((int)params[1]));
        }
        break;
    case op::party_quick_attach_to_current_battle:
        // (party_quick_attach_to_current_battle, <party_id>, <side (0:players side, 1:enemy side)>)
        if (check_party((int)params[0]))
            game()->quick_attach_to_battle((int)params[0], (int)params[1]);
        break;
    case op::party_get_cur_town:
        // the town the party is in (ai_bhvr_in_town), else -1
        if (check_party((int)params[1])) {
            PartyMgr* pm = &game()->parties;
            int p = (int)params[1];
            if (((PartyX*)pm->get(p))->ai_behavior == ai_bhvr_in_town)
                SET_LV((__int64)((PartyX*)pm->get(p))->ai_object);
            else
                SET_LV(-1);
        }
        break;
    case op::party_leave_cur_battle:
        if (check_party((int)params[0]))
            party((int)params[0])->leave_battle();
        break;
    case op::party_set_next_battle_simulation_time:
        if (check_party((int)params[0])) {
            GameX* g = game();
            int p = (int)params[0];
            if (((PartyX*)g->parties.get(p))->battle >= 0) {
                float t = (float)params[1];
                g->battles.get(((PartyX*)g->parties.get(p))->battle)->next_simulation_time = t;
            }
        }
        break;
    case op::party_set_name:
        if (check_party((int)params[0]) && check_string_arg(types[1], (int)params[1]))
            party((int)params[0])->set_name(g_script.expand(g_strings.get((int)params[1], types[1])));
        break;
    case op::party_add_xp_to_stack:
        if (check_party((int)params[0]) && params[1] >= 0) {
            PartyMgr* pm = &game()->parties;
            int p = (int)params[0];
            if (params[1] < (__int64)((PartyX*)pm->get(p))->num_stacks && params[2] > 0)
                ((PartyX*)pm->get(p))->add_xp_to_stack((int)params[1], (int)params[2]);
        }
        break;
    case op::party_get_morale:
        if (check_party((int)params[1])) {
            SET_LV((__int64)round_float(party((int)params[1])->morale * 100.0f));
        }
        break;
    case op::party_set_morale:
        // (party_set_morale, <party_id>, <value>), #value is clamped to range [0...100].
        if (check_party((int)params[0])) {
            float m = (float)clamp_index((int)params[1], 0, 101) * 0.01f;
            party((int)params[0])->morale = m;
        }
        break;
    case op::party_upgrade_with_xp:
        // (party_upgrade_with_xp, <party_id>, <xp_amount>, <upgrade_path>), upgrade_path 0 random, 1 first, 2 second
        if (check_party((int)params[0]) && params[1] > 0 && IN_RANGE(params[2], 0, 3))
            game()->upgrade_with_xp((int)params[0], (float)params[1], (int)params[2]);
        break;
    case op::party_add_xp:
        if (check_party((int)params[0]) && params[1] > 0)
            game()->party_add_xp((int)params[0], (float)params[1]);
        break;
    case op::party_add_template:
        // (party_add_template, <party_id>, <party_template_id>, [reverse_prisoner_status])
        if (check_party((int)params[0]) && check_party_template((int)params[1]))
            game()->add_template((int)params[0], (int)params[1], (int)params[2]);
        break;
    case op::party_set_icon:
        if (check_party((int)params[0]) && check_map_icon((int)params[1])) {
            party((int)params[0])->set_icon((int)params[1]);
            party((int)params[0])->update_flags();
        }
        break;
    case op::party_set_banner_icon:
        if (check_party((int)params[0]) && check_map_icon((int)params[1])) {
            party((int)params[0])->banner_icon = (int)params[1];
            party((int)params[0])->update_flags();
        }
        break;
    case op::party_add_particle_system:
        if (check_party((int)params[0]) && warn_particle_system((int)params[1]))
            party((int)params[0])->add_particle_system((int)params[1]);
        break;
    case op::party_clear_particle_systems:
        if (check_party((int)params[0]))
            party((int)params[0])->clear_particle_systems();
        break;
    case op::party_get_battle_opponent:
        // -1, then the root party of the other side of the battle the party's root is in
        if (check_party((int)params[1])) {
            GameX* g = game();
            int root = g->battle_root((int)params[1]);
            int b = ((PartyX*)g->parties.get(root))->battle;
            SET_LV(-1);
            if (b >= 0) {
                BattleTable* bt = &game()->battles;
                if (bt->get(b)->party_b == root)
                    SET_LV((__int64)bt->get(b)->party_a);
                else
                    SET_LV((__int64)bt->get(b)->party_b);
            }
        }
        break;
    case op::party_get_icon:
        if (check_party((int)params[1]))
            SET_LV((__int64)(unsigned)((unsigned)party((int)params[1])->flags & 0xff));
        break;
    case op::party_get_skill_level:
        {
            int v = 0;
            if (check_party((int)params[1]) && IN_RANGE(params[2], 0, 0x2a))
                v = party((int)params[1])->skill_level((int)params[2]);
            SET_LV((__int64)v);
        }
        break;

    case op::get_battle_advantage:
        SET_LV((__int64)g_battle_advantage);
        break;
    case op::set_battle_advantage:
        g_battle_advantage = clamp_index((int)params[0], -32, 6);
        break;
    case op::party_get_attached_to:
        {
            int v = -1;
            if (check_party((int)params[1]))
                v = party((int)params[1])->attached_to;
            SET_LV((__int64)v);
        }
        break;
    case op::party_get_num_attached_parties:
        {
            int n = 0;
            if (check_party((int)params[1]))
                n = party((int)params[1])->attached.size();
            SET_LV((__int64)n);
        }
        break;
    case op::party_get_attached_party_with_rank:
        // no lower bound on the rank (the original reads before the vector for a negative one)
        {
            int v = -1;
            if (check_party((int)params[1])) {
                PartyMgr* pm = &game()->parties;
                int p = (int)params[1];
                unsigned n = ((PartyX*)pm->get(p))->attached.size();
                if (params[2] < (__int64)n)
                    v = ((PartyX*)pm->get(p))->attached.first[(int)params[2]];
            }
            SET_LV((__int64)v);
        }
        break;
    case op::inflict_casualties_to_party_group:
        // (inflict_casualties_to_party_group, <parent_party_id>, <attack_rounds>, <party_id_to_add_causalties_to>)
        if (check_party((int)params[0])) {
            if (params[2] != (__int64)game()->main_party && check_party((int)params[2])) {
                GameX* g = game();
                g->inflict_casualties((int)params[0], (int)params[1], (PartyX*)g->parties.get((int)params[2]));
            } else
                game()->inflict_casualties((int)params[0], (int)params[1], 0);
        }
        break;
    case op::distribute_party_among_party_group:
        if (check_party((int)params[0]) && check_party((int)params[1])) {
            GameX* g = game();
            g->distribute_party((PartyX*)g->parties.get((int)params[0]), (int)params[1]);
        }
        break;

    // ---- agents -------------------------------------------------------------------------------------------------
    case op::get_player_agent_no:
        if (g_mission)
            SET_LV((__int64)mission()->player_agent);
        break;
    case op::get_player_agent_kill_count:
        // (get_player_agent_kill_count, <destination>, [get_wounded])
        if (params[1] == 0)
            SET_LV((__int64)game()->player_kill_count);
        else
            SET_LV((__int64)game()->player_wounded_count);
        break;
    case op::get_player_agent_own_troop_kill_count:
        if (params[1] == 0)
            SET_LV((__int64)game()->player_own_troop_kill_count);
        else
            SET_LV((__int64)game()->player_own_troop_wounded_count);
        break;
    case op::agent_is_alive:
        if (warn_agent((int)params[0]))
            result = agent((int)params[0])->state == 1;
        break;
    case op::agent_is_wounded:
        if (warn_agent((int)params[0]))
            result = agent((int)params[0])->state == 4;
        break;
    case op::agent_is_human:
        if (warn_agent((int)params[0]))
            result = agent((int)params[0])->kind == 0;
        break;
    case op::agent_is_ally:
        if (warn_agent((int)params[0])) {
            int pa = mission()->player_agent;
            if (pa >= 0) {
                AgentTable* t = &mission()->agents;
                result = t->get(pa)->is_friend(t->get((int)params[0]));
            }
        }
        break;
    case op::agent_is_defender:
        // in a siege (mode 2) the defenders are the player's side unless the player attacks (side 1)
        if (warn_agent((int)params[0])) {
            MissionX* m = mission();
            int pa = m->player_agent;
            if (pa >= 0) {
                if (m->mode != 0 && (m->mode != 2 || m->mode_side != 1)) {
                    AgentTable* t = &m->agents;
                    result = t->get(pa)->is_enemy(t->get((int)params[0]));
                } else {
                    AgentTable* t = &m->agents;
                    result = t->get(pa)->is_friend(t->get((int)params[0]));
                }
            }
        }
        break;
    case op::agent_get_look_position:
        if (check_register((int)params[0]) && warn_agent((int)params[1])) {
            *(Rot*)&script_pos((int)params[0]) = identity_rot();
            Pos* dst = &script_pos((int)params[0]);
            AgentTable* t = &mission()->agents;
            int a = (int)params[1];
            dst->o = t->get(a)->pos;
            if (t->get(a)->body) {
                dst->f = *(rgl::vector4*)&t->get(a)->fx;      // the facing direction (+0x55d8)
                dst->orthonormalize();
            }
        }
        break;
    case op::agent_get_position:
        if (check_register((int)params[0]) && warn_agent((int)params[1])) {
            AgentTable* t = &mission()->agents;
            int a = (int)params[1];
            if (t->get(a)->body)
                script_pos((int)params[0]) = (Pos&)t->get(a)->body->m;      // the body frame (+0x58)
            else {
                *(Rot*)&script_pos((int)params[0]) = identity_rot();
                script_pos((int)params[0]).o = t->get(a)->pos;
            }
        }
        break;
    case op::agent_set_position:
        if (check_register((int)params[1]) && warn_agent((int)params[0])) {
            agent((int)params[0])->pos = script_pos((int)params[1]).o;
            agent((int)params[0])->position_changed();
        }
        break;
    case op::agent_get_horse:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->horse);
        break;
    case op::agent_get_rider:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->rider);
        break;
    case op::agent_get_party_id:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->party);
        break;
    case op::agent_get_entry_no:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->entry_no);
        break;
    case op::agent_get_troop_id:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->troop);
        break;
    case op::agent_get_item_id:
        // (works only for horses, returns -1 otherwise)
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->f8c.item);
        break;
    case op::store_agent_hit_points:
        // [absolute] 1 = hit points, else percent of the maximum
        if (warn_agent((int)params[1])) {
            if (params[2] == 1)
                SET_LV((__int64)agent((int)params[1])->health);
            else {
                AgentTable* t = &mission()->agents;
                int a = (int)params[1];
                float h = t->get(a)->health * 100.0f;
                SET_LV((__int64)(h / t->get(a)->max_health));
            }
        }
        break;
    case op::agent_set_hit_points:
        // [absolute] 1 = hit points, else percent of the maximum; clamped to 0..max
        if (warn_agent((int)params[0])) {
            if (params[2] == 1) {
                AgentTable* t = &mission()->agents;
                int a = (int)params[0];
                float h = (float)clamp_index((int)params[1], 0, (int)(__int64)(t->get(a)->max_health + 1.0f));
                t->get(a)->health = h;
            } else {
                AgentTable* t = &mission()->agents;
                int a = (int)params[0];
                float h = (float)clamp_index((int)(__int64)((float)params[1] * t->get(a)->max_health * 0.01f), 0,
                                             (int)(__int64)(t->get(a)->max_health + 1.0f));
                t->get(a)->health = h;
            }
        }
        break;
    case op::agent_deliver_damage_to_agent:
        // a blow from <agent_id_deliverer> as strong as its best wielded damage, at the victim's position
        if (warn_agent((int)params[0]) && warn_agent((int)params[1])) {
            AgentTable* t = &mission()->agents;
            int victim = (int)params[1];
            if (t->get(victim)->state == 1) {
                Blow b;
                b.init();
                int attacker = (int)params[0];
                b.flags = 0x20;
                b.f38 = 0;
                b.attacker = attacker;
                b.pos = t->get(victim)->pos;
                int best = 0;
                int d = (int)t->get(attacker)->slot_pair(0).stat_21x(0);
                if (d > 0)
                    best = d;
                d = (int)agent((int)params[0])->slot_pair(0).stat_21x(1);
                if (best < d)
                    best = d;
                d = (int)agent((int)params[0])->slot_pair(1).stat_21x(0);
                if (best < d)
                    best = d;
                d = (int)agent((int)params[0])->slot_pair(1).stat_21x(1);
                if (best < d)
                    best = d;
                b.damage = (float)best;
                agent((int)params[1])->deliver_blow(&b);
                agent((int)params[1])->update_after_blow(0);
            }
        }
        break;
    case op::agent_get_kill_count:
        // (agent_get_kill_count, <destination>, <agent_id>, [get_wounded])
        if (warn_agent((int)params[1])) {
            if (params[2] == 0)
                SET_LV((__int64)agent((int)params[1])->kill_count);
            else
                SET_LV((__int64)agent((int)params[1])->wounded_count);
        }
        break;
    case op::agent_get_wielded_item:
        // (agent_get_wielded_item, <destination>, <agent_id>, <hand_no>)
        if (warn_agent((int)params[1]) && IN_RANGE(params[2], 0, 2)) {
            ItemRef r = agent((int)params[1])->slot_pair((int)params[2]);
            SET_LV((__int64)r.item);
        }
        break;
    case op::agent_get_ammo:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->total_ammo());
        break;
    case op::agent_refill_ammo:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->refill_ammo((int)params[1]);
        break;
    case op::agent_has_item_equipped:
        if (warn_agent((int)params[0]) && check_item_kind((int)params[1])) {
            AgentTable* t = &mission()->agents;
            int a = (int)params[0];
            result = 0;
            for (int k = 0; k < 10; ++k)
                if ((__int64)t->get(a)->equipment[k].item == params[1])
                    result = 1;
        }
        break;
    case op::agent_set_scripted_destination:
        // [auto_set_z_to_ground_level] inside the scene bounds only
        if (warn_agent((int)params[0]) && check_register((int)params[1])) {
            MissionX* m = mission();
            if (params[2]) {
                rgl::vector4* o = &script_pos((int)params[1]).o;
                if (m->min_x <= o->x && o->x < m->max_x && m->min_y <= o->y && o->y < m->max_y) {
                    script_pos((int)params[1]).o.z = m->scene->ground_height(o);
                    m = mission();
                }
            }
            (Pos&)agent((int)params[0])->scripted_destination = script_pos((int)params[1]);
            agent((int)params[0])->scripted_mode = 1;
            agent((int)params[0])->f2c0 = 0;
        }
        break;
    case op::agent_get_scripted_destination:
        if (warn_agent((int)params[1]) && check_register((int)params[0]))
            script_pos((int)params[0]) = (Pos&)agent((int)params[1])->scripted_destination;
        break;
    case op::agent_clear_scripted_mode:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->scripted_mode = 0;
        break;
    case op::agent_set_speed_limit:
        // km/h to m/s
        if (warn_agent((int)params[0])) {
            float v = (float)params[1] * 0.2777778f;
            agent((int)params[0])->speed_limit = v;
        }
        break;
    case op::agent_ai_set_always_attack_in_melee:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->always_attack_in_melee = (int)params[1];
        break;
    case op::agent_get_simple_behavior:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->simple_behavior);
        break;
    case op::agent_get_combat_state:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->combat_state);
        break;
    case op::agent_set_animation:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->set_animation(0, (int)params[1], 0x500007f, 1.0f);
        break;
    case op::agent_set_stand_animation:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->stand_animation = (int)params[1];
        break;
    case op::agent_set_walk_forward_animation:
        if (warn_agent((int)params[0]))
            agent((int)params[0])->walk_forward_animation = (int)params[1];
        break;
    case op::agent_set_animation_progress:
        // the fraction part of the fixed-point value
        if (warn_agent((int)params[0])) {
            __int64 frac = params[1] % (__int64)g_fixed_point_multiplier;
            agent((int)params[0])->set_animation_progress(0, (float)frac / (float)g_fixed_point_multiplier);
        }
        break;
    case op::agent_play_sound:
        if (warn_agent((int)params[0]) && warn_sound((int)params[1])) {
            int snd = (int)params[1];
            agent((int)params[0])->play_sound(&snd);
        }
        break;
    case op::agent_get_team:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->clamped_team());
        break;
    case op::agent_set_team:
        // an agent joining a team without a leader becomes its leader
        if (warn_agent((int)params[0])) {
            int team = (int)params[1];
            agent((int)params[0])->set_team(&team);
            MissionX* m = mission();
            int a = (int)params[0];
            TeamRec* tr = &m->teams[m->agents.get(a)->clamped_team()];
            if (tr->leader < 0)
                tr->leader = m->agents.get(a)->id;
        }
        break;
    case op::agent_get_class:
        if (warn_agent((int)params[1]))
            SET_LV((__int64)agent((int)params[1])->agent_class());
        break;

    // ---- teams and orders ---------------------------------------------------------------------------------------
    case op::class_is_listening_order:
        // (class_is_listening_order, <team_no>, <sub_class>)
        if (IN_RANGE(params[0], 0, 8) && IN_RANGE(params[1], 0, 3))
            result = mission()->teams[(int)params[0]].cls[(int)params[1]].use == 1;
        break;
    case op::entry_point_get_position:
        if (check_register((int)params[0]) && IN_RANGE(params[1], 0, 0x40) && g_mission)
            script_pos((int)params[0]) = mission()->entries[(int)params[1]].pos;
        break;
    case op::team_get_hold_fire_order:
        // (team_get_hold_fire_order, <destination>, <team_no>, <sub_class>)
        if (IN_RANGE(params[1], 0, 8)) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            SET_LV((__int64)mission()->hold_fire_order((int)params[1]));
            mission()->unselect_team((int)params[1]);
        }
        break;
    case op::team_get_movement_order:
        if (IN_RANGE(params[1], 0, 8)) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            SET_LV((__int64)mission()->movement_order((int)params[1]));
            mission()->unselect_team((int)params[1]);
        }
        break;
    case op::team_get_riding_order:
        if (IN_RANGE(params[1], 0, 8)) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            SET_LV((__int64)mission()->riding_order((int)params[1]));
            mission()->unselect_team((int)params[1]);
        }
        break;
    case op::team_get_weapon_usage_order:
        if (IN_RANGE(params[1], 0, 8)) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            SET_LV((__int64)mission()->weapon_usage_order((int)params[1]));
            mission()->unselect_team((int)params[1]);
        }
        break;
    case op::teams_are_enemies:
        if (IN_RANGE(params[0], 0, 8) && IN_RANGE(params[1], 0, 8))
            result = mission()->teams[(int)params[0]].relation[(int)params[1]] < 0;
        break;

    case op::team_give_order:
        // (team_give_order, <team_no>, <sub_class>, <order_id>): a negative sub class orders the whole team
        if (IN_RANGE(params[0], 0, 8) && IN_RANGE(params[2], 0, 0xe)) {
            mission()->select_team((int)params[0]);
            if (params[1] >= 0)
                mission()->select_class((int)params[0], (int)params[1]);
            mission()->give_order((int)params[0], (int)params[2]);
            mission()->unselect_team((int)params[0]);
        }
        break;
    case op::team_set_order_position:
        // (team_set_order_position, <team_no>, <sub_class>, <position_no>)
        if (IN_RANGE(params[0], 0, 8) && check_register((int)params[2]) &&
            mission()->point_in_rect(&script_pos((int)params[2]).o)) {
            mission()->select_team((int)params[0]);
            mission()->select_class((int)params[0], (int)params[1]);
            MissionX* m = mission();
            rgl::vector4* p = &script_pos((int)params[2]).o;
            float z = 0, y = 0, x = 0;
            int face = m->scene->face_at(p);
            vec4 off;
            m->teams[(int)params[0]].set_order_position(p, &face, off.set(x, y, z));
            mission()->unselect_team((int)params[0]);
        }
        break;
    case op::team_get_leader:
        if (IN_RANGE(params[1], 0, 8))
            SET_LV((__int64)mission()->teams[(int)params[1]].leader);
        break;
    case op::team_set_leader:
        if (IN_RANGE(params[0], 0, 8) && warn_agent((int)params[1]))
            mission()->teams[(int)params[0]].leader = (int)params[1];
        break;
    case op::team_get_order_position:
        // (team_get_order_position, <position_no>, <team_no>, <sub_class>)
        if (IN_RANGE(params[1], 0, 8) && check_register((int)params[0])) {
            mission()->select_team((int)params[1]);
            mission()->select_class((int)params[1], (int)params[2]);
            MissionX* m = mission();
            int t = (int)params[1];
            script_pos((int)params[0]).o = m->teams[t].order_position();
            m->unselect_team(t);
        }
        break;
    case op::team_set_order_listener:
        if (IN_RANGE(params[0], 0, 8))
            mission()->select_class((int)params[0], (int)params[1]);
        break;
    case op::team_set_relation:
        // (team_set_relation, <team_no>, <team_no_2>, <value>), # -1 for enemy, 1 for friend, 0 for neutral
        if (IN_RANGE(params[0], 0, 8) && IN_RANGE(params[1], 0, 8) && IN_RANGE(params[2], -1, 2)) {
            mission()->teams[(int)params[0]].relation[(int)params[1]] = (int)params[2];
            mission()->teams[(int)params[1]].relation[(int)params[0]] = (int)params[2];
        }
        break;

    // ---- scene, weather and scene props -------------------------------------------------------------------------
    case op::set_rain:
        // (set_rain, <rain-type>, <strength>), (rain_type: 1= rain, 2=snow ; strength: 0 - 100)
        if (g_mission) {
            mission()->rain_type = (int)params[0];
            mission()->rain_strength = (float)params[1] * 0.01f;
            if (params[0] && params[1] > 50)
                mission()->rain_f65fc = 3;
        }
        break;
    case op::set_fog_distance:
        // (set_fog_distance, <distance_in_meters>, [fog_color]): 0 = the colour of the scene's environment row
        if (g_mission) {
            mission()->fog_distance = (float)params[0];
            if (mission()->scene) {
                MissionX* m = mission();
                unsigned color = g_env_rows_x[m->env_row].fog_color;
                if (params[1])
                    color = (unsigned)params[1];
                float d = (float)params[0];
                float one = 1.0f;
                m->scene->set_fog(&one, &d, &color);
            }
        }
        break;
    case op::get_scene_boundaries:
        if (check_register((int)params[0]) && check_register((int)params[1])) {
            script_pos((int)params[0]) = identity_frame();
            script_pos((int)params[1]) = identity_frame();
            MissionX* m = mission();
            script_pos((int)params[0]).o.x = m->rect_min_x;
            script_pos((int)params[0]).o.y = m->rect_min_y;
            script_pos((int)params[1]).o.x = m->rect_max_x;
            script_pos((int)params[1]).o.y = m->rect_max_y;
        }
        break;
    case op::scene_prop_get_num_instances:
        if (warn_scene_prop((int)params[1]))
            SET_LV((__int64)mission()->num_prop_instances((int)params[1]));
        break;
    case op::scene_prop_get_instance:
        // fails when there is no such instance
        if (warn_scene_prop((int)params[1])) {
            int n = mission()->prop_instance((int)params[1], (int)params[2]);
            if (n < 0)
                result = 0;
            else
                SET_LV((__int64)n);
        }
        break;
    case op::prop_instance_get_position:
        if (check_register((int)params[0]) && warn_scene_prop_instance((int)params[1]) && g_mission)
            script_pos((int)params[0]) = mission()->props.get((int)params[1])->frame;
        break;
    case op::prop_instance_get_starting_position:
        if (check_register((int)params[0]) && warn_scene_prop_instance((int)params[1]) && g_mission)
            script_pos((int)params[0]) = mission()->props.get((int)params[1])->start_frame;
        break;
    case op::prop_instance_get_scale:
        // the scale as the diagonal of an otherwise identity frame
        if (check_register((int)params[0]) && warn_scene_prop_instance((int)params[1]) && g_mission) {
            Pos* dst = &script_pos((int)params[0]);
            *dst = identity_frame();
            PropVec* pv = &mission()->props;
            int k = (int)params[1];
            dst->s.x = pv->get(k)->scale_x;
            dst->f.y = pv->get(k)->scale_y;
            dst->u.z = pv->get(k)->scale_z;
        }
        break;
    case op::prop_instance_set_position:
        if (warn_scene_prop_instance((int)params[0]) && check_register((int)params[1]) && g_mission)
            mission()->props.get((int)params[0])->set_position(&script_pos((int)params[1]));
        break;
    case op::prop_instance_animate_to_position:
        // (prop_instance_animate_to_position, <scene_prop_id>, position, <duration-in-1/100-seconds>)
        if (warn_scene_prop_instance((int)params[0]) && check_register((int)params[1]) && g_mission) {
            float zero = 0;
            float t = (float)params[2] * 0.01f;
            mission()->props.get((int)params[0])->animate_to(&script_pos((int)params[1]), &t, &zero);
        }
        break;
    case op::replace_scene_props:
        // only in the scene-load trigger (context -19)
        if (warn_scene_prop((int)params[0]) && warn_scene_prop((int)params[1]) && *context == -19)
            mission()->replace_props((int)params[0], (int)params[1]);
        break;


    // ---- mission control, lights, particles, spawning -----------------------------------------------------------
    case op::set_mission_result:
        mission()->result = (int)params[0];
        break;
    case op::finish_mission:
        mission()->finish((int)params[0]);
        break;
    case op::jump_to_scene:
        // (jump_to_scene, <scene_id>, <entry_no>): the entry is set with set_jump_entry
        if (check_scene((int)params[0]))
            game()->jump_scene = (int)params[0];
        break;
    case op::set_jump_mission:
        game()->jump_mission = (int)params[0];
        break;
    case op::set_jump_entry:
        if (IN_RANGE(params[0], 0, 0x40))
            game()->jump_entry = (int)params[0];
        break;
    case op::start_mission_conversation:
        {
            MissionX* m = mission();
            m->conversation = m->start_conversation((int)params[0]);
        }
        break;
    case op::add_reinforcements_to_entry:
        mission()->add_reinforcements((int)params[0], (int)params[1]);
        break;
    case op::mission_enable_talk:
        game()->talk_enabled = 1;
        break;
    case op::mission_disable_talk:
        game()->talk_disabled = 1;
        break;
    case op::mission_tpl_entry_set_override_flags:
        // (mission_entry_set_override_flags, <mission_template_id>, <entry_no>, <value>)
        if (check_mission_template((int)params[0]) && params[1] >= 0) {
            MissionTemplate* t = &g_mission_templates[(int)params[0]];
            if (params[1] < (__int64)t->num_entries)
                t->entries[(int)params[1]].override_flags = (int)params[2];
        }
        break;
    case op::mission_tpl_entry_clear_override_items:
        if (check_mission_template((int)params[0]) && params[1] >= 0) {
            MissionTemplate* t = &g_mission_templates[(int)params[0]];
            if (params[1] < (__int64)t->num_entries)
                t->entries[(int)params[1]].num_override_items = 0;
        }
        break;
    case op::mission_tpl_entry_add_override_item:
        // at most 8 override items per entry
        if (check_mission_template((int)params[0]) && params[1] >= 0 &&
            params[1] < (__int64)g_mission_templates[(int)params[0]].num_entries &&
            check_item_kind((int)params[2])) {
            MTEntry* e = &g_mission_templates[(int)params[0]].entries[(int)params[1]];
            int n = e->num_override_items;
            if (n < 8) {
                e->override_items[n] = (int)params[2];
                ++g_mission_templates[(int)params[0]].entries[(int)params[1]].num_override_items;
            }
        }
        break;
    case op::set_current_color:
        // red, green, blue: a value of 255 means 100% (the original scales by 1/100)
        {
            float z = (float)params[2] * 0.01f;
            float y = (float)params[1] * 0.01f;
            float x = (float)params[0] * 0.01f;
            vec4 c;
            game()->current_color = c.set(x, y, z);
        }
        break;
    case op::set_position_delta:
        {
            float z = (float)params[2] * 0.01f;
            float y = (float)params[1] * 0.01f;
            float x = (float)params[0] * 0.01f;
            vec4 d;
            game()->position_delta = d.set(x, y, z);
        }
        break;
    case op::add_point_light:
        // (add_point_light, [flicker_magnitude], [flicker_interval]): on the trigger's item (context -50) or the scene
        if (params[0] >= 0 && params[1] >= 0) {
            if (*context == -50) {
                SceneEntity* ent = (SceneEntity*)trigger_entity(-50);
                if (ent) {
                    int mesh = trigger_item_mesh(-50);
                    Light* l = (Light*)operator new(sizeof(Light));
                    l = l ? l->init() : 0;
                    l->flicker_magnitude = (float)params[0] * 0.01f;
                    l->flicker_interval = (float)params[1] * 0.01f;
                    l->color = game()->current_color;
                    l->type = 0;
                    l->v34 = game()->position_delta;
                    l->base_color = l->color;
                    l->update();
                    if (mesh >= 0 && ent->meshes)
                        ent->meshes->attach_light(mesh, l);
                }
            } else
                mission()->add_point_light((float)params[0] * 0.01f, (float)params[1] * 0.01f);
        }
        break;
    case op::particle_system_add_new:
        // (particle_system_add_new, <par_sys_id>, [position_no]): on the trigger's entity, at the position delta
        if (warn_particle_system((int)params[0])) {
            int ctx = *context;
            SceneEntity* ent = (SceneEntity*)trigger_entity(ctx);
            if (ent) {
                int mesh = trigger_item_mesh(ctx);
                int zero = 0;
                int psys = (int)params[0];
                void* ps = g_particle_mgr.create(&psys, &game()->position_delta, &zero);
                if (ps) {
                    if (mesh >= 0) {
                        if (ent->meshes)
                            ent->meshes->attach_particle_system(mesh, ps);
                    } else
                        ent->add_particle_system(ps);
                }
            }
        }
        break;
    case op::particle_system_emit:
        // (particle_system_emit, <par_sys_id>, <value_num_particles>, <value_period>)
        if (warn_particle_system((int)params[0])) {
            int ctx = *context;
            SceneEntity* ent = (SceneEntity*)trigger_entity(ctx);
            if (ent) {
                int mesh = trigger_item_mesh(ctx);
                PSys* ps;
                if (mesh >= 0) {
                    if (!ent->meshes)
                        break;
                    ps = ent->meshes->find_particle_system(mesh, (int)params[0]);
                } else {
                    int k = ent->find_particle_system((int)params[0]);
                    if (k < 0)
                        break;
                    ps = ent->particle_systems[k];
                }
                if (ps)
                    ps->emit((int)params[1]);
            }
        }
        break;
    case op::particle_system_burst:
        // (particle_system_burst, <par_sys_id>, <position_no>, [percentage_burst_strength]) default 100
        if (warn_particle_system((int)params[0]) && check_register((int)params[1]) && g_mission) {
            int strength = 100;
            if (params[2])
                strength = (int)params[2];
            mission()->burst((int)params[0], &script_pos((int)params[1]), strength);
        }
        break;
    case op::set_spawn_position:
        if (check_register((int)params[0]))
            game()->trigger_object_pos = script_pos((int)params[0]);
        break;
    case op::spawn_item:
        // (spawn_item, <item_kind_id>, <item_modifier>): the instance id goes to reg0
        if (check_item_kind((int)params[0])) {
            ItemRef r;
            r.item = (int)params[0];
            r.set_modifier((int)params[1]);
            r.reduce(0);
            g_script.reg[0] = mission()->spawn_item(&r);
        }
        break;
    case op::spawn_agent:
        // (spawn_agent, <troop_id>), (stores agent_id in reg0)
        g_script.reg[0] = -1;
        if (check_troop((int)params[0]))
            g_script.reg[0] = mission()->spawn_agent((int)params[0]);
        break;
    case op::spawn_horse:
        g_script.reg[0] = -1;
        if (check_item_kind((int)params[0]))
            g_script.reg[0] = mission()->spawn_horse((int)params[0], (int)params[1]);
        break;
    case op::spawn_scene_prop:
        // opcode 1974 (spawn_scene_prop in later versions): only validates the prop id in 1.011
        warn_scene_prop((int)params[0]);
        break;


    // ---- tableaus -----------------------------------------------------------------------------------------------
    case op::cur_tableau_add_tableau_mesh:
        // (cur_tableau_add_tableau_mesh, <tableau_material_id>, <value>, <position_register_no>, [scale_fixed_point])
        if (warn_tableau((int)params[0]) && check_register((int)params[2]) && g_tableaus.stack.size()) {
            vec4 a, b, c;
            const rgl::vector4* scale;
            if (params[3] == 0) {
                float z = 1.0f, y = 1.0f, x = 1.0f;
                scale = &a.set(x, y, z);
            } else {
                float s = 1.0f / g_fixed_point_multiplier;
                float x, y, z;
                x = y = z = (float)params[3];
                c = b.set(z, y, x) * s;
                scale = &c;
            }
            g_tableaus.add_tableau_mesh((int)params[0], (int)params[1], &script_pos((int)params[2]), *scale);
        }
        break;
    case op::cur_item_set_tableau_material:
        // only inside ti_on_init_item (context -50): the item, else the agent's mesh object
        if (warn_tableau((int)params[0]) && *context == -50) {
            TriggerItem* it = trigger_item(-50);
            if (it) {
                int code = (int)params[1], tab = (int)params[0];
                it->set_tableau_material(g_tableaus.material(&tab, &code));
            } else {
                TriggerMeshObj* o = trigger_mesh_obj(-50);
                if (o) {
                    int code = (int)params[1], tab = (int)params[0];
                    o->set_tableau_material(g_tableaus.material(&tab, &code));
                }
            }
        }
        break;
    case op::cur_scene_prop_set_tableau_material:
        // only inside ti_on_init_scene_prop (context -40)
        if (warn_tableau((int)params[0]) && *context == -40) {
            TriggerMeshObj* o = trigger_mesh_obj(-40);
            if (o) {
                int code = (int)params[1], tab = (int)params[0];
                o->set_tableau_material(g_tableaus.material(&tab, &code));
            }
        }
        break;
    case op::cur_map_icon_set_tableau_material:
        // only inside ti_on_init_map_icon (context -70)
        if (warn_tableau((int)params[0]) && *context == -70) {
            TriggerMeshObj* o = trigger_mesh_obj(-70);
            if (o) {
                int code = (int)params[1], tab = (int)params[0];
                o->set_tableau_material(g_tableaus.material(&tab, &code));
            }
        }
        break;
    case op::cur_tableau_render_as_alpha_mask:
        if (g_tableaus.stack.size())
            g_tableaus.alpha_mask = 1;
        break;
    case op::cur_tableau_set_background_color:
        if (g_tableaus.stack.size())
            g_tableaus.set_background_color((int)params[0]);
        break;
    case op::cur_tableau_set_ambient_light:
        // (cur_tableau_set_ambient_light, <red_fixed_point>, <green_fixed_point>, <blue_fixed_point>)
        if (g_tableaus.stack.size()) {
            float m = (float)g_fixed_point_multiplier;
            float z = (float)params[2] / m;
            float y = (float)params[1] / m;
            float x = (float)params[0] / m;
            vec4 c;
            g_tableaus.set_ambient_light(c.set(x, y, z));
        }
        break;
    case op::cur_tableau_set_camera_position:
        if (check_register((int)params[0]) && g_tableaus.stack.size())
            g_tableaus.set_camera_position(&script_pos((int)params[0]));
        break;
    case op::cur_tableau_set_camera_parameters:
        // (.., <is_perspective>, <camera_width_times_1000>, <camera_height_times_1000>, <camera_near_times_1000>,
        // <camera_far_times_1000>): fixed point in fact
        if (g_tableaus.stack.size()) {
            float m = (float)g_fixed_point_multiplier;
            float far_ = (float)params[4] / m;
            float near_ = (float)params[3] / m;
            float h = (float)params[2] / m;
            float w = (float)params[1] / m;
            bool perspective = params[0] != 0;
            g_tableaus.set_camera_parameters(&perspective, &w, &h, &near_, &far_);
        }
        break;
    case op::cur_tableau_add_point_light:
        // the position is operand 0 and the colour operands 1..3 (the header's <map_icon_id> is not read)
        if (check_register((int)params[0]) && g_tableaus.stack.size()) {
            float m = (float)g_fixed_point_multiplier;
            float z = (float)params[3] / m;
            float y = (float)params[2] / m;
            float x = (float)params[1] / m;
            vec4 c;
            g_tableaus.add_point_light(&script_pos((int)params[0]), c.set(x, y, z));
        }
        break;
    case op::cur_tableau_add_sun_light:
        if (check_register((int)params[0]) && g_tableaus.stack.size()) {
            float m = (float)g_fixed_point_multiplier;
            float z = (float)params[3] / m;
            float y = (float)params[2] / m;
            float x = (float)params[1] / m;
            vec4 c;
            g_tableaus.add_sun_light(&script_pos((int)params[0]), c.set(x, y, z));
        }
        break;
    case op::cur_tableau_add_mesh:
        // (cur_tableau_add_mesh, <mesh_id>, <position_no>, <scale_fixed_point>, <alpha_fixed_point>)
        if (check_mesh((int)params[0]) && check_register((int)params[1]) && g_tableaus.stack.size()) {
            int color = -1;
            float alpha;
            if (params[3] == 0)
                alpha = 1.0f;
            else
                alpha = (float)params[3] / g_fixed_point_multiplier;
            vec4 a, b, c;
            const rgl::vector4* scale;
            if (params[2] == 0) {
                float z = 1.0f, y = 1.0f, x = 1.0f;
                scale = &a.set(x, y, z);
            } else {
                float s = 1.0f / g_fixed_point_multiplier;
                float x, y, z;
                x = y = z = (float)params[2];
                c = b.set(z, y, x) * s;
                scale = &c;
            }
            g_tableaus.add_mesh((int)params[0], &script_pos((int)params[1]), *scale, &alpha, &color);
        }
        break;
    case op::cur_tableau_add_mesh_with_vertex_color:
        // (.., <mesh_id>, <position_no>, <scale_fixed_point>, <alpha_fixed_point>, <vertex_color>)
        if (check_mesh((int)params[0]) && check_register((int)params[1]) && g_tableaus.stack.size()) {
            int color = (int)params[4];
            float alpha;
            if (params[3] == 0)
                alpha = 1.0f;
            else
                alpha = (float)params[3] / g_fixed_point_multiplier;
            vec4 a, b, c;
            const rgl::vector4* scale;
            if (params[2] == 0) {
                float z = 1.0f, y = 1.0f, x = 1.0f;
                scale = &a.set(x, y, z);
            } else {
                float s = 1.0f / g_fixed_point_multiplier;
                float x, y, z;
                x = y = z = (float)params[2];
                c = b.set(z, y, x) * s;
                scale = &c;
            }
            g_tableaus.add_mesh((int)params[0], &script_pos((int)params[1]), *scale, &alpha, &color);
        }
        break;
    case op::cur_tableau_add_map_icon:
        // (cur_tableau_add_map_icon, <map_icon_id>, <position_no>, <scale_fixed_point>)
        if (check_map_icon((int)params[0]) && check_register((int)params[1]) && g_tableaus.stack.size()) {
            float scale;
            if (params[2] == 0)
                scale = 1.0f;
            else
                scale = (float)params[2] / g_fixed_point_multiplier;
            g_tableaus.add_map_icon((int)params[0], &script_pos((int)params[1]), &scale);
        }
        break;
    case op::cur_tableau_add_troop:
        // (.., <troop_id>, <position_no>, <animation_id>, <instance_no>): instance <= 0 = no random face (heroes)
        if (check_troop((int)params[0]) && check_register((int)params[1]) && check_animation((int)params[2]) &&
            g_tableaus.stack.size()) {
            int instance;
            if (params[3] > 0)
                instance = (int)params[3];
            else
                instance = -1;
            int anim = (int)params[2];
            tableau_add_agent((int)params[0], -1, &script_pos((int)params[1]), &anim, &instance);
        }
        break;
    case op::cur_tableau_add_horse:
        // (cur_tableau_add_horse, <item_id>, <position_no>, <animation_id>)
        if (check_item_kind((int)params[0]) && check_register((int)params[1]) && check_animation((int)params[2]) &&
            g_tableaus.stack.size()) {
            int anim = (int)params[2];
            int instance = -1;
            tableau_add_agent(-1, (int)params[0], &script_pos((int)params[1]), &anim, &instance);
        }
        break;
    case op::cur_tableau_set_override_flags:
        if (g_tableaus.stack.size())
            g_tableaus.current()->override_flags = (int)params[0];
        break;
    case op::cur_tableau_clear_override_items:
        if (g_tableaus.stack.size())
            g_tableaus.current()->num_override_items = 0;
        break;
    case op::cur_tableau_add_override_item:
        // no bound on the item count
        if (g_tableaus.stack.size()) {
            int n = g_tableaus.current()->num_override_items;
            g_tableaus.current()->override_items[n] = (int)params[0];
            ++g_tableaus.current()->num_override_items;
        }
        break;
    case op::cur_tableau_add_mesh_with_scale_and_vertex_color:
        // (.., <mesh_id>, <position_no>, <scale_position_no>, <alpha_fixed_point>, <vertex_color>): the scale is the
        // origin of the second position, in fixed point
        if (check_mesh((int)params[0]) && check_register((int)params[1]) && check_register((int)params[2]) &&
            g_tableaus.stack.size()) {
            int color = (int)params[4];
            float alpha;
            if (params[3] == 0)
                alpha = 1.0f;
            else
                alpha = (float)params[3] / g_fixed_point_multiplier;
            float s = 1.0f / g_fixed_point_multiplier;
            g_tableaus.add_mesh((int)params[0], &script_pos((int)params[1]),
                                V(script_pos((int)params[2]).o) * s, &alpha, &color);
        }
        break;


    // ---- mission camera and time speed --------------------------------------------------------------------------
    case op::mission_cam_set_mode:
        // (mission_cam_set_mode, <mission_cam_mode>, <duration-in-1/1000-seconds>, <value>): leaving manual mode
        // animates from the current frame
        if (g_mission) {
            MissionX* m = mission();
            m->cam_mode = clamp_index((int)params[0], 0, 2);
            if (mission()->cam_mode == 0 && params[1] > 0) {
                mission()->cam_target_frame = mission()->cam_current_frame();
                mission()->cam_target_aperture = mission()->cam_current_aperture();
                mission()->cam_anim_time = (float)params[1] * 0.001f;
                mission()->cam_timer.restart();
                mission()->cam_easing = 0;
                if (params[2] > 0)
                    mission()->cam_easing = (int)params[2];
            } else
                mission()->cam_anim_time = 0;
        }
        break;
    case op::mission_get_time_speed:
        if (g_mission)
            SET_LV((__int64)(mission()->current_time_speed() * g_fixed_point_multiplier));
        break;
    case op::mission_set_time_speed:
        // (mission_set_time_speed, <value_fixed_point>) #this works only when cheat mode is enabled
        if (g_mission && params[0] > 0) {
            mission()->time_speed = (float)params[0] / g_fixed_point_multiplier;
            mission()->time_speed_anim_time = 0;
        }
        break;
    case op::mission_time_speed_move_to_value:
        // (mission_speed_move_to_value, <value_fixed_point>, <duration-in-1/1000-seconds>)
        if (g_mission && params[0] > 0 && params[1] > 0) {
            MissionX* m = mission();
            m->time_speed = m->current_time_speed();
            mission()->target_time_speed = (float)params[0] / g_fixed_point_multiplier;
            mission()->time_speed_anim_time = (float)params[1] * 0.001f;
            mission()->time_speed_timer.restart();
        }
        break;
    case op::mission_cam_get_position:
        if (g_mission && check_register((int)params[0]))
            script_pos((int)params[0]) = mission()->cam_current_frame();
        break;
    case op::mission_cam_set_position:
        if (g_mission && check_register((int)params[0])) {
            mission()->cam_frame = script_pos((int)params[0]);
            mission()->cam_anim_time = 0;
        }
        break;
    case op::mission_cam_animate_to_position:
        // (.., <position_register_no>, <duration-in-1/1000-seconds>, <value>) # value 0: linear velocity
        if (g_mission && check_register((int)params[0]) && params[1] > 0) {
            mission()->cam_frame = mission()->cam_current_frame();
            mission()->cam_target_frame = script_pos((int)params[0]);
            MissionX* m = mission();
            m->cam_target_aperture = m->cam_current_aperture();
            mission()->cam_anim_time = (float)params[1] * 0.001f;
            mission()->cam_timer.restart();
            mission()->cam_easing = 0;
            if (params[2] > 0)
                mission()->cam_easing = (int)params[2];
        }
        break;
    case op::mission_cam_get_aparture:
        if (g_mission)
            SET_LV((__int64)mission()->cam_current_aperture());
        break;
    case op::mission_cam_set_aparture:
        // clamped to 37..149
        if (g_mission) {
            MissionX* m = mission();
            m->cam_aperture = (float)clamp_index((int)params[0], 0x25, 0x96);
            mission()->cam_anim_time = 0;
        }
        break;
    case op::mission_cam_animate_to_aparture:
        // (mission_cam_animate_to_aparture, <value>, <duration-in-1/1000-seconds>, <value>)
        if (g_mission && params[1] > 0) {
            mission()->cam_frame = mission()->cam_current_frame();
            mission()->cam_target_frame = mission()->cam_frame;
            MissionX* m = mission();
            m->cam_aperture = m->cam_current_aperture();
            mission()->cam_target_aperture = (float)clamp_index((int)params[0], 0x25, 0x96);
            mission()->cam_anim_time = (float)params[1] * 0.001f;
            mission()->cam_timer.restart();
            mission()->cam_easing = (int)params[2];
        }
        break;
    case op::mission_cam_animate_to_position_and_aparture:
        // (.., <position_register_no>, <value>, <duration-in-1/1000-seconds>, <value>)
        if (g_mission && check_register((int)params[0]) && params[2] > 0) {
            mission()->cam_frame = mission()->cam_current_frame();
            mission()->cam_target_frame = script_pos((int)params[0]);
            MissionX* m = mission();
            m->cam_aperture = m->cam_current_aperture();
            mission()->cam_target_aperture = (float)clamp_index((int)params[1], 0x25, 0x96);
            mission()->cam_anim_time = (float)params[2] * 0.001f;
            mission()->cam_timer.restart();
            mission()->cam_easing = (int)params[3];
        }
        break;
    case op::mission_cam_set_target_agent:
        // (mission_cam_set_target_agent, <agent_id>, <value>) #value 0: do not use the agent's rotation
        if (g_mission && warn_agent((int)params[0])) {
            mission()->cam_target_agent = (int)params[0];
            mission()->cam_use_agent_rotation = (int)params[1];
        }
        break;
    case op::mission_cam_clear_target_agent:
        if (g_mission)
            mission()->cam_target_agent = -1;
        break;


    // ---- talk info, menus, screens, trigger parameters ----------------------------------------------------------
    case op::talk_info_show:
        // (talk_info_show, <hide_or_show>) :0=hide 1=show
        game()->talk_info_shown = (int)params[0];
        break;
    case op::talk_info_set_relation_bar:
        // a value outside -100..100 hides the bar
        game()->talk_relation_bar = (int)params[0];
        break;
    case op::talk_info_set_line:
        // (talk_info_set_line, <line_no>, <string_no>)
        if (IN_RANGE(params[0], 0, 5) && check_string_arg(types[1], (int)params[1])) {
            game()->set_talk_line((int)params[0], g_script.expand(g_strings.get((int)params[1], types[1])));
            game()->talk_line_set[(int)params[0]] = 1;
        }
        break;
    case op::set_background_mesh:
        // only while a game menu is up
        if (check_mesh((int)params[0]) && check_menu(game()->current_menu))
            game()->background_mesh = g_meshes[(int)params[0]].name;
        break;
    case op::set_game_menu_tableau_mesh:
        // (set_game_menu_tableau_mesh, <tableau_material_id>, <value>, <position_register_no>)
        if (warn_tableau((int)params[0]) && check_menu(game()->current_menu) && check_register((int)params[2])) {
            game()->menu_tableau = (int)params[0];
            game()->menu_tableau_value = (int)params[1];
            game()->menu_tableau_pos = script_pos((int)params[2]).o;
        }
        break;
    case op::change_screen_return:
        game()->next_window = 1;
        break;
    case op::change_screen_loot:
        if (check_troop((int)params[0])) {
            game()->screen_troop = (int)params[0];
            game()->next_window = 2;
        }
        break;
    case op::change_screen_trade:
        // (change_screen_trade, <troop_id>)
        if (check_troop((int)params[0])) {
            game()->screen_troop = (int)params[0];
            game()->next_window = 4;
        }
        break;
    case op::change_screen_exchange_members:
        // (change_screen_exchange_members, [0,1 = exchange_leader])
        game()->screen_party = game()->encountered_party;
        if (params[0])
            game()->next_window = 6;
        else
            game()->next_window = 5;
        break;
    case op::change_screen_give_members:
        game()->screen_party = game()->encountered_party;
        game()->next_window = 7;
        break;
    case op::change_screen_trade_prisoners:
        game()->next_window = 9;
        break;
    case op::change_screen_buy_mercenaries:
        game()->next_window = 10;
        break;
    case op::change_screen_view_character:
        game()->next_window = 0xb;
        break;
    case op::change_screen_training:
        game()->next_window = 0xc;
        break;
    case op::change_screen_mission:
        game()->next_window = 0xd;
        break;
    case op::change_screen_map_conversation:
        // (change_screen_map_conversation, <troop_id>)
        if (check_troop((int)params[0])) {
            game()->screen_troop = (int)params[0];
            game()->next_window = 0xe;
        }
        break;
    case op::change_screen_exchange_with_party:
        if (check_party((int)params[0])) {
            game()->screen_party = (int)params[0];
            game()->next_window = 8;
        }
        break;
    case op::change_screen_equip_other:
        if (check_troop((int)params[0])) {
            game()->screen_troop = (int)params[0];
            game()->next_window = 3;
        }
        break;
    case op::change_screen_map:
        game()->next_window = 0x11;
        break;
    case op::change_screen_notes:
        // (change_screen_notes, <note_type>, <object_id>), 1 = troops, 2 = factions, 3 = parties, 4 = quests
        if (params[0] >= 1 && params[0] <= 4) {
            char ok;
            switch ((int)params[0]) {
            case 1: ok = check_troop((int)params[1]); break;
            case 2: ok = check_faction((int)params[1]); break;
            case 3: ok = check_party((int)params[1]); break;
            case 4: ok = check_quest((int)params[1]); break;
            }
            if (ok) {
                game()->next_window = 0x10;
                game()->notes_type = (int)params[0];
                game()->notes_object = (int)params[1];
            }
        }
        break;
    case op::change_screen_quit:
        // destroys the game object
        g_windows.close_all();
        if (g_game) {
            delete game();
            g_game = 0;
        }
        g_windows.set_state(4);
        break;
    case op::jump_to_menu:
        if (check_menu((int)params[0])) {
            game()->current_menu = (int)params[0];
            g_passage_menu = (int)params[0];
            g_menu_b = (int)params[0];
            game()->next_window = 0x12;
        }
        break;
    case op::disable_menu_option:
        game()->menu_option_disabled = 1;
        break;
    case op::store_trigger_param_1:
        set_lvalue(locals, g_trigger_param_1);
        break;
    case op::store_trigger_param_2:
        set_lvalue(locals, g_trigger_param_2);
        break;
    case op::set_trigger_result:
        g_trigger_result = params[0];
        break;

    // ---- arithmetic (64-bit; fixed point by g_fixed_point_multiplier) ------------------------------------------
    case op::val_add:
        SET_LV(params[0] + params[1]);
        break;
    case op::val_sub:
        SET_LV(params[0] - params[1]);
        break;
    case op::val_mul:
        SET_LV(params[1] * params[0]);
        break;
    case op::val_div:
        // division by zero stores 40000000
        if (params[1] == 0)
            SET_LV(40000000);
        else
            SET_LV(params[0] / params[1]);
        break;
    case op::val_mod:
        if (params[1] != 0)
            SET_LV(params[0] % params[1]);
        break;
    case op::val_min:
        // on the low 32 bits
        {
            int a = (int)params[0], b = (int)params[1];
            SET_LV((__int64)min_int(a, b));
        }
        break;
    case op::val_max:
        {
            int a = (int)params[0], b = (int)params[1];
            SET_LV((__int64)imax_ref(&a, &b));
        }
        break;
    case op::val_clamp:
        // dest = max(min(dest, <upper_bound> - 1), <lower_bound>)
        SET_LV((__int64)clamp_index((int)params[0], (int)params[1], (int)params[2]));
        break;
    case op::val_abs:
        {
            __int64 v = params[0];
            if (v < 0)
                v = -v;
            SET_LV(v);
        }
        break;
    case op::val_or:
        SET_LV(params[1] | params[0]);
        break;
    case op::val_and:
        SET_LV(params[1] & params[0]);
        break;
    case op::store_or:
        SET_LV(params[2] | params[1]);
        break;
    case op::store_and:
        SET_LV(params[2] & params[1]);
        break;
    case op::store_mod:
        if (params[2] > 0)
            SET_LV(params[1] % params[2]);
        break;
    case op::store_add:
        SET_LV(params[2] + params[1]);
        break;
    case op::store_sub:
        SET_LV(params[1] - params[2]);
        break;
    case op::store_mul:
        SET_LV(params[2] * params[1]);
        break;
    case op::store_div:
        if (params[2] == 0)
            SET_LV(40000000);
        else
            SET_LV(params[1] / params[2]);
        break;
    case op::set_fixed_point_multiplier:
        if (params[0] > 0)
            g_fixed_point_multiplier = (int)params[0];
        break;
    case op::store_sqrt:
        // (store_sqrt, <destination_fixed_point>, <value_fixed_point>)
        if (params[1] > 0) {
            float m = (float)g_fixed_point_multiplier;
            SET_LV(round_i64((float)(sqrt((float)params[1] / m) * m)));
        }
        break;
    case op::store_pow:
        {
            float m = (float)g_fixed_point_multiplier;
            SET_LV(round_i64((float)(pow((float)params[1] / m, (float)params[2] / m) * m)));
        }
        break;
    case op::store_sin:
        // degrees in fixed point
        {
            float m = (float)g_fixed_point_multiplier;
            SET_LV(round_i64((float)(sin((float)params[1] * 0.017453292f / m) * m)));
        }
        break;
    case op::store_cos:
        {
            float m = (float)g_fixed_point_multiplier;
            SET_LV(round_i64((float)(cos((float)params[1] * 0.017453292f / m) * m)));
        }
        break;
    case op::store_tan:
        {
            float m = (float)g_fixed_point_multiplier;
            SET_LV(round_i64((float)(tan((float)params[1] * 0.017453292f / m) * m)));
        }
        break;
    case op::convert_to_fixed_point:
        SET_LV((__int64)g_fixed_point_multiplier * params[0]);
        break;
    case op::convert_from_fixed_point:
        SET_LV(params[0] / (__int64)g_fixed_point_multiplier);
        break;
    case op::assign:
        set_lvalue(locals, params[1]);
        break;
    case op::shuffle_range:
        // (shuffle_range, <reg_no>, <reg_no>): registers [first, last); the swapped-in value keeps only 32 bits
        if (check_register((int)params[0]) && check_register((int)params[1])) {
            int first = (int)params[0];
            for (int k = first; (__int64)k < params[1]; ++k) {
                int n = (int)params[1] - first;
                int j = rand_below(&n) + (int)params[0];
                int v = (int)g_script.reg[k];
                g_script.reg[k] = g_script.reg[j];
                g_script.reg[j] = v;
            }
        }
        break;
    case op::store_random:
        // deprecated, use store_random_in_range instead
        {
            int n = (int)params[1];
            SET_LV((__int64)rand_below(&n));
        }
        break;
    case op::store_random_in_range:
        // [range_low, range_high) exclusive
        {
            int n = (int)params[2] - (int)params[1];
            SET_LV(rand_below(&n) + params[1]);
        }
        break;


    // ---- store_*: troops, parties, quests, time, map ------------------------------------------------------------
    case op::store_troop_gold:
        if (check_troop((int)params[1]))
            SET_LV((__int64)troop((int)params[1])->gold);
        break;
    case op::store_num_free_stacks:
        // (store_num_free_stacks, <destination>, <party_id>): the used stacks are counted on the main party
        {
            int used = party(game()->main_party)->num_regular_stacks();
            SET_LV((__int64)(party((int)params[1])->max_stacks() - used));
        }
        break;
    case op::store_num_free_prisoner_stacks:
        {
            int used = party(game()->main_party)->num_prisoner_stacks();
            SET_LV((__int64)(party((int)params[1])->max_stacks() - used));
        }
        break;
    case op::store_party_size:
        // (store_party_size, <destination>, [party_id])
        SET_LV((__int64)party((int)params[1])->num_members());
        break;
    case op::store_party_size_wo_prisoners:
        SET_LV((__int64)party((int)params[1])->num_regular_members());
        break;
    case op::store_troop_kind_count:
        // deprecated: matches the main party's stacks against <troop_id> but adds the sizes of the stacks of the
        // party whose id is the troop id (original bug)
        {
            int count = 0, k = 0;
            PartyMgr* pm = &game()->parties;
            int mp = game()->main_party;
            if (((PartyX*)pm->get(mp))->num_stacks > 0) {
                do {
                    if ((__int64)((PartyX*)pm->get(mp))->stacks[k].troop == params[1])
                        count += ((PartyX*)pm->get((int)params[1]))->stacks[k].count;
                    ++k;
                } while (k < ((PartyX*)pm->get(mp))->num_stacks);
            }
            SET_LV((__int64)count);
        }
        break;
    case op::store_num_regular_prisoners:
        // the main party's non-hero prisoner stacks, sizes read from party <party_id> (original bug)
        {
            int count = 0, k = 0;
            PartyMgr* pm = &game()->parties;
            int mp = game()->main_party;
            if (((PartyX*)pm->get(mp))->num_stacks > 0) {
                do {
                    if ((((PartyX*)pm->get(mp))->stacks[k].flags & 1) &&
                        !troop(((PartyX*)pm->get(mp))->stacks[k].troop)->is_hero())
                        count += ((PartyX*)pm->get((int)params[1]))->stacks[k].count;
                    ++k;
                } while (k < ((PartyX*)pm->get(mp))->num_stacks);
            }
            SET_LV((__int64)count);
        }
        break;
    case op::store_troop_count_companions:
        // (store_troop_count_companions, <destination>, <troop_id>, [party_id])
        SET_LV((__int64)game()->count_troops((int)params[1], (int)params[2], 0));
        break;
    case op::store_troop_count_prisoners:
        SET_LV((__int64)game()->count_troops((int)params[1], (int)params[2], 1));
        break;
    case op::store_item_kind_count:
        // (store_item_kind_count, <destination>, <item_id>, [troop_id])
        SET_LV((__int64)game()->item_count((int)params[1], (int)params[2], 1));
        break;
    case op::store_free_inventory_capacity:
        SET_LV((__int64)game()->item_count(-1, (int)params[1], 0));
        break;
    case op::store_skill_level:
        // (store_skill_level, <destination>, <skill_id>, [troop_id])
        if (check_troop((int)params[2]))
            SET_LV((__int64)game()->troop_skill_level((int)params[2], (int)params[1]));
        break;
    case op::store_character_level:
        if (check_troop((int)params[1]))
            SET_LV((__int64)troop((int)params[1])->level);
        break;
    case op::store_attribute_level:
        if (check_troop((int)params[1]) && IN_RANGE(params[2], 0, 4))
            SET_LV((__int64)troop((int)params[1])->attributes[(int)params[2]]);
        break;
    case op::store_proficiency_level:
        if (check_troop((int)params[1]) && IN_RANGE(params[2], 0, 7))
            SET_LV((__int64)troop((int)params[1])->proficiency((int)params[2]));
        break;
    case op::store_troop_faction:
        if (check_troop((int)params[1]))
            SET_LV((__int64)troop((int)params[1])->faction);
        break;
    case op::store_troop_health:
        // (store_troop_health, <destination>, <troop_id>, [absolute]): else percent of the maximum
        if (check_troop((int)params[1])) {
            if (params[2])
                SET_LV((__int64)(int)troop((int)params[1])->hit_points);
            else {
                TroopX* t = troop((int)params[1]);
                float h = t->hit_points * 100.0f;
                SET_LV((__int64)(h / t->max_hit_points()));
            }
        }
        break;
    case op::store_troop_op_2180:
        // opcode 2180 (no header_operations name in 1.011): over the main party's companion stacks, heroes count
        // ceil(15% of their missing hit points), regular stacks their wounded
        {
            int total = 0, k = 0;
            if (party(game()->main_party)->num_stacks > 0) {
                do {
                    PartyMgr* pm = &game()->parties;
                    int mp = game()->main_party;
                    if (!(((PartyX*)pm->get(mp))->stacks[k].flags & 1)) {
                        TroopX* troops = game()->troops;
                        if (troops[((PartyX*)pm->get(mp))->stacks[k].troop].is_hero()) {
                            float mhp = (float)troops[((PartyX*)pm->get(mp))->stacks[k].troop].max_hit_points();
                            total += (int)ceil_float_comdat(
                                (mhp - troops[((PartyX*)pm->get(mp))->stacks[k].troop].hit_points) * 0.15f);
                        } else
                            total += ((PartyX*)pm->get(mp))->stacks[k].num_wounded;
                    }
                    ++k;
                } while (k < party(game()->main_party)->num_stacks);
            }
            SET_LV((__int64)total);
        }
        break;

    case op::store_relation:
        // (store_relation, <destination>, <faction_id_1>, <faction_id_2>): -100..100
        if (check_faction((int)params[1]) && check_faction((int)params[2])) {
            SET_LV((__int64)round_float(faction((int)params[1])->relations[(int)params[2]] * 100.0f));
        }
        break;
    case op::store_conversation_agent:
        // the dialog's agent for non-heroes; else the last agent of the conversation troop
        {
            int v = -1;
            if (!troop(g_conversation_troop)->is_hero() && g_conversation_agent != -1) {
                v = g_conversation_agent;
                SET_LV((__int64)v);
            } else {
                AgentTable* t = &mission()->agents;
                unsigned id = t->first_used();
                while (id < (unsigned)t->count) {
                    if (t->get(id)->troop == g_conversation_troop)
                        v = id;
                    t->next_used(&id);
                }
                SET_LV((__int64)v);
            }
        }
        break;
    case op::store_conversation_troop:
        SET_LV((__int64)g_conversation_troop);
        break;
    case op::store_partner_faction:
        SET_LV((__int64)troop(g_conversation_troop)->faction);
        break;
    case op::store_encountered_party:
        SET_LV((__int64)game()->encountered_party);
        break;
    case op::store_encountered_party2:
        SET_LV((__int64)game()->encountered_party2);
        break;
    case op::store_faction_of_party:
        // (store_faction_of_party, <destination>, <party_id>)
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->faction);
        break;
    case op::store_current_scene:
        SET_LV((__int64)mission()->current_scene);
        break;
    case op::store_item_value:
        if (check_item_kind((int)params[1]))
            SET_LV((__int64)item_kind((int)params[1])->value);
        break;
    case op::store_troop_value:
        if (check_troop((int)params[1]))
            SET_LV((__int64)game()->troop_value((int)params[1]));
        break;
    case op::store_partner_quest:
        // the last active quest given by the conversation troop, -1 if none
        {
            int v = -1;
            int n = game()->num_quests;
            for (int k = 0; k < n; ++k) {
                QuestRec* q = &game()->quests[k];
                if ((q->flags & 0x100) && (q->flags2 & 2) && q->giver_troop == g_conversation_troop)
                    v = k;
            }
            SET_LV((__int64)v);
        }
        break;
    case op::store_random_quest_in_range:
        SET_LV((__int64)game()->random_quest((int)params[1], (int)params[2]));
        break;
    case op::store_random_troop_to_raise:
        SET_LV((__int64)game()->random_troop((int)params[1], 0));
        break;
    case op::store_random_troop_to_capture:
        SET_LV((__int64)game()->random_troop((int)params[1], 1));
        break;
    case op::store_random_party_in_range:
        SET_LV((__int64)game()->random_party((int)params[1], (int)params[2]));
        break;
    case op::store01_random_parties_in_range:
        // two different random parties to reg0 and reg1 (at most 100000 tries for the second)
        {
            int a = game()->random_party((int)params[0], (int)params[1]);
            int b = a;
            int n = 0;
            while (n < 100000) {
                ++n;
                b = game()->random_party((int)params[0], (int)params[1]);
                if (b != a)
                    break;
            }
            g_script.reg[0] = a;
            g_script.reg[1] = b;
        }
        break;
    case op::store_random_horse:
        SET_LV((__int64)game()->random_item(1, -1, -1, -1));
        break;
    case op::store_random_equipment:
        SET_LV((__int64)game()->random_item(2, 3, 4, -1));
        break;
    case op::store_random_armor:
        SET_LV((__int64)game()->random_item(0xc, 0xd, 0xe, -1));
        break;
    case op::store_quest_number:
        // no quest id check
        SET_LV((__int64)quest((int)params[1])->number);
        break;
    case op::store_quest_item:
        SET_LV((__int64)quest((int)params[1])->item);
        break;
    case op::store_quest_troop:
        if (check_quest((int)params[1]))
            SET_LV((__int64)quest((int)params[1])->troop);
        break;
    case op::store_current_hours:
        SET_LV((__int64)game()->current_hours);
        break;
    case op::store_time_of_day:
        SET_LV((__int64)((int)game()->clock.elapsed() % 24));
        break;
    case op::store_current_day:
        SET_LV((__int64)game()->note_time);
        break;
    case op::is_currently_night:
        result = game()->is_night();
        break;

    case op::store_distance_to_party_from_party:
        // (store_distance_to_party_from_party, <destination>, <party_id>, <party_id>): map distance, rounded
        if (check_party((int)params[1]) && check_party((int)params[2])) {
            PartyMgr* pm = &game()->parties;
            const rgl::vector4& b = ((PartyX*)pm->get((int)params[2]))->pos;
            SET_LV((__int64)round_float(rgl::length(((PartyX*)pm->get((int)params[1]))->pos - b)));
        }
        break;
    case op::get_party_ai_behavior:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->f8e4);
        break;
    case op::get_party_ai_object:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->ai_target_object);
        break;
    case op::party_get_ai_target_position:
        if (check_register((int)params[0]) && check_party((int)params[1])) {
            Pos* dst = &script_pos((int)params[0]);
            *dst = identity_frame();
            PartyX* p = party((int)params[1]);
            dst->o.x = p->ai_target_x;
            dst->o.y = p->ai_target_y;
        }
        break;
    case op::get_party_ai_current_behavior:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->ai_behavior);
        break;
    case op::get_party_ai_current_object:
        if (check_party((int)params[1]))
            SET_LV((__int64)party((int)params[1])->ai_object);
        break;
    case op::store_num_parties_created:
        {
            int n = 0;
            if (check_party_template((int)params[1]))
                n = g_party_templates[(int)params[1]].num_created;
            SET_LV((__int64)n);
        }
        break;
    case op::store_num_parties_destroyed:
        {
            int n = 0;
            if (check_party_template((int)params[1]))
                n = g_party_templates[(int)params[1]].num_destroyed;
            SET_LV((__int64)n);
        }
        break;
    case op::store_num_parties_destroyed_by_player:
        {
            int n = 0;
            if (check_party_template((int)params[1]))
                n = g_party_templates[(int)params[1]].num_destroyed_by_player;
            SET_LV((__int64)n);
        }
        break;
    case op::store_num_parties_of_template:
        {
            int n = 0;
            PartyMgr* pm = &game()->parties;
            int id = pm->first_party_id();
            unsigned end = game()->parties.deflt;
            while ((unsigned)id < end) {
                if ((__int64)((PartyX*)pm->get(id))->template_id == params[1])
                    ++n;
                pm->next_party_id(&id);
            }
            SET_LV((__int64)n);
        }
        break;
    case op::store_random_party_of_template:
        // fails if no party exists with the template (two passes over the party table)
        {
            int n = 0;
            PartyMgr* pm = &game()->parties;
            int id = pm->first_party_id();
            unsigned end = game()->parties.deflt;
            while ((unsigned)id < end) {
                if ((__int64)((PartyX*)pm->get(id))->template_id == params[1])
                    ++n;
                pm->next_party_id(&id);
            }
            int found = -1;
            if (n > 0) {
                int pick = rand_below(&n);
                pm = &game()->parties;
                n = 0;
                id = pm->first_party_id();
                if (pick >= 0) {
                    end = game()->parties.deflt;
                    do {
                        if ((unsigned)id >= end)
                            break;
                        if ((__int64)((PartyX*)pm->get(id))->template_id == params[1]) {
                            if (n == pick)
                                found = id;
                            ++n;
                        }
                        pm->next_party_id(&id);
                    } while (n <= pick);
                }
            }
            result = found >= 0;
            SET_LV((__int64)found);
        }
        break;

    // ---- string registers ----------------------------------------------------------------------------------------
    case op::str_clear:
        if (check_register((int)params[0]))
            g_script.str[(int)params[0]].clear();
        break;
    case op::str_store_string:
        // (str_store_string, <string_register>, <string_id>)
        if (check_register((int)params[0]) && check_string_arg(types[1], (int)params[1]))
            g_script.str[(int)params[0]] = g_script.expand(g_strings.get((int)params[1], types[1]));
        break;
    case op::str_store_string_reg:
        // copies one string register to another (expanded)
        if (check_register((int)params[0]) && check_register((int)params[1]))
            g_script.str[(int)params[0]] = g_script.expand(g_script.str[(int)params[1]]);
        break;
    case op::str_store_troop_name:
        if (check_register((int)params[0]) && check_troop((int)params[1]))
            g_script.str[(int)params[0]] = *troop((int)params[1])->display_name();
        break;
    case op::str_store_troop_name_plural:
        if (check_register((int)params[0]) && check_troop((int)params[1]))
            g_script.str[(int)params[0]] = *troop((int)params[1])->plural_name();
        break;
    case op::str_store_troop_name_by_count:
        // (str_store_troop_name_by_count, <string_register>, <troop_id>, <number>): plural from 2 on
        if (check_register((int)params[0]) && check_troop((int)params[1])) {
            if (params[2] >= 2)
                g_script.str[(int)params[0]] = *troop((int)params[1])->plural_name();
            else
                g_script.str[(int)params[0]] = *troop((int)params[1])->display_name();
        }
        break;
    case op::str_store_item_name:
        if (check_register((int)params[0]) && check_item_kind((int)params[1]))
            g_script.str[(int)params[0]] = *item_kind((int)params[1])->display_name();
        break;
    case op::str_store_item_name_plural:
        if (check_register((int)params[0]) && check_item_kind((int)params[1]))
            g_script.str[(int)params[0]] = *item_kind((int)params[1])->plural_name();
        break;
    case op::str_store_item_name_by_count:
        if (check_register((int)params[0]) && check_item_kind((int)params[1])) {
            if (params[2] >= 2)
                g_script.str[(int)params[0]] = *item_kind((int)params[1])->plural_name();
            else
                g_script.str[(int)params[0]] = *item_kind((int)params[1])->display_name();
        }
        break;
    case op::str_store_party_name:
        if (check_register((int)params[0]) && check_party((int)params[1]))
            g_script.str[(int)params[0]] = *party((int)params[1])->display_name();
        break;
    case op::str_store_agent_name:
        // the troop name, else the (horse) item name
        if (check_register((int)params[0]) && warn_agent((int)params[1])) {
            AgentTable* t = &mission()->agents;
            int a = (int)params[1];
            if (t->get(a)->troop >= 0)
                g_script.str[(int)params[0]] = *troop(t->get(a)->troop)->display_name();
            else if (t->get(a)->f8c.item >= 0)
                g_script.str[(int)params[0]] = *item_kind(t->get(a)->f8c.item)->display_name();
        }
        break;
    case op::str_store_faction_name:
        if (check_register((int)params[0]) && check_faction((int)params[1]))
            g_script.str[(int)params[0]] = *faction((int)params[1])->display_name();
        break;
    case op::str_store_quest_name:
        if (check_register((int)params[0]) && check_quest((int)params[1]))
            g_script.str[(int)params[0]] = g_script.expand(*quest((int)params[1])->raw_text());
        break;
    case op::str_store_date:
        // (str_store_date, <string_register>, <number_of_days_to_add_to_the_current_date>)
        if (check_register((int)params[0]) && params[1] >= 0)
            g_script.str[(int)params[0]] = game()->format_time((int)params[1]);
        break;
    case op::str_store_troop_name_link:
        if (check_register((int)params[0]) && check_troop((int)params[1])) {
            char link[0x400];
            sprintf(link, "<trp=%d>%s</>", (int)params[1], CS(troop((int)params[1])->display_name()));
            g_script.str[(int)params[0]].assign(link);
        }
        break;
    case op::str_store_party_name_link:
        if (check_register((int)params[0]) && check_party((int)params[1])) {
            char link[0x400];
            sprintf(link, "<p=%d>%s</>", (int)params[1], CS(party((int)params[1])->display_name()));
            g_script.str[(int)params[0]].assign(link);
        }
        break;
    case op::str_store_faction_name_link:
        if (check_register((int)params[0]) && check_faction((int)params[1])) {
            char link[0x400];
            sprintf(link, "<fac=%d>%s</>", (int)params[1], CS(faction((int)params[1])->display_name()));
            g_script.str[(int)params[0]].assign(link);
        }
        break;
    case op::str_store_quest_name_link:
        if (check_register((int)params[0]) && check_quest((int)params[1])) {
            char link[0x400];
            int q = (int)params[1];
            sprintf(link, "<qst=%d>%s</>", q, CS(&quest(q)->text));
            g_script.str[(int)params[0]].assign(link);
        }
        break;

    // ---- mission counters ----------------------------------------------------------------------------------------
    case op::store_remaining_team_no:
        SET_LV((__int64)mission()->last_team());
        break;
    case op::store_mission_timer_a:
        {
            SET_LV(round_i64(mission()->timer_a.elapsed()));
        }
        break;
    case op::store_mission_timer_b:
        {
            SET_LV(round_i64(mission()->timer_b.elapsed()));
        }
        break;
    case op::store_mission_timer_c:
        {
            SET_LV(round_i64(mission()->timer_c.elapsed()));
        }
        break;
    case op::reset_mission_timer_a:
        mission()->timer_a.restart();
        break;
    case op::reset_mission_timer_b:
        mission()->timer_b.restart();
        break;
    case op::reset_mission_timer_c:
        mission()->timer_c.restart();
        break;
    case op::store_enemy_count:
        SET_LV((__int64)mission()->count_agents(1));
        break;
    case op::store_friend_count:
        SET_LV((__int64)mission()->count_agents(0));
        break;
    case op::store_ally_count:
        SET_LV(0);
        break;
    case op::store_defender_count:
        // mode 1: team 1; mode 0: team 0; else team 0 when the player attacks, team 1 otherwise
        {
            MissionX* m = mission();
            if (m->mode == 1)
                SET_LV((__int64)m->count_agents(1));
            else if (m->mode == 0)
                SET_LV((__int64)m->count_agents(0));
            else if (g_siege_attack)
                SET_LV((__int64)m->count_agents(0));
            else
                SET_LV((__int64)m->count_agents(1));
        }
        break;
    case op::store_attacker_count:
        {
            MissionX* m = mission();
            if (m->mode == 1)
                SET_LV((__int64)m->count_agents(0));
            else if (m->mode == 0)
                SET_LV((__int64)m->count_agents(1));
            else if (g_siege_attack)
                SET_LV((__int64)m->count_agents(1));
            else
                SET_LV((__int64)m->count_agents(0));
        }
        break;
    case op::store_normalized_team_count:
        // (store_normalized_team_count, <destination>, <team_no>)
        if (IN_RANGE(params[1], 0, 8))
            SET_LV((__int64)mission()->count_agents((int)params[1]));
        else
            SET_LV(0);
        break;

    default:
        sprintf(buf, "Unrecognized opcode %d.", id);
        report(buf);
        break;
    }

    if (neg)
        result = !result;
    if (result) {
        if (or_next)
            g_or_latch = 1;
    } else if (or_next)
        result = 1;
    *error = g_script_error;
    return result;
}
