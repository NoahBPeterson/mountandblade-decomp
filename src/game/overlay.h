// overlay.h: the container widgets Overlay (the slices' Overlay / Panel: ctor 0x4931a0, 0x190 bytes, vtable 0x592848, the
// children vector at +0x178 and a byte at +0x188) and TextBox (the same constructor, 0x188 bytes, vtable 0x597050), plus the
// vec<T> view of the VC7.1 std::vector layout they hold their children in. ONE definition for char_sheet (screen_init,
// screen_camera: the campaign screen's overlays; conv_open: Panel / TextBox of the conversation screen). The screen_init /
// screen_camera views placed `first` / `last` at +0x184 / +0x188 through a miscounted pad; the constructors (conv_open.h,
// disassembled) put the vector at +0x178 and the flag byte at +0x188.
#ifndef MB_GAME_OVERLAY_H
#define MB_GAME_OVERLAY_H
#include "./ui_widget.h"

// std::vector<T> (0x10 bytes): control block, first, last, end
template <class T> struct vec {
    void* alloc;
    T* first;
    T* last;
    T* end;
    // (no out-of-line push_back: 0x4927d0 is _Insert_n(where, n, &v); the callers inline push_back, see cs_common.h's
    // push_back_inline. A one-argument declaration here linked to an unrelated vec<Widget*>::push_back COMDAT.)
    void push_back2(const T& v);                // 0x4936b0 (std::vector<T>::push_back)
    void fill_n(const T& v, int size);      // 0x40f8a0 (incremental copy_push_back)
};

struct Overlay : WidgetObj {                    // ctor 0x4931a0, vtable 0x592848, 0x190 bytes
    vec<Widget*> children;                      // +0x178
    unsigned char f188;                         // +0x188
    char pad189[3];
    Overlay();                                  // 0x4931a0
};

struct TextBox : WidgetObj {                    // ctor 0x4931a0, vtable 0x597050, 0x188 bytes
    vec<Widget*> children;                      // +0x178
};

#endif // MB_GAME_OVERLAY_H
