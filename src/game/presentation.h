// presentation.h: widget_mgr, the presentation (the running one is g_presentation, 0x5e299c): the overlays the module's
// presentation scripts create (a std::vector of MissionUi widgets), the presentation record it runs (g_presentations
// index, -1 none) with its flags, the clock it started on and how long it lasts, and the container its overlays are added
// to. ONE class: every screen record embeds one at +0x30 (MissionBlocker, ItemA / ItemB / ItemC, ItemRec / ItemRec2); the
// slices viewed it as BlockHead / BlockOwner (init 0x4dd920, activate 0x4dd9e0), MissionSource (the per-frame handler
// pick 0x4d5410), Presentation / c49e780::View (the script's overlay operations, create_text_overlay 0x49e780) and
// WidgetMgrW (the overlay makers of world_data). Light: the includers need only the forward declarations below.
#ifndef MB_GAME_PRESENTATION_H
#define MB_GAME_PRESENTATION_H
#include <vector>
#include "../include/timer.h"            // gui_timer
#include "../include/vec.h"              // vec2

struct MissionUi; struct ContainerBase; struct tstring;
struct widget_mgr;
extern widget_mgr* g_presentation;              // 0x5e299c the running presentation (0 when none)

struct widget_mgr {
    std::vector<MissionUi*> widgets;            // +0x00 the overlays (allocator word, first +4, last +8, end +0xc)
    int kind;                                   // +0x10 = -1 the presentation record (g_presentations index; -1 none; init 0x4dd920)
    int flags;                                  // +0x14 the record's flags (activate 0x4dd9e0; bit 0 read-only: no cursor, tick 0x5261e0)
    gui_timer tick;                             // +0x18 started on clock 0; activate restarts it, the animators fold it into duration
    float duration;                             // +0x28 = 1.0f the presentation ends once tick has run this long (0x48e5d0..)
    ContainerBase* scene;                       // +0x2c = 0 the container the overlays are added to (init builds it)
    widget_mgr() { duration = 1.0f; kind = -1; scene = 0; }   // inline in every screen record's constructor (0x4ce870, 0x4ce960, 0x4ceb30)
    ~widget_mgr();                              // 0x4c8bf0 (rgl/mesh2/gui_edit.cpp): g_presentation = 0, then the overlays' storage
    // end the presentation: no longer the running one, the overlay list emptied (VC7.1's clear() also frees it) (the screen tick
    // 0x5261e0 when the duration has run out, WindowMgrW::set_state 0x495700 on a screen change)
    void release() { g_presentation = 0; widgets.clear(); }

    // ---- the record (cls_map/cls_map.cpp) ----
    void init(ContainerBase* parent, int which);   // 0x4dd920 (ret 8): build the scene container under parent and start record `which`
    void activate();                            // 0x4dd9e0: becomes g_presentation; flags from the record, restart, its load triggers
    // ---- the per-frame handler (options_panel/hud_source.cpp) ----
    void pick(int* arg);                        // 0x4d5410 (ret 4; the callers pass &arg, the callback ignores it)
    // ---- the overlay operations of the script (script/script_interpreter.cpp, config/widget_mgr.cpp, world_data, mission_ui2,
    //      party2): each returns the new overlay's index in widgets ----
    int create_text_overlay(const tstring* text, int flags);   // 0x49e780 (ret 8)
    int create_button_overlay(const tstring& text, unsigned flags);   // 0x494110 (ret 8; world_data/world_data.cpp)
    int create_slider_overlay(int vmin, int vmax);  // 0x495970 (ret 8; world_data/mesh_hud.cpp)
    int create_mesh_overlay(int index);         // 0x4995e0 (ret 4; world_data/obj_tbl.cpp)
    int create_mesh_overlay_with_tableau(int a, int b, float c);   // 0x4996e0 (ret 0xc; world_data/obj_tbl.cpp)
    int create_game_button_overlay(const tstring& text);       // 0x499830
    int create_image_button_overlay(int a, int b);  // 0x499a20
    int create_check_box_overlay(int a, int b);     // 0x499b50
    int create_progress_overlay(int lo, int hi);    // 0x499b80
    int create_combo_button_overlay();          // 0x499d90
    int create_text_box_overlay();              // 0x49a0b0
    int create_image_button_overlay_with_tableau(int pos, int rec, int key);   // 0x4b14f0 (ret 0xc; src/game/party2)
    void set_size(int idx, const vec2& size);   // 0x49a300 (ret 8)
    void set_area_size(int idx, const vec2& size);  // 0x49ab80 (ret 8)
    // ---- src/game/mission_ui2: the widget setters of the script operations (overlay_set_* / overlay_animate_to_*) ----
    void* set_mesh_matrix(int idx, const void* m);  // 0x48db20 (ret 8: copies a 0x30-byte frame into the five child meshes)
    void set_color(int idx, unsigned rgb);      // 0x48dc30 (ret 8)
    void set_alpha(int idx, unsigned a);        // 0x48de00 (ret 8)
    void set_hilight_color(int idx, unsigned rgb);  // 0x48e1a0 (ret 8)
    void set_hilight_alpha(int idx, unsigned a);    // 0x48e270 (ret 8)
    void set_val(int idx, int v);               // 0x48e340 (ret 8)
    void set_limits(int idx, int lo, int hi);   // 0x48e4c0 (ret 0xc)
    void add_item(int idx, const tstring& text);    // 0x48e590 (ret 8)
    void set_text(int idx, const tstring* text);    // 0x48f5f0 (ret 8)
    void set_position(int idx, const vec2& pos);    // 0x48f6b0 (ret 8)
    void animate_pos(int idx, float t, const vec2& v);   // 0x48e5d0 (ret 0xc)
    void animate_color(int idx, float t, int rgb);  // 0x48e650 (ret 0xc)
    void animate_alpha(int idx, float t, int a);    // 0x48e6d0 (ret 0xc)
    void animate_hilight_color(int idx, float t, int rgb);   // 0x48e750 (ret 0xc)
    void animate_hilight_alpha(int idx, float t, int a);     // 0x48e7d0 (ret 0xc)
    void animate_size(int idx, float t, const vec2& v);  // 0x48e850 (ret 0xc)
};

#endif // MB_GAME_PRESENTATION_H
