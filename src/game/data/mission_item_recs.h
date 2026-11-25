// mission_item_recs.h: the mission item records built by init_mission_trigger_records (game/data/trigger_records.cpp):
// MissionBlocker (ctor 0x4ce960, vtable 0x59616c), ItemA (0x4ceb30, 0x596190), ItemD (0x4cee50, 0x5961c0),
// ItemB (0x4cef20, 0x5961e4), ItemC (0x4cef80, 0x596208). Defined in mission_item_records.cpp.
#ifndef GAME_OPTIONS_PANEL_MISSION_ITEM_RECS_H
#define GAME_OPTIONS_PANEL_MISSION_ITEM_RECS_H
#include <stddef.h>
#include "engine/core/layout32.h"
#include <vector>
#include "game/data/head_rec.h"
#include "engine/core/tstring.h"
#include "engine/math/rgl_math.h"      // vector4, matrix
#include "engine/scene/frustum.h"      // rgl::frustum
using rgl::vector4;
#include "game/data/item_head_records.h"   // ItemRec (the screen records all embed the presentation at +0x30)
#include "game/data/presentation.h"         // widget_mgr: the presentation at +0x30

struct item_base : head_rec {
    // head_rec's ctor sets +8 to 0x63; these records store 99 instead, so the ctor here finishes the job.
    item_base() { w08 = 99; }
};

