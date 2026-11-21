// cgev.swift (tools/port/run_native.py builds it into work/tools/cgev): find the game window and post real input to it.
//   cgev win <owner>              -> "<id> <x> <y> <w> <h>" of the owner's largest window (screen points), or "0"
//   cgev click <x> <y>            -> left click at a screen point (press and release ~80 ms apart)
//   cgev move <x> <y>             -> move the cursor
//   cgev sweep <x> <y> <dx> <n>   -> n mouse moves from (x, y), dx points each (the delta fields set: raw motion past an edge)
//   cgev key <keycode>            -> press and release one virtual key (macOS kVK_* code)
//   cgev front <pid>              -> activate the application with that pid
import AppKit
import CoreGraphics
import Foundation

let a = CommandLine.arguments
func post(_ t: CGEventType, _ x: Double, _ y: Double) {
    CGEvent(mouseEventSource: nil, mouseType: t, mouseCursorPosition: CGPoint(x: x, y: y), mouseButton: .left)?.post(tap: .cghidEventTap)
}
switch a.count > 1 ? a[1] : "" {
case "win":
    let list = CGWindowListCopyWindowInfo([.optionAll], kCGNullWindowID) as! [[String: Any]]
    var best: (Int, Double, [String: Double]) = (0, 0, [:])
    for w in list {
        let owner = w[kCGWindowOwnerName as String] as? String ?? ""
        guard owner.contains(a[2]), let b = w[kCGWindowBounds as String] as? [String: Double] else { continue }
        let area = (b["Width"] ?? 0) * (b["Height"] ?? 0)
        if area > best.1 { best = (w[kCGWindowNumber as String] as? Int ?? 0, area, b) }
    }
    if best.0 == 0 { print(0) } else {
        let b = best.2
        print(best.0, Int(b["X"]!), Int(b["Y"]!), Int(b["Width"]!), Int(b["Height"]!))
    }
case "click":
    let x = Double(a[2])!, y = Double(a[3])!
    post(.mouseMoved, x, y); usleep(60000)
    post(.leftMouseDown, x, y); usleep(80000)
    post(.leftMouseUp, x, y)
case "move":
    post(.mouseMoved, Double(a[2])!, Double(a[3])!)
case "sweep":
    let sx = Double(a[2])!, sy = Double(a[3])!, dx = Double(a[4])!, n = Int(a[5])!
    for i in 0..<n {
        let e = CGEvent(mouseEventSource: nil, mouseType: .mouseMoved, mouseCursorPosition: CGPoint(x: sx + Double(i) * dx, y: sy), mouseButton: .left)
        e?.setIntegerValueField(.mouseEventDeltaX, value: Int64(dx))
        e?.setIntegerValueField(.mouseEventDeltaY, value: 0)
        e?.post(tap: .cghidEventTap)
        usleep(8000)
    }
case "key":
    let k = CGKeyCode(UInt16(a[2])!)
    CGEvent(keyboardEventSource: nil, virtualKey: k, keyDown: true)?.post(tap: .cghidEventTap); usleep(80000)
    CGEvent(keyboardEventSource: nil, virtualKey: k, keyDown: false)?.post(tap: .cghidEventTap)
case "front":
    NSRunningApplication(processIdentifier: pid_t(Int32(a[2])!))?.activate(options: [])
default:
    print("usage: cgev win <owner> | click <x> <y> | move <x> <y> | key <code> | front <pid>")
}
