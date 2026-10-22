// Slice s004910a0: party recruit/dissolve helpers on the game object (0x5e2930).
//   0x4910a0  World::recruit(id, count, dest)   -- turn prisoners of a party into recruits
//   0x491430  World::handout(party, id)         -- spread a party's stacks over its members
//   0x4915d0  World::dissolve(id)               -- tear a party down and detach it
// Layouts follow the verified manager layout of s004902a0 (pages at mgr+0x138d0, count +0x138dc,
// troop table +0x13908) and the 0x5ac8-byte party record. Names provisional.
#include <windows.h>
#include <stdlib.h>

// ---- troops -----------------------------------------------------------------------------------------------
struct Troop {                                   // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    unsigned flags;                              // +0x30 (0x10 = mounted, 0x80 = ...)
    char pad34[0xa0 - 0x34];
    float hp;                                    // +0xa0
    char pada4[0xb0 - 0xa4];
    int value;                                   // +0xb0
    char padb4[0x508 - 0xb4];
};

// ---- parties ----------------------------------------------------------------------------------------------
struct Stack {                                   // 0x20 bytes, array at party+0xa8
    int count;                                   // +0x00
    int troop;                                   // +0x04
    int f08;                                     // +0x08
    int f0c;                                     // +0x0c
    int wounded;                                 // +0x10
    int pad14[3];
};

struct Party {                                   // 0x5ac8 bytes
    void* vtbl;                                  // +0x00
    int type;                                    // +0x04 (1 = ordinary party)
    char pad8[0x8c - 0x8];
    int owner;                                   // +0x8c
    char pad90[0xa8 - 0x90];
    Stack stacks[64];                            // +0xa8
    int nstacks;                                 // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    unsigned char f8b4;                          // +0x8b4
    char pad8b5[0x8b8 - 0x8b5];
    int f8b8;                                    // +0x8b8
    int* v8bc;                                   // +0x8bc (vector begin)
    int* v8c0;                                   // +0x8c0 (vector end)
    int* v8c4;                                   // +0x8c4 (vector cap)
    int f8c8;                                    // +0x8c8
    unsigned char f8cc;                          // +0x8cc
    char pad8cd[0x8e4 - 0x8cd];
    int f8e4;                                    // +0x8e4
    int f8e8;                                    // +0x8e8
    char pad8ec[0x918 - 0x8ec];
    int f918;                                    // +0x918
    int f91c;                                    // +0x91c
    char pad920[0x5ac8 - 0x920];

    int add(int troop, int amount, int flag, int a4, int a5);   // 0x48c840 (thiscall, ret 0x14)
    int sum(int what);                           // 0x4b1cf0 (thiscall, ret 4)
    int find_stack(int amount, int what);        // 0x4b1d30 (thiscall, ret 8)
    void clear_map();                            // 0x4b32f0 (thiscall, ret 0)
};

// ---- the game object's party manager at game+0xf8 ----------------------------------------------------------
struct Mgr {
    char pad0[0x138b8];
    int f138b8;                                  // +0x138b8 (a smaller id bound)
    char pad138bc[0x138d0 - 0x138bc];
    Party** pages;                               // +0x138d0 (index >> 4 -> page of 16)
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                              // +0x138dc (number of party slots)
    char pad138e0[0x139d4 - 0x138e0];
    unsigned last_id;                            // +0x139d4

    int index_of(unsigned id);                   // 0x48ad00 (thiscall, ret 4)
    int is_slot(unsigned id);                    // 0x4903e0 (thiscall, ret 4)
    unsigned first_party_id();                   // 0x460d20 (thiscall, ret 0)
    void next_party_id(unsigned* io);            // 0x460ca0 (thiscall, ret 4)
};

inline Party* party_at(Mgr* m, unsigned i)
{
    return m->pages[i >> 4] + (i & 0xf);
}
inline Party* party_s(Mgr* m, int i)
{
    return m->pages[i >> 4] + (i & 0xf);
}

struct World {                                   // the game object (0x5e2930)
    char pad0[0x8c];
    int player;                                  // +0x8c
    char pad90[0xf8 - 0x90];
    Mgr mgr;                                     // +0xf8
    Troop* troops;                               // +0x13a00 (troop table)

    void get_members(int id, int* list, int* n);                 // 0x490fa0 (ret 0xc)
    void get_prisoners(int id, int* a, int* b, int* c, int* d);  // 0x490ce0 (ret 0x14)
    bool agent_low(int idx);                                     // 0x4638f0 (ret 4)
    void drop_party(int id);                                     // 0x490a30 (ret 4)
    void recruit(int id, int count, Party* dest);                // 0x4910a0 (ret 0xc)
    void handout(Party* p, int id);                              // 0x491430 (ret 8)
    void dissolve(int id);                                       // 0x4915d0 (ret 4)
};

extern char* g_party_table;                      // 0x5e29f8 (0xb0-byte owner rows, +0x38 counter)
extern float g_k15;                              // 0x590da0 (15.0f)
extern float g_k065;                             // 0x590b6c (0.65f)
extern float g_thr_self;                         // 0xa1813c

