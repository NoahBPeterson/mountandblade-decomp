// mission_item_recs.h: the mission item records built by init_mission_trigger_records (options_panel/trigger_records.cpp):
// MissionBlocker (ctor 0x4ce960, vtable 0x59616c), ItemA (0x4ceb30, 0x596190), ItemD (0x4cee50, 0x5961c0),
// ItemB (0x4cef20, 0x5961e4), ItemC (0x4cef80, 0x596208). Defined in mission_items.cpp.
#ifndef GAME_OPTIONS_PANEL_MISSION_ITEM_RECS_H
#define GAME_OPTIONS_PANEL_MISSION_ITEM_RECS_H
#include <stddef.h>
#include "../head_rec.h"
#include "../tstring.h"

struct item_base : head_rec {
    // head_rec's ctor sets +8 to 0x63; these records store 99 instead, so the ctor here finishes the job.
    item_base() { w08 = 99; }
};

struct item_base_nz : head_rec {
    item_base_nz() : head_rec(no_w04()) { w08 = 99; }
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
    virtual void slot_empty_1();                                // slot 1  0x4c6e00 reset_gui (gui_edit.cpp)
    virtual void slot_empty_2();                                // slot 2  0x4f7f00 MissionHud::refresh (agent_hud.cpp)
    virtual void scale_3(float);                         // slot 3  0x5261e0 Screen_s261e0::tick(float) (mission_5.cpp)
    virtual void slot_empty_4();                                // slot 4  0x4cea10 MissionParams::init (this file)
    virtual char pure_7() { return 1; }                         // slot 7  0x48b170 (overrides the base's pure slot)
    virtual char always_true() { return 0; }                    // slot 8  0x4280e0 rgl::static_entity::awake (xor al,al; ret: ICF folds it)
    MissionBlocker();                                           // 0x4ce960
};


struct ItemA : item_base_nz {
                                      // vtable 0x596190
    virtual void slot_empty_1();                // slot 1  0x4c6e10 MapDialog::reset_state (gui_edit.cpp)
    virtual void slot_empty_2();                // slot 2  0x4fbfa0 GameScreen::build (agent_hud.cpp)
    virtual void scale_3(float);         // slot 3  0x533070 EventScreen::on_event(int) (exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    virtual void s_last();                      // slot 9  0x4ceb90 MissionUpdater::update (this file)
    char pad2c[0x8];
    int f34, f38, f3c, f40, f44, f48, f4c;
    int clock, clock_hi; float f58; int f5c;
    ItemA();                                                    // 0x4ceb30
};
struct ItemB : item_base_nz {
                                      // vtable 0x5961e4
    virtual void slot_empty_1();                // slot 1  0x4c6e00 reset_gui (gui_edit.cpp)
    virtual void slot_empty_2();                // slot 2  0x4e5d80 SaveWinCtl::create_stub (progress_bar.cpp)
    virtual void scale_3(float);         // slot 3  0x534180 SourceScreen::hook_a(int) (exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    char pad2c[0x8];
    int f34, f38, f3c, f40, f44, f48, f4c;
    int clock, clock_hi; float f58; int f5c;
    ItemB();                                                    // 0x4cef20
};
struct ItemC : item_base_nz {
                                      // vtable 0x596208
    virtual void slot_empty_1();                // slot 1  0x4c6e00 reset_gui (gui_edit.cpp)
    virtual void slot_empty_2();                // slot 2  0x4e5e50 SaveWinCtl::create_flagged (progress_bar.cpp)
    virtual void scale_3(float);         // slot 3  0x534230 SourceScreen::hook_b(int) (exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    char pad2c[0x8];
    int f34, f38, f3c, f40, f44, f48, f4c;
    int clock, clock_hi; float f58; int f5c;
    ItemC();                                                    // 0x4cef80
};


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
struct ItemD_vt : item_base {

    char pad2c[0x7c - 0x2c];
    owned_triple blk0;                                          // +0x7c = 0 (vector begin/end/cap)
    int f_88;                                                   // +0x88
    owned_triple blk1;                                          // +0x8c = 0 (vector begin/end/cap)
    char pad98[0x4];                                            // +0x98
    ItemD_vt() { w04 = 0; w08 = 99; }
};
struct ItemD : ItemD_vt {                                       // vtable 0x5961c0
    // overrides go on ItemD only: ItemD_vt's own table (the base vptr store in 0x4ceec0) must keep the head_rec slots
    virtual void slot_empty_1();                // slot 1  0x4c6e00 reset_gui (gui_edit.cpp)
    virtual void slot_empty_2();                // slot 2  0x50a140 view_obj::reset (view_open.cpp)
    virtual void scale_3(float);         // slot 3  0x533aa0 InputScreen::on_input(int) (exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    ItemD();                                                    // 0x4cee50
};


#endif
