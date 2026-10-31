// patches/play/hide_cursor.cpp: in a window, show the game's cursor only on screens that want one (patch set "play").
//
// Changes behaviour on purpose. The faithful window procedure (GameApp::MsgProc 0x50f130) answers every WM_SETCURSOR
// in windowed mode with SetCursor(0) + device->ShowCursor(TRUE), so the cursor shows in battle and on the map too;
// only a screen change (WindowMgrW::open 0x494260) applies the screen's own choice, kept in g_gui+0x560, and the next
// mouse move undoes it. This subclasses the game window once it exists and answers WM_SETCURSOR over the client area
// with SetCursor(0) + ShowCursor(g_gui+0x560) instead. Everything else goes to the game's procedure unchanged.
#include <windows.h>

struct Core;
struct gui_manager;
extern Core g_app_obj;          // 0x8ee3c48 (m_hWnd at +0xe8, m_bWindowed at +0xa4)
extern gui_manager g_gui;       // 0x5d7080 (+0x560: the current screen wants the cursor)
extern void* g_a17f28;          // 0xa17f28: the IDirect3DDevice9

namespace {

WNDPROC g_prev = 0;

LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    char* app = (char*)&g_app_obj;
    if (msg == WM_SETCURSOR && LOWORD(lp) == HTCLIENT && g_a17f28 && *(app + 0xa4)) {
        SetCursor(0);
        typedef BOOL (__stdcall *ShowFn)(void*, BOOL);
        ShowFn show = (ShowFn)(*(void***)g_a17f28)[12];         // IDirect3DDevice9::ShowCursor
        show(g_a17f28, *((unsigned char*)&g_gui + 0x560) != 0);
        return 1;
    }
    return CallWindowProcW(g_prev, h, msg, wp, lp);
}

DWORD WINAPI hook(void*)
{
    char* app = (char*)&g_app_obj;
    for (;;) {
        Sleep(100);
        HWND h = *(HWND*)(app + 0xe8);
        if (h && IsWindow(h)) {
            g_prev = (WNDPROC)GetWindowLongPtrW(h, GWLP_WNDPROC);
            SetWindowLongPtrW(h, GWLP_WNDPROC, (LONG_PTR)proc);
            return 0;
        }
    }
}

struct Start {
    Start() { CloseHandle(CreateThread(0, 0, hook, 0, 0, 0)); }
} g_start;

}  // namespace
