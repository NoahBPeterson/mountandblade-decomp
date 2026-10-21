// Shared declarations of slices s0043c9c0 / s0043d040 / s0043db60 (Agent navigation, input queue, animation
// blend). Copied into each slice dir under the same name; keep identical. Names are provisional; every offset
// comes from the member functions at 0x43c9c0..0x43e575.
#ifndef MB_AGENT_NAV_H
#define MB_AGENT_NAV_H
#include <stddef.h>
#include "rgl.h"
using rgl::vector4;

// ---- navigation mesh (see s00439100) ------------------------------------------------------------------
struct NavMesh {
    bool inside(const int* face, const float* p);       // 0x4391d0
    bool trace(int face, vector4 from, int end_face, const float* to, int* max_steps);   // 0x439460 (vector4 by value)
};
struct Scene {                                           // *g_mission
    char pad0[0xb4];
    NavMesh* nav;                                        // +0xb4
    int locate(const vector4* pos);                      // 0x439a50 face containing/near pos
    int locate_near(const vector4* pos, int* hint, int* mode);   // 0x43abf0
};

// ---- animation tables ------------------------------------------------------------------------------------
struct SubAnim {                                         // 0x40 bytes (AnimCmd of s0043a280)
    char pad0[8];
    unsigned flags;                                      // +8 (0x10000000 = ...)
    char pad0c[0x2c - 0xc];
    float v2c, v30, v34, v38;                            // +0x2c (vector4)
    float offset;                                        // +0x3c
};
struct AnimDef {                                         // 0x30 bytes
    char pad0[0x10];
    SubAnim* subs;                                       // +0x10
    char pad14[4];
    unsigned flags;                                      // +0x18
    char pad1c[0x2c - 0x1c];
    int f2c;                                             // +0x2c
};

struct Channel {                                         // 0xd8 bytes (Chan of s0043a280)
    char pad0[8];
    unsigned flags;                                      // +8
    int fc;                                              // +0xc
    unsigned mask_lo, mask_hi;                           // +0x10 / +0x14 (bone mask bits)
    char pad18[0x3c - 0x18];
    int f3c;                                             // +0x3c
    char pad40[0x68 - 0x40];
    float* xform;                                        // +0x68 (quaternion blocks at +0, +0x70, +0x80, ...)
    float v6c[4];                                        // +0x6c
    char pad7c[0xc0 - 0x7c];
    float w0, w1;                                        // +0xc0 / +0xc4
    char padc8[0xcc - 0xc8];
    int rate;                                            // +0xcc
    char padd0[0xd8 - 0xd0];
    bool busy();                                         // 0x433840 (ecx = this)
    void f433930(int a);                                 // 0x433930
};
struct Skeleton {                                        // *(*(agent+0x250)+0x304)  (Skel of s0043a280)
    int nrecs;                                           // +0
    char* recs;                                          // +4 (0x1dc-byte records)
    int* sel;                                            // +8 -> ints at +0x28/+0x2c/+0x30
    char pad0c[0x40 - 0xc];
    Channel* chans;                                      // +0x40
    void set_rec_phase(int i, float p);                  // 0x433b20
};
struct AgentBody { char pad0[0x58]; float dx, dy, dz; char pad64[0x304 - 0x64]; Skeleton* skel; };

// ---- agent anim slot (2 x 0x2c bytes at agent+0xd4) -------------------------------------------------------
struct AnimSlot {
    int cur;                                             // +0
    int next;                                            // +4
    int sub;                                             // +8
    unsigned prev_mask;                                  // +0xc
    int f10;                                             // +0x10
    unsigned events;                                     // +0x14
    float scale;                                         // +0x18
    int f1c;                                             // +0x1c
    float rate;                                          // +0x20
    float phase;                                         // +0x24 (>= 0: pending phase for set_rec_phase)
    int f28;                                             // +0x28
};

struct InputRec { unsigned bits; char pad[0x14]; union { __int64 time; struct { unsigned lo; int hi; }; }; };
   // queued input record (+0x18 time)
