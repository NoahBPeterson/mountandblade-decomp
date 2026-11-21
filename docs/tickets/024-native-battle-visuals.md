# 024: Native Quick Battle: visual and input bugs from the first play-through

Reported 2026-11-20 after a Quick Battle on the native macOS build. Screenshots (user's session):
`images/43.png`, `images/44.png` (under the Claude session's tmp images dir). Lane G2.

## 1. Trees float above the ground
Symptom: whole trees and bare trunks hang in the sky (43.png, top).
Cause: (pending)
Fix commit: (pending)

## 2. Shields render magenta / purple
Symptom: every shield is a flat magenta or purple (43.png, 44.png): the missing-texture colour.
Cause: (pending)
Fix commit: (pending)

## 3. Mouse look limited to about 180 degrees
Symptom: the player cannot turn all the way around.
Cause: (pending)
Fix commit: (pending)

## 4. "Speed bonus: +319%" / "Delivered 171 damage to horse" on a lance hit
Symptom: implausible numbers (44.png, bottom left). Compare with the original formula before calling it a bug.
Cause: (pending)
Fix commit: (pending)

## 5. Black band (~45 px) at the top of the window
Symptom: 43.png and 44.png: the top ~45 px are black. Suspect viewport versus the macOS title bar / notch safe area.
Cause: (pending)
Fix commit: (pending)
