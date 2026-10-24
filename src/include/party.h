// party.h: the party record (Party, 0x5ac8 bytes; pages of 16 behind PartyMgr::pages), its 0x20-byte troop stacks (Stack)
// and the party manager embedded in the game data at +0xf8 (PartyMgr, game_data.h). ONE definition for src/game (agent3,
// dialog_manager, terrain_b, ui, script) and src/rgl/mesh2/gui_edit.cpp (which called the party record PartyRec). Layout:
// 0x460ca0 (type, id), 0x460b90 (owner), 0x489420..0x489540 (flags at +0xa0, stacks at +0xa8, count at +0x8a8, dirty
// byte at +0x8b4), 0x463190 (stack flags), 0x4c8ca0 (name +0x0c, flag +0x1c, key +0x20, owner +0x8c).
// Overlaid names (anonymous unions of plain ints): Stack count / f0, spawned / f0c, wounded / f10; Party nstacks / count.
#ifndef MB_PARTY_H
#define MB_PARTY_H
#include <vector>
#include "../game/tstring.h"
#include "./timer.h"
#include "./ptr_vec.h"
#include "../game/slot_array.h"
namespace rgl { struct mesh; }
struct PartyV4 { float x, y, z, w; };       // a plain 4-float vector (no constructors: the party record is built by memory copies)

struct Stack {                              // 0x20 bytes, array at party+0xa8
    union {
        int count;                          // +0x00 total / number of troops in the stack   0x489490, 0x4428c0
        int f0;
    };
    int troop;                              // +0x04 troop index   0x489620, 0x4428c0
    int flags;                              // +0x08 bit 0: hero / prisoner stack   0x4894b0, 0x4894e0, 0x489510, 0x463190
    union {
        int spawned;                        // +0x0c
        int f0c;
    };
    union {
        int wounded;                        // +0x10
        int f10;
    };
    int f14, f18, f1c;
    void clear();                           // 0x4893e0 (script: party_remove_members_of_troop / clear paths)
};

