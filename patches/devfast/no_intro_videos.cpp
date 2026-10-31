// patches/devfast/no_intro_videos.cpp: skip the two binkplay intro videos (patch set "devfast").
//
// CHANGES BEHAVIOUR: WinMain (cls_map.cpp) starts binkplay.exe for paradox.bik and taleworlds_intro.bik and waits up
// to ~30 s for them, which every test run under Wine pays. This makes the binkplay launches fail, as if binkplay.exe
// were missing (which the original handles: g_video_played stays 0); any other process starts normally.
#include "iat_hook.h"

namespace {

typedef BOOL (WINAPI* CreateProcessA_t)(LPCSTR, LPSTR, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, BOOL, DWORD,
                                        LPVOID, LPCSTR, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
CreateProcessA_t g_real;

BOOL WINAPI create_process(LPCSTR app, LPSTR cmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL inherit,
                           DWORD flags, LPVOID env, LPCSTR dir, LPSTARTUPINFOA si, LPPROCESS_INFORMATION pi)
{
    if (app && _stricmp(app, "binkplay.exe") == 0) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    return g_real(app, cmd, pa, ta, inherit, flags, env, dir, si, pi);
}

struct Start {
    Start() { g_real = (CreateProcessA_t)iat_hook("kernel32.dll", "CreateProcessA", (void*)create_process); }
} g_start;

}   // namespace
