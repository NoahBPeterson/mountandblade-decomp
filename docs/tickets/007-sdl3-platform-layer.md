# 007 SDL3 platform layer: window, input, text, filesystem, time, threads

Area: port | Effort: M | Impact: high | Depends on: 004

## Summary

Implement `mbp::` (sketch: `patches/port_sketch/mb_platform.h`) on SDL3 and replace the Win32 call sites. SDL3 covers every OS service the
game needs except the launcher dialog / registry / DxDiag (handled below).

## Evidence

- Main loop: `CD3DApplication::Run` 0x402ee0 (src/dxframework/d3dapp.cpp:1287): `PeekMessageW` if active, `GetMessageW` if not, then
  `Render3DEnvironment` 0x402430 -> `FrameMove` + `Render` + `Present`. `MsgProc` has 14 cases (`WM_CLOSE, COMMAND, ENTERMENULOOP, ENTERSIZEMOVE, EXITMENULOOP,
  EXITSIZEMOVE, GETMINMAXINFO, MOUSEMOVE, NCHITTEST, PAINT, POWERBROADCAST, SETCURSOR, SIZE, SYSCOMMAND`); text input arrives via `WM_CHAR` in game_app.cpp.
- Input: DirectInput8 keyboard (`c_dfDIKeyboard`) + mouse `c_dfDIMouse2` (20 bytes, 11 objects; config.cpp:867-891). The keyboard state is a 256-entry array indexed by
  **DIK scancodes** (`g_keyboard + 0xc)[29]` = DIK_LCONTROL 0x1d, `[157]` = DIK_RCONTROL, `[67]` = F9...). Mapping SDL scancodes -> DIK is a 100-entry table.
- Other Win32 sites: ShowCursor 9, GetCursorPos 6, ClipCursor 5, SetCursorPos 1, OpenClipboard 3, SetWindowPos 19 (framework), MessageBoxA 26 (framework error paths),
  CreateDirectoryA 13, SHGetFolderPathA 12, FindFirstFileA 9, timeGetTime 6, QueryPerformanceCounter (dxutil.cpp:64), Sleep 4, `GetProcessAffinityMask` (cls_map.cpp, decides threading).
- Windows-only extras: `Software\MountAndBladeKeys` registry value (options_dlg.cpp:228, world_app.cpp:531), `WSAStartup/socket/connect` version check to taleworlds.com
  (options_dlg.cpp:112), `ShellExecuteA` x2, `CoCreateInstance(CLSID_DxDiagProvider)` (options_dlg.cpp:404), `binkplay.exe` x2 (cls_map.cpp:154,174), `SkinMagic_*` skinning of the launcher dialog.
- Paths: 92 backslash literals in 25 files ("Data\\item_modifiers.txt", "data\\ground_specs.txt" - mixed case). Windows and default macOS are case-insensitive; Linux is not.

## Detailed plan

1. `port/platform_sdl.cpp`: window (`SDL_CreateWindow` + `SDL_WINDOW_RESIZABLE|HIGH_PIXEL_DENSITY`), event pump replacing `Run`/`MsgProc`, relative mouse mode for mouse look
   (`SDL_SetWindowRelativeMouseMode`), text input (`SDL_StartTextInput`, `SDL_EVENT_TEXT_INPUT`, game is 8-bit codepage -> convert UTF-8 to the module's language encoding table), clipboard, cursor visibility/clip (`SDL_SetWindowMouseGrab`).
2. Replace DirectInput: poll `SDL_GetKeyboardState` into the `g_keyboard` DIK array each frame via the table; mouse delta from `SDL_GetRelativeMouseState`; wheel from events.
3. Timer: `mbp::seconds()` on `SDL_GetPerformanceCounter`; `DXUtil_Timer` stays as is above it.
4. Threads: `SDL_CreateThread`, `SDL_Mutex`, `SDL_Condition` (the event is auto-reset: implement as mutex+condvar+flag). Moves into the job system (012).
5. Filesystem: `mbp::open` normalises `\` to `/` and resolves each component case-insensitively against a cached directory listing when the FS is case-sensitive (module dirs have mixed case on disk:
   "Modules/Native/Resource", "SceneObj"). User dirs from `SDL_GetPrefPath("Taleworlds","Mount&Blade")` for `rgl_config.txt`, saves, screenshots.
6. Launcher dialog (resource 0xaa, SkinMagic): replace by (a) first-run settings screen in the engine's own UI, plus (b) `rgl_config.txt` as today. `RegCreateKeyEx` -> a small key file in the pref dir.
   Version-check socket and DxDiag: drop (DxDiag only fills a text box). Video: ship no Bink; play `.bik` only if an optional `ffmpeg`-based plugin exists, else skip.
7. `SDL_GetCPUCount()` replaces `GetProcessAffinityMask`.

## Code sketch

```cpp
// port/platform_sdl.cpp (excerpt)
static const unsigned char sdl_to_dik[SDL_SCANCODE_COUNT] = { [SDL_SCANCODE_ESCAPE]=0x01, [SDL_SCANCODE_1]=0x02, /* ... */ [SDL_SCANCODE_LCTRL]=0x1d, [SDL_SCANCODE_F9]=0x43 };
void mbp_poll_keyboard(int* dik_state /*256*/) {
    int n; const bool* k = SDL_GetKeyboardState(&n);
    memset(dik_state, 0, 256 * sizeof(int));
    for (int sc = 0; sc < n; ++sc) if (k[sc] && sdl_to_dik[sc]) dik_state[sdl_to_dik[sc]] = 0x80;
}
```

## Risks

- Alt-tab / focus semantics (`m_bActive` gates `PeekMessage` vs blocking `GetMessage` - keep blocking waits in the inactive state to avoid burning a core).
- Mouse look feel (DirectInput relative vs SDL relative): tune.
- Non-Latin text: the game predates Unicode; keep 8-bit strings, convert at the SDL boundary only.

## Acceptance

Window opens, menu navigable by mouse/keyboard, text fields accept typing and paste, window resize and fullscreen toggle work, config and saves land in the pref dir.

## Effort

M (4-6 wk).
