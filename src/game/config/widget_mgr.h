// widget_mgr.h: the editor window's widget manager (factory methods that build buttons, a progress bar, a
// drop-down and a party button, hook them into the scene and keep them in a vector), the preset placement
// helpers, and the widget classes that have no out-of-line members of their own. Used by widget_mgr.cpp
// (0x499830..0x49ad20) and widget_node.cpp (0x49ad20..). ALL NAMES PROVISIONAL; every offset cites a function.
//
// The widgets are MissionUi-derived (../ui/mission_ui.h; base ctor 0x48ae20). MissionUi names only some of its 50
// virtual slots; the factories call several with other argument types, so they go through WVt (a view of the same
// vtable with the signatures the call sites show) instead of editing that header.
#ifndef GAME_CONFIG_WIDGET_MGR_H
#define GAME_CONFIG_WIDGET_MGR_H
#include <new>
#include <vector>
#include "../tstring.h"
#include "../ui/ui_widgets.h"
#include "./ui_widgets_c.h"
#include "../../rgl/resource.h"

using rgl::vector4;
#include "../../include/vec.h"

// ---- the widget vtable as the factories see it (slot n at +4n) ------------------------------------------------
struct WVt {
    virtual void d00(); virtual bool d01(); virtual void d02();
    virtual void d03();                         // +0x0c  rebuild / finish layout (0x499a20, 0x499d90)
    virtual void d04(); virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08();
    virtual void d09(); virtual void d10();
    virtual void d11(int index);                // +0x2c  select / focus an item (src/game/cls_map: 0x4e05c0)
    virtual void d12();
    virtual void d13(float v);                  // +0x34  set value (0x499b80)
    virtual void d14(); virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18();
    virtual void d19(int flags);                // +0x4c  (0x49ad20)
    virtual void d20(int a);                    // +0x50  (0x49ad20)
    virtual void d21();
    virtual void d22(void* p);                  // +0x58  position pair (0x499830)
    virtual void d23(); virtual void d24();
    virtual void d25(void* v);                  // +0x64  set_xy
    virtual void d26();
    virtual void d27(void* v);                  // +0x6c  set_size
    virtual void d28(void* v);                  // +0x70  set_offset
    virtual void d29(void* v);                  // +0x74  (0x499830)
    virtual void d30(const int* v);             // +0x78  text flags (src/game/cls_map: 0x4dc630)
    virtual void d31(int kind, rgl::mesh* m);   // +0x7c  install a mesh (src/game/cls_map: 0x4dada0)
    virtual void d32(int slot, rgl::mesh* m);   // +0x80  install a mesh in slot 0..3 (0x499830)
    virtual void d33(); virtual void d34(); virtual void d35(); virtual void d36();
    virtual void d37(const void* color);        // +0x94  (src/game/cls_map: 0x4e05c0)
    virtual void d38(const void* color);        // +0x98  (src/game/cls_map: 0x4dada0)
    virtual void d39(const void* color);        // +0x9c  (0x499830)
    virtual void d40();
    virtual void d41(void* a, void* b);         // +0xa4  (0x499830, 0x49ad20)
    virtual void d42();
    virtual void d43(void* out, const void* text);   // +0xac  (src/game/cls_map: 0x4dada0)
    virtual void d44(); virtual void d45();
    virtual void d46();                         // +0xb8  hide (src/game/cls_map: 0x4dcef0)
    virtual void d47();
    virtual int  kind();                        // +0xc0  widget type code, 0..9 (0x49a300)
    virtual void d49();
    virtual void d50(void* child);              // +0xc8  container: add a child (0x499830, 0x49ad20)
};
#define vt(w) ((WVt*)(w))

