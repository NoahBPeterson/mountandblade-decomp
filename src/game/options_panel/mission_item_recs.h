// mission_item_recs.h: the mission item records built by init_mission_trigger_records (options_panel/trigger_records.cpp):
// MissionBlocker (ctor 0x4ce960, vtable 0x59616c), ItemA (0x4ceb30, 0x596190), ItemD (0x4cee50, 0x5961c0),
// ItemB (0x4cef20, 0x5961e4), ItemC (0x4cef80, 0x596208). Defined in mission_items.cpp.
#ifndef GAME_OPTIONS_PANEL_MISSION_ITEM_RECS_H
#define GAME_OPTIONS_PANEL_MISSION_ITEM_RECS_H
#include <stddef.h>
#include "../head_rec.h"
#include "../tstring.h"
#include "../../rgl/rgl_math.h"      // vector4, matrix
#include "../../include/frustum.h"      // rgl::frustum
using rgl::vector4;
#include "../triggers/item_rec.h"   // owned_block (the menu layout block ItemA..C start with, as ItemRec does)

struct item_base : head_rec {
    // head_rec's ctor sets +8 to 0x63; these records store 99 instead, so the ctor here finishes the job.
    item_base() { w08 = 99; }
};

// The mission screen: the record 0x4d0620 allocates at slot 14 (0x328 bytes, vtable 0x59616c). Its slots are its own functions: reset_gui,
// the widget builder `refresh` 0x4f7f00 (agent5/agent_hud.cpp), the per-frame `tick` 0x5261e0 (mission5/mission_5.cpp) and the camera /
// parameter initialiser 0x4cea10 (options_panel/mission_items.cpp); update_camera 0x4f8c80, update_status 0x4fb350, update_equipment
// 0x4f8b40 and update_marks 0x529130 are its other members.
namespace rgl { struct mesh; }
struct NodeA; struct MapWidget; struct Mark_s29130;
struct MissionBlocker : item_base {
    NodeA* frame;                                               // +0x2c the screen's root widget
    owned_block owned;                                          // +0x30 the item source's buffer (begin / end / cap at +0x34..+0x3c, = 0); its destructor is the one the whole family's deleting destructor 0x4ce930 runs
    int f40;                                                    // +0x40 = -1 (the block owner's menu kind)
    unsigned char flags44;                                      // +0x44
    unsigned char f45, f46, f47;
    int f48;                                                    // +0x48 = 0 (tick clock index)
    int f4c;
    __int64 stamp;                                              // +0x50 = g_ticks[0]
    float f58;                                                  // +0x58 = 1.0f
    void* f5c;                                                  // +0x5c = 0 (the block owner's menu block)
    MapWidget* name_widget;                                     // +0x60 (0x4f7f00; 0x4f8c80 copies the frustum into it)
    int f64;                                                    // +0x64
    MissionUi* w[7];                                            // +0x68 the three crosshair arms, then the four arrows (0x4f7f00; 0x529130 shows / hides them)
    float color[7];                                             // +0x84 the arms' fade (cleared by 0x4f7f00, driven by 0x529130)
    rgl::mesh* marks[7];                                        // +0xa0 their meshes (0x4f7f00; the colour word at mesh +0x3c is 0x529130's)
    MissionUi* ui_font;                                         // +0xbc ("ui_font" text, 0x4fb350 shield text cur)
    MissionUi* ui_font_bg;                                      // +0xc0 (0x4fb350 shield text max)
    ContainerBase* status_group;                                // +0xc4 (the status background, vtable 0x5924a0; 0x4fb350 hides / shows it)
    MissionUi* shield_icon;                                     // +0xc8 (0x4fb350 ammo icon)
    MissionUi* status_bar;                                      // +0xcc (0x4fb350 shield piece)
    MissionUi* health_bar;                                      // +0xd0
    rgl::mesh* shield_mesh;                                     // +0xd4 = 0 (0x4cea10)
    rgl::mesh* shadow_mesh;                                     // +0xd8 = 0 (0x4cea10)
    rgl::mesh* health_mesh;                                     // +0xdc ("status_health_bar"; 0x4fb350)
    int shown_health;                                           // +0xe0 (0x4f7f00 -1; 0x4fb350 the health shown)
    int shield_max;                                             // +0xe4 (0x4f7f00 -2)
    int shield_cur;                                             // +0xe8
    int shield_aux;                                             // +0xec
    int ammo_count;                                             // +0xf0
    int ammo_stage;                                             // +0xf4
    rgl::frustum fr;                                            // +0xf8 (0x4ce960 constructs it; 0x4f7f00 sets the near / far pair and the four edges, 0x4f8c80 refreshes it every tick)
    vector4 head;                                               // +0x218 (0x4f8c80: the agent's head position plus the camera height)
    rgl::matrix k;                                              // +0x228 the camera frame (0x4f8c80; s, f, u, o rows)
    float yaw;                                                  // +0x268 = 0 (0x4f7f00, 0x4f8c80)
    float pitch;                                                // +0x26c = -0.1f
    int f270, f274;                                             // +0x270 = 0
    float f278;                                                 // +0x278 = 1.1f
    float f27c;                                                 // +0x27c (0x4f7f00 0 / 0.1; 0x4f8c80)
    float f280;                                                 // +0x280 (0x4f7f00 1.65 / 2.4; 0x4f8c80)
    float f284;                                                 // +0x284 = 0
    float zoom;                                                 // +0x288 = 97.0f (field of view, degrees)
    vector4 lag;                                                // +0x28c (0x4f8c80: the smoothed head bone position; x = -1e6f)
    int f29c;                                                   // +0x29c = 0
    int f2a0;                                                   // +0x2a0 = -1 (the order group tick() steps through)
    int f2a4;                                                   // +0x2a4
    int clock_a;                                                // +0x2a8 tick clock index
    int f2ac;
    __int64 stamp_a;                                            // +0x2b0 (10 s before clock_a's time)
    int clock_b;                                                // +0x2b8
    int f2bc;
    __int64 stamp_b;                                            // +0x2c0
    int f2c8;                                                   // +0x2c8
    int f2cc;                                                   // +0x2cc
    int f2d0;                                                   // +0x2d0
    int f2d4, f2d8, f2dc;                                       // +0x2d4
    unsigned char f2e0;                                         // +0x2e0 = 0 (cheat: freeze)
    unsigned char f2e1;                                         // +0x2e1 = 0 (cheat: slow motion)
    unsigned char f2e2, f2e3;
    int f2e4, f2e8;                                             // +0x2e4
    float f2ec;                                                 // +0x2ec = 1.1f (0x4f8c80: the smoothed camera pull-in distance)
    bool f2f0;                                                  // +0x2f0 = 0
    bool refresh_slots;                                         // +0x2f1 = 0 (0x4f7f00: the selected agent's slots need re-initialising)
    bool grow_health;                                           // +0x2f2 = 0 (0x4f7f00: the status block height must grow)
    char f2f3;
    int status_height;                                          // +0x2f4
    char f2f8, f2f9;                                            // +0x2f8 = 0
    char f2fa, f2fb;
    int f2fc;                                                   // +0x2fc
    float f300;                                                 // +0x300 = 0 (0x4f8c80: the free camera's speed)
    int f304, f308, f30c, f310;                                 // +0x304
    int f314, f318, f31c;                                       // +0x314 = 0
    int f320;                                                   // +0x320 = -1
    int f324;                                                   // +0x324 = -1
    // The image's table (0x59616c) overrides five of the base's slots; each is the record's own function.
    virtual void reset_screen();                                // slot 1  0x4c6e00 reset_gui (mission_items.cpp)
    virtual void init_screen();                                // slot 2  0x4f7f00 refresh (agent_hud.cpp)
    virtual void frame_update(float);                         // slot 3  0x5261e0 tick (mission_5.cpp)
    virtual void slot_empty_4();                                // slot 4  0x4cea10 the camera / parameter initialiser (mission_items.cpp)
    virtual char pure_7() { return 1; }                         // slot 7  0x48b170 (overrides the base's pure slot)
    virtual char always_true() { return 0; }                    // slot 8  0x4280e0 rgl::static_entity::awake (xor al,al; ret: ICF folds it)
    void update_camera(float dt);                               // 0x4f8c80 (ret 4; agent_hud.cpp)
    void update_status();                                       // 0x4fb350 (agent_hud.cpp)
    void update_equipment();                                    // 0x4f8b40 (this unused; agent_hud.cpp)
    __declspec(noinline) void update_marks();                   // 0x529130 (mission_5.cpp)
    MissionBlocker();                                           // 0x4ce960
};
typedef char MissionBlocker_size_check[sizeof(MissionBlocker) == 0x328 ? 1 : -1];


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
