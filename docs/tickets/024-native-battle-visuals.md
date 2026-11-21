# 024: Native Quick Battle: visual and input bugs from the first play-through

Reported 2026-11-20 after a Quick Battle on the native macOS build. Screenshots (user's session):
`images/43.png`, `images/44.png` (under the Claude session's tmp images dir). Lane G2.

## 1. Trees float above the ground
Symptom: whole trees and bare trunks hang in the sky (43.png, top).
Cause: (pending)
Fix commit: (pending)

## 2. Shields render magenta / purple
Symptom: every shield is a flat magenta or purple (43.png, 44.png): the missing-texture colour.
Cause (measured): the shield materials are tableaus (scene_step renders the shield and banner meshes into a 256 x 256 render target,
a read-back of it shows the right atlas). The target is created D3DUSAGE_AUTOGENMIPMAP; Direct3D 9 refreshes its lower mip
levels after every draw to level 0, the port only did so on an explicit GenerateMipSubLevels / UnlockRect. The shield is sampled
with mip filtering from a few metres away, so it read levels nobody had written. An unlit test (ps_main returning the texture)
still showed it magenta: the texture, not the lighting.
Fix: port/gfx/device_draw.cpp: when a render pass into an auto-mipmap target level 0 ends, SDL_GenerateMipmapsForGPUTexture.
Fix commit: (this commit). Seen: Skirmish 1 / 2 / 3 / 4 at 300 mission frames: no magenta shield left.

## 3. Mouse look limited to about 180 degrees
Symptom: the player cannot turn all the way around; the limit is about 270 degrees (corrected from 180). Still there on main at 87a7dc44.
Cause (probable, not reproduced with synthetic events): the game hides the pointer (CD3DApplication, ShowCursor FALSE) and reads
turning from DirectInput's relative mouse. The port gave it SDL's motion events in absolute mode, where the pointer is clamped to
the window / screen (ClipCursor -> SDL_SetWindowMouseRect + grab), so the turning stops when the pointer reaches the edge: about
half a window width of mouse travel in each direction.
Fix: port/platform/window.cpp: while the pointer is hidden SDL's relative mouse mode is on (raw motion without end) and the pointer
GetCursorPos reports is virtual (clamped to the clip rectangle, as Windows clips it). port/platform/tests/test_window.cpp checks it.
Seen: synthetic sweeps of 6000 points turn the view 1.4 revolutions (yaw -8.9 rad), menu clicks still reach the buttons.
Fix commit: (this commit); needs the user's confirmation with a real mouse.
Follow-up (menus had no mouse): "hidden" was judged from SetCursor(NULL) / ShowCursor only, but the game always sends
SetCursor(NULL) (GameApp::MsgProc, fullscreen) and shows its pointer through the D3D device's ShowCursor (WindowMgrW::open,
mission_5, trigger_vec: g_gui.show_cursor); port/gfx's Device::ShowCursor never told the platform layer
(mbp_set_device_cursor had no caller), so every menu stayed in relative mode with a virtual pointer. Now Device::ShowCursor calls
mbp_set_device_cursor: the game's own signal picks the mode (device cursor shown or a visible system cursor: absolute, real pointer,
clicks at the pointer; hidden: relative, virtual pointer). test_window.cpp checks the switch.

## 4. "Speed bonus: +319%" / "Delivered 171 damage to horse" on a lance hit
Symptom: implausible numbers (44.png, bottom left). Compare with the original formula before calling it a bug.
Finding: no defect found. Agent::hit_damage (0x453710) is a byte-matched row (in agent_logic.manifest.txt, not in its
nonmatching list), so the formula is the binary's: bonus = (t^p - 1) * 100 with t = (c * w - closing speed along the weapon)
/ w clamped to 0.3 .. 3 and p = melee_damage_speed_power (2.0 in the module's rgl_log). +319% is t = 2.05 (2.05^2 = 4.19): a
lance thrust at a gallop against a horse, inside the clamp. 171 is the lance's damage times that factor. The inputs are the two
agents' velocities (rot_xy of vel_xy, or the horse's speed along its forward), real values here. Not reproduced as wrong; the
combat fixes of 87a7dc44 / ef666c7c (hits that did not land) are the likely reason the numbers looked off.
Fix commit: none.

