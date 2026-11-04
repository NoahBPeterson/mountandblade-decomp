// screen_records.h: the head_rec records that ARE the screens (vtables 0x595740..0x596c68): a head_rec whose slot 2 builds the
// screen's windows and whose slot 3 is its per-frame / event handler. The slice tooling had named each half by the screen it
// thought it belonged to (MessageMenu, LoadingScreen, dev_list, ...) and tied them to the record only through the vtable;
// the layouts agree, so each record is ONE class: its constructor and destructor sit with the other head_rec records
// (rgl/mesh2/rec_tables.cpp, 0x4c2c50..0x4c3300) and its virtual overrides with the screen code they came from, each
// annotated `// @ 0xADDR` in the TU that holds the original. One definition here, for every one of those TUs.
// See docs/standalone_vtables.md (ResultWindow, game/result_window.h, is the pilot).
#ifndef MB_GAME_SCREEN_RECORDS_H
#define MB_GAME_SCREEN_RECORDS_H
#include "./head_rec.h"
#include "./ui_widget.h"          // MissionUi: the engine's widget interface (the screens' buttons are reached through it)

struct ContainerBase;
struct MissionUi;
struct res_rec;
struct UiA; struct NodeA; struct Camera;
struct UiB;
struct DropDown;
struct bin_reader;

struct MenuEntry { int id; int value; };      // an entry of the mission start menu (Rec118::entries)

struct tstring_pair {
    tstring a;       // +0
    tstring b;       // +0x10
};

// the header 0x4e54e0 reads out of a slot file (list_saves keeps one on its stack; the loading panel keeps one at +0x58)
struct SlotReader;
struct SlotHeader {
    tstring title;                              // +0x00 the save's title
    tstring body;                               // +0x10 (the hero's name in the loading panel)
    int f20;                                    // +0x20 (the sub-reader's record: first word + 4 pad)
    float time;                                 // +0x24 (game hours)
    int year;                                   // +0x28 the file version word
    int month;                                  // +0x2c (the magic "WRDR" word in a valid file)
    int f30;                                    // +0x30
    bool read_slot(SlotReader* r);              // 0x4e54e0 (ret 4; cls_map/progress_bar.cpp)
};

// The "creating game" loading panel (vtable 0x595798, ctor 0x4c2fb0): builds the loading window (slot 2, cls_map.cpp
// 0x4daa10) and advances a seven-step sequence one step per frame (slot 3, party/game.cpp 0x51c5a0, with sub 0x51c720).
// w04 is the start mode: 0 = new game, 1 = load a save, 2/3 = other starts.
struct C4c2fb0 : head_rec {
    ContainerBase* overlay;    // +0x2c the window (vtable 0x592848)
    MissionUi* hint;           // +0x30 the status text widget (set_text 0x51c5a0)
    MissionUi* text;           // +0x34 the random "ui_hint" text line (flag 0x100)
    MissionUi* logo;           // +0x38 the "warrider_logo" picture
    int counter;               // +0x3c current step (the slot-2 builder clears it)
    int f40;                   // +0x40 (cleared by the slot-2 builder; sub's unused argument)
    bin_reader* save;          // +0x44 the save game being read (mode 1)
    tstring path;              // +0x48 save file name
    SlotHeader header;         // +0x58
    int f8c;                   // +0x8c the value handed to Game::init (step 1)
    C4c2fb0();                 // 0x4c2fb0
    virtual void reset_screen();      // slot 1  0x4c6e00: g_gui.reset()
    virtual void init_screen();      // slot 2  0x4daa10 (cls_map.cpp)
    virtual void frame_update(float);      // slot 3  0x51c5a0 (party/game.cpp)
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    bool sub(int* counter, int* unused, tstring* out);          // 0x51c720 (ret 0xc)
};

