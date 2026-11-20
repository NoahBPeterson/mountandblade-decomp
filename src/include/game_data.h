// game_data.h: the game-data object (GameData, global pointer g_game at 0x5e2930, 0x1a2b0 bytes). ONE definition for
// src/game and src/rgl: the fields the views over g_game named (GameX of the script interpreter, EditorGameView of the
// scene editor, GameConfig of the launcher page, SiteKeys, Frame, World3, GDTimer, GameMapView) live here, under the names
// of the operations that read them; party/game.h's Game is the same object as its constructor and loaders see it.
// Offsets cite the function that reads them. Names that two modules gave to one offset overlay in anonymous unions of
// plain members (main_party / cur_party, player / player_troop, troops / chars, levels / scenes, ...). The contiguous
// blocks are member sub-objects: the script debugger (+0x1a060), the scene editor (+0x1a090), the trigger event context
// (+0x1a1f0). Note: the number of members and anonymous aggregates here moves the register allocation of two
// non-matching rows that include this header (render_batch::render_scene 0x46e590, PartyScreen::setup_skills 0x5059c0);
// check both after a layout edit.
#ifndef MB_GAME_DATA_H
#define MB_GAME_DATA_H
#include "../game/tstring.h"
namespace rgl { struct matrix; struct world; }
#include "./troop.h"
#include "./party.h"
#include "./pool.h"                             // Pool<Event> events
struct Event;                                   // game/event.h
#include "./game_recs.h"
#include <stddef.h>
#include "./layout32.h"
namespace rgl { struct mesh; struct entity; struct lod_mesh2; }
#include "../rgl/rgl_math.h"                     // rgl::vector4, rgl::matrix
#include "./timer.h"                            // Timer
#include "./trigger_tags.h"                     // TriggerTag, SimpleTag
#include "./game_parts.h"                       // ScriptDebugger, EventContext
#include <vector>
struct TroopX;                                  // script/script_interpreter.h: the interpreter's view of the troop record
struct SceneFile;                               // terrain_b/scene_file.h: the 0x494-byte scene record (Level494 below)
struct TriggerRec;                              // party/game.h: a module trigger (0x78 bytes)
struct Light;                                   // game/light.h
struct Vec2;                                    // include/vec.h
struct SimpleRec;                               // party/game.h: a simple trigger (0x30 bytes)

// record types of the save-game writer (char_sheet/save_state.h defines them), the party-info record of the map tooltip and the
// serialisation streams: only pointers to them are members / parameters here
struct Rec300;
struct RecB0;
struct Rec138;
struct PartyInfo;
struct OutStream;
struct SaveCtx;

// The 0x5c-byte event / fleet / track record (the element of the vector at game+0x13a60). party2/party_2.h and
// party/game.h name the same record E92 / TrackRec; game_data.h cannot include either header (party_2.h includes this
// one), so the element type is named Event92 here. The layout is identical (0x5c bytes, empty destructor).
struct Event92 { int v[23]; ~Event92() {} };     // 0x5c: event record; the empty destructor keeps _Tidy's loop
struct E92;                                     // party2/party_2.h's name for the same record (init_event's parameter)

// The 0x494-byte level / scene-object record (table at game+0x13a50).
struct Level494 {
    char pad0[0x10];
    tstring name10;                             // +0x10 the scene object file's name (scene_path 0x463950: "SceneObj\\<name>.sco")
    char pad20[0x30 - 0x20];
    union {
        unsigned flags;                         // +0x30 (0x100: flat level, 0x1: no fog sheet) (0x479e20, 0x47a0c0, 0x4792d0)
        unsigned char flags8;                   // +0x30 as a byte (0x4435c0)
    };
    tstring name34;                             // +0x34 (0x47a800: the prefix find_meshes searches)
    tstring name44;                             // +0x44 (0x479e20: the body resource looked up with get_object60)
    char pad54[0x114 - 0x54];
    struct { int a, b; } pairs[64];             // +0x114 spawn pairs (0x473a60): the visitors' troop and face seed (script_interpreter.h SceneRec)
    int visitor_counts[64];                     // +0x314 (set_visitors 0x4a4d75; terrain_a/terrain_emit.cpp)
    char pad414[0x494 - 0x414];
};


