// placed_obj.h: the 0x1c8-byte placed scene object (vtable 0x590b84, "PlacedObj"; the slices also called it PlacedObj /
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

struct EntFlag;
struct stream_base;
struct PlacedObj : IfaceB {
    int m10;                            // +0x10 (0x4352f0)
    int m14;                            // +0x14 (0x4352f0)
    rgl::matrix frame;                  // +0x18 .. +0x157: five 0x40-byte frames (0x437bb0 copies each; 0x435860 writes frame, start_frame);
                                        //   this one is the current position (script prop_instance_get_position 0x4aa404)
    rgl::matrix m58;                    // +0x58
    rgl::matrix start_frame;            // +0x98 the starting position (script prop_instance_get_starting_position 0x4aa459)
    rgl::mat4 md8;                      // +0xd8 (0x4352f0: reset to identity)
    rgl::matrix m118;                   // +0x118
    rgl::vector4 scale;                 // +0x158 x / y / z scale (0x4352f0; script prop_instance_get_scale 0x4aa4df)
    int m168;                           // +0x168 (0x4352f0)
    float f16c;                         // +0x16c (0x4352f0)
    Quad q170;                          // +0x170 (a = 2 on construction, c/d = clock tick)
    int m180, m184;                     // +0x180
    int kind;                           // +0x188 (0x435860; reset 0x4352f0)
    int id;                             // +0x18c (0x435860)
    tstring name;                       // +0x190 (0x435860)
    int value;                          // +0x1a0 (0x435860)
    int m1a4, m1a8, m1ac;               // +0x1a4 (0x4352f0)
    int m1b0;                           // +0x1b0 (0x4352f0)
    EntFlag* ent;                       // +0x1b4 the prop's scene entity (0x4352f0 clears it; 0x472050 shows / hides it, src/game/prop_ent.h)
    int m1b8;                           // +0x1b8 (0x4352f0; agent_logic's second scene entity)
    float max_hp;                       // +0x1bc the prop kind's hit points (read 0x4631c0; troop_inv.cpp: 100 by default)
    float hp;                           // +0x1c0 the prop's hit points (0x45a170 takes the blow's damage off them; init_hp 0x461200)
    int m1c4;                           // +0x1c4
#ifndef PLACEDOBJ_CTOR_OUT_OF_LINE
    PlacedObj() { reset(); }            // 0x437830, inline: the original expands it into most callers (Mission update 0x52f250 ...)
#else
    PlacedObj();                        // 0x437830, out of line (agent2/agent_stats.cpp defines it): the TUs that define PLACEDOBJ_CTOR_OUT_OF_LINE
#endif                                  // before this header call it, as the original's init_scene 0x5144d0 and fn_529d80 0x529d80 do
    PlacedObj(const PlacedObj& o);      // 0x437bb0
    PlacedObj& operator=(const PlacedObj& o);   // 0x437d60
    // implicit virtual destructor: deleting destructor 0x437870
    void reset();                       // 0x4352f0 (src/game/agent/agent.cpp)
    void read(stream_base* s, int version);     // 0x4631c0 (src/game/ui/troop_inv.cpp)
    void init_hp();                     // 0x461200 (terrain_a/wind.cpp; agent4/agent_think.cpp spawn_prop 0x44c210 calls it): hp = max_hp, 100 when none
};

#endif // GAME_PLACED_OBJ_H
