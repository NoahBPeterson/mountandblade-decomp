// exchange_screens.h: the screen objects of code 0x531590..0x5342a6 (TU 119 party_3 + TU 120 screen): the mission start
// menus, the party exchange window and the screens whose first virtual is the per-frame hook. Names provisional.
//
// Every class here has its own vtable: the hooks sit in the first virtual slot of the vtables 0x595810, 0x595858,
// 0x59587c, 0x595a08, 0x59619c, 0x5961cc, 0x5961f0 and 0x596214 (found by scanning .rdata for the function addresses),
// so the slices' "Menu", "Mission", "Screen", "GameObj" were several different classes. Canonical classes used:
//   GameData (src/include/game_data.h; the offsets it lacks are read through GI/GF below), Party / PartyMgr / Stack
//   (src/include/party.h), Troop (src/include/troop.h), MsgLog (src/include/msg_log.h), and from
//   src/game/options_panel/hud.h: Widget (used() = vtable +0x44, set_key_flags() = +0x48), ValueSource, MainMenu,
//   MissionSource (the item source embedded at +0x30 of the screens).
// The classes below that are views of those (PartyX, PartyMgrX, GameDataX) only add member declarations of this region.
#ifndef GAME_MAP_EDITOR_PARTY_SCREEN_H
#define GAME_MAP_EDITOR_PARTY_SCREEN_H
#include <windows.h>
#include <stdlib.h>
#include <vector>
#include "../options_panel/hud.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/msg_log.h"
#include "../ui/ui_common.h"
#include "../int_vec.h"
#include "../str_table.h"
#include "../../include/dialog_rec.h"

// ---- GameData offsets this region reads that game_data.h does not name (each cites a function of this region) ------------
#define GI(off) (*(int*)((char*)g_game + (off)))
#define GF(off) (*(float*)((char*)g_game + (off)))

#include "../party_x.h"                   // PartyX : Party: the one definition (map_editor and script)

struct PartyMgrX : PartyMgr {
    PartyX* get3(int party, int slot, int kind);        // 0x51ee50 (the call site of 0x531800 pushes three words; the callee returns with ret 4)
};

inline PartyMgrX* party_mgr() { return (PartyMgrX*)&g_game->parties; }

// the party at index `id` of the manager (PartyMgr::pages is a std::vector<Party*>: operator[] gives the original's lea + load)
inline PartyX* party_at(PartyMgrX* m, int id)
{
    Party* page = m->pages[id >> 4];
    return (PartyX*)&page[id & 0xf];
}

inline PartyX* party_index(int id)
{
    PartyMgrX* m = party_mgr();
    int idx = m->find(id);
    return party_at(m, idx);
}

// the same lookup through a game-data pointer that is not g_game (update_party_sprites runs with this = the game data)
inline PartyX* party_index(GameData* g, int id)
{
    PartyMgrX* m = (PartyMgrX*)&g->parties;
    int idx = m->find(id);
    return party_at(m, idx);
}

struct GameDataX : GameData {               // the game object with the members this region calls
    int fn_48a320(int troop);               // 0x48a320 (ret 4)
    __declspec(noinline) void update_party_sprites();            // 0x532de0
    int add_sprite(int a, int b, int kind); // 0x49ebd0 (ret 0xc)
};
inline GameDataX* gx() { return (GameDataX*)g_game; }

// 0x54: the second linked troop of a troop record (Troop::pad54; 0x531800)
inline int troop_f54(Troop* t) { return *(int*)((char*)t + 0x54); }

// ---- small objects ----------------------------------------------------------------------------------------------------
struct FrameFlags { void commit(void* out); };      // 0x513710 (ret 4): the framed command block at 0x5e291c
extern FrameFlags g_flags;                          // 0x5e291c
extern MsgLog g_msglog;                             // 0x5d7080

// the menu / mission controller object at 0xa73300 (the slices called it MissionUi, Screen and AuxObj)
struct MenuCtl {
    char pad0[0x10];
    int f10;                                // +0x10 (0x531600)
    char pad14[0x1c - 0x14];
    int f1c;                                // +0x1c (0x5316f0)
    char pad20[0x2c - 0x20];
    int f2c;                                // +0x2c (0x531600)
    int f30;                                // +0x30
    char pad34[0x3c - 0x34];
    int f3c;                                // +0x3c
    char pad40[0x160 - 0x40];
    int f160;                               // +0x160
    void setup(unsigned flags, int delta);  // 0x513f80 (ret 8)
    void reset();                           // 0x513b40
    void show_item(int a, int b, int c, int d);     // 0x514890 (ret 0x10)
    __declspec(noinline) void setup_entry(struct MenuEntry* e);  // 0x531600
    __declspec(noinline) int weight(unsigned flags);             // 0x5316f0
};
extern MenuCtl g_mui;                       // 0xa73300

