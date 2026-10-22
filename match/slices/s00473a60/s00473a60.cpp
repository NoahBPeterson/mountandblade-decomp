// Slice s00473a60: choosing which party stack spawns next for a mission spawn rule (0x473a60), the "next used pool entry"
// iterator (0x474300), a vector<ArrSet>::_Ucopy thunk (0x474350) and the std::_Insertion_sort of 8-byte records
// (0x474370, stable_sort's small-chunk sorter). All names provisional.
#include <windows.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

// ---- game data (object at 0x5e2930; see s004608d0 / s00463790 / s00467250) -----------------------------------------
struct Troop {                                  // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    __int64 flags;                              // +0x30 (0x10 = hero, 0x1000000 / 0x4000000 = troop class bits)
    char pad38[0xa0 - 0x38];
    float hp;                                   // +0xa0
    char pada4[0x3b8 - 0xa4];
    unsigned f3b8;                              // +0x3b8 (low 16 = party/side id, bits 16..23 = rule id)
    int used;                                   // +0x3bc
    char pad3c0[0x4e4 - 0x3c0];
    int f4e4;                                   // +0x4e4
    char pad4e8[0x508 - 0x4e8];
};
struct Stack {                                  // 0x20 bytes, array at party+0xa8
    int count;                                  // +0x00 total
    int troop;                                  // +0x04
    int flags;                                  // +0x08 (bit 0 = hero)
    int spawned;                                // +0x0c
    int wounded;                                // +0x10
    int pad14[3];
};
struct Party {                                  // 0x5ac8 bytes
    char pad0[0xa8];
    Stack stacks[64];                           // +0xa8
    int nstacks;                                // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
    int sum_stat(int what);                     // 0x4b1cf0 (thiscall, ret 4)
};
struct PartyMgr {                               // embedded in the game data at +0xf8
    char pad0[0x138cc];
    std::vector<Party*> pages;                  // +0x138cc (first at +0x138d0; index >> 4 -> page of 16)
    int index_of(int id);                       // 0x48ad00 (thiscall, ret 4)
};
struct SceneObj {                               // 0x494 bytes, table at game+0x13a50
    char pad0[0x114];
    struct { int a, b; } pairs[1];              // +0x114
    char pad11c[0x494 - 0x11c];
};
struct GameData {
    char pad0[4];
    int f4;                                     // +0x04
    char pad8[0x78 - 8];
    int f78;                                    // +0x78 (default party)
    char pad7c[0x8c - 0x7c];
    int player;                                 // +0x8c (the player's troop id)
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                           // +0xf8
    char pad_after_parties[0x13a00 - 0xf8 - sizeof(PartyMgr)];
    Troop* troops;                              // +0x13a00
    int ntroops;                                // +0x13a04
    char pad13a08[0x13a50 - 0x13a08];
    SceneObj* sceneobjs;                        // +0x13a50
    char pad13a54[0x1405c - 0x13a54];
    int tbl[64][32];                            // +0x1405c
    bool agent_low(int idx);                    // 0x4638f0 (thiscall, ret 4)
};
extern GameData* g_game;                        // 0x5e2930

extern float g_thr_self;                        // 0xa1813c
extern float g_thr_other;                       // 0xa18138
inline bool agent_low_inl(int idx)
{
    if (idx == g_game->player)
        return g_game->troops[idx].hp < g_thr_self;
    return g_game->troops[idx].hp < g_thr_other;
}
inline Party* get_party(int id)
{
    PartyMgr* pm = &g_game->parties;
    int i = pm->index_of(id);
    return &pm->pages[i >> 4][i & 15];
}

struct SpawnRec {                               // 0x4c bytes: a mission spawn rule
    int id;                                     // +0x00
    int flags;                                  // +0x04 (see rule_accepts / pick_spawn)
    unsigned flags2;                            // +0x08
    int padc[4];
    int f1c;                                    // +0x1c
    char pad20[0x4c - 0x20];
};
struct ModeRec2 { char pad[0x34]; SpawnRec* recs; char pad2[0x58 - 0x38]; };    // 0x58, array at 0x5e2a00
extern ModeRec2* g_modes2;                      // 0x5e2a00

