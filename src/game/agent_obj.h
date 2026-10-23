// agent_obj.h: the mission agent (Agent, 0x5928 bytes; vtable 0x591848 = the RecZ of src/rgl/xmlcfg/records.h) and its body
// (AgentBody, the scene-side object at +0x250), ONE definition for src/game (agent/agent.cpp, agent2/agent_stats.cpp +
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
#include <vector>
#include "../include/rgl_base.h"
#include "../include/mat4.h"
#include "../include/vec.h"
#include "../include/timer.h"
#include "./tstring.h"
#include "./item_ref.h"
#include "./anim_slot.h"
#include "./slot_array.h"

using rgl::vector4;

struct Skeleton;
struct node_groups;
struct shape_base;
struct CamParams;
struct Blow;

// a frame with the operations the agent code calls (rgl::mat4 + the rotation 0x40e570; mul is mat4's operator* 0x40e870)
struct posm : rgl::mat4 {
    rgl::matrix mul(const rgl::matrix& m) const;       // 0x40e870 (hidden return pointer)
    void rotate_fu(const float& angle);                // 0x40e570
    vector4 transform_point(const vector4& p) const;   // 0x408460
};

// ---- animation requests and the definition list at agent+0x12c ----------------------------------------------------------
struct SubAnim {                                        // 0x40 bytes: one sub-animation request (AnimCmd)
    const void* anim;                                   // +0 (0x43a980)
    char pad4[4];
    unsigned flags;                                     // +8 (0x10000000 = ...; 0x432f60)
    float scale;                                        // +0xc
    int a, b;                                           // +0x10 / +0x14
    char pad18[0x2c - 0x18];
    float v2c, v30, v34, v38;                           // +0x2c (vector4, 0x43d040)
    float offset;                                       // +0x3c
};
struct AnimDef {                                        // 0x30 bytes
    char pad0[0x10];
    SubAnim* subs;                                      // +0x10
    int nsubs;                                          // +0x14 (game_util.cpp 0x433140 reads it as the count)
    unsigned flags;                                     // +0x18
    char pad1c[0x2c - 0x1c];
    int f2c;                                            // +0x2c
};
struct DefList { AnimDef* defs; int count; };           // +0x12c of the agent

// ---- the agent --------------------------------------------------------------------------------------------------
// 0x18-byte entry of the sorted nearest-enemy list (0x43b300, 0x443430)
struct Threat { int id; float dist; vector4 dir; };
struct InputRec { unsigned bits; char pad[0x14]; unsigned lo; int hi; };   // queued input record (+0x18 time)
struct InputState { unsigned v[6]; };
struct Obj_a0 { char pad[0xa0]; vector4 d; };            // +0xa0 direction vector (0x436130)

struct AgentBody {                                      // the scene-side body (+0x250 of the agent)
    int f0, f4;
    int id;                                             // +8 scene entity id (0x43a6f0)
    char padc[0x58 - 0xc];
    posm m;                                             // +0x58: s, f, u, o rows of the body frame (0x4417d0, 0x43b7b0, 0x43cc30, 0x43f6d0, 0x4406a0)
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                     // +0x304 (0x435c10, 0x43f560, 0x43f970, 0x440e30, 0x443aa0, 0x444370)
};