// one menu option (8 bytes, array at Menu+0x94)
struct MenuEntry { int id; int value; };

// the screens' option records (g_a739a0 ...): id +4, +0x10/+0x14 arguments, +0x18 the option to return to
struct MenuItem {
    char pad0[4];
    int f4;                                 // +0x04
    char pad8[0x10 - 8];
    int f10;                                // +0x10
    int f14;                                // +0x14
    int f18;                                // +0x18
};

// mission template tables (records at 0xa735d0, 0x20 bytes each)
struct TplOpt {                             // 0x18 bytes
    char pad0[0x10];
    unsigned flags;                         // +0x10
    int setup_arg;                          // +0x14
};
struct TplRec {
    char pad0[0x10];
    int f10;                                // +0x10
    char pad14[4];
    int count;                              // +0x18
    TplOpt* list;                           // +0x1c
};
extern TplRec g_templates[];                // 0xa735d0

// ---- the mission start menus: vtables 0x595810 / 0x595858, same member layout -------------------------------------------
struct MenuBase {
    char pad0[4];
    int f4;                                 // +0x04
    char pad8[0x30 - 8];
    Widget* buttons[0x19];                  // +0x30 (25 pointers, ends at +0x94)
    MenuEntry entries[16];                  // +0x94
    int count;                              // +0x114
};
struct StartMenu : MenuBase {               // vtable 0x595810
    __declspec(noinline) void select(int arg);                   // 0x531590
};
struct ContinueMenu : MenuBase {            // vtable 0x595858
    void update(int arg);                   // 0x531780
};

// ---- the party exchange window (vtable 0x59587c) ---------------------------------------------------------------------
struct TargetW {                            // a scroll target (a list widget): the value source behind +0x484
    char pad[0xf4];
    float f_f4;                             // +0xf4
    char pad2[0x484 - 0xf8];
    ValueSource* src;                       // +0x484 (hud.h: the value source behind the widget)
};
struct XWidget : Widget {                   // a window button: the flag byte at +0x104 (0x531800)
    char pad[0x104 - sizeof(Widget)];
    char f104;                              // +0x104
};

struct XferWin {
    char pad0[4];
    int state;                              // +0x04 (0x531800)
    char pad8[8];
    int sel_party;                          // +0x10
    int troop;                              // +0x14 (0x532560, 0x5325b0)
    int f18;                                // +0x18
    char pad1c[0x30 - 0x1c];
    Widget* w30;                            // +0x30
    Widget* w34;                            // +0x34
    char pad38[0x44 - 0x38];
    Widget* w44;                            // +0x44
    Widget* w48;                            // +0x48
    Widget* w4c;                            // +0x4c
    XWidget* w50;                           // +0x50
    Widget* w54;                            // +0x54
    Widget* w58;                            // +0x58
    char pad5c[0x90 - 0x5c];
    TargetW* f90;                           // +0x90
    TargetW* f94;                           // +0x94
    char pad98[0xb8 - 0x98];
    Widget* grid[2][2][64];                 // +0xb8
    char pad4b8[0x4c8 - 0x4b8];
    int slots[256];                         // +0x4c8
    int f8c8;                               // +0x8c8
    char pad8cc[0x8dc - 0x8cc];
    int f8dc;                               // +0x8dc
    int f8e0;                               // +0x8e0
    int f8e4;                               // +0x8e4
    char pad8e8[0x3908 - 0x8e8];
    char f3908;                             // +0x3908

    void update(int);                       // 0x531800
    void refresh_party_window();            // 0x5063e0
    void fn_503ed0(int);                    // 0x503ed0
    void fn_503a50();                       // 0x503a50
    __declspec(noinline) void clear_selection();                 // 0x5324d0
    __declspec(noinline) void get_cell(int a, int b, int c, int* main, int* cell);   // 0x532520
    __declspec(noinline) void move_men(int amount);              // 0x532560
    __declspec(noinline) void apply(int which);                  // 0x5325b0
    __declspec(noinline) void swap_rows(int which);              // 0x532740
    __declspec(noinline) void stack_remove(int party_id, int slot, int dst_id, int* was_present, int* removed);   // 0x532830 (ret 0x14)
    __declspec(noinline) void set_selection(int a, int b, int c);   // 0x533020 (ret 0xc)
};

