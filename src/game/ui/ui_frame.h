// ui_frame.h: the container widget under its old name. UiFrame (vtable 0x5924a0, 0x188 bytes; the party window's blocks, the main /
// escape menu's menu block) is ui/container.h's ContainerBase: the vector at +0x178 (allocator word, first +0x17c, last +0x180, end
// +0x184) is its `kids`, and release 0x48ef40 is its update_children.
#ifndef GAME_UI_UI_FRAME_H
#define GAME_UI_UI_FRAME_H
#include "./container.h"

typedef ContainerBase UiFrame;

#endif // GAME_UI_UI_FRAME_H
