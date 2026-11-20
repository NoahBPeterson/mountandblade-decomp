// agent_obj.h: the mission agent (Agent, 0x5928 bytes; vtable 0x591848 = the RecZ of src/rgl/xmlcfg/records.h) and its body
// (body: the scene-side rgl::strategic_entity at +0x250), ONE definition for src/game (agent/agent.cpp, agent2/agent_stats.cpp +
// agent_ai.cpp, agent3/agent_combat.cpp + agent_fx_sound.cpp, script, terrain_a/wind.cpp, rgl/xmlcfg/game_util.cpp). Each module
// declared the fields it needed under its own names; the union of them is below. Where two modules name the same bytes
// differently, the name used most is the field and the others are members of an anonymous union beside it (comment:
// "alias"). Every offset is read or written by a function of one of those modules (address cited); the rest is padding.
// Names provisional. The slices called the class Agent, AgentX, Rec5928 and MAgent (src/game/terrain_b/agent_rec.h keeps
// MAgent for the constructor 0x477a30 and the pool slot view).
// Needs match/include/rgl.h (src/game) or src/rgl/rgl_math.h (src/game/terrain_a) included first (rgl_base.h picks the
// module's own). Skeleton / node_groups / Channel / shape_base are only pointed to here: a module that calls through them
// includes skeleton.h / channel.h / shape.h itself.
#ifndef GAME_AGENT_OBJ_H
#define GAME_AGENT_OBJ_H
#include <stddef.h>
#include "../include/layout32.h"
#include <vector>
#include <deque>
#include "../include/rgl_base.h"
#include "../include/mat4.h"
#include "../include/vec.h"
#include "../include/timer.h"
#include "../include/pool.h"
#include "./tstring.h"
#include "./item_ref.h"
#include "./anim_slot.h"
#include "./slot_array.h"
#include "./blow.h"
#include "./agent_ai.h"                               // AgentAi (+0x258), InputState, Threat
#include "../include/iface.h"                           // IfaceB: the pooled record base (vptr, in_use, pool_id, serial)

using rgl::vector4;

struct Skeleton; struct Meter;
struct face;                                            // include/shape.h: a ground polygon
struct node_groups;
struct shape_base;
struct capsule;
struct NewtonBody;
struct Missile;
namespace rgl { struct psys; struct strategic_entity; }
struct ClassInfo;
struct Blow;

#include "../include/anim_set.h"                    // AnimSet / AnimDef / SubAnim: the animations (Agent::defs +0x12c)

// ---- the agent --------------------------------------------------------------------------------------------------
#include "./appearance_lods.h"                         // AppearanceLods (+0x194)

