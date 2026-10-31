// patches/play/clip_cursor.cpp: keep the mouse inside the game window while it has focus (patch set "play").
//
// Changes behaviour on purpose. The faithful CD3DApplication only clips the cursor in fullscreen
// (m_bClipCursorWhenFullscreen && !m_bWindowed), so in a window the pointer leaves it. A static constructor starts a
// thread that every 100 ms clips the cursor to the window's client area while the game window is the foreground window
// and not minimised, and releases the clip as soon as it is not (Cmd+Tab, another app clicked). Only ClipCursor is
// called; the game's own input code is untouched.
#include <windows.h>

struct Core;
extern Core g_app_obj;      // 0x8ee3c48 (src/game/config/core.h; m_hWnd at +0xe8)

namespace {

DWORD WINAPI clip(void*)
{
    char* app = (char*)&g_app_obj;
    bool clipped = false;
    RECT last = {0, 0, 0, 0};
    for (;;) {
        Sleep(100);
        HWND h = *(HWND*)(app + 0xe8);
        bool want = h && IsWindow(h) && GetForegroundWindow() == h && !IsIconic(h);
        RECT rc;
        if (want) {
            GetClientRect(h, &rc);
            POINT tl = {rc.left, rc.top}, br = {rc.right, rc.bottom};
            ClientToScreen(h, &tl);
            ClientToScreen(h, &br);
            SetRect(&rc, tl.x, tl.y, br.x, br.y);
            want = rc.right > rc.left && rc.bottom > rc.top;
        }
        if (want && (!clipped || !EqualRect(&rc, &last))) {
            ClipCursor(&rc);    // re-applied when the window moves or resizes
            last = rc;
            clipped = true;
        } else if (!want && clipped) {
            ClipCursor(0);
            clipped = false;
        }
    }
    return 0;
}

struct Start {
    Start() { CloseHandle(CreateThread(0, 0, clip, 0, 0, 0)); }
} g_start;

}  // namespace
