// mission.h: the mission object (global pointer g_mission, 0x5e292c) with the per-team records (TeamRec / ClassInfo) and the
// mark records (MarkRec) it embeds. ONE definition for src/game: agent/agent.cpp (mode flag, team reads), agent2 (agent_stats,
// agent_ai: relations, nav mesh, play-area rectangle), agent3 (agent_combat, agent_fx_sound: dead list, effects, clocks, view
// frame), terrain_b (mission.cpp, terrain_b.cpp: reset, spawn, team statistics, props, weather blend) and ui (mission_ui.cpp:
// replay frames, per-team counters, combatant slots). Each module declared the fields it read; the union is below, in offset
// order, every field with the address of a function that proves it. Names provisional (the slices called it Mission, Mission4,
// FxMarks, Scene and, in script_interpreter.h, MissionX, which stays a separate view of the script interpreter's names).
//
// Views that differ between modules sit in anonymous unions of pointers: the scene container at +0 is `scene` for terrain_b
// (CellGrid), `scene_world` for agent3 (world), `scene_level` for agent2 (level) and `scene_L` for agent (Level). The agent table at +4
// is AgentTable (agent_obj.h); terrain_b reads the same bytes as Pool<MAgent> (agent_pool() in terrain_b/mission.h), and the record pool at +0x30
// as Pool<RecJ> (pool_j()). Needs rgl::vector4 / rgl::matrix (rgl_base.h picks the module's own).
#ifndef GAME_MISSION_H
#define GAME_MISSION_H
#include <stddef.h>
#include <vector>
#include "../include/rgl_base.h"
#include "../include/mat4.h"
#include "../include/timer.h"
#include "./tstring.h"
#include "./terrain_desc.h"
#include "./nav_mesh.h"
#include "./agent_obj.h"

struct CellGrid;                                // terrain_b/cell_grid.h: the scene container terrain_b sees
struct world;                                   // world.h: the scene as agent3 sees it
struct level;                                   // agent2/agent_types.h: the scene as agent2 sees it
struct Level;                                   // agent/agent.h: the scene as agent sees it
struct Emit;
struct Wind;
struct Rec1c8;
struct PlacedObj;                                  // the 0x1c8-byte prop record (terrain_b/records.h, agent3/agent_combat.h)
struct MAgent;                                  // terrain_b/agent_rec.h: the agent slot as the pool sees it
struct RecJ;
struct SpawnRec;
struct Stack;
struct EntFlag;
namespace rgl { struct mesh; }

// ---- per-team records (0x16c bytes, array at +0x5280): leader, three per-class blocks, the team x team relations ------------
struct ClassInfo {                              // 0x6c bytes, per team per troop class
    int kind;                                   // +0x00 (2 = tracked; 0x48aac0 switch)
    int k4;                                     // +0x04 (0x48ab30)
    int a;                                      // +0x08 game option per class (0x437f00, 0x48ab90)
    int b;                                      // +0x0c (0x437f00, 0x48abf0)
    rgl::vector4 pos;                           // +0x10 (0x43a510)
    rgl::vector4 target;                        // +0x20
    int nav;                                    // +0x30
    rgl::vector4 dir;                           // +0x34 (dir.x, dir.y: the hand offsets of 0x43a510)
    int use;                                    // +0x44 slot in use (0x48a9e0..0x48ac50, listening: 0x4a9a6c)
    int saved_use;                              // +0x48 saved copy of use (0x48a9e0, 0x48aa20)
    float half;                                 // +0x4c
    float count;                                // +0x50
    float total;                                // +0x54
    int mode;                                   // +0x58 (0x43a510)
    rgl::vector4 sum;                           // +0x5c
};
struct TeamRec {                                // 0x16c bytes
    int leader;                                 // +0x00 agent id, -1 = none (0x435b60, agent_set_team 0x4a99af)
    ClassInfo cls[3];                           // +0x04
    int relation[8];                            // +0x148 relation to team b: -1 hostile, 1 friend (0x43b300, 0x435b90, 0x4a9d5d)
    int pad168;
    void set_order_position(const rgl::vector4* p, const int* face, const rgl::vector4& offset);   // 0x4898e0 (ret 0xc)
    rgl::vector4 order_position();              // 0x489890 (ret 4: hidden return)
};
typedef char TeamRec_size_check[sizeof(TeamRec) == 0x16c ? 1 : -1];

