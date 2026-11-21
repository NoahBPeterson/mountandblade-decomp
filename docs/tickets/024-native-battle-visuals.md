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
Finding: not a defect of the game's viewport. On the Mac used here the game window is 1728 x 1084 points at y = 33 (run_native.py
prints the rectangle on every shot) on a 1728 x 1117 screen: macOS places the full-screen window below the notch / menu-bar band,
and a whole-screen screenshot shows that band as black; the window's own capture (screencapture -l) has no band, and the game
draws to the full window height (shots end with the health bar at the bottom edge). Nothing to fix in the port unless the notch
area should be used (a design decision: content under the notch is cut by the camera housing).
Fix commit: none.

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

Order of work: 6 + 1, then 2 + 7, then 3, 4, 5.
