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
#include <stddef.h>
#include <string.h>
#include "../dialog_manager.h"
#include "../agent/agent.h"
#include "./script_opcodes.h"

// raw field of a view class: FLD(type, offset)
#define FLD(T, off) (*(T*)((char*)this + (off)))
// compile-time layout check (VC7.1 has no static_assert)
#define SI_ASSERT(name, cond) typedef char si_assert_##name[(cond) ? 1 : -1]

// ---- the interpreter's own globals ----------------------------------------------------------------------------
extern int g_lineno;            // 0x5e2598  line of the statement being run (report)
extern int g_script_error;      // 0x5e259c  set by report(); copied to *error at the end of execute
extern int g_cur_opcode;        // 0x5e25a0  opcode word of the statement being run (validators print it)
extern int g_or_latch;          // 0x5e25a4  this_or_next chain already satisfied: skip until the chain ends

// ---- counters and small tables (bounds of the validators) -----------------------------------------------------
extern int g_presentation_count;        // 0xa75f30 (check_presentation 0x49cd50)
extern int g_mesh_count;                // 0x8ed4f5c (check_mesh)
extern int g_menu_count;                // 0x5e29b4 (check_menu 0x49ce90)
extern int g_mission_template_count;    // 0x5e2a04 (0x49d070)
// mission templates (0x58-byte records at 0x5e2a00) and their 0x4c-byte entry points
#include "../mode_rec.h"             // MissionTemplate (the 0x58-byte records at 0x5e2a00) and SpawnRec (its 0x4c-byte entries): the one definition
extern MissionTemplate* g_mission_templates;   // 0x5e2a00
extern int g_party_template_count;      // 0x5e29fc (0x49d0c0)
extern int g_map_icon_count;            // 0x5e29f4 (0x49d110)
extern int g_particle_system_count;     // 0x8ed5094 (warn_particle_system)
#define g_scene_prop_count g_tbl78_count    // 0x5e29e0: agent.h's 0x78-byte record table is the scene prop kinds
extern int g_sound_count;               // 0x5aa130 (0x49d310)
extern int g_track_count;               // 0xa73a14 (0x49d3d0)
extern int g_animation_count;           // 0xa7662c (0x49d4b0)
extern int g_warnings_off_a;            // 0x5a9fd4 (0x49d3d0)
extern int g_warnings_off_b;            // 0x5a9fd8 (0x49d310, 0x49d3d0)

// ---- message log and text log (report 0x49cc60) -----------------------------------------------------------------
// MsgLogX is a view of the message log object (MsgLog in src/game/agent3/agent_fx_sound.h / src/rgl/xmlcfg/game_util.h)
struct MsgLogX {
    void add(tstring msg, unsigned color);                      // 0x452c00 (thiscall, ret 0x14: text by value)
    void push(const tstring& s, int v);                         // 0x4334b0
    void tutorial_message(const tstring& text, int color);      // 0x49b630 (ret 8)
};
extern MsgLogX g_msglog;                // 0x5d7080
extern char g_log_obj[];                // 0x5a9f60 the text log stream (a stream_base; slot 10 = w_cstr)

// ---- conversations and the window manager -------------------------------------------------------------------------
// DialogMgrX: DialogMgr (dialog_manager.h) members it does not declare
struct DialogMgrX : DialogMgr {
    tstring current_text();             // 0x489c30 (copy of cur_text, +0xe028)
};
extern DialogMgrX g_dialogs;            // 0x8ed50c8
struct DialogBox {
    void show(const tstring& title, const tstring& text);                   // 0x48a3a0 (ret 8)
    void ask(const tstring& text, const tstring& yes, const tstring& no, int* answer);   // 0x48a3d0 (ret 0x10)
};
struct WindowMgr {
    DialogBox* dialog_box;              // +0 window 0 (tutorial_box 0x4a3baa)
    void open(int window);              // 0x494260 (thiscall, ret 4)
    void close_all();                   // 0x489be0 (change_screen_quit 0x4ac332)
    void set_state(int state);          // 0x495700 (ret 4)
};
extern WindowMgr g_windows;             // 0xa73980

// ---- translated UI strings (object 0x5e2a18; StrTable in dialog_manager.h) ------------------------------------------
struct StrTableX : StrTable {
    tstring* lookup(const tstring* key);    // 0x43a1e0 (ret 4: the translation, or the key)
};
#define g_ui_strings (*(StrTableX*)&g_strtable)
// tstring.h's c_str() is inline; the interpreter calls the out-of-line copy (rgl::string::c_str, 0x407c10)
struct tstr : tstring {
    const char* c_str_() const;             // 0x407c10
};
inline const char* CS(const tstring* s) { return ((const tstr*)s)->c_str_(); }
// the translated text of a UI string id
#define UI(id) CS(g_ui_strings.lookup(&tstring(id)))

// ---- string tables (object 0xa75ea0) ----------------------------------------------------------------------------
#include "../string_table.h"           // StringTable (0xa75ea0): the one definition (script and inventory_panel/module_files)
extern tstring g_result_string;         // 0xa70464 (set_result_string 0x49fce6)

// ---- input ---------------------------------------------------------------------------------------------------------
struct Keyboard {
    char pad0[0xc];
    int down[256];                      // +0xc (key_is_down 0x49fd8c)
    char clicked(int key);              // 0x487900 (thiscall, ret 4)
};
extern Keyboard g_keyboard;             // 0xa170c8
struct GameKeys {
    char is_down(int key);              // 0x48ac50 (thiscall, ret 4)
    char clicked(int key);              // 0x48ac90
};
extern GameKeys g_game_keys;            // 0xa73730
extern float g_mouse_x;                 // 0x5d7094 (mouse_get_position 0x49fe7f)
extern float g_mouse_y;                 // 0x5d7098

// the window/screen stack (a deque of screen ids; 0x10 = conversation)
struct ScreenStack {
    int& back();                        // 0x490310 (thiscall)
};
extern ScreenStack g_screen_stack;      // 0xa739f8

