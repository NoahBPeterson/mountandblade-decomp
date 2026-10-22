// Slice s004b5c30: three mission-side helpers around the 92-byte event record (see s004b54a0):
//  0x4b5c30  a large "how strong is this group" evaluator over a collected troop list,
//  0x4b5fe0  pushes an event record seeded from a position with a random facing,
//  0x4b6070  pushes an event record for a party at a position (with per-troop kind bits).
// Offsets come from these functions plus Party/GameData from the neighbouring slices. The evaluator is
// reconstructed from the disassembly (see bookkeeping for its verification status).
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <vector>

struct E92 { int v[23]; ~E92() {} };            // 0x5c: mission event record (vector element)

// ---- world / party records --------------------------------------------------------------------------------
struct Party {                                  // 0x5ac8
    char pad0[0x8c];
    int troop;                                  // +0x8c
    char pad90[0x5ac8 - 0x90];
};

struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                              // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                             // +0x138dc
    int index(int id);                          // 0x48ad00
};

union EventFlags {                              // the dword at event+0x18
    unsigned all;
    struct {
        unsigned low : 8;                       // +0x18 bits 0..7
        unsigned mid : 8;                       // +0x19 bits 8..15
        unsigned high : 4;                      // +0x1a bits 16..19
        unsigned unused : 8;
        unsigned top : 4;                       // +0x1b bits 28..31
    } f;
};
struct Pos4 { int x, y, z, w; };
struct event_rec {                              // 0x5c
    Pos4 p;                                     // +0x00 (position / four dwords)
    float angle;                                // +0x10 (random facing or supplied heading)
    int f14;                                    // +0x14
    EventFlags flags;                           // +0x18
    char pad1c[0x5c - 0x1c];
};

struct GameData {
    char pad0[8];
    int id;                                     // +0x08
    char pad0c[0xe8 - 0x0c];
    void** bodies;
    char padec[0xf8 - 0xec];
    PartyMgr parties;                           // +0xf8
    char pad139d8[0x13a00 - 0x139d8];
    char* troops;                               // +0x13a00 (0x508-byte troop records)
    char pad13a04[0x13a60 - 0x13a04];
    std::vector<E92> events;                    // +0x13a60
    void fill_parties(int id, void* rec);       // 0x48fd60 (this = g_game)
    void _collect_party_units(int party, int* a, int* b, int* c, int* d);  // 0x490ce0
    void _init_event(event_rec* r);             // 0x4b36a0
    void _push_event(const int* pos, unsigned nation);                // 0x4b5fe0
    void _push_party_event(int party, const int* pos, const float* src);  // 0x4b6070
};
extern GameData* g_game;                        // 0x5e2930
extern char* g_units;                           // 0x5e29d0 (0x238-byte unit records)

// ---- troop helpers ----------------------------------------------------------------------------------------
struct UnitRef { int id; int pad; };            // 8-byte entry of the troop's unit list
struct Troop {                                  // 0x508 bytes
    char pad0[0x30];
    unsigned flags;                             // +0x30 (bit 0x10 = has unit list)
    char pad34[0x80 - 0x34];
    UnitRef units[1];                           // +0xb4
    int tier();                                 // 0x462b60
    float value();                              // 0x4b21f0
};
struct UnitRec {                                // 0x238 bytes
    char pad0[0x1c8];
    unsigned kind;                              // +0x1c8 (low byte)
    char pad1cc[0x208 - 0x1cc];
    float power;                                // +0x208
};

// ---- the collected-list record (built on the stack by 0x4b5c30) -------------------------------------------
struct list_entry {                             // 0x20 bytes
    int count;                                  // +0
    int troop;                                  // +4 (index into g_game->troops)
    unsigned flags;                             // +8 (bit 0 = the "other" side)
    char padc[0x20 - 0xc];
};
struct RecR {                                   // ctor 0x4934d0, dtor 0x431b90
    RecR();                                     // 0x4934d0
    ~RecR();                                    // 0x431b90
    void prepare();                             // 0x4b1eb0
    char pad0[0xa8];
    list_entry entries[64];                     // +0xa8
    int count;                                  // +0x8a8
    char pad8ac[0x8b0 - 0x8ac];
    int f8b0;                                   // +0x8b0
    char pad8b4[0x5ac8 - 0x8b4];                // the record is a party-sized scratch block
};

