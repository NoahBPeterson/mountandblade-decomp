// mission.h: the mission object (global pointer g_mission, 0x5e292c) with the per-team records (TeamRec / ClassInfo) and the
// mark records (MarkRec) it embeds. ONE definition for src/game: agent/agent.cpp (mode flag, team reads), agent2 (agent_stats,
// agent_ai: relations, nav mesh, play-area rectangle), agent3 (agent_combat, agent_fx_sound: dead list, effects, clocks, view
// frame), terrain_b (mission.cpp, terrain_b.cpp: reset, spawn, team statistics, props, weather blend) and ui (mission_ui.cpp:
// replay frames, per-team counters, combatant slots). Each module declared the fields it read; the union is below, in offset
// order, every field with the address of a function that proves it. Names provisional (the slices called it Mission, Mission4,
// FxMarks, Scene and, in script_interpreter.h, MissionX, which stays a separate view of the script interpreter's names).
//
// Views that differ between modules sit in anonymous unions of pointers; the scene container at +0 is the rgl::world. The agent table at +4
// is AgentTable (agent_obj.h, the Pool<Agent>); terrain_b reads the same bytes as Pool<MAgent> (agent_pool() in terrain_b/mission.h). The
// record pool at +0x30 is Pool<Missile>. Needs rgl::vector4 / rgl::matrix (rgl_base.h picks the module's own).
// The constructor (0x4d3880, options_panel/world_app.cpp) builds the members in declaration order: the two pools, the entry
// points, the clock-2 timers, the vectors, the terrain description, the nav mesh and the tstring, then clears scene and
// scene_counter.
#ifndef GAME_MISSION_H
#define GAME_MISSION_H
#include <stddef.h>
#include "../include/layout32.h"
#include <vector>
#include "../include/rgl_base.h"
#include "../include/mat4.h"
#include "../include/timer.h"
#include "./tstring.h"
#include "./terrain_desc.h"
#include "./terrain_b/poly_mesh.h"
#include "./agent_obj.h"

namespace rgl { struct world; }                                // terrain_b/cell_grid.h: the scene container terrain_b sees

namespace rgl { struct psys; }
struct Light;
struct PlacedObj;
struct PlacedObj;                                  // the 0x1c8-byte prop record (terrain_b/records.h, agent3/agent_combat.h)
struct Missile;
struct SpawnRec;
struct ItemStackG;                              // mesh_slot/mesh_game.cpp: the item stack a spawned agent carries
struct Stack;
namespace rgl { struct mesh; struct entity; }

struct RectF { float x0, y0, x1, y1; };          // an axis-aligned rectangle (Mission::scene_box)

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
struct TeamRec {                                // 0x16c bytes, from mission +0x527c: the class selector code (0x489890, 0x4898e0) and
                                                // agent_set_team (0x4a99af: lea edi,[..+0x527c], leader at [edi+4]) address it from there
    int f0;                                     // +0x00
    int leader;                                 // +0x04 agent id, -1 = none (0x435b60, agent_set_team 0x4a99af)
    ClassInfo cls[3];                           // +0x08
    int relation[8];                            // +0x14c relation to team b: -1 hostile, 1 friend (0x43b300, 0x435b90, 0x4a9d5d)
    // the class selector (ui/mission_ui.cpp): each operates on the classes in use (cls[i].use)
    void set_v(int v);                          // 0x489860: k4 = v
    rgl::vector4 first();                       // 0x489890 (hidden return): pos of the first class in use, else (0, 0, 0)
    void set_vec(const rgl::vector4* p, const int* face, const rgl::vector4* offset);   // 0x4898e0 (ret 0xc): pos, nav, target
    void bump(int d);                           // 0x4899e0: mode += d, clamped to 1..4
    void make_label(char* out);                 // 0x48d940 (mission_ui2/mission_3.cpp): the classes in use, "infantry, archers and cavalry"
};
MB_LAYOUT32_ASSERT(TeamRec_size_check, sizeof(TeamRec) == 0x16c);

// ---- the scene's entry points (mark records: replace_mark 0x472160, start_setup 0x484590): MarkRec, ./mark_rec.h ----------
#include "./mark_rec.h"

