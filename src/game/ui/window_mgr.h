// window_mgr.h: the window manager (g_windows 0xa73980, 0x8c bytes): 30 screen objects (head_rec*, one pointer per screen, the
// screen ctor stores itself into its slot; slot 0 is the dialog box, ResultWindow) followed by the open-screen id stack at +0x78
// (0xa739f8: a std::deque<int>, a ring of 4-int blocks; its top is the screen the game is in). ONE definition of the class and
// the declaration of the object for every module that reaches a screen through its slot. The constructor 0x4c8c20 and the
// object are defined in game/editor/editor_selection_and_translation.cpp (the compiler constructs it and registers the deque's out-of-line tail, _Tidy 0x4c4d10, for the atexit).
// The slots hold different screen classes; slot_as<T>(i) names the one a call site knows (a screen's own header will derive
// from head_rec).
#ifndef MB_GAME_WINDOW_MGR_H
#define MB_GAME_WINDOW_MGR_H
#include <deque>
#include "game/ui/screens/result_window.h"

struct WindowMgrW {
    head_rec* arr[30];                                           // +0x00 .. +0x74
    std::deque<int> dq;                                         // +0x78 the open-screen id stack (allocator word at +0x78; map at +0x7c)
    __declspec(noinline) WindowMgrW();                          // 0x4c8c20 (editor_selection_and_translation.cpp: the id stack's empty state, then the 30 slots cleared)
    ResultWindow*& box() { return reinterpret_cast<ResultWindow*&>(arr[0]); }   // slot 0: the dialog box (a slot, so the call sites load it when they use it)
    void open(int window);                                      // 0x494260 (ret 4)
    void set_state(int state);                                  // 0x495700 (ret 4)
    void clear_all();                                           // 0x489be0 (game/ui/widgets/widget_base.cpp: every screen's +4 cleared, +8 = 99)
    __declspec(noinline) void apply();                          // 0x4ca600 (game/editor/poly_mesh_edge_face_ops.cpp: closes the top screen: its reset_screen, then the next one's cursor flag)
};
extern WindowMgrW g_windows;                                    // 0xa73980

// the screen the game is in: the top of the id stack, read inline by GameApp::Update (0x50ec40) and VideoPage::update (0x4d63b0)
static __forceinline int current_screen_id() { return g_windows.dq.back(); }

template <class T> inline T*& slot_as(int i) { return reinterpret_cast<T*&>(g_windows.arr[i]); }

#endif // MB_GAME_WINDOW_MGR_H