// The scene editor's state (+0x1a090..+0x1a1d8): its dialog (options_dlg_proc / agent5/editor_dialog.cpp, the create-game options page
// 0x4b9e70 / 0x4bf640 of config.cpp reads the same words), its camera (agent5/agent_hud.cpp, map_editor.h) and its selection.
struct SceneEditor {
    int active;                                 // +0x1a090 the editor is on (0x472050; restore_defaults 0x4d3a50 clears it)
    union {
        unsigned mode;                          // +0x1a094 0..4: the transform mode (0 = brush) (editor_dialog.cpp)
        int game_mode;                          // +0x1a094 the same radio group on the create-game page (0 campaign, 1 quick, 2 custom, 3 client, 4 host; 0x4b9e70)
    };
    union {
        int rt_flag;                            // +0x1a098 "scale everything" toggle (editor_dialog.cpp; BM_SETCHECK of control 0x434, 0x4bf640)
        int object_index;                       // +0x1a098 the edited map-object index (gui_edit.cpp 0x4c7bf0)
    };
    rgl::vector4 rt;                            // +0x1a09c the edited object's position (gui_edit.cpp: the eye)
    union {
        int rt_extra;                           // +0x1a0ac (editor_dialog.cpp)
        float keep;                             // +0x1a0ac the camera's z (gui_edit.cpp, read by the menu screen)
    };
    int cam_step;                               // +0x1a0b0 camera key-repeat speed: doubles while a key is held (agent_hud.cpp)
    int cam_step_time;                          // +0x1a0b4 the tick of the last step
    union {
        int list_top;                           // +0x1a0b8 top-visible list entry (config.cpp 0x4bb260)
        int list_mode;                          // +0x1a0b8 the list's view / edit mode (gui_edit.cpp 0x4c7bf0)
    };
    int list_sel;                               // +0x1a0bc selected list item (0x4bb260, 0x4c7bf0; the editor's combo box)
    int f1a0c0;                                 // +0x1a0c0 (party/game.cpp 0x51cc80: -1)
    int f1a0c4;                                 // +0x1a0c4 (0x51cc80: -1)
    char flag_c8;                               // +0x1a0c8 (editor_dialog.cpp)
    char pad1a0c9[3];
    int opt_cc;                                 // +0x1a0cc check box 0x453 (0x4bf640)
    int opt_d0;                                 // +0x1a0d0 check box 0x454
    float mx, my, mz;                           // +0x1a0d4 slider 1: [mode 1], [mode 2], [default]
    float sx, sy, sz;                           // +0x1a0e0 slider 2
    float tx, ty, tz;                           // +0x1a0ec slider 3
    rgl::vector4 col;                           // +0x1a0f8
    int axis;                                   // +0x1a108 0/1/2 (radio buttons 0x474..0x476)
    rgl::matrix sel_copy;                       // +0x1a10c copy of a terrain's frame (its +0x18..+0x58); its o is the camera target (agent_hud.cpp)
    char pad1a14c[4];
    Timer timer;                                // +0x1a150 (editor_dialog.cpp; agent_hud.cpp: below zero = not started)
    int dialog_shown;                           // +0x1a160 the editor dialog is shown (inventory_panel/game_app.cpp)
    rgl::matrix cam_frame;                      // +0x1a164 the editor camera's frame (map_editor.h)
    rgl::vector4 cam_pos;                       // +0x1a1a4 the editor camera's position (map_editor.h, agent_hud.cpp; editor_dialog.cpp adds a quad there)
    int sel_index;                              // +0x1a1b4 selected terrain / record (-1 = none)
    std::vector<int> sel;                       // +0x1a1b8 the selected terrain indices (game_app.cpp reads its first / last)
    int list_dirty;                             // +0x1a1c8 the editor's list needs rebuilding (game_app.cpp)
    int dirty;                                  // +0x1a1cc unsaved changes: restore_defaults 0x4d3a50 asks ui_question_save_changes
    union {
        unsigned sub_mode;                      // +0x1a1d0 (editor_dialog.cpp)
        int sub_option;                         // +0x1a1d0 selected sub-option of the host page (0x4b9e70)
    };
    int f1a1d4;                                 // +0x1a1d4 (0x51cc80)
};


struct GameData {
    int version;                                // +0x00 (char_sheet/save_game.cpp 0x4e8e70; save_state.cpp 0x4e8f10 writes +4..)
    int f4;                                     // +0x04 (0x473a60)
    tstring title;                               // +0x08 the save's name (char_sheet/save_game.cpp 0x4e8980)
    union {
        int unk18;                              // +0x18 (char_sheet/save_game.cpp)
        char flag18;                            // +0x18 as a byte (cls_map.cpp, mission_ctx.cpp: set = no camera keys)
    };
    tstring module;                             // +0x1c the module name (module_path 0x4639d0: "Modules\\<name>\\" + sub); also the save slot name: save_game.cpp 0x4e8980 reads its buf at +0x20 (null = the text is inline)
    rgl::mesh* map_track;                       // +0x2c (mission5/game_2.cpp build_scene 0x520d60)
    rgl::entity* track_entity;                  // +0x30
    std::vector<rgl::mesh*> ents;               // +0x34 the map cell meshes (build_map_trees 0x5213d0)
    rgl::mesh* map_obj;                         // +0x44
    rgl::vector4 fog1;                          // +0x48 (0x520d60)
    float fog_color[4];                         // +0x58 the fog colour, passed by value to the colour packer 0x40e440 (char_sheet/screen_camera.cpp 0x4ece20)
    rgl::vector4 fog3;                          // +0x68 (0x520d60)
    union {
        int party_id_78;
        int main_party;                         // +0x78 the player's party (inventory_panel/map_tooltip.cpp; 0x4428c0; entering_town 0x49fb83)
        int side_party;                         // +0x78 (char_sheet/screen_init.cpp)
        int cur_party;                          // +0x78   ui/mission_ui.cpp 0x48a290, 0x48a320 (the party the sl_* queries are asked for)
    };
    int f7c;                                    // +0x7c a party id (inventory_panel/party_screen.cpp; party/game.cpp 0x51cc80)
    int f80;                                    // +0x80 (0x51cc10)
    union {
        int sel_party;                          // +0x84 the party selected on the map (inventory_panel/map_panel.cpp 0x5191e0, 0x51abd0; char_sheet/screen_camera.cpp 0x4ece20 follows it)
        int selected;
        int camera_follow_party;                // +0x84 (set_camera_follow_party 0x4a2afb)
    };
    char map_live;                              // +0x88 the map is live: the camera follows sel_party (0x5191e0, 0x4ece20; set_camera_follow_party 0x4a2afb sets it)

