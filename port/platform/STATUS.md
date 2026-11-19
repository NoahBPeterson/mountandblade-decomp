# port/platform status (lane: platform, ticket 007)

Library `mb_platform` (static; SDL3 + SDL3_ttf + POSIX). Tests: `port/platform/tests` (`ctest` in the build dir, or run
`mbp_test_window`, `mbp_test_kernel <game dir>`, `mbp_test_net`). Compiles for macos-arm64 (run here) and x86_64-linux-gnu (zig, syntax+codegen only).

## Done (all exercised by the tests)
- USER32: RegisterClass/CreateWindowEx{A,W} -> SDL_Window; PeekMessage/GetMessage/Translate/Dispatch over SDL events (WM_SIZE/MOVE/ACTIVATE*/SETFOCUS/
  KILLFOCUS/KEYDOWN/KEYUP/SYSKEY*/CHAR/MOUSEMOVE/L,R,M,X buttons/MOUSEWHEEL/PAINT/CLOSE/DESTROY/TIMER/SETCURSOR), PostMessage (thread-safe, wakes GetMessage),
  SendMessage, DefWindowProc, timers, SetWindowPos/MoveWindow/Show/Update/Enable/Focus/ClientToScreen, style changes (bordered/popup/resizable), metrics,
  ShowCursor counter / SetCursor / ClipCursor (mouse grab + rect) / GetCursorPos / SetCursorPos (warp; echo filtered from DirectInput deltas), GetKeyState,
  MessageBoxA/W (SDL message box; MBP_NO_DIALOGS=1 answers the default button), clipboard (CF_TEXT, CRLF <-> LF, 1252 <-> UTF-8), menus (virtual), accelerators (empty).
- Dialogs: virtual (controls appear on first GetDlgItem): text, check, combo/list items+data+selection, trackbar, tab. DialogBoxParamA runs WM_INITDIALOG,
  drains posted messages, then answers: the launcher (template 0xaa) presses Play (0x410); other dialogs are cancelled. CreateDialogParamA modeless.
- KERNEL: time (QPC ns, timeGetTime, Sleep), recursive critical sections, events (auto/manual), threads (CreateThread; _beginthread is in port/runtime),
  WaitForSingle/MultipleObjects (events, threads, processes), CreateProcess (Windows .exe such as binkplay: "starts" and exits 0 at once; native programs really run),
  Global/VirtualAlloc, code pages (1252/UTF-8), lstr*, wsprintf, MulDiv, system info, LoadLibrary (system DLLs the game loads are "provided"), GetProcAddress (dlsym).
- Files: mbp_resolve_path (backslash, drive prefix, any case, cached dir index, rebuilt on miss when the dir changed), mb_fopen (CRLF->LF in "r"/"rt"),
  mb_remove, mb_rename (no replace, as the CRT), mb_rmdir, mb_access, mb_mkdir, _stat/_findfirst (port/runtime/crt_extra.cpp), FindFirstFileA family (sorted,
  "."/".." first, dotfiles hidden), CreateDirectory/RemoveDirectory/DeleteFile/CopyFile/MoveFile/GetFileAttributes/cwd/temp, SHGetFolderPathA
  (macOS ~/Library/Application Support, Linux $XDG_DATA_HOME; MB_USER_DIR overrides), ShellExecuteA (URLs and files via SDL_OpenURL).
- Registry: text store <user data>/Mount&Blade/registry.ini (sz/dword/bin), case-insensitive keys, ERROR_MORE_DATA, disposition.
- GDI: memory DCs, DIB sections (24/32 bpp), bitmaps from .bmp, fonts (SDL3_ttf; Arial/Times/Courier mapped to system or Liberation/DejaVu; MB_FONT=file.ttf),
  ExtTextOut/TextOut (AA blend, OPAQUE cell), GetTextExtentPoint32, BitBlt/StretchBlt between memory DCs. Window DCs have no pixel store (draw is accepted, invisible).
- Winsock: net.cpp, `mbws_*` (WSAStartup/socket/connect/send/recv/closesocket/gethostbyname/htons/inet_addr) over POSIX; port/include/winsock2.h maps the game's names.
- COM/OLE/SkinMagic: CoInitialize ok, CoCreateInstance -> REGDB_E_CLASSNOTREG (DxDiag absent), Variant/BSTR, SkinMagic no-ops (dialog is virtual).
- DirectInput8: DirectInput8Create, IDirectInput8A::CreateDevice, keyboard (256-byte DIK state, buffered key events) and mouse (DIMOUSESTATE/2 relative deltas,
  buffered X/Y/Z/button records with coalescing, DIPROP_BUFFERSIZE, 0x14-byte records), Acquire/Unacquire/NOTACQUIRED, event notification, data formats.
- Seam: port/platform/mb_sdl.h (mbp_sdl_window, mbp_hwnd_from_sdl, mbp_sdl_init, mbp_pump_events, mbp_set_device_cursor), mb_paths.h (mbp_resolve_path for the gfx
  lane's D3DX file loaders: call it on every path).

## Integration notes
- port/stubs/platform_stubs.cpp duplicates these symbols: drop it from `mb_port_src`; link `mb_platform` (+ `mb_runtime`). Not mine to remove.
- GDI window DCs: GetDC(hwnd)/BeginPaint draw nowhere (the launcher bitmap is not shown; launcher is headless).
- Window client size is the SDL window size in points; non-client metrics are 0.
- Keyboard focus state is from the event pump; GetDeviceState pumps SDL itself.

## Remaining
- Real-hardware feel not tested: mouse-look (warp-based; SDL relative mouse mode is not used), alt-tab/focus semantics, high-DPI (MBP_HIGHDPI=1 untested).
- A visible first-run launcher / module chooser (dialog is answered automatically; set the module with the registry value Software\MountAndBladeKeys last_module).
- Linux: compiled with zig for x86_64-linux-gnu but not linked or run.
- DirectInput enumeration (EnumDevices), device capabilities and force feedback return E_NOTIMPL (src/ never calls them).
- Alt+Enter / fullscreen toggling is the game's (SetWindowPos + gfx); no SDL fullscreen mode is used by this layer.
