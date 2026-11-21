// script_interpreter.h: the Module System operation interpreter (Operation::execute, 0x49fa10) and every class,
// global and callee it uses. Game code. Names provisional unless they are Module System names
// (header_operations.py opcodes, header_*.py flags). Every offset below is read or written by the interpreter
// at the address cited (an address inside 0x49fa10..0x4ae458), unless another function is named.
//
// Reused headers (not redefined): src/game/tstring.h (tstring), match/include/rgl.h + src/game/mat4.h
// (vector4, matrix, mat4), src/game/dialog_manager.h (stream_base, GameData/g_game, PartyMgr, Party, TroopRec,
// ScriptState/g_script, Elem = the 0x98-byte operation record, DialogMgr), src/game/agent/agent.h (ItemRef,
// Troop, Mission/g_mission, Agent, round_float, clamp_index, rand_below ...). What those headers lack is added
// by derived "view" classes (suffix X: no new fields, only inline accessors at the raw offsets this function
// reads, see FLD) or, for records no src/ header defines yet, by plain structs with padded fields.
#ifndef GAME_SCRIPT_INTERPRETER_H
#define GAME_SCRIPT_INTERPRETER_H
#include <stdio.h>
#include "../../include/layout32.h"
#include <stddef.h>
#include <string.h>
#include <deque>
#include "../../include/fx.h"          // FxDef: a trigger entity's effect (elem_add_psys / elem_set_data)
#include "../dialog_manager.h"
#include "../agent/agent.h"
#include "./script_opcodes.h"
#include "../../include/vec.h"                  // Vec2
#include "../strat/gui_manager.h"             // MsgLog: the on-screen message log (add 0x452c00, push 0x4334b0)
#include "../../include/sound_mgr.h"           // SoundMgr: play 0x442650
#include "../ui_globals.h"      // UI / core globals


// ---- callees declared under the class that defines them ---------------------------------------------------------------------
// The interpreter reaches these functions through its own view classes (MissionX, Presentation, ... below), which keep the
// interpreter's names; each view method forwards (inline, __forceinline: nothing is emitted for it) to the function of the class that
// DEFINES that address in the sources, declared here with that class's name and signature. The modules that define them name the
// class after another of their users, and the linker folds identical bodies, so the view object is the object the definition works
// on (cast at the forwarder). The namespaces cNNNNNN are the namespaces script_interpreter.cpp wraps its local classes in.
struct mesh_part;
// (GameData's world_data.cpp operations are declared on GameData, include/game_data.h)
struct MissionW {                                                            // world_data.cpp
    void set_weapon_order_1(int team, unsigned value);                      // 0x492230 (ret 8)
    void set_weapon_order_2(int team, unsigned value);                      // 0x492350 (ret 8)
    bool all_defeated(int team);                                            // 0x4920e0
    bool hero_down();                                                       // 0x492170
    int count_agents(int team);                                             // 0x491f50
    unsigned find_agent_by_troop(int troop);                                // 0x4921b0
    void add_wind_effect(float a1, float a2);                                   // 0x4952f0
};
#include "../ui/mission.h"                  // MissionView (setup 0x4897a0, set 0x489820)
#include "../ui/world.h"                    // Named16 (set 0x489ab0)
#include "../ui/mission_ui.h"
#include "../slot.h"                                // Slot, g_keyboard (0xa170c8)
struct TimeKey { void calc(); };                                             // 0x487620 (slot.cpp)
#include "../../rgl/core/resmgr.h"                         // lod_group::set_material 0x48cfc0 (mission_ui2/mission_3.cpp)
struct mesh_inst { void set_part(mesh_part* p); };                           // 0x4620b0 (renderer.cpp)
struct vec2x { void rotate_b(float ang); };                                  // 0x4327f0 (rgl/xmlcfg/game_util.cpp)
struct AgentX { int agent_class() const; };   // 0x433140 (rgl/xmlcfg/game_util.cpp)

struct string_view_ : tstring { const char* get() const; };                  // 0x407c10 (rgl/shapes/shapes.cpp: the same class): the out-of-line c_str
#include "../presentation.h"                                                 // widget_mgr: the presentation (g_presentation) the overlay operations work on
namespace c49f060 {
void __cdecl spawn_party_scene(int a, int b, int* pos, int* anim, int* instance);   // 0x49f3d0
}
// a float handed to a callee that the definition's module declared as an int word (the bits are what the callee gets)
inline int fbits(float f) { return *(int*)&f; }

