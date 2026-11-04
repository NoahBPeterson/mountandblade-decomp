// mission_item_recs.h: the mission item records built by init_mission_trigger_records (options_panel/trigger_records.cpp):
// MissionBlocker (ctor 0x4ce960, vtable 0x59616c), ItemA (0x4ceb30, 0x596190), ItemD (0x4cee50, 0x5961c0),
// ItemB (0x4cef20, 0x5961e4), ItemC (0x4cef80, 0x596208). Defined in mission_items.cpp.
#ifndef GAME_OPTIONS_PANEL_MISSION_ITEM_RECS_H
#define GAME_OPTIONS_PANEL_MISSION_ITEM_RECS_H
#include <stddef.h>
#include "../head_rec.h"
#include "../tstring.h"
#include "../triggers/item_rec.h"   // owned_block (the menu layout block ItemA..C start with, as ItemRec does)

struct item_base : head_rec {
    // head_rec's ctor sets +8 to 0x63; these records store 99 instead, so the ctor here finishes the job.
    item_base() { w08 = 99; }
};

struct MissionBlocker : item_base {
                             // 0x2c8 bytes, vtable 0x59616c
    char pad2c[0x8];
    int f34, f38, f3c, f40, f44, f48, f4c;
    int clock;                                                  // +0x50 = g_ticks[0]
    int clock_hi;                                               // +0x54 = g_ticks[1]
    float f58;                                                  // +0x58 = 1.0f
    int f5c;                                                    // +0x5c = 0
    char pad60[0xd4 - 0x60];
    int f_d4;                                                   // +0xd4 = 0
    int f_d8;                                                   // +0xd8 = 0
    char pad_dc[0xf8 - 0xdc];
    char fst[0x120];                                            // +0xf8 frustum, constructed in the body by the
                                                                // original's own order (0x41ef60 at 0x4ce9b8)
    char pad1cc[0x2a8 - 0x218];
    int f_2a8;                                                  // +0x2a8 = 0
    char pad2ac[0x4];
    int f_2b0;                                                  // +0x2b0 = g_ticks[0]
    int f_2b4;                                                  // +0x2b4 = g_ticks[1]
    int f_2b8;                                                  // +0x2b8 = 0
    char pad2bc[0x4];
    int f_2c0;                                                  // +0x2c0 = g_ticks[0]
    int f_2c4;                                                  // +0x2c4 = g_ticks[1]
    // The image's table (0x59616c) overrides five of the base's slots: declaring them here under the base's slot
    // names, each with the image's own target in the comment (the item_rec.h annotation shape), binds the table's
    // slots to the image's functions. Nothing in this TU calls them, so they stay declared, not defined.
    virtual void reset_screen();                                // slot 1  0x4c6e00 reset_gui (gui_edit.cpp)
    virtual void init_screen();                                // slot 2  0x4f7f00 MissionHud::refresh (agent_hud.cpp)
    virtual void frame_update(float);                         // slot 3  0x5261e0 Screen_s261e0::tick(float) (mission_5.cpp)
    virtual void slot_empty_4();                                // slot 4  0x4cea10 MissionParams::init (this file)
    virtual char pure_7() { return 1; }                         // slot 7  0x48b170 (overrides the base's pure slot)
    virtual char always_true() { return 0; }                    // slot 8  0x4280e0 rgl::static_entity::awake (xor al,al; ret: ICF folds it)
    MissionBlocker();                                           // 0x4ce960
};