// IfaceB (include/iface.h) is the base, as for terrain_b's MAgent (the record the pool allocates; vtable 0x591848): its in_use is
// the pool's in-use flag (0x43b160, 0x4355f0, warn_agent 0x49d430), pool_id the agent id (0x43b300, 0x4a99c2; 0x435f10 compares
// it with Mission::f5c), serial +0xc.
struct Agent : IfaceB {                                 // 0x5928 bytes
    int kind;                                           // +0x10 0 = human, 1 = horse (agent_is_human 0x4a8af6, 0x435c10, 0x43f970)
    int state;                                          // +0x14 1 = alive, 4 = wounded (agent_is_alive 0x4a8a8d, 0x435c10, 0x441020)
    unsigned flags;                                     // +0x18 (bit 2: the player's agent, spawn_full 0x483870; reset 0x472f90 clears it)
    SlotVec slot_vars;                                  // +0x1c the agent's slots (agent_set_slot 0x4a09f3; terrain_b's reset 0x472f90 frees it)
    int horse;                                          // +0x2c mount id, -1 = on foot (agent_get_horse 0x4a8dee, 0x437f00, 0x435c10, 0x4362e0)
    int rider;                                          // +0x30 rider id, -1 = none (agent_get_rider 0x4a8e33, 0x43b8a0)
    int stand_animation;                                // +0x34 (agent_set_stand_animation 0x4a9811)
    int walk_forward_animation;                         // +0x38 (0x4a9845)
    unsigned flags3c;                    // +0x3c 0x448eb0
    clock2_timer t40;             // +0x40 0x4556d0 (armed with a delay when the agent spawns; movement waits for it)
    vector4 pos;                                        // +0x50 (0x43f970, 0x440070, 0x4406a0, 0x440e30, 0x4a8d37)
    vector4 vel;                                        // +0x60 move direction (0x4378f0, 0x438860, 0x43cc30, 0x435f10, 0x43f970)
    float rot[3];                    // +0x70 0x4543a0 (the body's roll angle; rotate_su reads it)
    int f7c;                    // +0x7c 0x4556d0, 0x45f660 (2 = the player's own agent: mesh_game spawn sets 2, mission_5 toggles 1/2)
    float scale;                                        // +0x80 (0x43f6d0, 0x435c10)
    float floor_blend;                                  // +0x84 0x4543a0 (blend towards the floor normal)
    int troop;                                          // +0x88 troop index, -1 = none (0x4406a0, 0x443870, 0x4a8ef8)
    ItemRef item;                                       // +0x8c the agent's own item: a horse agent's horse (0x43b8a0; agent_get_item_id 0x4a8f29)
    int seed;                                           // +0x94 the equipment roll seed (refresh_agent 0x4f7e00 hands it to build_appearance)
    vector4 v98;                                        // +0x98 (0x435c10)
    vec2 vel_xy;                                        // +0xa8 (0x43b7b0, 0x43bdb0; .y 0x43f970)
    float fb0;                                          // +0xb0 (reset 0x472f90: 1)
    float gait_ratio;                                   // +0xb4 0x4556d0 (speed / the gait's reference speed)
    unsigned gait;                                      // +0xb8 low 3 bits: the gait band of g_gait_walk (0x4556d0, 0x4435c0)
    face* floor_face;                                   // +0xbc the ground face under the agent (move_step 0x45d820 sets it; update_pitch 0x436130 reads its normal)
    face* obstacle;                                     // +0xc0 0x448eb0 (the steep ground face the capsule hit, or 0)
    vector4 nav_pos;                                    // +0xc4 (0x43cb30)
    AnimSlot anim[2];                                   // +0xd4 (stride 0x2c; anim[0].cur 0x4354d0, anim[1].cur = +0x100, .scale = +0x118)
    AnimSet* defs;                                      // +0x12c the animation definitions, g_anim_set (0x444370, 0x4354d0)
    int weapon_flags;                                   // +0x130 weapon class flags (0x444370, 0x432f60)
    int f134, f138;                                     // +0x134 / +0x138 (reset 0x472f90: -1)
    node_groups* slot_mesh[4];                          // +0x13c held-item display meshes (hide 0x4f8bc0 / show 0x4f8c20: vis mask at mesh+0xcc)
    union {
        node_groups* links[4];                          // +0x14c (0x43a800, 0x4379c0)
        node_groups* A[4];                              //   alias: 0x443aa0, 0x444370
    };
    node_groups* B[4];                                  // +0x15c
    union {
        node_groups* disp[4];                           // +0x16c quiver display (0x43a800)
        node_groups* C[4];                              //   alias: ammo / holster display
    };
    node_groups* slot_mesh4;                            // +0x17c (hide 0x4f8bc0 / show 0x4f8c20)
    node_groups* meters[4];                             // +0x180 the body lods whose node lists set_slot 0x470d80 caps (spawn caps [0] / [1] at 20)
    node_groups* name_label;                            // +0x190 the "agent_name" lod mesh (build_appearance 0x482240 ap[21]; draw_name 0x4552c0)
    AppearanceLods lods;                                // +0x194 build_appearance's extra lods and body lod (refresh_agent 0x4f7e00)
    rgl::psys* fx1ac;                    // +0x1ac 0x45f660
    rgl::psys* fx1b0;                    // +0x1b0 0x45f660
    rgl::psys* fx1b4;                    // +0x1b4 0x45f660
    rgl::psys* fx1b8;                    // +0x1b8 0x45f660
    int party;                                          // +0x1bc (agent_get_party_id 0x4a8e64, 0x443870)
    int party_stack;                                    // +0x1c0 the party stack the agent was spawned from (pick_spawn 0x473a60's out_stack, spawn_full)
    int team;                                           // +0x1c4 (0x435490, 0x435b60, 0x441020)
    char pad1c8[4];
    int class_rank;                                     // +0x1cc the agent's place in its team's class (update_team_stats 0x474e60 numbers them;
                                                        //   hand_pos 0x43a510: bit 0 = the hand position goes to the left)
    union {
        ItemRef pairs[11];                              // +0x1d0 equipped items (0x432790: 8-byte entries; 0x440070, 0x443aa0, 0x444370)
        struct {
            ItemRef equipment[10];                      //   alias: agent_has_item_equipped 0x4a94d8, Mission::pick_slots 0x46b2d0
            char slot_flag[10];                         // +0x220 (spawn_full 0x483870 / create 0x4836e0 set them to 1)
            char pad22a[2];
        };
    };
    union {
        int slots[5];                                   // +0x22c (slots[1] at +0x230, slots[4] at +0x23c; 0x432790, 0x440070)
        struct {
            int wield[2];                               // +0x22c wielded item index per hand (0x443aa0, 0x444370)
            int wield_prev[2];                          // +0x234 (0x444370)
            int both_hands;                             // +0x23c != 0: the wielded item takes both hands, slot 1 is unused (0x443aa0, 0x432790)
        };
    };
    int armor[3];                    // +0x240 0x454e30 (armor values by body part)
    float equip_value;                                  // +0x24c equipment_value() of the outfit (0x438180, 0x4556d0)
    rgl::strategic_entity* body;                        // +0x250 the scene body (rgl/scene/scene_entity.h: id, local frame +0x58, fx +0x304; 0x435c10, 0x4417d0)
    char pad254[0x258 - 0x254];
    AgentAi ai;                                         // +0x258 the AI / path state (game/agent_ai.h; constructor 0x475e50, init 0x463370)
    unsigned in_flags;                    // +0x55d0 0x4556d0 (movement input bits)
    char f55d4;                    // +0x55d4 0x4556d0 (the facing drives the velocity; mission_5 sets it in the scripted mode)
    char pad55d5[0x55d8 - 0x55d5];
    vector4 facing;                                     // +0x55d8 facing direction (0x43b8a0, 0x43bb80, 0x43bdb0, 0x442ed0, 0x435f10; 16 bytes copied by 0x43bdb0)
    int targeted_by;              // +0x55e8 (0x443270: how many agents have this one as their target)
    int snd_a, snd_b, snd_c;                            // +0x55ec / +0x55f0 / +0x55f4 shoot sound ids (0x43fd10)
    int mode1;                                          // +0x55f8 attack direction chosen for hand 0
    int snd2_a, snd2_b, snd2_c;                         // +0x55fc / +0x5600 / +0x5604
    int mode2;                                          // +0x5608
    int active;                                         // +0x560c (0x440070; 0x444370 clears it)
    int f5610;                                          // +0x5610 (0x444370: 1 = a block is in progress, cleared by an attack)
    int block_anim_a;                    // +0x5614 0x45c220 (the block animation, chosen by pick_sound_ids 0x440070)
    int block_anim_b;                    // +0x5618 0x45c220 (the second block animation, chosen by pick_sound_ids 0x440070)
    int f561c;                                          // +0x561c (the third block animation / the default order id)
    int reload_anim;                                    // +0x5620 0x45c220 (the reload animation of the ranged branch)
    vector4 prev_facing;                                // +0x5624 (0x43bdb0)
    float aim_error;                                    // +0x5634
    int has_blow;                    // +0x5638 0x4543a0.. (a blow is pending)
    BlowInit blow;                                      // +0x563c the last blow record (0x58 bytes; Blow::init 0x4353a0)
    vector4 impulse;                    // +0x5694 0x4556d0
    vector4 v56a4;                    // +0x56a4 0x448eb0 (the slices of the ragdoll code call it vel)
    vector4 v56b4;                                      // +0x56b4 (0x43db90, 0x441020)
    int variant;                                        // +0x56c4 (range table index, 0x438860, 0x43fd10, 0x435520)
    float cur;                                          // +0x56c8 eased value (0x438860); the angle of 0x43d040
    float pitch;                                        // +0x56cc eased pitch angle (update_pitch 0x436130, 0x43d040)
    int f56d0;                                          // +0x56d0 (reset 0x472f90 clears it)
    int weapon_mode;                    // +0x56d4 0x4461e0, 0x4465e0, 0x44d000
    int f56d8, f56dc;                                   // +0x56d8 / +0x56dc (reset 0x472f90 clears them)
    float prev_scale0;                                  // +0x56e0 0x45f660 (previous scale of animation slot 0)
    float prev_scale1;                                  // +0x56e4 0x45f660 (the scale of animation slot 0 one tick before prev_scale0)
    float turn_rate;                    // +0x56e8 0x448eb0 (the accumulated turn of a horse, decayed every tick; >0.1 triggers the turn order)
    float turn_rate2;                    // +0x56ec 0x4556d0 (decayed every tick; no other reader in this module)
    unsigned override_flags;                            // +0x56f0 the spawn rule's override flags (SpawnRec +8; Troop::auto_equip reads bits 9, 28)
    int override_items[8];                              // +0x56f4 (SpawnRec::override_items)
    int num_override_items;                             // +0x5714
    int entry_no;                                       // +0x5718 (agent_get_entry_no 0x4a8ead)
    float start_health;                                 // +0x571c (mission5 s284a0 family)
    float max_health;                                   // +0x5720 (heal_party 0x4a48ad)
    float health;                                       // +0x5724
    int kill_count;                                     // +0x5728 (agent_get_kill_count 0x4a9328)
    int wounded_count;                                  // +0x572c
    int xp_pool;                    // +0x5730 0x457b40 (experience accumulated for the party's ...)
    rgl::matrix fr1;                                    // +0x5734 the swing frame of the current tick (0x45a170); fr1.o.x = -2e7 on a weapon change (0x445a20, 0x44bc30)
    rgl::matrix fr2;                                    // +0x5774 the previous swing frame (o.x < -1e7: none)
    float charge_ramp;                                  // +0x57b4 0x45c220 (melee charge ramp)
    char pad57b8[0x57bc - 0x57b8];
    char hit_once;                                      // +0x57bc 0x45a170 (the swing already hit)
    char prop_done;                                     // +0x57bd 0x45a170 (the scene-prop test was done)
    char pad57be[0x57c0 - 0x57be];
    float charge;                                       // +0x57c0 0x45c220
    char pad57c4[0x57c8 - 0x57c4];
    clock2_active_timer mount_timer;                    // +0x57c8 0x4556d0 (its armed flag +0x57d8: 0x435c10)
    clock2_timer tm;                                           // +0x57e0 (clock id, start tick) (0x442dc0)
    clock2_timer timer;                                 // +0x57f0 (0x43bdb0)
    clock2_timer t5800;                    // +0x5800 0x4556d0
    clock2_timer t5810;                    // +0x5810 0x4556d0
    clock2_timer weave_timer;                           // +0x5820 0x448eb0 (start: the tick the weave gives up and re-plans)
    int f5830;                                          // +0x5830 (0x435c10, 0x442dc0: the body state, 2 = active, 3 = dying, 4 = fading, 5 = removed)
    char queued;                    // +0x5834 0x4529e0.. (the agent is queued as dead)
    char pad5835[0x5838 - 0x5835];
    int mount_id;                    // +0x5838 0x44fc60 (the agent whose horse capsule body is added)
    NewtonBody* mount_body;                    // +0x583c 0x44fc60
    char pad5840[0x5844 - 0x5840];
    int dismount_state;                    // +0x5844 0x44d000 (1 = the rider died, the horse must dismount; 2 = done)
    vector4 spawn;                    // +0x5848 0x4554c0 (corpse spawn position)
    int channel;                                        // +0x5858 FMOD channel (0x443870)
    char pad585c[4];
    gui_timer snd_stamp;                                // +0x5860 when the playing sound started (0x443870)
    float snd_length;                                   // +0x5870 the playing sound's length in seconds (0x443870)
    int mount_attempts;                    // +0x5874 0x4556d0 (the mount attempts of the player's agent, at most 3)
    char need_name;                    // +0x5878 0x45f660 (draw_name is due)
    char pad5879[0x587c - 0x5879];
    int nav_face;                                       // +0x587c
    int disabled;                                       // +0x5880
    rgl::mat4 hold;                    // +0x5884 0x4543a0 (the last bone frame that touched the floor)
    char mount_sync;                    // +0x58c4 0x44fc60 (the ragdoll's mount link still has to be applied)
    char mount_vel_sync;                    // +0x58c5 0x44fc60 (the linked agent's velocity still has to be added)
    char pad58c6[0x58c8 - 0x58c6];
    vector4 link_vel;                    // +0x58c8 0x44fc60 (velocity of the linked agent)
    char floor_sound_done;                    // +0x58d8 0x44fc60 (the ragdoll's floor-hit sound was played)
    char pad58d9[0x58dc - 0x58d9];
    int phys_idx;                                       // +0x58dc (0x4417d0)
    vector4 anchor;                                     // +0x58e0 (0x4417d0)
    vector4 hit_vec;                    // +0x58f0 0x457b40
    int hit_bone;                    // +0x5900 0x457b40
    vector4 hit_pos;                    // +0x5904 0x457b40
    char pad5914[0x5918 - 0x5914];
    clock2_timer t5918;                    // +0x5918 0x457710

