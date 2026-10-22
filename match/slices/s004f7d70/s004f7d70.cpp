// Slice s004f7d70: the agent equipment reset (0x4f7d70) and the agent appearance refresh (0x4f7e00, also a
// standalone helper: neighbouring code reaches it with the agent in EAX).  0x4f7d70 re-picks the equipment,
// refreshes the appearance, clears the wielded item ids and re-wields the two hand slots -- cycling them when
// the agent wears the same equipment kind as the current player item (bit 1 of the module table entry).
// 0x4f7e00 resets the body, clears the 22-dword equipment id array, rebuilds body and equipment through
// 0x482240 / 0x47cfe0 and caches the equipment value.  Agent layout from s0045ff30; names provisional.
// clang-format off
#include <windows.h>
#include "rgl.h"

struct AgentBody {
    char pad0[0x304];
    int f304;                                            // +0x304 (reset by 0x420ec0)
};

struct Agent {
    char pad0[8]; int f8;                                // +8
    char padc[4]; int f10;                               // +0x10
    char pad14[0x88 - 0x14]; int f88; int f8c; int f90; int f94;
    char pad98[0x130 - 0x98]; int f130; int f134; int f138; int f13c[22]; int f194;
    char pad198[0x1a4 - 0x198]; int f1a4; int f1a8;
    char pad1ac[0x1d0 - 0x1ac]; int f1d0;
    char pad1d4[0x22c - 0x1d4]; int f22c; int f230;
    char pad234[0x240 - 0x234]; int f240; float f24c; AgentBody* body;   // +0x250
    char pad254[0x5df0 - 0x254]; int f5df0;              // +0x5df0 (equipment kind)
    char pad5df4[0x6528 - 0x5df4]; unsigned char f6528;  // +0x6528 (bit 0: no mount)
    float equipment_value() const;                       // 0x436050
    void on_wield_reset();                               // 0x445a20
    void cycle_slot(int i);                              // 0x44bc30
    void f470(int i, int j);                             // 0x470d80
    void init();                                         // 0x4f7d70
    void refresh();                                      // 0x4f7e00
};

void AgentEq_pick_slots(Agent* a);                       // 0x46b2d0 (stdcall, one arg)
void refresh_agent(Agent* a);                            // 0x4f7e00 as reached by the original (see below)
void FUN_00420ec0(int x);                                // 0x420ec0
extern int g_5e2a00;                                     // 0x5e2a00 (module table, entry = 0x58 bytes)

// 0x482240: pushes f8, f1d0, f1a8, f1a4, f194, f240, f13c, f8c, f94, f88, f10, body -- in this order
// @ 0x482240
void build_appearance(AgentBody* b, int f10, int f88, int f94, int* f8c, int* f13c,
                      int* f240, int* f194, int* f1a4, int* f1a8, int* f1d0, int f8);
// 0x47cfe0: pushes f8, f1d0, f130, f13c, f88, f10, body
// @ 0x47cfe0
void build_equipment(AgentBody* b, int f10, int f88, int* f13c, int* f130, int* f1d0, int f8);

// @ 0x4f7e00
void Agent::refresh()
{
    FUN_00420ec0(body->f304);
    int zero = 0;                                        // one zero register reused for all 0x17 stores
    for (int p = 0; p < 0x17; p++)
        *(int*)((char*)f13c + p * 4) = zero;
    build_appearance(body, f10, f88, f94, &f8c, f13c, &f240, &f194, &f1a4, &f1a8, &f1d0, f8);
    build_equipment(body, f10, f88, f13c, &f130, &f1d0, f8);
    f24c = equipment_value();
    f470(0, 20);
    f470(1, 20);
}

// the original reaches 0x4f7e00 as a one-argument function (argument in EAX): its caller moves `this` to EAX,
// keeps it in EDI and enters through `mov esi, [esp+8]`, so the prologue differs from a member call.
void refresh_agent(Agent* a)
{
    a->refresh();
}

// @ 0x4f7d70
void Agent::init()
{
    AgentEq_pick_slots(this);
    refresh_agent(this);
    f22c = -1;
    f230 = -1;
    on_wield_reset();
    if (f8 == ((int*)this)[0x5c / 4] &&
        ((f6528 & 1) || (1 & (*(int*)((char*)&g_5e2a00 + f5df0 * 0x58 + 0x20) >> 1)))) {
        cycle_slot(0);
        cycle_slot(1);
    }
    f470(0, 20);
    f470(1, 20);
}
