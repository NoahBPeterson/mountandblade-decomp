// mission.h: the mission object (global pointer g_mission, 0x5e292c) as far as terrain_b.cpp / mission.cpp read or
// write it: the scene container pointer (+0), the agent table and record pool (+4, +0x30), the mark records (+0x6b0), the
// eight teams with their three class records (+0x5280), the mission clock and spawn-rule state (+0x5de0..), the
// per-team counters (+0x621c), the terrain description with the play-area rectangle (+0x63d0), the ambient wind and
// camera handles, the prop list (+0x6688), the weather blend state (+0x67cc..) and the spawn-done flags (+0x6824).
// Offsets are cited at each field; src/game/agent/agent.h, agent3/agent_types.h and ui/mission.h declare their own
// partial views of the same object. Names provisional (the slices called it Mission, Mission4, FxMarks, Scene).
#ifndef TERRAIN_B_MISSION_H
#define TERRAIN_B_MISSION_H
#include <stddef.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"
#include "./cell_grid.h"
#include "./pool.h"
#include "./agent_rec.h"
#include "../../rgl/mesh.h"
#include "./records.h"
#include "./face.h"
#include "./game_data.h"

struct Emit;
struct SpawnSet;
struct PropKind {                               // 0x78 bytes (array at 0x5e29dc) (0x472050)
    char pad0[0x11];
    unsigned char flags;                        // +0x11
    char pad12[0x78 - 0x12];
};
extern PropKind* g_propkinds;                   // 0x5e29dc
extern int g_flag5a9fb0;                        // 0x5a9fb0
struct EntFlag { char pad0[0x140]; char flag; };         // scene entity: +0x140 visibility (0x472050)

// ---- spawn rules (0x473a60) ---------------------------------------------------------------------------------------
struct SpawnRec {                               // 0x4c bytes: a mission spawn rule
    int id;                                     // +0x00
    int flags;                                  // +0x04 (see rule_accepts / pick_spawn)
    unsigned flags2;                            // +0x08
    int padc[4];
    int f1c;                                    // +0x1c
    char pad20[0x4c - 0x20];
};
struct ModeRec2 { char pad[0x34]; SpawnRec* recs; char pad2[0x58 - 0x38]; };    // 0x58, array at 0x5e2a00
extern ModeRec2* g_modes2;                      // 0x5e2a00
int __cdecl rand_below(const int* n);           // 0x432580 (0 when *n < 2, else rand() % *n)
extern float g_thr_self;                        // 0xa1813c
extern float g_thr_other;                       // 0xa18138

// ---- per-team statistics (0x474e60) -------------------------------------------------------------------------------
struct ClassInfo {                              // 0x6c bytes, per team per troop class
    int kind;                                   // +0x00 (2 = tracked)
    char pad4[0xc];
    rgl::vector4 pos;                           // +0x10
    rgl::vector4 target;                        // +0x20
    int nav;                                    // +0x30
    rgl::vector4 dir;                           // +0x34
    char pad44[0x4c - 0x44];
    float half;                                 // +0x4c
    float count;                                // +0x50
    float total;                                // +0x54
    char pad58[4];
    rgl::vector4 sum;                           // +0x5c
};
struct Team {                                   // 0x16c bytes
    unsigned leader;                            // +0x00 agent id, -1 = none
    ClassInfo cls[3];                           // +0x04
    int relation[8];                            // +0x148
    int pad168;
};
#include "../../include/timer.h"

// ---- mark records (0x472160) ----------------------------------------------------------------------------------------
struct MarkRec {                                // 0x130 bytes; array at Mission +0x6b0 (offsets relative to the record)
    rgl::matrix m;                              // +0x00 current frame (s, f, u, o)
    char pad40[0x80 - 0x40];
    rgl::matrix m2;                             // +0x80 next frame
    int cnt2;                                   // +0xc0
    int cnt;                                    // +0xc4
    char pad_c8[0xd0 - 0xc8];
    int clock;                                  // +0xd0
    char pad_d4[0xd8 - 0xd4];
    __int64 stamp;                              // +0xd8
    float f790;                                 // +0xe0
    char pad_e4[0xe8 - 0xe4];
    face* floor;                                // +0xe8
    char pad_ec[0x130 - 0xec];
};

