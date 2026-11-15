// agent_rec.h: the 0x5928-byte agent record (vtable 0x591848): the id-pool slot of the mission's agent table
// (Mission::agents), its constructor (0x477a30), the reset that clears it (0x472f90), the per-frame view sub-object at
// +0x258 and the fields the mission statistics read (team +0x1c4, class counts, targets). Offsets are the ones
// read or written by a function of terrain_b.cpp (cited); the rest is padding. The slices called the record
// RecZ / MAgent / AgentReset and the view sub-object RecZSubA / TimedObj / ViewState.
#ifndef TERRAIN_B_AGENT_REC_H
#define TERRAIN_B_AGENT_REC_H
#include "../../rgl/rgl_math.h"


#include "../view_state.h"           // tick, tk, Timer16, Vec4z, ViewBody, ViewState (shared with ui/troop_inv)
struct Timed {                                      // 0x10: state (2 = idle) and the tick it was set
    int state; int pad; tick t;
    Timed() { state = 2; t = tk(2); }
};
struct TimedFlag : Timed {                          // a Timed and its armed flag (+0x57c8)
    char flag;
    TimedFlag() { flag = 0; }
};
struct ClockStamp {                                 // +0x5860: clock index 0 and the tick it was set
    int state; int pad; tick t;
    ClockStamp() { state = 0; t = tk(0); }
};
#include "../blow.h"                 // Blow: the last blow at agent +0x563c, whose initialiser (0x4353a0) the constructor calls
struct EmptySlot { virtual void slot(); };      // 0x477b70: the empty slot 1 / 2 of the IfaceB-style records

// the object at agent +0x250 (the agent's scene-side body): its slot 25 (+0x64) takes an emitter to keep
namespace rgl { struct psys; }
struct EmitterSink {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15(); virtual void s16(); virtual void s17();
    virtual void s18(); virtual void s19(); virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24();
    virtual void add_emitter(rgl::psys* e);          // slot 25 (+0x64) (0x47a0c0)
};