// the object passed as the first argument of 0x4b5c30 (a mission "group")
struct GroupObj {
    char pad0[8];
    int id;                                     // +0x08
    char pad0c[0xa6 - 0x0c];
    unsigned char nation;                       // +0xa6
    int bonus(int kind);                        // 0x4b2370
};

// @ 0x4b5c30
float __cdecl evaluate_group(GroupObj* self, char* other)
{
    float ally_sum = 0.0f;
    float min_val = 100000.0f;
    int total = 0;
    int ally_total = 0;
    int other_sum = 0;
    int kind1 = 0;
    int bonus = self->bonus(0xe);
    float base_power = (float)self->nation * 50.0f;

    RecR rec;
    rec.count = 0;
    g_game->fill_parties(self->id, &rec);
    rec.prepare();

    if (rec.count > 0) {
        char* troops = g_game->troops;
        char* units = g_units;
        for (int i = 0; i < rec.count; i++) {
            list_entry* e = &rec.entries[i];
            Troop* t = (Troop*)(troops + e->troop * 0x508);
            float v = t->value();
            int n = e->count;
            if (e->flags & 1)
                other_sum = other_sum + n;
            else
                ally_total = ally_total + n;
            ally_sum = (float)n * v + ally_sum;
            if (v < min_val)
                min_val = v;
            total = total + n;
            if (t->flags & 0x10) {
                int cnt = t->tier();
                for (int k = 0; k < cnt; k++) {
                    int u = t->units[k].id;
                    if (u < 0)
                        continue;
                    UnitRec* ur = (UnitRec*)(units + u * 0x238);
                    if ((ur->kind & 0xff) == 0xb)
                        base_power = base_power + ur->power;
                    else if ((ur->kind & 0xff) == 1)
                        kind1 = kind1 + 1;
                }
            }
        }
    }

    float fx = (*(float*)(other + 0x7c) - 0.5f) * 0.05f + 1.0f;
    float fy = 1.0f - *(float*)(other + 0x80) * 0.1f;
    if (min_val > 10000.0f)
        min_val = 0.0f;
    if (ally_sum == 0.0f)
        return 0.0f;
    int logarg = (other_sum >> 1) + 0x10 + (kind1 >> 1) + ally_total;
    float mid = min_val * 0.2f + (ally_sum / (float)total) * 0.8f;
    float inv = 1.0f / ((base_power / (float)(total + 3 + kind1 * 4)) * 0.007f + 1.0f);
    float r = ((mid * inv) * (1.0f + (float)bonus * 0.03f)) * fy;
    r = r * fx * 2.6f;
    r = r / log((float)logarg);
    return r;
}

// ---- the mission event record -----------------------------------------------------------------------------
// @ 0x4b5fe0
void GameData::_push_event(const int* pos, unsigned nation)
{
    event_rec r;
    r.p = *(const Pos4*)pos;
    r.angle = (float)(rand() % 15817) * 0.00039724255f;
    r.f14 = 0;
    r.flags.all = (nation & 0xff) | 0x10000000;
    _init_event(&r);
    events.push_back(*(E92*)&r);
}

// @ 0x4b6070
void GameData::_push_party_event(int party, const int* pos, const float* src)
{
    PartyMgr* pm = &parties;
    int idx = pm->index(party);
    Party* p = pm->pages[idx >> 4] + (idx & 0xf);
    event_rec r;
    r.p = *(const Pos4*)pos;
    r.angle = *src;
    int a = 0, b = 0, c = 0, d = 0;
    r.f14 = 0;
    _collect_party_units(party, &a, &b, &c, &d);
    r.flags.f.low = (a + c) & 0xff;
    r.flags.f.high = rand() % 16;
    int troop = p->troop;
    if (troop == -1)
        troop = 0xff;
    r.flags.f.mid = troop;
    _init_event(&r);
    events.push_back(*(E92*)&r);
}
