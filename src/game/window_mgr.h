// window_mgr.h: the window manager (g_windows 0xa73980): 30 screen objects (ScreenObj*, one pointer per screen, the screen
// ctor stores itself into its slot) followed by the open-screen deque at +0x78 (0xa739f8). ONE definition of the class and the
// declaration of the object for every module that reaches a screen through its slot (the callers used to declare each slot
// as a separate global: g_rec_88 0xa73988, g_menu_w1 0xa7398c, g_wAC 0xa739ac ... or index the object as a char array).
// The slots hold different screen classes; slot_as<T>(i) names the one a call site knows (the ScreenObj base is still the
// vtable view of cls_map: a screen's own header will derive from it).
#ifndef MB_GAME_WINDOW_MGR_H
#define MB_GAME_WINDOW_MGR_H
#include <deque>
#include "./screen_obj.h"

struct WindowMgrW {
    ScreenObj* arr[30];                                         // +0x00 .. +0x74
    std::deque<int> dq;                                         // +0x78 (allocator word at +0x78; map at +0x7c)
    void open(int window);                                      // 0x494260 (ret 4)
    void set_state(int state);                                  // 0x495700 (ret 4)
};
extern WindowMgrW g_windows;                                    // 0xa73980

template <class T> inline T*& slot_as(int i) { return reinterpret_cast<T*&>(g_windows.arr[i]); }

#endif // MB_GAME_WINDOW_MGR_H