struct Party {                              // 0x5ac8 bytes
    int f0;
    int type;                               // +4 (1 = ordinary party)   0x460ca0
    int id;                                 // +8   0x460ca0
    tstring name;                           // +0x0c   0x4c6530
    bool flag;                              // +0x1c
    char pad1d[3];
    tstring key;                            // +0x20   0x4c6530
    PartyV4 pos;                            // +0x30 map position (x, y, z, w)   0x4b2880 place_on_map, 0x4b3910, 0x4b80d0
    float dirx, diry;                       // +0x40 heading   0x4b3ce0, 0x4b80d0
    float oldx, oldy;                       // +0x48 previous heading   0x4b80d0
    float angle;                            // +0x50 facing angle   0x4b3ce0, 0x4b80d0
    tstring extra_text;                     // +0x54 (script party_set_extra_text 0x4a6d03)
    float radius;                           // +0x64 0x4b80d0
    float alpha;                            // +0x68 map-model opacity   0x4b80d0
    int f6c;                                // +0x6c 0x4b7e50
    union {
        int f70;                            // +0x70 troop index into the harvest table   0x4b4a00
        int faction;                        // +0x70 (script party_set_faction 0x4a73ab)
    };
    unsigned f74;                           // +0x74 packed nibbles (low 4: aggression, 0x100: flag)   0x4b4a00
    union {
        float f78;                          // +0x78 0x4b4a00 (alt score weight), 0x4b2c00
        float bandit_attraction;            // +0x78 (script party_set_bandit_attraction 0x4a7b59)
    };
    union {
        float f7c;                          // +0x7c 0x4b2c00 (morale), 0x4b5c30
        float morale;                       // +0x7c 0..1 (script party_set_morale 0x4a8387)
    };
    union {
        float zoom;                         // +0x80 0x4b2c00 (zoom easing), 0x4b5c30
        int f80;                            // +0x80 (script 0x4a4a76)
    };
    char pad84[0x88 - 0x84];
    union {
        int f88;                            // +0x88 battle record index, -1 = none   0x4b1fa0, 0x4b3910, 0x4b4a00
        int battle;                         // +0x88 (script 0x4a814d)
    };
    union {
        int owner;                          // +0x8c   0x460b90
        int template_id;                    // +0x8c (script party_get_template_id 0x4a6d9f)
    };
    char pad90[0x94 - 0x90];
    union {
        int f94;                            // +0x94 rider troop (type table index)   0x4b7840
        int banner_icon;                    // +0x94 (script party_set_banner_icon 0x4a854a)
    };
    char pad98[0xa0 - 0x98];
    union {
        struct {
            int flags_lo;                   // +0xa0 (64-bit flag word; low byte = id)   0x489420, 0x489460 (0x3000 = size class)
            int flags_hi;                   // +0xa4
        };
        unsigned __int64 flags;             // +0xa0 pf_* (script party_is_active 0x4a0462)
        struct { char flags_lo7[7]; unsigned char carry_gold; };    // +0xa7 pf_carry_gold_mask byte (script 0x4a2d0f)
    };
    Stack stacks[64];                       // +0xa8
    union {
        int nstacks;                        // +0x8a8
        int count;
        int num_stacks;                     // +0x8a8 (script main_party_has_troop 0x4a0349)
    };
    float f8ac;                             // +0x8ac 0x4b20b0 (speed factor)
    float f8b0;                             // +0x8b0 0x4b2120 (set from perk 0x1a), 0x4b20b0
    char dirty;                             // +0x8b4
    char f8b5;                              // +0x8b5 0x4b7e50 (kill-feed visible)
    char pad8b6[2];
    ptr_vec attached;                       // +0x8b8 std::vector<int> of attached party ids (allocator word, first, last, end)   0x4b4800, 0x4b6180
    union {
        int f8c8;                           // +0x8c8 attached-to party, -1 = none   0x4b2c00, 0x4b3910
        int attached_to;                    // +0x8c8 (script party_get_attached_to 0x4a87e2)
    };
    char f8cc;                              // +0x8cc in mission   0x4b6180
    char pad8cd[0x8d0 - 0x8cd];
    Timer clk8d0;                           // +0x8d0 re-plan timer   0x4b80d0
    union {
        float interval;                     // +0x8e0 re-plan interval   0x4b80d0, 0x4b3ce0
        int ai_state;                       // +0x8e0 (script party_set_ai_behavior 0x4a79bb)
    };
    union {
        int state;                          // +0x8e4 pending action   0x4b4a00
        int f8e4;                           // +0x8e4 scripted ai behaviour (script add_companion_party: 6)
    };
    union {
        int state_target;                   // +0x8e8
        int ai_target_object;               // +0x8e8 (script party_set_ai_object 0x4a7a12)
    };
    union {
        int state_f8ec;                     // +0x8ec
        float ai_target_x;                  // +0x8ec (script party_set_ai_target_position 0x4a7a6e)
    };
    union {
        int state_f8f0;                     // +0x8f0
        float ai_target_y;                  // +0x8f0
    };
    union {
        float f8f4;                         // +0x8f4 0x4b3ce0
        float ai_patrol_radius;             // +0x8f4 (script party_set_ai_patrol_radius 0x4a7abc)
    };
    int f8f8;                               // +0x8f8 clock index   0x4b3ce0
    char pad8fc[4];
    int f900;                               // +0x900 0x4b3ce0 (clock lo)
    int f904;                               // +0x904 (clock hi)
    Timer clk908;                           // +0x908 footstep timer   0x4b80d0
    union {
        int action;                         // +0x918 map order (ai behaviour)   0x4b3910, 0x4b3ce0, 0x4b4a00
        int ai_behavior;                    // +0x918 ai_bhvr_* (script party_is_in_town 0x4a03d1)
    };
    union {
        int target;                         // +0x91c order target   0x4b3ce0
        int ai_object;                      // +0x91c (script entering_town 0x49fb99)
    };
    int f920;                               // +0x920
    union {
        PartyV4 order_pos;                  // +0x924 order position   0x4b3bc0, 0x4b3ce0
        struct { int f924, f928; float vx, vy; };    // 0x4b4a00 (int copies), 0x4b7460 (velocity jitter)
    };
    union {
        int order_state;                    // +0x934 0x4b3bc0
        float t934;                         // 0x4b80d0 (next order-plan delay)
    };
    int idx938;                             // +0x938 clock index   0x4b80d0
    char pad93c[4];
    __int64 t940;                           // +0x940 0x4b80d0
    Timer clk948;                           // +0x948 0x4b80d0
    union {
        float f958;                         // +0x958 aggression scale   0x4b4a00
        float ai_initiative;                // +0x958 (script party_set_ai_initiative 0x4a7921)
    };
    union {
        struct { float goal_x, goal_y; };   // +0x95c path goal (start_path 0x4b73c0, step_route 0x4b4320)
        struct { int goal_xi, goal_yi; };   // 0x4b2880 (int copies)
    };
    union {
        struct { float dest_x, dest_y; };   // +0x964 current destination   0x4b80d0, 0x4b7460
        struct { int dest_xi, dest_yi; };   // 0x4b4320 / 0x4b2880 (int copies)
    };
    int path[0x400];                        // +0x96c node-id chain   0x4b73c0, 0x4b6b40
    float detail[0x400 * 4];                // +0x196c per-node positions
    int cursor;                             // +0x596c path cursor   0x4b4320
    int npath;                              // +0x5970 path length
    struct face* mesh;                      // +0x5974 current nav-mesh face   0x4b2880, 0x4b80d0
    union {
        unsigned map_mask;                  // +0x5978 map record mask under the party   0x4b2880, 0x4b3910
        int current_terrain;                // +0x5978 (script party_get_current_terrain 0x4a6d53)
    };
    float f597c;                            // +0x597c 0x4b2880
    int moving;                             // +0x5980
    int f5984;                              // +0x5984 map record index the party is on
    int f5988;                              // +0x5988 needs repath
    int f598c;                              // +0x598c
    float f5990, f5994;                     // +0x5990 0x4b2ab0
    int kind;                               // +0x5998 order kind   0x4b2970
    union {
        int target_pos[2];                  // +0x599c 0x4b2970 (int copies)
        struct { float lx, ly; };           // 0x4b7460
    };
    struct face* tgt;                       // +0x59a4 target face   0x4b2970
    int tgt_id;                             // +0x59a8 target party id   0x4b2ab0
    char pad59ac[0x59c0 - 0x59ac];
    struct PartyModel* model;               // +0x59c0 0x4b32f0 (release), 0x4b80d0
    float trail_len;                        // +0x59c4 0x4b80d0
    PartyV4 trail;                          // +0x59c8 last event position   0x4b2880, 0x4b80d0
    SlotArray slots;                        // +0x59d8 (script party_set_slot 0x4a0815)
    char pad59d9[0x59e8 - 0x59d9];
    rgl::mesh* label;                       // +0x59e8 rgl::mesh* of the hud label   0x4b7e50, 0x49bea0
    rgl::mesh* e59ec;                       // +0x59ec 0x4b7840, 0x4b80d0
    rgl::mesh* e59f0;                       // +0x59f0
    ptr_vec deferred;                       // +0x59f4 std::vector<int> (allocator word; first at +0x59f8) emitter ids   0x4b7840
    char f5a04;                             // +0x5a04
    char pad5a05[0x5a08 - 0x5a05];
    int note_times[8];                      // +0x5a08 (script add_party_note_from_dialog 0x4a35fb)
    tstring notes[8];                       // +0x5a28 (script 0x4a353c)
    int note_tableau;                       // +0x5aa8 (script add_party_note_tableau_mesh 0x4a31a6)
    int pad5aac;
    union {
        Timer ignore_timer;                 // +0x5ab0 (script: party_ignore_player 0x4a7b0c)   0x4b4a00
        Timer ignore_player_timer;
    };
    int visited;                            // +0x5ac0 nodes visited by the last path search   0x4b73c0
    char pad5ac4[0x5ac8 - 0x5ac4];

