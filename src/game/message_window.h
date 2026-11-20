// message_window.h: the fill-in of the message window C4c3050 (screen_records.h, slot 1 of g_windows) that the character sheet
// calls before g_windows.open(1). init is a member of a derived class with no data of its own rather than of C4c3050 itself:
// one more member declaration in screen_records.h swaps two g_windows loads in video_page.cpp's Recf0::frame_update 0x4d63b0
// (byte-exact only with screen_records.h as it is). slot_as<MessageWindowSetup>(1) is the C4c3050 object.
#ifndef MB_GAME_MESSAGE_WINDOW_H
#define MB_GAME_MESSAGE_WINDOW_H
#include "./screen_records.h"

struct MessageWindowSetup : C4c3050 {
    // the message, the button count and the six button texts; the pressed button (sel) is cleared
    void init(const tstring& msg, int n, const char* b0, const char* b1, const char* b2, const char* b3, const char* b4,
              const char* b5);   // 0x509410 (inventory_panel/char_window.cpp)
};
#endif // MB_GAME_MESSAGE_WINDOW_H
