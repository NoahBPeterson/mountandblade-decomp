// Slice s0048f8f0: party helpers of the game object (the class whose party table sits at this+0xf8, GameX in
// src/game/script/script_interpreter.h).  0x48f8f0 add_members (adds troops to a party, then reseeds the per-stack
// random table of the main party), 0x48f9f0 remove_members, 0x48fbd0 add_template (fills a party from a party
// template), 0x48fce0 battle_root (follows the attached-to chain), 0x48fd60 clear_stacks (re-adds every stack to
// a destination party, then recurses into the attached parties), 0x48ff70 collect_attachments, 0x48ffa0 relation
// between two parties' factions, 0x490030/0x490160 pick a random position around a point.  Names provisional.
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

// ---- party records (0x5ac8 bytes, 16 per page; the page pointers are a std::vector at mgr+0x138cc) --------------
struct Stack {                                  // 0x20 bytes, array at party+0xa8
    int count;                                  // +0x00
    int troop;                                  // +0x04
    int flags;                                  // +0x08 (bit 0: prisoner stack)
    int f0c;
    int wounded;                                // +0x10
    int f14;
    int f18;
    int f1c;
};
struct Party {                                  // 0x5ac8 bytes
    char pad0[0x70];
    int faction;                                // +0x70 (row/column of the relation table)
    char pad74[0xa8 - 0x74];
    Stack stacks[64];                           // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x8b8 - 0x8ac];
    std::vector<int> attached;                  // +0x8b8 attached party ids (allocator lane at +0)
    int leader;                                 // +0x8c8 (attached-to party id, -1 when none)
    char pad8cc[0x5ac8 - 0x8cc];
    int add(int troop, int count, int prisoners, int a4, int a5);     // 0x48c840 (thiscall, ret 0x14)
    void slot_consume(int stack, int amount, int* table, int a4);      // 0x48c3c0 (thiscall, ret 0x10)
    void clear();                                                      // 0x4b1eb0
};
struct PartyMgr {                               // embedded in the game object at +0xf8
    char pad0[0x138cc];
    std::vector<char*> pages;                   // +0x138cc (begin pointer at +0x138d0)
    int index_of(int id) const;                 // 0x48ad00 (thiscall, ret 4)
    Party* get(int id);                         // 0x51ee50 (thiscall, ret 4); an out-of-line copy of the inline lookup below
};
inline Party* party_at(PartyMgr* m, int k)
{
    return (Party*)((k & 0xf) * 0x5ac8 + *(m->pages.begin() + (k >> 4)));
}
inline Party* party_of(PartyMgr* m, int id) { return party_at(m, m->index_of(id)); }

struct Troop {                                  // 0x508 bytes
    char pad0[0xb0];
    int level;                                  // +0xb0
    char padb4[0x508 - 0xb4];
};
extern __int64 g_ticks[];                       // 0x5e2948 (1e-5 s tick counters)

// party template slots: g_slotrecs[template] has six {troop, min, max, flags} records at +0x40
struct Slot {
    int troop;                                  // +0x00 (-1: unused)
    int f4;
    int f8;
    int flags;                                  // +0x0c (bit 0: prisoner)
};
struct SlotRec {                                // 0xb0 bytes
    char pad0[0x40];
    Slot slots[6];
};
extern SlotRec* g_slotrecs;                     // 0x5e29f8

// collision faces of the level
struct V2 {
    float x, y;
    V2() {}
    V2(float a, float b) : x(a), y(b) {}
};
struct Face {
    int m0;
    int m4;                                     // +4 face index (-1: none)
};
struct SceneObj {                               // level->obj
    char pad0[0x14];
    char* kinds;                                // +0x14 0x88-byte face records (+0x24: surface kind)
    char pad18[0xb0 - 0x18];
    bool overlap(int* a, int* b);               // 0x48efa0 (thiscall, ret 8)
};
struct Level {
    char pad0[0xb0];
    SceneObj* obj;                              // +0xb0
};
extern Level* g_level;                          // 0xa732b4

struct Game {
    char pad0[0x78];
    int main_party;                             // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                 // +0x8c (the player's troop id)
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                           // +0xf8
    char pad_mgr[0x13a00 - 0xf8 - sizeof(PartyMgr)];
    Troop* troops;                              // +0x13a00
    char pad13a04[0x13a48 - 0x13a04];
    float* rel;                                 // +0x13a48 (faction relation, 0xc0 floats per faction)
    char pad13a4c[0x1405c - 0x13a4c];
    int tbl[64][32];                            // +0x1405c (per-stack random seeds)

    int count_troops(int troop, int party, char prisoners);          // 0x48f790 (thiscall, ret 0xc)
    int template_count(int tmpl, int slot, float t);                  // 0x48a150 (thiscall, ret 0xc)
    Face* find_at(float* p);                                          // 0x48ea80 (thiscall, ret 4)
    int add_members(int party, int troop, int count, char prisoners);                  // 0x48f8f0
    int remove_members(int party, int troop, int count, char prisoners, int a5);       // 0x48f9f0
    void add_template(int party, int tmpl, int reverse);                                // 0x48fbd0
    int battle_root(int party);                                                         // 0x48fce0
    void clear_stacks(int party, Party* dest);                                          // 0x48fd60
    void collect_attachments(int party, Party* dest);                                   // 0x48ff70
    float relation(int a, int b);                                                       // 0x48ffa0
    void pick_near(V2 pos, float r, V2* out);                                           // 0x490030
    char pick_near_type(V2 pos, float r, V2* out, char any);                            // 0x490160
};