    char pad89[0x8c - 0x89];
    union {
        int player;                             // +0x8c (the player's troop id)   0x460160, 0x460640
        int player_troop;                       //       (set_player_troop 0x49fcb5)
        int cur_troop;                          // +0x8c   ui/mission_ui.cpp 0x48a080, 0x489c50
    };
    int hero_troop;                             // +0x90 the troop the inventory screens compare with (party_window 0x502320; 0x51cc10 sets 1)
    char f94;                                   // +0x94 (0x51cc80)
    char reveal_all;                            // +0x95 every party shows on the map (the cheat key ctrl+T toggles it, GameG::tick 0x516ac0;
                                                //   map_panel.cpp 0x5191e0, GameG::update_cursor 0x5179f0)
    char pad96[0x98 - 0x96];
    int cnt_killed;                             // +0x98 0x457b40 (kills of the player; get_player_agent_kill_count 0x4a89e6)
    int cnt_killed_friend;                      // +0x9c 0x457b40 (own troops killed)
    int cnt_ko;                                 // +0xa0 0x457b40 (knock-outs by the player)
    int cnt_ko_friend;                          // +0xa4 0x457b40
    unsigned encounter_flags;                   // +0xa8 bit 0 = leave (leave_encounter 0x4a5589)
    int background_presentation;                // +0xac (start_background_presentation 0x4a2121)
    union {
        int fb0;                                // +0xb0   0x4610b0
        unsigned encounter_flags2;              // +0xb0 bit 6 = attack (encounter_attack 0x4a559a)
    };
    int fb4;                                    // +0xb4 (char_sheet/cs_update.cpp: set from the profile's +0x50; nonzero scales the party value by 0.75, ui/mission_ui.cpp 0x489f40)
    tstring nameb8;                             // +0xb8 (party/game.cpp 0x51cfa0)
    union {
        int mode;                               // +0xc8 screen mode (0x12 = party screen)   ui/mission_ui.cpp 0x48add0, 0x460620, 0x460d80
        int next_window;                        // +0xc8 window to switch to (0xf = presentation; start_presentation 0x4a20d5)
    };
    Pool<Event> events;                         // +0xcc the map event / battle records (game/event.h), pages of 16 at +0xe4: alloc 0x493700,
                                                // 0x48ef20 (battle_rec.h BattleTable::get), map_panel.cpp 0x5191e0 / 0x51abd0, 0x49bbb0, 0x49e9c0
    PartyMgr parties;                           // +0xf8 .. +0x139dc (gui_edit.cpp called it `pm`)
    Light* wind;                                // +0x139dc the map's sun light (mission5/game_2.cpp build_scene 0x520d60)
    int f139e0;                                 // +0x139e0
    int f139e4;                                 // +0x139e4 (0x443aa0)
    TriggerTag triggers;                        // +0x139e8 the module triggers (count, array, 1)
    SimpleTag simple;                           // +0x139f4 the simple triggers
    union {
        Troop* troops;                          // +0x13a00 (stride 0x508)
        TroopRec* chars;                        // +0x13a00   0x460160, 0x4428c0
        Troop* records;                         // +0x13a00 (char_sheet/save_state.cpp 0x4e97e0)
        char* chars_c;                          // +0x13a00 as bytes (inventory_panel/view_open.cpp, char_sheet/facegen*.cpp: records are indexed by 0x508)
        TroopX* troops_x;                // +0x13a00 (script/script_interpreter.h: the interpreter's names)
    };
    union {
        int ntroops;                            // +0x13a04
        int n_chars;
        int nrecords;                           // char_sheet/save_state.cpp
        int num_troops;                         // (check_troop 0x49cee0)
    };
    int f13a08;                                 // +0x13a08 (mission_ctx.cpp; 0x51cc80)
    union {
        int excluded;                           // +0x13a0c record index skipped by sl_roll_other (ui/mission_ui.cpp 0x489ee0)
        int attach_party;                       // +0x13a0c the party the join point belongs to (deploy_party 0x49f060)
        int encountered_party;                  // +0x13a0c (store_encountered_party 0x4ad248)
    };
    union {
        int attach_scene;                       // +0x13a10 its scene object index, -100000 = none (deploy_party)
        int encountered_party2;                 // +0x13a10 (store_encountered_party2 0x4ad258)
    };
    union {
        int attach_flip;                        // +0x13a14 1 when the player is the second party of the join (deploy_party)
        int encountered_party_is_attacker;      // +0x13a14 (0x49fc5f)
    };
    char f13a18;                                // +0x13a18 (mission_ctx.cpp; 0x51cc80)
    char pad13a19[0x13a1c - 0x13a19];
    int repeat_object;                          // +0x13a1c current repeat index (store_repeat_object 0x49fcc5)   0x460d80, 0x460de0
    int f13a20;                                 // +0x13a20 (char_sheet/save_state.cpp; 0x51cc80: 20)
    int f13a24;                                 // +0x13a24 (0x51cc80)
    int f13a28;                                 // +0x13a28 (0x51cc80)
    int f13a2c;                                 // +0x13a2c (0x51cc80)
    int mercenary_source_party;                 // +0x13a30 (set_mercenary_source_party 0x4a5629)
    int f13a34;                                 // +0x13a34   0x4428c0
    float merchandise_modifier_quality;         // +0x13a38 item quality for generated loot (0x48cc40: passed to ItemRef::randomize_modifier; 0x4a566c)
    char start_map_conversation;                // +0x13a3c (start_map_conversation 0x4a2b33)
    char pad13a3d[0x13a40 - 0x13a3d];
    int map_conversation_mode;                  // +0x13a40
    int map_conversation_troop;                 // +0x13a44
    union {
        FactionRec* factions;                   // +0x13a48   0x4c8ca0
        Rec300* f13a48;                         // +0x13a48 (char_sheet/save_state.cpp: the 0x300-byte records as its writer sees them)
        float* rel;                             // +0x13a48 (inventory_panel/map_tooltip.cpp: 0xc0 floats per troop)
    };
    union {
        int n_factions;                         // +0x13a4c
        int n13a4c;
        int num_factions;                       // (check_faction 0x49cf80)
    };
    union {
        Level494* levels;                       // +0x13a50 (the scene / level table)
        Level494* slots;
        RecB0* party_recs;                      // +0x13a50 (char_sheet/save_state.cpp: the 0x494-byte records, a List8 at +0x474)
        SceneFile* scenes;                              // +0x13a50 (script/script_interpreter.h: the same record)
    };
    union {
        int nparties;                           // +0x13a54 (char_sheet/save_state.cpp write_list_party 0x4e9670)
        int num_scenes;                         // +0x13a54 (check_scene 0x49d020; "Number of scenes in file", 0x51e240)
    };
    union {
        QuestRec* quests;                       // +0x13a58   0x4c8ca0
        Rec138* f13a58;                         // +0x13a58 (char_sheet/save_state.cpp)
    };
    union {
        int n_quests;                           // +0x13a5c
        int n13a5c;
        int num_quests;                         // (check_quest 0x49cf30)
    };
    std::vector<Event92> tracks;                // +0x13a60 the event / fleet / track records (std::vector<Event92>: allocator word at +0x13a60, first +0x13a64, last +0x13a68, end +0x13a6c; party2 push_event 0x4b5fe0, push_party_event 0x4b6070; party/game.h Game::tracks; save_state.cpp write_vec5c 0x4e93f0)
    // The vector's storage under the names the map panel / save writer / map tooltip still read it by (their .cpp files
    // belong to other regions); the accessors keep those users compiling until they move to tracks.
    char*& fleet_slot() { return (char*&)tracks._Myfirst; }              // +0x13a64 first of the 0x5c-byte fleet records
    char*& fleet_end_slot() { return (char*&)tracks._Mylast; }          // +0x13a68
    char*& fleet_cap_slot() { return (char*&)tracks._Myend; }           // +0x13a6c
    PartyInfo*& party_info_slot() { return (PartyInfo*&)tracks._Myfirst; }   // the same first pointer as PartyInfo* (map_tooltip.cpp)
    tstring t13a70;                             // +0x13a70 (char_sheet/save_state.cpp write_clock: w_struct)
    union {
        Timer timer13a80;                       // +0x13a80 (party/game.cpp 0x51cfa0: clock 1)
        struct {
            char pad13a80[8];
            int f13a88;                         // +0x13a88 (char_sheet/save_state.cpp: low word of the 64-bit stamp)
        };
    };
    Timer timer13a90;                           // +0x13a90 (party2/party_2.cpp Party::tick 0x4b80d0: the main party's re-plan interval)
    int f13aa0;                                 // +0x13aa0 (char_sheet/save_state.cpp; party/game.cpp reads a bool)
    int f13aa4;                                 // +0x13aa4 (inventory_panel/mission_ctx.cpp: 0, 1, 2)
    tstring t13aa8;                             // +0x13aa8 (0x51cfa0)
    tstring t13ab8;                             // +0x13ab8 (0x51cfa0)
    char f13ac8;                                // +0x13ac8 (mission_ctx.cpp)
    char pad13ac9[0x13acc - 0x13ac9];
    float f13acc;                               // +0x13acc scaled zoom (char_sheet/screen_init.cpp 0x4ecce0)
    union {
        int f13ad0;                             // +0x13ad0 (char_sheet/save_state.cpp write_clock 0x4e9190)
        float rest_period;                      // +0x13ad0 hours (rest_for_hours 0x4a2b49; deploy_party 0x49f060: > 0 with rest_attackable set refuses a join)
    };
    int rest_attackable;                        // +0x13ad4 0 = remain attackable (write_clock 0x4e9190)
    int rest_interactive;                       // +0x13ad8
    int rest_time_speed;                        // +0x13adc
    int range_lo;                               // +0x13ae0 random integer range of the party-template roll   ui/mission_ui.cpp 0x48a150 (set_party_creation_random_limits 0x4a2d7f)
    int range_hi;                               // +0x13ae4
    int f13ae8;                                 // +0x13ae8 (char_sheet/save_state.cpp write_clock 0x4e9190)
    int current_menu;                           // +0x13aec (jump_to_menu 0x4ac38b)
    int screen_troop;                           // +0x13af0 troop of the loot / trade / equip screens (0x4ac0a1)
    int jump_scene;                             // +0x13af4 (jump_to_scene 0x4aa696)
    int jump_entry;                             // +0x13af8 (set_jump_entry 0x4aa6e5)
    int jump_mission;                           // +0x13afc (set_jump_mission 0x4aa6a7)
    int screen_party;                           // +0x13b00 (change_screen_exchange_with_party 0x4ac21d)
    int presentation;                           // +0x13b04 presentation to start (start_presentation 0x4a20e8)
    int notes_type;                             // +0x13b08 (change_screen_notes 0x4ac312; inventory_panel/view_open.cpp)
    int notes_object;                           // +0x13b0c
    char talk_enabled;                          // +0x13b10 (mission_enable_talk 0x4aa721)
    char talk_disabled;                         // +0x13b11 (mission_disable_talk 0x4aa732)
    char pad13b12[0x13b18 - 0x13b12];
    union {
        Timer game_clock;                       // +0x13b18 the game clock (display_log_message 0x4a2edc; Timer::elapsed 0x432c10)
        struct {
            int f13b18;                         // +0x13b18 its clock index (8-byte entries) (0x4792d0)
            int pad13b1c;
            union {
                struct {
                    unsigned f13b20;            // +0x13b20 low word of the start tick
                    int f13b24;                 // +0x13b24 high word
                };
                __int64 clock;                  // +0x13b20 (char_sheet/save_game.cpp)
            };
        };
    };
    int current_hours;                          // +0x13b28 (store_current_hours 0x4ad57e)
    int string_count;                           // +0x13b2c the string counter the report pages compare ids against (inventory_panel/note_window.cpp 0x50a1c0); the current day (0x4a3330)
    int f13b30;                                 // +0x13b30 (char_sheet/save_state.cpp write_clock)
    int f13b34;                                 // +0x13b34 (char_sheet/save_state.cpp write_clock)
    int f13b38;                                 // +0x13b38 (char_sheet/save_state.cpp write_clock)
    union {
        float cloud_amount;                     // +0x13b3c 0..1 (get_global_cloud_amount 0x49fecb; char_sheet/screen_camera.cpp)
        int i13b3c;                             // (char_sheet/save_state.cpp)
    };
    union {
        float haze_amount;                      // +0x13b40 (get_global_haze_amount 0x49ff46)
        int i13b40;                             // (char_sheet/save_state.cpp)
    };
    union {
        int f13b44;                             // +0x13b44 (char_sheet/save_state.cpp)
        float avg_value;                        // +0x13b44 running party value   ui/mission_ui.cpp 0x48a000 (get_average_game_difficulty 0x4a2a51)
    };
    union {
        int f13b48;                             // +0x13b48 (char_sheet/save_state.cpp)
        float avg_weight;                       // +0x13b48
    };
    float avg_prev_time;                        // +0x13b4c
    tstring message_log;                        // +0x13b50 "|time|colour|text" records (display_log_message 0x4a2ec9; char_sheet/save_state.cpp)
    int f13b60;                                 // +0x13b60 (char_sheet/save_state.cpp)
    float val[4][4];                            // +0x13b64 four wind anchors: four weights / values each (0x471040)
    float pos[4][2];                            // +0x13ba4 their xy positions
    int n_labels;                               // +0x13bc4 the context-menu / map label count (map_panel.cpp 0x5191e0, 0x51abd0; context_menu_add_item 0x4a29ed)
    int cur_label;                              // +0x13bc8
    tstring labels[50];                         // +0x13bcc the map label / context-menu texts (0x10 each)
    int label_target[50];                       // +0x13eec the party id of each label (map_panel.cpp; context_menu_add_item)
    rgl::vector4 current_color;                 // +0x13fb4 (set_current_color 0x4aa91e; world_data.cpp 0x4952f0)
    rgl::vector4 position_delta;                // +0x13fc4 (set_position_delta 0x4aa974)
    char pad13fd4[0x14014 - 0x13fd4];
    rgl::matrix trigger_frame;                  // +0x14014 the trigger frame: the spawn / placement frame of the script's trigger block (set_spawn_position 0x4aac92; party2/camera.cpp 0x49b4b0, 0x49f980; set_matrix 0x435650)
    char menu_option_disabled;                  // +0x14054 (disable_menu_option 0x4ac3b9)
    char pad14055[0x14058 - 0x14055];
    float f14058;                               // +0x14058 (inventory_panel/mission_ctx.cpp; 0x51cc80)
    int stack_seeds[64][32];                      // +0x1405c per stack, the face seed of each member: rows move with the stacks (0x473a60, 0x4a4490; party/game.cpp 0x51eaa0)
    int stack_seeds_side[2][64][32];              // +0x1605c, +0x1805c the same for the two sides of a battle (inventory_panel/mission_ctx.cpp)
    char scene_open;                            // +0x1a05c the scene arrays are allocated (free_arrays 0x492a20 clears it, world_free 0x492f40 tests it)
    char pad1a05d[0x1a060 - 0x1a05d];
    ScriptDebugger debugger;                    // +0x1a060
    SceneEditor editor;                         // +0x1a090
    int f1a1d8;                                 // +0x1a1d8 (char_sheet/save_state.cpp write_clock)
    int f1a1dc;                                 // +0x1a1dc (char_sheet/save_state.cpp write_clock)
    int f1a1e0;                                 // +0x1a1e0 (char_sheet/save_state.cpp write_clock)
    int f1a1e4;                                 // +0x1a1e4 (char_sheet/save_state.cpp write_clock)
    int f1a1e8;                                 // +0x1a1e8 (char_sheet/save_state.cpp write_clock)
    int f1a1ec;                                 // +0x1a1ec (char_sheet/save_state.cpp write_clock)
    EventContext event_ctx;                     // +0x1a1f0
    int question_answer;                        // +0x1a208 (question_box 0x4a3ca4)
    int f1a20c;                                 // +0x1a20c
    tstring background_mesh;                    // +0x1a210 the game menu's mesh override (set_background_mesh 0x4abfc8; MenuRec::mesh, menus.txt
                                                //         +0x28, copied here by GameScreen::build; constructed by the save
                                                //         block reader Block::read 0x51d100, reset at 0x492f6f)
    int menu_tableau;                           // +0x1a220 (set_game_menu_tableau_mesh 0x4ac02f)
    int menu_tableau_value;                     // +0x1a224
    rgl::vector4 menu_tableau_pos;              // +0x1a228 (x, y; w is the scale: agent_hud.cpp)
    int talk_relation_bar;                      // +0x1a238 (talk_info_set_relation_bar 0x4abeb6)
    int talk_info_shown;                        // +0x1a23c (talk_info_show 0x4abea6)   0x4610b0
    tstring talk_lines[5];                      // +0x1a240 (0x51cc80, 0x51cfa0)
    char talk_line_set[5];                      // +0x1a290 (talk_info_set_line 0x4abf70)
    char pad1a295[0x1a298 - 0x1a295];
    Timer timer1a298;                           // +0x1a298 (0x51cfa0: clock 1)
    rgl::entity* compass;                       // +0x1a2a8 (mission5/game_2.cpp 0x520d60)
    char pad1a2ac[0x1a2b0 - 0x1a2ac];

