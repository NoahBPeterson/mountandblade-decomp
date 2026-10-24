// key_names.h: the keyboard state object at 0xa170c8 as the game code calls into it. One definition for
// options_panel/keys_page.cpp (the key table page) and inventory_panel/mission_ctx.cpp (the map camera): both call the
// same function, 0x487900 (thiscall, ret 4: is key/action n bound?), on the same global.
#ifndef GAME_KEY_NAMES_H
#define GAME_KEY_NAMES_H
struct KeyNames {
    char is_bound(int index);                                               // 0x487900 (ret 4)  // old name of Slot::is_ready, removed after the names merge
};
extern KeyNames g_key_names;                                                // 0xa170c8  // old name of g_keyboard, removed after the names merge
struct Keyboard;   // defined in script/script_interpreter.h until the names merge
extern Keyboard g_keyboard; // 0xa170c8
#endif
