// agent_ai.h: AgentAi, the agent's AI / movement-planning sub-object (0x5378 bytes at Agent +0x258, game/agent/agent_obj.h:
// Agent::ai): the goal, the path find_path fills, the AI timers, the input queue, the sorted threats and the scripted
// destination. Its constructor 0x475e50 (src/game/terrain/poly_mesh.cpp) runs as the member's constructor inside the
// agent's 0x477a30; init 0x463370 (src/game/data/troop_inventory.cpp) is the reset path (Agent::reset 0x472f90 calls it).
// The slices called it ViewState / ViewBody / TimedObj / RecZSubA. Offsets in the comments are the agent's (ai + 0x258),
// the functions cited are Agent members (agent2 / agent3 / agent4) unless noted.
#ifndef GAME_AGENT_AI_H
#define GAME_AGENT_AI_H
#include <deque>
#include "engine/core/rgl_base.h"
#include "engine/math/vec.h"
#include "engine/core/timer.h"                   // Timer, clock2_timer

struct shape_base;

// 0x18-byte entry of the sorted nearest-enemy list (0x43b300, 0x443430)
struct Threat { int id; float dist; rgl::vector4 dir; };
// queued input record (+0x18 the tick it is due; one 0x20-byte record per deque block: 0x43c9c0)
struct InputRec { unsigned bits; char pad[0x14]; __int64 time; };
// an input snapshot (0x18): the bits, a flag word and the move direction (Agent::in_now is one too)
struct InputState {
    unsigned bits;                                      // +0x00 movement / action bits
    bool f4;                                            // +0x04 (a byte flag: 0x4556d0 tests it with test al,al)
    char pad5[3];
    rgl::vector4 move;                                  // +0x08 the move direction
};

