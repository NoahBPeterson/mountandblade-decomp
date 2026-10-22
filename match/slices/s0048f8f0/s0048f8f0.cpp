// Slice s0048f8f0: party-roster helpers of a mission party object. 0x48f8f0 seeds a stack's per-slot random table where
// the spawned range was consumed; 0x48f9f0 finds the last stack of a troop and adds to it; 0x48fbd0 applies the
// "regeneration" ticks of six timed slots; 0x48fce0 walks the leader chain; 0x48fd60 clears all stacks and recurses into
// the party's attached parties; 0x48ff70 clears a party; 0x48ffa0 looks up a troop-pair relation value;
// 0x490030/0x490160 pick a random nearby point that lies on a collision face. All names provisional.
#include <windows.h>
#include <stdlib.h>

// ---- game data (object at 0x5e2930; see s00473a60 / s004608d0) ---------------------------------------------------
struct Troop {                                  // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    __int64 flags;                              // +0x30 (0x10 = hero)
    char pad38[0xb0 - 0x38];
    int level;                                  // +0xb0
    char padb4[0x508 - 0xb4];
};
struct Stack {                                  // 0x20 bytes, array at party+0xa8
    int count;                                  // +0x00
    int troop;                                  // +0x04
    int flags;                                  // +0x08
    int spawned;                                // +0x0c
    int wounded;                                // +0x10
    int f14;
    int f18;
    int f1c;
};
struct Party {                                  // 0x5ac8 bytes (see s0048c0c0 / s004910a0)
    char pad0[0xa8];
    Stack stacks[64];                           // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    char dirty;                                 // +0x8b4
    char pad8b5[0x8b8 - 0x8b5];
    int* extra_first;                           // +0x8b8 std::vector<int>
    int* extra_last;                            // +0x8bc
    int* extra_end;                             // +0x8c0
    int f8c4;
    int leader;                                 // +0x8c8 (leader party id, -1 when none)
    int f70;                                    // +0x8cc (object kind index into the relation table)
    int sum_counts(int troop, int party_id, int amount);           // 0x48f790 (thiscall, ret 0xc, = count_item)
    int add(int troop, int amount, char flag, int a4, int a5);     // 0x48c840 (thiscall, ret 0x14)
    void add_many(int idx, int amount, int* table, int a4);        // 0x48c3c0 (thiscall, ret 0x10)
    int regen_tick(int party_id, int idx, float t);                 // 0x48a150 (thiscall, ret 0xc)
    void clear();                                                  // 0x4b1eb0
    int seed_slots(int party_id, int troop, int from, int amount, char flag);     // 0x48f8f0
    int add_to_stack(int party_id, int troop, int amount, char flag, int a5);     // 0x48f9f0
    void tick_slots(int party_id, int a, int a4);                  // 0x48fbd0
    int last_leader(int party_id);                                 // 0x48fce0
    void clear_with_extras(int party_id);                          // 0x48fd60
    float relation(int a, int b);                                  // 0x48ffa0
    void pick_near(float x, float y, float r, float* out);         // 0x490030
    int pick_near_type(float x, float y, float r, float* out, char any);  // 0x490160
};

struct PartyPool;                               // (only reached through PartyMgr above)
struct GameData {
    char pad0[0x78];
    int f78;                                    // +0x78 (main/default party id)
    char pad7c[0x8c - 0x7c];
    int player;                                 // +0x8c (the player's troop id)
    char pad90[0xf8 - 0x90];
    char pad_f8[0x13a00 - 0xf8];                // the PartyMgr (0x138d8 bytes) lives here
    Troop* troops;                              // +0x13a00
    int ntroops;                                // +0x13a04
    char pad13a08[0x13a48 - 0x13a08];
    float* rel;                                 // +0x13a48 (troop-pair relation, 0xc0 floats per first troop)
    int tbl[64][32];                            // +0x1405c (per-stack random seeds)
};
extern GameData* g_game;                        // 0x5e2930
// the class the original works with: every member here is reached relative to game+0xf8
struct PartyMgr {
    int index_of(int id);                       // 0x48ad00 (thiscall, ret 4) -- the map object is at +0x138d0
    Party* get(int id);                         // 0x51ee50 (thiscall, ret 4; inlines index_of)
    Party* find(int id);                        // 0x51ee50, the same lookup as index_of -> page (inlined)
};
#define MGR ((PartyMgr*)((char*)this + 0xf8))
inline Party* party_part(PartyMgr* m, int idx)
{
    int off = (idx & 0xf) * 0x5ac8 + ((int*)g_game)[0x138d0 / 4 + (idx >> 4)];
    return (Party*)((char*)g_game + off);
}
inline Party* party_find(PartyMgr* m, int id) { return party_part(m, m->index_of(id)); }