    // ---- members: src/game/agent (agent.cpp, phys_set.cpp) ----
    // 0x432790: the item reference of equipment slot idx, by hidden pointer. An inline function of the shared header: the
    // linker kept the first object's copy (agent2 / agent3 call it at 0x4385e0 and inline the same body at their other uses).
    __declspec(noinline) ItemRef slot_pair(int idx) const;
    ItemRef slot_pair_inl(int idx) const                // the same body, inlined by the compiler at some call sites
    {
        const ItemRef* p = 0;
        int i = slots[idx];
        if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
        ItemRef r;
        r.item = -1;
        if (p) {
            r.item = p->item;
            r.m4 = p->m4;
        }
        return r;
    }
    ItemRef cur_pair() const
    {
        const ItemRef* p = 0;
        if (slots[0] >= 0) p = &pairs[slots[0]];
        ItemRef r;
        r.item = -1;
        if (p) {
            r.item = p->item;
            r.m4 = p->m4;
        }
        return r;
    }
    __forceinline ItemRef cur_pair_fi() const        // cur_pair / slot_pair_inl as cl inlined them in agent4/agent_logic.cpp (forced: the in-class versions above are left to the compiler)
    {
        const ItemRef* p = 0;
        if (slots[0] >= 0) p = &pairs[slots[0]];
        ItemRef r;
        r.item = -1;
        if (p) {
            r.item = p->item;
            r.m4 = p->m4;
        }
        return r;
    }
    __forceinline ItemRef slot_pair_fi(int idx) const
    {
        const ItemRef* p = 0;
        int i = slots[idx];
        if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
        ItemRef r;
        r.item = -1;
        if (p) {
            r.item = p->item;
            r.m4 = p->m4;
        }
        return r;
    }
    int clamped_team() const;                    // 0x435490
    bool flag_0x400_or_0x114() const;            // 0x4354d0
    void classify(int key);                      // 0x435520
    float range_scale() const;                      // 0x4355f0
    float volume_scale() const;                     // 0x435620
    void reset_if_enemy_charged();               // 0x435b90
    void camera_params(capsule* p);       // 0x435c10 (the body's collision capsule: segment l0 / l1 and radius)
    void steer(float dt);                        // 0x435f10
    ItemRef best_weapon_slot() const;            // 0x435fc0
    float equipment_value() const;               // 0x436050
    void update_pitch(float dt);                 // 0x436130
    void pick_sounds(int key, int mode, int* snd_a, int* snd_b, int* snd_c, int* variant);   // 0x4362e0
    void weapon_info(int idx, unsigned* skill, int* stat);                                   // 0x436810
    ItemRef best_ammo(int idx, int* missing, int* total, int* slot_out);                     // 0x436880
    int total_missing_ammo();                    // 0x436a70
    void build_static_collision();               // 0x4417d0 (phys_set.cpp)