// ---- vectors and frames ----------------------------------------------------------------------------------------------
// vec4: the vector4 members rgl.h does not declare
struct vec4 : rgl::vector4 {
    vec4& operator=(const rgl::vector4& v) { *(rgl::vector4*)this = v; return *this; }
    vec4& set(const float& x_, const float& y_, const float& z_);   // 0x407c30 (rgl_math.h)
    vec4 operator*(const float& s) const;                       // 0x407cd0 (hidden return)
    void operator+=(const rgl::vector4& o);                     // 0x40e420 (thiscall, ret 4)
    void rotate_z(float angle);                                 // 0x4327f0 (x / y about the z axis)
};
inline vec4& V(rgl::vector4& v) { return (vec4&)v; }
// Pos: a frame (Module System `pos`: rows s(ide) f(orward) u(p), origin o), with the members the interpreter calls
// (rgl::matrix / rgl::mat4 / pos2 in the shared headers carry subsets of them)
struct Pos {
    vec4 s, f, u, o;
    Pos operator*(const Pos& b) const;                          // 0x40e870 (to parent: this * b)
    Pos to_local(const Pos& b) const;                           // 0x4329d0 (b in the frame of this)
    void rotate_x(const float* angle);                          // 0x40e570 (rows f and u about s)
    void rotate_y(const float* angle);                          // 0x40e670 (rows s and u about f)
    void rotate_z(const float* angle);                          // 0x40e770 (rows s and f about u)
    void orthonormalize();                                      // 0x407ef0 (rgl_math.h: keeps f)
};
struct Rot { vec4 s, f, u; };                                   // the rotation part of a Pos
Rot identity_rot();                                             // 0x40cd60 (cdecl, hidden return)
Pos identity_frame();                                           // 0x40cd00 (cdecl, hidden return; item_kind.h: identity_pos)
float __cdecl clamp_float(const float* v, const float* lo, const float* hi);    // 0x40e3d0
// by-reference spellings of the clamp helpers (same ABI as the pointer declarations in agent.h / game_util.h);
// the original passes temporaries
int __cdecl clamp_index(const int& v, const int& lo, const int& hi);   // 0x432510 (result in [lo, hi - 1])
__int64 __cdecl round_i64(const float& v);                     // 0x4875a0
int __cdecl round_float(const float& f);                        // 0x432540 (agent.h: round_float(float*))
int __cdecl rand_below(const int& n);                           // 0x432580 (game_util.h: rand_below(const int*))
int __cdecl imax_ref(const int& a, const int& b);               // 0x4324f0
float __cdecl fmin_ref(const float& a, const float& b);         // 0x4324b0
// the out-of-line copy of <math.h>'s inline float ceil(float) the linker took from d3dfont.obj
double __cdecl ceil_float_comdat(float x);                     // 0x404590 (src/dxframework/d3dfont.cpp)
int __cdecl imax_ref(const int* a, const int* b);                // 0x4324f0 (game_util.h)
__int64 __cdecl round_i64(const float* v);                      // 0x4875a0 (floor, +1 above .5)

// ---- position registers: g_script + 0x450, 128 frames of 0x40 bytes ----------------------------------------------
inline Pos& script_pos(int i) { return ((Pos*)((char*)&g_script + 0x450))[i]; }
extern int g_fixed_point_multiplier;    // 0xa70460 (g_script + 0x20; position_get_x 0x4a1d65)

// std::vector<int> (VC7.1: allocator word, then first / last / end)
#include "../int_vec.h"                 // IntVec (was IntVector here: size() 0x40f010)

// ---- tableaus (object 0xa75f40): vector of 0x68-byte records ----------------------------------------------------
struct TableauVec {
    int pad0;
    char* first;                        // +4 (warn_tableau 0x49d360)
    char* last;                         // +8
    int padc;
    IntVec stack;                       // +0x10 tableaus being rendered (cur_tableau_* 0x4aadca: none = no-op)
    int alpha_mask;                     // +0x20 (cur_tableau_render_as_alpha_mask 0x4ab022)
    int size() const { return (last - first) / 0x68; }
    struct Current {                    // the state of the tableau being rendered
        char pad0[0x344];
        int override_flags;             // +0x344 (cur_tableau_set_override_flags 0x4ab6bf)
        int num_override_items;         // +0x348
        int override_items[1];          // +0x34c (no bound check in cur_tableau_add_override_item)
    };
    Current* current();                                                     // 0x48f450
    void add_tableau_mesh(int tableau, int value, const Pos* p, const rgl::vector4& scale);   // 0x4992b0 (ret 0x10)
    void* material(const int* tableau, const int* instance_code);          // 0x4b14b0 (ret 8)
    void set_background_color(int color);                                   // 0x48f570 (ret 4)
    void set_ambient_light(const rgl::vector4& c);                          // 0x48f5a0 (ret 4)
    void set_camera_position(const Pos* p);                                 // 0x48f480 (ret 4)
    void set_camera_parameters(const bool* perspective, const float* w, const float* h, const float* near_,
                               const float* far_);                          // 0x48f4c0 (ret 0x14)
    void add_point_light(const Pos* p, const rgl::vector4& c);             // 0x493f80 (ret 8)
    void add_sun_light(const Pos* p, const rgl::vector4& c);               // 0x494020 (ret 8)
    void add_mesh(int mesh, const Pos* p, const rgl::vector4& scale, const float* alpha, const int* color);   // 0x499390 (ret 0x14)
    void add_map_icon(int icon, const Pos* p, const float* scale);          // 0x499540 (ret 0xc)
};
extern TableauVec g_tableaus;           // 0xa75f40
void __cdecl tableau_add_agent(int troop, int horse_item, const Pos* p, const int* animation, const int* instance);   // 0x49f3d0
// the item / scene object of a trigger context (ti_on_init_item -50, scene prop -40, map icon -70)
struct TriggerItem {
    void set_tableau_material(void* m); // 0x48cfc0 (ret 4)
};
struct TriggerMeshObj {
    void set_tableau_material(void* m); // 0x4620b0 (ret 4)
};
TriggerItem* __fastcall trigger_item(int context);                         // 0x48cec0
TriggerMeshObj* __fastcall trigger_mesh_obj(int context);                  // 0x48cf40