struct Slot {                                   // 0x10 bytes, six per record at 0x5e29f8
    int timer;                                  // +0x00 (-1: inactive)
    int f4;
    int f8;
    int flags;                                  // +0x0c (bit 0)
};
struct SlotRec {                                // 0xb0 bytes
    int f0;
    char pad4[0x40 - 4];
    Slot slots[6];                              // +0x40
};
extern SlotRec* g_slotrecs;                     // 0x5e29f8
extern __int64 g_ticks;                         // 0x5e2950 (100 kHz? tick counter, 64-bit)

// Level: collision faces.
struct face {                                   // see s004333c0
    int m0, m4;
    char pad8[0x20 - 8];
    float pad20[0x34];
    bool contains_xy(const float* p);           // 0x4333c0 (thiscall, ret 4)
};
struct cell {
    char pad[0x34];
    void collect_point(const float* p, int* use3d, unsigned mask, face** out, int* count);  // 0x439780
};
struct Level {
    char pad0[0x28];
    cell** cells;                               // +0x28
    char pad2c[0xb0 - 0x2c];
    char* obj;                                  // +0xb0
    void locate(const float* p, int* out) const;  // 0x41ed00
};
extern Level* g_level;                          // 0xa732b4
face* find_face(float* p);                      // 0x48ea80
bool faces_overlap(int* a, int* b);             // 0x48efa0 (thiscall on level+0xb0, ret 8)

// @ 0x48f8f0
int Party::seed_slots(int party_id, int troop, int from, int amount, char flag)
{
    PartyMgr* pidx = MGR;
    int total = sum_counts(troop, party_id, amount);
    int idx = pidx->index_of(party_id);
    int r = party_part(pidx, idx)->add(troop, amount, flag != 0, 0, 0);
    if (party_id == g_game->f78 && r >= 0 && r < 64) {
        int spawned = MGR->get(party_id)->stacks[r].spawned;
        int end = spawned - amount;
        int s = spawned < 0 ? 0 : (spawned >= 32 ? 31 : spawned);
        int e = end < 0 ? 0 : (end >= 32 ? 31 : end);
        if (e < s) {
            int* q = &g_game->tbl[r][e];
            int n = s - e;
            do {
                *q++ = rand();
            } while (--n != 0);
        }
    }
    return sum_counts(troop, party_id, amount) - total;
}

// @ 0x48f9f0
int Party::add_to_stack(int party_id, int troop, int amount, char flag, int a5)
{
    PartyMgr* pidx = MGR;
    int found = -1;
    int i = 0;
    for (; i < party_part(pidx, pidx->index_of(party_id))->nstacks; i++) {
        if (party_part(pidx, pidx->index_of(party_id))->stacks[i].troop == troop) {
            if (flag) {
                if (party_part(pidx, pidx->index_of(party_id))->stacks[i].flags & 1)
                    found = i;
            } else {
                if ((party_part(pidx, pidx->index_of(party_id))->stacks[i].flags & 1) == 0)
                    found = i;
            }
        }
    }
    if (found < 0)
        return 0;
    int n = party_part(pidx, pidx->index_of(party_id))->stacks[found].count;
    if (n < amount)
        amount = n;
    if (party_id == g_game->f78)
        party_part(pidx, pidx->index_of(party_id))->add_many(found, amount, g_game->tbl[0], a5);
    else
        party_part(pidx, pidx->index_of(party_id))->add_many(found, amount, 0, a5);
    return amount;
}