struct AgentAi {
    int ai_flags;                                       // +0x258 the spawn rule's ai flags (SpawnRec +0x20; 0x10 = aif_start_alarmed; init clears it)
    rgl::vector4 goal;                                  // +0x25c (0x43db90; init: (-3, 0, 0))
    rgl::vector4 goal_dir;                              // +0x26c
    int goal_face;                                      // +0x27c (= -1)
    int goal_face_done;                                 // +0x280 0x445910 (plan_path): the goal face the path was planned for
    int goal_changed;                                   // +0x284
    float melee_standoff;                               // +0x288 0x4465e0 (the stand-off distance of a melee attack: 1.4 x of it is the lateral threshold; = 1)
    float melee_standoff_low;                           // +0x28c 0x4465e0 (melee_standoff - 0.4: the lower stand-off distance)
    float action_countdown;                             // +0x290 0x44df30 (a countdown: set to 5 at every choose_action)
    rgl::vector4 move_dir;                              // +0x294 the move direction (0x448eb0; blend_target's second argument 0x441020; = (0, 1, 0))
    float alertness;                                    // +0x2a4 (0x44df30: decays by dt*0.002, floored at 0.5; = 0.5)
    int f2a8, f2ac;                                     // +0x2a8 (init clears them)
    clock2_timer ta;                                    // +0x2b0 (clock index + start tick; 0x44df30)
    float ta_interval;                                  // +0x2c0 seconds of ta: the delay before the next attack (start_attack 0x4464d0,
                                                        //   ai_idle_tick 0x44df30; cleared by agent_set_scripted_destination 0x4a9604 and 0x443140; = 0.1)
    unsigned alarm;                                     // +0x2c4 bit 0 = alarmed (0x443140)
    int simple_behavior;                                // +0x2c8 aisb_* (agent_get_simple_behavior 0x4a9737, 0x435b90)
    int action_target;                                  // +0x2cc 0x4465e0 (the action's target, id[index] of choose_action; it outlives `target`)
    rgl::vector4 action_point;                          // +0x2d0 0x4465e0, 0x44d000 (the point of the chosen action, pv[index] of choose_action)
    int shot_failed;                                    // +0x2e0 0x44d000, 0x448eb0 (2 = the shot failed, the next scoring of the action is damped)
    int circle_mode;                                    // +0x2e4 0x4465e0 (1 = circling the melee target, 0 = closing in)
    int circle_mode_prev;                               // +0x2e8 0x4465e0 (circle_mode of the previous decision; -1 = none yet; init: -100000)
    float tb_interval;                                  // +0x2ec seconds of tb (ai_idle_tick 0x44df30; choose_action 0x44d000 clears it; = 1)
    clock2_timer tb;                                    // +0x2f0 0x44df30
    clock2_timer tc;                                    // +0x300 0x44df30
    float tc_interval;                                  // +0x310 seconds of tc (ai_idle_tick 0x44df30; update_ai 0x4465e0 clears it; = 0.1)
    float melee_charge;                                 // +0x314 0x4465e0 (the melee charge accumulated in the circling step, 0..11 /..30)
    int goal_agent;                                     // +0x318 (the agent set_goal_agent last aimed at, -1: a point)
    int target;                                         // +0x31c 0x4465e0, 0x44e2b0, 0x4556d0 (the current target id of the AI, -1 = none; 0x474e60)
    int path_face[0x401];                               // +0x320 [0] is the slot before the face list find_path fills (pface())
    rgl::vector4 path_pos[0x400];                       // +0x1324 the portal midpoints find_path fills (plan_path 0x445910)
    int path_active;                                    // +0x5324
    int path_cur;                                       // +0x5328
    int path_count;                                     // +0x532c
    char path_flag;                                     // +0x5330
    int path_steps;                                     // +0x5334 0x445910 (find_path out_steps)
    int path_state;                                     // +0x5338
    int move_mode;                                      // +0x533c (0x435b90: the movement mode of ai_update_control, bit 1 = toward the target, 0x20 = ranged)
    Vec2 hand_off;                                      // +0x5340 0x43a510 (hand_pos's offsets: x across, y along the class dir; 0x474e60 writes the formation slot)
    float hand_off_z;                                   // +0x5348 0x43a510 (the across-dir offset of hand_pos)
    float hand_off_w;                                   // +0x534c 0x43a510 (the second across-dir offset); spawn_corpse fills the four as a vector4
    rgl::vector4 wp;                                    // +0x5350 0x448eb0 (the waypoint)
    float speed;                                        // +0x5360 0x448eb0
    float veer;                                         // +0x5364 0x448eb0
    Vec2 strafe;                                        // +0x5368 0x448eb0 (the local x / y of the strafe direction)
    clock2_timer timer;                                 // +0x5370 0x448eb0
    clock2_timer timer2;                                // +0x5380 0x448eb0
    unsigned strafe_state;                              // +0x5390 0x448eb0 (bit 0 = a strafe is active, 2/4 = back/forward, 8/0x10 = right/left)
    int turn_sign;                                      // +0x5394 0x448eb0 (the sign of the previous strafe turn, 0/1)
    int combat_state;                                   // +0x5398 (agent_get_combat_state 0x4a9780)
    int f539c;                                          // +0x539c (init clears it)
    int fire_result;                                    // +0x53a0 0x448eb0, 0x44d000 (fire_check's result in the ranged branch of update_ai)
    InputState in_prev;                                 // +0x53a4 (in_prev.move: the previous move direction, 0x448eb0)
    unsigned input;                                     // +0x53bc 0x4461e0 (|= 0x200), 0x448eb0, 0x44df30 (the input bits the AI asks for: 0x100 attack, 0x200 block)
    std::deque<InputRec> input_q;                       // +0x53c0 queued input (0x43c9c0); the constructor 0x475e50 runs its constructor, the agent's destructor 0x431f40 its destructor
    InputState in_last;                                 // +0x53d4
    Threat threats[16];                                 // +0x53ec (sorted by distance, nearest first; 0x443430)
    int nthreat;                                        // +0x556c
    float weave;                                        // +0x5570 0x448eb0, 0x44d000 (the weave state of the approach: +0.2 per tick, -0.03 when blocked, clamped -1..1; = 1)
    rgl::matrix scripted_destination;                   // +0x5574 (agent_set_scripted_destination 0x4a95d2; = identity)
    int scripted_mode;                                  // +0x55b4
    int always_attack_in_melee;                         // +0x55b8 (0x4a970a)
    shape_base* last_hit;                               // +0x55bc 0x448eb0
    int last_attacker;                                  // +0x55c0 id of the agent that last hit this one (take_blow sets it; = -1; update_ai 0x4465e0)
    unsigned rand_bits;                                 // +0x55c4 0x448eb0, 0x44df30 (a fresh rand() per AI tick; bit 1 jitters the aim height)
    float speed_limit;                                  // +0x55c8 m/s (agent_set_speed_limit 0x4a96d2; = 15)

    AgentAi();                                          // 0x475e50 (game/terrain/poly_mesh.cpp)
    void init();                                        // 0x463370 (game/data/troop_inventory.cpp: the reset path)
    int* pface() { return &path_face[1]; }              // +0x324: the face list find_path fills (0x445910; path_face[0] is the slot before it)
};

#endif // GAME_AGENT_AI_H