    int sum_stat(int what);                 // 0x4b1cf0 (thiscall, ret 4)
    tstring translated();                   // 0x4c6530
    int first_flagged();                    // 0x463190 (index of the first prisoner stack, else the count)
    void set_id(int v);                     // 0x489420
    float scale();                          // 0x489460
    int sum_all();                          // 0x489490
    int sum_head();                         // 0x4894b0
    int sum_flagged();                      // 0x4894e0
    int count_tail();                       // 0x489510
    int* remove_row(int* out, int index, int* arr);          // 0x489540
    int find_or_add(unsigned troop, unsigned flag, char force);   // 0x489620
    int max_members();                      // 0x4b22d0
    int max_prisoners();                    // 0x4b2330
    int max_4b1700();                       // 0x4b1700
    // ---- src/game/mission_ui2 (0x48c330..0x48d7e0; method declarations only) ----
    void remove_slot(int index);            // 0x48c330 (ret 4: moves a non-prisoner stack up behind the prisoner stacks)
    int consume_stack(int index, int amount, int* rows, char keep);   // 0x48c3c0 (ret 0x10: takes `amount` from stack `index`, removes it when empty)
    int add_members(int troop, int count, int flags, int wounded, char force);   // 0x48c840 (ret 0x14)
    tstring* display_name();                // 0x48d7b0 (the key's translation, or the name)
    void init();                            // 0x48f180 (resets the map party's order, path, speed and per-frame state)
    void clear_particle_systems();          // 0x48d7e0 (frees the id list at +0x59f4, then calls slot 0x68 of the model at +0x59c0)
    // ---- src/game/party2 (0x4b1160..0x4b8f40): the map party's economy, orders, navigation and per-frame update ----
    bool update();                          // 0x4b1b80
    int slot_value(int idx, unsigned flags) const;      // 0x4b1be0 (ret 8)
    int find_worth(int value, unsigned flags) const;    // 0x4b1d30 (ret 8)
    void add_xp_to_stack(int idx, int amount);          // 0x4b1d80 (ret 8)
    void prepare();                         // 0x4b1eb0 (heroes first)
    void rotate_stacks(int from, int to);   // 0x4b1710 (ret 8: move stack `to` down to `from`)
    float strength();                       // 0x4b1fa0
    float speed_factor();                   // 0x4b20b0
    void set_speed_from_stat();             // 0x4b2120
    int skill_level(int skill);             // 0x4b2370 (ret 4)
    float map_speed();                      // 0x4b25c0
    void place_on_map();                    // 0x4b2880
    void set_order_face(int* pos, struct face* f);      // 0x4b1780 (ret 8)
    void set_order_pos(int* pos);           // 0x4b2970 (ret 4)
    void set_order_party(int* id);          // 0x4b29d0 (ret 4)
    void set_order_agent(int* id);          // 0x4b2ab0 (ret 4)
    void tick_stats();                      // 0x4b2c00
    void clear_map_flag();                  // 0x4b32f0
    void collect_targets(int* out, int* max, int* count);   // 0x4b3910 (ret 0xc)
    void reissue_order();                   // 0x4b3bc0
    void plan_order();                      // 0x4b3ce0
    void step_route();                      // 0x4b4320
    int force_add_members(int troop, int count, unsigned flags, int f10);   // 0x4b4470 (ret 0x10)
    void think();                           // 0x4b4a00
    void leave_battle();                    // 0x4b6ae0
    bool start_path(int from, float tx, float ty);      // 0x4b73c0 (ret 0xc)
    void update_nav();                      // 0x4b7460
    void update_flags();                    // 0x4b7840
    void update_hud_text();                 // 0x4b7e50
    void tick(float dt);                    // 0x4b80d0 (ret 4)
};