// The mission screen: the record 0x4d0620 allocates at slot 14 (0x328 bytes, vtable 0x59616c). Its slots are its own functions: reset_gui,
// the widget builder `refresh` 0x4f7f00 (game/ui/hud/agent_hud.cpp), the per-frame `tick` 0x5261e0 (mission5/mission_5.cpp) and the camera /
// parameter initialiser 0x4cea10 (game/data/mission_item_records.cpp); update_camera 0x4f8c80, update_status 0x4fb350, update_equipment
// 0x4f8b40 and update_marks 0x529130 are its other members.
namespace rgl { struct mesh; }
struct PlacedObj;                                               // placed_obj.h
struct NodeA; struct MapWidget;
struct MissionBlocker : item_base {
    NodeA* frame;                                               // +0x2c the screen's root widget
    widget_mgr pres;                                            // +0x30 the screen's presentation (g_presentation while it runs; built before the frustum member, 0x4ce9b8)
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
    float yaw_rate, pitch_rate;                                 // +0x270 = 0 (update_view 0x5284a0)
    float cam_dist;                                             // +0x278 = 1.1f the third-person camera distance (0x4f8c80; update_view clamps it 0.7 .. 10)
    float cam_back;                                             // +0x27c extra distance: 0.1 mounted, 0 on foot (0x4f7f00; 0x4f8c80 eases it)
    float cam_height;                                           // +0x280 above the agent: 2.4 mounted, 1.65 on foot (0x4f7f00; 0x4f8c80 eases it)
    float roll;                                                 // +0x284 = 0 (0x4f8c80: the third rotation about the forward axis)
    float zoom;                                                 // +0x288 = 97.0f (field of view, degrees)
    vector4 lag;                                                // +0x28c (0x4f8c80: the smoothed head bone position; x = -1e6f)
    int order_state;                                            // +0x29c = 0 (1 = the order menu is up; order_tick 0x52b300)
    int order;                                                  // +0x2a0 = -1 the selected order (tick 0x5261e0 sets it from the order keys)
    int f2a4;                                                   // +0x2a4
    gui_timer timer_a;                                          // +0x2a8 the order menu's timeout (order_tick 0x52b300 starts it 7.5 s back; tick 0x5261e0 reads it)
    gui_timer timer_b;                                          // +0x2b8
    int sel_mode;                                               // +0x2c8 the interaction mode under the cursor (-1 none; 1 conversation, 4..8 the use keys' kinds; 0x4f7f00)
    int sel_target;                                             // +0x2cc the selection id (the scene prop / agent; probe_interaction 0x529ec0)
    int f2d0;                                                   // +0x2d0
    int sel_valid;                                              // +0x2d4 (0x4f7f00 0; probe_interaction)
    int f2d8;                                                   // +0x2d8 (0x4f7f00 -1; tick ends its pass when 0)
    int leave_answer;                                           // +0x2dc the "ui_question_leave_area" box's answer (-1 pending, 0 = leave; tick 0x5261e0)
    unsigned char cheat_freeze;                                 // +0x2e0 = 0 (ctrl+F11 toggles: the frame's dt is 0)
    unsigned char cheat_slowmo;                                 // +0x2e1 = 0 (ctrl+F9 toggles: the frame's dt * 0.2)
    unsigned char f2e2, f2e3;
    int f2e4, f2e8;                                             // +0x2e4
    float cam_pull;                                             // +0x2ec = 1.1f (0x4f8c80: the smoothed camera pull-in distance)
    bool scene_dirty;                                           // +0x2f0 = 0 (the terrain brush changed the ground; Mission_s2b5c0)
    bool refresh_slots;                                         // +0x2f1 = 0 (0x4f7f00: the selected agent's slots need re-initialising)
    bool grow_health;                                           // +0x2f2 = 0 (0x4f7f00: the status block height must grow)
    char f2f3;
    int status_height;                                          // +0x2f4
    char placement_adjusted;                                    // +0x2f8 = 0 (MapEditor_s2f250, poly_mesh_pick.h)
    char f2f9;                                                  // +0x2f9 = 0
    char f2fa, f2fb;
    int snap;                                                   // +0x2fc frames left to snap the view (0x4f7f00 0; update_view)
    float free_cam_speed;                                       // +0x300 = 0 (0x4f8c80: the free camera's speed)
    vector4 picked_pos;                                         // +0x304 the editor's last picked position (MapEditor_s2f250)
    int down_repeat, up_repeat;                                 // +0x314 = 0 the editor's key-repeat timers
    int variant;                                                // +0x31c = 0 the editor's selected variant
    int prev_variant;                                           // +0x320 = -1
    int presentation;                                           // +0x324 = -1 the presentation to start (GameData::pending_presentation; tick 0x5261e0)
    // The image's table (0x59616c) overrides five of the base's slots; each is the record's own function.
    virtual void reset_screen();                                // slot 1  0x4c6e00 reset_gui (mission_item_records.cpp)
    virtual void init_screen();                                // slot 2  0x4f7f00 refresh (agent_hud.cpp)
    virtual void frame_update(float);                         // slot 3  0x5261e0 tick (mission_5.cpp)
    virtual void slot_empty_4();                                // slot 4  0x4cea10 the camera / parameter initialiser (mission_item_records.cpp)
    virtual char pure_7() { return 1; }                         // slot 7  0x48b170 (overrides the base's pure slot)
    virtual char always_true() { return 0; }                    // slot 8  0x4280e0 rgl::static_entity::awake (xor al,al; ret: ICF folds it)
    void update_camera(float dt);                               // 0x4f8c80 (ret 4; agent_hud.cpp)
    void update_status();                                       // 0x4fb350 (agent_hud.cpp)
    void update_equipment();                                    // 0x4f8b40 (this unused; agent_hud.cpp)
    __declspec(noinline) void update_view(float dt);            // 0x5284a0 (mission_5.cpp: look input, view frame, the agent's input bits)
    __declspec(noinline) void update_marks();                   // 0x529130 (mission_5.cpp)
    __declspec(noinline) void probe_interaction();              // 0x529ec0 (mission_5.cpp: what the player's agent looks at; sets sel_mode / sel_target / sel_valid)
    __declspec(noinline) void order_tick();                     // 0x52b300 (mission_5.cpp: give the selected order, re-arm the order menu)
    void draw_order_menu();                                     // 0x52b3c0 (mission_5.cpp)
    // ---- the map editor's per-frame state machine (mission_5.cpp) ----
    __declspec(noinline) void update_ui();                      // 0x52ba10
    __declspec(noinline) void update_ui_continue();             // 0x52ba1d (interior continuation of update_ui)
    void update_cursor();                                       // 0x52bc80 ray-pick the ground and drive the terrain brush
    __declspec(noinline) void find_position(char* ok, vector4* pos);   // 0x52bdb0 the ground point under the cursor
    void update_mesh_edit();                                    // 0x52cf60 (game/editor/poly_mesh_pick.cpp) the nav mesh editor: pick, move, rotate, scale, copy
    void update_placement();                                    // 0x52f250 (game/editor/poly_mesh_poly_ray_edit.cpp) the prop placement preview and commit
    void apply(bool& moved, vector4& pos, bool& rotated, rgl::mat3& frame, bool& drag, vector4& dir, char& flag);   // 0x52fe40 (game/editor/poly_mesh_poly_ray_edit.cpp, ret 0x1c: the placement keys, turned by yaw)
    void update_editor();                                       // 0x5305b0 (game/editor/map_editor_update.cpp) the map editor's frame: selection, delete, drag / rotate
    void add_selected(rgl::mesh* target, const rgl::matrix* frame, PlacedObj* prop, unsigned color);   // 0x5314e0 (game/editor/map_editor_update.cpp, ret 0x10)
    void pick_entity();                                         // 0x4ed8d0 (game/mission/pick_entity.cpp) the scene prop under the cursor: sel_target, its label
    MissionBlocker();                                           // 0x4ce960
};
MB_LAYOUT32_ASSERT(MissionBlocker_size_check, sizeof(MissionBlocker) == 0x328);


