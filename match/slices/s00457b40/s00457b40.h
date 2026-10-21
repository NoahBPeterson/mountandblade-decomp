// Slice s00457b40 declarations (names provisional; offsets from 0x457b40 itself and the neighbour slices
// s00453710, s004532f0, s004529e0, s004520d0, s004428c0, s00432130, s00434ad0, s0043a280, s00442180).
#ifndef S00457B40_H
#define S00457B40_H
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

#define MB_NOTHROW throw()

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() MB_NOTHROW;                           // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
struct StrTable { tstring* lookup(const tstring& key) MB_NOTHROW; };   // 0x43a1e0
extern StrTable g_strtable;                         // 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };                   // 0x452c00 (thiscall, ret 0x14)
extern MsgLog g_msglog;                             // 0x5d7080

struct mat4 {
    vector4 s, f, u, o;
    mat4 operator*(const mat4& b) const;            // 0x40e870 (this = left operand)
};
struct pos : matrix {
    matrix relative(const matrix& base) const;      // 0x4329d0 (thiscall, ret 8: hidden return pointer + base)
};

// ---- items --------------------------------------------------------------------------------------------
struct ItemKind {                                   // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    unsigned type_flags;                            // +0x1c8 (low byte = itp_type_*)
    char pad1cc[4];
    __int64 capability_flags;                       // +0x1d0 itcf_*
    char pad1d8[0x238 - 0x1d8];
    bool is_ranged();                               // 0x433d90
};
extern ItemKind* g_items;                           // 0x5e29d0
enum { itp_type_one_handed_wpn = 2, itp_type_polearm = 4, itp_type_bow = 8, itp_type_crossbow = 9, itp_type_thrown = 10 };
#define itp_unique 0x00001000
#define itp_wooden_attack 0x00020000
#define itcf_throw_stone 0x0000000000010000i64
#define tf_hero 0x10
#define tf_allways_fall_dead 0x80
#define imod_lame 30

struct ItemRef {                                    // 8 bytes: item index + amount/modifier word
    int item;
    union {
        unsigned m4;
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;
    };
    int stat_214();                                 // 0x435040
    void reduce(int n);                             // 0x437510
};

// ---- game data ------------------------------------------------------------------------------------------
struct Party {                                      // 0x5ac8 bytes
    int skill(int skill);                           // 0x4b2370 (thiscall, ret 4): best skill level in the party
};
struct PartyTable { Party* get(int id); };          // 0x51ee50 (thiscall, ret 4)
struct CharRec {                                    // troop record, 0x508 bytes
    char pad0[0x30];
    unsigned __int64 flags;                         // +0x30 tf_*
    char pad38[0x58 - 0x38];
    int strength;                                   // +0x58
    char pad5c[0x80 - 0x5c];
    float prof[7];                                  // +0x80 weapon proficiencies
    char pad9c[0xb0 - 0x9c];
    int level;                                      // +0xb0
    char padb4[0x4a4 - 0xb4];
    ItemRef items[10];                              // +0x4a4 (items[8] = the horse)
    char pad4f4[0x508 - 0x4f4];
    bool has_flag10();                              // 0x435130 (tf_hero)
    int scaled_b0();                                // 0x435170
    tstring* display_name();                        // 0x43a480
    void adjust(int i, float k);                    // 0x437770 (ret 8)
};
struct GameData {
    char pad0[0x78];
    int main_party;                                 // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                     // +0x8c
    char pad90[0x98 - 0x90];
    int cnt_killed, cnt_killed_friend, cnt_ko, cnt_ko_friend;   // +0x98
    char pada8[0xf8 - 0xa8];
    PartyTable parties;                             // +0xf8
    char padf9[0x13a00 - 0xf9];
    CharRec* chars;                                 // +0x13a00
    void add_experience(int troop, int amount);     // 0x4428c0 (ret 8)
};
extern GameData* g_game;                            // 0x5e2930

// ---- the skeleton / body -------------------------------------------------------------------------------
struct Psys;
struct mesh {                                       // rgl::mesh
    char pad0[0x260];
    void translate(const vector4* d);               // 0x44f240
};
struct Skeleton {
    int count;                                      // +0
    char* elems;                                    // +4 (0x1dc-byte records, frame at +0x84)
    char pad8[0x50 - 8];
    char done;                                      // +0x50
    char pad51[0x64 - 0x51];
    int has_poses;                                  // +0x64
    char pad68;                                     // +0x68
    char* rec(int i);                               // 0x43f260
    void attach_mesh(int idx, mesh* p, const matrix& m);    // 0x442460 (ret 0xc)
};
struct AgentBody {
    char pad0[0x58];
    mat4 m;                                         // +0x58
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                 // +0x304
};