struct InputState { unsigned v[6]; };

struct Agent;
struct Mission {
    Scene* scene;                                        // +0
    char pad4[0x1c - 4];
    Agent** pages;                                // +0x1c (page table: id >> 4; 16 agents per page)
    char pad20[0x653c - 0x20];
    float min_x, min_y, max_x, max_y;                    // +0x653c (play-area bounds)
    char pad654c[0x6698 - 0x654c];
    NavMesh nav;                                         // +0x6698
};
extern Mission* g_mission;                               // 0x5e292c
extern __int64 g_ticks[];                                // 0x5e2948 (g_ticks[2] = now at 0x5e2958)

struct Agent {                                           // 0x5928 bytes
    char pad0[0x10];
    int f10;                                             // +0x10
    char pad14[0x2c - 0x14];
    int id;                                              // +0x2c
    char pad30[0x50 - 0x30];
    vector4 pos;                                         // +0x50
    float fwd_x, fwd_y, fwd_z;                           // +0x60
    char pad6c[0xc4 - 0x6c];
    vector4 nav_pos;                                     // +0xc4
    AnimSlot slot[2];                                    // +0xd4
    AnimDef** lib;                                       // +0x12c (*lib)[anim] = animation definition
    char pad130[0x250 - 0x130];
    AgentBody* body;                                     // +0x250
    char pad254[0x25c - 0x254];
    vector4 goal;                                        // +0x25c
    vector4 goal_dir;                                    // +0x26c
    int goal_face;                                       // +0x27c
    char pad280[0x284 - 0x280];
    int goal_changed;                                    // +0x284
    char pad288[0x318 - 0x288];
    int f318;                                            // +0x318
    char pad31c[0x320 - 0x31c];
    int path_face[0x405];                                // +0x320
    vector4 path_pos[0x3ff];                             // +0x1334
    int path_active;                                     // +0x5324
    int path_cur;                                        // +0x5328
    int path_count;                                      // +0x532c
    char path_flag;                                      // +0x5330
    char pad5331[0x5338 - 0x5331];
    int path_state;                                      // +0x5338
    char pad533c[0x53a4 - 0x533c];
    InputState in_prev;                                  // +0x53a4
    char pad53bc[0x53c4 - 0x53bc];
    InputRec** q_buf;                                    // +0x53c4
    unsigned q_cap, q_head, q_count;                     // +0x53c8
    InputState in_last;                                  // +0x53d4
    char pad53ec[0x55e0 - 0x53ec];
    float dir_z;                                         // +0x55e0
    char pad55e4[0x5640 - 0x55e4];
    int target_id;                                       // +0x5640
    char pad5644[0x5664 - 0x5644];
    float move_x, move_y, move_z;                        // +0x5664
    char pad5670[0x5674 - 0x5670];
    int weapon_kind;                                     // +0x5674
    char pad5678[0x56c8 - 0x5678];
    float f56c8, f56cc;                                  // +0x56c8
    char pad56d0[0x56e4 - 0x56d0];
    float f56e4;                                         // +0x56e4
    char pad56e8[0x587c - 0x56e8];
    int nav_face;                                        // +0x587c
    int disabled;                                        // +0x5880
    char pad5884[0x5928 - 0x5884];

    void poll_input(int unused, InputState* out);         // 0x43c9c0 (ret 8)
    int update_nav_face();                               // 0x43cb30
    int move_anim(int mode);                             // 0x43cc30
    int pick_slot_anim(int slot);                        // 0x433040
    void start_channel(int idx, SubAnim* cmd, unsigned* in_flags, float* base);   // 0x43a980 (ret 0x10)
    void update_anims(int unused);                       // 0x43d040 (ret 4)
    void set_goal_agent(unsigned id);                    // 0x43db90
    void set_goal_point(vector4 p, int face);            // 0x43dcb0
    void set_goal_near(const vector4* center, const float* radius);   // 0x43dd70
    void advance_path();                                 // 0x43df60
};

#endif
