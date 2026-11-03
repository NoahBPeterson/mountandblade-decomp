// winshot.swift (tools/relink/smoke.py builds it into work/tools/winshot): the CGWindowID of a window by owner name, so
// screencapture -l can grab the game window even while it is behind another one.
import CoreGraphics
import Foundation
// winshot <owner-substring>: print the CGWindowID of the largest on-screen window whose owner name contains the argument
let want = CommandLine.arguments[1]
let list = CGWindowListCopyWindowInfo([.optionAll], kCGNullWindowID) as! [[String: Any]]
var best: (Int, Double) = (0, 0)
for w in list {
    let owner = w[kCGWindowOwnerName as String] as? String ?? ""
    guard owner.contains(want), let b = w[kCGWindowBounds as String] as? [String: Double] else { continue }
    let area = (b["Width"] ?? 0) * (b["Height"] ?? 0)
    if area > best.1 { best = (w[kCGWindowNumber as String] as? Int ?? 0, area) }
}
print(best.0)
