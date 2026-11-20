//-----------------------------------------------------------------------------
// File: troop_adjust.cpp  (0x437770..0x437830)
//
// Desc: the second object of agent.cpp's range: Troop::adjust (steps a proficiency value by falloff()), which calls
//       Troop::falloff 0x4375b0 as an external function, and the Missile constructor and slot1. Functions in ascending
//       address order. Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "./agent.h"
#include "../agent_obj.h"
#include "../../include/troop.h"

// @ 0x437770
void Troop::adjust(int i, float k)
{
    float r = falloff(stat[i], k);
    stat[i] = r;
    if (r < 0.0f) {
        stat[i] = 0.0f;
        return;
    }
    if (r > 1000.0f)
        stat[i] = 1000.0f;
}

// @ 0x4377d0
Missile::Missile()
{
    slot1();
}
// @ 0x437810
void Missile::slot1()
{
    ent = 0;
    tumble = 0;
    ammo.item = -1;
    launcher.item = -1;
}