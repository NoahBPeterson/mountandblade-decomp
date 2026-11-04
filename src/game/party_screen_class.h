// party_screen_class.h: the party / prisoner management screen (vtable 0x595870, record 12 of the window manager, 0x3910 bytes as
// 0x4d0620 allocates it; constructor 0x4c3160): a head_rec whose slot 1 resets the GUI, slot 2 (open_party_window 0x502cc0) builds the
// window, slot 3 (0x531800, map_editor/exchange_screens.cpp) is the per-frame update that also drives the exchange (selection,
// stack moves, row swap). The slices called it Mission, PartyWindow, party_window, party_menu, C4c3160 and XferWin; ONE class for
// inventory_panel/party_screen.cpp (the construction, the halves, the labels), map_editor/exchange_screens.cpp (the exchange) and
// rgl/mesh2/rec_tables.cpp (the constructor). head_rec's words: w04 is the command (0 / 5 party, 1 / 6 / 7 give-take, 2 sell,
// 3 hire, 4 capture), w10 the party id of the second half, w14 the owner troop of the second half.
#ifndef MB_GAME_PARTY_SCREEN_CLASS_H
#define MB_GAME_PARTY_SCREEN_CLASS_H
#include "./head_rec.h"
#include "./ui/container.h"

struct Camera;
struct Party;
struct TargetW;

// a 0x20-byte stack row copied by 0x503810 (the same record as Stack, src/include/party.h)
struct PsRow { int v[8]; };

struct PartyScreen : head_rec {
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
    Camera* skills_list;                    // +0x94 (0x5059c0)
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
    PsRow rows[2][64];                      // +0x8e8 (0x503810; the constructor 0x4c3160 clears them)
    int nstacks[2];                         // +0x18e8
    int owner_troop[2];                     // +0x18f0
    int party_7c[2];                        // +0x18f8
    int party_80[2];                        // +0x1900
    int table[64][32];                      // +0x1908
    char locked;                            // +0x3908 (0x5063e0)
    char pad3909[3];
    MissionUi* char_quad;                   // +0x390c the "party_i_character" quad (0x502cc0, 0x507310, 0x5063e0)

    PartyScreen();                          // 0x4c3160
    virtual void reset_screen();            // slot 1: 0x4c6e00 (options_panel/rec_slots_c4c.cpp)
    virtual void init_screen();             // slot 2: 0x502cc0 (inventory_panel/party_screen.cpp), builds the window
    virtual void frame_update(float);       // slot 3: 0x531800 (map_editor/exchange_screens.cpp)
    virtual char pure_7() { return 1; }     // slot 7: 0x48b170

    void read(int which);                                   // 0x503810
    void refresh_morale();                                  // 0x503a50
    void refresh_party_members(int half);                   // 0x503ed0
    void fill_member(int page, int type, Party* unused, int count);   // 0x504ba0 (ret 0x10)
    void money_label(int type, int troop);                  // 0x504d50
    void setup_party(int side);                             // 0x504ef0
    void setup_skills();                                    // 0x5059c0
    void refresh_selection();                               // 0x5063e0 (no arguments: ret)
    void show_selected_character(int troop, int slot);      // 0x507310
    __declspec(noinline) void clear_selection();                 // 0x5324d0
    __declspec(noinline) void get_cell(int a, int b, int c, int* main, int* cell);   // 0x532520
    __declspec(noinline) void move_men(int amount);              // 0x532560
    __declspec(noinline) void apply(int which);                  // 0x5325b0
    __declspec(noinline) void swap_rows(int which);              // 0x532740
    __declspec(noinline) void stack_remove(int party_id, int slot, int dst_id, int* was_present, int* removed);   // 0x532830 (ret 0x14)
    __declspec(noinline) void set_selection(int a, int b, int c);   // 0x533020 (ret 0xc)
};

#endif // MB_GAME_PARTY_SCREEN_CLASS_H
