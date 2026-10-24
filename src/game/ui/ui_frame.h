// ui_frame.h: UiFrame, the container widget that owns a vector of objects (vtable 0x5924a0, 0x188 bytes; destroying destructor
// 0x5021f0, release 0x48ef40). ONE definition for src/game/party (party_window.h: the container the party window owns),
// src/game/cls_map (the menu block of the main / escape menus) and src/game/mission_ui2/mission_4.h (release 0x48ef40). The
// vector lives at +0x178: allocator word, first +0x17c, last +0x180, end +0x184 (0x4dd920 clears them; vec_clear 0x41f4b0 frees it).
#ifndef GAME_UI_UI_FRAME_H
#define GAME_UI_UI_FRAME_H
#include "./mission_ui.h"

struct UiFrame : MissionUi {
    int f178;                               // +0x178 (the vector's allocator word)
    int f17c, f180, f184;                   // +0x17c (first, last, end: 0x4dd920 clears them)
    void release();                         // 0x48ef40
    void* destroy(unsigned flags);          // 0x5021f0 (ret 4)
};

#endif // GAME_UI_UI_FRAME_H
