// Slice s004d7370: the options panel's key names.
//   0x4d7370 options_panel::key_name  name of one of the four bindable keys (+0 menu, +8 prev, +0x10 select,
//                                     +0x18 next), translated through the game's string table
//   0x4d7610 key_name                 English name of a key code
// Names are provisional; every layout was derived from the disassembly of this slice.
//
// Shape of the original (what the disassembly shows):
//  - 0x4d7370 has one 0xb0-byte frame: three tstring temporaries (0x30), plus the case that needs it keeps the
//    "ui_no_key_assigned" tstring alive across the strtab_lookup call and deletes it after the out-parameter
//    assignment. Leftover build flags make the case-local temporaries share stack slots across cases.
//  - 0x4d7610 has a 0x10-byte frame holding the name, and copies it into the 16 bytes above the key code
//    (0x42c110 into the caller's slot, then 0x42bc40 on its own copy): `return tstring(name)`.
//    It returns the keyboard object in EAX (`mov eax, esi; pop esi`), the register it was called with.
// This source is complete (every case, every call, every lookup of the original) but not byte-exact.
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
    tstring* assign(const char* s);                                 // 0x42bcb0 (returns this)
};

#define UI_NO_KEY_ASSIGNED  "ui_no_key_assigned"                    // 0x59687c

// the keyboard object (global at 0xa170c8); the key-name function only passes it through
struct ctr {
    char pad0[0x100];
};

// the game's localized-string table (0x5e2a18): lookup by a scratch tstring holding the text key
tstring* strtab_lookup(tstring* key);                               // 0x43a1e0 (this = table, ret 4)

// the options panel; only the four bindable keys are read here
//  +0x2c..+0x38 controls, +0x40/+0x44 import/export entries (see 0x4d7d90/0x4d8000)
struct options_panel {
    int key_menu;                                                   // +0x00
    char pad04[4];                                                  // +0x04
    int key_prev;                                                   // +0x08
    char pad0c[4];                                                  // +0x0c
    int key_select;                                                 // +0x10
    char pad14[4];                                                  // +0x14
    int key_next;                                                   // +0x18
};

