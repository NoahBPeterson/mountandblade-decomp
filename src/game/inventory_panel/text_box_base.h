// text_box_base.h: the text box's widget base as widget_text.cpp declares it (the TU's rgl.h cannot go with ui/mission_ui.h, so
// the class cannot derive from MissionUi there): the 50 virtuals of vtable 0x597050 with MissionUi's slot names and signatures
// (ui/mission_ui.h), and no data but the 0x178-byte base record. Every slot is defined in text_box_slots.cpp (a TU that does include
// mission_ui.h): a leaf the original's slot holds or a bare `jmp` to the function it names. Needs only forward declarations.
#ifndef GAME_INVENTORY_PANEL_TEXT_BOX_BASE_H
#define GAME_INVENTORY_PANEL_TEXT_BOX_BASE_H
struct tstring;
namespace rgl { struct scene; }

struct text_box_base {
    virtual void* destroy(unsigned flags);          // +0x00 slot 0 -> UiFrame::destroy 0x5021f0
    virtual bool d01();                             // +0x04 returns 0
    virtual void d02(int a);                        // +0x08 -> ContainerW::update_all 0x4908c0
    virtual void d03();                             // +0x0c -> FieldPanel::update_bounds 0x48d020
    virtual void d04(float a, int b);               // +0x10 -> MissionUi::set_a
    virtual void d05(float a, int b);               // +0x14 -> set_b
    virtual void d06(float a, int b);               // +0x18 -> set_c
    virtual void d07(float a, int b);               // +0x1c -> set_d
    virtual void d08(float a, float* p);            // +0x20 -> set_e
    virtual void d09(float a, float* p);            // +0x24 -> set_f
    virtual void d10(rgl::scene* sc, float* count, int c, int d);   // +0x28 -> ContainerW::bounds_of 0x490710
    virtual void d11(int v);                        // +0x2c -> set15c
    virtual int d12();                              // +0x30 -> get15c
    virtual void d13(int v);                        // +0x34 -> set168
    virtual float d14();                            // +0x38 -> get168
    virtual bool d15(int m);                        // +0x3c -> test_c8
    virtual void d16();                             // +0x40 -> ContainerW::reset_all 0x4906e0
    virtual bool d17(int m);                        // +0x44 -> clear_mask
    virtual void set_key_flags(int f);              // +0x48 -> or_flags
    virtual void d19(int v);                        // +0x4c -> set_d0
    virtual void d20(int v);                        // +0x50 -> set_d4
    virtual void d21(int m);                        // +0x54 -> or_d4
    virtual void d22(void* p);                      // +0x58 -> ContainerW::place 0x490570
    virtual void d23(void* child);                  // +0x5c -> ContainerW::track_bounds 0x4905c0
    virtual void d24();                             // +0x60 -> ContainerW::activate_all 0x4906b0
    virtual void set_xy(float* v);                  // +0x64 -> move_f0
    virtual void d26(float* v);                     // +0x68 -> move_f8
    virtual void set_size(float* v);                // +0x6c -> set_text_pos
    virtual void set_offset(float* v);              // +0x70 -> set138
    virtual void d29(float* v);                     // +0x74 -> set148
    virtual void d30(float* v);                     // +0x78 -> or148
    virtual void d31(int a, int b);                 // +0x7c -> run
    virtual void d32(int a, int b);                 // +0x80 -> set_track
    virtual void d33(int a);                        // +0x84 -> MeshX::normalize_uv 0x495fa0
    virtual void d34(void* a);                      // +0x88 `ret 4`
    virtual char d35(unsigned mask);                // +0x8c -> gui_window::mouse_event 0x42cf50
    virtual void refresh();                         // +0x90 -> gui_window::wheel_focus 0x42cec0
    virtual void get_xy(float* v);                  // +0x94 -> PObj::text_rect 0x49b4f0
    virtual void set_color(unsigned* c);            // +0x98 -> set14c
    virtual void d39(void* p);                      // +0x9c -> set_target
    virtual void d40(int p);                        // +0xa0 -> set_pos
    virtual void d41(float* out, const tstring* s); // +0xa4 -> set_text
    virtual void d42(tstring* out);                 // +0xa8 -> put
    virtual void d43(const tstring* s, const float* p);   // +0xac -> MissionUiText::layout_text_to 0x49abc0
    virtual void d44();                             // +0xb0 -> hide
    virtual void d45();                             // +0xb4 -> show
    virtual void d46();                             // +0xb8 -> flag2_set
    virtual void d47();                             // +0xbc -> flag2_clear
    virtual int d48();                              // +0xc0 returns 0
    virtual void d49();                             // +0xc4 -> FrameBox::release 0x48ef40
    char pad4[0x178 - 4];
    text_box_base();                                // 0x48ae20 (MissionUi::MissionUi)
};
#endif