// @ 0x4910a0
void World::recruit(int id, int count, Party* dest)
{
    if ((unsigned)id >= mgr.count)
        return;
    Mgr* m = &mgr;
    int idx = m->index_of(id);
    if (idx < 0)
        return;
    if (party_at(m, idx)->type != 1)
        return;

    int n = 0;
    int members[128];
    get_members(id, members, &n);
    int a = 0, b = 0, c = 0, d = 0;
    get_prisoners(id, &a, &b, &c, &d);
    if (dest != 0)
        dest->nstacks = 0;
    int total = a - b;

    idx = m->index_of(id);
    Party* p = party_s(m, idx);
    if (p->stacks[0].troop == player && !agent_low(player))
        total--;

    if (count <= 0)
        return;
    int left = count;
    do {
        if (total > 0) {
            int pick;
            if (total < 2)
                pick = 0;
            else
                pick = rand() % total;
            int base = 0;
            bool found = false;
            int j = 0;
            do {
                if (j >= n)
                    break;
                int mi = m->index_of(members[j]);
                Party* mp = party_s(m, mi);
                int sum = mp->sum(0x134);
                base += sum;
                if (pick < base) {
                    found = true;
                    Stack* st = &mp->stacks[mp->find_stack(pick - (base - sum), 0x134)];
                    int troop = st->troop;
                    Troop* tr = &troops[troop];
                    int hp = tr->value + 10;
                    if ((tr->flags & 0x10) != 0)
                        hp += 10;
                    if ((float)(rand() % 0x3dc9) * (float)hp * 6.322312e-05f < 2.0f) {
                        tr = &troops[troop];
                        if ((tr->flags & 0x10) == 0) {
                            if ((float)(rand() % 0x3dc9) * 6.322312e-05f < g_k065
                                || (troops[troop].flags & 0x80) != 0) {
                                st->count--;
                                if (dest != 0)
                                    dest->add(troop, 1, 0, 0, 0);
                            } else {
                                st->wounded++;
                                if (dest != 0)
                                    dest->add(troop, 1, 0, 1, 0);
                            }
                        } else {
                            if (troops[troop].hp > g_thr_self)
                                troops[troop].hp = g_thr_self;
                            troops[troop].hp -= g_k15;
                            if (troops[troop].hp < 0.0f)
                                troops[troop].hp = 0.0f;
                            if (dest != 0)
                                dest->add(troop, 1, 0, 1, 0);
                        }
                        total--;
                    }
                }
                j++;
            } while (!found);
        }
        left--;
    } while (left != 0);
}

// @ 0x491430
void World::handout(Party* p, int id)
{
    int n = 0;
    int members[128];
    int cum[128];
    int total = 0;
    get_members(id, members, &n);
    Mgr* m = &mgr;
    for (int i = 0; i < n; i++) {
        total += party_s(m, m->index_of(members[i]))->sum(0x100);
        cum[i] = total;
    }
    if (total == 0)
        return;
    for (int i = 0; i < p->nstacks; i++) {
        if ((troops[p->stacks[i].troop].flags & 0x10) != 0)
            continue;
        for (int j = 0; j < p->stacks[i].count; j++) {
            int pick;
            if (total < 2)
                pick = 0;
            else
                pick = rand() % total;
            int k = 0;
            while (k < n && pick >= cum[k])
                k++;
            party_s(m, m->index_of(members[k]))->add(p->stacks[i].troop, 1, p->stacks[i].f08, 0, 0);
        }
    }
}

// @ 0x4915d0
void World::dissolve(int id)
{
    if ((unsigned)id >= mgr.count)
        return;
    Mgr* m = &mgr;
    int idx = m->index_of(id);
    if (idx < 0)
        return;
    if (party_at(m, idx)->type != 1)
        return;

    idx = m->index_of(id);
    Party* p = party_s(m, idx);
    if (p->f8c8 >= 0) {
        if (m->is_slot(p->f8c8))
            drop_party(id);
        p->f8c8 = -1;
    }
    if (p->v8bc != 0 && p->v8c0 - p->v8bc != 0) {
        for (unsigned k = 0; p->v8bc != 0 && k < (unsigned)((p->v8c0 - p->v8bc) >> 2); k++) {
            if ((unsigned)p->v8bc[k] >= m->count)
                continue;
            int j = m->index_of(p->v8bc[k]);
            if (j < 0)
                continue;
            if (party_at(m, j)->type != 1)
                continue;
            party_s(m, m->index_of(p->v8bc[k]))->f8c8 = -1;
            party_s(m, m->index_of(p->v8bc[k]))->f8cc = 0;
            party_s(m, m->index_of(p->v8bc[k]))->f8b4 = 1;
        }
        if (p->v8bc != 0)
            operator delete(p->v8bc);
        p->v8bc = 0;
        p->v8c0 = 0;
        p->v8c4 = 0;
    }
    if (p->owner >= 0)
        *(int*)(g_party_table + p->owner * 0xb0 + 0x38) += 1;
    p->clear_map();

    idx = m->index_of(id);
    if (idx >= 0 && idx < mgr.f138b8)
        party_s(m, idx)->type = 2;

    unsigned cur = m->first_party_id();
    if (cur < mgr.count) {
        do {
            Party* q = party_s(m, m->index_of(cur));
            if (q->f91c == id) {
                q->f918 = 0;
                q->f91c = -1;
            }
            if (q->f8e8 == id) {
                q->f8e4 = 0;
                q->f8e8 = -1;
            }
            m->next_party_id(&cur);
        } while (cur < mgr.count);
    }
}
