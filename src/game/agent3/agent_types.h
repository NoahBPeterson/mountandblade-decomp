// agent_types.h: the classes shared by agent_combat.cpp (0x43e190..0x441360) and agent_fx_sound.cpp
// (0x441ff0..0x444c28): the mission agent (Agent, 0x5928 bytes), its body / skeleton / animation slots, the
// mission object (Mission, AgentTable), the scene (world) and the small helpers both modules call. ONE definition
// of each class for this directory; every offset is read or written by a function of these two modules (the
// address is cited at the field). Names provisional.
//
// Known overlap with other headers (src/game/agent/agent.h, src/game/ui/mission.h, src/game/ui/item_ref.h,
// src/game/strat/strat_entity.h): they define their own, smaller views of Agent / Mission / ItemRef / Skeleton /
// AgentBody / world. They cannot be included together with this header (different field sets under the same
// names); the layouts agree where they overlap (Agent.team +0x1c4, pairs +0x1d0, slots +0x22c, body +0x250;
// Mission.scene +0, f5c/player_id +0x5c; world.cells +0x28).
#ifndef AGENT3_TYPES_H
#define AGENT3_TYPES_H
#include <stddef.h>
#include <vector>
#include "rgl.h"
#include "../tstring.h"
#include "../agent/item_kind.h"       // ItemKind, g_items, pos2
#include "../mat4.h"                   // rgl::mat4

using rgl::vector4;
using rgl::matrix;

// a frame with the operations the agent code calls (rgl::matrix + 0x40e870 / 0x40e570 / 0x408460)
struct posm : matrix {
    matrix mul(const matrix& m) const;                 // 0x40e870 (hidden return pointer)
    void rotate_fu(const float& angle);                // 0x40e570
    vector4 transform_point(const vector4& p) const;   // 0x408460
};

// ---- collision shapes (vtable 0x54a4b8 shape_base, 0x54a540 capsule; the full classes are in
// src/rgl/shapes/shapes.h, which cannot be included together with rgl.h) ---------------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}
};
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, void* out);    // 0x40bfe0
extern shape_base* g_hit_shape;                         // 0x5e246c: last shape hit by trace_ray

struct Emit;                                            // particle emitter (agent_fx_sound.h)
struct cell;                                            // world grid cell (agent_fx_sound.h)
struct Mesh;                                            // mesh object hanging off the agent (agent_fx_sound.h)

// the scene (rgl world): first field of the mission object. The members of other objects are only declared.
struct world {
    char pad0[0x28];
    cell** cells;                                // +0x28 grid of cells (0x442180)
    char pad2c[0xb4 - 0x2c];
    void* nav;                                          // +0xb4 navigation mesh
    void locate(const vector4* p, int* out) const;      // 0x41ed00 (grid cell of a point)
    Emit* alloc_emitter();                              // 0x427ea0 (0x442ca0)
    int trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60 (0x440e30)
    void collect_boxed(const vector4* bmin, const vector4* bmax, std::vector<unsigned>* out);   // 0x442180 (agent_fx_sound.cpp)
};

