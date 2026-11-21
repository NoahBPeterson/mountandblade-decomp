// party.h: the party record (Party, 0x5ac8 bytes; pages of 16 behind PartyMgr::pages), its 0x20-byte troop stacks (Stack)
// and the party manager embedded in the game data at +0xf8 (PartyMgr, game_data.h). ONE definition for src/game (agent3,
// dialog_manager, terrain_b, ui, script) and src/rgl/mesh2/gui_edit.cpp (which called the party record PartyRec). Layout:
// 0x460ca0 (type, id), 0x460b90 (owner), 0x489420..0x489540 (flags at +0xa0, stacks at +0xa8, count at +0x8a8, dirty
// byte at +0x8b4), 0x463190 (stack flags), 0x4c8ca0 (name +0x0c, flag +0x1c, key +0x20, owner +0x8c).
// Overlaid names (anonymous unions of plain members): Party nstacks / count.
#ifndef MB_PARTY_H
#define MB_PARTY_H
#include <vector>
#include "layout32.h"
#include "../game/tstring.h"
#include "./note_block.h"
#include "./timer.h"
#include "./iface.h"
#include "../game/slot_array.h"
#include "./map10007.h"
#include "./pool.h"
#include "./vec.h"
#include "../rgl/rgl_math.h"            // rgl::vector4 (pos, order_pos, trail, detail)
namespace rgl { struct mesh; struct strategic_entity; }
struct stream_base;
struct stream_base;
struct Party;
// a Timer started on clock 1 by its constructor (the Party members; clk908 is a gui_timer, clock 0)
struct clock1_timer : Timer {
    clock1_timer() { clock = 1; start = g_ticks[1]; }
    void rewind() { start = g_ticks[clock] - 100000; }   // inline: one second in the past (as gui_timer::rewind)
};

struct Stack {                              // 0x20 bytes, array at party+0xa8
    int count;                              // +0x00 total / number of troops in the stack   0x489490, 0x4428c0
    int troop;                              // +0x04 troop index   0x489620, 0x4428c0
    int flags;                              // +0x08 bit 0: hero / prisoner stack   0x4894b0, 0x4894e0, 0x489510, 0x463190
    int spawned;                            // +0x0c
    int wounded;                            // +0x10
    int f14;                                // +0x14 (save word after xp: party_2.cpp counts the stack's pending upgrades in it)
    float xp;                               // +0x18 experience gathered towards the next upgrades (party_2.cpp add_xp; the save files hold it as a float)
    int f1c;
    Stack() { count = 0; troop = -1; flags = 0; f14 = 0; spawned = 0; wounded = 0; xp = 0; }   // inline (Party::Party 0x4934d0 builds the array with it)
    void clear();                           // 0x4893e0 (script: party_remove_members_of_troop / clear paths)
};