struct Mission {
    rgl::world* scene;                          // +0x0000 the scene container (src/rgl/scene/world.h; the slices called it grid / nav / Scene / world / level / Level)
    AgentTable agents;                          // +0x0004 (0x43b160, 0x43b1b0, 0x43b1f0; terrain_b reads it as Pool<MAgent>: agent_pool() in terrain_b/mission.h)
    Pool<Missile> poolJ;                           // +0x0030 the 0xb0-byte records (missiles; reset 0x477f90, alloc 0x440c90)
    int player_id;                              // +0x005c the player's agent id (0x435f10 compares it with Agent::id; 0x43fd10, 0x440070, 0x443730, 0x443870)
    int f60;                                    // +0x0060
    char pad64[0x1ac - 0x64];
    rgl::psys* fog;                             // +0x01ac emitters spawned by the last post_spawn (0x47a0c0)
    rgl::psys* layer0;                               // +0x01b0
    rgl::psys* layer1;                               // +0x01b4
    rgl::psys* layer2;                               // +0x01b8
    char pad1bc[0x670 - 0x1bc];
    MarkRec recs[64];                           // +0x0670 the entry points (0x472160, 0x484590)
    int scene_counter;                          // +0x5270 (MissionV::init 0x5143f0; start_setup: % 3 salts the weather seeds)
    int f5274;                                  // +0x5274 (start_setup clears it)
    int num_marks;                              // +0x5278 entry points in use: 0x40, or 5 with a custom camera (start_setup)
    TeamRec teams[8];                           // +0x527c (0x474e60, 0x435b90, 0x43b300, 0x48a9e0); leaders from +0x5280
    clock2_timer tm;                            // +0x5de0
    int mode;                                   // +0x5df0 index into g_mission_templates (0x473a60, 0x435840)
    int scene_idx;                              // +0x5df4 the scene the mission runs in: index into GameData::levels (0x4435c0, MissionV::init 0x5143f0)
    int f5df8;                                  // +0x5df8
    std::vector<int> v5dfc;                     // +0x5dfc (retire list, 0x479160)
    int list[2][128];                           // +0x5e0c candidate party ids per side (0x473a60)
    int cnt[2];                                 // +0x620c
    int f6214;                                  // +0x6214
    int f6218;                                  // +0x6218
    int counts[8][6];                           // +0x621c per-team counters (0x4759c0, 0x48a8b0, 0x48a910; 0x435b90 reads [t][4], [t][5])
    float changed_at;                           // +0x62dc
    int dismount_count;                         // +0x62e0 riders thrown off the player's horse (at most 4 per mission; 0x457b40)
    clock2_timer setup_timer;                   // +0x62e8 the time since start_setup (0x444370); start_setup restamps it and the next three
    clock2_timer timer62f8;                     // +0x62f8 (separate members: the constructor starts them at fixed offsets, not as an array)
    clock2_timer timer6308;                     // +0x6308
    clock2_timer timer6318;                     // +0x6318
    clock2_timer clock;                         // +0x6328 the mission clock (start_clock 0x48a860; the screen tick 0x5261e0 reads its elapsed time)
    char clock_set;                             // +0x6338
    char f6339;                                 // +0x6339 (init_scene clears it)
    int num_interior;                           // +0x633c (build_interior 0x47a800's count; light_entities 0x47ce00)
    rgl::mesh* interior[32];                    // +0x6340 the interior meshes build_interior returns
    std::vector<rgl::strategic_entity*> prop_ents;   // +0x63c0 the scene props' entities (spawn_prop 0x44c210 appends the world::create_entity(9) ones; MissionV::init_scene clears)
    TerrainDesc terrain;                        // +0x63d0 (0x479160)
    int object_kind;                            // +0x651c the map editor's selected object kind (the terrain dialog's list, 0x4fe860)
    int advantage;                              // +0x6520 the battle advantage (MissionV::init: MissionCtx::advantage; add_amount 0x4677a0 scales the spawn amounts by it)
    int mission_result;                         // +0x6524 (set_mission_result 0x4aa65d, the screen tick 0x5261e0; start_setup clears it)
    unsigned flags;                             // +0x6528 the mission template's flags (MissionV::init_scene; 0x448eb0 tests bit 0, 0x4556d0)
    RectF scene_box;                            // +0x652c the props' bounding box (start_setup)
    float min_x, min_y, max_x, max_y;           // +0x653c play-area rectangle (0x479e20, 0x48a990, 0x43cb30)
    float base_z;                               // +0x654c ground height (0x47a0c0, 0x4435c0)
    int f6550;                                  // +0x6550 (0x473a60)
    char f6554;                                 // +0x6554 (0x4465e0, 0x448eb0; init_scene clears it)
    bool muted;                                 // +0x6555 (0x4435c0; init_scene sets it)
    int talk_troop;                             // +0x6558 troop of the conversation partner (the screen tick 0x5261e0: MissionCtx::start_battle's a1)
    char pad655c[0x6598 - 0x655c];                // +0x655c evidence: no instruction of the original .text has a displacement in +0x655c..0x6597 (any base register)
    rgl::vector4 f6598;                         // +0x6598
    rgl::mesh* water;                           // +0x65a8 the water mesh (Terrain::regenerate_water 0x482170 clears it; Mission::frame scrolls its texture)
    float f65ac;                                // +0x65ac
    Light* wind_a;                              // +0x65b0 ambient wind A (setup_ambient 0x4792d0)
    Light* wind_b;                              // +0x65b4 ambient wind B
    rgl::entity* precip_fx;                     // +0x65b8 the rain / snow emitter entity (start_setup; camera_to 0x4fa790 keeps it at the eye)
    int f65bc;
    rgl::entity* sun_fx;                        // +0x65c0 the sun billboard entity (camera_to: 500 units towards the sun)
    rgl::mesh* sun_fx_mesh;                     // +0x65c4 its first (empty) mesh
    rgl::mesh* sun_mesh;                        // +0x65c8 its "sun" mesh
    float flare_amt;                            // +0x65cc the sun flare's visibility (camera_to)
    rgl::entity* cursor;                        // +0x65d0 the edit cursor entity (start_setup)
    rgl::mesh* cursor_mesh;                     // +0x65d4 "edit_cursor"
    rgl::mesh* cursor_fill;                     // +0x65d8 vertex_color_add_mat
    rgl::entity* sel_marker;                    // +0x65dc the selection marker (char_sheet/pick_entity.cpp: frame at +0x58, visible at +0x140)
    rgl::mesh* sel_mesh;                        // +0x65e0 vertex_color_add_mat (pick_entity: select_entity's mesh)
    rgl::entity* edit_obj;                      // +0x65e4 the edit-mode object marker (start_setup creates it, scale 33.5; the editor dialog's edit_obj, gui_edit refresh 0x4c77c0 copies its frame)
    float day_time;                             // +0x65e8 hours, 0..24 (start_setup)
    int f65ec;                                  // +0x65ec (start_setup: the party's nav face +4)
    float temper;                               // +0x65f0 (start_setup's log: "temper = %3.2f, humidity = %3.2f, height = %3.2f fog_far = %d")
    float humidity;                             // +0x65f4
    float height;                               // +0x65f8
    int cloudiness;                             // +0x65fc 0..3 (rain raises it; ranks the env rows)
    float rain_amount;                          // +0x6600
    int precip_mode;                            // +0x6604 0 none, 1 rain, 2 snow (g_emit_defs used[0] / used[1]; 0x47a0c0)
    int f6608;                                  // +0x6608
    int env_row;                                // +0x660c index into g_env_rows, the sky / ambient row (0x47a0c0, 0x442ca0)
    float fog_far;                              // +0x6610 (start_setup; agent4/agent_think.cpp 0x44e2b0 scales the aim error by it)
    float fog_density;                          // +0x6614
    float wind_x;                               // +0x6618 the precipitation drift
    float wind_y;                               // +0x661c
    int conversation;                           // +0x6620 (script_interpreter.h MissionX)
    rgl::matrix view;                                  // +0x6624 (0x43f970, 0x440070)
    int rain_snd;                               // +0x6664 the rain's sound slot, -1 = none (camera_to plays it while precip_mode is 1)
    char announce_reinforcements;               // +0x6668 "ui_reinforcements_have_arrived" is pending (mesh_slot/mesh_game.cpp)
    char announce_enemies;                      // +0x6669 "ui_new_enemies_have_arrived" is pending
    int ordered_blunt;                          // +0x666c 1 after the "use blunt weapons" order (9), 0 after "use any weapon" (10) (screen tick 0x5261e0)
    int ordered_hold_fire;                      // +0x6670 1 after "hold fire" (12), 0 after "fire at will" (13)
    int ordered_dismount;                       // +0x6674 1 after "dismount" (4), 0 after "mount" (3)
    std::vector<Agent*> dead;                   // +0x6678 (first at +0x667c; dead / culled list: 0x442dc0, terrain_b reset 0x479160)
    std::vector<PlacedObj> props;                  // +0x6688 (first at +0x668c) (0x472050)
    PolyMesh nav;                                // +0x6698 navigation mesh (agent2: 0x43cb30, 0x43dd70, 0x43df60)
    rgl::mesh* marker_poly;                     // +0x672c the order-marker meshes (init_order_markers 0x52ae70): the polygon
    rgl::mesh* marker_spikes;                   // +0x6730
    rgl::mesh* marker_ribbons;                  // +0x6734
    rgl::entity* marker_entity;                 // +0x6738 the entity that carries the three meshes
    rgl::mesh* marker_ground;                   // +0x673c the ground marker mesh (on `special`)
    rgl::entity* special;                           // +0x6740 (0x472050; reset clears it)
    char need_redraw[3];                        // +0x6744 the map editor's rebuild flags of marker_spikes / marker_ribbons / marker_poly (0x52cf60)
    int cam_mode;                               // +0x6748 the scripted camera: 0 automatic, 1 manual (mission_cam_set_mode 0x4ab858; reset clears it)
    rgl::matrix frame_a;                        // +0x674c replay frame A (0x48a420)
    rgl::matrix frame_b;                        // +0x678c replay frame B
    float blend_from, blend_to, blend_time;     // +0x67cc (0x48a740)
    clock2_timer blend_timer;                   // +0x67d8 the blend's start
    int cam_target_agent;                       // +0x67e8 the agent the scripted camera follows, -1 = none (mission_cam_set_target_agent 0x4abe6f)
    int cam_use_agent_rotation;                 // +0x67ec
    int blend_n;                                // +0x67f0 easing steepness
    float time_speed;                           // +0x67f4 the time speed the blend starts from (blend2_param 0x48a7d0; mission_set_time_speed 0x4ab98c)
    float target_time_speed;                    // +0x67f8 the time speed it ends at
    clock2_timer time_speed_timer;              // +0x6800 the blend's start
    float time_speed_anim_time;                 // +0x6810 the blend's length in seconds, 0 = no blend
    tstring str;                                // +0x6814
    char done[64];                              // +0x6824 spawn-done flags per rule (0x473a60)

