// key_state.h: key-state words of the input table (0xa17000 current frame / 0xa17400 previous frame, one dword per scan code) that
// the editor, the mission screens and the video options read as plain words. Every word is ONE global here (declared once,
// defined in the module named on its line); no module declares its own copy. The table itself (camera.cpp indexes it as
// g_key_a[] / g_key_b[], the other words below are single elements of it) is the input pump's; its owner folds these words
// into the two arrays.
#ifndef GAME_KEY_STATE_H
#define GAME_KEY_STATE_H
extern int g_a17144;                    // 0xa17144 key state word, current frame (read by exchange_screens.cpp, rgl/mesh2/gui_edit.cpp; defined in map_editor/exchange_screens.cpp)
extern int g_a17420;                    // 0xa17420 delete key, pressed (defined in map_editor/map_editor.cpp)
extern int g_a1752c;                    // 0xa1752c (previous frame; defined in map_editor/map_editor.cpp)
extern int g_a17544;                    // 0xa17544 previous-frame counterpart of 0xa17144 (defined in map_editor/exchange_screens.cpp)
extern int g_a1754c;                    // 0xa1754c (previous frame; defined in map_editor/map_editor.cpp)
extern int g_a17588;                    // 0xa17588 key state word, previous frame (defined in map_editor/exchange_screens.cpp)
extern int g_a17820;                    // 0xa17820 delete key, previous frame (defined in map_editor/map_editor.cpp)
#endif // GAME_KEY_STATE_H
