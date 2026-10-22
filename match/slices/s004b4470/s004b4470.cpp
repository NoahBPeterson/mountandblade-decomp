// Slice s004b4470: the party inventory / "spawn troop" helpers of the world map object.
//   0x4b4470  Party::add_stack       (append a 0x20-byte stack row: promote an existing stack in the other
//                                    slot class when there is room, otherwise push a new row and rebuild it)
//   0x4b4700  PartyMgr::refresh_party (cap the "spawned" counter of every stack and drop the empty tail)
//   0x4b4800  GameData::party_value  (sum the equipment value of a party's stacks, recursing into its extras)
//   0x4b49d0  alloc_item             (construct one 0x5c-byte item record and return it)
// Names provisional; the layouts follow s0048f8f0 (Stack / Party stacks / PartyMgr pages), s004b1b20
// (InventorySlot rows at +0xa8, update() at 0x4b1b80) and s004b4a00 (GameData, party_value).
#include <string.h>
#include <stddef.h>

// ---- engine helpers -----------------------------------------------------------------------------------------
struct Troop;                                   // 0x508-byte troop record, table at g_game + 0x13a00
struct PartyMgr;
struct Party;

int   __cdecl make_item(int* out, int idx, int* src, unsigned flags);  // 0x4b3350 (cdecl, ret 0x14)
int*  __cdecl alloc(void* dst, int count, int src, void* ctx, int extra);  // 0x4b3350 (cdecl, fills `count`
                                                                          // 0x5c-byte items at dst)
struct Party* __cdecl party_part(PartyMgr* mgr, int idx);   // returns &pages[idx>>4][idx&0xf]

// ---- the party (0x5ac8 bytes) -------------------------------------------------------------------------------
struct Stack {                                  // 0x20 bytes, array at party + 0xa8
    int count;                                  // +0x00
    int troop;                                  // +0x04  troop index
    unsigned flags;                             // +0x08  bit 0 = class (hero/spawned)
    int f0c;                                    // +0x0c
    int f10;                                    // +0x10
    int spawned;                                // +0x14  "spawned" counter, never above count
    int f18;                                    // +0x18
    int f1c;                                    // +0x1c
};

struct Party {
    char pad0[8];
    int id;                                     // +0x08  party id
    char padc[0xa8 - 0xc];
    Stack stacks[0x40];                         // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    char dirty;                                 // +0x8b4
    char pad8b5[0x8b8 - 0x8b5];
    int* extra_first;                           // +0x8b8  extra troop ids (std::vector<int>)
    int* extra_last;                            // +0x8bc
    int* extra_end;                             // +0x8c0
    char pad8c4[0x5ac8 - 0x8c4];

    int  add_stack(int troop, int count, unsigned flags, int f0c, int a5);   // 0x4b4470 (ret 0x10)
    int  remove_row(int* out, int idx, int* table);                          // 0x489540 (ret 0xc)
    void remove_slot(int idx);                                               // 0x48c330 (ret 4)
    int  add_to_stack(int id, int troop, int amount, char flag, int a5);     // 0x48f9f0 (ret 0x14)
    bool update();                                                           // 0x4b1b80
    void* alloc_item(int a, int b, int c);                                  // 0x4b49d0
};
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                              // +0x138d0 (g_game + 0x139c8)
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                             // +0x138dc (g_game + 0x139d4)
    int index_of(int id);                       // 0x48ad00
    void refresh_party(unsigned id);            // 0x4b4700
};

// ---- game data (0x5e2930) -----------------------------------------------------------------------------------
struct Troop {
    char pad0[0x30];
    unsigned flags;                             // +0x30  bit 0x10 = hero
    char pad34[0xb0 - 0x34];
    int level;                                  // +0xb0
};

struct GameData {
    char pad0[0x78];
    int main_party;                             // +0x78
    char pad7c[0xf8 - 0x7c];
    PartyMgr parties;                           // +0xf8
    char pad139d8[0x13a00 - 0x139d8];
    Troop* troops;                              // +0x13a00
    float party_value(unsigned id);             // 0x4b4800
    float relation(int a, int b);               // 0x48ffa0
};
extern GameData* g_game;                        // 0x5e2930
bool agent_low(int troop);                      // 0x4638f0

// ---- globals ------------------------------------------------------------------------------------------------
extern int g_item_bytes;                        // 0x54b8a8 -> 12.0f (equipment weight / value factor)
extern int g_result;                            // 0xa70458 (item-construction result slot)

