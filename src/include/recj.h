// recj.h: RecJ, the 0xb0-byte IfaceB-derived record (vtable 0x590b78: slot 0 = vector deleting destructor 0x431750, slot 1 =
// reset 0x437810; constructor 0x4377d0). ONE definition for src/game/agent (agent.cpp: constructor and slot1), src/game/terrain_b
// (Pool<RecJ>: next_used 0x474300 / reset 0x477f90 walk the 0x10-page records through IfaceB's f4 = in-use flag, f8 = id, fc =
// serial) and src/rgl/xmlcfg (records.cpp: emits ??_ERecJ 0x431750).
#ifndef MB_RECJ_H
#define MB_RECJ_H
#include "./iface.h"        // IfaceB, Quad

// IfaceB carries three words at +4..+0xc (vtable 0x5908f8 = ??_7IfaceB@@6B@, s00431050).
struct RecJ : IfaceB {
    int pad10[(0x50 - 0x10) / 4];
    int m50;                                    // +0x50 (0x437810)
    int pad54[(0x70 - 0x54) / 4];
    Quad q70;                                   // +0x70 (a = 2, c/d = clock tick copied from g_time[2]; ctor 0x4377d0)
    int m80;                                    // +0x80 (0x437810)
    int pad84;
    int m88;                                    // +0x88 (0x437810)
    int pad8c[(0x98 - 0x8c) / 4];
    int m98;                                    // +0x98 (0x437810)
    int pad9c[(0xb0 - 0x9c) / 4];
    RecJ();                                     // 0x4377d0
    virtual void slot1();                       // reset: 0x437810
};

#endif // MB_RECJ_H