// ---- animation channels / skeleton (the animator at body+0x304) ---------------------------------------------
struct CapDef { vector4 a; vector4 b; float r; int pad[2]; };     // 0x2c bytes (0x43f6d0)
struct ChanRec {                                      // 0xdc bytes (0x43f6d0: items[i] stride)
    char pad0[0xcc];
    std::vector<CapDef> caps;                         // +0xcc (first +0xd0, last +0xd4)
};
struct SkelData {
    char pad0[0x18];
    int count;                                        // +0x18 (0x43f6d0)
    ChanRec* items;                                   // +0x1c
};
struct Channel { char pad0[0xd8]; float charge; };    // animation channel: charge at +0xd8 (0x4406a0, via charged_0x48)
struct Skeleton {
    int nbones;                                       // +0
    char* recs;                                       // +4 (0x1dc-byte records, matrix at +0x84; +0x1140 aim bone, +0x1170 head)
    SkelData* data;                                   // +8 (0x43f6d0)
    char pad0c[0x40 - 0xc];
    Channel* chans;                                   // +0x40 (0x4406a0)
    char pad44[0x50 - 0x44];
    char done;                                        // +0x50 (0x43e740, 0x43f260, 0x43f560, 0x43f6d0)
    char pad51[0x64 - 0x51];
    int has_poses;                                    // +0x64
    void update();                                    // 0x422130
    void update_poses();                              // 0x422080
    void ensure_updated();                            // 0x43e740
    char* rec(int i);                                 // 0x43f260 (animator record i, brought up to date first; matrix at +0x84)
    void set_rec_phase(int i, float p);               // 0x433b20 (0x444370)
    void remove_shape(Mesh* m);                       // 0x439ec0 (0x443aa0)
};
struct AgentBody {
    char pad0[0x58];
    posm m;                                           // +0x58 (0x43f6d0, 0x43f560, 0x4406a0; m.o at +0x98: 0x443730)
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                   // +0x304 (0x43f560, 0x43f970, 0x440e30, 0x443aa0, 0x444370)
};

// ---- the 0x38-byte record of the container instantiations: three words, two vector<int> (+0xc, +0x1c), three words
// (the PhysRec of src/game/agent/phys_set.h) and its allocator (agent_combat.cpp 0x43f510; agent_fx_sound.cpp 0x442060)
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o);         // 0x43f510
};
struct alloc_e56 {
    __declspec(noinline) void construct(E56* p, const E56& v);      // 0x442060
    void destroy(E56* p);                                           // 0x43e150
};

// ---- item reference (item index + packed word; src/game/agent/agent.h has the full class) --------------------
struct ItemRef { int item; unsigned m4; };

// ---- animation slot: 2 x 0x2c bytes at agent+0xd4 (0x4406a0 reads slot[1].cur at +0x100 and .scale at +0x118;
// 0x444370 sets cur/next/events/rate through the inlined 0x4378a0) ----------------------------------------------
struct AnimSlot {
    int cur;                                          // +0 current animation / action id (-1 none)
    int next;                                         // +4
    int sub;                                          // +8
    unsigned prev_mask;                               // +0xc
    int f10;                                          // +0x10
    unsigned events;                                  // +0x14 priority word (0x444370)
    float scale;                                      // +0x18
    int f1c;                                          // +0x1c
    float rate;                                       // +0x20 (0x444370)
    float phase;                                      // +0x24
    int f28;                                          // +0x28
    // 0x4378a0 as cl inlines it into 0x444370: accept a new animation when its priority is not lower
    bool set(int id, unsigned prio)
    {
        if ((prio & 0x1ffff) < (events & 0x1ffff) && (cur != -1 || next != -1))
            return false;
        next = id;
        events = prio;
        if (id == -1)
            events = 0;
        return true;
    }
};
struct DefList { void* defs; int count; };            // +0x12c of the agent (0x444370)
struct Threat { unsigned id; float dist; vector4 dir; };      // 0x18 bytes (0x443430)