    // ---- members of other objects (earlier / shared) ----
    inline bool f100_is_0x48_and_charged() const;            // 0x432470 inlined (defined in agent3/agent_types.h, which has Skeleton and Channel)
    int is_neutral(const Agent* o) const;  // 0x4323f0 (returns int; the callers test only al: (char) at the call)
    bool is_enemy(const Agent* o) const;         // 0x432350 (game_util.cpp AgentX; relation == -1; script)
    bool is_friend(const Agent* o) const;        // 0x4323a0 (relation == 1)
    int id_matches_dir(int id, int dir);         // 0x4326d0 (ret 8; this unused)
    int pick(int slot);                // 0x433040 (earlier object)
    void fill_weights(float* out, int idx, int rec);   // 0x432f60 (idx unused)
    int agent_class() const;                     // 0x433140 (game_util.cpp hand_kind)

    // ---- members: src/game/agent2 (agent_stats.cpp, agent_ai.cpp) ----
    void get_dir(vec2* out);                     // 0x4378f0
    inline void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
    bool has_weapon_for(int mode);               // 0x437f00
    float stamina_scale();                       // 0x438180
    void set_default_order();                    // 0x4382e0
    float attack_speed();                        // 0x438360
    void clear_ammo(int unused);                 // 0x438520
    float damage_factor(int idx);                // 0x4385e0
    void smooth_turn(float dt);                  // 0x438860
    void hand_pos(vector4* out);                 // 0x43a510
    void release_body();                         // 0x43a6f0
    void update_quiver();                        // 0x43a800
    void start_channel(int idx, SubAnim* cmd, unsigned* in_flags, float* base);   // 0x43a980 (ret 0x10)
    void collect_enemies(int unused);            // 0x43b300 (ret 4)
    unsigned pick_target();                      // 0x43b4d0
    unsigned pick_random();                      // 0x43b5d0
    vector4* local_velocity(vector4* out);       // 0x43b7b0
    unsigned find_talk_target();                 // 0x43b8a0
    unsigned pick_front();                       // 0x43b9e0
    unsigned pick_duel();                        // 0x43bb80
    void update_aim_error();                     // 0x43bdb0
    void poll_input(float unused, InputState* out);   // 0x43c9c0 (ret 8)
    int update_nav_face();                       // 0x43cb30
    int move_anim(int mode);                     // 0x43cc30
    void update_anims(int unused);               // 0x43d040 (ret 4)
    void set_goal_agent(unsigned id);            // 0x43db90
    void set_goal_point(vector4 p, int face);    // 0x43dcb0
    void set_goal_near(const vector4* center, const float* radius);   // 0x43dd70
    void advance_path();                         // 0x43df60

