// Slice s005179f0: the per-frame party / settlement visibility update and the weather drift of the
// campaign map (GameData methods; g_game at 0x5e2930, shared with s00517350).
//   0x5179f0  GameData::update_visibility(float dt) - keeps the main party selected, walks every party that
//             can be seen, emits the sight query 0x19 and eases each party's +0x68 visibility (and its
//             scene entity fade at +0x59c0) up/down by dt*30 / dt*5, then walks the paged settlement pool
//             at +0xcc and does the same for each record's +0x44.
//   0x517e70  GameData::update_weather() - ticks every visible party, converts the frame clock pair
//             (g_ticks[+0x13b18] minus the 64-bit reference at +0x13b20) to an hour, drifts the two weather
//             parameters +0x13b3c / +0x13b40 (sun height, fog) with the morning/evening schedules and fires
//             0x5181c0 when the +0x13b2c counter advances at dusk/night.
// Names and layouts provisional; offsets from the disassembly and the neighbouring campaign slices
// s00517350 (same object, Pool at +0xcc), s004608d0 (PartyMgr at +0xf8), s004b3420 (party handle ->
// Party* paging) and s00514d40 (the g_ticks 64-bit clock union).
// flags /O2 /MT /Gy /TP
#include <math.h>
#include <stdlib.h>
#include "rgl.h"

using rgl::vector4;

// ---- the 64-bit frame clock ------------------------------------------------------------------------------
struct TickPair {                                    // 64-bit millisecond pairs, lo first
    int lo, hi;
};
union Tick64 {                                       // g_ticks[i].lo + g_ticks[i].hi * 2^32
    TickPair p;
    __int64 v;
};
extern Tick64 g_ticks[];                             // 0x5e2948

// ---- event-query result globals --------------------------------------------------------------------------
extern int g_result;                                 // 0xa70458
extern int g_store1;                                 // 0xa7045c

// ---- the paged pool of 0x98-byte records (settlements) ---------------------------------------------------
struct Pool {
    int count;                                       // +0
    char pad4[0x14];
    char** pages;                                    // +0x18  page = 16 records of 0x98 bytes
    void next(int* index);                           // 0x517960
    int first()                                      // 0x5179b0 (inlined by the callers)
    {
        int i = 0;
        if (count > 0) {
            do {
                char* rec = pages[(unsigned)i >> 4] + (i & 0xf) * 0x98;
                if (*(int*)(rec + 4) != 0)
                    break;
                i++;
            } while (i < count);
        }
        return i;
    }
};

struct SettleRec {                                   // 0x98 bytes
    char pad0[4];
    int used;                                        // +4
    char pad8[0x24 - 8];
    float x;                                         // +0x24
    float y;                                         // +0x28
    char pad2c[0x44 - 0x2c];
    float vis;                                       // +0x44
    char pad48[0x98 - 0x48];
};

// ---- parties ---------------------------------------------------------------------------------------------
struct SceneEnt {                                    // the map entity at Party+0x59c0
    void set_fade(const float* v);                   // 0x4b1800
};

struct Party {
    char pad0[0x30];
    vector4 pos;                                     // +0x30
    char pad40[0x68 - 0x40];
    float f68;                                       // +0x68  visibility 0..1
    int   f6c;                                       // +0x6c  on-map/visible state
    char pad70[0xa0 - 0x70];
    unsigned __int64 flags_a0;                       // +0xa0 (lo) / +0xa4 (hi)
    char pada8[0x8b4 - 0xa8];
    char  f8b4;                                      // +0x8b4  dirty
    char pad8b5[0x8c8 - 0x8b5];
    int   f8c8;                                      // +0x8c8  < 0 = candidate
    char pad8cc[0x59c0 - 0x8cc];
    SceneEnt* f59c0;                                 // +0x59c0 scene entity
    char pad59c4[0x5ac8 - 0x59c4];
    float sight_radius();                            // 0x4b25c0
    float strength();                                // 0x4b1fa0
    void tick();                                     // 0x4b2c00
};

struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                                   // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    unsigned deflt;                                  // +0x138dc  (g_game+0x139d4)
    int index_of(int id);                            // 0x48ad00
    int first_party_id();                            // 0x460d20
    void next_party_id(unsigned* id);                // 0x460ca0
    Party* at(int idx) { return pages[idx >> 4] + (idx & 0xf); }
};

// ---- the game data block (g_game) ------------------------------------------------------------------------
struct GameData {
    char pad0[0x78];
    int main_party;                                  // +0x78
    char pad7c[0x95 - 0x7c];
    char f95;                                        // +0x95  hide all parties
    char pad96[0xcc - 0x96];
    Pool pool;                                       // +0xcc  settlements
    char pade8[0xf8 - 0xe8];                         // +0xe8
    PartyMgr parties;                                // +0xf8
    char pad139d8[0x13b18 - 0x139d8];
    int f13b18;                                      // +0x13b18 frame index
    char pad13b1c[0x13b20 - 0x13b1c];
    int f13b20;                                      // +0x13b20 reference clock lo
    int f13b24;                                      // +0x13b24 reference clock hi
    char pad13b28[0x13b2c - 0x13b28];
    int f13b2c;                                      // +0x13b2c weather counter
    char pad13b30[0x13b3c - 0x13b30];
    float f13b3c;                                    // +0x13b3c sun height
    float f13b40;                                    // +0x13b40 fog
    char pad13b44[0x13b60 - 0x13b44];
    int f13b60;                                      // +0x13b60 last counter snapshot
    char sl_query(int a, int b, int c);              // 0x435790 (thiscall, ret 0xc)
    void f5181c0(bool arg);                          // 0x5181c0 (thiscall, ret 4)

    void f5179f0(float dt);
    void f517e70();
};

// @ 0x5179f0
void GameData::f5179f0(float dt)
{
    PartyMgr* mgr = &parties;

    mgr->at(mgr->index_of(main_party))->f6c = 1;
    float radius = mgr->at(mgr->index_of(main_party))->sight_radius();

    unsigned id = (unsigned)mgr->first_party_id();
    while (id < mgr->deflt) {
        unsigned cur = id;
        Party* p = mgr->at(mgr->index_of((int)id));
        if (cur != (unsigned)main_party && p->f8c8 < 0) {
            unsigned __int64 flags = p->flags_a0;
            if ((flags & 0x1000100) == 0 && p->f59c0 != 0) {
                Party* mp = mgr->at(mgr->index_of(main_party));
                float dx = p->pos.x - mp->pos.x;
                float dy = p->pos.y - mp->pos.y;
                float dist = (float)sqrt(dx * dx + dy * dy);
                if ((flags & 0x4000) == 0) {
                    if (dist <= radius && dist <= p->strength() * radius) {
                        g_store1 = 0;
                        g_result = 1;
                        sl_query(0x19, main_party, (int)cur);
                        if ((g_result | g_store1) != 0) {
                            float f = p->f68 + dt * 30.0f;
                            p->f68 = f;
                            if (f > 1.0f || f < -100.0f)
                                p->f68 = 1.0f;
                            if (p->f6c == 0) {
                                p->f6c = 1;
                                sl_query(0xc, (int)cur, 0);
                            }
                        } else {
                            float f = p->f68 - dt * 5.0f;
                            p->f68 = f;
                            if (f < 0.0f || f < -100.0f)
                                p->f68 = 0.0f;
                            if (p->f6c != 0) {
                                p->f6c = 0;
                                sl_query(0xd, (int)cur, 0);
                            }
                        }
                    } else {
                        float f = p->f68 - dt * 5.0f;
                        p->f68 = f;
                        if (f < 0.0f || f < -100.0f)
                            p->f68 = 0.0f;
                        if (p->f6c != 0) {
                            p->f6c = 0;
                            sl_query(0xd, (int)cur, 0);
                        }
                    }
                } else {
                    p->f68 = 1.0f;
                    if (dist <= radius * 0.5f && p->f6c == 0) {
                        p->f6c = 1;
                        p->f8b4 = 1;
                        sl_query(0xc, (int)cur, 0);
                    } else if (radius + radius < dist && p->f6c != 0) {
                        p->f6c = 0;
                        p->f8b4 = 1;
                        sl_query(0xd, (int)cur, 0);
                    }
                }
                if (f95 == 0) {
                    float v = (p->f68 < 0.01f) ? 0.0f : 1.0f;
                    p->f59c0->set_fade(&v);
                } else {
                    p->f68 = 1.0f;
                    p->f6c = 1;
                }
            }
        }
        mgr->next_party_id(&id);
    }

    int i = pool.first();
    while ((unsigned)i < (unsigned)pool.count) {
        SettleRec* r = (SettleRec*)(pool.pages[i >> 4] + (i & 0xf) * 0x98);
        if (f95 != 0) {
            r->vis = 1.0f;
        } else {
            Party* mp = mgr->at(mgr->index_of(main_party));
            float dx = r->x - mp->pos.x;
            float dy = r->y - mp->pos.y;
            float dist = (float)sqrt(dx * dx + dy * dy);
            if (dist <= radius) {
                float f = dt * 30.0f + r->vis;
                r->vis = f;
                if (f > 1.0f)
                    r->vis = 1.0f;
                else if (f < -100.0f)
                    r->vis = 1.0f;
            } else {
                float f = r->vis - dt * 5.0f;
                r->vis = f;
                if (f < 0.0f || f < -100.0f)
                    r->vis = 0.0f;
            }
        }
        pool.next(&i);
    }
}