// The save / load window (vtable 0x595924, ctor 0x4c31c0): the nine slot records (list_saves 0x4e5270 fills them from "sgDD.sav",
// slot_button 0x4e55f0 reads them), one long button per slot and the three command buttons. Slot 2 builds the window
// (cls_map/progress_bar.cpp 0x4e4790), slot 3 handles the buttons and the confirmation boxes (char_sheet/save_panel.cpp 0x4ea290,
// select 0x4ea850).
struct SlotRec {
    tstring name;                               // +0x00 the save's title
    tstring date;                               // +0x10 the formatted date line
    int seconds;                                // +0x20 non-zero when the slot holds a save
    int year;                                   // +0x24 the file version word
    int month;                                  // +0x28 the module version word
    unsigned char bad_type, bad_version, bad_module, pad2f;    // +0x2c the three "incompatible" flags
};
struct C4c31c0 : head_rec {
    SlotRec slots[9];          // +0x2c (stride 0x30)
    char pad1dc[4];
    ContainerBase* window;     // +0x1e0
    MissionUi* labels[9];         // +0x1e4 one long button per slot
    MissionUi* btn_done;          // +0x208
    MissionUi* btn_cancel;        // +0x20c
    MissionUi* btn_delete;        // +0x210
    char pad214[4];
    int sel218, sel21c, sel220;    // +0x218 the selected slot (-1); +0x21c / +0x220 the confirmation answers (-1)
    tstring t224;              // +0x224 the typed save name
    bool is_load;              // +0x234
    C4c31c0();                 // 0x4c31c0
    virtual void reset_screen();      // slot 1  0x4c6e00: g_gui.reset()
    virtual void init_screen();      // slot 2  0x4e4790 (cls_map/progress_bar.cpp)
    virtual void frame_update(float);      // slot 3  0x4ea290 (char_sheet/save_panel.cpp)
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    void list_saves();                // 0x4e5270
    void slot_button(ContainerBase* parent, int slot, MissionUi** out);   // 0x4e55f0 (ret 0xc)
    void select(int idx);             // 0x4ea850
};

// The start-up loading screen (vtable 0x595a20, ctor 0x4c3290): a random picture, the logo, a progress bar and the hint line
// (slot 2, cls_map.cpp 0x4e3aa0), then one loading step per frame (slot 3, inventory_panel/game_app.cpp 0x50f390, with advance
// 0x50f440 in inventory_panel/loading_screen.cpp).
struct C4c3290 : head_rec {
    ContainerBase* overlay;    // +0x2c the window
    MissionUi* hint;           // +0x30 the hint line; update sets its text
    MissionUi* logo;           // +0x34 the "warrider_logo" picture
    MissionUi* bar;            // +0x38 the progress bar; update sets its value
    int step;                  // +0x3c current step
    int total;                 // +0x40
    int f44;                   // +0x44 (the slot-2 builder stores 10; advance 0x50f440 the module-info table pointer)
    int f48;                   // +0x48
    char pad4c[4];
    tstring t50;               // +0x50
    tstring_pair t60;          // +0x60, +0x70
    C4c3290();                 // 0x4c3290
    virtual void reset_screen();      // slot 1  0x4c6e00: g_gui.reset()
    virtual void init_screen();      // slot 2  0x4e3aa0 (cls_map.cpp)
    virtual void frame_update(float);      // slot 3  0x50f390 (inventory_panel/game_app.cpp)
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    bool advance(int* step, int* total, tstring* text);   // 0x50f440
};

