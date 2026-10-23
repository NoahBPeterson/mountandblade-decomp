// Slice s004b4470: the party inventory / "spawn troop" helpers of the world map object.
//   0x4b4470  Party::add_stack       (append a 0x20-byte stack row: promote an existing stack in the other
//                                    slot class when there is room, otherwise push a new row and rebuild it)
//   0x4b4700  GameData::refresh_party (cap the "spawned" counter of every stack and drop the empty tail)
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
    char pad0[4];
    int kind;                                   // +0x04  ==1 for a live party (checked by refresh/value)
    int id;                                     // +0x08  party id
    char padc[0xa8 - 0xc];
    Stack stacks[0x40];                         // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    char dirty;                                 // +0x8b4
    char pad8b5[0x8b8 - 0x8b5];
    int extra_al;                               // +0x8b8  std::vector<int> (allocator pad, first, last, end)
    int* extra_first;                           // +0x8bc  extra troop ids
    int* extra_last;                            // +0x8c0
    int* extra_end;                             // +0x8c4
    char pad8c8[0x5ac8 - 0x8c8];

    int  add_stack(int troop, int count, unsigned flags, int f10);           // 0x4b4470 (ret 0x10)
    int  remove_row(int* out, int idx, int* table);                          // 0x489540 (ret 0xc)
    void remove_slot(int idx);                                               // 0x48c330 (ret 4)
    bool update();                                                           // 0x4b1b80
    void* alloc_item(int a, int b, int c);                                  // 0x4b49d0
};
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                              // +0x138d0 (g_game + 0x139c8)
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                             // +0x138dc (g_game + 0x139d4)
    int index_of(int id);                       // 0x48ad00
};

// ---- game data (0x5e2930) -----------------------------------------------------------------------------------
struct Troop {
    char pad0[0x30];
    unsigned flags;                             // +0x30  bit 0x10 = hero
    char pad34[0xb0 - 0x34];
    int level;                                  // +0xb0
    char padb4[0x508 - 0xb4];
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
    void refresh_party(unsigned id);            // 0x4b4700 (this = GameData)
    bool agent_low(int troop);                  // 0x4638f0
    int  add_to_stack(int id, int troop, int amount, char flag, int a5);     // 0x48f9f0 (ret 0x14, this = g_game)
};
extern GameData* g_game;                        // 0x5e2930

// ---- globals ------------------------------------------------------------------------------------------------
extern int g_item_bytes;                        // 0x54b8a8 -> 12.0f (equipment weight / value factor)
extern int g_result;                            // 0xa70458 (item-construction result slot)

// ---- helpers ------------------------------------------------------------------------------------------------
static __forceinline PartyMgr* mgr() { return (PartyMgr*)((char*)g_game + 0xf8); }

// @ 0x4b4470
// 4 stack arguments (ret 0x10): the 4th is stored in the row's +0x10 field.  The "already present" path
// returns the (never written) result local, exactly like the original (it reads [esp+0x10] uninitialised).
int Party::add_stack(int troop, int count, unsigned flags, int f10)
{
    int result;
    if (count == 0)
        return 0;
    int n = nstacks;
    int found = -1;
    for (int i = 0; i < n; i++) {
        if (stacks[i].troop == troop) {
            if (((stacks[i].flags ^ flags) & 1) == 0)
                found = i;
        }
    }
    if (found != -1)
        return result;
    int cls = flags & 1;
    int lead;
    if (cls != 0) {
        lead = 0;
        for (int k = 0; k < n; k++) {
            if (stacks[k].flags & 1)
                break;
            lead++;
        }
        if (n - lead < 0x20)
            goto make;
    }
    if (cls == 0) {
        lead = 0;
        for (int k = 0; k < n; k++) {
            if (stacks[k].flags & 1)
                break;
            lead++;
        }
        if (lead < 0x20)
            goto make;
    }
    goto full;
make:
    if (n >= 0) {
        Stack* s = &stacks[n];
        s->count = 0;
        s->troop = -1;
        s->flags = 0;
        s->f0c = 0;
        s->f10 = 0;
        s->spawned = 0;
        s->f18 = 0;
        stacks[nstacks].troop = troop;
        stacks[nstacks].count = count;
        stacks[nstacks].f10 = f10;
        stacks[nstacks].spawned = 0;
        *(int*)((char*)this + (nstacks + 6) * 0x20) = 0;
        stacks[nstacks].flags = flags;
        int idx = nstacks + 1;
        nstacks = idx;
        if (cls == 0)
            remove_slot(idx - 1);
        result = n;
        dirty = 1;
        return result;
    }
full:
    result = -1;
    if ((g_game->troops[troop].flags & 0x10) != 0) {
        found = -1;
        for (int i = 0; i < nstacks; i++) {
            if ((g_game->troops[stacks[i].troop].flags & 0x10) == 0 && (int)(stacks[i].flags & 1) == cls)
                found = i;
        }
        if (found >= 0) {
            g_game->add_to_stack(id, stacks[found].troop, stacks[found].count, (char)cls, 0);
            result = add_stack(troop, count, flags, f10);
        }
    }
    dirty = 1;
    return result;
}

// @ 0x4b4700
void GameData::refresh_party(unsigned id)
{
    PartyMgr* m = &parties;
    if (id >= parties.count)
        return;
    int idx = m->index_of(id);
    if (idx < 0)
        return;
    Party** pp = &m->pages[(unsigned)idx >> 4];
    if ((*pp)[idx & 0xf].kind != 1)
        return;
    int j = m->index_of(id);
    Party** pq = &m->pages[j >> 4];
    Party* p = &(*pq)[j & 0xf];
    for (int i = 0; i < p->nstacks; i++) {
        Stack* st = &p->stacks[i];
        int c = st->count;
        if (st->spawned > c)
            st->spawned = c;
    }
    for (int i = p->nstacks - 1; i >= 0; i--) {
        if (p->stacks[i].count <= 0) {
            int slot[8];
            p->remove_row(slot, i, (int)id == main_party ? (int*)((char*)this + 0x1405c) : 0);
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
    if (parties.pages[idx >> 4][idx & 0xf].kind != 1)
        return total;
    int j = parties.index_of(id);
    Party* p = &parties.pages[j >> 4][j & 0xf];
    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if ((s.flags & 1) != 0)
            continue;
        Troop* t = &troops[s.troop];
        float lv = (float)t->level + 12.0f;
        if ((t->flags & 0x10) != 0) {
            if (this->agent_low(s.troop))
                continue;
            total += lv * lv * 0.01f;
        } else {
            total += (float)(s.count - s.f10) * lv * lv * 0.01f;
        }
    }
    for (int i = 0;; i++) {
        int idx2 = parties.index_of(id);
        Party* q = &parties.pages[idx2 >> 4][idx2 & 0xf];
        if (q->extra_first == 0 || (unsigned)(q->extra_last - q->extra_first) <= (unsigned)i)
            break;
        int idx3 = parties.index_of(id);
        int copy = parties.pages[idx3 >> 4][idx3 & 0xf].extra_first[i];
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

