// view_state.h: the 0x5374-byte view / camera sub-object of an agent (agent +0x258; a camera block reached by offset in
// 0x463370 / 0x463610). One definition for src/game/terrain_b (constructor 0x475e50, agent record) and src/game/ui
// (troop_inv.cpp: init 0x463370, get_dir 0x463610).
#ifndef GAME_VIEW_STATE_H
#define GAME_VIEW_STATE_H
#include "../include/rgl_base.h"
#include "../include/vec.h"
#include "../include/clock_table.h"
#include "../include/tail2.h"      // Tail2 (~Tail2 0x4c4d10): the list at +0x5168, destroyed with the agent


// 8-byte tick values stored as two dwords: the agent's sub-objects are not 8-byte aligned (ViewState is 0x5374 long)
struct tick { int lo, hi; };
inline const tick& tk(int i) { return ((const tick*)g_ticks)[i]; }
struct Timer16 {
    int clock;                  // +0 index into g_ticks
    int pad4;
    tick start;                 // +8
    void init(int c) { clock = c; start = tk(clock); }
    void restamp() { *(__int64*)&start = g_ticks[clock]; }   // one 8-byte copy (ViewBody::init 0x463370: the clock loads stay behind the vector stores)
};
struct Vec4z { float x, y, z, w; };

// The view sub-object of an agent (0x5374 bytes at agent +0x258): five timers, the target agent id and two floats
// the team statistics write. The agent's constructor calls the timer set-up 0x475e50 (defined below as the constructor
// of ViewState, the slices' TimedObj) explicitly, after its first stores; init() 0x463370 is the reset path.
struct ViewThreat { int id; float dist; rgl::vector4 dir; };   // one threat record (0x18; Agent::threats, agent_obj.h)
struct ViewInput {                  // an input snapshot (0x18; Agent::in_prev / in_last, agent_obj.h InputState)
    unsigned bits;              // +0x00
    unsigned f4;                // +0x04
    rgl::vector4 move;          // +0x08 the move direction (Agent::prev_move)
};
// Member names follow Agent (agent_obj.h) at agent offset = view offset + 0x258; init 0x463370 gives the types.
struct ViewBody {
    int f0;                     // +0x00 (agent +0x258)
    rgl::vector4 goal;          // +0x04 (agent +0x25c; init: (-3, 0, 0))
    rgl::vector4 goal_dir;      // +0x14 (agent +0x26c)
    int goal_face;              // +0x24 (= -1)
    int goal_face_done;         // +0x28
    int goal_changed;           // +0x2c
    float f30;                  // +0x30 (agent +0x288; = 1.0f)
    int f34;                    // +0x34 (agent +0x28c)
    int f38;                    // +0x38 (agent +0x290)
    rgl::vector4 move_dir;      // +0x3c (agent +0x294; = (0, 1, 0))
    float f4c;                  // +0x4c (agent +0x2a4; = 0.5f)
    int f50, f54;               // +0x50
    Timer16 t58;                // +0x58 (agent +0x2b0 ai_ta)
    float f68;                  // +0x68 (agent +0x2c0; = 0.1f)
    unsigned f6c;               // +0x6c (agent +0x2c4: bit 0 = alarmed)
    int simple_behavior;        // +0x70 (agent +0x2c8: aisb_*)
    int f74;                    // +0x74
    rgl::vector4 f78;           // +0x78 (agent +0x2d0)
    int f88;                    // +0x88 (agent +0x2e0)
    int f8c;                    // +0x8c
    int f90;                    // +0x90 (= -100000)
    float f94;                  // +0x94 (agent +0x2ec: the second timer's interval; = 1.0f)
    Timer16 t98;                // +0x98 (agent +0x2f0 ai_tb)
    Timer16 ta8;                // +0xa8 (agent +0x300 ai_tc)
    float fb8;                  // +0xb8 (agent +0x310: the third timer's interval; = 0.1f)
    int fbc;                    // +0xbc (agent +0x314)
    int fc0;                    // +0xc0 (agent +0x318; = -1)
    int target;                 // +0xc4 (agent +0x31c) id of another agent (0x474e60)
    int path_face[0x405];       // +0xc8 (agent +0x320)
    rgl::vector4 path_pos[0x3ff];   // +0x10dc (agent +0x1334)
    int path_active;            // +0x50cc (agent +0x5324)
    int path_cur;               // +0x50d0
    int path_count;             // +0x50d4
    char path_flag;             // +0x50d8
    char pad50d9[0x50dc - 0x50d9];
    int path_steps;             // +0x50dc
    int path_state;             // +0x50e0
    int f50e4;                  // +0x50e4 (agent +0x533c)
    Vec2 v50e8;                 // +0x50e8 (agent +0x5340) (0x474e60)
    char pad50f0[0x50f8 - 0x50f0];
    rgl::vector4 wp;            // +0x50f8 (agent +0x5350 wp_x / wp_y / wp_z)
    int speed;                  // +0x5108 (agent +0x5360, a float there; init stores the word 0)
    int veer;                   // +0x510c (agent +0x5364, a float there)
    Vec2 v5110;                 // +0x5110 (agent +0x5368)
    Timer16 t5118;              // +0x5118 (agent +0x5370 ai_timer)
    Timer16 t5128;              // +0x5128 (agent +0x5380 ai_timer2)
    unsigned f5138;             // +0x5138 (agent +0x5390)
    int f513c;                  // +0x513c
    int combat_state;           // +0x5140 (agent +0x5398)
    int f5144;                  // +0x5144
    int f5148;                  // +0x5148 (agent +0x53a0)
    ViewInput in_prev;          // +0x514c (agent +0x53a4)
    unsigned f5164;             // +0x5164 (agent +0x53bc)
    Tail2 tail2;                // +0x5168 (agent +0x53c0 input_q): the constructor 0x475e50 zeroes arr / count / refbase / refcap; the agent's destructor 0x431f40 runs ~Tail2
    char pad5188[0x5194 - 0x5188];  // the rest of in_last (Agent::in_last is +0x517c: Tail2 is 0x20 long, the deque only 0x14)
    ViewThreat threats[16];     // +0x5194 (agent +0x53ec)
    int nthreat;                // +0x5314
    float f5318;                // +0x5318 (agent +0x5570; = 1.0f)
    rgl::matrix scripted_destination;   // +0x531c (agent +0x5574; = identity)
    int scripted_mode;          // +0x535c
    int always_attack_in_melee; // +0x5360
    struct shape_base* last_hit;    // +0x5364
    int last_attacker;          // +0x5368 (= -1)
    unsigned f536c;             // +0x536c
    float speed_limit;          // +0x5370 m/s (= 15.0f)
    unsigned& in_last_bits() { return *(unsigned*)tail2.pad14; }   // +0x517c Agent::in_last.v[0], inside Tail2's padding
    void init_timers();         // 0x475e50  // old name of ViewState::ViewState, removed after the names merge
    void init();                // 0x463370 (src/game/ui/troop_inv.cpp: the reset path)
};
struct ViewState : ViewBody {
    ViewState();                // 0x475e50 (terrain_b.cpp)
};

#endif // GAME_VIEW_STATE_H