// ---- mark records (0x472160) ----------------------------------------------------------------------------------------
struct face;
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

struct Mission {
    union {
        CellGrid* scene;                        // +0x0000 terrain_b (the slices' grid / nav / Scene)
        world* scene_world;                     //   agent3 (0x440e30, 0x442ca0)
        level* scene_level;                     //   agent2 (0x43cb30, 0x43dd70)
        Level* scene_L;                         //   agent (0x4417d0)
    };
    AgentTable agents;                          // +0x0004 (0x43b160, 0x43b1b0, 0x43b1f0; terrain_b reads it as Pool<MAgent>: agent_pool() in terrain_b/mission.h)
    char pad2c[0x30 - 0x2c];
    char poolJ[0x28];                           // +0x0030 Pool<RecJ> (terrain_b/pool.h: pool_j(); reset 0x477f90)
    char pad58[0x5c - 0x58];
    union {
        int f5c;                                // +0x005c (0x435f10 compares it with Agent::f8)
        int player_id;                          //   agent3 (0x43fd10, 0x440070, 0x443730, 0x443870)
    };
    int f60;                                    // +0x0060
    char pad64[0x1ac - 0x64];
    Emit* fog;                                  // +0x01ac emitters spawned by the last post_spawn (0x47a0c0)
    Emit* layer0;                               // +0x01b0
    Emit* layer1;                               // +0x01b4
    Emit* layer2;                               // +0x01b8
    char pad1bc[0x6b0 - 0x1bc];
    MarkRec recs[1];                            // +0x06b0 (0x472160; more than one record)
    char pad7e0[0x5280 - 0x6b0 - sizeof(MarkRec)];
    TeamRec teams[8];                           // +0x5280 (0x474e60, 0x435b90, 0x43b300, 0x48a9e0)
    Timer tm;                                   // +0x5de0
    int mode;                                   // +0x5df0 index into g_modes2 (0x473a60, 0x435840)
    int f5df4;                                  // +0x5df4 index of the active level slot (0x4435c0)
    int f5df8;                                  // +0x5df8
    std::vector<int> v5dfc;                     // +0x5dfc (retire list, 0x479160)
    int list[2][128];                           // +0x5e0c candidate party ids per side (0x473a60)
    int cnt[2];                                 // +0x620c
    int f6214;                                  // +0x6214
    int f6218;                                  // +0x6218
    int counts[8][6];                           // +0x621c per-team counters (0x4759c0, 0x48a8b0, 0x48a910; 0x435b90 reads [t][4], [t][5])
    float changed_at;                           // +0x62dc
    int dismount_count;                         // +0x62e0 riders thrown off the player's horse (at most 4 per mission; 0x457b40)
    char pad62e4[0x62e8 - 0x62e4];
    int time_idx;                               // +0x62e8 clock index (0x444370)
    char pad62ec[4];
    __int64 time_base;                          // +0x62f0 (0x444370)
    char pad62f8[0x6328 - 0x62f8];
    int clock_index;                            // +0x6328 (0x48a860)
    int pad632c;
    __int64 clock_start;                        // +0x6330
    char clock_set;                             // +0x6338
    char pad6339[0x63d0 - 0x6339];
    TerrainDesc terrain;                        // +0x63d0 (0x479160; rectangle +0x653c: 0x48a990, 0x43cb30; base_z +0x654c: 0x4435c0)
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
    int cam;                                    // +0x660c index of the camera row (0x47a0c0, 0x442ca0)
    float f6610;                                // +0x6610 (agent4/agent_think.cpp 0x44e2b0: the view distance scale of the aim error)
    char pad6614[0x6624 - 0x6614];
    posm view;                                  // +0x6624 (0x43f970, 0x440070)
    int f6664;                                  // +0x6664
    char pad6668[0x6678 - 0x6668];
    std::vector<Agent*> dead;                   // +0x6678 (first at +0x667c; dead / culled list: 0x442dc0, terrain_b reset 0x479160)
    std::vector<Rec1c8> props;                  // +0x6688 (first at +0x668c) (0x472050)
    NavMesh nav;                                // +0x6698 navigation mesh (agent2: 0x43cb30, 0x43dd70, 0x43df60)
    char pad66d0[0x672c - 0x6698 - sizeof(NavMesh)];
    int f672c;                                  // +0x672c
    int f6730;
    int f6734;
    int f6738;
    int f673c;
    EntFlag* special;                           // +0x6740 (0x472050; reset clears it)
    int f6744;
    void* f6748;                                // +0x6748 camera record
    rgl::matrix frame_a;                        // +0x674c replay frame A (0x48a420)
    rgl::matrix frame_b;                        // +0x678c replay frame B
    float blend_from, blend_to, blend_time;     // +0x67cc (0x48a740)
    int origin;                                 // +0x67d8 clock index of the blend
    char pad67dc[4];
    __int64 start;                              // +0x67e0
    int f67e8;                                  // +0x67e8 (frame clock origin, -1 = unset)
    char pad67ec[4];
    int blend_n;                                // +0x67f0 easing steepness
    float blend2_from, blend2_to;               // +0x67f4 (0x48a7d0)
    char pad67fc[0x6800 - 0x67fc];
    int origin2;                                // +0x6800
    char pad6804[4];
    __int64 start2;                             // +0x6808
    float blend2_time;                          // +0x6810
    tstring str;                                // +0x6814
    char done[64];                              // +0x6824 spawn-done flags per rule (0x473a60)