// ---- sound and music -------------------------------------------------------------------------------------------------
struct SoundSys {
    int play_at(int sound, int flags, const rgl::vector4* pos, float pitch, int out);   // 0x442650 (ret 0x14)
    void play(int sound, int flags);                            // 0x4942f0 (ret 8)
    void stop_all();                                            // 0x494310
};
extern SoundSys g_sound;                // 0x5aa128
extern int g_next_track;                // 0xa70474 (play_track 0x4a1716)
extern int g_next_track_options;        // 0xa70478
extern int g_cue_track;                 // 0xa7047c (play_cue_track 0x4a173f)
extern int g_music_situation;           // 0xa73a10 (music_set_situation 0x4a174a)
extern int g_music_culture;             // 0xa75e58 (music_set_culture 0x4a1755)
// the scene entity of the object a trigger runs for (scene prop / agent triggers; 0 otherwise); +0x88 is its position
int __fastcall trigger_entity(int context);                     // 0x490430

// ---- presentations (the running presentation, g_presentation 0x5e299c; 0 when none) ------------------------------
struct Presentation {
    char pad0[0x28];
    float duration;                                                     // +0x28 (presentation_set_duration 0x4a2143)
    int create_text_overlay(const tstring& text, int flags);            // 0x49e780 (ret 8)
    int create_mesh_overlay(int mesh);                                  // 0x4995e0 (ret 4)
    int create_button_overlay(const tstring& text, int flags);          // 0x494110 (ret 8)
    int create_image_button_overlay(int mesh, int mesh_pressed);        // 0x499a20 (ret 8)
    int create_slider_overlay(int min_value, int max_value);            // 0x495970 (ret 8)
    int create_progress_overlay(int min_value, int max_value);          // 0x499b80 (ret 8)
    int create_combo_button_overlay();                                  // 0x499d90
    int create_text_box_overlay();                                      // 0x49a0b0
    int create_check_box_overlay(int mesh, int mesh_checked);           // 0x499b50 (ret 8)
    int create_image_button_overlay_with_tableau(int mesh, int tableau, int value);   // 0x4b14f0 (ret 0xc)
    int create_mesh_overlay_with_tableau(int mesh, int tableau, int value);           // 0x4996e0 (ret 0xc)
    int create_game_button_overlay(const tstring& text);                // 0x499830 (ret 4)
    void set_text(int overlay, const tstring& text);                    // 0x48f5f0 (ret 8)
    void set_color(int overlay, int color);                             // 0x48dc30
    void set_alpha(int overlay, int alpha);                             // 0x48de00
    void set_hilight_color(int overlay, int color);                     // 0x48e1a0
    void set_hilight_alpha(int overlay, int alpha);                     // 0x48e270
    void set_size(int overlay, const rgl::vector4* size);               // 0x49a300
    void set_position(int overlay, const rgl::vector4* pos);            // 0x48f6b0
    void set_val(int overlay, int value);                               // 0x48e340
    void set_boundaries(int overlay, int min_value, int max_value);     // 0x48e4c0 (ret 0xc)
    void set_area_size(int overlay, const rgl::vector4* size);          // 0x49ab80
    void set_mesh_rotation(int overlay, const Pos* pos);                // 0x48db20
    void add_item(int overlay, const tstring& text);                    // 0x48e590
    void animate_to_color(int overlay, float duration, int color);      // 0x48e650 (ret 0xc)
    void animate_to_alpha(int overlay, float duration, int alpha);      // 0x48e6d0
    void animate_to_highlight_color(int overlay, float duration, int color);  // 0x48e750
    void animate_to_highlight_alpha(int overlay, float duration, int alpha);  // 0x48e7d0
    void animate_to_size(int overlay, float duration, const rgl::vector4* size);      // 0x48e850
    void animate_to_position(int overlay, float duration, const rgl::vector4* pos);   // 0x48e5d0
};
extern Presentation* g_presentation;    // 0x5e299c

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
SI_ASSERT(party_size, sizeof(PartyX) == 0x5ac8);
SI_ASSERT(party_stacks, offsetof(PartyX, stacks) == 0xa8);
SI_ASSERT(party_ai, offsetof(PartyX, ai_behavior) == 0x918);
SI_ASSERT(party_notes, offsetof(PartyX, notes) == 0x5a28);
// PartyMgrX: the members of the party table (PartyMgr in dialog_manager.h) the interpreter adds
// PartyMgr::is_live 0x4903e0 of include/party.h replaces the PartyMgrX view

// ---- troops (0x508 bytes, array at game +0x13a00) ----------------------------------------------------------------
// TroopX: view of the troop record (TroopRec in dialog_manager.h, Troop in agent.h)
// TroopX: the troop record (TroopRec, include/troop.h) with the interpreter's names for its members; the fields are aliases in
// anonymous unions of troop.h (script operations cited there)
struct TroopX : TroopRec {
    int num_inventory_slots();          // 0x462b60 (TroopRec::inv_count)
    char is_hero();                     // 0x435130 (tf_hero; Troop::has_flag10)
    int max_hit_points();               // 0x468770
    int proficiency(int p);             // 0x435150 (ret 4; Troop::stat_int)
    void set_site(int scene, int entry);    // 0x489320 (ret 8; Troop::set_colors)
    void set_type(int skin);            // 0x4892a0 (ret 4; Troop::set_flags_low)
    void sort_inventory();              // 0x48c0c0 (Troop::sort_inv)
    void raise_attribute(int attribute, int value);     // 0x48bef0 (ret 8; Troop::add_attr)
    void raise_skill(int skill, int value);             // 0x48c090 (ret 8; Troop::add_skill)
    void raise_proficiency(int proficiency, float value);   // 0x437770 (ret 8; Troop::adjust)
    void raise_proficiency_linear(int proficiency, int value);  // 0x4892d0 (ret 8; Troop::add_stat)
    void remove_gold(int gold);         // 0x489340 (ret 4; Troop::sub_3b4)
    int first_free_inventory_slot();    // 0x463030 (Troop::find_free)
    void set_item(int item, int slot, int modifier);    // 0x48c7f0 (ret 0xc; Troop::set_slot)
    // inventory slot n: 0..9 are the equipment, 10.. the inventory
    ItemRef& slot(int n) { return n < 10 ? equipment[n] : items[n - 10]; }
};
void __fastcall clear_inventory_items(ItemRef* items);  // 0x489060 (the 0x60 inventory entries of a troop)
// ItemRef (src/game/item_ref.h) carries the script editors set_item / set_modifier / randomize_modifier
inline ItemRef& IR(ItemRef& r) { return r; }
struct ItemList {                       // this = the first ItemRef; -1 item = free
    int first_free(int count);          // 0x489070
};
float frand();                                      // 0x40e400 (0..1)
int __cdecl rand_below(const int* n);               // 0x432580 (0 when *n < 2, else rand() % *n)
float __cdecl fmin_ref(const float* a, const float* b);     // 0x4324b0 (game_util.h)
SI_ASSERT(troop_size, sizeof(TroopX) == 0x508);
SI_ASSERT(troop_equipment, offsetof(TroopX, equipment) == 0x4a4);
SI_ASSERT(troop_notes, offsetof(TroopX, notes) == 0x420);