    // ---- members: src/game/agent3 (agent_combat.cpp) ----
    void aim_point_and_dir(vector4* pos_out, vector4* dir_out);     // 0x43f560
    bool capsule_hit(shape_base* other, vector4* out_point, vector4* out_a, int* out_bone);   // 0x43f6d0
    int pick_attack_dir(unsigned target_id);     // 0x43f970
    void start_shoot(int slot);                  // 0x43fd10
    void pick_sound_ids(int* a, int* b, int* c); // 0x440070 (ret 0xc)
    int can_hit(unsigned aid, unsigned bid, int alt, int flag, const float* target);   // 0x4406a0 (ret 0x14)
    bool line_of_sight(unsigned tid, shape_base* ignore);           // 0x440e30
    int fire_check(unsigned tid, int* out_blocker);                 // 0x441020

    // ---- members: src/game/agent3 (agent_fx_sound.cpp) ----
    bool can_target(unsigned aid, unsigned flags, float maxdist, float* out_dist);   // 0x442ed0 (ret 0x10)
    bool any_hostile_near();                     // 0x443020
    void update_alarm();                         // 0x443140
    unsigned pick_shout_target(int flags, float maxdist);   // 0x443270
    unsigned pick_threat();                      // 0x443430
    void play_hit_sound(int* kind);              // 0x4435c0
    void play_sound_var(int* kind, float* var);  // 0x443730
    void play_sound(int* snd);                   // 0x443870
    void play_sound(const int* sound);           // 0x443870 (ret 4; the script's view)
    void play_pain_sound(float health);          // 0x443a00
    void update_equipment_display();             // 0x443aa0
    void on_wield_changed();                     // 0x444370