struct PartyMgr {                           // 0x138e0 bytes, embedded in the game data at +0xf8
    char pad0[0x138b8];
    int count;                              // +0x138b8   0x460ca0
    char pad138bc[0x138cc - 0x138bc];
    std::vector<Party*> pages;              // +0x138cc (first at +0x138d0; index >> 4 -> page of 16). A std::vector: terrain_b's reset_spawned 0x4739f0 indexes it
                                            // through operator[] (lea + load); the raw readers (dialog_manager, agent3, gui_edit) use pages._Myfirst[...]
    union {
        int deflt;                          // +0x138dc   0x460ca0, 0x460de0
        unsigned n_parties;                 // +0x138dc (GameData+0x139d4)   0x4c8ca0
    };
    int index_of(int id);                   // 0x48ad00 (thiscall, ret 4)
    int find(unsigned id);                  // 0x48ad00
    Party* get(int id);                     // 0x51ee50 (thiscall, ret 4)
    Party* get_party(unsigned id) { int idx = find(id); return (Party*)((char*)pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x5ac8); }
    void next_party_id(int* id);            // 0x460ca0
    void next_party_id(unsigned* id);       // 0x460ca0
    int first_party_id();                   // 0x460d20
    bool is_live(int id);                   // 0x4903e0 (thiscall, ret 4: id names a live party) (src/game/party2)
};

typedef char mb_party_size_chk[sizeof(Party) == 0x5ac8 ? 1 : -1];

#endif // MB_PARTY_H