struct NodeA; struct Camera;                    // widget classes the ItemA members point at (agent5 / party2)
struct ItemA : head_rec {                     // vtable 0x596190, 0xf8 bytes (the game menu window: GameScreen::build, the event hook, the updater)
    ContainerBase* win;                         // +0x2c the menu's frame window (a NodeA)
    widget_mgr pres;                            // +0x30 the menu's presentation (init 0x4dd920, pick 0x4d5410)
    MissionUi* kids[16];                        // +0x60 the sixteen option rows
    int count;                                  // +0xa0 the rows shown
    int ids[16];                                // +0xa4 the menu record index of each row
    MissionUi* panel;                           // +0xe4
    Camera* tree;                               // +0xe8 the scrolling list (Camera::relayout / update)
    Camera* slider;                             // +0xec the quest list the rows sit in
    int fF0;                                    // +0xf0
    MissionUi* fill;                            // +0xf4
    NodeA* frame() { return (NodeA*)win; }      // GameScreen::build's name for the window
    virtual void reset_screen();                // slot 1  0x4c6e10 (editor_selection_and_translation.cpp: reset the GUI, clear the dialog's selection)
    virtual void init_screen();                // slot 2  0x4fbfa0 GameScreen::build (agent_hud.cpp)
    virtual void frame_update(float);         // slot 3  0x533070 EventScreen::on_event (mission_menu_and_exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    virtual void s_last();                      // slot 9  0x4ceb90 MissionUpdater::update (this file)
    ItemA();                                                    // 0x4ceb30
};
MB_LAYOUT32_ASSERT(ItemA_size_check, sizeof(ItemA) == 0xf8);
struct ItemB : head_rec {                     // vtable 0x5961e4, 0x60 bytes (the save-window panel: create 0x4e5d90, the repeat hook)
    ContainerBase* win;                         // +0x2c
    widget_mgr pres;                            // +0x30 the panel's presentation (duration: the repeat interval)
    virtual void reset_screen();                // slot 1  0x4c6e00 reset_gui (editor_selection_and_translation.cpp)
    virtual void init_screen();                // slot 2  0x4e5d80 jmp create (progress_bar_and_load_screens.cpp)
    virtual void frame_update(float);         // slot 3  0x534180 hook_a (mission_menu_and_exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    void create();                              // 0x4e5d90 (progress_bar_and_load_screens.cpp)
    ItemB();                                                    // 0x4cef20
};
MB_LAYOUT32_ASSERT(ItemB_size_check, sizeof(ItemB) == 0x60);
struct ItemC : head_rec {                     // vtable 0x596208, 0x60 bytes (the load-window panel)
    ContainerBase* win;                         // +0x2c
    widget_mgr pres;                            // +0x30 the panel's presentation
    virtual void reset_screen();                // slot 1  0x4c6e00 reset_gui (editor_selection_and_translation.cpp)
    virtual void init_screen();                // slot 2  0x4e5e50 sets the music situation, then create2 (progress_bar_and_load_screens.cpp)
    virtual void frame_update(float);         // slot 3  0x534230 hook_b (mission_menu_and_exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    void create2();                             // 0x4e5e60 (progress_bar_and_load_screens.cpp)
    ItemC();                                                    // 0x4cef80
};
MB_LAYOUT32_ASSERT(ItemC_size_check, sizeof(ItemC) == 0x60);