// ---- the terrain description at Mission +0x63d0 (0x1c8 bytes; init 0x464400 in terrain_a's object) -----------------
#include "../terrain_desc.h"                 // TerrainDesc (shared with terrain_a; init 0x464400 is in terrain_a.cpp)

struct Mission {
    CellGrid* scene;                            // +0x0000 (the slices' grid / nav / Scene)
    Pool<MAgent> agents;                         // +0x0004 (0x474e60, 0x477f00, 0x478020)
    char pad2c[0x30 - 0x2c];
    Pool<RecJ> poolJ;                           // +0x0030 (0x477f90)
    char pad58[0x5c - 0x58];
    int f5c;                                    // +0x005c
    int f60;                                    // +0x0060
    char pad64[0x1ac - 0x64];
    Emit* fog;                                  // +0x01ac emitters spawned by the last post_spawn (0x47a0c0)
    Emit* layer0;                               // +0x01b0
    Emit* layer1;                               // +0x01b4
    Emit* layer2;                               // +0x01b8
    char pad1bc[0x6b0 - 0x1bc];
    MarkRec recs[1];                            // +0x06b0 (0x472160; more than one record)
    char pad7e0[0x5280 - 0x6b0 - sizeof(MarkRec)];
    Team teams[8];                              // +0x5280 (0x474e60)
    Timer tm;                                   // +0x5de0
    int mode;                                   // +0x5df0 index into g_modes2 (0x473a60)
    int f5df4;                                  // +0x5df4 index of the active level slot
    int f5df8;                                  // +0x5df8
    std::vector<int> v5dfc;                     // +0x5dfc (retire list, 0x479160)
    int list[2][128];                           // +0x5e0c candidate party ids per side (0x473a60)
    int cnt[2];                                 // +0x620c
    int f6214;                                  // +0x6214
    int f6218;                                  // +0x6218
    int counts[8][6];                           // +0x621c (0x4759c0)
    float changed_at;                           // +0x62dc
    char pad62e0[0x63d0 - 0x62e0];
    TerrainDesc terrain;                        // +0x63d0 (0x479160; rectangle +0x653c, f180 at +0x6550)
    rgl::vector4 f6598;                         // +0x6598
    float f65a8;                                // +0x65a8 (sky object / sky time)
    float f65ac;                                // +0x65ac
    Wind* f65b0;                                // +0x65b0 ambient wind A
    Wind* f65b4;                                // +0x65b4 ambient wind B
    int f65b8;                                  // +0x65b8 camera node
    int f65bc;
    int f65c0;                                  // +0x65c0 camera node
    int f65c4;
    int f65c8;
    int f65cc;
    int f65d0;
    int f65d4;
    int f65d8;
    int f65dc;
    int f65e0;
    int f65e4;
    char pad65e8[0x6604 - 0x65e8];
    int fog_dense;                              // +0x6604 fog mode: 0 dense, 1 thin (0x47a0c0)
    int f6608;                                  // +0x6608
    int cam;                                    // +0x660c index of the camera row (0x47a0c0)
    char pad6610[0x6664 - 0x6610];
    int f6664;                                  // +0x6664
    char pad6668[0x6678 - 0x6668];
    std::vector<int> v6678;                     // +0x6678 (dead / culled list)
    std::vector<Rec1c8> props;                  // +0x6688 (first at +0x668c) (0x472050)
    char pad6698[0x672c - 0x6698];
    int f672c;                                  // +0x672c
    int f6730;
    int f6734;
    int f6738;
    int f673c;
    EntFlag* special;                           // +0x6740 (0x472050; reset clears it)
    int f6744;
    void* f6748;                                // +0x6748 camera record
    rgl::matrix frame_a;                        // +0x674c
    char pad678c[0x67cc - 0x678c];
    float blend_from;                           // +0x67cc
    float blend_to;                             // +0x67d0
    float blend_time;                           // +0x67d4
    char pad67d8[0x67e8 - 0x67d8];
    int f67e8;                                  // +0x67e8 (frame clock origin, -1 = unset)
    char pad67ec[0x67f0 - 0x67ec];
    int blend_n;                                // +0x67f0
    float blend2_from;                          // +0x67f4
    char pad67f8[0x6810 - 0x67f8];
    float blend2_time;                          // +0x6810
    tstring str;                                // +0x6814
    char done[64];                              // +0x6824 spawn-done flags per rule (0x473a60)

