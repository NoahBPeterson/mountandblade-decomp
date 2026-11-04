// game_keys.h: GameKeys, the game-action key bindings (g_game_keys 0xa73730, 0x250 bytes): the 0x25 actions' {key, button}
// DirectInput codes (-1 = none) and the copy of them the options page restores. ONE definition for the in-mission input, the map
// editor, the script interpreter, the options page and the loader. Defined in game/options_panel/keys_page.cpp.
#ifndef GAME_GAME_KEYS_H
#define GAME_GAME_KEYS_H
struct tstring;
struct GameKeys {
    int entry[0x25][2];                 // +0x000 the key code and the button code bound to each action
    int original[0x25][2];              // +0x128 the bindings as the options page found them (backup 0x4e2cb0, restore 0x4d6fd0)
    GameKeys();                         // every code -1, then load() (the original's static initializer 0x547e90)
    ~GameKeys() {}                      // (empty: the original registers an empty atexit function, 0x548210)
    void load();                        // 0x4c2ea0 (rgl/mesh2/rec_tables.cpp): controls.dat, or set_defaults() when it is missing or wrong
    void set_defaults();                // 0x4be7c0 (game/triggers/trigger_vec.cpp)
    bool is_down(int action);           // 0x48ac50 (game/ui/mission_ui.cpp)
    bool clicked(int action);           // 0x48ac90
    void forget(int index, int button); // 0x4d6f90 (game/options_panel/keys_page.cpp)
    __declspec(noinline) void restore();// 0x4d6fd0 (kept as a call by 0x4d6e50)
    void save();                        // 0x4d7000 (writes controls.dat)
    void key_name(int which, tstring* out);                             // 0x4d7370 (game/options_panel/options_panel.cpp)
    void key_binding_name(int slot, tstring* out, int* translated);     // 0x4d7d10
    void backup();                      // 0x4e2cb0 (game/cls_map/cls_map.cpp): entry -> original
};
extern GameKeys g_game_keys;            // 0xa73730 (defined in game/options_panel/keys_page.cpp)
#endif // GAME_GAME_KEYS_H