    // ---- members: src/game/agent4 (agent_think.cpp) ----
    void reset_path();                                  // 0x432080 (rgl/xmlcfg/records.cpp: drops the planned path)
    Agent();                                            // 0x477a30 (terrain_b/poly_mesh.cpp)
    void reset();                                       // 0x472f90 (terrain_b/poly_mesh.cpp: the spawn reset)
    inline ClassInfo* class_rec();                          // the per-team, per-troop-class record (inlined at the use sites of agent4/agent_think.cpp)
    void pick_reachable_goal(const vector4* center, const float* radius, const int* face, const int* max_steps);   // 0x445720
    void goal_near_for_class(const vector4* center, const float* radius);   // 0x445850
    bool plan_path();                            // 0x445910
    void play_class_sound(const int* kind);      // 0x4459c0
    int top_up_ammo(int count);                  // 0x445a90 (tops up the held ranged weapon's ammo stacks; returns the units added)
    void on_wield_reset();                       // 0x445a20
    void set_weapon_mode(int mode);              // 0x4461e0
    void cycle_slot(int which);                  // 0x44bc30
    void start_attack(int kind);                 // 0x4464d0
    void update_ai();                            // 0x4465e0
    void ai_update_control();                    // 0x448eb0
    void choose_action(int unused);              // 0x44d000
    void ai_idle_tick(const float* dt);          // 0x44df30
    void shoot();                                // 0x44e2b0