// raw field of a view class: FLD(type, offset)
#define FLD(T, off) (*(T*)((char*)this + (off)))
// compile-time layout check (VC7.1 has no static_assert)
#define SI_ASSERT(name, cond) MB_LAYOUT32_ASSERT(si_assert_##name, (cond))

// ---- the interpreter's own globals ----------------------------------------------------------------------------
#include "./script_globals.h"   // g_cur_param, g_lineno, g_script_error, g_cur_opcode, g_or_latch (defined in script_interpreter.cpp)

// ---- counters and small tables (bounds of the validators) -----------------------------------------------------
#include "../../include/prsnt_table.h"   // g_presentations (0xa75f2c): .count is the bound of check_presentation 0x49cd50
#include "../../include/mesh_table.h"       // g_meshes (0x8ed4f58): .count is the bound of check_mesh
extern int g_menu_count;                // 0x5e29b4 (check_menu 0x49ce90)
// mission templates (0x58-byte records at 0x5e2a00) and their 0x4c-byte entry points
#include "../mode_rec.h"             // MissionTemplate (the 0x58-byte records at 0x5e2a00) and SpawnRec (its 0x4c-byte entries): the one definition
extern MissionTemplate* g_mission_templates;   // 0x5e2a00
extern int g_party_template_count;      // 0x5e29fc (0x49d0c0)

#define g_scene_prop_count g_propkind_count    // 0x5e29e0: agent.h's 0x78-byte record table is the scene prop kinds

#include "../../include/anim_set.h"       // g_anim_set (0xa76628): .count is the bound of check_animation 0x49d4b0

// ---- message log and text log (report 0x49cc60) -----------------------------------------------------------------
// the message log object (MsgLog, src/include/msg_log.h)

// ---- conversations and the window manager -------------------------------------------------------------------------
#include "../dialog_mgr.h"                  // DialogMgr g_dialogs (0x8ed50c8)
#include "../window_mgr.h"             // g_windows (0xa73980): box() is the dialog box (tutorial_box 0x4a3baa), clear_all 0x489be0 (change_screen_quit 0x4ac332)

// ---- translated UI strings (object 0x5e2a18; StrTable in dialog_manager.h) ------------------------------------------
#define g_ui_strings g_strtable       // StrTable::lookup(const tstring&) 0x43a1e0 (ret 4: the translation, or the key)
// tstring.h's c_str() is inline; the interpreter calls the out-of-line copy (rgl::string::c_str, 0x407c10)
struct tstr : tstring {
    __forceinline const char* get() const { return ((const string_view_*)this)->get(); }   // 0x407c10
};
inline const char* CS(const tstring* s) { return ((const tstr*)s)->get(); }
// the translated text of a UI string id
#define UI(id) CS(g_ui_strings.lookup(tstring(id)))

// ---- string tables (object 0xa75ea0) ----------------------------------------------------------------------------
#include "../string_table.h"           // StringTable (0xa75ea0): the one definition (script and inventory_panel/module_files)

// ---- input ---------------------------------------------------------------------------------------------------------
#include "../key_names.h"             // Keyboard g_keyboard (0xa170c8)
#include "../game_keys.h"            // GameKeys g_game_keys (0xa73730)

// the window/screen stack is g_windows.dq (a deque of screen ids; 0x10 = conversation)

