// game_keys.h: the game-action key bindings (object 0xa73730, g_game_keys): the DirectInput key code bound to each action
// (-1 = none; defaults filled by fn_4c2ea0 in static_init), read by the map camera and the in-mission input.
// The query methods are defined inline in script/script_interpreter.h (they call the origin-pair functions of ui/mission_ui.cpp).
#ifndef GAME_GAME_KEYS_H
#define GAME_GAME_KEYS_H
struct GameKeys {
    int bind[0x4a];                     // +0x00 the key code of each action (+0x04.. the camera keys the map code reads)
    char is_down(int key);              // 0x48ac50 (thiscall, ret 4)
    char clicked(int key);              // 0x48ac90
};
extern GameKeys g_game_keys;            // 0xa73730
#endif // GAME_GAME_KEYS_H
