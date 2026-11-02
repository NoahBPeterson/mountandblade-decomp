// patches/port_sketch/mb_platform.h: SKETCH, UNTESTED, NOT WIRED INTO ANY BUILD.
// Ticket docs/tickets/007-sdl3-platform-layer.md.
//
// The narrow seam between the reconstruction (src/) and the OS. Every Win32 / CRT-internal call the game makes outside
// D3D, DirectInput and FMOD goes through one of these. On the MSVC 7.1 match build the functions are __forceinline
// wrappers around the exact Win32 calls the original made (so the byte-exact rows are unaffected: the wrapper inlines to
// the same call); on the portable build they are implemented in port/platform_sdl.cpp.
//
// Measured need (src/, 2026-11-02): CreateDirectoryA 13, SHGetFolderPathA 12, FindFirstFileA 9, ShowCursor 9,
// timeGetTime 6, GetCursorPos 6, ClipCursor 5, Sleep 4, OpenClipboard 3, CRITICAL_SECTION 4, _beginthread 1,
// CreateEventA/WaitForSingleObject/SetEvent (loader thread) ~8, RegCreateKeyExA 2, socket 2 (version check),
// CreateProcessA 2 (binkplay), ShellExecuteA 2, MessageBoxA 26, SetWindowPos 19 (all in the SDK framework).
#ifndef MB_PLATFORM_H
#define MB_PLATFORM_H

#include <stddef.h>

namespace mbp {

// ---- time ------------------------------------------------------------------------------------------------------
double   seconds();                     // QueryPerformanceCounter / CLOCK_MONOTONIC_RAW; DXUtil_Timer sits on this
unsigned millis();                      // timeGetTime
void     sleep_ms(unsigned ms);         // Sleep
void     set_timer_resolution(unsigned ms);   // timeBeginPeriod (no-op elsewhere)

// ---- files and paths --------------------------------------------------------------------------------------------
// All game paths are written with '\\' and mixed case ("Data\\item_modifiers.txt", "data\\ground_specs.txt").
// open_game_file() normalises separators and, on case-sensitive file systems, resolves each component
// case-insensitively against a cached directory index (see ticket 007).
struct FILE_;                           // opaque; the game still uses stdio FILE* via fopen wrappers
void*  fopen_game(const char* path, const char* mode);        // returns FILE*
bool   find_first(const char* pattern, char* out_name, size_t cap, void** handle);
bool   find_next(void* handle, char* out_name, size_t cap);
void   find_close(void* handle);
bool   make_dir(const char* path);      // CreateDirectoryA semantics (false if it exists)
bool   user_dir(int kind, char* out, size_t cap);   // kind: 0 = Documents (CSIDL_PERSONAL), 1 = AppData (CSIDL_APPDATA)
bool   file_mtime(const char* path, long long* t);  // _stat

// ---- threads ----------------------------------------------------------------------------------------------------
struct Mutex;   struct Event;
Mutex* mutex_create();  void mutex_lock(Mutex*);  void mutex_unlock(Mutex*);
Event* event_create(bool auto_reset);  void event_set(Event*);  void event_wait(Event*);
void   thread_start(void (*fn)(void*), void* arg, unsigned stack_bytes);
unsigned cpu_count();                   // GetProcessAffinityMask popcount (decides g_mt_inited, cls_map.cpp)

// ---- window / cursor / clipboard (used by src/dxframework/d3dapp.cpp and the GUI text widgets) ---------------------
void   show_cursor(bool);  void clip_cursor(bool);  void get_cursor_pos(int* x, int* y);  void set_cursor_pos(int x, int y);
bool   clipboard_get(char* out, size_t cap);        void clipboard_set(const char* text);
void   message_box(const char* title, const char* text);   // fatal-error path of the framework (26 call sites)
void   open_url(const char* url);                   // ShellExecuteA("open", ...)

// ---- persistent small settings ("Software\\MountAndBladeKeys" in the registry) ----------------------------------------
bool   setting_get(const char* name, void* buf, size_t* size);
bool   setting_set(const char* name, const void* buf, size_t size);

}   // namespace mbp
#endif