// ONE class: the compiler emits its vtable 0x592644 (slot 0 the vector deleting destructor 0x431cb0 of `delete[]` / `new Party[16]`, slot 1 the
// reset 0x4936a0, slot 2 IfaceA's empty 0x477b70), the constructor 0x4934d0 (world_data) and the implicit destructor 0x431b90.
struct Party : IfaceA {                     // 0x5ac8 bytes
    Party() throw();                        // 0x4934d0 (src/game/world_data/world_data.cpp)
    virtual void slot1();                   // +0x04 0x4936a0 (tail jump to init)
    int type;                               // +4 (1 = ordinary party)   0x460ca0
    int id;                                 // +8   0x460ca0
    tstring name;                           // +0x0c   0x4c6530
    bool flag;                              // +0x1c
    tstring key;                            // +0x20   0x4c6530
    rgl::vector4 pos;                            // +0x30 map position (x, y, z, w)   0x4b2880 place_on_map, 0x4b3910, 0x4b80d0
    Vec2 dir;                               // +0x40 heading   0x4b3ce0, 0x4b80d0 (reset 0x48f180 assigns vector2 temporaries)
    Vec2 old_dir;                           // +0x48 previous heading   0x4b80d0
    float angle;                            // +0x50 facing angle   0x4b3ce0, 0x4b80d0
    tstring extra_text;                     // +0x54 (script party_set_extra_text 0x4a6d03)
    float radius;                           // +0x64 0x4b80d0
    float alpha;                            // +0x68 map-model opacity   0x4b80d0
    int in_sight;                           // +0x6c 1 while the party is within sight of the main party (GameData::update_cursor 0x5179f0 raises
                                            //   map events 0xc / 0xd on the change; 0x4b7e50, map_panel 0x51ba69)
    int faction;                            // +0x70 (script party_set_faction 0x4a73ab; the template's faction, add_sprite 0x49ebd0; 0x4b4a00)
    unsigned personality;                   // +0x74 the template's personality: packed nibbles (low 4: aggressiveness, bits 4..7: courage,
                                            //   0x100: banditness)   0x4b4a00, add_sprite 0x49ebd0
    float bandit_attraction;                // +0x78 (script party_set_bandit_attraction 0x4a7b59; 0x4b4a00, 0x4b2c00)
    float morale;                           // +0x7c 0..1 (script party_set_morale 0x4a8387; 0x4b2c00, 0x4b5c30)
    union {
        float zoom;                         // +0x80 0x4b2c00 (zoom easing), 0x4b5c30
        int f80;                            // +0x80 (script 0x4a4a76)
    };
    float f84;                          // +0x84 (read 0x51ee80)
    int battle;                             // +0x88 battle record index, -1 = none (script 0x4a814d; 0x4b1fa0, 0x4b3910, 0x4b4a00)
    union {
        int owner;                          // +0x8c   0x460b90
        int template_id;                    // +0x8c (script party_get_template_id 0x4a6d9f)
    };
    int menu;                               // +0x90 the encounter menu (GameData::add_sprite 0x49ebd0: the party template's menu, +0x28)
    int banner_icon;                        // +0x94 (script party_set_banner_icon 0x4a854a; 0x4b7840)
    float f98;                              // +0x98 1.0 after the reset (init 0x48f180)
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
    float base_speed;                       // +0x8ac the march speed (update_hud_text 0x4b7e50; speed_factor 0x4b20b0)
    float speed_multiplier;                 // +0x8b0 the script's party speed multiplier / 100 (event 0x1a: set_speed_from_stat 0x4b2120, 0x4b7e50)
    char dirty;                             // +0x8b4
    char label_dirty;                       // +0x8b5 the map label needs rebuilding (update_hud_text 0x4b7e50; set by party_set_flags, the faction marks 0x490990)
    std::vector<int> attached;              // +0x8b8 attached party ids (allocator word, first, last, end)   0x4b4800, 0x4b6180
    int attached_to;                        // +0x8c8 attached-to party, -1 = none (script party_get_attached_to 0x4a87e2; 0x4b2c00, 0x4b3910)
    char engaged;                           // +0x8cc set with an objective / an encounter, cleared when detached   0x4b6180, 0x490990
    clock1_timer clk8d0;                    // +0x8d0 re-plan timer   0x4b80d0
    union {
        float interval;                     // +0x8e0 re-plan interval   0x4b80d0, 0x4b3ce0
        int ai_state;                       // +0x8e0 (script party_set_ai_behavior 0x4a79bb)
    };
    int state;                              // +0x8e4 pending action / scripted ai behaviour (script add_companion_party: 6)   0x4b4a00
    union {
        int state_target;                   // +0x8e8
        int ai_target_object;               // +0x8e8 (script party_set_ai_object 0x4a7a12)
    };
    Vec2 ai_target;                         // +0x8ec (script party_set_ai_target_position 0x4a7a6e; copied as two dwords)
    float ai_patrol_radius;                 // +0x8f4 (script party_set_ai_patrol_radius 0x4a7abc; 0x4b3ce0)
    clock1_timer clk8f8;                    // +0x8f8 (constructor 0x4934d0: clock 1; 0x4b3ce0 reads clock and start)
    gui_timer clk908;                       // +0x908 footstep timer   0x4b80d0
    union {
        int action;                         // +0x918 map order (ai behaviour)   0x4b3910, 0x4b3ce0, 0x4b4a00
        int ai_behavior;                    // +0x918 ai_bhvr_* (script party_is_in_town 0x4a03d1)
    };
    union {
        int target;                         // +0x91c order target   0x4b3ce0
        int ai_object;                      // +0x91c (script entering_town 0x49fb99)
    };
    int f920;                               // +0x920
    rgl::vector4 order_pos;                 // +0x924 order position (x, y: 0x4b3bc0, 0x4b3ce0, 0x4b4a00; z, w: 0x4b7460's velocity jitter)
    union {
        int order_state;                    // +0x934 0x4b3bc0
        float t934;                         // 0x4b80d0 (next order-plan delay)
    };
    clock1_timer clk938;                    // +0x938 (constructor 0x4934d0: clock 1; 0x4b80d0 reads clock and start)
    clock1_timer clk948;                    // +0x948 0x4b80d0
    float ai_initiative;                    // +0x958 aggression scale (script party_set_ai_initiative 0x4a7921; 0x4b4a00)
    Vec2 goal;                              // +0x95c path goal (start_path 0x4b73c0, step_route 0x4b4320)
    Vec2 dest;                              // +0x964 current destination   0x4b80d0, 0x4b7460 (struct copies: two dwords)
    int path[0x400];                        // +0x96c node-id chain   0x4b73c0, 0x4b6b40
    rgl::vector4 detail[0x400];             // +0x196c per-node positions
    int cursor;                             // +0x596c path cursor   0x4b4320
    int npath;                              // +0x5970 path length
    struct face* mesh;                      // +0x5974 current nav-mesh face   0x4b2880, 0x4b80d0
    union {
        unsigned map_mask;                  // +0x5978 map record mask under the party   0x4b2880, 0x4b3910
        int current_terrain;                // +0x5978 (script party_get_current_terrain 0x4a6d53)
    };
    float f597c;                            // +0x597c 0x4b2880
    int moving;                             // +0x5980
    int face_id;                            // +0x5984 the id (m4) of the nav-mesh face the party is on, -1 = none (init 0x48f180)
    int needs_repath;                       // +0x5988
    int f598c;                              // +0x598c
    Vec2 tgt_seen_pos;                      // +0x5990 the followed party's position when the route was planned (set_order_agent 0x4b2ab0)
    int kind;                               // +0x5998 order kind   0x4b2970
    union {
        int target_pos[2];                  // +0x599c 0x4b2970 (int copies)
        struct { float lx, ly; };           // 0x4b7460
    };
    struct face* tgt;                       // +0x59a4 target face   0x4b2970
    int tgt_id;                             // +0x59a8 target party id   0x4b2ab0
    int f59ac;                              // +0x59ac
    clock1_timer clk59b0;                   // +0x59b0 (constructor 0x4934d0: clock 1; its start is re-set to 1 s back after init)
    rgl::strategic_entity* model;           // +0x59c0 the map model (world::create_entity(9)) 0x4b32f0 (release), 0x4b80d0
    float trail_len;                        // +0x59c4 0x4b80d0
    rgl::vector4 trail;                          // +0x59c8 last event position   0x4b2880, 0x4b80d0
    SlotArray slots;                          // +0x59d8 (script party_set_slot 0x4a0815)
    rgl::mesh* label;                       // +0x59e8 rgl::mesh* of the hud label   0x4b7e50, 0x49bea0
    rgl::mesh* e59ec;                       // +0x59ec 0x4b7840, 0x4b80d0
    rgl::mesh* e59f0;                       // +0x59f0
    std::vector<int> deferred;              // +0x59f4 emitter ids (allocator word; first at +0x59f8)   0x4b7840
    char f5a04;                             // +0x5a04
    SoaBlock notes;                     // +0x5a08 the notes block (add_*_note_from_dialog, read_soa 0x51e1f0)
    clock1_timer ignore_timer;              // +0x5ab0 (script: party_ignore_player 0x4a7b0c)   0x4b4a00
    int visited;                            // +0x5ac0 nodes visited by the last path search   0x4b73c0