// ---- vectors and frames ----------------------------------------------------------------------------------------------
// the interpreter's helper over the rgl vector type (0x4327f0 rotates x / y about the z axis)
__forceinline void rotate_b(rgl::vector4& v, float angle) { ((vec2x*)&v)->rotate_b(angle); }
__forceinline rgl::mat3 identity3() { rgl::mat3 r; identity3((rgl::mat3*)&r); return r; }      // (the interpreter's by-value spelling)
__forceinline rgl::matrix identity_frame() { rgl::matrix p; identity_pos((pos*)&p); return p; }    // (the interpreter's by-value spelling; item_kind.h: identity_pos)
float __cdecl clamp(const float* v, const float* lo, const float* hi);    // 0x40e3d0
// by-reference spellings of the clamp helpers: the helpers take pointers (agent.h / game_util.h declare them so; the original passes
// temporaries), so these forward to the pointer declarations
int __cdecl rand_below(const int* n);                           // 0x432580 (game_util.h; 0 when *n < 2, else rand() % *n)
int __cdecl imax_ref(const int* a, const int* b);                // 0x4324f0 (game_util.h)
float __cdecl fmin_ref(const float* a, const float* b);         // 0x4324b0 (game_util.h)
__int64 __cdecl round_i64(const float* v);                      // 0x4875a0 (floor, +1 above .5; the definition in mesh_start.cpp declares it int, but it returns edx:eax: adc edx, 0)
__forceinline int clamp_index(const int& v, const int& lo, const int& hi) { return clamp_index(&v, &lo, &hi); }   // 0x432510 (result in [lo, hi - 1])
__forceinline int round_float(const float& f) { return round_float((float*)&f); }                                  // 0x432540 (agent.h: round_float(float*))
__forceinline __int64 round_i64(const float& v) { return round_i64(&v); }                                         // 0x4875a0
__forceinline int rand_below(const int& n) { return rand_below(&n); }                                              // 0x432580
__forceinline int imax_ref(const int& a, const int& b) { return imax_ref(&a, &b); }                                // 0x4324f0
__forceinline float fmin_ref(const float& a, const float& b) { return fmin_ref(&a, &b); }                          // 0x4324b0
// the out-of-line copy of <math.h>'s inline float ceil(float) the linker took from d3dfont.obj
double __cdecl ceil_float_comdat(float x);                     // 0x404590 (src/dxframework/d3dfont.cpp)

// ---- position registers: g_script.pos, 128 frames of 0x40 bytes (rgl::matrix) ---
__forceinline rgl::matrix& script_pos(int i) { return (rgl::matrix&)g_script.pos[i]; }


// std::vector<int> (VC7.1: allocator word, then first / last / end)
#include "../int_vec.h"                 // IntVec (was IntVector here: size() 0x40f010)
#include <vector>
template<> size_t std::vector<int, std::allocator<int> >::size() const;     // 0x40f010 (rgl/particles/mesh_obj.cpp: the out-of-line instance; only declared)

// ---- tableaus (g_tableaus 0xa75f40, include/tableau_table.h) ----------------------------------------------------
#include "../../include/tableau_table.h"
__forceinline void tableau_add_agent(int troop, int horse_item, const rgl::matrix* p, const int* animation, const int* instance) { c49f060::spawn_party_scene(troop, horse_item, (int*)p, (int*)animation, (int*)instance); }   // 0x49f3d0
// the item / scene object of a trigger context (ti_on_init_item -50, scene prop -40, map icon -70)
struct TriggerItem {
    __forceinline void set_owner(void* m) { ((rgl::lod_mesh*)this)->set_material((skin*)m); } // 0x48cfc0 (ret 4)
};
struct TriggerMeshObj {
    __forceinline void set_part(void* m) { ((mesh_inst*)this)->set_part((mesh_part*)m); } // 0x4620b0 (ret 4)
};
rgl::lod_mesh* __fastcall trigger_item(int context);                                  // 0x48cec0 (mission_ui2/mission_3.cpp: returns the item object's address)
rgl::mesh* __fastcall trigger_mesh_obj(int context);                              // 0x48cf40 (mission_3.cpp: the mesh object's address)


// ---- sound and music -------------------------------------------------------------------------------------------------
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128: play 0x442650, play_ui 0x4942f0, stop_all 0x494310



// the scene entity of the object a trigger runs for (scene prop / agent triggers; 0 otherwise); +0x88 is its position
void* __fastcall trigger_entity(int context);                     // 0x490430

// ---- presentations (the running presentation, g_presentation 0x5e299c; 0 when none: presentation.h) ------------------
// the overlay operations take a vec2 (size, position): the script's position registers hold a vector4, of which x, y
inline vec2 xy(const rgl::vector4& v) { vec2 r = { v.x, v.y }; return r; }