    Mission();                                  // 0x4d3880 (options_panel/world_app.cpp)
    ~Mission();                                 // 0x4d51d0 (options_panel/hud.cpp)

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
    rgl::psys* spawn_effect(int idx, const rgl::mat4* m, int duration);   // 0x442ca0 (agent_fx_sound.cpp)

    // ---- agent4 (agent_think.cpp) ----
    int spawn_missile(const rgl::vector4* pos, const rgl::vector4* dir, ItemRef* launcher, ItemRef* ammo, unsigned shooter_id);   // 0x44be50
    void spawn_prop(int* idx, int* id);         // 0x44c210
    void blood_burst(float x, float y, float z, int unused, float amount);   // 0x4525a0 (ret 0x14)
    int add_placed(const int* kind, const int* id, const int* value, const rgl::matrix* m, int* spawn_id);   // 0x44f870
    void set_object_target(int n);              // 0x4c6dc0 (rgl/mesh2/gui_edit.cpp): prop n's entity takes the prop's frame and scale

    // ---- terrain_b ----
    void update_prop_flags();                   // 0x472050
    void update_team_stats();                   // 0x474e60 (fastcall)
    void update_team_counts();                  // 0x4759c0 (fastcall)
    void replace_mark(int idx);                 // 0x472160
    bool rule_accepts(SpawnRec* a, Stack* b, int p3, char p4, char p5, char p6);   // 0x467940 (thiscall, ret 0x18, ignores this)
    void pick_spawn(int idx, int* out_party, int* out_stack, int* out_troop, int* out_side, int* out_flag, int* out_seed);   // 0x473a60
    void reset();                               // 0x479160
    void setup_ambient(int level_idx, int spec_idx, float t);                  // 0x4792d0
    void rebuild_nav(int which, int);           // 0x479e20
    void post_spawn(Agent* a);                 // 0x47a0c0
    void build_interior(int which, rgl::mesh** out, int* n);   // 0x47a800
    // ---- cls_map (progress_bar.cpp) ----
    void frame_step();                          // 0x4e8720 the map editor's marker meshes rebuilt from the nav mesh
    // ---- mission5 (mission_5.cpp) ----
    __declspec(noinline) bool order_agent(unsigned agent_id, ItemRef* order);   // 0x52b5c0 give an item / order to one of the agent's four slots
    __declspec(noinline) void save_scene_data();                 // 0x527f90 (the live terrain layers and the nav mesh into the scene record's streams)
    __declspec(noinline) void release_item(int idx);             // 0x528410 drop the scene entity of prop idx
    __declspec(noinline) void apply_agent_results(int mode);     // 0x529840 (mode unused) the agents' xp, the heroes' hit points, the casualties
    void record_killed(Agent* a);                                // 0x529b70
    void drop_stack_seed(int* seeds, int party, int slot, int value, int refill);   // 0x529c30 (ret 0x14) drop value from the stack's face seeds
    void record_wounded(Agent* a);                               // 0x529d00
    __declspec(noinline) void sync_scene_props();                // 0x529d80 the live props back into the scene record (GameData::scenes)
    __declspec(noinline) void init_order_markers();              // 0x52ae70 build the order-marker entities (marker_entity, special) once