// ---- quests (0x138 bytes, array at game +0x13a58) ----------------------------------------------------------------
#include "../../include/game_recs.h"   // QuestRec / FactionRec / PartyTemplate: the shared record classes (one definition for src/game and src/rgl)
SI_ASSERT(quest_size, sizeof(QuestRec) == 0x138);
SI_ASSERT(quest_notes, offsetof(QuestRec, notes) == 0x88);

// ---- factions (0x300 bytes, game +0x13a48), scenes (0x494, game +0x13a50), party templates (0xb0, 0x5e29f8) -------
SI_ASSERT(faction_size, sizeof(FactionRec) == 0x300);
SI_ASSERT(faction_notes, offsetof(FactionRec, notes) == 0x27c);
struct Visitor { int troop; int dna; };     // 8 bytes (set_visitor 0x4a4bff)
struct SceneRec {
    char pad0[0x114];
    Visitor visitors[64];               // +0x114 (reset_visitors 0x4a4b58; dna 0xffff2f33 = none)
    int visitor_counts[64];             // +0x314 (set_visitors 0x4a4d75)
    char pad414[0x474 - 0x414];
    SlotArray slots;                    // +0x474 (scene_set_slot 0x4a0906)
    char pad475[0x494 - 0x475];
};
SI_ASSERT(scene_size, sizeof(SceneRec) == 0x494);
SI_ASSERT(ptemplate_size, sizeof(PartyTemplate) == 0xb0);
extern PartyTemplate* g_party_templates;     // 0x5e29f8 (0xb0-byte records; PartyTemplate in src/include/game_recs.h)
// ItemKindX: view of the item kind record (ItemKind in src/game/agent/item_kind.h, array g_items 0x5e29d0)
struct ItemKindX {
    char pad0[0x30];
    SlotArray slots;                    // +0x30 (item_set_slot 0x4a0ada)
    char pad31[0x1c8 - 0x31];
    unsigned type_flags;                // +0x1c8 low byte = itp_type_* (set_price_rate_for_item_type 0x4a3eb0)
    char pad1cc[0x1d8 - 0x1cc];
    int value;                          // +0x1d8 (troop_loot_troop)
    char pad1dc[0x21c - 0x1dc];
    float price_rate;                   // +0x21c (set_price_rate_for_item 0x4a3e87)
    char pad220[0x224 - 0x220];
    float merchandise_probability;      // +0x224 (set_item_probability_in_merchandise 0x4a56ef)
    char pad228[0x238 - 0x228];
    tstring* display_name();            // 0x43a480 (folded with TroopX::display_name)
    tstring* plural_name();             // 0x48d6d0 (folded with TroopX::plural_name)
};
SI_ASSERT(itemkind_size, sizeof(ItemKindX) == 0x238);
inline ItemKindX* item_kind(int id) { return &((ItemKindX*)g_items)[id]; }

extern int g_visitor_scene;             // 0x5e2594 (modify_visitors_at_site 0x4a4b17)
extern float g_spawn_radius;            // 0x5a9c18 (set_spawn_radius 0x4a2e0c)
extern int g_text_log_enabled;          // 0x5a9fb0 (display_message 0x4a305a)
extern char g_show_messages;            // 0x5d8794 (set_show_messages 0x4a30a0)

// ---- map battles (table at game +0xcc) ---------------------------------------------------------------------------
#include "../battle_rec.h"            // BattleRec, BattleTable: the one definition (party2 and script)