// The message window (vtable 0x5957bc, ctor 0x4c3050): up to six buttons with the texts at +0x6c. Slot 2 (cls_map.cpp,
// 0x4ddf80) builds the window; slot 3 (rgl/mesh2/mesh_2.cpp, 0x4ca6c0) finds the pressed button.
struct C4c3050 : head_rec {
    ContainerBase* win;        // +0x2c the window
    char pad30[4];             // +0x30
    MissionUi* image;          // +0x34 the picture / text block
    MissionUi* rows[6];        // +0x38 the buttons
    int sel;                   // +0x50 the pressed button (-1 = none; the constructor sets it)
    int count;                 // +0x54 number of buttons (the constructor clears it)
    char f58;                  // +0x58 (g_gui.show_cursor at the end of slot 2)
    char pad59[3];
    tstring t5c;               // +0x5c
    char t6c[0x60];            // +0x6c the six button texts (tstrings 0x10 apart; built / destroyed by hand in the ctor / dtor)
    C4c3050();                 // 0x4c3050
    __declspec(noinline) ~C4c3050();   // 0x4c30e0 (called by the deleting destructor 0x4c30c0)
    virtual void reset_screen();       // slot 1  0x4c30b0: g_gui.f1709 = 1
    virtual void init_screen();       // slot 2  0x4ddf80 (cls_map.cpp)
    virtual void frame_update(float);   // slot 3  0x4ca6c0 (mesh_2.cpp)
    virtual char pure_7() { return 0; }   // slot 7: 0x4280e0 (xor al,al; ret), not head_rec's return 1
};


// ---- the trigger records (vtables 0x5957e0..0x595c68), created by init_mission_trigger_records (options_panel/trigger_records.cpp) ----
#include <stddef.h>
#include <string.h>
#include "../include/frustum.h"
void* __cdecl operator new(size_t n);   // 0x53545e (scalar operator new)