// ---- the screens with raw layouts -------------------------------------------------------------------------------------
struct ListWidget : Widget {                // a scroll list: current index +0x1cc into the arrays at +0x19c / +0x1ac
    __declspec(noinline) void get3(int* a, int* b, int* c);      // 0x533a60 (ret 0xc)
};

struct OutIntVec : IntVec {                 // a std::vector<int> embedded in a screen (src/game/int_vec.h) whose push_back the original calls out of line
    void push_back_call(const int& v);      // 0x440b30 (ret 4)
};

struct VecIter {                            // VC7.1 vector<int>::iterator + n, in the out-parameter form
    int* ptr;                               // +0x00
    __declspec(noinline) int** advance(int** out, int n);        // 0x534160 (ret 8)
};

struct ScreenBase {                         // what the raw-layout screens share: +0x18 the option to return to
    char pad0[0x18];
    int f18;                                // +0x18
};
struct TickScreen : ScreenBase {            // vtable 0x595a08
    void on_tick(int code);                 // 0x5337a0 (ret 4)
    void refresh_anim(int a, int b);        // 0x50a1c0 (ret 8)
    void method_4fc820(int arg);            // 0x4fc820 (ret 4)
};
struct EventScreen : ScreenBase {           // vtable 0x59619c
    void on_event(int code);                // 0x533070 (ret 4)
};
struct InputScreen : TickScreen {           // vtable 0x5961cc (shares the animation helpers)
    void on_input(int code);                // 0x533aa0 (ret 4)
};
struct SourceScreen : ScreenBase {          // vtables 0x5961f0 / 0x596214: +0x30 the item source, +0x58 the repeat interval
    char pad1c[0x30 - 0x1c];
    MissionSource src;                      // +0x30
    float interval;                         // +0x58
    void hook_a(int arg);                   // 0x534180 (vtable 0x5961f0 slot 0)
    void hook_b(int arg);                   // 0x534230 (vtable 0x596214 slot 0)
};

// ---- globals -----------------------------------------------------------------------------------------------------------
extern int g_a17144;                        // 0xa17144
extern int g_a17544;                        // 0xa17544
extern int g_a17148;                        // 0xa17148
extern int g_a17348;                        // 0xa17348
extern int g_a17188;                        // 0xa17188
extern int g_a17588;                        // 0xa17588
extern int g_a5a9fbc;                       // 0x5a9fbc
extern int g_trial_mode;                    // 0x5e29c4
extern int g_a735c4;                        // 0xa735c4
extern int g_a70458;                        // 0xa70458
extern int g_a7045c;                        // 0xa7045c
extern int g_a181b4;                        // 0xa181b4
extern int g_a7333c;                        // 0xa7333c
extern int* g_menu_data;                    // 0xa739c0
extern char* g_menu_rows;                   // 0x5e29b0 (GameMenu* in rgl/mesh2/gui_edit.h; the rows of 0x60 bytes are read raw here)
extern float g_k59c48c;                     // 0x59c48c (weight ramp constant)
extern float g_half;                        // 0x549d70 (0.5f)
extern float g_033;                         // 0x54b894 (0.33f)
extern MenuItem* g_a739a0;                  // 0xa739a0
extern MenuItem* g_a739b0;                  // 0xa739b0
extern MenuItem* g_a739b4;                  // 0xa739b4
extern MenuItem* g_a739bc;                  // 0xa739bc
extern MenuItem* g_a739e4;                  // 0xa739e4
extern MenuItem* g_a739e8;                  // 0xa739e8
extern MenuItem* g_a739ec;                  // 0xa739ec
struct KeyDown { bool down(int key); };     // 0x48ac90 (ret 4): the key state object at 0xa73730
extern KeyDown g_a73730;                    // 0xa73730
#define STR_TABLE ((StrTable*)0x5e2a18)     // the localized string table (str_table.h)

#endif // GAME_MAP_EDITOR_PARTY_SCREEN_H