// ---- mesh members the shared headers lack (same object as rgl::mesh) ----------------------------------------
// (src/rgl/core/resmgr.h has mesh_x::clone but cannot be included with ../ui/mission_ui.h: both define rgl::mesh.)
#include "../../rgl/mesh_x.h"      // mesh_x::set_color 0x47af30
struct gui_mesh : rgl::mesh {
    gui_mesh* clone();                          // 0x41c6d0
    void scale(float s);                        // 0x44f0c0
    void set_color(vector4 v) { ((mesh_x*)this)->set_color(v); }     // 0x47af30 (mesh_x::set_color, rgl/mesh_x.h; v by value, .w is copied uninitialised)
};
#define clone_gui(m) (((gui_mesh*)(m))->clone())

// the resource manager object (0x5d6fa0; resource.h's `resource_manager` is a partial view of it)
struct resmgr_gui : resource_manager {
    rgl::mesh* get_object(const rgl::string* name);     // 0x419540 (a mesh resource by name; callers clone it)
};
#define g_resmgr_full (*(resmgr_gui*)&g_resmgr)

// a 2-float vector with a constructor, for temporaries passed by pointer
// V2 (a vec2 with constructors) is in ../../include/vec.h

// ---- the editor's scene table: 0x48-byte records, placed(i) -> mesh (0x499050, another object) ---------------
struct scene_tbl {
    void* recs;                                 // +0
    rgl::mesh* placed(int index);               // 0x499050
};
extern scene_tbl g_meshes;                       // 0x8ed4f58
extern scene_tbl g_scene;                       // 0x8ed4f58  // old name of g_meshes, removed after the names merge

// ---- widget classes (vtable addresses in the comments) ---------------------------------------------------------
// The field initialisation of these sits in an intermediate base whose inline constructor the concrete classes
// inherit, so cl stores the concrete class's vptr after the field stores (0x499830, 0x499a20).
struct ButtonBase : MissionUi {
    int f178;                                   // +0x178  0x499830
    char pad17c[4];
    unsigned char b180;                         // +0x180
    char pad181[3];
    int f184;                                   // +0x184
    ButtonBase() { f178 = 0; b121 = 0; b180 = 0; f184 = 0; }
    void set_flag() { f184 = 1; }               // 0x499b50
};
// The slots these widgets override (config/widget_slots.cpp: each a leaf or a jmp to the function the original's table names)
struct MediumButton : ButtonBase {              // vtable 0x591e20, size 0x188
    bool d01();                                 // +0x04 returns 1 (0x48b170)
    void d10(rgl::scene* sc, float* count, int c, int d);   // +0x28 -> UiA::update 0x48b320
    char d35(unsigned mask);                    // +0x8c -> UiA::key 0x48b200
    void d41(float* out, const tstring* s);     // +0xa4 -> MissionUi::make_text 0x48b3a0
    void d44();                                 // +0xb0 -> MissionUi::flag1 0x48b1a0
    void d46();                                 // +0xb8 -> MissionUi::flag2 0x48b180
    int d48();                                  // +0xc0 -> UiA::type9 0x48b390
};
struct XyzButton : ButtonBase {                 // vtable 0x591ee8, size 0x188
    bool d01();                                 // +0x04 returns 1 (0x48b170)
    void d02(int a);                            // +0x08 -> HudBar::update_2 0x4966b0
    void d10(rgl::scene* sc, float* count, int c, int d);   // +0x28 -> UiA::update 0x48b320
    char d35(unsigned mask);                    // +0x8c -> UiA::key 0x48b200
    void d44();                                 // +0xb0 -> MissionUi::flag1 0x48b1a0
    void d46();                                 // +0xb8 -> MissionUi::flag2 0x48b180
    int d48();                                  // +0xc0 -> UiA::type8 0x48b4a0
};