    // ---- members: src/game/agent4 (agent_update.cpp) ----
    void start_ragdoll();                        // 0x44fc60
    void hit_damage(int hand, Missile* missile, float* out_dmg, unsigned* out_flag, unsigned victim, float* scale);   // 0x453710
    void apply_blood(const float* center, float amount);   // 0x4519f0 (ret 8)
    void drop_held();                            // 0x4529e0
    void get_label_box(void* out);               // 0x453050
    void shield_hit(int damage);                 // 0x4532f0
    void on_removed();                           // 0x454320
    void refresh_carried(Missile* other);        // 0x452650 (ret 4: (re)creates the lod mesh showing the carried ranged item of slot 1)

    void ragdoll_update();                       // 0x4543a0
    void reduce_blow(Blow* b);                   // 0x454e30
    void draw_name();                            // 0x4552c0
    void spawn_corpse();                         // 0x4554c0
    void update_controls(float dt);              // 0x4556d0
    void tick_death();                           // 0x457710
    void note_blow(Blow* b);                     // 0x457af0
    void process_blow(float unused);               // 0x457b40 (ret 4): the handler of a pending blow
    void update(float dt);                       // 0x45f660: the per-frame agent update (animation scales, footsteps, input, attack and movement)
    void tick();                                 // 0x45ff30: the per-frame agent tick (animations, rider/mount copy, off-map check, death)
    void move_step(float dt);                    // 0x45d820: the per-frame movement / collision step (capsule sweep, floor probe, landing blow)
    void update_attack(float dt);                // 0x45c220: the per-tick attack / reload state machine (state in `active`)
    int swing_trace(int hand, Missile* missile, Blow* out, float* scale);   // 0x45a170: the melee swing / missile hit test (returns 0 none, 1 hit, 2 blocked / friendly, 3 hit a prop)
    // ---- members: src/game/script ----
    void set_anim_phase(int channel, float progress);   // 0x4354b0 (ret 8): anim[channel].phase
    void set_sel(const int* team);              // 0x489a90 (ret 4)


    // ---- members: src/game/agent5 (agent_hud.cpp) ----
    void hide();                                 // 0x4f8bc0 (ret: the held-item meshes get the hidden vis mask)
    void show();                                 // 0x4f8c20
    void set_slot(int idx, int v);         // 0x470d80
};

MB_LAYOUT32_ASSERT(Agent_size_check, sizeof(Agent) == 0x5928);

// ---- the agent table (mission+4): the Pool<Agent> of the mission's agents, pages of 16, id >> 4 -> page ---------------------
// count +0 (0x443270, 0x441020, warn_agent 0x49d430), pages +0x14 (first at +0x18), serial +0x24 (terrain_b: Pool<MAgent>::serial);
// its accessors are the agent code's own out-of-line copies (0x43b160, 0x43b1b0, 0x43b1f0)
struct AgentTable : Pool<Agent> {
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
    __declspec(noinline) Agent* get(unsigned id);       // 0x43b1f0 (thiscall, ret 4)
    Agent& operator[](unsigned id) { return pages[id >> 4][id & 0xf]; }   // inline lookup: pages of 16 agents
};
MB_LAYOUT32_ASSERT(AgentTable_size_check, sizeof(AgentTable) == 0x2c);

// per-variant {lo, hi} turn range of Agent::smooth_turn (0x438860, agent_stats.cpp defines it) and the hit test of 0x43fd10
struct RangePair { float lo, hi; };
extern const RangePair g_range_tbl[6];                  // 0x590aac, indexed by Agent::variant

#endif // GAME_AGENT_OBJ_H
