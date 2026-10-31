// patches/fps/fps_title.cpp: show the frame rate in the game window's title bar (patch set "fps").
//
// Changes behaviour on purpose (the title text) and nothing else. The faithful CD3DApplication::UpdateStats (0x402580)
// already measures the frame rate once a second into m_fFPS (+0x27c of the application object g_app_obj, 0x8ee3c48;
// m_hWnd at +0xe8). A static constructor starts a thread that every 500 ms reads those two fields and sets the title
// to "<original title> - 59.9 fps (16.7 ms)". Reads only; WM_SETTEXT goes through SendMessageTimeout so a busy main
// thread (loading) delays the update instead of blocking this thread forever.
#include <windows.h>
#include <stdio.h>

struct Core;
extern Core g_app_obj;      // 0x8ee3c48 (src/game/config/core.h)

namespace {

DWORD WINAPI watch(void*)
{
    char* app = (char*)&g_app_obj;
    wchar_t base[256] = L"";
    HWND seen = 0;
    for (;;) {
        Sleep(500);
        HWND h = *(HWND*)(app + 0xe8);
        if (!h || !IsWindow(h))
            continue;
        if (h != seen) {        // first sight of the window (or a new one): remember its own title
            DWORD_PTR n = 0;
            SendMessageTimeoutW(h, WM_GETTEXT, 256, (LPARAM)base, SMTO_ABORTIFHUNG, 500, &n);
            base[255] = 0;
            seen = h;
        }
        float fps = *(float*)(app + 0x27c);
        wchar_t title[320];
        if (fps > 0.0f)
            _snwprintf(title, 320, L"%s - %.1f fps (%.1f ms)", base, fps, 1000.0f / fps);
        else
            _snwprintf(title, 320, L"%s - -- fps", base);
        title[319] = 0;
        DWORD_PTR r = 0;
        SendMessageTimeoutW(h, WM_SETTEXT, 0, (LPARAM)title, SMTO_ABORTIFHUNG, 500, &r);
    }
    return 0;
}

struct Start {
    Start() { CloseHandle(CreateThread(0, 0, watch, 0, 0, 0)); }
} g_start;

}  // namespace