// @ 0x48fbd0
void Party::tick_slots(int party_id, int a, int a4)
{
    SlotRec* r = (SlotRec*)((char*)g_slotrecs + a * 0xb0);
    int idx = MGR->index_of(party_id);
    Party* p = party_part(MGR, idx);
    for (int i = 0; i < 6; i++) {
        Slot* s = &r->slots[i];
        if (s->timer >= 0) {
            float t = (float)(int)g_ticks * 1e-09f
                    + (float)g_game->troops[g_game->player].level * 0.033f;
            if (t < 0.0f)
                t = 0.0f;
            else if (t >= 1.0f)
                t = 1.0f;
            int n = this->regen_tick(party_id, i, t);
            if (n > 0)
                p->add(s->timer, n, (s->flags & 1) ? (a4 == 0) : (a4 != 0), 0, 0);
        }
    }
}

// @ 0x48fce0
int Party::last_leader(int party_id)
{
    PartyMgr* pidx = (PartyMgr*)((char*)this + 0xf8);
    int cur = party_id;
    int nxt = party_part(pidx, pidx->index_of(cur))->leader;
    while (nxt >= 0) {
        cur = nxt;
        nxt = party_part(pidx, pidx->index_of(cur))->leader;
    }
    return cur;
}

// @ 0x48fd60
void Party::clear_with_extras(int party_id)
{
    PartyMgr* pidx = MGR;
    for (int i = 0; i < party_part(pidx, pidx->index_of(party_id))->nstacks; i++) {
        Party* p = party_part(pidx, pidx->index_of(party_id));
        p->add(p->stacks[i].troop, party_part(pidx, pidx->index_of(party_id))->stacks[i].count,
               party_part(pidx, pidx->index_of(party_id))->stacks[i].flags, 0,
               party_part(pidx, pidx->index_of(party_id))->stacks[i].wounded);
    }
    for (unsigned i = 0; ; i++) {
        Party* p = party_part(pidx, pidx->index_of(party_id));
        if (p->extra_first == 0 || (unsigned)(p->extra_last - p->extra_first) <= i)
            break;
        Party* q = party_part(pidx, pidx->index_of(party_id));
        q->clear_with_extras(q->extra_first[i]);
    }
}

// @ 0x48ff70 void __stdcall clear_party(Party* p, int a)
void __stdcall clear_party(Party* p, int a)
{
    p->nstacks = 0;
    p->clear_with_extras(a);
    p->clear();
}

// @ 0x48ffa0
float Party::relation(int a, int b)
{
    if (a == b)
        return 1.0f;
    PartyMgr* pidx = (PartyMgr*)((char*)this + 0xf8);
    Party* pa = party_part(pidx, pidx->index_of(a));
    Party* pb = party_part(pidx, pidx->index_of(b));
    return g_game->rel[pa->f70 * 0xc0 + pb->f70 + 0xb];
}

// @ 0x490030
void Party::pick_near(float x, float y, float r, float* out)
{
    out[1] = y;
    char ok = 0;
    out[0] = x;
    int iter = 0;
    float p0 = x, p1 = y;
    face* f = find_face(&p0);
    if (f == 0 || f->m4 < 0)
        return;
    int first = f->m4;
    do {
        if (iter >= 5000)
            return;
        iter++;
        float dx = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        float dy = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        if (dx * dx + dy * dy <= r * r) {
            out[0] = x + dx;
            out[1] = y + dy;
            face* g = find_face(out);
            if (g != 0) {
                int fi = first;
                int gi = g->m4;
                ok = faces_overlap(&fi, &gi);
            }
        }
    } while (ok == 0);
}

// @ 0x490160
int Party::pick_near_type(float x, float y, float r, float* out, char any)
{
    out[0] = x;
    out[1] = y;
    char ok = 0;
    int iter = 0;
    while (true) {
        if (iter >= 50000)
            return ok;
        iter++;
        float dx = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        float dy = ((float)(rand() % 15817) * 0.00012644623f - 1.0f) * r;
        if (dx * dx + dy * dy > r * r)
            continue;
        out[0] = x + dx;
        out[1] = y + dy;
        face* f = find_face(out);
        if (f == 0)
            continue;
        unsigned kind = *(unsigned*)(*(int*)(g_level->obj + 0x14) + f->m4 * 0x88 + 0x24);
        if (any) {
            if ((kind & 7) == 0 || kind == 7)
                return 1;
            ok = 0;
        } else {
            unsigned low = kind & 7;
            if ((low >= 2 && low < 7) || kind == 7)
                return 1;
            ok = 0;
        }
    }
}
