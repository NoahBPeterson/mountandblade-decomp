// named_rec.h: NamedRec, the dialog box screen that is slot 0 of the window manager (window_mgr.h, g_windows 0xa73980): a screen
// (ScreenObj: the vtable) with its answer state at +4 (0 = a message, 1 = a question) and three names. setup 0x48a3a0 (ret 8,
// ui/mission_ui.cpp) fills a message, setup2 0x48a3d0 (ret 0x10) a question whose answer lands at the pointer stored in +0x4c.
// The option pages build the message with the same stores inline (set_inline); the box is reached through
// WindowMgrW::box(). ONE definition for the script interpreter, the option pages, the character sheets and the mission
// screens (they had named it main_menu, menu_rec, MainMenu_s261e0, DialogBox, TutorialMenuRec and NamedRec).
#ifndef GAME_UI_NAMED_REC_H
#define GAME_UI_NAMED_REC_H
#include "../tstring.h"
#include "../screen_obj.h"

struct NamedRec : ScreenObj {
    int f04;                                // +0x04 answer state / the page selector
    char pad08[0x18 - 0x08];
    int sel;                                // +0x18
    tstring name1c;                         // +0x1c the text
    int f2c, f30, f34, f38, f3c, f40, f44, f48;
    int f4c;                                // +0x4c selection state / the answer pointer
    int f50;                                // +0x50
    tstring name54;                         // +0x54 the save dialog's slot name
    char pad64[0x68 - 0x64];
    tstring name68;                         // +0x68 the header
    tstring name78;                         // +0x78 the "yes" answer
    tstring name88;                         // +0x88 the "no" answer
    void setup(const tstring* a, const tstring* b);                                  // 0x48a3a0 (ret 8)
    void setup2(const tstring* a, const tstring* b, const tstring* c, int v);       // 0x48a3d0 (ret 0x10)
    // the script interpreter's spelling: the strings are temporaries (the callee takes pointers)
    __forceinline void show(const tstring& title, const tstring& text) { setup(&title, &text); }                                  // 0x48a3a0 (ret 8)
    __forceinline void ask(const tstring& text, const tstring& yes, const tstring& no, int* answer) { setup2(&text, &yes, &no, (int)answer); }   // 0x48a3d0 (ret 0x10)
    // the same stores as setup, inlined by the compiler into the option pages
    inline void set_inline(const tstring* hdr, const tstring* text)
    {
        name68 = *hdr;
        name1c = *text;
        f4c = 0;
        f50 = 0;
        f04 = 0;
    }
};
#endif // GAME_UI_NAMED_REC_H