int __cdecl rand_below(const int* n);          // 0x432580 (0 when *n < 2, else rand() % *n)
extern 
struct Mission4 {
    char pad0[0x5df0];
    int mode;                                   // 0x5df0
    int f5df4;                                  // 0x5df4
    int f5df8;                                  // 0x5df8
    char pad5dfc[0x5e0c - 0x5dfc];
    int list[2][128];                           // 0x5e0c candidate party ids per side
    int cnt[2];                                 // 0x620c
    int f6214;                                  // 0x6214
    int f6218;                                  // 0x6218
    char pad621c[0x6550 - 0x621c];
    int f6550;                                  // 0x6550
    char pad6554[0x6824 - 0x6554];
    char done[64];                              // 0x6824
    bool rule_accepts(SpawnRec* a, Stack* b, int p3, char p4, char p5, char p6);   // 0x467940 (thiscall, ret 0x18, ignores this)
    void pick_spawn(int idx, int* out_party, int* out_stack, int* out_troop, int* out_side, int* out_flag, int* out_seed);   // 0x473a60
};

// @ 0x473a60
void Mission4::pick_spawn(int idx, int* out_party, int* out_stack, int* out_troop, int* out_side, int* out_flag, int* out_seed)
{
    *out_party = -1;
    *out_stack = -1;
    SpawnRec* r = &g_modes2[mode].recs[idx];
    *out_side = -1;
    *out_flag = -1;
    *out_seed = -1;
    *out_troop = -1;
    *out_side = ((r->flags >> 12) & 7) - 1;
    int list_idx = -1;
    int local_side = -1;
    if (r->flags & 0x10) {
        *out_flag = 0;
        *out_troop = g_game->sceneobjs[f5df4].pairs[idx].a;
        if (g_game->sceneobjs[f5df4].pairs[idx].b == -0xd0cd)
            *out_seed = g_game->f4 * 0x93 + r->f1c * 0x13 + idx * 0x25 + f5df4 * 0x4f;
        else
            *out_seed = g_game->sceneobjs[f5df4].pairs[idx].b;
    } else if (r->flags & 4) {
        if (r->id == f5df8) {
            *out_troop = g_game->player;
        } else if (*out_troop == -1 && f6550 == 0) {
            for (int i = 1; i < g_game->ntroops; i++) {
                unsigned v = g_game->troops[i].f3b8;
                if ((v & 0xffff) == (unsigned)f5df4 && ((v >> 16) & 0xff) == (unsigned)r->id && g_game->troops[i].used == 0)
                    *out_troop = i;
            }
        }
    } else if (r->flags & 0xc0) {
        int a = r->flags & 0x40;
        int b = r->flags & 0x80;
        if (((a && f6214 == 0) || (b && f6214 == 1)) || (f6214 == 2 && ((f6218 == 1 && a) || (f6218 == 0 && b)))) {
            local_side = 0;
            if (g_game->troops[g_game->player].used == 0) {
                *out_party = g_game->f78;
            } else {
                int k = rand_below(&cnt[1 - f6218]);
                *out_party = list[1 - f6218][k];
                list_idx = 1 - f6218;
            }
        } else {
            local_side = 1;
            int k = rand_below(&cnt[f6218]);
            *out_party = list[f6218][k];
            list_idx = f6218;
        }
    } else if (r->flags & 1) {
        int* pc = &cnt[f6218];
        int k;
        if (*pc > 1)
            k = rand() % *pc;
        else
            k = 0;
        *out_party = list[f6218][k];
        list_idx = f6218;
        *out_side = 1;
    } else {
        if (r->flags & 2) {
            int k = rand_below(&cnt[1 - f6218]);
            *out_party = list[1 - f6218][k];
            list_idx = 1 - f6218;
        } else {
            *out_party = g_game->f78;
        }
        *out_side = 0;
    }

    if (*out_party >= 0) {
        Party* party = get_party(*out_party);
        int end = party->nstacks;
        int start = 0;
        int step = 1;
        if (r->flags & 0x1000000) {
            start = party->nstacks - 1;
            end = -1;
            step = -1;
        }
        int total = party->sum_stat(0x112);
        char x4 = 0, x3 = 0, x5 = 0;
        int ncand = (list_idx == -1) ? 1 : cnt[list_idx];
        if (idx >= 0)
            done[idx] = 1;
        for (int c = 0; c < ncand; c++) {
            int tmpl = (list_idx == -1) ? g_game->f78 : list[list_idx][c];
            Party* p2 = get_party(tmpl);
            int n2 = p2->nstacks;
            int nfirst;
            for (nfirst = 0; nfirst < n2; nfirst++)
                if (p2->stacks[nfirst].flags & 1)
                    break;
            if (nfirst > 0) {
                for (int j = 0; j < nfirst; j++) {
                    Stack* s = &p2->stacks[j];
                    Troop* t = &g_game->troops[s->troop];
                    if ((t->flags & 0x10) == 0 && s->count > s->wounded + s->spawned) {
                        done[idx] = 0;
                        if ((t->flags & 0x1000000) && (r->flags2 & 0x100) == 0)
                            x5 = 1;
                        else if (t->flags & 0x4000000)
                            x4 = 1;
                        else
                            x3 = 1;
                    } else if (s->spawned == 0) {
                        if (!agent_low_inl(s->troop)) {
                            done[idx] = 0;
                            if (t->f4e4 < 0)
                                x3 = 1;
                            else
                                x5 = 1;
                        }
                    }
                }
            }
        }
        if (*out_party == g_game->f78 && party->stacks[0].spawned == 0) {
            *out_stack = 0;
            *out_troop = g_game->player;
            party->stacks[0].spawned = 1;
        }
        if (start != end) {
            for (int k = start; k != end; k += step) {
                if (*out_troop != -1)
                    break;
                Stack* s = &party->stacks[k];
                Troop* t = &g_game->troops[s->troop];
                __int64 hero = t->flags & 0x10;
                if ((hero != 0 && s->spawned == 0 && t->used == 0 && (s->troop == g_game->player || !g_game->agent_low(s->troop)))
                    || (hero == 0 && s->count > s->wounded + s->spawned)) {
                    if (rule_accepts(r, s, k, x3, x4, x5)) {
                        if (hero == 0) {
                            float chance = (float)(s->count - s->wounded) / (float)total + 0.25f;
                            float rnd = (float)(rand() % 15817) * (1.0f / 15817.0f);
                            if (rnd > chance)
                                continue;
                        }
                        *out_stack = k;
                        *out_troop = s->troop;
                        if ((g_game->troops[s->troop].flags & 0x10) == 0) {
                            *out_seed = rand();
                            if (s->spawned >= 0 && s->spawned < 32) {
                                if (*out_party == g_game->f78)
                                    *out_seed = g_game->tbl[k][s->spawned];
                            } else {
                                *out_seed = rand();
                            }
                        }
                        s->spawned++;
                    }
                }
            }
        }
    }
    if (*out_troop >= 0) {
        g_game->troops[*out_troop].used = 1;
        if (g_game->troops[*out_troop].flags & 0x10)
            *out_seed = -1;
    }
    if (*out_side == -1)
        *out_side = local_side;
}