// UiA (../ui/ui_widgets.h, vtable 0x591d58) as DropDown::create builds it: the constructor 0x48b1c0 is inlined there
// (intermediate base stores f178/b121, then the vptr, then f180/f184), so it needs its own inline constructor.
struct UiABase : MissionUi {
    int f178;                                   // +0x178
    char pad17c[4];
    char f180;                                  // +0x180
    char pad181[3];
    int f184;                                   // +0x184
    UiABase() { f178 = 0; b121 = 0; }
};
struct UiAInline : UiABase {                    // vtable 0x591d58 (= UiA's; the same overrides, config/widget_slots.cpp)
    bool d01();                                 // +0x04 returns 1 (0x48b170)
    void d10(rgl::scene* sc, float* count, int c, int d);   // +0x28 -> UiA::update 0x48b320
    char d35(unsigned mask);                    // +0x8c -> UiA::key 0x48b200
    void d44();                                 // +0xb0 -> MissionUi::flag1 0x48b1a0
    void d46();                                 // +0xb8 -> MissionUi::flag2 0x48b180
    int d48();                                  // +0xc0 -> UiA::type 0x48b1f0
    UiAInline() { f180 = 0; f184 = 0; }
};

// The list container a drop-down opens (vtable 0x592848, size 0x190). Its base constructor 0x4931a0 and base
// destructor 0x490520 belong to the object at 0x493xxx; the vptr store and f188 = 1 are NodeA's own (0x49ad20).
struct ContainerBase : MissionUi {
    ContainerBase();                            // 0x4931a0
    ~ContainerBase();                           // 0x490520
    char pad178[0x188 - 0x178];
    char f188;                                  // +0x188 (1 for a NodeA)
    char pad189[0x190 - 0x189];
};
struct NodeA : ContainerBase {
    // the slots vtable 0x592848 holds that the base's does not (config/widget_slots.cpp: jmps to ContainerW's members)
    void d00(int release);                      // +0x00 -> destroy 0x49b0f0
    void d02(int a);                            // +0x08 -> ContainerW::update_all 0x4908c0
    void d10(rgl::scene* sc, float* count, int c, int d);   // +0x28 -> ContainerW::bounds_of 0x490710
    void d16();                                 // +0x40 -> ContainerW::reset_all 0x4906e0
    void d22(void* p);                          // +0x58 -> ContainerW::place 0x490570
    void d23(MissionUi* child);                 // +0x5c -> ContainerW::track_bounds 0x4905c0
    void d24();                                 // +0x60 -> ContainerW::activate_all 0x4906b0
    char d35(unsigned mask);                    // +0x8c -> gui_window::handle_event 0x42d170
    void refresh();                             // +0x90 -> gui_window::wheel_focus 0x42cec0
    virtual void update_children();             // +0xc4, vtable slot 49 (0x48ef40, FrameBox::release)
    virtual void d50(MissionUi* child);         // +0xc8, vtable slot 50 -> ContainerW::add_child 0x493a30
    NodeA() { f188 = 1; }
    void* destroy(unsigned flags);              // 0x49b0f0 (scalar deleting destructor)
};

struct DropDown : MissionUi {                   // ctor 0x4931d0 (installs its own vtable), size 0x1d0
    char pad178[0x180 - 0x178];
    char f180;                                  // +0x180 open (0x49aca0)
    char pad181[0x188 - 0x181];
    NodeA* f188;                         // +0x188 the open list container (0x49aca0)
    char pad18c[4];
    tstring* rows_first;                        // +0x190 std::vector of 16-byte text rows (0x49aca0, 0x49ad20)
    tstring* rows_last;                         // +0x194
    tstring* rows_end;                          // +0x198
    char pad19c[4];
    rgl::mesh* m1a0;                            // +0x1a0 (0x499d90)
    rgl::mesh* m1a4;                            // +0x1a4
    rgl::mesh* m1a8;                            // +0x1a8
    int f1ac;                                   // +0x1ac
    float f1b0, f1b4, f1b8;                     // +0x1b0 row size, indent (0x49ad20)
    float f1bc, f1c0, f1c4, f1c8;               // +0x1bc child size / offset factors
    int pad1cc;
    DropDown();                                 // 0x4931d0
    char on_event(unsigned ev);                 // 0x49aca0
    NodeA* create();                     // 0x49ad20
};

