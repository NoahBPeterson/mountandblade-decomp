// Slice s00534160: the tail of the mission-screen family.
//
//   0x534160  VC7.1 vector-iterator arithmetic (`_Ptr + n` for a 4-byte element type) in the
//             out-parameter form this build emits: `*out = ptr + n`.  Called out of line from
//             0x4d56cf..0x4d58e6 (MissionSource) and 0x5163ca..0x51664d (Unit).
//   0x534180  a mission-screen frame hook (vtable 0x5961f0 slot 0): advance the item source, then
//             if the repeat interval has elapsed, run the object's pending menu option (+0x18):
//             100 commits the framed flags, anything else is handed to the main menu.  Otherwise
//             (interval not elapsed) run the queued mission command in g_game+0xc8: 0xf -> reset it
//             and choose menu option 0x1b.
//   0x534230  the same without the queued-command half (vtable 0x596214 slot 0).
//
// `this` is the screen object: +0x18 is a pending menu-option id, +0x30 an embedded MissionSource
// (0x4d5410 reads +0x10 family, +0x18 slot, +0x20 tick), +0x58 the repeat interval in seconds.
// The 64-bit clock table at 0x5e2948 and the game/menu globals are the ones already verified in
// slices s004d5410 / s00514d40.  Names are provisional; every offset is read off the disassembly.
#include <windows.h>
#include "rgl.h"

// ---- globals ---------------------------------------------------------------------------------------
extern __int64 g_ticks[];                           // 0x5e2948: 64-bit clocks, 1e-5 s units
extern char* g_game;                                // 0x5e2930
#define G_INT(off) (*(int*)(g_game + (off)))

// the main menu (0xa73980); choose() is 0x495700 (thiscall, ret 4) -- see s00514d40
struct MainMenu {
    void choose(int id);                            // 0x495700
};
extern MainMenu g_main_menu;                        // 0xa73980

// the framed command block at 0x5e291c; commit() is 0x513710 (thiscall, ret 4), out flag byte
struct Flags5e291c { void commit(void* out); };     // 0x513710
extern Flags5e291c g_flags;                         // 0x5e291c

// ---- the item source embedded at Screen+0x30 (layout from s004d5410) --------------------------------
struct MissionSource {
    char pad0[0x10];
    int family;                                     // +0x10
    int pad14;
    int slot;                                       // +0x18: index into g_ticks
    int pad1c;
    __int64 tick;                                   // +0x20: tick the slot was entered at
    void pick(int* arg);                            // 0x4d5410 (ret 4; arg unused)
};

// ---- the screen object -----------------------------------------------------------------------------
struct Screen {
    char pad0[0x18];
    int f18;                                        // +0x18 (pending menu option / command)
    char pad1c[0x30 - 0x1c];
    MissionSource src;                              // +0x30
    float interval;                                 // +0x58 (repeat interval, seconds)
    void f534180(int arg);                          // 0x534180
    void f534230(int arg);                          // 0x534230
};

// ---- 0x534160: the iterator helper --------------------------------------------------------------
// `*out = ptr + n`.  The original keeps it out of line (0x4d5xxx/0x516xxx call
// it), so it cannot be an inline body in this slice.
struct VecIter {
    int* ptr;
    void advance(int** out, int n);                 // 0x534160 (ret 8)
};

// @ 0x534160
void VecIter::advance(int** out, int n)
{
    *out = ptr + n;
}

// @ 0x534180 -----------------------------------------------------------------------------------------
void Screen::f534180(int arg)
{
    src.pick(&arg);
    if ((float)(g_ticks[src.slot] - src.tick) * 1e-5f > interval) {
        int id = f18;
        if (id == 100) {
            g_flags.commit(&arg);
            return;
        }
        g_main_menu.choose(id);
        return;
    }
    if (G_INT(0xc8) == 0xf) {
        G_INT(0xc8) = 0;
        g_main_menu.choose(0x1b);
    }
}

// @ 0x534230 -----------------------------------------------------------------------------------------
void Screen::f534230(int arg)
{
    src.pick(&arg);
    if ((float)(g_ticks[src.slot] - src.tick) * 1e-5f > interval) {
        int id = f18;
        if (id == 100) {
            g_flags.commit(&arg);
            return;
        }
        g_main_menu.choose(id);
    }
}
