// placed_obj.h: the 0x1c8-byte placed scene object (vtable 0x590b84, "PlacedObj"; the slices also called it Rec1c8 /
// E1c8) and its polymorphic base IfaceB (vtable 0x5908f8) with the Quad tick record. One definition for
// src/game/agent (reset 0x4352f0, init_placed 0x435860) and src/game/agent2 (constructor 0x437830, copy constructor
// 0x437bb0, assignment 0x437d60; deleting destructor 0x437870 is compiler generated).
// Offsets cite the function that proves them. Needs match/include/rgl.h or src/rgl/rgl_math.h included first
// (rgl_base.h picks the module's own), and the game's tstring.
#ifndef GAME_PLACED_OBJ_H
#define GAME_PLACED_OBJ_H
#include "../include/rgl_base.h"
#include "../include/mat4.h"
#include "./tstring.h"

#include "../include/iface.h"        // IfaceB, Quad, g_tick_lo / g_tick_hi

struct PlacedObj : IfaceB {
    int m10;                            // +0x10 (0x4352f0)
    int m14;                            // +0x14 (0x4352f0)
    rgl::matrix m18;                    // +0x18 .. +0x157: five 0x40-byte frames (0x437bb0 copies each; 0x435860 writes m18, m98)
    rgl::matrix m58;                    // +0x58
    rgl::matrix m98;                    // +0x98
    rgl::mat4 md8;                      // +0xd8 (0x4352f0: reset to identity)
    rgl::matrix m118;                   // +0x118
    rgl::vector4 v158;                  // +0x158 (0x4352f0)
    int m168;                           // +0x168 (0x4352f0)
    float f16c;                         // +0x16c (0x4352f0)
    Quad q170;                          // +0x170 (a = 2 on construction, c/d = clock tick)
    int m180, m184;                     // +0x180
    int kind;                           // +0x188 (0x435860; reset 0x4352f0)
    int id;                             // +0x18c (0x435860)
    tstring name;                       // +0x190 (0x435860)
    int value;                          // +0x1a0 (0x435860)
    int m1a4, m1a8, m1ac;               // +0x1a4 (0x4352f0)
    int m1b0, m1b4, m1b8;               // +0x1b0 (0x4352f0)
    int m1bc, m1c0, m1c4;               // +0x1bc (m1c0 is the prop's hit points as a float: hp())
    PlacedObj();                        // 0x437830
    PlacedObj(const PlacedObj& o);      // 0x437bb0
    PlacedObj& operator=(const PlacedObj& o);   // 0x437d60
    // implicit virtual destructor: deleting destructor 0x437870
    float& hp() { return *(float*)&m1c0; }   // +0x1c0 as a float: the prop's hit points (0x45a170; update_1c0 0x461200)
    void reset();                       // 0x4352f0 (src/game/agent/agent.cpp)
    void update_1c0();                  // 0x461200 (agent4/agent_think.cpp spawn_prop; ICF-folded with Agent::init_max_hp)
};

#endif // GAME_PLACED_OBJ_H