// ---- the mission object (g_mission 0x5e292c) ------------------------------------------------------------
struct Agent;
struct AgentTable {                                     // mission+4
    int count;                                          // +0 (0x443270, 0x441020)
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (id >> 4 -> page of 16 agents; first at +0x18)
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
};
struct Mission {
    world* scene;                                       // +0 (0x440e30, 0x442ca0)
    AgentTable agents;                                  // +4
    char pad28[0x5c - 0x28];
    int player_id;                                      // +0x5c (0x43fd10, 0x440070, 0x443730, 0x443870)
    char pad60[0x53c8 - 0x60];
    int relation[1];                                    // +0x53c8 team x team, stride 0x5b ints, 1 = friend (0x441020, 0x443020)
    char pad53cc[0x5df4 - 0x53cc];
    int f5df4;                                          // +0x5df4 index into the 0x494-byte slot table of g_game (0x4435c0)
    char pad5df8[0x62e8 - 0x5df8];
    int time_idx;                                       // +0x62e8 (0x444370)
    char pad62ec[4];
    __int64 time_base;                                  // +0x62f0 (0x444370)
    char pad62f8[0x654c - 0x62f8];
    float f654c;                                        // +0x654c ground level (0x4435c0)
    char pad6550[0x6555 - 0x6550];
    bool muted;                                         // +0x6555 (0x4435c0)
    char pad6556[0x660c - 0x6556];
    int cam;                                            // +0x660c (0x442ca0)
    char pad6610[0x6624 - 0x6610];
    posm view;                                          // +0x6624 (0x43f970, 0x440070)
    char pad6664[0x6678 - 0x6664];
    std::vector<Agent*> dead;                           // +0x6678 (first at +0x667c) (0x442dc0)
    bool friends(int a, int b) const
    {
        if (a >= 0 && a < 8 && b >= 0 && b < 8)
            return relation[a * 0x5b + b] == 1;
        return false;
    }
    void queue_dead(Agent* a);                          // 0x442dc0 (agent_fx_sound.cpp)
    Emit* spawn_effect(int idx, const rgl::mat4* m, int duration);   // 0x442ca0 (agent_fx_sound.cpp)
};
extern Mission* g_mission;                              // 0x5e292c
extern __int64 g_ticks[];                               // 0x5e2948 clock table (1e-5 s ticks)

// ---- the agent ---------------------------------------------------------------------------------------------
struct Agent {                                          // 0x5928 bytes
    char pad0[8];
    int id;                                             // +8 (0x43fd10, 0x440070, 0x441020)
    char padc[4];
    int f10;                                            // +0x10 (0x43f970; 1 = non-human, 0x443730)
    int f14;                                            // +0x14 (1 = active) (0x441020, 0x442ed0)
    char pad18[0x2c - 0x18];
    int f2c;                                            // +0x2c target / mount id, -1 = none (0x43f970, 0x441020)
    char pad30[0x50 - 0x30];
    vector4 pos;                                        // +0x50 (0x43f970, 0x440070, 0x4406a0, 0x440e30)
    vector4 vel;                                        // +0x60 blend_target's first argument (0x43f970, 0x441020)
    char pad70[0x80 - 0x70];
    float scale;                                        // +0x80 (0x43f6d0)
    char pad84[0x88 - 0x84];
    int troop;                                          // +0x88 troop index, -1 = none (0x4406a0, 0x443870)
    char pad8c[0xac - 0x8c];
    float fac;                                          // +0xac (0x43f970)
    char padb0[0xb8 - 0xb0];
    unsigned fb8;                                       // +0xb8 class bits (& 7) (0x4435c0)
    char padbc[0xd4 - 0xbc];
    AnimSlot anim[2];                                   // +0xd4 (stride 0x2c; anim[1].cur = +0x100, .scale = +0x118)
    DefList* defs;                                      // +0x12c (0x444370)
    int f130;                                           // +0x130 weapon class flags (0x444370)
    char pad134[0x14c - 0x134];
    Mesh* A[4];                                         // +0x14c (0x443aa0, 0x444370)
    Mesh* B[4];                                         // +0x15c
    Mesh* C[4];                                         // +0x16c (ammo / holster display)
    char pad17c[0x1bc - 0x17c];
    int f1bc;                                           // +0x1bc (0x443870)
    char pad1c0[4];
    int team;                                           // +0x1c4 (0x441020, 0x443020)
    char pad1c8[0x1d0 - 0x1c8];
    ItemRef pairs[11];                                  // +0x1d0 equipped items (0x440070, 0x443aa0, 0x444370)
    int pad228;
    union {
        int slots[5];                                   // +0x22c (slots[4] at +0x23c; 0x440070)
        struct {
            int cur[2];                                 // +0x22c wielded item index per hand (0x443aa0, 0x444370)
            int prev[2];                                // +0x234 (0x444370)
            int f23c;                                   // +0x23c (0x443aa0)
        };
    };
    char pad240[0x250 - 0x240];
    AgentBody* body;                                    // +0x250
    char pad254[0x294 - 0x254];
    char blk294[0x2c0 - 0x294];                         // +0x294 blend_target's second argument (0x441020)
    int f2c0;                                           // +0x2c0 (0x443140)
    unsigned f2c4;                                      // +0x2c4 bit 0 = alarmed (0x443140)
    char pad2c8[0x53ec - 0x2c8];
    Threat threats[16];                                 // +0x53ec (0x443430)
    int nthreat;                                        // +0x556c
    char pad5570[0x55d8 - 0x5570];
    float fx, fy, fz;                                   // +0x55d8 facing direction (0x442ed0); &fx is blend_target's second argument (0x43f970)
    char pad55e4[4];
    int f55e8;                                          // +0x55e8 (0x443270)
    int snd_a, snd_b, snd_c;                            // +0x55ec / +0x55f0 / +0x55f4 shoot sound ids (0x43fd10)
    int mode1;                                          // +0x55f8 attack direction chosen for hand 0
    int snd2_a, snd2_b, snd2_c;                         // +0x55fc / +0x5600 / +0x5604
    int mode2;                                          // +0x5608
    int active;                                         // +0x560c (0x440070; 0x444370 clears it)
    int f5610;                                          // +0x5610 (0x444370)
    char pad5614[0x56b4 - 0x5614];
    vector4 v56b4;                                      // +0x56b4 (0x441020)
    int variant;                                        // +0x56c4 (0x43fd10; range table index 0x441020)
    char pad56c8[0x57e0 - 0x56c8];
    struct Timer {
        int clock; int pad; __int64 start;
        void reset() { start = g_ticks[clock]; }
    } tm;                                               // +0x57e0 (clock id, start tick) (0x442dc0)
    char pad57f0[0x5830 - 0x57f0];
    int state;                                          // +0x5830 (0x442dc0)
    char pad5834[0x5858 - 0x5834];
    int channel;                                        // +0x5858 FMOD channel (0x443870)
    char pad585c[4];
    int f5860;                                          // +0x5860 (0x443870)
    char pad5864[4];
    __int64 stamp;                                      // +0x5868 (0x443870)
    float f5870;                                        // +0x5870
    char pad5874[0x5928 - 0x5874];