#include "../../include/iface.h"       // IfaceB
#include <vector>
struct IdVec : std::vector<int> {
    void tidy() { _Tidy(); }
};
struct BlowInit : Blow { BlowInit() { init(); } };   // the blow member's constructor is init 0x4353a0
struct DefList; struct node_groups; struct Meter; struct AgentFx; struct NewtonBody; struct Obj_a0;
struct MAgent : IfaceB {
    // vtable 0x591848 (stored by the constructor). There is no destructor in source: the implicit one destroys the members
    // in reverse order, the view's Tail2 at +0x53c0 (~Tail2 0x4c4d10) and then the id vector at +0x1c, and ends with the base's
    // vptr store; slot 0 is its vector deleting destructor 0x431f40 (??_EMAgent, emitted with this class's vtable).
    // IfaceB (vtable 0x5908f8, src/include/iface.h) is the base: its slots 1 / 2 (the empty function 0x477b70) are the table's, its words
    // f4 / f8 / fc are the pool's in-use flag (0 = free slot), id and serial; ~IfaceB stores the base vptr at the end of the vector deleting
    // destructor 0x431f40, which is why that function ends with `mov [edi], 0x5908f8`.
    // Member names are Agent's (game/agent_obj.h) at the same offsets; offsets Agent leaves unnamed keep an offset name.
    int kind;   // +0x10 0 = human, 1 = horse
    int state;   // +0x14 1 = alive
    int f18;   // +0x18
    IdVec ids;   // +0x1c vector<int> (first +0x20): freed by reset 0x472f90 (Agent::slot_vars)
    int horse;   // +0x2c mount id, -1 = on foot
    int rider;   // +0x30
    int stand_animation;   // +0x34
    int walk_forward_animation;   // +0x38
    int flags3c;   // +0x3c
    Timed t40;   // +0x40 state (2 = idle) / +0x48 tick it was set (0x477a30, 0x472f90)
    rgl::vector4 pos;   // +0x50
    rgl::vector4 vel;   // +0x60
    int f70;   // +0x70 (Agent::rot: stored as ints by reset)
    int f74;   // +0x74
    float f78;   // +0x78
    int f7c;   // +0x7c
    float scale;   // +0x80
    int floor_blend;   // +0x84 (cleared as an int by reset; Agent reads it as a float)
    int troop;   // +0x88
    char pad8c[0x8];
    int f94;   // +0x94
    rgl::vector4 v98;   // +0x98
    int vel_xy[2];   // +0xa8 (Agent::vel_xy; reset assigns it a whole Vec2)
    float fb0;   // +0xb0
    int gait_ratio;   // +0xb4 (cleared as an int)
    int gait;   // +0xb8
    struct Obj_a0* fbc;   // +0xbc (Agent: the floor face)
    int fc0;   // +0xc0
    rgl::vector4 nav_pos;   // +0xc4
    struct {                // +0xd4 Agent::anim (AnimSlot, 0x2c bytes each), as reset stores it
        int cur, next, sub, prev_mask, f10, events, scale;
        float rate_scale, rate, phase;
        int f28;
    } anim[2];
    struct DefList* defs;   // +0x12c
    int weapon_flags;   // +0x130
    int f134;   // +0x134
    int f138;   // +0x138
    struct node_groups* slot_mesh[4];   // +0x13c
    struct node_groups* links[4];   // +0x14c
    struct node_groups* B[4];   // +0x15c
    struct node_groups* disp[4];   // +0x16c
    struct node_groups* slot_mesh4;   // +0x17c
    struct Meter* meters[4];   // +0x180
    struct AgentFx* fxp;   // +0x190
    char pad194[0x10];
    int f1a4;   // +0x1a4
    int f1a8;   // +0x1a8
    rgl::psys* fx1ac;   // +0x1ac
    rgl::psys* fx1b0;   // +0x1b0
    rgl::psys* fx1b4;   // +0x1b4
    rgl::psys* fx1b8;   // +0x1b8
    int party;   // +0x1bc
    int f1c0;   // +0x1c0
    int team;   // +0x1c4 (0x474e60)
    char pad1c8[4];
    int head_count;   // +0x1cc (0x474e60)
    char pad1d0[0x22c - 0x1d0];
    int wield[2];   // +0x22c
    int wield_prev[2];   // +0x234
    char pad23c[0x14];
    EmitterSink* body;   // +0x250
    char pad254[0x4];
    ViewState view;   // +0x258
    char pad55cc[0x4];
    int in_flags;   // +0x55d0
    char pad55d4[0x4];
    rgl::vector4 facing;   // +0x55d8 (Agent: fx, fy, fz, fw)
    int f55e8;   // +0x55e8
    char pad55ec[0x20];
    int active;   // +0x560c
    int f5610;   // +0x5610
    int f5614;   // +0x5614
    char pad5618[0xc];
    rgl::vector4 prev_facing;   // +0x5624
    int aim_error;   // +0x5634 (cleared as an int)
    int has_blow;   // +0x5638
    BlowInit last_blow;   // +0x563c (0x477a30)
    rgl::vector4 impulse;   // +0x5694
    rgl::vector4 v56a4;   // +0x56a4
    rgl::vector4 v56b4;   // +0x56b4
    int variant;   // +0x56c4
    int cur;   // +0x56c8 (cleared as an int)
    int pitch;   // +0x56cc (cleared as an int)
    int f56d0;   // +0x56d0
    int weapon_mode;   // +0x56d4
    int f56d8;   // +0x56d8
    int f56dc;   // +0x56dc
    float prev_scale0;   // +0x56e0
    float f56e4;   // +0x56e4
    char pad56e8[0x4];
    int f56ec;   // +0x56ec
    int f56f0;   // +0x56f0
    char pad56f4[0x20];
    int f5714;   // +0x5714
    int entry_no;   // +0x5718
    char pad571c[0xc];
    int kill_count;   // +0x5728
    int wounded_count;   // +0x572c
    int xp_pool;   // +0x5730
    char pad5734[0x80];
    float charge_ramp;   // +0x57b4
    char pad57b8[0x10];
    TimedFlag t57c8;   // +0x57c8 (0x477a30), armed flag +0x57d8
    char pad57dc[0x4];
    Timed tm;   // +0x57e0 state / +0x57e8 tick
    Timed timer;   // +0x57f0 state / +0x57f8 tick
    Timed t5800;   // +0x5800 state / +0x5808 tick
    Timed t5810;   // +0x5810 state / +0x5818 tick
    Timed t5820;   // +0x5820 state / +0x5828 tick (Agent: clock, deadline)
    int f5830;   // +0x5830
    char queued;   // +0x5834
    char pad5835[0x3];
    int mount_id;   // +0x5838
    struct NewtonBody* mount_body;   // +0x583c
    char pad5840[0x4];
    int f5844;   // +0x5844
    char pad5848[0x8];
    float f5850;   // +0x5850 (Agent::spawn.z)
    char pad5854[0x4];
    int channel;   // +0x5858 FMOD channel
    char pad585c[0x4];
    ClockStamp clk5860;   // +0x5860 (Agent: stamp_clock, stamp)
    char pad5870[0x4];
    int f5874;   // +0x5874
    char need_name;   // +0x5878
    char pad5879[0x3];
    int nav_face;   // +0x587c
    int disabled;   // +0x5880
    char pad5884[0x40];
    char f58c4;   // +0x58c4
    char f58c5;   // +0x58c5
    char pad58c6[0x12];
    char f58d8;   // +0x58d8
    char pad58d9[0x3];
    int phys_idx;   // +0x58dc
    rgl::vector4 anchor;   // +0x58e0
    rgl::vector4 hit_vec;   // +0x58f0
    int hit_bone;   // +0x5900
    char pad5904[0x14];
    Timed t5918;   // +0x5918 (0x477a30)
    MAgent();                    // 0x477a30
    void reset();               // 0x472f90
    int agent_class();          // 0x433140 (0 infantry, 1 archer, 2 cavalry; forwards to AgentX::agent_class, defined in mission.h)
    int update_nav_face();      // 0x43cb30 (forwards to Agent::update_nav_face, defined in mission.h)
};

#include <stddef.h>
typedef char assert_agent_size[sizeof(MAgent) == 0x5928 ? 1 : -1];
typedef char assert_agent_team[offsetof(MAgent, team) == 0x1c4 ? 1 : -1];
typedef char assert_agent_view[offsetof(MAgent, view) == 0x258 ? 1 : -1];
typedef char assert_agent_f55e8[offsetof(MAgent, f55e8) == 0x55e8 ? 1 : -1];
typedef char assert_agent_v56b4[offsetof(MAgent, v56b4) == 0x56b4 ? 1 : -1];
typedef char assert_agent_t5918[offsetof(MAgent, t5918) == 0x5918 ? 1 : -1];
typedef char assert_view_size[sizeof(ViewBody) == 0x5374 ? 1 : -1];
typedef char assert_view_target[offsetof(ViewBody, target) == 0xc4 ? 1 : -1];
typedef char assert_agent_last_blow[offsetof(MAgent, last_blow) == 0x563c ? 1 : -1];

#endif // TERRAIN_B_AGENT_REC_H