// ---- helpers ------------------------------------------------------------------------------------------------
static __forceinline PartyMgr* mgr() { return (PartyMgr*)((char*)g_game + 0xf8); }

// @ 0x4b4470
int Party::add_stack(int troop, int count, unsigned flags, int f0c, int a5)
{
    if (count == 0)
        return 0;
    PartyMgr* m = mgr();
    int n = nstacks;
    int found = -1;
    for (int i = 0; i < n; i++) {
        if (stacks[i].troop == troop) {
            if ((stacks[i].flags & 1) == (flags & 1))
                found = i;
        }
    }
    if (found != -1)
        return found;
    int empty = 0;
    if ((flags & 1) != 0) {
        if (n >= 0x20)
            goto full;
    } else {
        int i = 0;
        for (; i < n; i++) {
            if (stacks[i].flags & 1)
                break;
        }
        empty = i;
    }
    if (n - empty >= 0x20)
        goto full;

    {
        Stack* s = &stacks[n];
        s->count = 0;
        s->troop = -1;
        s->flags = 0;
        s->f0c = 0;
        s->f10 = 0;
        s->spawned = 0;
        s->f18 = 0;
        s->f1c = 0;
        stacks[nstacks].troop = troop;
        stacks[nstacks].count = count;
        stacks[nstacks].f10 = a5;
        stacks[nstacks].f0c = 0;
        *(int*)((char*)this + (nstacks + 6) * 0x20) = 0;
        stacks[nstacks].flags = flags;
    }
    {
        int idx = nstacks;
        nstacks = idx + 1;
        if ((flags & 1) == 0)
            remove_slot(idx);
    }
    dirty = 1;
    return n;

full:
    found = -1;
    int out = -1;
    if ((g_game->troops[troop].flags & 0x10) != 0) {
        for (int i = 0; i < nstacks; i++) {
            if ((g_game->troops[stacks[i].troop].flags & 0x10) == 0 && (stacks[i].flags & 1) == (flags & 1))
                found = i;
        }
        if (found >= 0) {
            add_to_stack(id, stacks[found].troop, stacks[found].count, flags & 1, 0);
            out = add_stack(troop, count, flags, f0c, a5);
            dirty = 1;
            return out;
        }
    }
    dirty = 1;
    return out;
}

// @ 0x4b4700
void PartyMgr::refresh_party(unsigned id)
{
    if (id >= count)
        return;
    int idx = index_of(id);
    if (idx < 0)
        return;
    Party* p = &pages[idx >> 4][idx & 0xf];
    if (p->id != 1)
        return;
    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].count < p->stacks[i].spawned)
            p->stacks[i].spawned = p->stacks[i].count;
    }
    for (int i = p->nstacks - 1; i >= 0; i--) {
        if (p->stacks[i].count < 1) {
            int slot[8];
            party_part(this, i)->remove_row(slot, i, g_game->main_party == id ? (int*)((char*)g_game + 0x1405c) : 0);
        }
    }
    p->update();
}

// @ 0x4b4800
float GameData::party_value(unsigned id)
{
    float total = 0.0f;
    if (id >= parties.count)
        return total;
    int idx = parties.index_of(id);
    if (idx < 0)
        return total;
    Party* p = &parties.pages[idx >> 4][idx & 0xf];
    if (p->id != 1)
        return total;
    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if ((s.flags & 1) != 0)
            continue;
        Troop* t = &troops[s.troop];
        float lv = (float)t->level + 12.0f;
        if ((t->flags & 0x10) != 0) {
            if (agent_low(s.troop))
                continue;
            total += lv * lv * 0.01f;
        } else {
            total += (float)(s.count - s.f0c) * lv * lv * 0.01f;
        }
    }
    for (int i = 0;; i++) {
        int idx2 = parties.index_of(id);
        Party* q = &parties.pages[idx2 >> 4][idx2 & 0xf];
        if (q->extra_first == 0 || (unsigned)(q->extra_last - q->extra_first) <= (unsigned)i)
            break;
        int copy = (q->extra_first)[i];
        total = party_value(copy) + total;
    }
    return total;
}

// @ 0x4b49d0
void* Party::alloc_item(int a, int b, int c)
{
    int* p = alloc((void*)a, b, c, this, c);
    (void)p;
    return (char*)a + b * 0x5c;
}

