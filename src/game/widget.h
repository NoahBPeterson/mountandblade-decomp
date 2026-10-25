// widget.h: the UI element base (0x178 bytes, constructor 0x48ae20 installs vptr 0x591c80, the 49-slot vtable whose entries were read
// from the binary: argument counts below are the `ret N` of each slot, e.g. +0x7c ret 8 = two arguments). ONE definition for src/game:
// char_sheet (quest_log.h, facegen_init.h), inventory_panel (view_open.cpp, widget_text.cpp) and strat (gui_manager.h: the container
// that adds the `children` vector at +0x178) each declared a partial view of it. Slots without a call site are placeholders named by
// their byte offset. The includer must have `tstring` declared; `rgl::mesh` is only forward-declared. Needs no other header.
#ifndef GAME_WIDGET_H
#define GAME_WIDGET_H

namespace rgl { struct mesh; }
struct tstring;
struct widget {
    virtual ~widget();                          // +0x00 scalar deleting destructor (0x502300)
    virtual void s04();                         // +0x04
    virtual void s08();                         // +0x08
    virtual void s0c();                         // +0x0c
    virtual void set_a(float a, int b);           // +0x10 (ui/mission_ui.h MissionUi::set_a 0x488560)
    virtual void s14(float a, int b);           // +0x14
    virtual void s18(float a, int b);           // +0x18
    virtual void s1c(float a, int b);           // +0x1c
    virtual void s20(int a, int b);             // +0x20
    virtual void s24(int a, int b);             // +0x24
    virtual void render(int a, void* v, int b, int c);   // +0x28 (0x488720, GuiCtx::follow)
    virtual void set_focus(int index);          // +0x2c (strat wheel_focus 0x42cec0)
    virtual void s30();                         // +0x30
    virtual void s34(float v);                  // +0x34 (face-gen window: the slider value)
    virtual float value();                      // +0x38
    virtual char hit_test(unsigned mask);       // +0x3c (strat wheel_focus 0x42cec0)
    virtual void s40();                         // +0x40
    virtual void s44(int a);                    // +0x44
    virtual void on_input(unsigned mask);       // +0x48 (strat refresh_all 0x42cd60, mouse_event 0x42cf50)  // old name of gui_window::refresh_all, removed after the names merge
    virtual void s4c(int a);                    // +0x4c
    virtual void s50(int a);                    // +0x50
    virtual void s54(int a);                    // +0x54
    virtual void set_pos(const float* p);       // +0x58
    virtual void s5c(int a);                    // +0x5c
    virtual void s60();                         // +0x60
    virtual void set_size(const float* s);      // +0x64
    virtual void s68(int a);                    // +0x68
    virtual void set_color(const float* c);     // +0x6c
    virtual void s70(int a);                    // +0x70
    virtual void set_flags(const int* f);       // +0x74
    virtual void s78(int a);                    // +0x78
    virtual void add_item(int slot, rgl::mesh* m);    // +0x7c (ret 8)
    virtual void add_item2(int slot, rgl::mesh* m);   // +0x80 (ret 8)
    virtual void s84(int a);                    // +0x84
    virtual void s88(int a);                    // +0x88
    virtual char on_event(unsigned mask);       // +0x8c (strat wheel_focus, mouse_event)
    virtual void refresh();                     // +0x90 (strat refresh_all, mouse_event)
    virtual void s94(int a);                    // +0x94
    virtual void set_value_a(int* v);           // +0x98
    virtual void set_value_b(int* v);           // +0x9c
    virtual void sa0(int a);                    // +0xa0
    virtual void set_text(tstring* s, const void* p);   // +0xa4 (ret 8)
    virtual void sa8(int a);                    // +0xa8
    virtual void sac(int a, int b);             // +0xac (ret 8)
    virtual void sb0();                         // +0xb0
    virtual void sb4();                         // +0xb4
    virtual void sb8();                         // +0xb8
    virtual void sbc();                         // +0xbc
    virtual void sc0();                         // +0xc0
    virtual void sc4();                         // +0xc4
    virtual void add_child(widget* c);          // +0xc8 (the container classes: elem / overlay)

    char pad4[0xcc - 4];
    int pending;                                // +0xcc  (strat wheel_step 0x42ce50)
    int enabled_mask;                           // +0xd0  (wheel_step)
    int wheel;                                  // +0xd4  bit0 up, bit1 down, bit2 focus-on-top (wheel_step, wheel_focus)
    char padd8[0xe0 - 0xd8];
    float x0, y0;                               // +0xe0, +0xe4  (mouse_event: origin added to the child's rectangle)
    float ex;                                   // +0xe8 (text/widget flavour; the ctor stores 0 here)
    float ey;                                   // +0xec
    float x1, y1;                               // +0xf0, +0xf4 upper bound of the rectangle (mouse_event 0x42cf50); the extent when the low corner is the origin (face-gen window)
    float x_lo, y_lo;                           // +0xf8, +0xfc lower bound
    char pad100[0x104 - 0x100];
    unsigned flags;                             // +0x104 (bits 0-1: hidden/disabled; mouse_event; widget_text flag1 0x50cc60)
    char pad108[0x121 - 0x108];
    char hover;                                 // +0x121 (mouse_event)
    char pad122[0x158 - 0x122];
    widget* focus;                              // +0x158 (wheel_focus)
    int sel;                                    // +0x15c (wheel_step)
    int sel_lo, sel_hi;                         // +0x160, +0x164
    char pad168[0x178 - 0x168];

    widget() throw();                           // 0x48ae20
    unsigned flag1() const;                     // 0x50cc60
};
typedef char widget_size_check[(sizeof(widget) == 0x178) ? 1 : -1];

#endif // GAME_WIDGET_H