struct MissionItem;
struct Rec118 : head_rec { MB_ZERO_TAIL(Rec118)   // vtable 0x595804
    ContainerBase* overlay;   // +0x2c the window
    MissionUi* row[16];       // +0x30 the faction rows
    ContainerBase* col[3];    // +0x70 per-side column windows
    char pad7c[8];
    tstring second;     // +0x84
    MenuEntry entries[16];    // +0x94 the mission start menu's entries (slot 3 0x531590 picks the pressed one)
    int count;                // +0x114
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec38 : head_rec { MB_ZERO_TAIL(Rec38)   // vtable 0x595828
    ContainerBase* win;       // +0x2c
    MissionUi* f30;           // +0x30
    MissionUi* f34;           // +0x34 the "leave" button (slot 3, hud.cpp 0x4d59f0)
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 0; }    // slot 7: 0x4280e0 (xor al,al; ret), not head_rec's return 1
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec44 : head_rec { MB_ZERO_TAIL(Rec44)   // vtable 0x5957e0 (the mission window: slot 2 cls_map.cpp 0x4dae40, slot 3 hud.cpp 0x4d5980)
    ContainerBase* overlay;   // +0x2c the window
    MissionUi* row[5];        // +0x30 (row[2] is the mission text, row[4] is cleared first)
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec34 : head_rec { MB_ZERO_TAIL(Rec34)   // vtable 0x59584c (the debrief window: slot 2 cls_map.cpp 0x4de5b0)
    ContainerBase* win;       // +0x2c
    MissionUi* f30;           // +0x30
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 0; }    // slot 7: 0x4280e0 (xor al,al; ret), not head_rec's return 1
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec454 : head_rec { MB_ZERO_TAIL(Rec454)   // vtable 0x595894 (the quest log: slot 2 char_sheet/quest_log.cpp 0x4ee230, slot 3 options_panel/hud.cpp 0x4d5a20)
    NodeA* window;               // +0x2c
    UiA* done;                   // +0x30 the "done" text
    UiA* active_tab;             // +0x34
    UiA* finished_tab;           // +0x38
    Camera* list;                // +0x3c
    ContainerBase* frame;        // +0x40 the quest row frame
    MissionUi* panel1;           // +0x44
    MissionUi* panel2;           // +0x48
    int indices[0x80];        // +0x4c
    int count;                // +0x24c
    int f250;                 // +0x250 selected row
    UiA* rows[0x80];             // +0x254
    void __fastcall refresh();                // 0x4ee840 (char_sheet/quest_log.cpp)
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
// Rec1208 (vtable 0x595bfc, the conversation screen) is declared in conv_screen.h
struct Rec3c : head_rec { MB_ZERO_TAIL(Rec3c)   // vtable 0x595948 (the "done" window: slot 2 cls_map.cpp 0x4db660, slot 3 options_panel.cpp 0x4d7d90)
    ContainerBase* overlay;   // +0x2c the window
    MissionUi* row[3];        // +0x30 ui_done button, -, the factions text
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Recd8 : head_rec { MB_ZERO_TAIL(Recd8) // vtable 0x5958b8
    ContainerBase* root;                        // +0x2c options_window container
    MissionUi* header;                          // +0x30 warrider_logo container
    MissionUi* h34;                             // +0x34 the "days" caption (update_days 0x4d6270 rewrites it)
    UiA* b38, *b3c, *b40;                 // +0x38 ui_controls, +0x3c ui_video_options, +0x40 ui_done
    DropDown* dd44, *dd48, *dd4c, *dd50, *dd54, *dd58;   // +0x44..0x58 the drop-downs
    UiA* b5c, *b60, *b64, *b68, *b6c, *b70, *b74;  // +0x5c..0x74 the ui_enabled toggles
    UiB* sl78;                                  // +0x78
    UiB* s7c, *s80, *s84;                       // +0x7c..0x84 the sliders
    UiA* b88;                             // +0x88
    MissionUi* h8c, *h90, *h94, *h98, *h9c, *ha0, *ha4, *ha8, *hac, *hb0, *hb4, *hb8, *hbc, *hc0, *hc4, *hc8, *hcc, *hd0, *hd4;   // +0x8c..0xd4 the hint labels
    void update_difficulty();                   // 0x4d6300 (options_panel/quest_page.cpp; defined in cls_map/cls_map.h)
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
};
struct Recf0 : head_rec { MB_ZERO_TAIL(Recf0) // vtable 0x5958dc
    ContainerBase* window;                      // +0x2c
    MissionUi* done;                            // +0x30
    UiB* ui_scale;                              // +0x34
    UiB* y_scale;                               // +0x38
    UiB* z_scale;                               // +0x3c
    UiB* days;                                  // +0x40
    UiB* ragdoll_limit;                         // +0x44
    DropDown* corpses;                          // +0x48
    DropDown* ragdolls;                         // +0x4c
    DropDown* blood;                            // +0x50
    DropDown* plant_shadows;                    // +0x54
    DropDown* resolution;                       // +0x58
    DropDown* shadow_quality;                   // +0x5c
    MissionUi* change;                          // +0x60
    UiA* quality[6];                      // +0x64..0x78
    MissionUi* lbl_texture_detail;              // +0x7c "ui_texture_detail"
    MissionUi* val_texture_detail;              // +0x80 texture detail percentage text
    MissionUi* lbl_shadow_quality;              // +0x84 "ui_shadow_quality2"
    MissionUi* lbl_vertex_shaders;              // +0x88 "ui_enable_vertex_shaders2"
    MissionUi* val_vertex_shaders;              // +0x8c gamma text, replaced by "DirectX 9/7"
    MissionUi* lbl_screen_size;                 // +0x90 "ui_screen_size2"
    MissionUi* lbl_gamma;                       // +0x94 "ui_gamma" (hidden without shaders)
    MissionUi* val_gamma;                       // +0x98 the gamma text, e.g. 2.20 (hidden without shaders)
    MissionUi* lbl_grass_density;               // +0x9c "ui_grass_density"
    MissionUi* val_grass_density;               // +0xa0 ragdoll-limit percentage text
    MissionUi* lbl_char_detail;                 // +0xa4 "ui_character_detail"
    MissionUi* lbl_char_low;                    // +0xa8 "ui_low"
    MissionUi* lbl_char_high;                   // +0xac "ui_high"
    MissionUi* lbl_char_shadow_detail;          // +0xb0 "ui_character_shadow_detail"
    MissionUi* lbl_shadow_low;                  // +0xb4 "ui_low"
    MissionUi* lbl_shadow_high;                 // +0xb8 "ui_high"
    MissionUi* lbl_blood_stains;                // +0xbc "ui_blood_stains"
    MissionUi* lbl_dynamic_lighting;            // +0xc0 "ui_dynamic_lighting"
    MissionUi* lbl_character_shadows;           // +0xc4 "ui_character_shadows"
    MissionUi* lbl_environment_shadows;         // +0xc8 "ui_environment_shadows"
    MissionUi* lbl_realistic_shadows;           // +0xcc "ui_realistic_shadows_on_plants"
    MissionUi* lbl_particle_systems;            // +0xd0 "ui_particle_systems"
    MissionUi* lbl_anisotropic;                 // +0xd4 "ui_anisotropic_filtering"
    MissionUi* lbl_fast_water;                  // +0xd8 "ui_fast_water_reflection"
    MissionUi* lbl_corpses;                     // +0xdc "ui_number_of_corpses"
    MissionUi* lbl_ragdolls;                    // +0xe0 "ui_number_of_ragdolls"
    MissionUi* quality_label;                   // +0xe4 empty text (set by update_perf)
    int day_saved;                              // +0xe8
    char f_ec;                                  // +0xec
    char f_ed;                                  // +0xed
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
};
struct Rec1bc : head_rec { MB_ZERO_TAIL(Rec1bc)   // vtable 0x595900 (the key-binding page: slot 2 cls_map.cpp 0x4e25a0)
    ContainerBase* quad;                        // +0x2c the options_window container
    MissionUi* f30;                             // +0x30 ui_done button
    MissionUi* f34;                             // +0x34 ui_undo_changes
    MissionUi* f38;                             // +0x38 ui_set_default_keys
    MissionUi* slots[0x25];                     // +0x3c .. +0xd0 the 37 key-row labels
    MissionUi* slot_buttons[0x25];              // +0xd0 .. +0x164 their buttons
    MissionUi* f164;                            // +0x164 "press a key" label
    MissionUi* f168;                            // +0x168 battle controls caption
    MissionUi* f16c;                            // +0x16c map controls caption
    MissionUi* captions[7];                     // +0x170 .. +0x18c the 7 key names
    MissionUi* key_values[7];                   // +0x18c .. +0x1a8 their bound keys
    MissionUi* f1a8;                            // +0x1a8 general controls caption
    MissionUi* f1ac;                            // +0x1ac ui_take_screenshot
    MissionUi* f1b0;                            // +0x1b0 "Ctrl + Insert"
    char f1b4;                                  // +0x1b4 modified
    char pad1b5[3];
    int f1b8;                                   // +0x1b8 the row being assigned, -1 for none
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
struct Rec48 : head_rec { MB_ZERO_TAIL(Rec48) // vtable 0x59596c
    ContainerBase* bg;                          // +0x2c the window
    MissionUi* done;                            // +0x30 ui_done (meeting) / the hint label (loading)
    MissionUi* import_btn;                      // +0x34 ui_import_character / the logo
    MissionUi* export_btn;                      // +0x38 ui_export_character / the progress bar
    MissionUi* panel;                           // +0x3c the character list panel (0x597050)
    int f40;                                    // +0x40
    int f44;                                    // +0x44
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
};
struct NodeA;
struct MWidget;
struct Rec58 : head_rec { MB_ZERO_TAIL(Rec58)   // vtable 0x5959fc (the game-log window; w04 nonzero: the full log. slot 2 agent5/agent_hud.cpp 0x4fc810, slot 3 map_editor/exchange_screens.cpp 0x5337a0)
    NodeA* frame;                                           // +0x2c
    MWidget* log;                                                 // +0x30
    ContainerBase* bar;                                               // +0x34
    MissionUi* slider;                                            // +0x38
    MissionUi* tab_return;                                        // +0x3c
    MissionUi* tab_log;                                           // +0x40
    MissionUi* tab_recent;                                        // +0x44
    MissionUi* tab_troops;                                        // +0x48
    MissionUi* tab_locations;                                     // +0x4c
    MissionUi* tab_factions;                                      // +0x50
    MissionUi* tab_quests;                                        // +0x54
    void build(bool show);                                        // 0x4fc820 (ret 4)
    virtual void reset_screen();
    virtual void init_screen();
    virtual void frame_update(float);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};
#endif
