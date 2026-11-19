// port/platform/clipboard.cpp: OpenClipboard / GetClipboardData(CF_TEXT) / SetClipboardData over the SDL clipboard.
// Text crosses the boundary the way Windows does it: CF_TEXT is 8-bit (Windows-1252) with CRLF line ends; SDL's is UTF-8
// with LF.
#include "mbp_win.h"

using namespace mbp;

namespace {
bool g_open = false;
void* g_cache = nullptr;       // the block GetClipboardData last returned (the clipboard owns it)
}

extern "C" {
BOOL WINAPI OpenClipboard(HWND)
{
    if (!mbp_sdl_init()) return FALSE;
    g_open = true;
    return TRUE;
}
BOOL WINAPI CloseClipboard()
{
    if (!g_open) { set_last_error(1418); return FALSE; }       // ERROR_CLIPBOARD_NOT_OPEN
    g_open = false;
    return TRUE;
}
BOOL WINAPI EmptyClipboard()
{
    if (!g_open) { set_last_error(1418); return FALSE; }
    free(g_cache);
    g_cache = nullptr;
    SDL_SetClipboardText("");
    return TRUE;
}
BOOL WINAPI IsClipboardFormatAvailable(UINT fmt)
{
    return fmt == CF_TEXT && mbp_sdl_init() && SDL_HasClipboardText();
}
HANDLE WINAPI GetClipboardData(UINT fmt)
{
    if (!g_open || fmt != CF_TEXT) return nullptr;
    char* t = SDL_GetClipboardText();
    if (!t) return nullptr;
    std::string a = utf8_to_ansi(t), out;
    SDL_free(t);
    if (a.empty()) return nullptr;
    for (char c : a) { if (c == '\n') out += '\r'; out += c; }
    free(g_cache);
    g_cache = malloc(out.size() + 1);
    memcpy(g_cache, out.c_str(), out.size() + 1);
    return (HANDLE)g_cache;
}
HANDLE WINAPI SetClipboardData(UINT fmt, HANDLE mem)
{
    if (!g_open || fmt != CF_TEXT || !mem) return nullptr;
    std::string a = (const char*)GlobalLock(mem), lf;
    GlobalUnlock(mem);
    for (size_t i = 0; i < a.size(); ++i) { if (a[i] == '\r' && i + 1 < a.size() && a[i + 1] == '\n') continue; lf += a[i]; }
    SDL_SetClipboardText(ansi_to_utf8(lf.c_str()).c_str());
    return mem;                // ownership passes to the system on Windows; the data is copied here and the block stays valid
}
}