struct Agent {                                          // 0x5928 bytes
    char pad0[4];                                       // +0 vtable (0x591848)
    union {
        int used;                                       // +4 (0x43b160)
        int f4;                                         //   alias: 0x4355f0
        int live;                                       //   alias: warn_agent 0x49d430
    };
    union {
        int id;                                         // +8 side / agent id (0x43b300, 0x4a99c2)
        int f8;                                         //   alias: 0x435f10 compares it with Mission::f5c
    };
    char padc[4];
    union {
        int f10;                                        // +0x10 (1 = non-human; 0x435c10, 0x43f970)
        int kind;                                       //   alias: agent_is_human 0x4a8af6 (0 = human, 1 = horse)
    };
    union {
        int f14;                                        // +0x14 (1 = active; 0x435c10, 0x441020)
        int state;                                      //   alias: agent_is_alive 0x4a8a8d (1 = alive, 4 = wounded)
    };
    char pad18[4];
    SlotArray slot_vars;                                // +0x1c (agent_set_slot 0x4a09f3)
    char pad1d[0x2c - 0x1d];
    union {
        int f2c;                                        // +0x2c mount id, -1 = on foot (0x437f00, 0x435c10, 0x4362e0)
        int horse;                                      //   alias: agent_get_horse 0x4a8dee
    };
    union {
        int f30;                                        // +0x30 (0x43b8a0)
        int rider;                                      //   alias: agent_get_rider 0x4a8e33
    };
    int stand_animation;                                // +0x34 (agent_set_stand_animation 0x4a9811)
    int walk_forward_animation;                         // +0x38 (0x4a9845)
    char pad3c[0x50 - 0x3c];
    vector4 pos;                                        // +0x50 (0x43f970, 0x440070, 0x4406a0, 0x440e30, 0x4a8d37)
    vector4 vel;                                        // +0x60 move direction (0x4378f0, 0x438860, 0x43cc30, 0x435f10, 0x43f970)
    char pad70[0x80 - 0x70];
    union {
        float scale;                                    // +0x80 (0x43f6d0)
        float f80;                                      //   alias: 0x435c10
    };
    char pad84[4];
    int troop;                                          // +0x88 troop index, -1 = none (0x4406a0, 0x443870, 0x4a8ef8)
    ItemRef f8c;                                        // +0x8c (0x43b8a0; .item = the horse item, agent_get_item_id 0x4a8f29)
    char pad94[4];
    vector4 v98;                                        // +0x98 (0x435c10)
    union {
        vec2 vel_xy;                                    // +0xa8 (0x43b7b0, 0x43bdb0)
        struct { float fa8; float fac; };               //   alias: fac = +0xac (0x43f970)
    };
    char padb0[0xb8 - 0xb0];
    unsigned fb8;                                       // +0xb8 class bits (& 7) (0x4435c0)
    Obj_a0* fbc;                                        // +0xbc (0x436130)
    char padc0[4];
    vector4 nav_pos;                                    // +0xc4 (0x43cb30)
    AnimSlot anim[2];                                   // +0xd4 (stride 0x2c; anim[0].cur 0x4354d0, anim[1].cur = +0x100, .scale = +0x118)
    union {
        DefList* defs;                                  // +0x12c (0x444370)
        struct TroopDef { char pad[0x18]; unsigned flags; char pad2[0x30 - 0x1c]; } **f12c;   //   alias: 0x4354d0
    };
    int f130;                                           // +0x130 weapon class flags (0x444370, 0x432f60)
    char pad134[0x14c - 0x134];
    union {
        node_groups* links[4];                          // +0x14c (0x43a800, 0x4379c0)
        node_groups* A[4];                              //   alias: 0x443aa0, 0x444370
    };
    node_groups* B[4];                                  // +0x15c
    union {
        node_groups* disp[4];                           // +0x16c quiver display (0x43a800)
        node_groups* C[4];                              //   alias: ammo / holster display
    };
    char pad17c[0x1bc - 0x17c];
    union {
        int f1bc;                                       // +0x1bc (0x443870)
        int party;                                      //   alias: agent_get_party_id 0x4a8e64
        float hp;                                       //   alias: init_max_hp 0x461200
    };
    float max_hp;                                       // +0x1c0 (0x461200)
    int team;                                           // +0x1c4 (0x435490, 0x435b60, 0x441020)
    char pad1c8[4];
    unsigned char f1cc;                                 // +0x1cc (0x43a510)
    char pad1cd[3];
    union {
        ItemRef pairs[11];                              // +0x1d0 equipped items (0x432790: 8-byte entries; 0x440070, 0x443aa0, 0x444370)
        ItemRef equipment[10];                          //   alias: agent_has_item_equipped 0x4a94d8, AgentEq::sel 0x46b2d0
    };
    int pad228;
    union {
        int slots[5];                                   // +0x22c (slots[1] at +0x230, slots[4] at +0x23c; 0x432790, 0x440070)
        struct {
            int wield[2];                               // +0x22c wielded item index per hand (0x443aa0, 0x444370)
            int wield_prev[2];                          // +0x234 (0x444370)
            int f23c;                                   // +0x23c (0x443aa0)
        };
    };
    char pad240[0x24c - 0x240];
    float f24c;                                         // +0x24c (0x438180)
    AgentBody* body;                                    // +0x250 (0x435c10, 0x4417d0)
    char pad254[0x25c - 0x254];
    vector4 goal;                                       // +0x25c (0x43db90)
    vector4 goal_dir;                                   // +0x26c
    int goal_face;                                      // +0x27c
    char pad280[4];
    int goal_changed;                                   // +0x284
    char pad288[0x294 - 0x288];
    char blk294[0x2c0 - 0x294];                         // +0x294 blend_target's second argument (0x441020)
    int f2c0;                                           // +0x2c0 (0x443140; cleared by agent_set_scripted_destination 0x4a9604)
    unsigned f2c4;                                      // +0x2c4 bit 0 = alarmed (0x443140)
    union {
        int f2c8;                                       // +0x2c8 (0x435b90)
        int simple_behavior;                            //   alias: aisb_* (agent_get_simple_behavior 0x4a9737)
    };
    char pad2cc[0x318 - 0x2cc];
    int f318;                                           // +0x318
    char pad31c[4];
    int path_face[0x405];                               // +0x320
    vector4 path_pos[0x3ff];                            // +0x1334
    int path_active;                                    // +0x5324
    int path_cur;                                       // +0x5328
    int path_count;                                     // +0x532c
    char path_flag;                                     // +0x5330
    char pad5331[0x5338 - 0x5331];
    int path_state;                                     // +0x5338
    int f533c;                                          // +0x533c (0x435b90)
    float f5340, f5344, f5348, f534c;                   // +0x5340 (0x43a510)
    char pad5350[0x5398 - 0x5350];
    int combat_state;                                   // +0x5398 (agent_get_combat_state 0x4a9780)
    char pad539c[0x53a4 - 0x539c];
    InputState in_prev;                                 // +0x53a4
    char pad53bc[0x53c4 - 0x53bc];
    InputRec** q_buf;                                   // +0x53c4
    unsigned q_cap, q_head, q_count;                    // +0x53c8
    InputState in_last;                                 // +0x53d4
    Threat threats[16];                                 // +0x53ec (sorted by distance, nearest first; 0x443430)
    int nthreat;                                        // +0x556c
    char pad5570[0x5574 - 0x5570];
    rgl::matrix scripted_destination;                   // +0x5574 (agent_set_scripted_destination 0x4a95d2)
    int scripted_mode;                                  // +0x55b4
    int always_attack_in_melee;                         // +0x55b8 (0x4a970a)
    char pad55bc[0x55c8 - 0x55bc];
    float speed_limit;                                  // +0x55c8 m/s (agent_set_speed_limit 0x4a96d2)
    char pad55cc[0x55d8 - 0x55cc];
    union {
        struct { float fx, fy, fz, fw; };               // +0x55d8 facing direction (0x43b8a0, 0x43bb80, 0x43bdb0, 0x442ed0; 16 bytes copied by 0x43bdb0)
        struct { float dirx, diry, dirz, dirw; };       //   alias: 0x435f10
    };
    int f55e8;                                          // +0x55e8 (0x443270)
    int snd_a, snd_b, snd_c;                            // +0x55ec / +0x55f0 / +0x55f4 shoot sound ids (0x43fd10)
    int mode1;                                          // +0x55f8 attack direction chosen for hand 0
    int snd2_a, snd2_b, snd2_c;                         // +0x55fc / +0x5600 / +0x5604
    int mode2;                                          // +0x5608
    int active;                                         // +0x560c (0x440070; 0x444370 clears it)
    int f5610;                                          // +0x5610 (0x444370)
    char pad5614[4];
    char pad5618[4];
    int f561c;                                          // +0x561c (default order id)
    char pad5620[4];
    vector4 prev_facing;                                // +0x5624 (0x43bdb0)
    float aim_error;                                    // +0x5634
    char pad5638[0x5640 - 0x5638];
    int target_id;                                      // +0x5640 (0x43cc30)
    char pad5644[0x5664 - 0x5644];
    float move_x, move_y, move_z;                       // +0x5664
    char pad5670[4];
    int weapon_kind;                                    // +0x5674
    char pad5678[0x56b4 - 0x5678];
    vector4 v56b4;                                      // +0x56b4 (0x43db90, 0x441020)
    union {
        int variant;                                    // +0x56c4 (range table index, 0x438860, 0x43fd10)
        int f56c4;                                      //   alias: 0x435520
    };
    float cur;                                          // +0x56c8 eased value (0x438860); the angle of 0x43d040
    float f56cc;                                        // +0x56cc (0x43d040, 0x436130)
    char pad56d0[0x56e4 - 0x56d0];
    float f56e4;                                        // +0x56e4
    char pad56e8[0x5718 - 0x56e8];
    int entry_no;                                       // +0x5718 (agent_get_entry_no 0x4a8ead)
    char pad571c[4];
    float max_health;                                   // +0x5720 (heal_party 0x4a48ad)
    float health;                                       // +0x5724
    int kill_count;                                     // +0x5728 (agent_get_kill_count 0x4a9328)
    int wounded_count;                                  // +0x572c
    char pad5730[0x57d8 - 0x5730];
    char f57d8;                                         // +0x57d8 (0x435c10)
    char pad57d9[0x57e0 - 0x57d9];
    Timer tm;                                           // +0x57e0 (clock id, start tick) (0x442dc0)
    Timer timer;                                        // +0x57f0 (0x43bdb0)
    char pad5800[0x5830 - 0x5800];
    int f5830;                                          // +0x5830 (0x435c10, 0x442dc0)
    char pad5834[0x5858 - 0x5834];
    int channel;                                        // +0x5858 FMOD channel (0x443870)
    char pad585c[4];
    int f5860;                                          // +0x5860 (0x443870)
    char pad5864[4];
    __int64 stamp;                                      // +0x5868 (0x443870)
    float f5870;                                        // +0x5870
    char pad5874[0x587c - 0x5874];
    int nav_face;                                       // +0x587c
    int disabled;                                       // +0x5880
    char pad5884[0x58dc - 0x5884];
    int phys_idx;                                       // +0x58dc (0x4417d0)
    vector4 anchor;                                     // +0x58e0 (0x4417d0)
    char pad58f0[0x5928 - 0x58f0];

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
    int clamped_team() const;                    // 0x435490
    bool flag_0x400_or_0x114() const;            // 0x4354d0
    void classify(int key);                      // 0x435520
    float scale_f4() const;                      // 0x4355f0
    float scale_f10() const;                     // 0x435620
    void reset_if_enemy_charged();               // 0x435b90
    void camera_params(CamParams* p);     // 0x435c10
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
    inline bool charged_0x48() const;            // 0x432470 inlined (defined in agent3/agent_types.h, which has Skeleton and Channel)
    bool rel_a(const Agent* o);                  // 0x432350
    bool rel_b(const Agent* o);                  // 0x4323a0
    bool rel_c(const Agent* o);                  // 0x4323f0
    bool is_enemy(const Agent* o) const;         // 0x432350 (game_util.cpp AgentX; relation == -1; script)
    bool is_friend(const Agent* o) const;        // 0x4323a0 (relation == 1)
    int id_matches_dir(int id, int dir);         // 0x4326d0 (ret 8; this unused)
    int pick_slot_anim(int slot);                // 0x433040 (earlier object)
    int agent_class() const;                     // 0x433140 (game_util.cpp hand_kind)

