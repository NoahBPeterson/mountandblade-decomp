// agent_rec.h: the 0x5928-byte agent record (vtable 0x591848): the id-pool slot of the mission's agent table
// (Mission::agents), its constructor (0x477a30), the reset that clears it (0x472f90), the per-frame view sub-object at
// +0x258 and the fields the mission statistics read (team +0x1c4, class counts, targets). Offsets are the ones
// read or written by a function of terrain_b.cpp (cited); the rest is padding. The slices called the record
// RecZ / MAgent / AgentReset and the view sub-object RecZSubA / TimedObj / ViewState.
#ifndef TERRAIN_B_AGENT_REC_H
#define TERRAIN_B_AGENT_REC_H
#include "../../rgl/rgl_math.h"

extern __int64 g_ticks[];                       // 0x5e2948 clock table, 1e-5 s ticks

// 8-byte tick values stored as two dwords: the agent's sub-objects are not 8-byte aligned (ViewState is 0x5374 long)
struct tick { int lo, hi; };
inline const tick& tk(int i) { return ((const tick*)g_ticks)[i]; }
struct Timed { int state; int pad; tick t; };       // 0x10: state (2 = idle) and the tick it was set
struct Timer16 {
    int clock;                  // +0 index into g_ticks
    int pad4;
    tick start;                 // +8
    void init(int c) { clock = c; start = tk(clock); }
};
#include "../../include/vec.h"
struct Vec4z { float x, y, z, w; };

// The view sub-object of an agent (0x5374 bytes at agent +0x258): five timers, the target agent id and two floats
// the team statistics write. The agent's constructor calls the timer set-up 0x475e50 (defined below as the constructor
// of ViewState, the slices' TimedObj) explicitly, after its first stores; init() 0x463370 is the reset path.
struct ViewBody {
    char pad0[0x58];
    Timer16 t58;                // +0x58
    char pad68[0x98 - 0x68];
    Timer16 t98;                // +0x98
    Timer16 ta8;                // +0xa8
    char padb8[0xc4 - 0xb8];
    int target;                 // +0xc4 (agent +0x31c) id of another agent (0x474e60)
    char padc8[0x50e8 - 0xc8];
    Vec2 v50e8;                 // +0x50e8 (agent +0x5340) (0x474e60)
    char pad50f0[0x5118 - 0x50f0];
    Timer16 t5118;              // +0x5118
    Timer16 t5128;              // +0x5128
    char pad5138[0x516c - 0x5138];
    Vec4z v516c;                // +0x516c
    char pad517c[0x5374 - 0x517c];
    void init_timers();         // 0x475e50
    void init();                // 0x463370
};
struct ViewState : ViewBody {
    ViewState();                // 0x475e50
};
// the sub-object at agent +0x563c (0x68 bytes) whose initialiser the constructor calls
struct AgentSubB {
    char b[0x68];
    AgentSubB* init();          // 0x4353a0 (returns this)
};
struct EmptySlot { virtual void slot(); };      // 0x477b70: the empty slot 1 / 2 of the IfaceB-style records

// the object at agent +0x250 (the agent's scene-side body): its slot 25 (+0x64) takes an emitter to keep
struct Emit;
struct EmitterSink {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15(); virtual void s16(); virtual void s17();
    virtual void s18(); virtual void s19(); virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23();
    virtual void s24();
    virtual void add_emitter(Emit* e);          // slot 25 (+0x64) (0x47a0c0)
};