    // ---- ui (mission_ui.cpp) ----
    rgl::matrix get_frame();                    // 0x48a420
    float blend_param();                        // 0x48a740
    float blend2_param();                       // 0x48a7d0
    void start_clock(int v);                    // 0x48a860
    int team_count();                           // 0x48a8b0
    int last_team();                            // 0x48a910
    bool point_in_rect(const float* p);         // 0x48a990
    void select_team(int i); // 0x48a9e0
    void unselect_team(int i); // 0x48aa20
    void select_class(int i, int s); // 0x48aa60
    int movement_order(int i); // 0x48aac0
    int riding_order(int i); // 0x48ab30
    int weapon_usage_order(int i); // 0x48ab90
    int hold_fire_order(int i); // 0x48abf0
    // ---- src/game/mission_ui2 (the scene props at +0x668c, 0x1c8-byte PlacedObj records) ----
    void retag_props(int from, int to);         // 0x48ed60 (ret 8: every intact prop named like tag `from` is renamed to tag `to`)
    int count_props(int tag);                   // 0x48ee20 (ret 4: intact props of tag `tag`)
    int find_prop(int tag, int nth);            // 0x48ee90 (ret 8: index of the nth intact prop of tag `tag`, -1 when none)
    // ---- src/game/agent5 (agent_hud.cpp) ----
    void pick_slots(Agent* a);                  // 0x46b2d0 (terrain_a/terrain_render.cpp: ret 4; this unused: pick the ten equipment slots; callers load ecx)
    void init_agent(Agent* a);                  // 0x4f7d70 (ret 4: pick the agent's equipment slots, refresh its look, re-wield the hand slots)
    void refresh_agent(Agent* a);               // 0x4f7e00 (ret 4; this unused: rebuild the agent's appearance and equipment bodies)
    void camera_to(const rgl::matrix* eye, const float* far_plane);   // 0x4fa790 (ret 8: store the camera frame, move the listener, the sun and the flare)
    void apply_sun_light(const rgl::vector4* pos, const float* scale);   // 0x4fb180 (ret 8: the per-agent sun intensity and fade)
    // ---- src/game/mesh_slot (mesh_start.cpp: Mission::start_setup addresses the object by displacement) ----
    void start_setup();                         // 0x484590 (the per-mission start-up sequence: level + mode, weather, sky/cloud/fog bits, sun, the 64-slot light table, props and spawns)
    void clear_done();                          // 0x4676c0 (terrain_a: clears the spawn-done flags)
    void copy_slot(int k);                      // 0x4676e0 (terrain_a/misc.h: copies a mark slot)
    float speed_factor(float x);                // 0x467740 (terrain_a; thiscall, ignores this)
    void add_amount(int i, unsigned n);         // 0x4677a0 (terrain_a: spawn-rule amount; the script calls it add_visitors)
    void reset_spawned(int id);                 // 0x4739f0 (terrain_b/terrain_b.cpp)
    void light_entities(rgl::mesh** list, int count);   // 0x47ce00 (mesh_slot/mesh_scene.cpp)
    void light_props();                         // 0x47ced0
    void spawn(Agent* a);                      // 0x4832b0 (mesh_slot/mesh_game.cpp: initialise one agent: skin mesh, capsule, equipment, corpse spawn)
    int spawn_full(const rgl::matrix* m, int troop, int a, int b, int c, int d, int e, int f, int cnt, int* arr);   // 0x483870 (ret 0x28)
    int create(const rgl::matrix* m, const ItemStackG* it, int troop);   // 0x4836e0 (ret 0xc)
    bool spawn_one(int id);                     // 0x483de0
    void run_spawns();                          // 0x483eb0 (mesh_slot/mesh_game.cpp: up to 100 passes spawning every pending entry)
    void frame(float dt, bool spawn);           // 0x483f60 (mesh_game.cpp: the per-frame mission tick; start_setup calls it with (0.1, 1))
    int place(const int* item);                 // 0x49b4b0 (party2/camera.cpp: ret 4; the result is add_placed's)
    int exec_trigger(int val);                  // 0x49f9e0 (party2/camera.cpp: ret 4; the result is spawn_full's)
    int enter_trigger(int val, int ip);         // 0x49f980 (party2/camera.cpp: ret 8)
    void check_slot(int id, unsigned bits); // 0x49c170
    void spawn_battle_icon(unsigned id); // 0x49bbb0
};
extern Mission* g_mission;                      // 0x5e292c

#endif // GAME_MISSION_H