    // ---- members: src/game/agent2 (agent_stats.cpp, agent_ai.cpp) ----
    void get_dir(vec2* out);                     // 0x4378f0
    void get_dir(float* out);                    // 0x4378f0 (ret 4; agent3's view of the same function)
    inline void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
    void set_animation(int channel, int anim, int flags, float speed);   // 0x4379c0 (ret 0x10; the script's name)
    bool has_weapon_for(int mode);               // 0x437f00
    float stamina_scale();                       // 0x438180
    void set_default_order();                    // 0x4382e0
    float attack_speed();                        // 0x438360
    float duration();                            // 0x438360 (agent3's name)
    void clear_ammo(int unused);                 // 0x438520
    void refill_ammo(int a1);                    // 0x438520 (ret 4; the script's name)
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
    int find_target();                           // 0x43b9e0 (agent3's name)
    unsigned pick_duel();                        // 0x43bb80
    void update_aim_error();                     // 0x43bdb0
    void poll_input(int unused, InputState* out);   // 0x43c9c0 (ret 8)
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

    // ---- members: src/game/script ----
    void position_changed();                     // 0x4554c0
    void deliver_blow(const Blow* b);     // 0x457af0 (ret 4)
    void update_after_blow(int a1);              // 0x457b40 (ret 4)
    void set_animation_progress(int channel, float progress);   // 0x4354b0 (ret 8)
    void set_team(const int* team);              // 0x489a90 (ret 4)
    int total_ammo();                            // 0x436a70 (the script's name of total_missing_ammo)

    // ---- members: src/game/terrain_a/wind.cpp ----
    void init_max_hp();                          // 0x461200
};

// ---- the agent table (mission+4): agents in pages of 16, id >> 4 -> page --------------------------------------------------
struct AgentTable {
    int count;                                          // +0 (0x443270, 0x441020, warn_agent 0x49d430)
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (first at +0x18; id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
    __declspec(noinline) Agent* get(unsigned id);       // 0x43b1f0 (thiscall, ret 4)
    int serial;                                         // +0x24 (terrain_b: Pool<MAgent>::serial)
};

#endif // GAME_AGENT_OBJ_H
