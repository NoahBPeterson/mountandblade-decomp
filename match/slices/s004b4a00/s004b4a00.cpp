// Slice s004b4a00: the mission agent's "threat/target scoring" step (0x4b4a00, ~2.7 KB). Called from
// agent::tick (0x4b80d0); it gathers nearby parties (0x4b3910), scores each one from troop strength,
// relation and distance, and either commits a new action (this+0x918/0x91c) or falls back to the pending
// action stored at this+0x8e4. Everything is float-heavy x87; field offsets below come from this function
// and neighbouring slices (Party 0x5ac8 in s00457b40, GameData/PartyMgr in s0049f060).
// The exact score formula is provisional (see the bookkeeping note).
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

// ---- globals ---------------------------------------------------------------------------------------------
struct GameData;
extern GameData* g_game;                        // 0x5e2930
extern __int64 g_ticks[];                       // 0x5e2948 (100 kHz clocks; [party->f5ab0 * 2])

// ---- party record (0x5ac8 bytes; layout from s00457b40 / s004608d0) ---------------------------------------
struct Party {
    char pad0[4];
    int f4;                                     // +4 (== 1 for a live party)
    char pad8[0x30 - 8];
    float x;                                    // +0x30
    float y;                                    // +0x34
    char pad38[0x50 - 0x38];
    float angle;                                // +0x50
    char pad54[0x70 - 0x54];
    int f70;                                    // +0x70 (troop index for the harvest pair table)
    unsigned f74;                               // +0x74 (packed: low nibble + bit 0x100)
    float f78;                                  // +0x78
    char pad7c[0x88 - 0x7c];
    int f88;                                    // +0x88 (sub-party / attached party id, -1 none)
    char pad8c[0xa0 - 0x8c];
    unsigned flags;                             // +0xa0
    int fa4;                                    // +0xa4
    char pada8[0x8ac - 0xa8];
    float f8ac;                                 // +0x8ac
    float f8b0;                                 // +0x8b0
    char padb4[0x8c8 - 0x8b4];
    int f8c8;                                   // +0x8c8
    char pad8cc[0x918 - 0x8cc];
    int action;                                 // +0x918
    char pad91c[0x5ac8 - 0x91c];                // record stride used by the party table
};
struct BodyAgent {                              // 0x98-byte record; g_game+0xe4 pages of 16
    char pad0[0x54];
    int f54;                                    // +0x54
    unsigned f58;                               // +0x58
    char pad5c[0x98 - 0x5c];
};

// ---- party table at g_game+0xf8 (see s0049f060) -----------------------------------------------------------
struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                              // +0x138d0 (g_game+0x139c8; page = 16 records)
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                             // +0x138dc (g_game+0x139d4)
    int index(int id);                          // 0x48ad00
};

struct GameData {
    char pad0[0x78];
    int camera_id;                              // +0x78
    char pad7c[0xe4 - 0x7c];
    BodyAgent** bodies;                         // +0xe4 (id >> 4 -> page, id & 0xf -> 0x98 record)
    char pade8[0xf8 - 0xe8];
    PartyMgr parties;                           // +0xf8
    char pad139d8[0x13a48 - 0x139d8];
    float* harvest;                             // +0x13a48 (0xc0 floats per first troop)
    float party_value(int id);                  // 0x4b4800 (this = g_game)
    float relation(int a, int b);               // 0x48ffa0
};

// ---- the agent (0x5928 bytes; base offsets from s00440a30 / s004b80d0) -------------------------------------
struct agent {
    char pad0[8];
    int id;                                     // +8
    char padc[0x30 - 0xc];
    float x;                                    // +0x30
    float y;                                    // +0x34
    char pad38[0x70 - 0x38];
    int f70;                                    // +0x70 (troop index)
    unsigned f74;                               // +0x74 (packed team/faction nibbles)
    char pad78[0x88 - 0x78];
    int troop;                                  // +0x88 (-1 when unattached)
    char pad8c[0xa0 - 0x8c];
    unsigned flags;                             // +0xa0
    char pada4[0x8ac - 0xa4];
    float f8ac;                                 // +0x8ac
    float f8b0;                                 // +0x8b0
    char padb4[0x8c8 - 0x8b4];
    int f8c8;                                   // +0x8c8
    char pad8cc[0x8e4 - 0x8cc];
    int state;                                  // +0x8e4 (pending action)
    int state_target;                           // +0x8e8 (pending target)
    int state_f8ec;                             // +0x8ec
    int state_f8f0;                             // +0x8f0
    char pad8f4[0x918 - 0x8f4];
    int action;                                 // +0x918
    int target;                                 // +0x91c
    char pad920[0x924 - 0x920];
    int f924;                                   // +0x924
    int f928;                                   // +0x928
    char pad92c[0x958 - 0x92c];
    float f958;                                 // +0x958 (aggression scale)
    char pad95c[0x5ab0 - 0x95c];
    int f5ab0;                                  // +0x5ab0
    char pad5ab4[0x5ab8 - 0x5ab4];
    unsigned f5ab8;                             // +0x5ab8
    unsigned f5abc;                             // +0x5abc