    void update_prop_flags();                   // 0x472050
    void update_team_stats();                   // 0x474e60 (fastcall)
    void update_team_counts();                  // 0x4759c0 (fastcall)
    void replace_mark(int idx);                 // 0x472160
    bool rule_accepts(SpawnRec* a, Stack* b, int p3, char p4, char p5, char p6);   // 0x467940 (thiscall, ret 0x18, ignores this)
    void pick_spawn(int idx, int* out_party, int* out_stack, int* out_troop, int* out_side, int* out_flag, int* out_seed);   // 0x473a60
    void reset();                               // 0x479160
    void setup_ambient(int level_idx, int spec_idx, float t);                  // 0x4792d0
    void rebuild_nav(int which, int);           // 0x479e20
    void post_spawn(MAgent* a);                  // 0x47a0c0
    void build_interior(int which, rgl::mesh** out, int* n);   // 0x47a800
};
extern Mission* g_mission;                      // 0x5e292c

template<> void Pool<MAgent>::next_used(unsigned* i);    // 0x43b160 (the agent table's iterator, in another object)

#define MISSION_OFFSET_CHECK(m, off) typedef char Mission_##m##_at[(offsetof(Mission, m) == (off)) ? 1 : -1]
MISSION_OFFSET_CHECK(agents, 0x4);
MISSION_OFFSET_CHECK(poolJ, 0x30);
MISSION_OFFSET_CHECK(recs, 0x6b0);
MISSION_OFFSET_CHECK(teams, 0x5280);
MISSION_OFFSET_CHECK(tm, 0x5de0);
MISSION_OFFSET_CHECK(mode, 0x5df0);
MISSION_OFFSET_CHECK(v5dfc, 0x5dfc);
MISSION_OFFSET_CHECK(list, 0x5e0c);
MISSION_OFFSET_CHECK(counts, 0x621c);
MISSION_OFFSET_CHECK(terrain, 0x63d0);
MISSION_OFFSET_CHECK(f6598, 0x6598);
MISSION_OFFSET_CHECK(f65b0, 0x65b0);
MISSION_OFFSET_CHECK(f65e4, 0x65e4);
MISSION_OFFSET_CHECK(fog_dense, 0x6604);
MISSION_OFFSET_CHECK(f6664, 0x6664);
MISSION_OFFSET_CHECK(v6678, 0x6678);
MISSION_OFFSET_CHECK(props, 0x6688);
MISSION_OFFSET_CHECK(f672c, 0x672c);
MISSION_OFFSET_CHECK(special, 0x6740);
MISSION_OFFSET_CHECK(frame_a, 0x674c);
MISSION_OFFSET_CHECK(blend_from, 0x67cc);
MISSION_OFFSET_CHECK(f67e8, 0x67e8);
MISSION_OFFSET_CHECK(blend2_time, 0x6810);
MISSION_OFFSET_CHECK(str, 0x6814);
MISSION_OFFSET_CHECK(done, 0x6824);

#endif // TERRAIN_B_MISSION_H