// ---- Module System constants ---------------------------------------------------------------------------------------
enum {  // header_parties.py ai behaviours (party +0x918)
    ai_bhvr_hold = 0, ai_bhvr_travel_to_party, ai_bhvr_patrol_location, ai_bhvr_patrol_party,
    ai_bhvr_attack_party, ai_bhvr_avoid_party, ai_bhvr_travel_to_point, ai_bhvr_negotiate_party,
    ai_bhvr_in_town, ai_bhvr_travel_to_ship, ai_bhvr_escort_party, ai_bhvr_driven_by_party
};
enum {  // header_troops.py / header_items.py
    tf_guarantee_horse = 0x01000000, tf_guarantee_ranged = 0x04000000,
    itp_unique = 0x1000, itp_always_loot = 0x2000,
    ek_item_0 = 0, ek_head = 4, ek_body, ek_foot, ek_gloves, ek_horse, ek_food
};

// ---- slot arrays (troop / party / faction / scene / party template / agent / quest / item) -------------------------
#include "../slot_array.h"

#include "../../include/timer.h"

// ---- parties (0x5ac8 bytes; pages at PartyMgr +0x138d0) ----------------------------------------------------------
// the 0x20-byte troop stacks are Stack of include/party.h (clear 0x4893e0 declared there)
#include "../party_x.h"                // PartyX : Party (aliases in party.h, member names of the interpreter): the one definition (map_editor and script)
SI_ASSERT(party_size, sizeof(Party) == 0x5ac8);
SI_ASSERT(party_stacks, offsetof(Party, stacks) == 0xa8);
SI_ASSERT(party_ai, offsetof(Party, ai_behavior) == 0x918);
SI_ASSERT(party_notes, offsetof(Party, notes) == 0x5a08);
// PartyMgrX: the members of the party table (PartyMgr in dialog_manager.h) the interpreter adds
// PartyMgr::is_live 0x4903e0 of include/party.h replaces the PartyMgrX view

// ---- troops (0x508 bytes, array at game +0x13a00) ----------------------------------------------------------------
void __fastcall clear_slots(ItemRef* items);  // 0x489060 (the 0x60 inventory entries of a troop)
// ItemRef (src/game/item_ref.h) carries the script editors set_item / set_modifier / randomize_modifier
inline ItemRef& IR(ItemRef& r) { return r; }
float frand();                                      // 0x40e400 (0..1)
int __cdecl rand_below(const int* n);               // 0x432580 (0 when *n < 2, else rand() % *n)
float __cdecl fmin_ref(const float* a, const float* b);     // 0x4324b0 (game_util.h)
SI_ASSERT(troop_size, sizeof(TroopRec) == 0x508);
SI_ASSERT(troop_equipment, offsetof(TroopRec, equipment) == 0x4a4);
SI_ASSERT(troop_notes, offsetof(TroopRec, notes) == 0x400);

// ---- quests (0x138 bytes, array at game +0x13a58) ----------------------------------------------------------------
#include "../../include/game_recs.h"   // QuestRec / FactionRec / PartyTemplate: the shared record classes (one definition for src/game and src/rgl)
SI_ASSERT(quest_size, sizeof(QuestRec) == 0x138);
SI_ASSERT(quest_notes, offsetof(QuestRec, notes) == 0x68);

// ---- factions (0x300 bytes, game +0x13a48), scenes (0x494, game +0x13a50), party templates (0xb0, 0x5e29f8) -------
SI_ASSERT(faction_size, sizeof(FactionRec) == 0x300);
SI_ASSERT(faction_notes, offsetof(FactionRec, notes) == 0x25c);
#include "../terrain_b/scene_file.h"   // SceneFile: the 0x494-byte scene record (visitors +0x114, visitor_counts +0x314, slots +0x474)
#include "../../rgl/stream_inline.h"   // the stream constructors / destructors inline (the scene record's embedded streams)
SI_ASSERT(scene_size, sizeof(SceneFile) == 0x494);
SI_ASSERT(ptemplate_size, sizeof(PartyTemplate) == 0xb0);
#include "../../include/item_kind.h"   // ItemKind::display_name (0x43a480)
SI_ASSERT(itemkind_size, sizeof(ItemKind) == 0x238);
__forceinline ItemKind* item_kind(int id) { return &g_items[id]; }

extern int g_visitor_scene;             // 0x5e2594 (modify_visitors_at_site 0x4a4b17)
extern float g_spawn_radius;            // 0x5a9c18 (set_spawn_radius 0x4a2e0c)


// ---- map battles (table at game +0xcc) ---------------------------------------------------------------------------
#include "../event.h"                 // Event: the battle record (bind_host 0x4b2770)

