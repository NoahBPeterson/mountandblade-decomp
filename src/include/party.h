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
};

struct Party {                              // 0x5ac8 bytes
    int f0;
    int type;                               // +4 (1 = ordinary party)   0x460ca0
    int id;                                 // +8   0x460ca0
    tstring name;                           // +0x0c   0x4c6530
    bool flag;                              // +0x1c
    char pad1d[3];
    tstring key;                            // +0x20   0x4c6530
    char pad30[0x8c - 0x30];
    int owner;                              // +0x8c   0x460b90
    char pad90[0xa0 - 0x90];
    int flags_lo;                           // +0xa0 (64-bit flag word; low byte = id)   0x489420, 0x489460 (0x3000 = size class)
    int flags_hi;                           // +0xa4
    Stack stacks[64];                       // +0xa8
    union {
        int nstacks;                        // +0x8a8
        int count;
    };
    char pad8ac[0x8b4 - 0x8ac];
    char dirty;                             // +0x8b4
    char pad8b5[0x5ac8 - 0x8b5];

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
};

#endif // MB_PARTY_H
