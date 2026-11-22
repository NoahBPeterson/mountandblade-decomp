// placed_obj.h: the 0x1c8-byte placed scene object (vtable 0x590b84, "PlacedObj"; the slices also called it PlacedObj /
// E1c8) and its polymorphic base IfaceB (vtable 0x5908f8) with the Quad tick record. One definition for
// src/game/agent (reset 0x4352f0, init_placed 0x435860) and src/game/agent2 (constructor 0x437830, copy constructor
// 0x437bb0, assignment 0x437d60; deleting destructor 0x437870 is compiler generated).
// Offsets cite the function that proves them. Needs match/include/rgl.h or src/engine/math/rgl_math.h included first
// (rgl_base.h picks the module's own), and the game's tstring.
#ifndef GAME_PLACED_OBJ_H
#define GAME_PLACED_OBJ_H
#include "engine/core/rgl_base.h"
#include "engine/math/mat4_include.h"
#include "engine/core/tstring.h"

#include "game/common/iface.h"        // IfaceB, Quad, g_tick_lo / g_tick_hi
#include "engine/core/timer.h"        // clock2_timer: the tween clock

namespace rgl { struct entity; }
struct stream_base;
struct PlacedObj : IfaceB {
    int m10;                            // +0x10 (0x4352f0)
    int m14;                            // +0x14 (0x4352f0)
    rgl::matrix frame;                  // +0x18 .. +0x157: five 0x40-byte frames (0x437bb0 copies each; 0x435860 writes frame, start_frame);
                                        //   this one is the current position (script prop_instance_get_position 0x4aa404)
    rgl::matrix m58;                    // +0x58
    rgl::matrix start_frame;            // +0x98 the starting position (script prop_instance_get_starting_position 0x4aa459)
    rgl::mat4 md8;                      // +0xd8 the tween's target frame (0x4352f0: reset to identity; animate_to 0x4897a0)
    rgl::matrix m118;                   // +0x118 the tween's start frame
    rgl::vector4 scale;                 // +0x158 x / y / z scale (0x4352f0; script prop_instance_get_scale 0x4aa4df)
    int m168;                           // +0x168 the tween state (-1 idle; 0x4352f0)
    float f16c;                         // +0x16c the tween's duration in seconds (0x4352f0)
    clock2_timer tween_clock;           // +0x170 the tween's clock and start tick (constructed on clock 2)
    float last_t;                       // +0x180 the tween phase of the last update
    int m184;                           // +0x184
    int kind;                           // +0x188 (0x435860; reset 0x4352f0)
    int id;                             // +0x18c (0x435860)
    tstring name;                       // +0x190 (0x435860)
    int value;                          // +0x1a0 (0x435860)
    int m1a4, m1a8, m1ac;               // +0x1a4 (0x4352f0)
    int m1b0;                           // +0x1b0 (0x4352f0)
    rgl::entity* ent;                   // +0x1b4 the prop's scene entity (0x4352f0 clears it; 0x472050 shows / hides it)
    rgl::entity* ent2;                  // +0x1b8 the second scene entity of a kind-7 prop (spawn_prop 0x44c210; 0x4352f0 clears it)
    float max_hp;                       // +0x1bc the prop kind's hit points (read 0x4631c0; troop_inventory.cpp: 100 by default)
    float hp;                           // +0x1c0 the prop's hit points (0x45a170 takes the blow's damage off them; init_hp 0x461200)
    int m1c4;                           // +0x1c4
#ifndef PLACEDOBJ_CTOR_OUT_OF_LINE
    PlacedObj() { reset(); }            // 0x437830, inline: the original expands it into most callers (Mission update 0x52f250 ...)
#else
    PlacedObj();                        // 0x437830, out of line (game/agent/agent_stats.cpp defines it): the TUs that define PLACEDOBJ_CTOR_OUT_OF_LINE
#endif                                  // before this header call it, as the original's init_scene 0x5144d0 and fn_529d80 0x529d80 do
    PlacedObj(const PlacedObj& o);      // 0x437bb0
    PlacedObj& operator=(const PlacedObj& o);   // 0x437d60
    // implicit virtual destructor: deleting destructor 0x437870
    void reset();                       // 0x4352f0 (src/game/agent/agent_queries.cpp)
    void read(stream_base* s, int version);     // 0x4631c0 (src/game/data/troop_inventory.cpp)
    void save(stream_base* s);                  // 0x527ef0 (src/game/mission5/mission_5.cpp)
    void init_hp();                     // 0x461200 (game/terrain/wind_helpers.cpp; agent4/agent_think.cpp spawn_prop 0x44c210 calls it): hp = max_hp, 100 when none
    void animate(float dt);             // 0x467c50 (game/terrain/terrain_particles.cpp: the tween from m118 to md8; dt unused)
    void animate_to(const rgl::matrix* to, const float* seconds, const int* state);   // 0x4897a0 (game/ui/widgets/widget_base.cpp: prop_instance_animate_to_position)
    void set_position(const rgl::matrix* m);   // 0x489820 (game/ui/widgets/widget_base.cpp: prop_instance_set_position)
    void format(char* out);             // 0x4c6620 (game/editor/editor_selection_and_translation.cpp): the map editor's label of the object, by kind
};

#endif // GAME_PLACED_OBJ_H
