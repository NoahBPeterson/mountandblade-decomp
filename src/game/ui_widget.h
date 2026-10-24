// ui_widget.h: the UI widget interface (the engine's widget base: ctor 0x48ae20, vtable family 0x591c80 / 0x597050 / 0x598698
// ... derived per widget kind; MissionUi, src/game/ui/mission_ui.h, is the base's data layout). ONE definition of the slices'
// Widget / UiVt / UiWidget / Overlay vtable views for src/game/char_sheet, src/game/inventory_panel and
// src/game/options_panel. It names no engine type (every argument is a plain pointer or the includer's tstring), so it can be
// used in every TU universe; fields of the base are not declared here (the TUs that read them go through their own derived views).
//
// The slot offsets are the call offsets of the original (disassembly), not the declaration positions the slices counted:
// every slot below carries its byte offset; unnamed slots are `sNN` placeholders. Names are the best of the slices' spellings
// (the alternatives are in the comments). MSVC numbers a class's virtuals in declaration order, except that overloads of one
// name are placed in reverse order, so every slot here has its own name.
#ifndef MB_GAME_UI_WIDGET_H
#define MB_GAME_UI_WIDGET_H
// Needs `tstring` declared by the includer (game/tstring.h, or the options_panel TUs' typedef of rgl::string).

struct Widget {
    Widget* base_ctor();                                           // 0x48ae20 called as an ordinary member (returns this in eax; the slices' editor_ui)
    Widget();                                                      // 0x48ae20 (the base constructor; defined in src/game/ui/mission_ui.cpp as MissionUi::MissionUi)
    virtual void  d00(int release);                                // +0x00 deleting destructor (called with push 1 by MissionUi 0x488ad0)
    virtual bool  is_active();                                     // +0x04 "is active" (tested by 0x488720)
    virtual void s08();    // +0x08..
    virtual void  update(float dt);                                // +0x0c inventory_panel/game_app 0x50ec40
    virtual void s10(); virtual void s14(); virtual void s18(); virtual void s1c();    // +0x10..
    virtual void s20(); virtual void s24(); virtual void s28();    // +0x20..
    virtual void  activate(int v);                                 // +0x2c 
    virtual int   get_value();                                     // +0x30 
    virtual void  set_value(float v);                              // +0x34 
    virtual float get_float();                                     // +0x38 
    virtual void s3c(); virtual void s40();    // +0x3c..
    virtual char  changed(int flag);                               // +0x44 input test: 2 press, 4 release, 0x10000 click (the slices called it used / clicked_on)
    virtual void  set_key_flags(int f);                            // +0x48 
    virtual void  refresh_anim();                                  // +0x4c 
    virtual void s50(); virtual void s54();    // +0x50..
    virtual void  set_pos(const float* p);                         // +0x58 position pair (the slices: set_rect / set_xy)
    virtual void  on_child_moved(void* src);                       // +0x5c "child moved" notification, passed this (MissionUi d23)
    virtual void s60();    // +0x60..
    virtual void  set_scale(const float* p);                       // +0x64 (the slices: set_xy)
    virtual void  set_y(const float* p);                           // +0x68 
    virtual void  set_size(const float* p);                        // +0x6c (the slices: set_text_size / set_scale2)
    virtual void  set_offset(const float* p);                      // +0x70 (the slices: set_pos2 / set_position / set_size)
    virtual void  set_flag(const int* f);                          // +0x74 one argument: the call sites push only &flag (screen_init 0x4ec8d7)
    virtual void s78();    // +0x78..
    virtual void  set_mesh(int slot, void* mesh);                    // +0x7c (the originals push the mesh, then the slot; the slices: set_mesh(m, kind) / add_image(idx, m))
    virtual void  set_track(int slot, void* mesh);                 // +0x80 (the slices: add_image / set_mesh)
    virtual void s84(); virtual void s88(); virtual void s8c();    // +0x84..
    virtual void  refresh();                                       // +0x90 (the slices: set_default)
    virtual void  get_xy(void* out); // +0x94 `out` is the hidden pointer of the 8-byte non-POD result (the slices: set_default / get_xy(float* out) / get_text_size)
    virtual void  set_color(const unsigned* c);                       // +0x98 by address: the original pushes the row class's colour field address (the slices: set_colour)
    virtual void s9c(); virtual void sa0();    // +0x9c..
    virtual void* set_text(void* out, const tstring& text); // +0xa4 `out` is the hidden pointer of the 8-byte non-POD result (WRet, src/game/wret.h): callers discard it or read it back
    virtual void  get_text(tstring* out);                          // +0xa8 
    virtual void  set_text_at(const tstring& text, const float* pos); // +0xac 
    virtual void  hide();                                          // +0xb0 sets bit 0 of the flag word at +0x104 (0x488c90; MissionUi flag1_set). The slices called this slot show / hide by guess; call offsets, not names, are preserved
    virtual void  show();                                          // +0xb4 clears bit 0 (0x488ca0; the slices: show_selected / hide)
    virtual void  enable();                                        // +0xb8 
    virtual void  disable();                                       // +0xbc 
    virtual int   kind();                                          // +0xc0 widget kind (hud.h MissionItem)
    virtual void  update_children();                               // +0xc4 
    virtual void  add_child(void* child);                          // +0xc8 
};

// The base object that the constructor 0x48ae20 initialises (0x178 bytes; MissionUi, src/game/ui/mission_ui.h, names every
// field): only the members that the char_sheet / inventory TUs read are named, the rest is padding. Derived widget kinds
// (Overlay, TextBox, MapWidget, ...) extend it from +0x178.
struct WidgetObj : Widget {
    char pad04[0xe0 - 4];
    float rect_x, rect_y;                                          // +0xe0 / +0xe4 offset vector (MissionUi fE0 / fE4)
    float rect_w, rect_h;                                          // +0xe8 / +0xec (MissionUi fE8 / fEC)
    float size_x, size_y;                                          // +0xf0 / +0xf4 (MissionUi screen[0] / screen[1])
    char padf8[0x104 - 0xf8];
    unsigned flags;                                                // +0x104 flag word (bit 0 hidden, bit 1 tint)
    char pad108[0x121 - 0x108];
    char b121;                                                     // +0x121
    char pad122[0x14c - 0x122];
    unsigned color14c;                                             // +0x14c
    unsigned color150;                                             // +0x150
    char pad154[0x178 - 0x154];
};

#endif // MB_GAME_UI_WIDGET_H