    // members of other objects
    ItemRef slot_pair(int idx) const;                   // 0x432790 (hidden return pointer)
    ItemRef slot_pair_inl(int idx) const                // the same body, inlined by cl at some call sites
    {
        const ItemRef* p = 0;
        int i = slots[idx];
        if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
        ItemRef r;
        r.item = -1;
        if (p) r = *p;
        return r;
    }
    bool charged_0x48() const { return anim[1].cur == 0x48 && body->skel->chans->charge > 0.1f; }   // 0x432470 inlined
    bool rel_a(const Agent* o);                         // 0x432350
    bool rel_b(const Agent* o);                         // 0x4323a0
    bool rel_c(const Agent* o);                         // 0x4323f0
    int id_matches_dir(int id, int dir);                // 0x4326d0 (ret 8; this unused)
    void get_dir(float* out);                           // 0x4378f0 (ret 4)
    float duration();                                   // 0x438360
    int find_target();                                  // 0x43b9e0
    int pick_duel();                                    // 0x43bb80
    void pick_sounds(int key, int mode, int* snd_a, int* snd_b, int* snd_c, int* variant);   // 0x4362e0
    void update_quiver();                               // 0x43a800

    // agent_combat.cpp
    void aim_point_and_dir(vector4* pos_out, vector4* dir_out);     // 0x43f560
    bool capsule_hit(shape_base* other, vector4* out_point, vector4* out_a, int* out_bone);   // 0x43f6d0
    int pick_attack_dir(unsigned target_id);            // 0x43f970
    void start_shoot(int slot);                         // 0x43fd10
    void pick_sound_ids(int* a, int* b, int* c);        // 0x440070 (ret 0xc)
    int can_hit(unsigned aid, unsigned bid, int alt, int flag, const float* target);   // 0x4406a0 (ret 0x14)
    bool line_of_sight(unsigned tid, shape_base* ignore);           // 0x440e30
    int fire_check(unsigned tid, int* out_blocker);     // 0x441020