// ---- the game object (g_game, 0x5e2930; GameData in dialog_manager.h) --------------------------------------------
struct GameX {
    char pad0[0x78];
    int main_party;                     // +0x78 (entering_town 0x49fb83)
    char pad7c[0x84 - 0x7c];
    int camera_follow_party;            // +0x84 (set_camera_follow_party 0x4a2afb)
    char camera_follow;                 // +0x88
    char pad89[0x8c - 0x89];
    int player_troop;                   // +0x8c (set_player_troop 0x49fcb5)
    char pad90[0x98 - 0x90];
    int player_kill_count;              // +0x98 (get_player_agent_kill_count 0x4a89e6)
    int player_own_troop_kill_count;    // +0x9c (0x4a8a3a)
    int player_wounded_count;           // +0xa0 (0x4a8a0c)
    int player_own_troop_wounded_count; // +0xa4
    unsigned encounter_flags;           // +0xa8 bit 0 = leave (leave_encounter 0x4a5589)
    int background_presentation;        // +0xac (start_background_presentation 0x4a2121)
    unsigned encounter_flags2;          // +0xb0 bit 6 = attack (encounter_attack 0x4a559a)
    char padb4[0xc8 - 0xb4];
    int next_window;                    // +0xc8 window to switch to (0xf = presentation; start_presentation 0x4a20d5)
    BattleTable battles;                // +0xcc (party_set_next_battle_simulation_time 0x4a8172)
    PartyMgr parties;                   // +0xf8 .. +0x139d8 (PartyMgr, dialog_manager.h)
    char pad139d8[0x13a00 - 0x139d8];
    TroopX* troops;                     // +0x13a00
    int num_troops;                     // +0x13a04 (check_troop 0x49cee0)
    char pad13a08[0x13a0c - 0x13a08];
    int encountered_party;              // +0x13a0c (store_encountered_party 0x4ad248; change_screen_exchange_members)
    int encountered_party2;             // +0x13a10 (store_encountered_party2 0x4ad258)
    int encountered_party_is_attacker;  // +0x13a14 (0x49fc5f)
    char pad13a18[0x13a1c - 0x13a18];
    int repeat_object;                  // +0x13a1c (store_repeat_object 0x49fcc5)
    char pad13a20[0x13a30 - 0x13a20];
    int mercenary_source_party;         // +0x13a30 (set_mercenary_source_party 0x4a5629)
    int pad13a34;
    float merchandise_modifier_quality; // +0x13a38 (set_merchandise_modifier_quality 0x4a566c)
    char start_map_conversation;        // +0x13a3c (start_map_conversation 0x4a2b33)
    char pad13a3d[3];
    int map_conversation_mode;          // +0x13a40 (5)
    int map_conversation_troop;         // +0x13a44
    FactionRec* factions;               // +0x13a48 (faction_set_slot 0x4a0884)
    int num_factions;                   // +0x13a4c (check_faction 0x49cf80)
    SceneRec* scenes;                   // +0x13a50 (scene_set_slot 0x4a0900)
    int num_scenes;                     // +0x13a54 (check_scene 0x49d020)
    QuestRec* quests;                   // +0x13a58 (check_quest_active 0x4a0627)
    int num_quests;                     // +0x13a5c (check_quest 0x49cf30)
    char pad13a60[0x13ad0 - 0x13a60];
    float rest_period;                  // +0x13ad0 hours (rest_for_hours 0x4a2b49)
    int rest_attackable;                // +0x13ad4 0 = remain attackable
    int rest_interactive;               // +0x13ad8
    int rest_time_speed;                // +0x13adc
    int party_creation_min_random;      // +0x13ae0 (set_party_creation_random_limits 0x4a2d7f)
    int party_creation_max_random;      // +0x13ae4
    char pad13ae8[0x13aec - 0x13ae8];
    int current_menu;                   // +0x13aec (jump_to_menu 0x4ac38b)
    int screen_troop;                   // +0x13af0 troop of the loot / trade / equip screens (0x4ac0a1)
    int jump_scene;                     // +0x13af4 (jump_to_scene 0x4aa696)
    int jump_entry;                     // +0x13af8 (set_jump_entry 0x4aa6e5)
    int jump_mission;                   // +0x13afc (set_jump_mission 0x4aa6a7)
    int screen_party;                   // +0x13b00 (change_screen_exchange_with_party 0x4ac21d)
    int presentation;                   // +0x13b04 presentation to start (start_presentation 0x4a20e8)
    int notes_type;                     // +0x13b08 (change_screen_notes 0x4ac312)
    int notes_object;                   // +0x13b0c
    char talk_enabled;                  // +0x13b10 (mission_enable_talk 0x4aa721)
    char talk_disabled;                 // +0x13b11 (mission_disable_talk 0x4aa732)
    char pad13b12[0x13b18 - 0x13b12];
    Timer clock;                        // +0x13b18 (display_log_message 0x4a2edc; Timer::elapsed 0x432c10 reads the clock index at +0 and the start at +0x13b20)
    int current_hours;                  // +0x13b28 (store_current_hours 0x4ad57e)
    int note_time;                      // +0x13b2c the current day: time stamp of an added note (0x4a3330, store_current_day)
    char pad13b30[0x13b3c - 0x13b30];
    float cloud_amount;                 // +0x13b3c 0..1 (get_global_cloud_amount 0x49fecb)
    float haze_amount;                  // +0x13b40 (get_global_haze_amount 0x49ff46)
    float average_difficulty;           // +0x13b44 (get_average_game_difficulty 0x4a2a51)
    char pad13b48[0x13b50 - 0x13b48];
    tstring message_log;                // +0x13b50 "|time|colour|text" records (display_log_message 0x4a2ec9)
    char pad13b60[0x13bc4 - 0x13b60];
    int num_context_menu_items;         // +0x13bc4 (context_menu_add_item 0x4a29ed)
    int pad13bc8;
    tstring context_menu_texts[50];     // +0x13bcc
    int context_menu_values[50];        // +0x13eec
    rgl::vector4 current_color;         // +0x13fb4 (set_current_color 0x4aa91e)
    rgl::vector4 position_delta;        // +0x13fc4 (set_position_delta 0x4aa974)
    char pad13fd4[0x14014 - 0x13fd4];
    Pos trigger_object_pos;             // +0x14014 also the spawn position (set_spawn_position 0x4aac92)
    char menu_option_disabled;          // +0x14054 (disable_menu_option 0x4ac3b9)
    char pad14055[0x1405c - 0x14055];
    char stack_rows[0x80 * 32];         // +0x1405c per-stack rows that move with remove_stack (0x4a4490)
    char pad1505c[0x1a208 - 0x1505c];
    int question_answer;                // +0x1a208 (question_box 0x4a3ca4)
    int pad1a20c;
    tstring background_mesh;            // +0x1a210 (set_background_mesh 0x4abfc8)
    int menu_tableau;                   // +0x1a220 (set_game_menu_tableau_mesh 0x4ac02f)
    int menu_tableau_value;             // +0x1a224
    rgl::vector4 menu_tableau_pos;      // +0x1a228
    int talk_relation_bar;              // +0x1a238 (talk_info_set_relation_bar 0x4abeb6)
    int talk_info_shown;                // +0x1a23c (talk_info_show 0x4abea6)
    char pad1a240[0x1a290 - 0x1a240];
    char talk_line_set[5];              // +0x1a290 (talk_info_set_line 0x4abf70)
    // thiscall on the game object
    ~GameX();                                                               // 0x492f40
    void set_talk_line(int line, const tstring& text);                      // 0x48a380 (ret 8)
    char party_can_join_party(int joiner, int host, char flip_prisoners);   // 0x4918d0 (ret 0xc)
    void update_average_difficulty();                                       // 0x48a000
    void add_xp_to_troop(int troop, int xp);                                // 0x4428c0 (ret 8)
    void add_gold_as_xp(int gold);                                          // 0x4952a0 (ret 4)
    void add_xp_as_reward(int xp);                                          // 0x4957d0 (ret 4)
    int spawn_around(float x, float y, int party_template, float radius);   // 0x49f020 (ret 0x10)
    tstring format_time(int seconds);                                       // 0x48c470 (ret 8: hidden return)
    void reset_price_rates();                                               // 0x48a210
    void transfer_stacks(int from, int to, char as_prisoners);              // 0x492530 (ret 0xc)
    int remove_troops(int party, int troop, int count, char prisoners, int a5);   // 0x48f9f0 (ret 0x14: count removed)
    void remove_party(int party);                                           // 0x4915d0 (ret 4)
    int create_companion_party(int troop, rgl::vector4 pos);                // 0x49ef00 (ret 0x14)
    void update_faction_parties(int faction);                               // 0x490990 (ret 4)
    void start_quest(int quest, int giver_troop);                           // 0x49b930 (ret 8)
    void complete_quest(int quest);                                         // 0x494410 (ret 4)
    void succeed_quest(int quest);                                          // 0x494560
    void fail_quest(int quest);                                             // 0x494680
    void cancel_quest(int quest);                                           // 0x4948c0
    void conclude_quest(int quest);                                         // 0x4947a0
    void start_encounter(int party, int other);                             // 0x49f060 (ret 8)
    void set_item_probabilities(float rate);                                // 0x48a250 (ret 4)
    char troop_is_wounded(int troop);                                       // 0x4638f0 (ret 4)
    void ensure_inventory_space(int troop, int value);                      // 0x48c4c0 (ret 8)
    void add_merchandise(int item_type, int value, int troop);              // 0x48cc40 (ret 0xc)
    void add_gold(int gold, int troop);                                     // 0x49b720 (ret 8)
    void remove_player_gold(int gold, int a2);                              // 0x49b820 (ret 8)
    void player_add_item(int item, int modifier);                           // 0x48eb50 (ret 8)
    void add_items(int troop, int item, int count);                         // 0x48c9b0 (ret 0xc)
    int remove_items(int troop, int item, int count, char a4);              // 0x489d10 (ret 0x10: value removed)
    int add_members(int party, int troop, int count, char prisoners);       // 0x48f8f0 (ret 0x10: number added)
    void relocate_near(float x, float y, int party, float radius);          // 0x491e50 (ret 0x10)
    void random_position_around(float x, float y, float radius, rgl::vector4* out);           // 0x490030 (ret 0x10)
    char position_around(float x, float y, float radius, rgl::vector4* out, char water);      // 0x490160 (ret 0x14)
    int count_troops(int troop, int party, char prisoners);                 // 0x48f790 (ret 0xc)
    int stack_dna(int party, int stack, int a3, int a4);                    // 0x4902a0 (ret 0x10)
    int item_count(int item, int troop, char count_items);                  // 0x489c50 (ret 0xc; item -1, 0: free slots)
    int troop_skill_level(int troop, int skill);                            // 0x437b20 (ret 8)
    int troop_value(int troop);                                             // 0x48a320 (ret 4)
    int random_quest(int lo, int hi);                                       // 0x489e70 (ret 8)
    int random_troop(int a1, char to_capture);                              // 0x48a080 (ret 8)
    int random_party(int lo, int hi);                                       // 0x489ee0 (ret 8)
    int random_item(int type1, int type2, int type3, int type4);            // 0x48ca30 (ret 0x10)
    char is_night();                                                        // 0x435740
    void attach_party(int party, int host);                                 // 0x4949d0 (ret 8)
    void detach_party(int party);                                           // 0x490a30 (ret 4)
    void collect_attachments(int party, PartyX* dest);                      // 0x48ff70 (ret 8)
    void quick_attach_to_battle(int party, int side);                       // 0x494ad0 (ret 8)
    void upgrade_with_xp(int party, float xp, int path);                    // 0x494e40 (ret 0xc)
    void party_add_xp(int party, float xp);                                 // 0x494b60 (ret 8)
    void add_template(int party, int party_template, int reverse_prisoners);   // 0x48fbd0 (ret 0xc)
    int battle_root(int party);                                             // 0x48fce0 (ret 4)
    void inflict_casualties(int party, int rounds, PartyX* dest);           // 0x4910a0 (ret 0xc)
    void distribute_party(PartyX* party, int root);                         // 0x491430 (ret 8)
};
SI_ASSERT(game_troops, offsetof(GameX, troops) == 0x13a00);
SI_ASSERT(game_parties, sizeof(PartyMgr) == 0x139d8 - 0xf8);
SI_ASSERT(game_cloud, offsetof(GameX, cloud_amount) == 0x13b3c);
SI_ASSERT(game_log, offsetof(GameX, message_log) == 0x13b50);
SI_ASSERT(game_menu, offsetof(GameX, context_menu_values) == 0x13eec);
SI_ASSERT(game_tpos, offsetof(GameX, trigger_object_pos) == 0x14014);
inline GameX* game() { return (GameX*)g_game; }
inline PartyX* party(int id) { return (PartyX*)g_game->parties.get(id); }
inline rgl::vector4& party_pos(PartyX* p) { return *(rgl::vector4*)&p->pos; }       // Party::pos (+0x30) is a plain 4-float record; the interpreter uses it as a vector4
inline TroopX* troop(int id) { return &game()->troops[id]; }
inline QuestRec* quest(int id) { return &game()->quests[id]; }
inline FactionRec* faction(int id) { return &game()->factions[id]; }
inline SceneRec* scene(int id) { return &game()->scenes[id]; }