struct MWidget;                                                 // game/ui/widgets/map_list.h: the report / link text lists (get3 0x533a60)
struct text_box;                                                // game/ui/widgets/container.h
struct sel_list;                                                // game/ui/widgets/text_box.cpp: the sorter's view of an id vector
struct Camera;                                                  // game/mission/camera_class.h: a scrolling list (its scroll bar is follow[1])
struct MissionUi;
// The notes window (built by build 0x50a1c0, game/ui/screens/notes_window.cpp).
struct ItemD_vt : item_base {
    NodeA* panel;                                               // +0x2c the notes window's root (note_window::panel)
    union {
    MissionUi* w[18];                                           // +0x30..+0x74 the screen's widgets (on_input reads their clear_mask)
    struct {
    MissionUi* picture;                                         // +0x30
    MWidget* report;                                            // +0x34 the report text (its spots: get3 0x533a60)
    MissionUi* title;                                           // +0x38
    MWidget* links;                                             // +0x3c the pages that link here
    text_box* keys_panel;                                       // +0x40
    text_box* data_panel;                                       // +0x44
    Camera* keys_scroll;                                        // +0x48
    Camera* data_scroll;                                        // +0x4c
    MissionUi* btn_game_log;                                    // +0x50
    MissionUi* btn_recent;                                      // +0x54
    MissionUi* btn_troops;                                      // +0x58 page 1
    MissionUi* btn_quests;                                      // +0x5c page 4
    MissionUi* btn_factions;                                    // +0x60 page 2
    MissionUi* btn_locations;                                   // +0x64 page 3
    MissionUi* btn_return;                                      // +0x68
    MissionUi* btn_back;                                        // +0x6c
    MissionUi* btn_forward;                                     // +0x70
    MissionUi* btn_map;                                         // +0x74
    };
    };
    ItemD_vt() { w04 = 0; w08 = 99; }
    void sort_list(sel_list* l, int mode);      // 0x50cc70 (game/ui/widgets/text_box.cpp, ret 8: the body never reads this)
};
struct ItemD : ItemD_vt {                                       // vtable 0x5961c0
    // the page lists and build are ItemD's, not ItemD_vt's: 0x4cee50 stores ItemD's own vptr BEFORE the two vector
    // constructors, and cl constructs a class's own members inside that class's constructor
    std::vector<int> back_pages;                                // +0x78 the back list (page, arg pairs; VC7.1: allocator word, then first / last / end)
    std::vector<int> fwd_pages;                                 // +0x88 the forward list
    int back_sel_a, back_sel_b;                                 // +0x98 the page / argument on top of the back list
    int fwd_sel_a, fwd_sel_b;                                   // +0xa0 the page / argument on top of the forward list
    int page, arg;                                              // +0xa8 / +0xac the page shown (-1 after reset; note_window)
    float scroll_pos;                                           // +0xb0 the scroll ratio (-1.0f after reset)
    void build(int page, int arg);              // 0x50a1c0 (game/ui/screens/notes_window.cpp, ret 8): the page's widgets
    // overrides go on ItemD only: ItemD_vt's own table (the base vptr store in 0x4ceec0) must keep the head_rec slots
    virtual void reset_screen();                // slot 1  0x4c6e00 reset_gui (editor_selection_and_translation.cpp)
    virtual void init_screen();                // slot 2  0x50a140 ItemD::init_screen (character_sheet_view.cpp)
    virtual void frame_update(float);         // slot 3  0x533aa0 ItemD::frame_update (mission_menu_and_exchange_screens.cpp)
    virtual char pure_7() { return 1; }         // slot 7  0x48b170 (overrides the base's pure slot)
    ItemD();                                                    // 0x4cee50
};
MB_LAYOUT32_ASSERT(ItemD_size_check, sizeof(ItemD) == 0xb4);


#endif
