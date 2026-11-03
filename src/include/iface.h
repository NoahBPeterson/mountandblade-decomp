// iface.h: IfaceB, the polymorphic base of the engine's record classes (vtable 0x5908f8 = ??_7IfaceB@@6B@; slots 1 and 2 are
// the empty function 0x477b70, slot 0 is the scalar deleting destructor 0x431420), and Quad, the tick record the
// 0x1c8-byte placed object embeds. ONE definition for src/game (placed_obj.h, terrain_b/records.h, agent) and src/rgl/xmlcfg
// (records.h: the vector deleting destructors of RecF / RecJ / RecY / RecZ). IfaceB carries three words at +4..+0xc (copied
// by the 0x1c8-byte record's copy constructor 0x437bb0). The constructor is inline: cl inlines it into every derived
// constructor (0x437830, 0x4377d0) and emits the vtable and ??_G wherever one is used.
#ifndef MB_IFACE_H
#define MB_IFACE_H
#include "./clock_table.h"




struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Quad {
    int a, b, c, d;
    Quad() : a(2), c((int)g_ticks[2]), d((int)(g_ticks[2] >> 32)) {}
};

#endif // MB_IFACE_H