struct MAgent {
    virtual ~MAgent();           // vtable 0x591848 (stored first by the constructor)
    virtual void slot1();       // 0x477b70 (empty)
    virtual void slot2();
    int used;                   // +0x04 0 = free slot (the pool's in-use flag)
    int id;                     // +0x08
    int serial;                 // +0x0c
    int f10;   // +0x10
    int f14;   // +0x14
    int f18;   // +0x18
    char pad1c[0x4];
    int* f20;   // +0x20
    int f24;   // +0x24
    int f28;   // +0x28
    int f2c;   // +0x2c
    int f30;   // +0x30
    int f34;   // +0x34
    int f38;   // +0x38
    int f3c;   // +0x3c
    Timed t40;   // +0x40 state (2 = idle) / +0x48 tick it was set (0x477a30, 0x472f90)
    rgl::vector4 f50;   // +0x50
    rgl::vector4 f60;   // +0x60
    int f70;   // +0x70
    int f74;   // +0x74
    float f78;   // +0x78
    int f7c;   // +0x7c
    float f80;   // +0x80
    int f84;   // +0x84
    int f88;   // +0x88
    char pad8c[0x8];
    int f94;   // +0x94
    rgl::vector4 f98;   // +0x98
    int fa8;   // +0xa8
    int fac;   // +0xac
    float fb0;   // +0xb0
    int fb4;   // +0xb4
    int fb8;   // +0xb8
    int fbc;   // +0xbc
    int fc0;   // +0xc0
    rgl::vector4 fc4;   // +0xc4
    int fd4;   // +0xd4
    int fd8;   // +0xd8
    int fdc;   // +0xdc
    int fe0;   // +0xe0
    int fe4;   // +0xe4
    int fe8;   // +0xe8
    int fec;   // +0xec
    float ff0;   // +0xf0
    float ff4;   // +0xf4
    float ff8;   // +0xf8
    int ffc;   // +0xfc
    int f100;   // +0x100
    int f104;   // +0x104
    int f108;   // +0x108
    int f10c;   // +0x10c
    int f110;   // +0x110
    int f114;   // +0x114
    int f118;   // +0x118
    float f11c;   // +0x11c
    float f120;   // +0x120
    float f124;   // +0x124
    int f128;   // +0x128
    int f12c;   // +0x12c
    int f130;   // +0x130
    int f134;   // +0x134
    int f138;   // +0x138
    int f13c;   // +0x13c
    int f140;   // +0x140
    int f144;   // +0x144
    int f148;   // +0x148
    int f14c;   // +0x14c
    int f150;   // +0x150
    int f154;   // +0x154
    int f158;   // +0x158
    int f15c;   // +0x15c
    int f160;   // +0x160
    int f164;   // +0x164
    int f168;   // +0x168
    int f16c;   // +0x16c
    int f170;   // +0x170
    int f174;   // +0x174
    int f178;   // +0x178
    int f17c;   // +0x17c
    int f180;   // +0x180
    int f184;   // +0x184
    int f188;   // +0x188
    int f18c;   // +0x18c
    int f190;   // +0x190
    char pad194[0x10];
    int f1a4;   // +0x1a4
    int f1a8;   // +0x1a8
    int f1ac;   // +0x1ac
    int f1b0;   // +0x1b0
    int f1b4;   // +0x1b4
    int f1b8;   // +0x1b8
    int f1bc;   // +0x1bc
    int f1c0;   // +0x1c0
    int team;   // +0x1c4 (0x474e60)
    char pad1c8[4];
    int head_count;   // +0x1cc (0x474e60)
    char pad1d0[0x22c - 0x1d0];
    int f22c;   // +0x22c
    int f230;   // +0x230
    int f234;   // +0x234
    int f238;   // +0x238
    char pad23c[0x14];
    EmitterSink* f250;   // +0x250
    char pad254[0x4];
    ViewBody view;   // +0x258
    char pad55cc[0x4];
    int f55d0;   // +0x55d0
    char pad55d4[0x4];
    rgl::vector4 f55d8;   // +0x55d8
    int f55e8;   // +0x55e8
    char pad55ec[0x20];
    int f560c;   // +0x560c
    int f5610;   // +0x5610
    int f5614;   // +0x5614
    char pad5618[0xc];
    rgl::vector4 f5624;   // +0x5624
    int f5634;   // +0x5634
    int f5638;   // +0x5638
    AgentSubB subB;   // +0x563c (0x477a30)
    rgl::vector4 f56a4;   // +0x56a4
    rgl::vector4 f56b4;   // +0x56b4
    int f56c4;   // +0x56c4
    int f56c8;   // +0x56c8
    int f56cc;   // +0x56cc
    int f56d0;   // +0x56d0
    int f56d4;   // +0x56d4
    int f56d8;   // +0x56d8
    int f56dc;   // +0x56dc
    float f56e0;   // +0x56e0
    float f56e4;   // +0x56e4
    char pad56e8[0x4];
    int f56ec;   // +0x56ec
    int f56f0;   // +0x56f0
    char pad56f4[0x20];
    int f5714;   // +0x5714
    int f5718;   // +0x5718
    char pad571c[0xc];
    int f5728;   // +0x5728
    int f572c;   // +0x572c
    int f5730;   // +0x5730
    char pad5734[0x80];
    float f57b4;   // +0x57b4
    char pad57b8[0x10];
    Timed t57c8;   // +0x57c8 (0x477a30)
    char f57d8;   // +0x57d8
    char pad57d9[0x7];
    Timed t57e0;   // +0x57e0 state / +0x57e8 tick
    Timed t57f0;   // +0x57f0 state / +0x57f8 tick
    Timed t5800;   // +0x5800 state / +0x5808 tick
    Timed t5810;   // +0x5810 state / +0x5818 tick
    Timed t5820;   // +0x5820 state / +0x5828 tick
    int f5830;   // +0x5830
    char f5834;   // +0x5834
    char pad5835[0x3];
    int f5838;   // +0x5838
    int f583c;   // +0x583c
    char pad5840[0x4];
    int f5844;   // +0x5844
    char pad5848[0x8];
    float f5850;   // +0x5850
    char pad5854[0x4];
    int f5858;   // +0x5858
    char pad585c[0x4];
    int f5860;   // +0x5860
    char pad5864[0x4];
    tick f5868;   // +0x5868
    char pad5870[0x4];
    int f5874;   // +0x5874
    char f5878;   // +0x5878
    char pad5879[0x3];
    int f587c;   // +0x587c
    int f5880;   // +0x5880
    char pad5884[0x40];
    char f58c4;   // +0x58c4
    char f58c5;   // +0x58c5
    char pad58c6[0x12];
    char f58d8;   // +0x58d8
    char pad58d9[0x3];
    int f58dc;   // +0x58dc
    rgl::vector4 f58e0;   // +0x58e0
    rgl::vector4 f58f0;   // +0x58f0
    int f5900;   // +0x5900
    char pad5904[0x14];
    Timed t5918;   // +0x5918 (0x477a30)
    MAgent();                    // 0x477a30
    void reset();               // 0x472f90
    int troop_class();          // 0x433140 (0 infantry, 1 archer, 2 cavalry)
    int update_nav_face();      // 0x43cb30
};

#include <stddef.h>
typedef char assert_agent_size[sizeof(MAgent) == 0x5928 ? 1 : -1];
typedef char assert_agent_team[offsetof(MAgent, team) == 0x1c4 ? 1 : -1];
typedef char assert_agent_view[offsetof(MAgent, view) == 0x258 ? 1 : -1];
typedef char assert_agent_f55e8[offsetof(MAgent, f55e8) == 0x55e8 ? 1 : -1];
typedef char assert_agent_f56b4[offsetof(MAgent, f56b4) == 0x56b4 ? 1 : -1];
typedef char assert_agent_t5918[offsetof(MAgent, t5918) == 0x5918 ? 1 : -1];
typedef char assert_view_size[sizeof(ViewBody) == 0x5374 ? 1 : -1];
typedef char assert_view_target[offsetof(ViewBody, target) == 0xc4 ? 1 : -1];
typedef char assert_agent_subB[offsetof(MAgent, subB) == 0x563c ? 1 : -1];

#endif // TERRAIN_B_AGENT_REC_H