extern int g_conversation_troop;        // 0x8ed50d4 (g_dialogs +0xc; start_quest)
extern int g_encountered_party;         // 0x8ed50d8 (g_dialogs +0x10; party_can_join)
extern int g_siege_attack;              // 0xa73438 (store_defender_count 0x4ae36e)
extern int g_conversation_agent;        // 0x8ed50dc (g_dialogs +0x14; store_conversation_agent)
struct MissionUiX {
    void select_enemy(int side);        // 0x492750 (thiscall, ret 4)
};
extern MissionUiX g_mission_ui;         // 0xa73300
extern int g_battle_advantage;           // 0xa7331c (set_battle_advantage 0x4a87af)
extern int g_passage_menu;              // 0xa735c4 (set_passage_menu 0x4a55f3)
extern int g_menu_b;                    // 0x5e2924 (jump_to_menu 0x4ac3a0)
struct MeshRec { tstring name; char pad10[0x48 - 0x10]; };
extern MeshRec* g_meshes;               // 0x8ed4f58 (set_background_mesh)
extern __int64 g_trigger_param_1;       // 0xa70448 (g_script +8)
extern __int64 g_trigger_param_2;       // 0xa70450 (g_script +0x10)
extern __int64 g_trigger_result;        // 0xa70458 (g_script +0x18; set_trigger_result 0x4ac3f2)
extern unsigned g_battle_flags;         // 0xa73464 bit 0 = end the battle (end_current_battle 0x4a55fe)
extern int g_skin_count;                // 0x8ed4f88 (troop_set_type)
extern char g_party_battle_mode;         // 0xa735c8 (set_party_battle_mode 0x4a2ad0)
extern char g_in_meta_mission;          // 0x8ee30ec (read by in_meta_mission)