    int sum_stat(int what);                 // 0x4b1cf0 (thiscall, ret 4)
    tstring translated();                   // 0x4c6530
    int first_flagged();                    // 0x463190 (index of the first prisoner stack, else the count)
    __declspec(noinline) void read(stream_base* s, int flag);   // 0x51ee80 (ret 8: the save-game reader)
    void write(stream_base* out);             // 0x4e9d10 (ret 4: the save-game writer, src/game/char_sheet/save_records.cpp)
    void set_name(const tstring& s);        // 0x489400 (ret 4: the name, then flag)
    bool at_destination();                  // 0x51be20 (inventory_panel/map_tooltip.cpp: pos.xy == dest)
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
    int max_stacks(); // 0x4b1700
    // ---- src/game/mission_ui2 (0x48c330..0x48d7e0; method declarations only) ----
    void remove_slot(int index);            // 0x48c330 (ret 4: moves a non-prisoner stack up behind the prisoner stacks)
    int consume_stack(int index, int amount, int* rows, char keep);   // 0x48c3c0 (ret 0x10: takes `amount` from stack `index`, removes it when empty)
    int add_members(int troop, int count, int flags, int wounded, char force);   // 0x48c840 (ret 0x14)
    tstring* display_name();                // 0x48d7b0 (the key's translation, or the name)
    void init();                            // 0x48f180 (resets the map party's order, path, speed and per-frame state)
    void add_emitter(int e);                // 0x4955e0 (world_data.cpp: spawns emitter e from g_emit_defs, hands it to the model, remembers the id)
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
    int skill_level(int skill);             // 0x4b2370 (ret 4)  // old name of Party::skill_level, removed after the names merge
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

// The party manager: the id -> index hash (the base), then the party records' pool (include/pool.h, at +0x138b8: count, free_ids
// +0x138bc, pages +0x138cc, serial +0x138dc = the next party id, f28 +0x138e0). The two words after it (game +0x139dc, +0x139e0)
// are the game object's own: the map's sun light (Game::wind) and a word nothing names yet.
struct PartyMgr : Map10007, Pool<Party> {   // 0x138e4 bytes, embedded in the game data at +0xf8 (Game::scene)
    PartyMgr();                             // 0x51d250 (src/game/party/game.cpp)
    __declspec(noinline) Party* get(int id);   // 0x51ee50 (thiscall, ret 4)
    Party* get_party(unsigned id) { int idx = lookup(id); return (Party*)((char*)(&*pages.begin())[idx >> 4] + (idx & 0xf) * sizeof(Party)); }
    Party* get_w(unsigned id) { int i = lookup(id); return at(i); }   // the inline lookup: index_of, then the page (by-value parameter)
    void clear();                           // 0x490340 (frees the pages and the free list; world_data)
    PartyMgr* grow();                       // 0x493830 (adds a page of 16 records; world_data)
    int add_slot();                         // 0x493960 (the new party's id; world_data)
    __declspec(noinline) void read(stream_base* s, int flag);   // 0x51eb90 (ret 8; src/game/mission5/troop_rec.cpp)
    void write_table(stream_base* out);       // 0x4e9c50 (ret 4: the live records, src/game/char_sheet/save_records.cpp)
    void flush_ids();                       // 0x4e9fe0 (drops the records whose type is 2)
    void id_remove(int id);                 // 0x4ea030 (ret 4: removes the id from the hash, frees the record)
    __declspec(noinline) void reset_ids();  // 0x51eda0 (empties the id hash, frees every record)
    void next_party_id(int* id);            // 0x460ca0
    int first_party_id();                   // 0x460d20
    int is_live(int id);                    // 0x4903e0 (thiscall, ret 4: id names a live party) (src/game/party2)
};

MB_LAYOUT32_ASSERT(mb_party_size_chk, sizeof(Party) == 0x5ac8);
MB_LAYOUT32_ASSERT(mb_partymgr_size_chk, sizeof(PartyMgr) == 0x138e4);

#endif // MB_PARTY_H
