// party_screen.h: the party / prisoner management screen (code 0x502cc0..0x507400), shown from the map and from the
// mission. The slices called its class Mission, PartyWindow, party_window and party_menu; it is none of those
// (src/game/mission.h Mission is the mission object, src/game/party/party_window.h PartyWindow the inventory window).
// One layout for all functions; every offset cites the function that proves it. Names provisional.
#ifndef GAME_INVENTORY_PANEL_PARTY_SCREEN_H
#define GAME_INVENTORY_PANEL_PARTY_SCREEN_H
#include "./inv_common.h"

// a 0x20-byte stack row copied by 0x503810 (the same record as Stack, src/include/party.h)
struct PsRow { int v[8]; };

struct PsScrollList;
struct PartyScreen {
    char pad0[4];
    int mode;                               // +0x04 command: 0/5 party, 1/6/7 give-take, 2 sell, 3 hire, 4 capture (0x502cc0, 0x503ed0, 0x5063e0)
    char pad8[0x10 - 0x8];
    int other_party;                        // +0x10 party id of the second half (0x502cc0, 0x503810, 0x503ed0)
    int other_owner;                        // +0x14 owner troop of the second half (0x503810)
    char pad18[0x2c - 0x18];
    MissionUi* window;                      // +0x2c the main window; its child vector is at +0x178 (0x502cc0, 0x504ef0)
    MissionUi* done_btn;                    // +0x30 (0x502cc0)
    MissionUi* reset_btn;                   // +0x34
    MissionUi* upgrade_panel;               // +0x38 (0x502cc0, 0x5063e0)
    MissionUi* cost_panel;                  // +0x3c troop cost / wage label (0x502cc0, 0x5063e0)
    MissionUi* prisoner_mgmt;               // +0x40 (0x504ef0)
    MissionUi* talk_btn;                    // +0x44 (0x502cc0, 0x5063e0)
    MissionUi* upgrade1;                    // +0x48
    MissionUi* upgrade2;                    // +0x4c
    MissionUi* action_btn;                  // +0x50 release / give / take (0x502cc0, 0x5063e0)
    MissionUi* move_up;                     // +0x54
    MissionUi* move_down;                   // +0x58
    MissionUi* gold;                        // +0x5c (0x504d50, 0x504ef0)
    char pad60[4];
    MissionUi* weekly_cost_lbl;             // +0x64 (0x503ed0, 0x504ef0)
    MissionUi* max_companions;              // +0x68
    MissionUi* max_prisoners;               // +0x6c
    MissionUi* choose_companions;           // +0x70 (0x504ef0)
    MissionUi* choose_prisoners;            // +0x74
    MissionUi* party_skills_bar;            // +0x78 (0x5059c0)
    MissionUi* name_lbl;                    // +0x7c selected troop name label (0x502cc0, 0x507310)
    MissionUi* morale_lbl;                  // +0x80 (0x503810, 0x504ef0)
    MissionUi* info;                        // +0x84 info panel (0x502cc0, 0x5063e0)
    MissionUi* members_base[2];             // +0x88 (0x504ef0)
    MissionUi* skills_base;                 // +0x90 (0x5059c0)
    PsScrollList* skills_list;              // +0x94 (0x5059c0)
    MissionUi* member_list[2][2];           // +0x98 [side][page] (0x504ef0, 0x504ba0)
    MissionUi* members_page[2][2];          // +0xa8
    MissionUi* buttons[4][0x40];            // +0xb8 [2 * side + page][i] (0x504ef0, 0x503ed0)
    float bar_amount[2];                    // +0x4b8 per-type bar total (0x504ba0, 0x502cc0)
    float bar_start[2];                     // +0x4c0
    int slots[2][2][64];                    // +0x4c8 [type][row][slot] -> stack index (0x503ed0)
    int f8c8;                               // +0x8c8 (0x502cc0)
    int f8cc;                               // +0x8cc (0x502cc0, 0x5063e0)
    int f8d0;                               // +0x8d0
    int f8d4;                               // +0x8d4
    char show_healthy[2];                   // +0x8d8 (0x503ed0)
    char show_wounded[2];                   // +0x8da
    int sel_type;                           // +0x8dc selected half (0x502cc0, 0x5063e0)
    int sel_slot;                           // +0x8e0 selected row slot
    int sel_row;                            // +0x8e4
    PsRow rows[2][64];                      // +0x8e8 (0x503810)
    int nstacks[2];                         // +0x18e8
    int owner_troop[2];                     // +0x18f0
    int party_7c[2];                        // +0x18f8
    int party_80[2];                        // +0x1900
    int table[64][32];                      // +0x1908
    char locked;                            // +0x3908 (0x5063e0)
    char pad3909[3];
    MissionUi* char_quad;                   // +0x390c the "party_i_character" quad (0x502cc0, 0x507310, 0x5063e0)