struct gui_timer {                                  // 16 bytes
    char pad[16];
    void set_delay(float seconds);                  // 0x42cd20
};

// ---- the mission --------------------------------------------------------------------------------------
struct Agent;
struct AgentTable {
    char pad0[0x18];
    Agent** pages;                                  // +0x18 (vector<Agent*> first; id >> 4 -> page of 16 agents)
    Agent* get(unsigned id);                        // 0x43b1f0 (thiscall on mission+4)
};
struct Rec58 { char pad[0x20]; unsigned flags; char pad24[0x58 - 0x24]; };
extern Rec58* g_rec58;                              // 0x5e2a00
struct Mission {
    int f0;
    AgentTable tbl;                                 // +4
    char pad_[0x5c - 4 - sizeof(AgentTable)];
    int local_id;                                   // +0x5c
    int mount_id;                                   // +0x60 (the player's horse)
    char pad64[0x5df0 - 0x64];
    int cur_rec;                                    // +0x5df0
    char pad5df4[0x62e0 - 0x5df4];
    int dismount_count;                             // +0x62e0
    char pad62e4[0x6528 - 0x62e4];
    unsigned flags;                                 // +0x6528
    void blood_burst(vector4 center, float amount); // 0x4525a0 (ret 0x14)
    void log_timed(const tstring* s, float seconds);    // 0x453020 (ret 8)
    struct Emit* spawn_effect(int idx, const mat4* m, int duration);    // 0x442ca0
};
extern Mission* g_mission;                          // 0x5e292c

struct Emit {
    char pad0[0x184];
    vector4 v184;                                   // +0x184
    char pad194[4];
    float f198;                                     // +0x198
    char pad19c[0x1bc - 0x19c];
    void set_v184(const vector4* v);                // 0x433740
};
struct EmitDef {                                    // 0xc0 bytes
    char pad0[0x24];
    float f24;                                      // +0x24
    char pad28[0xa4 - 0x28];
    float a4, a8, ac;                               // +0xa4
    char padb0[0xc0 - 0xb0];
};
struct EmitTable { EmitDef* defs; int count; };
extern EmitTable g_emit_defs;                       // 0x8ed5090
struct VoiceTable {
    char pad0[0x18];
    int pad18;
    char pad1c[0x28 - 0x1c];
    unsigned count;
    int get_f1f0(unsigned i);                       // 0x43b100
    int get_f1f4(unsigned i);                       // 0x43b130
};
extern VoiceTable g_voices;                         // 0x8ed4f60
extern int g_voice_default_a;                       // 0x8ed50a0
extern int g_voice_default_b;                       // 0x8ed50a4

struct AnimInfo { char pad0[0x3c]; float length; }; // +0x3c
struct AnimDef { char pad0[0x10]; AnimInfo* info; char pad14[0x30 - 0x14]; };   // 0x30 bytes
extern AnimDef* g_anims;                            // 0xa76628

extern __int64 g_ticks[];                           // 0x5e2948 (100 kHz clocks)

// ---- the weapon object in a blow (arrow / thrown / swung) ---------------------------------------------
struct WeaponObj {
    char pad0[0x80];
    int item_a;                                     // +0x80
    char pad84[4];
    int item_b;                                     // +0x88
    mesh* make_ranged_mesh();                       // 0x44f6a0
    float stuck_length();                           // 0x467f80 (item length * 0.01)
};