    // ---- agent / agent3 ----
    char mode_flag1() const;                    // 0x435840
    int* rel_flat() { return (int*)&teams[0].relation[0]; }       // the relation matrix as the agent code indexes it: [a * 0x5b + b]
    int* leader_flat() { return (int*)&teams[0].leader; }         // the team leaders, stride 0x5b ints (0x435b60)
    bool friends(int a, int b) const
    {
        if (a >= 0 && a < 8 && b >= 0 && b < 8)
            return ((const int*)&teams[0].relation[0])[a * 0x5b + b] == 1;
        return false;
    }
    void queue_dead(Agent* a);                  // 0x442dc0 (agent_fx_sound.cpp)
    Emit* spawn_effect(int idx, const rgl::mat4* m, int duration);   // 0x442ca0 (agent_fx_sound.cpp)

    // ---- agent4 (agent_think.cpp) ----
    int spawn_missile(const rgl::vector4* pos, const rgl::vector4* dir, ItemRef* launcher, ItemRef* ammo, unsigned shooter_id);   // 0x44be50
    void spawn_prop(int* idx, int* id);         // 0x44c210
    void init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const rgl::matrix* m);   // 0x435860 (ret 0x14; the call sites here load ecx = the mission, unused by the body)
    void blood_burst(float x, float y, float z, int unused, float amount);   // 0x4525a0 (ret 0x14)
    int add_placed(const int* kind, const int* id, const int* value, const rgl::matrix* m, int* spawn_id);   // 0x44f870

    // ---- terrain_b ----
    void update_prop_flags();                   // 0x472050
    void update_team_stats();                   // 0x474e60 (fastcall)
    void update_team_counts();                  // 0x4759c0 (fastcall)
    void replace_mark(int idx);                 // 0x472160
    bool rule_accepts(SpawnRec* a, Stack* b, int p3, char p4, char p5, char p6);   // 0x467940 (thiscall, ret 0x18, ignores this)
    void pick_spawn(int idx, int* out_party, int* out_stack, int* out_troop, int* out_side, int* out_flag, int* out_seed);   // 0x473a60
    void reset();                               // 0x479160
    void destroy();                             // 0x4d51d0 (the destructor wrapper: tears down the sub-objects at +0x4, +0x30, +0x5e00.. +0x6814; options_panel/hud.cpp)
    void setup_ambient(int level_idx, int spec_idx, float t);                  // 0x4792d0
    void rebuild_nav(int which, int);           // 0x479e20
    void post_spawn(MAgent* a);                 // 0x47a0c0
    void build_interior(int which, rgl::mesh** out, int* n);   // 0x47a800

    // ---- ui (mission_ui.cpp) ----
    TeamRec* rec(int i) { return (TeamRec*)((char*)this + 0x5280 + i * 0x16c); }   // team i's record, addressed as the ui accessors do
    void get_frame(rgl::matrix* out);           // 0x48a420
    float blend_param();                        // 0x48a740
    float blend2_param();                       // 0x48a7d0
    void start_clock(int v);                    // 0x48a860
    int team_count();                           // 0x48a8b0
    int last_team();                            // 0x48a910
    bool point_in_rect(const float* p);         // 0x48a990
    void slot_finish(int i);                    // 0x48a9e0
    void slot_restore(int i);                   // 0x48aa20
    void slot_set(int i, int s);                // 0x48aa60
    int active_kind(int i);                     // 0x48aac0
    int slot_mode(int i);                       // 0x48ab30
    int slot_kind_a(int i);                     // 0x48ab90
    int slot_kind_b(int i);                     // 0x48abf0
    // ---- src/game/mission_ui2 (the scene props at +0x668c, 0x1c8-byte Rec1c8 records) ----
    void retag_props(int from, int to);         // 0x48ed60 (ret 8: every intact prop named like tag `from` is renamed to tag `to`)
    int count_props(int tag);                   // 0x48ee20 (ret 4: intact props of tag `tag`)
    int find_prop(int tag, int nth);            // 0x48ee90 (ret 8: index of the nth intact prop of tag `tag`, -1 when none)
    // ---- src/game/agent5 (agent_hud.cpp) ----
    void init_agent(Agent* a);                  // 0x4f7d70 (ret 4: pick the agent's equipment slots, refresh its look, re-wield the hand slots)
    void refresh_agent(Agent* a);               // 0x4f7e00 (ret 4; this unused: rebuild the agent's appearance and equipment bodies)
    void camera_to(const rgl::matrix* eye, const float* far_plane);   // 0x4fa790 (ret 8: store the camera frame, move the listener, the sun and the flare)
    void apply_sun_light(const rgl::vector4* pos, const float* scale);   // 0x4fb180 (ret 8: the per-agent sun intensity and fade)
    // ---- src/game/mesh_slot (mesh_start.cpp: Mission::start_setup addresses the object by displacement) ----
    template<class T> T& at(int off) { return *(T*)((char*)this + off); }
    void start_setup();                         // 0x484590 (the per-mission start-up sequence: level + mode, weather, sky/cloud/fog bits, sun, the 64-slot light table, props and spawns)
    void copy_slot(int k);                      // 0x4676e0 (terrain_a/misc.h: copies a mark slot)
    void add_amount(int i, unsigned n);         // 0x4677a0 (terrain_a: spawn-rule amount; the script calls it add_visitors)
    void reset_spawned(int id);                 // 0x4739f0 (terrain_b/terrain_b.cpp)
    void light_entities(rgl::mesh** list, int count);   // 0x47ce00 (mesh_slot/mesh_scene.cpp)
    void light_props();                         // 0x47ced0
    void run_spawns();                          // 0x483eb0 (mesh_slot/mesh_game.cpp: up to 100 passes spawning every pending entry)
    void frame(float dt, int spawn);            // 0x483f60 (mesh_game.cpp: the per-frame mission tick; start_setup calls it with (0.1, 1))
};
extern Mission* g_mission;                      // 0x5e292c

#endif // GAME_MISSION_H