struct NodeA; struct Camera;                    // widget classes the ItemA members point at (agent5 / party2)
struct ItemA : head_rec {                     // vtable 0x596190, 0xf8 bytes (the game menu window: GameScreen::build, the event hook, the updater)
    ContainerBase* win;                         // +0x2c the menu's frame window (a NodeA)
    owned_block owned;                          // +0x30 the menu layout's owned block (BlockOwner / MissionSource view: init 0x4dd920, pick 0x4d5410)
    int f40;                                    // +0x40 -1 (the layout's menu kind)
    int pad44;
    int clock;                                  // +0x48 clock index (0)
    int pad4c;
    __int64 stamp;                              // +0x50 g_ticks[0]
    float f58;                                  // +0x58 1.0f
    int f5c;                                    // +0x5c 0
    MissionUi* kids[16];                        // +0x60 the sixteen option rows
    int count;                                  // +0xa0 the rows shown
    int ids[16];                                // +0xa4 the menu record index of each row
    MissionUi* panel;                           // +0xe4
    Camera* tree;                               // +0xe8 the scrolling list (FieldPanel::relayout / update)
    Camera* slider;                             // +0xec the quest list the rows sit in
    int fF0;                                    // +0xf0
    MissionUi* fill;                            // +0xf4
    NodeA* frame() { return (NodeA*)win; }      // GameScreen::build's name for the window
    virtual void reset_screen();                // slot 1  0x4c6e10 (gui_edit.cpp: reset the GUI, clear the dialog's selection)
    virtual void init_screen();                // slot 2  0x4fbfa0 GameScreen::build (agent_hud.cpp)
    virtual void frame_update(float);         // slot 3  0x533070 EventScreen::on_event (exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    virtual void s_last();                      // slot 9  0x4ceb90 MissionUpdater::update (this file)
    ItemA();                                                    // 0x4ceb30
};
typedef char ItemA_size_check[sizeof(ItemA) == 0xf8 ? 1 : -1];
struct ItemB : head_rec {                     // vtable 0x5961e4, 0x60 bytes (the save-window panel: create 0x4e5d90, the repeat hook)
    ContainerBase* win;                         // +0x2c
    owned_block owned;                          // +0x30
    int f40;                                    // +0x40 -1
    int pad44;
    int clock;                                  // +0x48
    int pad4c;
    __int64 stamp;                              // +0x50
    float f58;                                  // +0x58 1.0f (the repeat interval)
    int f5c;                                    // +0x5c
    virtual void reset_screen();                // slot 1  0x4c6e00 reset_gui (gui_edit.cpp)
    virtual void init_screen();                // slot 2  0x4e5d80 jmp create (progress_bar.cpp)
    virtual void frame_update(float);         // slot 3  0x534180 hook_a (exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    struct BlockOwner& block() { return *(struct BlockOwner*)&owned; }       // the +0x30 sub-object as the menu block owner sees it (cls_map.h)
    struct MissionSource& src() { return *(struct MissionSource*)&owned; }   // ... and as the item source (mission_source.h)
    void create();                              // 0x4e5d90 (progress_bar.cpp)
    ItemB();                                                    // 0x4cef20
};
typedef char ItemB_size_check[sizeof(ItemB) == 0x60 ? 1 : -1];
struct ItemC : head_rec {                     // vtable 0x596208, 0x60 bytes (the load-window panel)
    ContainerBase* win;                         // +0x2c
    owned_block owned;                          // +0x30
    int f40;                                    // +0x40 -1
    int pad44;
    int clock;                                  // +0x48
    int pad4c;
    __int64 stamp;                              // +0x50
    float f58;                                  // +0x58 1.0f
    int f5c;                                    // +0x5c
    virtual void reset_screen();                // slot 1  0x4c6e00 reset_gui (gui_edit.cpp)
    virtual void init_screen();                // slot 2  0x4e5e50 sets the music situation, then create2 (progress_bar.cpp)
    virtual void frame_update(float);         // slot 3  0x534230 hook_b (exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    struct BlockOwner& block() { return *(struct BlockOwner*)&owned; }       // the +0x30 sub-object as the menu block owner sees it (cls_map.h)
    struct MissionSource& src() { return *(struct MissionSource*)&owned; }   // ... and as the item source (mission_source.h)
    void create2();                             // 0x4e5e60 (progress_bar.cpp)
    ItemC();                                                    // 0x4cef80
};
typedef char ItemC_size_check[sizeof(ItemC) == 0x60 ? 1 : -1];


// A three-pointer owned block that releases and clears itself (the item record holds two of these). The record's
// destructor is the compiler's: it runs the two members in reverse declaration order (the +0x8c block first, as the
// original 0x4ceec0 does) with no vptr scaffolding of its own.
struct owned_triple {
    void* p0;                                                   // +0x00 = 0
    void* p1;                                                   // +0x04
    void* p2;                                                   // +0x08
    owned_triple() {}
    ~owned_triple() { if (p0) operator delete(p0); p0 = 0; p1 = 0; p2 = 0; }
};
struct ChoiceList;                                              // map_editor/exchange_screens.h: the screen's list widgets
struct ItemD_vt : item_base {
    int f2c;                                                    // +0x2c
    ChoiceList* w[18];                                          // +0x30..+0x74 the screen's widgets (on_input reads their clear_mask)
    int a78;                                                    // +0x78 the animation vector's allocator word (std::vector<int> at +0x78)
    owned_triple blk0;                                          // +0x7c = 0 (vector begin/end/cap)
    int f_88;                                                   // +0x88
    owned_triple blk1;                                          // +0x8c = 0 (vector begin/end/cap)
    int f98, f9c, fa0, fa4;                                     // +0x98..+0xa4 the two target pages (build() arguments)
    int fa8, fac;                                               // +0xa8 / +0xac the page shown (-1 after reset)
    float fb0;                                                  // +0xb0 the scroll ratio (-1.0f after reset)
    ItemD_vt() { w04 = 0; w08 = 99; }
};
struct ItemD : ItemD_vt {                                       // vtable 0x5961c0
    // overrides go on ItemD only: ItemD_vt's own table (the base vptr store in 0x4ceec0) must keep the head_rec slots
    virtual void reset_screen();                // slot 1  0x4c6e00 reset_gui (gui_edit.cpp)
    virtual void init_screen();                // slot 2  0x50a140 ItemD::init_screen (view_open.cpp)
    virtual void frame_update(float);         // slot 3  0x533aa0 ItemD::frame_update (exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    ItemD();                                                    // 0x4cee50
};
typedef char ItemD_size_check[sizeof(ItemD) == 0xb4 ? 1 : -1];


#endif