// @ 0x4d7610
ctr* key_name(ctr* self, int key, tstring* out)
{
    tstring s("unknown");                                           // 0x596b9c
    const char* n = 0;
    switch (key) {
    case 1: n = "1"; break;                                         // 0x596b98
    case 2: n = "2"; break;                                         // 0x596b94
    case 3: n = "3"; break;                                         // 0x596b90
    case 4: n = "4"; break;                                         // 0x596b8c
    case 5: n = "5"; break;                                         // 0x596b88
    case 6: n = "6"; break;                                         // 0x596b84
    case 7: n = "7"; break;                                         // 0x596b80
    case 8: n = "8"; break;                                         // 0x596b7c
    case 9: n = "9"; break;                                         // 0x596b78
    case 10: n = "0"; break;                                        // 0x590e24
    case 11: n = "-"; break;                                        // 0x595ef4
    case 12: n = "="; break;                                        // 0x54ba44
    case 13: n = "Esc"; break;                                      // 0x5969a4
    case 14: n = "Back Space"; break;                               // 0x59698c
    case 15: n = "Tab"; break;                                      // 0x596998
    case 16: n = "Q"; break;                                        // 0x596b34
    case 17: n = "W"; break;                                        // 0x596b1c
    case 18: n = "E"; break;                                        // 0x596b64
    case 19: n = "R"; break;                                        // 0x596b30
    case 20: n = "T"; break;                                        // 0x596b28
    case 21: n = "Y"; break;                                        // 0x596b14
    case 22: n = "U"; break;                                        // 0x596b24
    case 23: n = "I"; break;                                        // 0x596b54
    case 24: n = "O"; break;                                        // 0x596b3c
    case 25: n = "P"; break;                                        // 0x596b38
    case 26: n = "["; break;                                        // 0x596988
    case 27: n = "]"; break;                                        // 0x596984
    case 28: n = "Enter"; break;                                    // 0x59699c
    case 29: n = "Ctrl (left)"; break;                              // 0x596940
    case 30: n = "A"; break;                                        // 0x596b74
    case 31: n = "S"; break;                                        // 0x596b2c
    case 32: n = "D"; break;                                        // 0x596b68
    case 33: n = "F"; break;                                        // 0x596b60
    case 34: n = "G"; break;                                        // 0x596b5c
    case 35: n = "H"; break;                                        // 0x596b58
    case 36: n = "J"; break;                                        // 0x596b50
    case 37: n = "K"; break;                                        // 0x596b4c
    case 38: n = "L"; break;                                        // 0x596b48
    case 39: n = ";"; break;                                        // 0x59697c
    case 40: n = "'"; break;                                        // 0x54ba3c
    case 41: n = "~"; break;                                        // 0x596978
    case 42: n = "Shift (left)"; break;                             // 0x59695c
    case 43: n = "\\"; break;                                       // 0x54a4b0
    case 44: n = "Z"; break;                                        // 0x596b10
    case 45: n = "X"; break;                                        // 0x596b18
    case 46: n = "C"; break;                                        // 0x596b6c
    case 47: n = "V"; break;                                        // 0x596b20
    case 48: n = "B"; break;                                        // 0x596b70
    case 49: n = "N"; break;                                        // 0x596b40
    case 50: n = "M"; break;                                        // 0x596b44
    case 51: n = ","; break;                                        // 0x596980
    case 52: n = "."; break;                                        // 0x596828
    case 53: n = "/"; break;                                        // 0x59379c
    case 54: n = "Shift (right)"; break;                            // 0x59694c
    case 55: n = "Numpad *"; break;                                 // 0x596a74
    case 56: n = "Alt (left)"; break;                               // 0x596924
    case 57: n = "Space"; break;                                    // 0x5969a8
    case 58: n = "Caps Lock"; break;                                // 0x59696c
    case 59: n = "F1"; break;                                       // 0x5969dc
    case 60: n = "F2"; break;                                       // 0x5969d8
    case 61: n = "F3"; break;                                       // 0x5969d4
    case 62: n = "F4"; break;                                       // 0x5969d0
    case 63: n = "F5"; break;                                       // 0x5969cc
    case 64: n = "F6"; break;                                       // 0x5969c8
    case 65: n = "F7"; break;                                       // 0x5969c4
    case 66: n = "F8"; break;                                       // 0x5969c0
    case 67: n = "F9"; break;                                       // 0x5969bc
    case 68: n = "F10"; break;                                      // 0x5969b8
    case 69: n = "Num Lock"; break;                                 // 0x596a8c
    case 71: n = "Numpad 7"; break;                                 // 0x596ab0
    case 72: n = "Numpad 8"; break;                                 // 0x596aa4
    case 73: n = "Numpad 9"; break;                                 // 0x596a98
    case 74: n = "Numpad -"; break;                                 // 0x596a68
    case 75: n = "Numpad 4"; break;                                 // 0x596ad4
    case 76: n = "Numpad 5"; break;                                 // 0x596ac8
    case 77: n = "Numpad 6"; break;                                 // 0x596abc
    case 78: n = "Numpad +"; break;                                 // 0x596a5c
    case 79: n = "Numpad 1"; break;                                 // 0x596af8
    case 80: n = "Numpad 2"; break;                                 // 0x596aec
    case 81: n = "Numpad 3"; break;                                 // 0x596ae0
    case 82: n = "Numpad 0"; break;                                 // 0x596b04
    case 83: n = "Numpad ."; break;                                 // 0x596a40
    case 87: n = "F11"; break;                                      // 0x5969b4
    case 88: n = "F12"; break;                                      // 0x5969b0
    case 156: n = "Numpad Enter"; break;                            // 0x596a4c
    case 157: n = "Ctrl (right)"; break;                            // 0x596930
    case 181: n = "Numpad /"; break;                                // 0x596a80
    case 184: n = "Alt (right)"; break;                             // 0x596918
    case 199: n = "Home"; break;                                    // 0x596a28
    case 200: n = "Up Arrow"; break;                                // 0x596a04
    case 201: n = "Page Up"; break;                                 // 0x596a1c
    case 203: n = "Left Arrow"; break;                              // 0x5969ec
    case 205: n = "Right Arrow"; break;                             // 0x5969e0
    case 207: n = "End"; break;                                     // 0x596a24
    case 208: n = "Down Arrow"; break;                              // 0x5969f8
    case 209: n = "Page Down"; break;                               // 0x596a10
    case 210: n = "Insert"; break;                                  // 0x596a38
    case 211: n = "Delete"; break;                                  // 0x596a30
    case 224: n = "Left Mouse Btn"; break;                          // 0x596908
    case 225: n = "Right Mouse Btn"; break;                         // 0x5968f8
    case 226: n = "Middle Mouse Btn"; break;                        // 0x5968e4
    case 227: n = "Mouse Btn 4"; break;                             // 0x5968d8
    case 228: n = "Mouse Btn 5"; break;                             // 0x5968cc
    case 229: n = "Mouse Btn 6"; break;                             // 0x5968c0
    case 230: n = "Mouse Btn 7"; break;                             // 0x5968b4
    case 231: n = "Mouse Btn 8"; break;                             // 0x5968a8
    case 238: n = "Scroll Up"; break;                               // 0x59689c
    case 239: n = "Scroll Down"; break;                             // 0x596890
    }
    if (n)
        s.assign(n);
    *out = s;
    return self;
}

// @ 0x4d7370
void options_panel_key_name(options_panel* self, int which, tstring* out)
{
    tstring no_key(UI_NO_KEY_ASSIGNED);                             // 0x59687c
    switch (which) {
    case 0:
        if (self->key_menu < 0) {
            *out = *strtab_lookup(&no_key);
            break;
        }
        key_name((ctr*)0xa170c8, self->key_menu, out);
        break;
    case 1:
        if (self->key_prev < 0) {
            *out = *strtab_lookup(&no_key);
            break;
        }
        key_name((ctr*)0xa170c8, self->key_prev, out);
        break;
    case 2:
        if (self->key_next < 0) {
            *out = *strtab_lookup(&no_key);
            break;
        }
        key_name((ctr*)0xa170c8, self->key_next, out);
        break;
    case 3:
        if (self->key_select < 0) {
            *out = *strtab_lookup(&no_key);
            break;
        }
        key_name((ctr*)0xa170c8, self->key_select, out);
        break;
    case 4:
        key_name((ctr*)0xa170c8, 0x4e, out);                        // Numpad Enter
        break;
    case 5:
        key_name((ctr*)0xa170c8, 0x4a, out);                        // Numpad +
        break;
    case 6:
        key_name((ctr*)0xa170c8, 0x39, out);                        // Space
        break;
    }
}