    int skill_level(int troop, int skill);      // 0x437b20 (thiscall, ret 8)  // old name of GameData::skill_level, removed after the names merge
    void step(float dt);                        // 0x427f40 (char_sheet/cs_update.cpp)
    // ---- char_sheet/save_game.cpp, save_state.cpp (the save-game serialiser; the stream is the binary writer OutStream) ----
    void load_header(SaveCtx* h);               // 0x4e8e70 (ret 4)
    int save_state(OutStream* out);             // 0x4e8f10
    void free_arrays();                         // 0x492a20 (deletes the troop, level, faction and quest arrays)
    void world_free();                          // 0x492f40 (the destructor body of the game object: the caller deletes the storage)
    __declspec(noinline) int  write_clock(OutStream* out);               // 0x4e9190
    __declspec(noinline) void write_vec5c(OutStream* out);               // 0x4e93f0
    __declspec(noinline) void write_list138(OutStream* out);             // 0x4e9550
    __declspec(noinline) void write_list_party(OutStream* out);          // 0x4e9670
    __declspec(noinline) void write_list300(OutStream* out);             // 0x4e96c0
    __declspec(noinline) void write_records(OutStream* out);             // 0x4e97e0
    __declspec(noinline) void write_items(OutStream* out);               // 0x4e9790
    void write_missions(OutStream* out); // 0x4e9b50
    float time_of_day();                        // 0x4356e0 (agent5/agent_hud.cpp: the sun pass reads it through the game object; agent/agent.h time_of_day is the same code)
    char emit_event(int id, int a, int b);      // 0x435790 (ret 0xc)
    void set_matrix(const rgl::matrix* m);      // 0x435650 (agent4: sound-event context frame)
    void add_experience(int troop, int amount); // 0x4428c0 (ret 8) (agent_fx_sound.cpp)
    void party_counts(unsigned id, int* num, int* num2, int* prisoners);   // 0x51be60 (inventory_panel/map_tooltip.cpp, called by map_panel.cpp 0x51abd0 as (id, &size, &wounded, &prisoners))
    bool agent_low(int idx);                    // 0x4638f0 (thiscall, ret 4: the hit points of troop idx are below the agent-low threshold)
    tstring scene_path(int idx);                // 0x463950 (ret 8: hidden return)
    const char* module_path(const char* sub);   // 0x4639d0
    void load_scene(int idx);                   // 0x47fbd0 (mesh_slot/mesh_char.cpp: reads the scene object file of level idx)
    void blend(float* out, float x, float y);   // 0x471040 (wind / sky blend over the anchors)
    bool ground_height(rgl::world* lv, const float* p, float* out);   // 0x470f40 (ignores this: blend loads ecx=this, 0x4ed410 ecx=g_game)
    void save_language_files(bool translated);  // 0x4c8ca0
    // ---- src/game/party2 (method declarations only; fields at the offsets noted are reached by offset there) ----
    struct face* find_at(const float* p);       // 0x48ea80 (nav-mesh face under a map position; other object)
    void init_event(struct E92* r);             // 0x4b36a0 (ret 4: orient an event record on the map)
    float party_value(unsigned id);             // 0x4b4800 (ret 4)
    void refresh_party(unsigned id);            // 0x4b4700 (ret 4)
    void push_event(const int* pos, unsigned nation);       // 0x4b5fe0 (ret 8: events vector at +0x13a60)
    void push_party_event(int party, const int* pos, const float* heading);   // 0x4b6070 (ret 0xc)
    unsigned color_of(float relation, int flag);           // 0x4b1930 (ret 8: relation colour ramp)
    void count_units(unsigned id, int* a, int* b, int* c, int* d);   // 0x490ce0 (ret 0x14: world_data/world_data.cpp)
    void list_members(unsigned id, int* out, int* n);          // 0x490fa0 (ret 0xc: the party and its attached parties)
    bool is_night();                            // 0x435740 (the slices' not_afternoon)
    int remove_troops(int party, int troop, int count, char prisoners, int a5);   // 0x48f9f0 (ret 0x14: count removed)
    float relation(int a, int b);               // 0x48ffa0 (ret 8: party relation, -1..1)
    void detach_party(int id); // 0x490a30
    void remove_party(int party);               // 0x4915d0 (ret 4)
    void label_text(rgl::mesh* m, const tstring* text, char flag, int arg, const float* h);   // 0x49baa0 (ret 0x14: lays text out into a label mesh)
    void spawn_battle_icon(unsigned id);        // 0x49bbb0 (ret 4: the map icon entity of event `id`)
    void add_order_arrow(int id);               // 0x49bea0 (ret 4: the map entity, shape and label of party `id`)
    bool deploy_party(int a, int b);            // 0x49f060 (ret 8)
    void add_gold(int gold, int troop);         // 0x49b720 (ret 8: adds to the troop's gold (+0x3b4) and, for the player, logs ui_you_got_value)
    void remove_gold(int gold, int troop);      // 0x49b820 (ret 8: subtracts (clamped at 0) and logs ui_you_lost_value)
    void start_quest(int quest, int giver_troop);   // 0x49b930 (ret 8: stamps a quest record as started by the giver, logs ui_new_quest_taken)
    void fill_parties(int id, struct Party* out);   // 0x48fd60 (ret 8: merges the party and its attached parties into `out`)
    // ---- src/game/ui (mission_ui.cpp; the slices called the object World, `sl_query` is emit_event 0x435790) ----
    tstring get_name();                         // 0x489c30 (hidden return pointer, ret 4: the string at +0xe028)
    int count_in(int idx, int id, char flag);   // 0x489c50
    int sl_take(int group, int item, int want, char extras);   // 0x489d10
    int sl_roll_party(int lo, int hi);          // 0x489e70
    int sl_roll_other(int lo, int hi);          // 0x489ee0
    int sl_value_factor();                      // 0x489f40 (truncated to int)
    void sl_update_avg();                       // 0x48a000
    int sl_pick_chain(int leader, char lancer); // 0x48a080 (walks the +0x50 link chain from leader)
    void sl_item_flag_all();                    // 0x48a210 (no use of this: the callers load ecx with g_game)
    void sl_item_set_all(float v);              // 0x48a250 (likewise)
    int sl_party_value(int party);              // 0x48a290
    int sl_party_price(int party);              // 0x48a320
    void set_names(int slot, const tstring* a); // 0x48a380
    // ---- src/game/mission_ui2 (method declarations only) ----
    tstring event_string(int id);               // 0x48c470 (ret 8: hidden return; runs emit_event(0xf, id / 6, id), returns the result string 0xa70464)
    void init_slots(int troop, int want);       // 0x48c4c0 (ret 8: frees inventory slots of a troop until `want` are free, then compacts)
    void give_item(int troop, int item, int count);   // 0x48c9b0 (ret 0xc)
    int pick_random_item(int t0, int t1, int t2, int t3);   // 0x48ca30 (ret 0x10)
    void spawn_items(int type, int want, int troop);   // 0x48cc40 (ret 0xc)
    int add_members(int party, int troop, int count, char prisoners);   // 0x48f8f0 (ret 0x10: returns how many members were added)
    void add_template(int party, int tmpl, int reverse);   // 0x48fbd0 (ret 0xc: fills a party from a party template's six slots)
    int battle_root(int party);                 // 0x48fce0 (ret 4: the end of the attached-to chain)
    // ---- src/game/party2/camera.cpp (0x49e9c0..0x49f020): battle records and the parties that stand for them ----
    int attach_party_center(const float* pt, int a, int b);              // 0x49e9c0 (ret 0xc: a new battle record at (pt[0], pt[1]) for parties a and b; its id)
    int add_sprite(Vec2 pos, int tmpl);                                  // 0x49ebd0 (ret 0xc: a new party of party template tmpl at pos; its id)
    int create_companion_party(int troop, float x, float y, float z);    // 0x49ef00 (ret 0x14: a party named after the troop; its id)
    int spawn_around(Vec2 pos, int tmpl, float radius);                  // 0x49f020 (ret 0x10: add_sprite at a random position around pos)
    void collect_attachments(int party, Party* dest);   // 0x48ff70 (ret 8)
    int template_count(int tmpl, int slot, float t);   // 0x48a150 (ret 0xc: member count of a party template slot; rolls between range_lo..range_hi percent of the slot's min..max; the third word is unused: ui/mission_ui.cpp)
    int count_troops(int troop, int party, char prisoners);   // 0x48f790 (ret 0xc: members of `troop` in the party's regular (prisoners 0) or prisoner stacks)
    tstring string_value(int index);            // 0x48e9e0 (ret 8: hidden return; the script string `index` of the current text table, or its fallback)
    void add_inventory(int item, int amount);   // 0x48eb50 (ret 8: the current troop receives an item; a unique item displaces a plain one)
};
extern GameData* g_game;                        // 0x5e2930
// The legacy names of tracks' storage, still read by map_panel.cpp / save_state.cpp / map_tooltip.cpp (other regions'
// files). They are the same four dwords; remove these when those files move to tracks.
#define fleet fleet_slot()
#define fleet_end fleet_end_slot()
#define fleet_cap fleet_cap_slot()
#define party_info party_info_slot()
// ---- layout checks (32-bit x86): every named field at its offset ----
MB_LAYOUT32_ASSERT(gd_map_track, offsetof(GameData, map_track) == 0x2c);
MB_LAYOUT32_ASSERT(gd_ents, offsetof(GameData, ents) == 0x34);
MB_LAYOUT32_ASSERT(gd_map_obj, offsetof(GameData, map_obj) == 0x44);
MB_LAYOUT32_ASSERT(gd_fog1, offsetof(GameData, fog1) == 0x48);
MB_LAYOUT32_ASSERT(gd_fog_color, offsetof(GameData, fog_color) == 0x58);
MB_LAYOUT32_ASSERT(gd_fog3, offsetof(GameData, fog3) == 0x68);
MB_LAYOUT32_ASSERT(gd_main_party, offsetof(GameData, main_party) == 0x78);
MB_LAYOUT32_ASSERT(gd_f7c, offsetof(GameData, f7c) == 0x7c);
MB_LAYOUT32_ASSERT(gd_sel_party, offsetof(GameData, sel_party) == 0x84);
MB_LAYOUT32_ASSERT(gd_map_live, offsetof(GameData, map_live) == 0x88);
MB_LAYOUT32_ASSERT(gd_player, offsetof(GameData, player) == 0x8c);
MB_LAYOUT32_ASSERT(gd_hero_troop, offsetof(GameData, hero_troop) == 0x90);
MB_LAYOUT32_ASSERT(gd_reveal_all, offsetof(GameData, reveal_all) == 0x95);
MB_LAYOUT32_ASSERT(gd_cnt_killed, offsetof(GameData, cnt_killed) == 0x98);
MB_LAYOUT32_ASSERT(gd_encounter_flags, offsetof(GameData, encounter_flags) == 0xa8);
MB_LAYOUT32_ASSERT(gd_fb0, offsetof(GameData, fb0) == 0xb0);
MB_LAYOUT32_ASSERT(gd_nameb8, offsetof(GameData, nameb8) == 0xb8);
MB_LAYOUT32_ASSERT(gd_mode, offsetof(GameData, mode) == 0xc8);
MB_LAYOUT32_ASSERT(gd_events, offsetof(GameData, events) == 0xcc);
MB_LAYOUT32_ASSERT(gd_parties, offsetof(GameData, parties) == 0xf8);
MB_LAYOUT32_ASSERT(gd_f139e4, offsetof(GameData, f139e4) == 0x139e4);
MB_LAYOUT32_ASSERT(gd_triggers, offsetof(GameData, triggers) == 0x139e8);
MB_LAYOUT32_ASSERT(gd_simple, offsetof(GameData, simple) == 0x139f4);
MB_LAYOUT32_ASSERT(gd_troops, offsetof(GameData, troops) == 0x13a00);
MB_LAYOUT32_ASSERT(gd_f13a08, offsetof(GameData, f13a08) == 0x13a08);
MB_LAYOUT32_ASSERT(gd_attach_party, offsetof(GameData, attach_party) == 0x13a0c);
MB_LAYOUT32_ASSERT(gd_f13a18, offsetof(GameData, f13a18) == 0x13a18);
MB_LAYOUT32_ASSERT(gd_repeat_object, offsetof(GameData, repeat_object) == 0x13a1c);
MB_LAYOUT32_ASSERT(gd_mercenary_source_party, offsetof(GameData, mercenary_source_party) == 0x13a30);
MB_LAYOUT32_ASSERT(gd_merchandise_modifier_quality, offsetof(GameData, merchandise_modifier_quality) == 0x13a38);
MB_LAYOUT32_ASSERT(gd_start_map_conversation, offsetof(GameData, start_map_conversation) == 0x13a3c);
MB_LAYOUT32_ASSERT(gd_map_conversation_mode, offsetof(GameData, map_conversation_mode) == 0x13a40);
MB_LAYOUT32_ASSERT(gd_factions, offsetof(GameData, factions) == 0x13a48);
MB_LAYOUT32_ASSERT(gd_levels, offsetof(GameData, levels) == 0x13a50);
MB_LAYOUT32_ASSERT(gd_quests, offsetof(GameData, quests) == 0x13a58);
MB_LAYOUT32_ASSERT(gd_tracks, offsetof(GameData, tracks) == 0x13a60);
MB_LAYOUT32_ASSERT(gd_t13a70, offsetof(GameData, t13a70) == 0x13a70);
MB_LAYOUT32_ASSERT(gd_f13a88, offsetof(GameData, f13a88) == 0x13a88);
MB_LAYOUT32_ASSERT(gd_timer13a90, offsetof(GameData, timer13a90) == 0x13a90);
MB_LAYOUT32_ASSERT(gd_f13aa0, offsetof(GameData, f13aa0) == 0x13aa0);
MB_LAYOUT32_ASSERT(gd_t13aa8, offsetof(GameData, t13aa8) == 0x13aa8);
MB_LAYOUT32_ASSERT(gd_f13ac8, offsetof(GameData, f13ac8) == 0x13ac8);
MB_LAYOUT32_ASSERT(gd_f13acc, offsetof(GameData, f13acc) == 0x13acc);
MB_LAYOUT32_ASSERT(gd_rest_period, offsetof(GameData, rest_period) == 0x13ad0);
MB_LAYOUT32_ASSERT(gd_range_lo, offsetof(GameData, range_lo) == 0x13ae0);
MB_LAYOUT32_ASSERT(gd_current_menu, offsetof(GameData, current_menu) == 0x13aec);
MB_LAYOUT32_ASSERT(gd_screen_party, offsetof(GameData, screen_party) == 0x13b00);
MB_LAYOUT32_ASSERT(gd_notes_type, offsetof(GameData, notes_type) == 0x13b08);
MB_LAYOUT32_ASSERT(gd_talk_enabled, offsetof(GameData, talk_enabled) == 0x13b10);
MB_LAYOUT32_ASSERT(gd_game_clock, offsetof(GameData, game_clock) == 0x13b18);
MB_LAYOUT32_ASSERT(gd_f13b20, offsetof(GameData, f13b20) == 0x13b20);
MB_LAYOUT32_ASSERT(gd_clock, offsetof(GameData, clock) == 0x13b20);
MB_LAYOUT32_ASSERT(gd_current_hours, offsetof(GameData, current_hours) == 0x13b28);
MB_LAYOUT32_ASSERT(gd_cloud_amount, offsetof(GameData, cloud_amount) == 0x13b3c);
MB_LAYOUT32_ASSERT(gd_message_log, offsetof(GameData, message_log) == 0x13b50);
MB_LAYOUT32_ASSERT(gd_val, offsetof(GameData, val) == 0x13b64);
MB_LAYOUT32_ASSERT(gd_n_labels, offsetof(GameData, n_labels) == 0x13bc4);
MB_LAYOUT32_ASSERT(gd_labels, offsetof(GameData, labels) == 0x13bcc);
MB_LAYOUT32_ASSERT(gd_label_target, offsetof(GameData, label_target) == 0x13eec);
MB_LAYOUT32_ASSERT(gd_current_color, offsetof(GameData, current_color) == 0x13fb4);
MB_LAYOUT32_ASSERT(gd_position_delta, offsetof(GameData, position_delta) == 0x13fc4);
MB_LAYOUT32_ASSERT(gd_trigger_frame, offsetof(GameData, trigger_frame) == 0x14014);
MB_LAYOUT32_ASSERT(gd_menu_option_disabled, offsetof(GameData, menu_option_disabled) == 0x14054);
MB_LAYOUT32_ASSERT(gd_f14058, offsetof(GameData, f14058) == 0x14058);
MB_LAYOUT32_ASSERT(gd_stack_seeds, offsetof(GameData, stack_seeds) == 0x1405c);
MB_LAYOUT32_ASSERT(gd_stack_seeds_side, offsetof(GameData, stack_seeds_side) == 0x1605c);
MB_LAYOUT32_ASSERT(gd_scene_open, offsetof(GameData, scene_open) == 0x1a05c);
MB_LAYOUT32_ASSERT(gd_debugger, offsetof(GameData, debugger) == 0x1a060);
MB_LAYOUT32_ASSERT(gd_debugger_text, offsetof(GameData, debugger.text) == 0x1a080);
MB_LAYOUT32_ASSERT(gd_editor, offsetof(GameData, editor) == 0x1a090);
MB_LAYOUT32_ASSERT(gd_editor_rt, offsetof(GameData, editor.rt) == 0x1a09c);
MB_LAYOUT32_ASSERT(gd_editor_list_sel, offsetof(GameData, editor.list_sel) == 0x1a0bc);
MB_LAYOUT32_ASSERT(gd_editor_mx, offsetof(GameData, editor.mx) == 0x1a0d4);
MB_LAYOUT32_ASSERT(gd_editor_axis, offsetof(GameData, editor.axis) == 0x1a108);
MB_LAYOUT32_ASSERT(gd_editor_sel_copy, offsetof(GameData, editor.sel_copy) == 0x1a10c);
MB_LAYOUT32_ASSERT(gd_editor_timer, offsetof(GameData, editor.timer) == 0x1a150);
MB_LAYOUT32_ASSERT(gd_editor_dialog_shown, offsetof(GameData, editor.dialog_shown) == 0x1a160);
MB_LAYOUT32_ASSERT(gd_editor_cam_frame, offsetof(GameData, editor.cam_frame) == 0x1a164);
MB_LAYOUT32_ASSERT(gd_editor_cam_pos, offsetof(GameData, editor.cam_pos) == 0x1a1a4);
MB_LAYOUT32_ASSERT(gd_editor_sel_index, offsetof(GameData, editor.sel_index) == 0x1a1b4);
MB_LAYOUT32_ASSERT(gd_editor_sel, offsetof(GameData, editor.sel) == 0x1a1b8);
MB_LAYOUT32_ASSERT(gd_editor_dirty, offsetof(GameData, editor.dirty) == 0x1a1cc);
MB_LAYOUT32_ASSERT(gd_editor_sub_mode, offsetof(GameData, editor.sub_mode) == 0x1a1d0);
MB_LAYOUT32_ASSERT(gd_f1a1d8, offsetof(GameData, f1a1d8) == 0x1a1d8);
MB_LAYOUT32_ASSERT(gd_event_ctx, offsetof(GameData, event_ctx) == 0x1a1f0);
MB_LAYOUT32_ASSERT(gd_event_ctx_mesh, offsetof(GameData, event_ctx.mesh) == 0x1a200);
MB_LAYOUT32_ASSERT(gd_question_answer, offsetof(GameData, question_answer) == 0x1a208);
MB_LAYOUT32_ASSERT(gd_background_mesh, offsetof(GameData, background_mesh) == 0x1a210);
MB_LAYOUT32_ASSERT(gd_menu_tableau_pos, offsetof(GameData, menu_tableau_pos) == 0x1a228);
MB_LAYOUT32_ASSERT(gd_talk_info_shown, offsetof(GameData, talk_info_shown) == 0x1a23c);
MB_LAYOUT32_ASSERT(gd_talk_lines, offsetof(GameData, talk_lines) == 0x1a240);
MB_LAYOUT32_ASSERT(gd_talk_line_set, offsetof(GameData, talk_line_set) == 0x1a290);
MB_LAYOUT32_ASSERT(gd_timer1a298, offsetof(GameData, timer1a298) == 0x1a298);
MB_LAYOUT32_ASSERT(gd_compass, offsetof(GameData, compass) == 0x1a2a8);
MB_LAYOUT32_ASSERT(gd_size, sizeof(GameData) == 0x1a2b0);
MB_LAYOUT32_ASSERT(gd_level494, sizeof(Level494) == 0x494 && offsetof(Level494, visitor_counts) == 0x314);

#endif // MB_GAME_DATA_H