    void open_party_window();                               // 0x502cc0
    void read(int which);                                   // 0x503810
    void refresh_morale();                                  // 0x503a50
    void refresh_party_members(int half);                   // 0x503ed0
    void fill_member(int page, int type, Party* unused, int count);   // 0x504ba0 (ret 0x10)
    void money_label(int type, int troop);                  // 0x504d50
    void setup_party(int side);                             // 0x504ef0
    void setup_skills();                                    // 0x5059c0
    void refresh_selection();                               // 0x5063e0 (no arguments: ret)
    void show_selected_character(int troop, int slot);      // 0x507310
};


// ---- widget classes this screen builds (all derive from the widget base MissionUi, ctor 0x48ae20, 0x178 bytes) ----------
struct PsWindow : MissionUi {                       // 0x190 bytes: base ctor 0x4931a0 (vptr 0x592848 / 0x597050 stored by the caller)
    std::vector<MissionUi*> panels;                 // +0x178 child vector (0x502cc0, 0x504ef0)
    char f188;                                      // +0x188
};
struct PsFrame : MissionUi {                        // 0x188 bytes, ctor 0x4931a0
    int f178;                                       // +0x178 (left uninitialised by the ctor)
    int f17c;                                       // +0x17c
    int f180;                                       // +0x180
    unsigned f184;                                  // +0x184
    PsFrame();                                      // 0x4931a0
};
struct PsBar : PsFrame {};                          // vptr 0x597050 (implicit ctor: base call then the vptr store)
struct PsListWidget : MissionUi {                   // 0x188 bytes, vptr 0x5924a0, the ctor is inlined at the `new`
    int f178, f17c, f180;
    unsigned f184;
    PsListWidget() { f17c = 0; f180 = 0; f184 = 0; }
};
struct PsScrollList : MissionUi {                   // 0x4a8 bytes (ctor 0x49deb0): f104 flags (MissionUi), scrollbar +0x484, list +0x490
    char pad178[0x484 - 0x178];
    MissionUi* scrollbar;                           // +0x484
    char pad488[0x490 - 0x488];
    MissionUi* list;                                // +0x490
    char pad494[0x4a8 - 0x494];
    PsScrollList();                                 // 0x49deb0
};
struct SkillRow {                                   // the 42 x 0x44-byte skill table seen from its name field (0x8ee3110 .. 0x8ee3c38)
    tstring name;                                   // +0x00 (the skill's display name)
    tstring key;                                    // +0x10 (translation key; 0x5059c0 translates it)
    char pad20[0x30 - 0x20];
    unsigned flags;                                 // +0x30 bit 4 = shown on the skills page (0x5059c0)
    char pad34[0x44 - 0x34];
};
extern SkillRow g_skill_rows[];                     // 0x8ee3110

// ---- objects and functions of other translation units --------------------------------------------------------------
extern char g_ui_changed;                           // 0x5d8788 (set to 1 whenever a window was installed)
extern float g_grow_frac;                           // 0x54a6e4 (0.2f)
extern float g_hero_wounded_treshold;                     // 0xa18138
extern float g_player_wounded_treshold;                   // 0xa1813c
extern char g_show_party_ids_instead_of_names;                         // 0xa181b8
extern int g_item_sel;                              // 0xa75f6c (the selected item index; negative = nothing selected)
extern ObjTable g_tableaus;                          // 0xa75f40
void __fastcall window_ctor(void* p);               // 0x4931a0
void __fastcall panel_ctor(void* p);                // 0x48ae20
MissionUi* __cdecl make_short_button(MissionUi* parent, tstring* text, int a, int b, float h);   // 0x4e5570
MissionUi* __cdecl new_medium_button(MissionUi* parent, tstring* text, int a, int b, float h);     // 0x4dce70
MissionUi* __cdecl new_long_button(MissionUi* parent, tstring* text, int a, int b, float h);       // 0x4dcbd0
MissionUi* __cdecl make_scrollbar(MissionUi* parent, const tstring* name);                  // 0x4f6160
MissionUi* __cdecl make_member_list(MissionUi* parent, const tstring* name, MissionUi* sc); // 0x4f62f0
MissionUi* __cdecl make_editor_list(MissionUi* parent, const tstring* name, MissionUi* sc); // 0x4f62f0
MissionUi* __cdecl new_member_button(MissionUi* parent, const tstring& name, float x, int zero, float h,
                                     const tstring& text, const tstring& pressed, float width_ratio,
                                     float height, float ratio);                              // 0x4dcc50
void __fastcall quarter_update(MissionUi* w);       // 0x488d50
int skill_level(int member, int skill);            // 0x437b20 (GameData::skill_level, called here with the global as an argument)

#endif // GAME_INVENTORY_PANEL_PARTY_SCREEN_H