    // agent_fx_sound.cpp
    bool can_target(unsigned aid, unsigned flags, float maxdist, float* out_dist);   // 0x442ed0 (ret 0x10)
    bool any_hostile_near();                            // 0x443020
    void update_alarm();                                // 0x443140
    unsigned pick_shout_target(int flags, float maxdist);   // 0x443270
    unsigned pick_threat();                             // 0x443430
    void play_hit_sound(int* kind);                     // 0x4435c0
    void play_sound_var(int* kind, float* var);         // 0x443730
    void play_sound(int* snd);                          // 0x443870
    void play_pain_sound(float health);                 // 0x443a00
    void update_equipment_display();                    // 0x443aa0
    void on_wield_changed();                            // 0x444370
};


// ---- game data: troop records, parties, slot table (g_game 0x5e2930) ---------------------------------------------
struct TroopRec {                                // 0x508 bytes (array at game+0x13a00)
    tstring id;                                  // +0 (0x4428c0)
    char pad10[0x20 - 0x10];
    tstring name;                                // +0x20 (0x4428c0)
    unsigned flags;                              // +0x30 low nibble = class (0x443870)
    char pad34[0x9c - 0x34];
    int exp;                                     // +0x9c (0x4428c0)
    char pada0[4];
    int fa4, fa8, fac;                           // +0xa4 (fac += 10 per level)
    int level;                                   // +0xb0
    char padb4[0x508 - 0xb4];
};
struct Slot494 { char pad0[0x30]; unsigned char flags; char pad31[0x494 - 0x31]; };   // 0x494 bytes (0x4435c0)
struct PartySlot { int f0; int troop; char pad8[0x18]; };      // 0x20 bytes (0x4428c0)
struct Party {                                   // 0x5ac8 bytes
    char pad0[0xa8];
    PartySlot m[64];                             // +0xa8
    int count;                                   // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0 (index >> 4 -> page of 16)
    int index_of(int id);                        // 0x48ad00
};
struct GameData {
    char pad0[0x78];
    int f78;                                     // +0x78 (0x4428c0)
    char pad7c[0x8c - 0x7c];
    int player;                                  // +0x8c
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                            // +0xf8 (embedded; pages pointer at +0x139c8)
    char pad_x[0x139e4 - 0xf8 - sizeof(PartyMgr)];
    int f139e4;                                  // +0x139e4 (0x443aa0)
    char pad139e8[0x13a00 - 0x139e8];
    union {
        TroopRec* chars;                         // +0x13a00 (0x4428c0)
    };
    char pad13a04[0x13a34 - 0x13a04];
    int f13a34;                                  // +0x13a34 (0x4428c0)
    char pad13a38[0x13a50 - 0x13a38];
    Slot494* slots;                              // +0x13a50 (0x4435c0)
    int skill(int troop, int id);                // 0x437b20 (thiscall, ret 8) (0x4406a0)
    void add_experience(int troop, int amount);  // 0x4428c0 (ret 8) (agent_fx_sound.cpp)
};
extern GameData* g_game;                         // 0x5e2930

// the agent of an id (page table lookup, as the callers inline it)
static __forceinline Agent* agent_by_id(unsigned id)
{
    return &g_mission->agents.pages[id >> 4][id & 0xf];
}

// the same lookup with the page table read as a raw pointer array (0x43f970 compiles it so: 813 vs 837 differing bytes
// with the vector form)
static __forceinline Agent* agent_by_id_raw(unsigned id)
{
    return (Agent*)(((char**)g_mission->agents.pages._Myfirst)[id >> 4] + (id & 0xf) * 0x5928);
}

#endif // AGENT3_TYPES_H
