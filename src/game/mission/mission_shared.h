//-----------------------------------------------------------------------------
// File: mission_shared.h  (shared by mission_frame_update.cpp, mission_hud_and_orders.cpp and terrain_brush.cpp)
//
// Desc: bodies of callees another object defines that the old mission_5.cpp saw (AgentTable walkers, GameKeys::clicked, Blow::init,
//       vector4::normalize); declared inline noinline so the calls stay calls. Kept once for the three parts.
//-----------------------------------------------------------------------------
#pragma once

// The agent table's walkers: the original's bodies were visible here (cl keeps g_mission in a register across them,
// as they write no global), though it left them out of line. agent2/agent_ai.cpp owns the rows at these addresses.
inline __declspec(noinline) void AgentTable::next_used(unsigned* i)
{
    ++*i;
    while ((int)*i < count) {
        unsigned v = *i;
        if (pages[v >> 4][v & 0xf].in_use != 0) break;
        ++*i;
    }
}

inline __declspec(noinline) int AgentTable::first_used()
{
    unsigned v = 0;
    if (0 < count) {
        Agent** pg = &pages[0];
        do {
            if (pg[v >> 4][v & 0xf].in_use != 0) break;
            v++;
        } while ((int)v < count);
    }
    return v;
}

// GameKeys::clicked 0x48ac90 (game/ui/widgets/widget_base.cpp owns the row): its body was visible here too; the screen tick keeps
// g_mission and g_game in registers across its calls.
inline __declspec(noinline) bool GameKeys::clicked(int i)
{
    bool r = false;
    int a = entry[i][0];
    if (a >= 0 && g_keyboard.down[a] != 0 && g_keyboard.prev[a] == 0)
        r = true;
    int b = entry[i][1];
    if (b >= 0 && g_keyboard.down[b] != 0 && g_keyboard.prev[b] == 0)
        r = true;
    return r;
}

inline __declspec(noinline) Blow* Blow::init()      // 0x4353a0: visible too (game/agent/agent_queries.cpp owns the row)
{
    vector4 zero(0.0f, 0.0f, 0.0f);
    vector4 unit(1.0f, 0.0f, 0.0f);
    pos = zero;
    dir = unit;
    dir2 = unit;
    flags = 0;
    attacker = -1;
    bone = 0;
    dmg_in = 0;
    kind = 0;
    item = -1;
    missile = 0;
    dmg_out = 0;
    absorbed = 0;
    return this;
}

// 0x407d60 (engine/physics/collision_shapes.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
namespace rgl {
inline __declspec(noinline) float vector4::normalize()      // 0x407d60
{
    float len = length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
}