// ---- pool iterator (16 entries per page, 0xb0 bytes each) -----------------------------------------------------------
struct PoolElem { int f0; int used; char pad[0xb0 - 8]; };
struct Pool {
    int count;                                  // +0
    char pad4[0x14 - 4];
    std::vector<PoolElem*> pages;               // +0x14 (first at +0x18)
    bool valid(int i) { return i < count; }
    void next_used(unsigned* i);                // 0x474300
};
// @ 0x474300
void Pool::next_used(unsigned* i)
{
    ++*i;
    while (valid(*i)) {
        if (pages[*i >> 4][*i & 15].used != 0)
            return;
        ++*i;
    }
}

// ---- vector<ArrSet>::_Ucopy (ArrSet = 0x38-byte name + five count/array pairs, see s0046bc70) --------------------------
struct ArrSet;
struct allocArr {};
struct nonscalar_tag {};
ArrSet* __cdecl uninit_copy_arrset(ArrSet* f, ArrSet* l, ArrSet* d, allocArr& al, nonscalar_tag tag);   // 0x46c1d0
struct vecArr : allocArr { ArrSet* first; ArrSet* last; ArrSet* end; ArrSet* ucopy(ArrSet* f, ArrSet* l, ArrSet* d); };
// @ 0x474350
ArrSet* vecArr::ucopy(ArrSet* f, ArrSet* l, ArrSet* d)
{
    nonscalar_tag t;
    return uninit_copy_arrset(f, l, d, *this, t);
}

// ---- std::stable_sort pieces for 8-byte records ---------------------------------------------------------------------
struct P8 { int key, val; };
typedef bool (__cdecl *Pred8)(const P8&, const P8&);
typedef std::vector<P8>::iterator It8;
// @ 0x474370 (_Insertion_sort)
template void std::_Insertion_sort(It8, It8, Pred8);