// ---- the agent (0x5928 bytes) -----------------------------------------------------------------------------
struct Agent {
    char pad0[8];
    int id;                                         // +8
    char padc[4];
    int f10;                                        // +0x10 (0 = human, 1 = horse)
    int f14;                                        // +0x14 (1 alive, 4 knocked out, 5 dead)
    char pad18[0x2c - 0x18];
    int mount_id;                                   // +0x2c
    int rider_id;                                   // +0x30
    char pad34[0x3c - 0x34];
    unsigned flags3c;                               // +0x3c
    gui_timer timer;                                // +0x40
    vector4 pos;                                    // +0x50
    vector4 dir;                                    // +0x60
    char pad70[0x88 - 0x70];
    int troop;                                      // +0x88
    ItemRef base_item;                              // +0x8c
    char pad94[0xac - 0x94];
    float ac;                                       // +0xac
    char padb0[0xd4 - 0xb0];
    int action;                                     // +0xd4
    char padd8[0xe8 - 0xd8];
    unsigned fe8;                                   // +0xe8
    char padec[0x1bc - 0xec];
    int f1bc;                                       // +0x1bc
    char pad1c0[4];
    int team;                                       // +0x1c4
    char pad1c8[0x250 - 0x1c8];
    AgentBody* body;                                // +0x250
    char pad254[0x55c0 - 0x254];
    int last_attacker;                              // +0x55c0
    char pad55c4[0x5638 - 0x55c4];
    int blow_pending;                               // +0x5638
    unsigned blow_flags;                            // +0x563c (0x10 = from a charging mount?, 0x20 = silent)
    int blow_attacker;                              // +0x5640
    vector4 blow_pos;                               // +0x5644
    vector4 blow_dir;                               // +0x5654
    vector4 blow_vec;                               // +0x5664
    int blow_bone;                                  // +0x5674
    float blow_force;                               // +0x5678
    float blow_damage;                              // +0x567c
    int blow_type;                                  // +0x5680
    int blow_dmg_type;                              // +0x5684 (2 = blunt)
    int blow_item;                                  // +0x5688
    unsigned blow_item_flags;                       // +0x568c
    WeaponObj* blow_weapon;                         // +0x5690
    vector4 impulse;                                // +0x5694
    char pad56a4[0x56b4 - 0x56a4];
    vector4 v56b4;                                  // +0x56b4
    char pad56c4[0x5724 - 0x56c4];
    float hp;                                       // +0x5724
    int cnt_killed;                                 // +0x5728
    int cnt_ko;                                     // +0x572c
    int xp_pool;                                    // +0x5730
    char pad5734[0x57e0 - 0x5734];
    int tick_idx;                                   // +0x57e0
    char pad57e4[4];
    __int64 tick_stamp;                             // +0x57e8
    char pad57f0[0x5830 - 0x57f0];
    int f5830;                                      // +0x5830
    char pad5834[4];
    int f5838;                                      // +0x5838
    char pad583c[0x5844 - 0x583c];
    int f5844;                                      // +0x5844
    char pad5848[0x58f0 - 0x5848];
    vector4 hit_vec;                                // +0x58f0
    int hit_bone;                                   // +0x5900
    vector4 hit_pos;                                // +0x5904
    char pad5914[4];
    gui_timer timer5918;                            // +0x5918

    void process_blow(int unused);                  // 0x457b40 (ret 4)
    void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
    void play_sound_var(int* kind, float* var);     // 0x443730
    void play_hit_sound(int* kind);                 // 0x4435c0
    void play_class_sound(const int* kind);         // 0x4459c0
    int move_anim(int mode);                        // 0x43cc30
    bool is_enemy(const Agent* o) const;            // 0x432350
    bool is_friend(const Agent* o) const;           // 0x4323a0
    ItemRef slot_pair(int idx) const;               // 0x432790 (thiscall, ret 8)
    void fn_4552c0();                               // 0x4552c0
};

typedef char assert_agent_size[sizeof(Agent) == 0x5928 ? 1 : -1];

int __cdecl round_float(float* f);                  // 0x432540
float __cdecl clamp_float(const float* x, const float* lo, const float* hi);   // 0x40e3d0
float __cdecl fmax_ref(const float* a, const float* b);   // 0x4324d0
float __cdecl fmin_ref(const float* a, const float* b);   // 0x4324b0
float frand();                                      // 0x40e400

extern int g_opt_dmg_to_player;                     // 0x5aa054
extern int g_opt_dmg_to_friends;                    // 0x5aa058
extern int g_opt_damage_msgs;                       // 0x5aa060
extern int g_opt_shot_difficulty;                   // 0x5aa064
extern int g_opt_kill_msgs;                         // 0x5aa068
extern int g_opt_blood;                             // 0x5aa020
extern int g_opt_hit_fx;                            // 0x5a9fc0
extern float g_k_a18178;                            // 0xa18178
extern float g_k_a1811c;                            // 0xa1811c
extern float g_k_a1812c;                            // 0xa1812c
extern float g_k_a18130;                            // 0xa18130
extern float g_k_a18134;                            // 0xa18134
#endif
