// Slice s004d7370: the options panel's key names.
//   0x4d7370 options_panel::key_name  name of one of the seven selectable keys (which = 0..3: the four bindable keys
//                                     +0 menu, +8 prev, +0x10 select, +0x18 next, with the "ui_no_key_assigned"
//                                     string-table fallback; 4/5/6: Numpad Enter, Numpad +, Space)
//   0x4d7610 ctr::key_name            English name of a key code, returned by value (hidden return pointer first,
//                                     ret 8); the keyboard object (0xa170c8) is only `this`
// Names are provisional; every layout was derived from the disassembly of this slice.
//
// Shape of the original: 0x4d7370 comes first in the TU and calls 0x4d7610 out of line (noinline: it is defined
// after its caller). Its 0xb0-byte frame holds eleven distinct 16-byte temporaries (7 returned names + 4
// scratch "ui_no_key_assigned" strings), each case returning from inside. The cases of 0x4d7610 are in the
// original's source order (block address order, not key order): "1".."9","0","A".."Z"-ish, ... which is what makes
// its jump tables and code match byte for byte.
#include <windows.h>

// ---------------------------------------------------------------------------------------------------------
// tstring (16 bytes): read cursor, heap buffer, capacity, length (layout from s0046ad50/tstring.h).
struct tstring {
    int pos;                                                        // +0
    char* buf;                                                      // +4 (text lives at &buf while empty)
    int cap;                                                        // +8
    int len;                                                        // +0xc
    tstring();                                                      // 0x42bbd0
    tstring(const char* s);                                         // 0x42bbe0
    tstring(const tstring& o);                                      // 0x42c110
    ~tstring();                                                     // 0x42bc40
    void operator=(const tstring& o);                               // 0x42bc50
    void assign(const char* s);                                     // 0x42bcb0
};

#define UI_NO_KEY_ASSIGNED  "ui_no_key_assigned"                    // 0x59687c

// the keyboard object (global at 0xa170c8); key_name is a thiscall member returning the name by value
// (hidden return pointer first, then the key code; ret 8). The object itself is unused.
struct ctr {
    char pad0[0x100];
    __declspec(noinline) tstring key_name(int key);                 // 0x4d7610 (defined after its caller)
};
extern ctr g_keyboard;                                              // 0xa170c8

// the game's localized-string table (0x5e2a18): lookup by a scratch tstring holding the text key
struct StrTable {
    tstring* lookup(tstring* key);                                  // 0x43a1e0 (ret 4)
};
extern StrTable g_strtable;                                         // 0x5e2a18

// the options panel; the four bindable keys are read here
struct options_panel {
    int key_menu;                                                   // +0x00
    char pad04[4];                                                  // +0x04
    int key_prev;                                                   // +0x08
    char pad0c[4];                                                  // +0x0c
    int key_select;                                                 // +0x10
    char pad14[4];                                                  // +0x14
    int key_next;                                                   // +0x18
    void key_name(int which, tstring* out);                         // 0x4d7370
};

// @ 0x4d7370
void options_panel::key_name(int which, tstring* out)
{
    switch (which) {
    case 6:
        *out = g_keyboard.key_name(0x39);                           // Space
        return;
    case 4:
        *out = g_keyboard.key_name(0x4e);                           // Numpad Enter
        return;
    case 5:
        *out = g_keyboard.key_name(0x4a);                           // Numpad +
        return;
    case 0:
        if (key_menu < 0) {
            *out = *g_strtable.lookup(&tstring(UI_NO_KEY_ASSIGNED));
            return;
        }
        *out = g_keyboard.key_name(key_menu);
        return;
    case 1:
        if (key_prev < 0) {
            *out = *g_strtable.lookup(&tstring(UI_NO_KEY_ASSIGNED));
            return;
        }
        *out = g_keyboard.key_name(key_prev);
        return;
    case 2:
        if (key_select < 0) {
            *out = *g_strtable.lookup(&tstring(UI_NO_KEY_ASSIGNED));
            return;
        }
        *out = g_keyboard.key_name(key_select);
        return;
    case 3:
        if (key_next < 0) {
            *out = *g_strtable.lookup(&tstring(UI_NO_KEY_ASSIGNED));
            return;
        }
        *out = g_keyboard.key_name(key_next);
        return;
    }
}

