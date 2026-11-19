// port/platform/mb_sdl.h: the seam between the Win32 layer (HWND) and the SDL3 objects behind it.
// Used by the gfx lane (swapchain / SDL_GPU claim of the game window) and the audio lane.
#pragma once
// Window coordinates are SDL points, equal to pixels unless MBP_HIGHDPI=1 (then SDL_GetWindowSizeInPixels is larger).
#include <SDL3/SDL.h>

struct HWND__;
#ifdef __cplusplus
extern "C" {
#endif
// The SDL window behind an HWND created by CreateWindowEx{A,W}; NULL for a dialog / child / invalid handle.
SDL_Window* mbp_sdl_window(struct HWND__* hwnd);
// The HWND whose SDL window is `w` (NULL if it is not one of ours).
struct HWND__* mbp_hwnd_from_sdl(SDL_Window* w);
// Initialise SDL's video + events subsystems (idempotent). Called by every window / input entry point.
bool mbp_sdl_init(void);
// Run the SDL event pump now (what PeekMessage does each call). The gfx lane need not call it.
void mbp_pump_events(void);
// For IDirect3DDevice9::ShowCursor(TRUE/FALSE): the device cursor overrides a Win32 SetCursor(NULL) hide (d3dapp.cpp's WM_SETCURSOR
// does SetCursor(NULL) then device->ShowCursor(TRUE)). The Win32 ShowCursor display counter is separate and still applies.
void mbp_set_device_cursor(bool visible);
#ifdef __cplusplus
}
#endif
