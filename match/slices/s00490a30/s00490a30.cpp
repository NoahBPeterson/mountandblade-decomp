// Slice s00490a30: party membership maintenance on the world object (the party manager is at +0xf8).
// 0x490a30 removes a party from its leader's follower list; 0x490ce0 totals a party's stacks into caller
// counters; 0x490fa0 recursively collects a party and its sub-parties. Names provisional.
#include <windows.h>
#include <stdlib.h>
#include <string.h>

struct Stack {                                  // 0x20 bytes, array at party+0xa8
    int count;                                  // +0x00
    int troop;                                  // +0x04
    int flags;                                  // +0x08 (bit 0 tested as a byte at +0xb0)
    int f0c;
    int f10;                                    // +0x10
    int f14;
    int f18;
    int f1c;
};
struct Party {                                  // 0x5ac8-byte party record
    int f0;
    int type;                                   // +0x04 (== 1 tests the "slot" kind)
    char pad8[0xa8 - 8];
    Stack stacks[64];                           // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    char dirty;                                 // +0x8b4
    char pad8b5[0x8bc - 0x8b5];
    int* followers;                             // +0x8bc (vector of party ids)
    int* followers_end;                         // +0x8c0
    int* followers_cap;                         // +0x8c4
    int leader;                                 // +0x8c8 (leader party id, -1 when none)
    char flag8cc;                               // +0x8cc
    char pad8cd[0x5ac8 - 0x8cd];
};
struct Mgr {
    char pad0[0x138d0];
    Party** pages;                              // +0x138d0 (page of 16 parties)
    Party** pages_end;                          // +0x138d4
    Party** pages_cap;                          // +0x138d8
    unsigned count;                             // +0x138dc (party id bound)
    int index_of(unsigned id);                  // 0x48ad00 (thiscall, ret 4)
};
inline Party* party_at(Mgr* m, int idx) { return m->pages[idx >> 4] + (idx & 0xf); }

// ---- global state -----------------------------------------------------------------------------------------
struct TroopRec {                               // 0x508 bytes, table at world+0x13a00
    char pad0[0x30];
    unsigned flags;                             // +0x30 (0x10 = hero)
    char pad34[0x508 - 0x34];
};
bool agent_low(int troop);                      // 0x4638f0 (ret 4)

struct World {
    char pad0[0xf8];
    Mgr mgr;                                    // +0xf8; its count is world+0x139d4
    char pad139d8[0x13a00 - 0x139d8];
    TroopRec* troops;                           // +0x13a00
    void drop_party(unsigned id);                       // 0x490a30
    void count_party(unsigned id, int* a, int* b, int* c, int* d);  // 0x490ce0
    void collect(unsigned id, int* out, int* n);        // 0x490fa0
};

// @ 0x490a30
void World::drop_party(unsigned id)
{
    Mgr* m = &mgr;
    if (id >= mgr.count)
        return;
    int i = m->index_of(id);
    if (i < 0)
        return;
    if (party_at(m, (unsigned)i)->type != 1)
        return;
    if (party_at(m, m->index_of(id))->leader < 0)
        return;
    unsigned* dst = (unsigned*)party_at(m,
        m->index_of(party_at(m, m->index_of(id))->leader))->followers;
    unsigned* cur = dst + 1;
    bool done = false;
    do {
        unsigned leader = party_at(m, m->index_of(id))->leader;
        if (party_at(m, m->index_of(leader))->followers_end <= (int*)dst)
            break;
        if (*dst == id) {
            unsigned l2 = party_at(m, m->index_of(id))->leader;
            Party* lp = party_at(m, m->index_of(l2));
            memmove(dst, cur,
                    (int)(lp->followers_end - (int*)cur) * 4);
            lp->followers_end -= 1;
            done = true;
        }
        dst++;
        cur++;
    } while (!done);
    party_at(m, m->index_of(party_at(m, m->index_of(id))->leader))->dirty = 1;
    party_at(m, m->index_of(id))->flag8cc = 0;
    party_at(m, m->index_of(id))->dirty = 1;
    party_at(m, m->index_of(id))->leader = -1;
}

// @ 0x490ce0
void World::count_party(unsigned id, int* a, int* b, int* c, int* d)
{
    Mgr* m = &mgr;
    if (id >= mgr.count)
        return;
    int i = m->index_of(id);
    if (i < 0)
        return;
    if (party_at(m, (unsigned)i)->type != 1)
        return;
    if (party_at(m, m->index_of(id))->nstacks > 0) {
        int k = 0;
        int n = 0;
        do {
            if ((*(unsigned char*)((char*)party_at(m, m->index_of(id)) + 0xb0 + k) & 1) != 0) {
                *c += *(int*)((char*)party_at(m, m->index_of(id)) + 0xa8 + k);
            } else {
                *a += *(int*)((char*)party_at(m, m->index_of(id)) + 0xa8 + k);
                int troop = *(int*)((char*)party_at(m, m->index_of(id)) + 0xac + k);
                if ((troops[troop].flags & 0x10) == 0) {
                    *b = *b + *(int*)((char*)party_at(m, m->index_of(id)) + 0xb8 + k);
                } else {
                    *d = *d + 1;
                    if (agent_low(*(int*)((char*)party_at(m, m->index_of(id)) + 0xac + k))) {
                        *b = *b + 1;
                    }
                }
            }
            n++;
            k += 0x20;
        } while (n < party_at(m, m->index_of(id))->nstacks);
    }
    for (unsigned j = 0; ; j++) {
        Party* p = party_at(m, m->index_of(id));
        if (p->followers == 0)
            break;
        if (j >= (unsigned)(p->followers_end - p->followers))
            break;
        count_party(party_at(m, m->index_of(id))->followers[j], a, b, c, d);
    }
}

// @ 0x490fa0
void World::collect(unsigned id, int* out, int* n)
{
    Mgr* m = &mgr;
    if (id >= mgr.count)
        return;
    int i = m->index_of(id);
    if (i < 0)
        return;
    if (party_at(m, (unsigned)i)->type != 1)
        return;
    if (*n >= 0x80)
        return;
    out[*n] = (int)id;
    (*n)++;
    for (unsigned j = 0; ; j++) {
        Party* p = party_at(m, m->index_of(id));
        if (p->followers == 0)
            break;
        if (j >= (unsigned)(p->followers_end - p->followers))
            break;
        collect(party_at(m, m->index_of(id))->followers[j], out, n);
    }
}