// @ 0x48f8f0
int Game::add_members(int party, int troop, int count, char prisoners)
{
    int total = count_troops(troop, party, prisoners);
    int r = party_of(&parties, party)->add(troop, count, prisoners != 0, 0, 0);
    if (party == main_party && r >= 0 && r < 64) {
        int cnt = parties.get(party)->stacks[r].count;
        int end = cnt - count;
        int s = cnt < 0 ? 0 : (cnt >= 32 ? 31 : cnt);
        int e = end < 0 ? 0 : (end >= 32 ? 31 : end);
        if (e < s) {
            int* q = &tbl[r][e];
            int n = s - e;
            do {
                *q++ = rand();
            } while (--n != 0);
        }
    }
    return count_troops(troop, party, prisoners) - total;
}

// @ 0x48f9f0
int Game::remove_members(int party, int troop, int count, char prisoners, int a5)
{
    int i = 0;
    int found = -1;
    for (; i < party_of(&parties, party)->nstacks; i++) {
        if (troop == party_of(&parties, party)->stacks[i].troop) {
            if ((prisoners && (party_of(&parties, party)->stacks[i].flags & 1))
                || (!prisoners && !(party_of(&parties, party)->stacks[i].flags & 1)))
                found = i;
        }
    }
    if (found >= 0) {
        Stack* s = &party_of(&parties, party)->stacks[found];
        if (s->count < count)
            count = s->count;
        if (party == main_party)
            party_of(&parties, party)->slot_consume(found, count, &tbl[0][0], a5);
        else
            party_of(&parties, party)->slot_consume(found, count, 0, a5);
        return count;
    }
    return 0;
}

// @ 0x48fbd0
// The template fraction (player level and the tick counter) is clamped to [0,1] as a repeated expression: the original
// compares the unspilled x87 value, which a named float variable would spill.
#define TEMPLATE_T ((float)troops[player].level * 0.033f + (float)g_ticks[1] * 1e-09f)
void Game::add_template(int party, int tmpl, int reverse)
{
    SlotRec* r = (SlotRec*)((char*)g_slotrecs + tmpl * 0xb0);
    Party* p = party_of(&parties, party);
    for (int i = 0; i < 6; i++) {
        Slot* s = &r->slots[i];
        if (s->troop >= 0) {
            float t = TEMPLATE_T < 0.0f ? 0.0f : (TEMPLATE_T >= 1.0f ? 1.0f : TEMPLATE_T);
            int n = template_count(tmpl, i, t);
            if (n > 0)
                p->add(s->troop, n, (s->flags & 1) ? (reverse == 0) : (reverse != 0), 0, 0);
        }
    }
}

// @ 0x48fce0
int Game::battle_root(int party)
{
    PartyMgr* pm = &parties;
    int cur = party;
    Party* p = party_of(pm, cur);
    while (p->leader >= 0) {
        cur = p->leader;
        p = party_of(pm, cur);
    }
    return cur;
}

// @ 0x48fd60
void Game::clear_stacks(int party, Party* dest)
{
    int i;
    for (i = 0; i < party_of(&parties, party)->nstacks; i++) {
        dest->add(party_of(&parties, party)->stacks[i].troop, party_of(&parties, party)->stacks[i].count,
                  party_of(&parties, party)->stacks[i].flags, party_of(&parties, party)->stacks[i].wounded, 0);
    }
    unsigned j;
    for (j = 0; j < party_of(&parties, party)->attached.size(); j++)
        clear_stacks(party_of(&parties, party)->attached[j], dest);
}

// @ 0x48ff70
void Game::collect_attachments(int party, Party* dest)
{
    dest->nstacks = 0;
    clear_stacks(party, dest);
    dest->clear();
}

// @ 0x48ffa0
float Game::relation(int a, int b)
{
    if (a == b)
        return 1.0f;
    PartyMgr* pm = &parties;
    return rel[party_of(pm, a)->faction * 0xc0 + party_of(pm, b)->faction + 0xb];
}

// @ 0x490030
void Game::pick_near(V2 pos, float r, V2* out)
{
    V2 np;
    float dx, dy;
    int gi;
    *out = pos;
    char ok = 0;
    int iter = 0;
    Face* f = find_at(&pos.x);
    if (f == 0)
        return;
    int first = f->m4;
    if (first < 0)
        return;
    while (!ok) {
        if (iter >= 5000)
            return;
        iter++;
        dx = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        dy = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        if (sqrt(dx * dx + dy * dy) <= r) {
            np = V2(pos.x + dx, pos.y + dy);
            *out = np;
            Face* g = find_at(&out->x);
            if (g != 0) {
                gi = g->m4;
                ok = g_level->obj->overlap(&first, &gi);
            }
        }
    }
}

// @ 0x490160
char Game::pick_near_type(V2 pos, float r, V2* out, char any)
{
    V2 np;
    float dx, dy;
    *out = pos;
    char ok = 0;
    int iter = 0;
    while (iter < 50000) {
        iter++;
        dx = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        dy = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        if (sqrt(dx * dx + dy * dy) > r)
            continue;
        np = V2(pos.x + dx, pos.y + dy);
        *out = np;
        Face* f = find_at(&out->x);
        if (f == 0)
            continue;
        int kind = *(int*)(g_level->obj->kinds + f->m4 * 0x88 + 0x24);
        int low = kind & 7;
        if (any)
            ok = low == 0 || kind == 7;
        else
            ok = (low >= 2 && low < 7) || kind == 7;
        if (ok)
            break;
    }
    return ok;
}