// ---- widget items as set_size sees them ---------------------------------------------------------------------
struct WItem : MissionUi {
    void terrain_refresh(const float* pos);     // 0x48f120 (ret 4)
    void scroll_refresh(const float* pos);   // 0x498420 (ret 4; the definition is HudOuter::scroll_refresh, world_data/mesh_hud.h: widget_mgr.cpp calls that)
};

// MissionUi::layout_text_to (0x49abc0) lives in this object; ../ui/mission_ui.h lacks it
struct Font;
struct MissionUiText : MissionUi {
    void layout_text_to(const tstring* text, const float* pos);     // 0x49abc0 (ret 8)
};
#include "../font.h"
extern Font* g_font;                            // 0x5e2934

// ---- the manager: a std::vector of widgets at +0 (first at +4), the scene container at +0x2c ------------------
struct widget_mgr {
    std::vector<MissionUi*> widgets;            // +0x00  0x499830
    char pad10[0x18 - 0x10];
    int clock;                                  // +0x18  index into g_ticks of the animation clock (0x48e5d0..0x48e850)
    char pad1c[4];
    int stamp_lo, stamp_hi;                     // +0x20  g_ticks[clock] when the last animation was started (read as one __int64)
    float busy_until;                           // +0x28  seconds until the running animations have finished (0x48e5d0..)
    WVt* scene;                                 // +0x2c  vtable slot 50 adds a widget (0x499830)

    int create_game_button_overlay(int id);                     // 0x499830
    int create_image_button_overlay(int a, int b);                  // 0x499a20
    int create_check_box_overlay(int a, int b);          // 0x499b50
    int create_progress_overlay(int lo, int hi);           // 0x499b80
    int create_combo_button_overlay();                         // 0x499d90
    int create_text_box_overlay();                            // 0x49a0b0
    int create_image_button_overlay_with_tableau(int pos, int rec, int key);   // 0x4b14f0 (ret 0xc; src/game/party2)
    void set_size(int idx, const float* pos);      // 0x49a300 (ret 8)
    void set_area_size(int idx, const float* pos);    // 0x49ab80 (ret 8)
    // ---- src/game/mission_ui2: the widget setters of the script operations (overlay_set_* / overlay_animate_to_*) ----
    void* set_mesh_matrix(int idx, const void* m);      // 0x48db20 (ret 8: copies a 0x30-byte frame into the five child meshes)
    void set_color(int idx, unsigned rgb);              // 0x48dc30 (ret 8)
    void set_alpha(int idx, unsigned a);                // 0x48de00 (ret 8)
    void set_hilight_color(int idx, unsigned rgb);      // 0x48e1a0 (ret 8)
    void set_hilight_alpha(int idx, unsigned a);        // 0x48e270 (ret 8)
    void set_val(int idx, int v);                       // 0x48e340 (ret 8)
    void set_limits(int idx, int lo, int hi);           // 0x48e4c0 (ret 0xc)
    void add_item(int idx, int v);                      // 0x48e590 (ret 8)
    void set_text(int idx, const tstring* text);        // 0x48f5f0 (ret 8)
    void set_position(int idx, const float* pos);       // 0x48f6b0 (ret 8)
    void animate_pos(int idx, float t, int v);          // 0x48e5d0 (ret 0xc)
    void animate_color(int idx, float t, int rgb);      // 0x48e650 (ret 0xc)
    void animate_alpha(int idx, float t, int a);        // 0x48e6d0 (ret 0xc)
    void animate_hilight_color(int idx, float t, int rgb);   // 0x48e750 (ret 0xc)
    void animate_hilight_alpha(int idx, float t, int a);     // 0x48e7d0 (ret 0xc)
    void animate_size(int idx, float t, int v);         // 0x48e850 (ret 0xc)
};

#endif
