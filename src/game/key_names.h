// key_names.h: the keyboard state object at 0xa170c8 as the game code calls into it. One definition for
// options_panel/keys_page.cpp (the key table page) and inventory_panel/mission_ctx.cpp (the map camera): both call the
// same function, 0x487900 (thiscall, ret 4: is key/action n bound?), on the same global.
#ifndef GAME_KEY_NAMES_H
#define GAME_KEY_NAMES_H
struct KeyNames {
    char is_bound(int index);                                               // 0x487900 (ret 4)  // old name of Slot::is_ready, removed after the names merge
};
extern KeyNames g_key_names;                                                // 0xa170c8  // old name of g_keyboard, removed after the names merge
// The keyboard / input state object (0xa170c8): the per-key state tables the game code reads by DirectInput key code
// (g_keyboard.down[0x1d] = left Ctrl, [0x9d] = right Ctrl, [1] = Esc ...). Members are the ones the code reaches.
struct Keyboard {
    char pad0[0xc];
    int down[256];                      // +0x00c key is held (key_is_down 0x49fd8c)
    int prev[256];                      // +0x40c key was held last frame (a press is down && !prev)
    float axis[3];                      // +0x80c the stick / mouse look axes (+0x80c x, +0x810, +0x814 y)
};
extern Keyboard g_keyboard; // 0xa170c8
#endif