// @ 0x517e70
void GameData::f517e70()
{
    PartyMgr* mgr = &parties;

    unsigned id = (unsigned)mgr->first_party_id();
    while (id < mgr->deflt) {
        if (id == (unsigned)main_party
            || (mgr->at(mgr->index_of((int)id))->flags_a0 & 0x80100) == 0)
            mgr->at(mgr->index_of((int)id))->tick();
        mgr->next_party_id(&id);
    }

    Tick64 cur = g_ticks[f13b18];
    Tick64 ref;
    ref.p.lo = f13b20;
    ref.p.hi = f13b24;
    int hour = (int)((float)(cur.v - ref.v) * 1e-5f) % 24;

    float v = ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.5f) * 0.1f + f13b3c;
    f13b3c = v;
    if (hour > 0 && hour < 8 && f13b40 < 0.9f) {
        float k = 1.0f;
        if (v > 0.4f)
            k = 1.5f;
        f13b40 += ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.1f) * k * 0.05f;
    } else if (hour > 10 && hour < 18 && f13b40 > 0.1f) {
        float k = 1.0f;
        if (v < 0.6f)
            k = 1.5f;
        f13b40 -= ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.1f) * k * 0.05f;
    }

    float u = (float)(rand() % 0x3dc9) * 6.3223117e-5f;
    f13b40 += ((f13b3c - 0.55f) * u) * 0.01f;

    float f = ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.5f) * 0.1f + f13b40;
    f13b40 = f;
    if (f < 0.0f)
        f13b40 = 0.0f;
    else if (f > 1.0f)
        f13b40 = 1.0f;

    if (f13b3c < 0.0f)
        f13b3c = 0.0f;
    else if (f13b3c > 1.0f)
        f13b3c = 1.0f;

    if (hour >= 18) {
        if ((float)(rand() % 0x3dc9) * 6.3223117e-5f < 0.25f && f13b2c > f13b60)
            goto fire;
    }
    if (hour >= 22 && f13b2c > f13b60) {
fire:
        f5181c0((f13b2c & 7) == 0);
        f13b60 = f13b2c;
    }
}
