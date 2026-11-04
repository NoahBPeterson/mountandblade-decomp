// window_mgr.h: the window manager (g_windows 0xa73980, 0x8c bytes): 30 screen objects (ScreenObj*, one pointer per screen, the
// screen ctor stores itself into its slot; slot 0 is the dialog box, NamedRec) followed by the open-screen id stack at +0x78
// (0xa739f8: a std::deque<int>, a ring of 4-int blocks; its top is the screen the game is in). ONE definition of the class and
// the declaration of the object for every module that reaches a screen through its slot. The constructor 0x4c8c20 and the
// object are defined in rgl/mesh2/gui_edit.cpp (the compiler constructs it and registers the deque's out-of-line tail, _Tidy 0x4c4d10, for the atexit).
// The slots hold different screen classes; slot_as<T>(i) names the one a call site knows (a screen's own header will derive
// from ScreenObj).
#ifndef MB_GAME_WINDOW_MGR_H
#define MB_GAME_WINDOW_MGR_H
#include <deque>
#include "./screen_obj.h"
#include "./ui/named_rec.h"

struct WindowMgrW {
    ScreenObj* arr[30];                                         // +0x00 .. +0x74
    std::deque<int> dq;                                         // +0x78 the open-screen id stack (allocator word at +0x78; map at +0x7c)
    __declspec(noinline) WindowMgrW();                          // 0x4c8c20 (gui_edit.cpp: the id stack's empty state, then the 30 slots cleared)
    NamedRec*& box() { return reinterpret_cast<NamedRec*&>(arr[0]); }   // slot 0: the dialog box (a slot, so the call sites load it when they use it)
    void open(int window);                                      // 0x494260 (ret 4)
    void set_state(int state);                                  // 0x495700 (ret 4)
    void clear_all();                                           // 0x489be0 (ui/mission_ui.cpp: every screen's +4 cleared, +8 = 99)
    __declspec(noinline) void apply();                          // 0x4ca600 (rgl/mesh2/mesh_2.cpp: the device-state request queue; the screens are read as dev_item there)
};
extern WindowMgrW g_windows;                                    // 0xa73980

// the screen the game is in: the top of the id stack, read inline by GameApp::Update (0x50ec40) and VideoPage::update (0x4d63b0)
static __forceinline int current_screen_id() { return g_windows.dq.back(); }

template <class T> inline T*& slot_as(int i) { return reinterpret_cast<T*&>(g_windows.arr[i]); }

#endif // MB_GAME_WINDOW_MGR_H
