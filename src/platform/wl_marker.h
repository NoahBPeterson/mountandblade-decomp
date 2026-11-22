// wl_marker.h: the WinLicense/Themida SDK region markers the original's source left inline in some functions.
// Each is 18 bytes: `jmp +0x10` over "WL  ", the id dword, a zero dword and "WL  ". The jmp skips them, so they have no
// runtime effect. Observed ids: 12 / 13 (start / end of a protected region), 4 / 5, and 6. This header is the one
// place `_emit` is allowed (tools/matching/asm_audit.py, docs/matching.md). The __asm statements are a scheduling
// barrier for the compiler, exactly as in the original.
#ifndef WL_MARKER_H
#define WL_MARKER_H

#define WL_MARKER(id) \
    __asm _emit 0xeb __asm _emit 0x10 \
    __asm _emit 0x57 __asm _emit 0x4c __asm _emit 0x20 __asm _emit 0x20 \
    __asm _emit (id) & 0xff __asm _emit ((id) >> 8) & 0xff __asm _emit ((id) >> 16) & 0xff __asm _emit ((id) >> 24) & 0xff \
    __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 \
    __asm _emit 0x57 __asm _emit 0x4c __asm _emit 0x20 __asm _emit 0x20

// The long variant seen once (id 7 at 0x4d0bfb, the end of init_mission_trigger_records): 23 bytes, `jmp +0x15` over
// "WL  ", id, zero dword, "WL  " and five zero bytes.
#define WL_MARKER_LONG(id) \
    __asm _emit 0xeb __asm _emit 0x15 \
    __asm _emit 0x57 __asm _emit 0x4c __asm _emit 0x20 __asm _emit 0x20 \
    __asm _emit (id) & 0xff __asm _emit ((id) >> 8) & 0xff __asm _emit ((id) >> 16) & 0xff __asm _emit ((id) >> 24) & 0xff \
    __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 \
    __asm _emit 0x57 __asm _emit 0x4c __asm _emit 0x20 __asm _emit 0x20 \
    __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0 __asm _emit 0

#endif