// ---- the game object (g_game, 0x5e2930; GameData in dialog_manager.h) --------------------------------------------
// the game object is GameData (include/game_data.h): g_game
SI_ASSERT(game_parties, sizeof(PartyMgr) == 0x139dc - 0xf8);   // then GameData::wind / f139e0
__forceinline Party* party(int id) { return g_game->parties.get(id); }
inline rgl::vector4& party_pos(Party* p) { return *(rgl::vector4*)&p->pos; }       // Party::pos (+0x30) is a plain 4-float record; the interpreter uses it as a vector4
__forceinline TroopRec* troop(int id) { return &g_game->chars[id]; }
__forceinline QuestRec* quest(int id) { return &g_game->quests[id]; }
__forceinline FactionRec* faction(int id) { return &g_game->factions[id]; }
__forceinline SceneFile* scene(int id) { return &g_game->scenes[id]; }

#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300)

#include "../map_flags.h"   // g_map_flags.f8 = 0x5e2924 (jump_to_menu 0x4ac3a0)
struct MeshRec { tstring name; char pad10[0x48 - 0x10]; };   // a 0x48-byte record of g_meshes.records (set_background_mesh)



// ---- the mission (g_mission, 0x5e292c; Mission in agent.h) --------------------------------------------------------
// The agent record is Agent (src/game/agent_obj.h; 0x5928 bytes, pages of 16): the slices' AgentX.
#include "../blow.h"                // Blow (agent_deliver_damage_to_agent fills it; the one definition, shared with agent4)
SI_ASSERT(agent_size, sizeof(Agent) == 0x5928);
#include "../prop_vec.h"      // PropVec: the one definition (agent4, map_editor, script)
namespace std { template<> PlacedObj& vector<PlacedObj, allocator<PlacedObj> >::operator[](size_t); }   // 0x438c10 (agent2/agent_stats.cpp: the out-of-line instance)
#define prop_at(v, id) (&(v)[id])   // the scene prop instance (PlacedObj, game/placed_obj.h); operator[] stays out of line (thiscall, ret 4)
// the scene object at mission +0 is `world` (src/game/world.h; Level in agent.h)
struct EntryPoint { rgl::matrix pos; char pad40[0x130 - 0x40]; };      // mission +0x670 (entry_point_get_position 0x4a9ab0)
// TeamRec (0x16c bytes at mission +0x527c): src/game/mission.h
// a point light added by add_point_light (0x7c bytes)
#include "../light.h"                   // Light (shared with agent2 / strat)
// the trigger's scene entity (prop instance or agent body, trigger_entity 0x490430): virtual slots 25 / 27
#include "../emit.h"                 // EmitTable, g_emit_defs (0x8ed5090): the particle-system table (terrain_b/mission.cpp)