## 5. Black band (~45 px) at the top of the window
Symptom: 43.png and 44.png: the top ~45 px are black. Suspect viewport versus the macOS title bar / notch safe area.
Finding: the game asks for fullscreen (start_windowed = 0) and SDL3 on macOS used native Spaces fullscreen, where AppKit lays the
window out below the notch / menu-bar band: 1728 x 1084 points at y = 33 on a 1728 x 1117 screen, while the back buffer was the
desktop mode 1728 x 1117 (so the picture was also squeezed 3% vertically). The user decided the game uses the notch area.
Fix: port/platform/window.cpp sets SDL_VIDEO_MAC_FULLSCREEN_SPACES=0 and SDL_VIDEO_MAC_FULLSCREEN_MENU_VISIBILITY=0 (SDL's
borderless desktop fullscreen over the whole display, menu bar hidden; no effect off macOS). The hints must be set before SDL's
video subsystem starts, and the gfx lane's SDL_InitSubSystem calls (enum_modes ran before the window was created) came first and
made the hint a no-op: port/gfx/d3d9_main.cpp now starts SDL through mbp_sdl_init() (the platform layer's init, which sets the
hints). The window rect that run_native.py prints is now 0 0 1728 1117; the window size equals the back buffer, so the render
resolution and the mouse mapping need nothing else (Quick Battle shot: no band). Not verified: run_native.py's click at Credits / Quick Battle changes no state, with this change and also with SDL_VIDEO_MAC_FULLSCREEN_SPACES=1 (the old layout), so it predates this fix (suspect: the hidden-pointer relative mouse of e00f8f95 in menus). Windowed mode
(start_windowed = 1) is untouched: the hints only matter when SDL_SetWindowFullscreen is called. The camera housing hides the
middle of the top ~33 px: the game's top edge content sits under the notch, as decided.
Fix commit: (this commit)

## 6. The town's buildings and fences are missing in the battle scene
Symptom: (image 45.png, overhead view) the town's buildings and fences are not there; later (46.png, 47.png): they are there, under
the terrain (scaffold and fences half sunk into a hill).
Cause: the same as bug 1: the terrain was not the one the props were authored on (libc rand instead of the VC7.1 CRT's).
Fix: port/runtime/crt_extra.cpp defines the CRT's `rand` / `srand` (per-thread state, start 1), replacing libc's in the executable.
Fix commit: (this commit)

## 7. Large magenta patches lying flat on the terrain
Symptom: 45.png: flat magenta patches on the ground (the river water, material "river", water_shader): another missing-texture case.
Cause: the same as bug 2: the water samples the reflection render target (256 x 256, rendered by the sky pass), also auto-mipmap.
Fix commit: the one of bug 2. Seen: Siege Defense (scenario 3) showed the patch on its right before the fix; not after.

## 8. The pointer stops above the bottom buttons (Done on the character sheet)
Symptom: (49.png) in fullscreen the pointer cannot reach the bottom row of buttons (Reset Changes / Done).
Cause: not the mapping (GetCursorPos, GetClientRect, the game's view 0 0 / screen 1728 x 1117 all agree with the window). src/dxframework/d3dapp.cpp
calls `ClipCursor(GetWindowRect)` right after CreateDevice; SDL's fullscreen switch is asynchronous, so the window was still the windowed
1728 x 971 and the clip (SDL mouse rect + grab, and the virtual pointer's clamp) stayed 971 high after the window became 1117 high: the
bottom 146 points were out of reach. (Logged: `ClipCursor 0 0 1728 971 (win 0 0 1728 971)`, then GetCursorPos with the window 1728 x 1117.)
Fix: port/platform/window.cpp: a clip equal to the window's rectangle at the call is the window itself and follows its size and position
(WINDOW_RESIZED / WINDOW_MOVED re-apply it); a clip that is not the window's stays as given. Unit test: test_window.cpp, "the clip
rectangle d3dapp sets right after CreateDevice follows the window" (the pointer reaches both far corners, none beyond; client/screen
at the edges). A native run on the character sheet clicks Done.
Fix commit: (this commit)

Order of work: 6 + 1, then 2 + 7, then 3, 4, 5.