// ---- the mission (g_mission, 0x5e292c; Mission in agent.h) --------------------------------------------------------
// The agent record is Agent (src/game/agent_obj.h; 0x5928 bytes, pages of 16): the slices' AgentX.
#include "../blow.h"                // Blow (agent_deliver_damage_to_agent fills it; the one definition, shared with agent4)
SI_ASSERT(agent_size, sizeof(Agent) == 0x5928);
struct PropInst {                       // 0x1c8-byte scene prop instance
    char pad0[0x18];
    Pos frame;                          // +0x18 (prop_instance_get_position 0x4aa404)
    char pad58[0x98 - 0x58];
    Pos start_frame;                    // +0x98 (prop_instance_get_starting_position 0x4aa459)
    char padd8[0x158 - 0xd8];
    float scale_x, scale_y, scale_z;    // +0x158 (prop_instance_get_scale 0x4aa4df)
    char pad164[0x1c8 - 0x164];
    void set_position(const Pos* p);                            // 0x489820 (ret 4)
    void animate_to(const Pos* p, const float* seconds, const float* a3);   // 0x4897a0 (ret 0xc)
};
#include "../prop_vec.h"      // PropVec: the one definition (agent4, map_editor, script)
// the scene object at mission +0 is `world` (src/game/world.h; Level in agent.h)
struct EntryPoint { Pos pos; char pad40[0x130 - 0x40]; };      // mission +0x670 (entry_point_get_position 0x4a9ab0)
// TeamRec (0x16c bytes at mission +0x5280): src/game/mission.h
// a point light added by add_point_light (0x7c bytes)
#include "../light.h"                   // Light (shared with agent2 / strat)
// the trigger's scene entity (prop instance or agent body, trigger_entity 0x490430): virtual slots 25 / 27
struct EntityMeshes {
    void attach_light(int mesh, Light* l);                      // 0x488e40 (ret 8)
    void attach_particle_system(int mesh, void* ps);            // 0x488e20 (ret 8)
    struct PSys* find_particle_system(int mesh, int psys);      // 0x48d5f0 (ret 8)
};
struct PSys {
    void emit(int count);               // 0x433810 (ret 4; game_util.h reset_pending)
};
struct SceneEntity {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23(); virtual void v24();
    virtual void add_particle_system(void* ps);                 // +0x64
    virtual void v26();
    virtual int find_particle_system(int psys);                 // +0x6c (-1 = none)
    char pad4[0x304 - 4];
    EntityMeshes* meshes;               // +0x304
    int pad308;
    PSys** particle_systems;            // +0x30c
};
// the item mesh of the trigger's agent (contexts -50 / -51), -1 otherwise. The original passes the context in
// EAX (a static of the s004902a0 object, agent_of); declared here as a plain call.
int trigger_item_mesh(int context);     // 0x4904a0
struct ParticleMgr {
    void* create(const int* psys, const rgl::vector4* pos, const int* a3);  // 0x479d20 (ret 0xc)
};
extern ParticleMgr g_particle_mgr;      // 0x8ed5090