    void step_timing();                         // 0x4b4a00
    void refresh_stat();                        // 0x4b2120 (updates +0x8b0)
    void collect_nearby(int* ids, int* maxn, int* count);   // 0x4b3910
    void commit();                              // 0x4b3bc0
};

// @ 0x4b4a00
void agent::step_timing()
{
    if (action == 7 || action == 8)
        return;
    if (troop != -1)
        return;
    if (flags & 0x400)
        return;

    int ids[32];
    int maxn = 32;
    int n;
    collect_nearby(ids, &maxn, &n);

    float base = g_game->party_value(id);
    float hi = base + 0.1f;
    float lo = 0.0f;
    refresh_stat();

    float near_v[32];
    float adj_v[32];
    float rel_v[32];
    float accum[32];
    float score[32];
    int kind[32];
    char friendly[32];

    if (n > 0) {
        for (int i = 0; i < n; i++) {
            accum[i] = 0.0f;
            adj_v[i] = 0.0f;
            kind[i] = 0;
            score[i] = 0.0f;
        }
        for (int i = 0; i < n; i++) {
            int tid = ids[i];
            int pi = g_game->parties.index(tid);
            if (pi < 0)
                continue;
            Party* p = (Party*)((char*)g_game->parties.pages[pi >> 4]
                                + (pi & 0xf) * sizeof(Party));
            float rel = g_game->relation(id, tid);
            rel_v[i] = rel;
            float v = g_game->party_value(tid);
            near_v[i] = v;
            adj_v[i] = v;
            friendly[i] = 1;
            int sub = p->f88;
            if (sub >= 0) {
                BodyAgent* b = (BodyAgent*)((char*)g_game->bodies[sub >> 4]
                                            + (sub & 0xf) * 0x98);
                unsigned other = b->f58;
                if (other == (unsigned)tid)
                    other = (unsigned)b->f54;
                int oi = -1;
                if (other < g_game->parties.count)
                    oi = g_game->parties.index((int)other);
                if (oi >= 0) {
                    Party* op = (Party*)((char*)g_game->parties.pages[oi >> 4]
                                         + (oi & 0xf) * sizeof(Party));
                    if (op->f4 == 1) {
                        float ov = g_game->party_value(other);
                        float a = v - ov * 0.9f;
                        if (a < 1.0f)
                            a = 1.0f;
                        adj_v[i] = a;
                        accum[i] = ov + accum[i];
                        friendly[i] = g_game->harvest[op->f70 * 0xc0 + f70 + 0xb] >= 0.0f;
                    }
                }
            }
            if (sub == -1) {
                if (rel < -0.01f)
                    lo = v + lo;
                else if (rel > 0.01f)
                    hi = v + hi;
            }
        }

        float frac = lo / hi;
        float inv = 1.0f / frac;
        unsigned a74 = f74;
        float hi_nib = (float)((int)a74 >> 4 & 0xf);
        float prod = f8b0 * f8ac;
        float prod2 = prod + 0.05f;

        for (int i = 0; i < n; i++) {
            int tid = ids[i];
            int pi = g_game->parties.index(tid);
            if (pi < 0)
                continue;
            Party* p = (Party*)((char*)g_game->parties.pages[pi >> 4]
                                + (pi & 0xf) * sizeof(Party));
            float adj = near_v[i] - accum[i] * 0.9f;
            if (adj < 1.0f)
                adj = 1.0f;
            float mod = (a74 & 0xf) == 0 ? 1.0f : (float)(a74 & 0xf) * 0.125f;
            mod = mod * ((base / adj) * inv);
            if (mod < 0.2f)
                mod = 0.2f;
            else if (mod > 5.0f)
                mod = 5.0f;

            float ax = p->x - x;
            float ay = p->y - y;
            float dist = (float)sqrt(ay * ay + ax * ax);
            if (dist > 0.0f) {
                ax = ax * (1.0f / dist);
                ay = ay * (1.0f / dist);
            } else {
                ax = 0.0f;
                ay = 1.0f;
            }
            float invd = 3.0f / dist;
            if (invd < 0.6f)
                invd = 0.6f;
            else if (invd > 2.0f)
                invd = 2.0f;

            float scale = prod / (p->f8ac * p->f8b0);
            if (scale < 0.5f)
                scale = 0.5f;
            else if (scale > 1.1f)
                scale = 1.1f;

            float sc = (((-rel_v[i] * hi_nib) * scale) * invd) * mod * 0.125f;
            if (rel_v[i] < 0.0f) {
                float s = (a74 & 0x100) ? 1.0f : 0.0f;
                float alt = (((s * p->f78) * scale) * invd) * mod;
                if (sc < alt)
                    sc = alt;
            }
            if (near_v[i] * 3.0f < accum[i]) {
                sc = (near_v[i] / accum[i]) * sc;
                sc = sc + sc;
            }

            float freq = (a74 & 0xf) == 0 ? 1.0f : (float)(a74 & 0xf) * 0.125f;
            if (mod < 1.0f / freq || !friendly[i]) {
                sc = 0.0f;
            } else if ((unsigned)tid == (unsigned)g_game->camera_id) {
                __int64 d = g_ticks[f5ab0] - (__int64)(unsigned)f5ab8;
                d -= (__int64)(unsigned)f5abc << 32;
                if ((double)d * 1e-05 < 0.0)
                    sc = 0.0f;
            }

            if (prod2 < p->f8ac * p->f8b0 && sc > 0.01f && p->f88 < 0) {
                float ang = p->angle;
                float s = (float)sin(ang);
                float c = (float)cos(ang);
                if (0.8f < (s * 0.0f + c) * ay + (c * 0.0f - s) * ax)
                    sc = sc * 0.1f;
            }

            if (score[i] < sc) {
                score[i] = sc;
                kind[i] = 4;
            }

            float alt2 = 0.0f;
            if (rel_v[i] < -0.01f && mod < 1.0f
                && (float)((int)p->f74 >> 4 & 0xf) * 0.125f > 0.0f
                && (p->flags & 0x400) == 0 && p->f88 < 0 && p->f8c8 < 0)
                alt2 = ((1.0f - rel_v[i]) * invd) / (mod * mod) - 1.0f;
            if (score[i] < alt2) {
                score[i] = alt2;
                kind[i] = 5;
            }
        }
    }

    float best = -10.0f;
    int bi = -1;
    for (int i = 0; i < n; i++) {
        if (best < score[i]) {
            best = score[i];
            bi = i;
        }
    }
    best = best * f958;

    int r = rand();
    float rnd = (float)(r % 0x3dc9) * 3.1611557e-06f + 0.15f;

    if (bi != -1 && !(best < rnd) && !(flags & 0x10000)) {
        action = kind[bi];
        target = ids[bi];
        commit();
        return;
    }

    bool pending_bad = false;
    if (state == 4) {
        unsigned pid = (unsigned)state_target;
        if (pid < g_game->parties.count) {
            int oi = g_game->parties.index((int)pid);
            if (oi >= 0) {
                Party* pp = (Party*)((char*)g_game->parties.pages[oi >> 4]
                                     + (oi & 0xf) * sizeof(Party));
                if (pp->f4 == 1) {
                    int oi2 = g_game->parties.index((int)pid);
                    Party* pp2 = (Party*)((char*)g_game->parties.pages[oi2 >> 4]
                                          + (oi2 & 0xf) * sizeof(Party));
                    if (pp2->f8c8 < 0 && (pp2->flags & 0x1080100) == 0) {
                        int oi3 = g_game->parties.index((int)pid);
                        Party* pp3 = (Party*)((char*)g_game->parties.pages[oi3 >> 4]
                                              + (oi3 & 0xf) * sizeof(Party));
                        pending_bad = pp3->action == 8;
                    } else {
                        pending_bad = true;
                    }
                } else {
                    pending_bad = true;
                }
            } else {
                pending_bad = true;
            }
        } else {
            pending_bad = true;
        }
    }

    if (state == 4 && pending_bad) {
        if (bi != -1) {
            action = kind[bi];
            target = ids[bi];
            commit();
            return;
        }
        action = 2;
        f924 = *(int*)((char*)this + 0x30);
        f928 = *(int*)((char*)this + 0x34);
        commit();
        return;
    }

    action = state;
    target = state_target;
    f924 = state_f8ec;
    f928 = state_f8f0;
    if (action == 2 || action == 3) {
        int q = rand();
        if ((float)(q % 0x3dc9) * 6.322312e-05f < 0.25f) {
            action = 0;
            commit();
            return;
        }
    }
    commit();
}
