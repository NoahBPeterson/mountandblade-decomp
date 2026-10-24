// rec1c8.h: Rec1c8, the 0x1c8-byte polymorphic record (vtable 0x590b84, constructor 0x437830 (inlined), copy constructor
// 0x437bb0, operator= 0x437d60, reset 0x4352f0). ONE definition for src/game/agent3 (agent_combat.cpp: the
// vector<Rec1c8> instantiations 0x43ea00 / 0x43f410 / 0x440b80) and src/game/terrain_b (props: 0x472050, 0x479000,
// read 0x4631c0). src/game/placed_obj.h (PlacedObj) describes the same record with its own name: the manifests and
// symbols/slices bind both mangled names (?..@Rec1c8@@ and ?..@PlacedObj@@), so the two names stay.
#ifndef MB_REC1C8_H
#define MB_REC1C8_H
#include "../game/tstring.h"
#include "./iface.h"        // IfaceB, Quad, g_tick_lo / g_tick_hi

struct EntFlag;
struct stream_base;
struct Rec1c8 : IfaceB {
    int m10;                                    // +0x10 (0x479000 stores the scene id)
    char body1[0x170 - 0x14];
    Quad q170;                                  // +0x170 (constructed by the inlined constructor 0x437830)
    char body2[0x188 - 0x180];
    int destroyed;                              // +0x188 (0 intact, 1 / 5 special states) (0x472050)
    int kind;                                   // +0x18c (0x472050)
    tstring name;                               // +0x190 (0x437bb0 copies it)
    char body3[0x1b4 - 0x1a0];
    EntFlag* ent;                               // +0x1b4 scene entity of the prop (0x472050)
    char body4[0x1c8 - 0x1b8];
    Rec1c8() { reset(); }                       // 0x437830 (inlined)
    void reset();                               // 0x4352f0
    Rec1c8(const Rec1c8& o);                    // 0x437bb0
    Rec1c8& operator=(const Rec1c8& o);         // 0x437d60
    void read(stream_base* s, int version);     // 0x4631c0 (the slices' Placed::read)
};

#endif // MB_REC1C8_H