struct MTimer {                         // clock index + start (Timer in src/rgl/xmlcfg/game_util.h)
    int clock;
    int pad4;
    __int64 start;
    __forceinline void restart() { ((Timer*)this)->restart(); }                 // 0x432bf0
    __forceinline float elapsed() const { return ((const Timer*)this)->elapsed(); }   // 0x432c10 (seconds)
};
// the script's Mission (../mission.h): the interpreter's names for Mission's functions and those of MissionW (world_data.cpp)
struct MissionX : Mission {
    // the interpreter's names for the functions of Mission (src/game/mission.h), MissionW (world_data.cpp) and the
    // camera.cpp classes that define these addresses; each forwards (inline) to the definition's class
    __forceinline char all_defeated(int team) { return ((MissionW*)this)->all_defeated(team); }                      // 0x4920e0 (thiscall, ret 4)
    __forceinline void clear_done() { ((Mission*)this)->clear_done(); }                                              // 0x4676c0
    __forceinline void add_amount(int entry, int count) { ((Mission*)this)->add_amount(entry, (unsigned)count); }              // 0x4677a0 (ret 8)
    __forceinline int team_count() { return ((Mission*)this)->team_count(); }                                         // 0x48a8b0
    __forceinline char hero_down() { return ((MissionW*)this)->hero_down(); }                                         // 0x492170
    __forceinline int last_team() { return ((Mission*)this)->last_team(); }                                           // 0x48a910 (ui/mission.h)
    __forceinline int count_agents(int team) { return ((MissionW*)this)->count_agents(team); }                        // 0x491f50 (ret 4)
    __forceinline void select_team(int team) { ((Mission*)this)->select_team(team); }                                 // 0x48a9e0 (ret 4)
    __forceinline void select_class(int team, int sub_class) { ((Mission*)this)->select_class(team, sub_class); }     // 0x48aa60 (ret 8)
    __forceinline void unselect_team(int team) { ((Mission*)this)->unselect_team(team); }                             // 0x48aa20 (ret 4)
    __forceinline int movement_order(int team) { return ((Mission*)this)->movement_order(team); }                     // 0x48aac0 (ret 4)
    __forceinline int riding_order(int team) { return ((Mission*)this)->riding_order(team); }                         // 0x48ab30 (ret 4)
    __forceinline int weapon_usage_order(int team) { return ((Mission*)this)->weapon_usage_order(team); }             // 0x48ab90 (ret 4)
    __forceinline int hold_fire_order(int team) { return ((Mission*)this)->hold_fire_order(team); }                   // 0x48abf0 (ret 4)
    void order(int team, int order);                  // 0x49c2c0 (ret 8)
    __forceinline char point_in_rect(const rgl::vector4* p) { return ((Mission*)this)->point_in_rect((const float*)p); }   // 0x48a990 (ret 4; ui/mission.h)
    __forceinline int count_props(int prop) { return ((Mission*)this)->count_props(prop); }                           // 0x48ee20 (ret 4)
    __forceinline int find_prop(int prop, int n) { return ((Mission*)this)->find_prop(prop, n); }                     // 0x48ee90 (ret 8: -1 = none)
    __forceinline void retag_props(int old_prop, int new_prop) { ((Mission*)this)->retag_props(old_prop, new_prop); } // 0x48ed60 (ret 8)
    __forceinline void start_clock(int a1) { ((Mission*)this)->start_clock(a1); }                                     // 0x48a860 (ret 4)
    __forceinline float blend_param() { return ((Mission*)this)->blend_param(); }                                     // 0x48a740 (blend_param)
    __forceinline float blend2_param() { return ((Mission*)this)->blend2_param(); }                                   // 0x48a7d0 (blend2_param)
    __forceinline int find_agent_by_troop(int troop) { return ((MissionW*)this)->find_agent_by_troop(troop); }        // 0x4921b0 (ret 4)
    __forceinline void add_wind_effect(float flicker_magnitude, float flicker_interval) { ((MissionW*)this)->add_wind_effect(flicker_magnitude, flicker_interval); }   // 0x4952f0 (ret 8)
    __forceinline void spawn_effect(int psys, const rgl::matrix* p, int strength) { ((Mission*)this)->spawn_effect(psys, (const rgl::mat4*)p, strength); }   // 0x442ca0 (ret 0xc)
    __forceinline int place(const ItemRef* item) { return ((Mission*)this)->place((const int*)item); }   // 0x49b4b0 (ret 4)
    __forceinline int exec_trigger(int troop) { return ((Mission*)this)->exec_trigger(troop); }          // 0x49f9e0 (ret 4)
    __forceinline int enter_trigger(int item, int modifier) { return ((Mission*)this)->enter_trigger(item, modifier); }   // 0x49f980 (ret 8)
};
SI_ASSERT(prop_scale, offsetof(PlacedObj, scale) == 0x158);
// the 0x4c-byte environment rows (EnvRow in src/game/terrain_b/emit.h)
#include "../env_row.h"                  // EnvRow, g_env_rows (0x5e28f4; set_fog_distance 0x4aa230)
__forceinline MissionX* mission() { return (MissionX*)g_mission; }
__forceinline Agent* agent(int id) { return mission()->agents.get(id); }

// ---- the operation record ---------------------------------------------------------------------------------------
// Elem (dialog_manager.h): +0 opcode word (flags neg / this_or_next in the top bits), +8 operand count, +0x10 the
// 16 raw 64-bit operands, +0x90 skip target. The interpreter is a member of the record.
struct Operation : Elem {
    // get_operand 0x49d650 (the value of operand *i: plain constant, register, global variable or local variable; type out) and
    // set_lvalue 0x49d500 (store *value into the lvalue operand 0) are Elem's (include/dialog_rec.h)
    // run one operation; returns the condition result; *error = 1 when a script error was reported
    char execute(__int64* locals, int* context, int* error);       // 0x49fa10 (thiscall, ret 0xc)
};

#endif // GAME_SCRIPT_INTERPRETER_H
