# 025: Native world map: camera view and keyboard (lane G3)

Reported by lane G1 (2026-11-21): the map opens in a low, near-horizontal view; Space did not unpause the map.
Reproduce: `tools/port/run_native.py --user-dir work/port_user/g1 --start map --wait start_done ...` (MB_START=map restores the first save).

## 1. The map camera is "wrong" (low, near-horizontal view)
Finding: no defect in the camera, the projection or the terrain; the view is what the binary computes.
- `CampaignScreen::camera_update` (0x4ece20, byte-matched) sets pitch = zoom * 0.014 + 0.23 (clamped 0.1 .. pi/2; both constants checked against
  the image: 0x590f7c = 0x3c656042, 0x590e68 = 0x3e6b851f) and places the eye `zoom` units behind the target.
- The only writers of the camera zoom (+0x2b8) in the whole exe are the ctor 0x4c49f0 (`mov [esi+0x2b8], 0x40a00000`: 5.0), camera_update's clamp
  (3.5 .. 105; 235 with the cheat keys) and read_stream (the save's value); a save written by the port stores 5.0. Zoom 5 gives pitch 0.30 rad
  (17 degrees), eye 5 units from the party: a low close-up with the horizon in view, as the code says.
- The terrain is the module's `map.txt` (31307 nav vertices, z -1.7 .. 16, the map about 220 units across). Read back through
  `Game::ground_height` at 7 spread-out vertices: identical to the file. Around the Training Field the file is flat (z 0..1) with a narrow ridge
  (z 5..6) 30 units north: at 17 degrees from 5 units away that ridge fills the top of the screen. The top-down view shows the same ridge.
- Zooming out works: the mouse wheel (run_native.py `scroll -3 25`) takes zoom to 105, pitch 1.5708: a straight top-down view of the whole
  region with its labels at readable size.
Cause: none found; the "tiny labels / huge terrain" is the 17-degree default view of authored terrain. If the original opens higher, the difference is
not in any code path of the exe that sets the zoom; needs a reference shot of the original's first map frame (Crossover) to compare.
Fix commit: none.

## 2. Space does not unpause the map
Cause: not the port. On the map Space is hold-to-advance: `CampaignScreen::frame_update` (0x514d40) sets the time step (`f13aa4 = 2`) only while
`g_keyboard.down[57]` (DIK_SPACE) is down; released, `GameData::tick` (0x516ac0) sets the step to 0 and the "Paused" banner shows. run_native.py's
`key` tapped for 80 ms (a few frames), so the banner was back by the time the shot was taken. Keyboard events do reach the DirectInput keyboard
(window and focus are fine, the device cursor is shown on the map so the mouse is in absolute mode: window.cpp logs vis=1).
Fix: tools/port/cgev.swift + run_native.py: `key <code> [hold ms]` (a held key runs in the background so later steps, shots, happen while it is
down) and a new `scroll <lines> [n]` action (mouse wheel).
Seen: `--do key 49 5000 --do sleep 3 --do shot b`: the "Paused" banner is gone while Space is held. A click on the map (absolute mode) sends the party
to the destination: the camera follows, the clock advances (Dawn -> Early morning), "Paused" disappears.
Fix commit: (this commit)