struct MTimer {                         // clock index + start (Timer in src/rgl/xmlcfg/game_util.h)
    int clock;
    int pad4;
    __int64 start;
    void restart();                     // 0x432bf0
    float elapsed() const;              // 0x432c10 (seconds)
};
struct MissionX {
    world* scene;                       // +0
    AgentTable agents;                  // +4
    char pad2c[0x5c - 0x2c];            // (AgentTable is 0x28 bytes: count, free-id list, page vector, serial)
    int player_agent;                   // +0x5c (heal_party 0x4a488d)
    char pad60[0x670 - 0x60];
    EntryPoint entries[64];             // +0x670
    char pad5270[0x5280 - 0x5270];
    TeamRec teams[8];                   // +0x5280
    char pad5de0[0x5df4 - 0x5de0];
    int current_scene;                  // +0x5df4 (store_current_scene 0x4ad2c8)
    char pad5df8[0x6214 - 0x5df8];
    int mode;                           // +0x6214 (agent_is_defender 0x4a8b80)
    int mode_side;                      // +0x6218
    char pad621c[0x62f8 - 0x621c];
    MTimer timer_a, timer_b, timer_c;   // +0x62f8 (store_mission_timer_a 0x4ae1f3)
    char pad6328[0x6524 - 0x6328];
    int result;                         // +0x6524 (set_mission_result 0x4aa65d)
    int pad6528;
    float min_x, min_y, max_x, max_y;   // +0x652c scene bounds (position_set_z_to_ground_level 0x4a203f)
    float rect_min_x, rect_min_y, rect_max_x, rect_max_y;   // +0x653c (get_scene_boundaries 0x4aa2fe)
    char pad654c[0x65fc - 0x654c];
    int rain_f65fc;                     // +0x65fc set to 3 by heavy rain (set_rain 0x4aa1f5)
    float rain_strength;                // +0x6600 0..1
    int rain_type;                      // +0x6604 1 = rain, 2 = snow
    int pad6608;
    int env_row;                        // +0x660c index into g_env_rows (set_fog_distance 0x4aa22a)
    float fog_distance;                 // +0x6610
    char pad6614[0x6620 - 0x6614];
    int conversation;                   // +0x6620 (start_mission_conversation 0x4aa6ff)
    char pad6624[0x6688 - 0x6624];
    PropVec props;                      // +0x6688 (first +0x668c, last +0x6690, end +0x6694; warn_scene_prop_instance 0x49d290)
    char pad6698[0x6748 - 0x6698];
    int cam_mode;                       // +0x6748 0 automatic, 1 manual (mission_cam_set_mode 0x4ab858)
    Pos cam_frame;                      // +0x674c (ui/mission.h frame_a)
    Pos cam_target_frame;               // +0x678c (frame_b)
    float cam_aperture;                 // +0x67cc (mission_cam_set_aparture 0x4abc2d; blend_from)
    float cam_target_aperture;          // +0x67d0 (blend_to)
    float cam_anim_time;                // +0x67d4 seconds, 0 = no animation
    MTimer cam_timer;                   // +0x67d8
    int cam_target_agent;               // +0x67e8 -1 = none (mission_cam_set_target_agent 0x4abe6f)
    int cam_use_agent_rotation;         // +0x67ec
    int cam_easing;                     // +0x67f0 0 = linear (blend_n)
    float time_speed;                   // +0x67f4 (mission_set_time_speed 0x4ab98c)
    float target_time_speed;            // +0x67f8
    int pad67fc;
    MTimer time_speed_timer;            // +0x6800
    float time_speed_anim_time;         // +0x6810
    char all_enemies_defeated(int team);                        // 0x4920e0 (thiscall, ret 4)
    void visitors_changed();                                    // 0x4676c0
    void add_visitors(int entry, int count);                    // 0x4677a0 (ret 8)
    int num_active_teams();                                     // 0x48a8b0
    char main_hero_fallen();                                    // 0x492170
    int last_team();                                            // 0x48a910 (ui/mission.h)
    int count_agents(int team);                                 // 0x491f50 (ret 4)
    void select_team(int team);                                 // 0x48a9e0 (ret 4)
    void select_class(int team, int sub_class);                 // 0x48aa60 (ret 8)
    void unselect_team(int team);                               // 0x48aa20 (ret 4)
    int movement_order(int team);                               // 0x48aac0 (ret 4)
    int riding_order(int team);                                 // 0x48ab30 (ret 4)
    int weapon_usage_order(int team);                           // 0x48ab90 (ret 4)
    int hold_fire_order(int team);                              // 0x48abf0 (ret 4)
    void give_order(int team, int order);                       // 0x49c2c0 (ret 8)
    char point_in_rect(const rgl::vector4* p);                  // 0x48a990 (ret 4; ui/mission.h)
    int num_prop_instances(int prop);                           // 0x48ee20 (ret 4)
    int prop_instance(int prop, int n);                         // 0x48ee90 (ret 8: -1 = none)
    void replace_props(int old_prop, int new_prop);             // 0x48ed60 (ret 8)
    void finish(int a1);                                        // 0x48a860 (ret 4)
    Pos cam_current_frame();                                    // 0x48a420 (ret 4: hidden return; ui/mission.h get_frame)
    float cam_current_aperture();                               // 0x48a740 (blend_param)
    float current_time_speed();                                 // 0x48a7d0 (blend2_param)
    int start_conversation(int troop);                          // 0x4921b0 (ret 4)
    void add_reinforcements(int entry, int count);              // 0x49c170 (ret 8)
    void add_point_light(float flicker_magnitude, float flicker_interval);   // 0x4952f0 (ret 8)
    void burst(int psys, const Pos* p, int strength);           // 0x442ca0 (ret 0xc)
    int spawn_item(const ItemRef* item);                        // 0x49b4b0 (ret 4)
    int spawn_agent(int troop);                                 // 0x49f9e0 (ret 4)
    int spawn_horse(int item, int modifier);                    // 0x49f980 (ret 8)
};
SI_ASSERT(mission_props, offsetof(MissionX, props) == 0x6688);
SI_ASSERT(mission_teams, offsetof(MissionX, teams) == 0x5280);
SI_ASSERT(mission_mode, offsetof(MissionX, mode) == 0x6214);
SI_ASSERT(mission_fog, offsetof(MissionX, fog_distance) == 0x6610);
SI_ASSERT(mission_cam, offsetof(MissionX, cam_frame) == 0x674c);
SI_ASSERT(mission_speed, offsetof(MissionX, time_speed_anim_time) == 0x6810);
SI_ASSERT(prop_scale, offsetof(PropInst, scale_x) == 0x158);
// the 0x4c-byte environment rows (EnvRow in src/game/terrain_b/emit.h)
struct EnvRowX { char pad0[0x44]; unsigned fog_color; char pad48[0x4c - 0x48]; };
extern EnvRowX* g_env_rows_x;           // 0x5e28f4 (set_fog_distance 0x4aa230)
inline MissionX* mission() { return (MissionX*)g_mission; }
inline Agent* agent(int id) { return mission()->agents.get(id); }

// ---- the operation record ---------------------------------------------------------------------------------------
// Elem (dialog_manager.h): +0 opcode word (flags neg / this_or_next in the top bits), +8 operand count, +0x10 the
// 16 raw 64-bit operands, +0x90 skip target. The interpreter is a member of the record.
struct Operation : Elem {
    // the value of operand *i: plain constant, register, global variable or local variable (type out)
    __int64 get_operand(__int64* locals, int* i, int* type);       // 0x49d650 (thiscall, ret 0xc)
    // store *value into the lvalue operand 0 (register, global variable or local variable)
    void set_lvalue(__int64* locals, const __int64& value);        // 0x49d500 (thiscall, ret 8; value by address)
    // run one operation; returns the condition result; *error = 1 when a script error was reported
    char execute(__int64* locals, int* context, int* error);       // 0x49fa10 (thiscall, ret 0xc)
};

#endif // GAME_SCRIPT_INTERPRETER_H