// @ 0x4d7610
tstring ctr::key_name(int key)
{
    tstring s("unknown");                                           // 0x596b9c
    switch (key) {
    case 2: s.assign("1"); break;                                   // 0x596b98
    case 3: s.assign("2"); break;                                   // 0x596b94
    case 4: s.assign("3"); break;                                   // 0x596b90
    case 5: s.assign("4"); break;                                   // 0x596b8c
    case 6: s.assign("5"); break;                                   // 0x596b88
    case 7: s.assign("6"); break;                                   // 0x596b84
    case 8: s.assign("7"); break;                                   // 0x596b80
    case 9: s.assign("8"); break;                                   // 0x596b7c
    case 10: s.assign("9"); break;                                   // 0x596b78
    case 11: s.assign("0"); break;                                   // 0x590e24
    case 30: s.assign("A"); break;                                   // 0x596b74
    case 48: s.assign("B"); break;                                   // 0x596b70
    case 46: s.assign("C"); break;                                   // 0x596b6c
    case 32: s.assign("D"); break;                                   // 0x596b68
    case 18: s.assign("E"); break;                                   // 0x596b64
    case 33: s.assign("F"); break;                                   // 0x596b60
    case 34: s.assign("G"); break;                                   // 0x596b5c
    case 35: s.assign("H"); break;                                   // 0x596b58
    case 23: s.assign("I"); break;                                   // 0x596b54
    case 36: s.assign("J"); break;                                   // 0x596b50
    case 37: s.assign("K"); break;                                   // 0x596b4c
    case 38: s.assign("L"); break;                                   // 0x596b48
    case 50: s.assign("M"); break;                                   // 0x596b44
    case 49: s.assign("N"); break;                                   // 0x596b40
    case 24: s.assign("O"); break;                                   // 0x596b3c
    case 25: s.assign("P"); break;                                   // 0x596b38
    case 16: s.assign("Q"); break;                                   // 0x596b34
    case 19: s.assign("R"); break;                                   // 0x596b30
    case 31: s.assign("S"); break;                                   // 0x596b2c
    case 20: s.assign("T"); break;                                   // 0x596b28
    case 22: s.assign("U"); break;                                   // 0x596b24
    case 47: s.assign("V"); break;                                   // 0x596b20
    case 17: s.assign("W"); break;                                   // 0x596b1c
    case 45: s.assign("X"); break;                                   // 0x596b18
    case 21: s.assign("Y"); break;                                   // 0x596b14
    case 44: s.assign("Z"); break;                                   // 0x596b10
    case 82: s.assign("Numpad 0"); break;                                   // 0x596b04
    case 79: s.assign("Numpad 1"); break;                                   // 0x596af8
    case 80: s.assign("Numpad 2"); break;                                   // 0x596aec
    case 81: s.assign("Numpad 3"); break;                                   // 0x596ae0
    case 75: s.assign("Numpad 4"); break;                                   // 0x596ad4
    case 76: s.assign("Numpad 5"); break;                                   // 0x596ac8
    case 77: s.assign("Numpad 6"); break;                                   // 0x596abc
    case 71: s.assign("Numpad 7"); break;                                   // 0x596ab0
    case 72: s.assign("Numpad 8"); break;                                   // 0x596aa4
    case 73: s.assign("Numpad 9"); break;                                   // 0x596a98
    case 69: s.assign("Num Lock"); break;                                   // 0x596a8c
    case 181: s.assign("Numpad /"); break;                                   // 0x596a80
    case 55: s.assign("Numpad *"); break;                                   // 0x596a74
    case 74: s.assign("Numpad -"); break;                                   // 0x596a68
    case 78: s.assign("Numpad +"); break;                                   // 0x596a5c
    case 156: s.assign("Numpad Enter"); break;                                   // 0x596a4c
    case 83: s.assign("Numpad ."); break;                                   // 0x596a40
    case 210: s.assign("Insert"); break;                                   // 0x596a38
    case 211: s.assign("Delete"); break;                                   // 0x596a30
    case 199: s.assign("Home"); break;                                   // 0x596a28
    case 207: s.assign("End"); break;                                   // 0x596a24
    case 201: s.assign("Page Up"); break;                                   // 0x596a1c
    case 209: s.assign("Page Down"); break;                                   // 0x596a10
    case 200: s.assign("Up Arrow"); break;                                   // 0x596a04
    case 208: s.assign("Down Arrow"); break;                                   // 0x5969f8
    case 203: s.assign("Left Arrow"); break;                                   // 0x5969ec
    case 205: s.assign("Right Arrow"); break;                                   // 0x5969e0
    case 59: s.assign("F1"); break;                                   // 0x5969dc
    case 60: s.assign("F2"); break;                                   // 0x5969d8
    case 61: s.assign("F3"); break;                                   // 0x5969d4
    case 62: s.assign("F4"); break;                                   // 0x5969d0
    case 63: s.assign("F5"); break;                                   // 0x5969cc
    case 64: s.assign("F6"); break;                                   // 0x5969c8
    case 65: s.assign("F7"); break;                                   // 0x5969c4
    case 66: s.assign("F8"); break;                                   // 0x5969c0
    case 67: s.assign("F9"); break;                                   // 0x5969bc
    case 68: s.assign("F10"); break;                                   // 0x5969b8
    case 87: s.assign("F11"); break;                                   // 0x5969b4
    case 88: s.assign("F12"); break;                                   // 0x5969b0
    case 57: s.assign("Space"); break;                                   // 0x5969a8
    case 1: s.assign("Esc"); break;                                   // 0x5969a4
    case 28: s.assign("Enter"); break;                                   // 0x59699c
    case 15: s.assign("Tab"); break;                                   // 0x596998
    case 14: s.assign("Back Space"); break;                                   // 0x59698c
    case 26: s.assign("["); break;                                   // 0x596988
    case 27: s.assign("]"); break;                                   // 0x596984
    case 51: s.assign(","); break;                                   // 0x596980
    case 52: s.assign("."); break;                                   // 0x596828
    case 53: s.assign("/"); break;                                   // 0x59379c
    case 43: s.assign("\\"); break;                                   // 0x54a4b0
    case 13: s.assign("="); break;                                   // 0x54ba44
    case 12: s.assign("-"); break;                                   // 0x595ef4
    case 39: s.assign(";"); break;                                   // 0x59697c
    case 40: s.assign("'"); break;                                   // 0x54ba3c
    case 41: s.assign("~"); break;                                   // 0x596978
    case 58: s.assign("Caps Lock"); break;                                   // 0x59696c
    case 42: s.assign("Shift (left)"); break;                                   // 0x59695c
    case 54: s.assign("Shift (right)"); break;                                   // 0x59694c
    case 29: s.assign("Ctrl (left)"); break;                                   // 0x596940
    case 157: s.assign("Ctrl (right)"); break;                                   // 0x596930
    case 56: s.assign("Alt (left)"); break;                                   // 0x596924
    case 184: s.assign("Alt (right)"); break;                                   // 0x596918
    case 224: s.assign("Left Mouse Btn"); break;                                   // 0x596908
    case 225: s.assign("Right Mouse Btn"); break;                                   // 0x5968f8
    case 226: s.assign("Middle Mouse Btn"); break;                                   // 0x5968e4
    case 227: s.assign("Mouse Btn 4"); break;                                   // 0x5968d8
    case 228: s.assign("Mouse Btn 5"); break;                                   // 0x5968cc
    case 229: s.assign("Mouse Btn 6"); break;                                   // 0x5968c0
    case 230: s.assign("Mouse Btn 7"); break;                                   // 0x5968b4
    case 231: s.assign("Mouse Btn 8"); break;                                   // 0x5968a8
    case 238: s.assign("Scroll Up"); break;                                   // 0x59689c
    case 239: s.assign("Scroll Down"); break;                                   // 0x596890
    }
    return s;
}
