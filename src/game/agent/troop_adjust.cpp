//-----------------------------------------------------------------------------
// File: troop_adjust.cpp  (0x437770..0x437830)
//
// Desc: the second object of agent.cpp's range: Troop::adjust (steps a proficiency value by falloff()), which calls
//       Troop::falloff 0x4375b0 as an external function, and the RecJ constructor and slot1. Functions in ascending
//       address order. Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "./agent.h"
#include "../agent_obj.h"
#include "../../include/troop.h"

// @ 0x437770
void Troop::adjust(int i, float k)
{
    float r = falloff(f80[i], k);
    f80[i] = r;
    if (r < 0.0f) {
        f80[i] = 0.0f;
        return;
    }
    if (r > 1000.0f)
        f80[i] = 1000.0f;
}

// @ 0x4377d0
RecJ::RecJ()
{
    slot1();
}
// @ 0x437810
void RecJ::slot1()
{
    m50 = 0;
    m98 = 0;
    m88 = -1;
    m80 = -1;
}