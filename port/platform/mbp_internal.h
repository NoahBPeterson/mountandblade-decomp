// port/platform/mbp_internal.h: shared declarations of the platform library's translation units.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <SDL3/SDL.h>
#include <windows.h>
#undef max
#undef min
#undef interface
#include <shlobj.h>
#ifndef WM_NCCREATE
#define WM_NCCREATE 0x0081
#endif
#ifndef LB_SETCURSEL
#define LB_SETCURSEL 0x0186
#endif
#include "mb_sdl.h"
#include "mb_paths.h"

namespace mbp {

// ---- kernel objects behind HANDLE (events, threads, processes, find handles) -----------------------------------
enum ObjType : uint32_t { OBJ_EVENT = 0x4d424576, OBJ_THREAD = 0x4d425468, OBJ_PROCESS = 0x4d425072, OBJ_FIND = 0x4d424664 };
struct KObj { uint32_t magic; };
void kobj_add(void* p);              // every HANDLE object is registered so a bad handle is rejected, not dereferenced
void kobj_remove(void* p);
bool kobj_live(void* p);
template <class T> inline T* as(HANDLE h, ObjType t)
{
    if (!h || h == INVALID_HANDLE_VALUE || !kobj_live(h)) return nullptr;
    KObj* k = (KObj*)h;
    return k->magic == (uint32_t)t ? (T*)h : nullptr;
}

// ---- text helpers -------------------------------------------------------------------------------------------------
std::string wide_to_utf8(const wchar_t* w);                       // UTF-32 (or UTF-16 if wchar_t is 2 bytes) -> UTF-8
std::string ansi_to_utf8(const char* s);                          // the game's 8-bit codepage (Windows-1252) -> UTF-8
std::string utf8_to_ansi(const char* s);                          // UTF-8 -> Windows-1252 (unmappable -> '?')
unsigned ansi_char_to_unicode(unsigned char c);

// ---- windows ------------------------------------------------------------------------------------------------------
HWND window_from_sdl_id(SDL_WindowID id);
void set_last_error(DWORD e);
void on_sdl_event_for_input(const SDL_Event& e);                  // dinput.cpp: feeds its buffers
int keymap_vk_for_scancode(SDL_Scancode sc, bool* extended);
int keymap_dik_for_scancode(SDL_Scancode sc);
SDL_Scancode keymap_scancode_for_vk(int vk);
SDL_Scancode keymap_scancode_for_dik(int dik);
void dinput_poll_keyboard(uint8_t state[256]);
bool key_is_down(SDL_Scancode sc);           // keyboard state as the event pump has seen it
unsigned mouse_button_mask();                // bit n = SDL button n+1 is down (event-tracked)
// dialogs (dialog.cpp)
LRESULT control_message(HWND h, UINT msg, WPARAM wp, LPARAM lp, bool* handled);
bool is_virtual_window(HWND h);

}   // namespace mbp
