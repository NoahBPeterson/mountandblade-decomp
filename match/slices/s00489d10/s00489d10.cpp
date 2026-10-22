// Slice s00489d10: troop-record and party helpers of the world object (0x5e2930) - consuming/rolling item
// picks, party value queries, item-price globals and four tstring setters. Names provisional.
#include <windows.h>
#include <stdlib.h>
#include <string.h>

// rgl string (16 bytes); assign is the non-virtual operator= at 0x42bc50, clear at 0x42be50.
struct tstring {
    int pos; char* buf; int cap; int len;
    void assign(const tstring* o);          // 0x42bc50
    void clear();                           // 0x42be50
};

// ---- item kinds (0x5e29d0) ---------------------------------------------------------------------------
struct ItemKind {
    char pad0[0x1d8];
    int price;                              // +0x1d8
    char pad1dc[0x238 - 0x1dc];
};
extern ItemKind* g_items;                   // 0x5e29d0
extern int g_item_count;                    // 0x5e29d4
extern float g_sort_val[];                  // 0xa75f90 (5-float stride, indexed by the record's byte at +7)

struct Pair8 { int a, b; };
struct TroopRec;                            // 0x508 bytes, table behind the pointer at +0x13a00

// ---- the world object (0x5e2930) ---------------------------------------------------------------------
struct World {
    char pad0[0x78];
    int cur_party;                          // +0x78
    char pad7c[0x8c - 0x7c];
    int cur_troop;                          // +0x8c
    char pad90[0xb4 - 0x90];
    Pair8 slots[100];                       // +0xb4 (mirror of the troop record slot table)
    char pad3d4[0x13a00 - 0x3d4];
    TroopRec* recs;                         // +0x13a00 (pointer to the 0x508-byte record table)
    char pad13a04[0x13a08 - 0x13a04];
    int f13a08;                             // +0x13a08
    int f13a0c;                             // +0x13a0c (excluded record index of sl_roll_other)
    char pad13a10[0x13a58 - 0x13a10];
    char* party_recs;                       // +0x13a58 (0x138-byte party records)
    char pad13a5c[0x13ae0 - 0x13a5c];
    int range_lo;                           // +0x13ae0 (random integer range for the party roll)
    int range_hi;                           // +0x13ae4
    char pad13ae8[0x13b44 - 0x13ae8];
    float avg_value;                        // +0x13b44 (running party value)
    float avg_weight;                       // +0x13b48
    float avg_prev_time;                    // +0x13b4c
    int sl_take(int group, int item, int want, char extras);    // 0x489d10
    int sl_roll_party(int lo, int hi);                          // 0x489e70
    int sl_pick_chain(int leader, char flag, char lancer);      // 0x48a080
    int sl_cell_coord(int party, int cell);                     // 0x48a150
    int sl_party_value(int party);                              // 0x48a290
    int sl_party_price(int party);                              // 0x48a320
    void set_names(int slot, const tstring* a);                 // 0x48a380
    int sl_value_factor();                                      // 0x489f40
    void sl_update_avg();                                       // 0x48a000
    bool sl_query(int kind, int troop, int arg);                // 0x435790 (thiscall, returns char/bool)
};

struct TroopRec {                           // 0x508 bytes
    char pad0[0x30];
    int flags30;                            // +0x30 (bit 0x10 = special record)
    char pad34[0x50 - 0x34];
    int f50;                                // +0x50 (linked/leader index, used as a chain)
    char pad54[0xb0 - 0x54];
    int fb0;                                // +0xb0 (troop size)
    Pair8 slots[100];                       // +0xb4 (item, quality byte at +7)
    int count();                            // 0x462b60
};

struct PartyRec {                           // 0x138 bytes
    char pad0[0x50];
    unsigned f50;                           // +0x50 (bit 1 tested)
    char pad54[0x64 - 0x54];
    unsigned f64;                           // +0x64 (bit 8 tested)
};

extern __int64 g_ticks[];                   // 0x5e2948 (tick table; element 0 = game time in 10us)
extern int g_difficulty;                    // 0x5aa04c
extern int g_agent_count;                   // 0x5aa050
extern int g_int_5aa054;                    // 0x5aa054
extern int g_int_5aa058;                    // 0x5aa058
extern int g_stick_mode;                    // 0x5aa078
extern float g_speed_base;                  // 0x5aa07c (1.0f)
extern float g_r43;                         // 0x590b94
extern float g_r44;                         // 0x591d50
extern float g_r45;                         // 0x591d54
extern int g_store0;                        // 0xa70458
extern int g_store1;                        // 0xa7045c

extern int g_rnd();
int sl_value(const int* e);                 // 0x488f80 (item value of a slot pair)
int sl_quarter(int* p);                     // 0x488d50 (ranged-integer roll of half a slot pair)
float sl_value_factor(World* w);
int sl_value_factor2(World* w);

// ---- 0x489d10: take up to `want` records carrying item `item`, adding their values --------------------
// @ 0x489d10
int World::sl_take(int group, int item, int want, char extras)
{
    int total = 0;
    TroopRec* t = (TroopRec*)((char*)recs + group * 0x508);
    int n = t->count();
    for (int i = 0; i < n; i++) {
        if (want <= 0)
            break;
        Pair8* p = (Pair8*)((char*)t + 0xb4) + i;        if (p->a == item) {
            total += sl_value(&p->a);
            p->a = -1;
            want--;
        }
    }
    if (extras) {
        int i = 0;
        while (i < 10) {
            if (want <= 0)
                break;
            Pair8* p = (Pair8*)((char*)t + 0x4a4) + i;
            if (p->a == item) {
                total += sl_quarter(&p->a);
                p->a = -1;
                want--;
            }
            i++;
        }
    }
    return total;
}

// ---- 0x489e70: roll a random party record with bit 1 set and bit 8 clear -------------------------------
// @ 0x489e70
int World::sl_roll_party(int lo, int hi)
{
    int found = -1;
    int tries = 0;
    do {
        if (tries >= 1000)
            return found;
        int span = hi - lo;
        int v;
        if (span > 1)
            v = g_rnd() % span;
        else
            v = 0;
        found = v + lo;
        PartyRec* pi = (PartyRec*)(party_recs + found * 0x138);
        tries++;
        if ((pi->f50 >> 1 & 1) == 0) {
            found = -1;
            continue;
        }
        if ((pi->f64 >> 8 & 1) != 0) {
            found = -1;
            continue;
        }
        if (found == -1)
            continue;
    } while (found == -1);
    return found;
}

// ---- 0x489ee0: repeat a roll until it avoids the record index at +0x13a0c ------------------------------
// @ 0x489ee0
int sl_roll_other(World* w, int lo, int hi)
{
    int tries = 0;
    int span = hi - lo;
    int v;
    do {
        if (span > 1)
            v = g_rnd() % span;
        else
            v = 0;
        v += lo;
        tries++;
        if (v == *(int*)((char*)w + 0x13a0c))
            v = -1;
        if (tries > 0x186a0)
            return 0;
    } while (v == -1);
    return v;
}

// ---- 0x489f40: party value factor from unit sizes, speed and difficulty --------------------------------
// @ 0x489f40
// Returns the factor truncated to int (the x87 value is popped by the _ftol call).
int World::sl_value_factor()
{
    float f = 1.0f;
    if (slots[0].a)
        f = 0.75f;
    f = f * (1.0f - (float)(g_difficulty - 2) * g_r45);
    f = f * ((g_speed_base - 1.0f) * 0.013f + 1.0f);
    if (g_stick_mode == 1)
        f = f * g_r43;
    f = f * ((float)(g_int_5aa054 - 2) * 0.013f + 1.0f);
    f = f * ((float)(g_int_5aa058 - 2) * 0.002f + 1.0f);
    return (int)(f * ((float)(g_agent_count - 2) * 0.02f + 40.0f) * 0.02f);
}

// ---- 0x48a000: advance the party's running average value ----------------------------------------------
// @ 0x48a000
void World::sl_update_avg()
{
    float now = (float)g_ticks[0] * 1e-05f;
    float dt = now - avg_prev_time;
    float add = sl_value_factor();
    avg_value = avg_value * avg_weight + add * dt;
    avg_prev_time = now;
    float wsum = dt + avg_weight;
    avg_weight = wsum;
    avg_value = avg_value / wsum;
}

// ---- 0x48a080: pick a troop from the leader/companion chain and a roll ---------------------------------
// @ 0x48a080
int World::sl_pick_chain(int leader, char flag, char lancer)
{
    int chain[32];
    int n = 0;
    int v = ((TroopRec*)((char*)recs + leader * 0x508))->f50;
    if (v != 0) {
        chain[n] = leader;
        n = 1;
        do {
            chain[n] = v;
            v = ((TroopRec*)((char*)recs + v * 0x508))->f50;
            n++;
        } while (v != 0);
    }
    int cnt = ((TroopRec*)((char*)recs + cur_troop * 0x508))->fb0 / 3 + 1;
    int r;
    if (cnt < 2)
        r = 0;
    else
        r = g_rnd() % cnt;
    r++;
    int lo = lancer ? 0 : 1;
    if (r < lo)
        r = lo;
    if (r > n)
        return chain[n - 1];
    return chain[r];
}

// ---- 0x48a150: random absolute coordinate inside a party record's cell ---------------------------------
// @ 0x48a150
int World::sl_cell_coord(int party, int cell)
{
    extern char* g_party_recs;
    char* base = g_party_recs + party * 0xb0;
    int roll = g_rnd() % 0x3dc9;
    int lo = range_lo;
    int hi = range_hi;
    char* c = base + cell * 0x10;
    int span = *(int*)(c + 0x48) - *(int*)(c + 0x44);
    float f = (float)(hi - lo);
    f = f * ((float)roll * 6.322312e-05f);
    f = f + (float)lo;
    f = f * (float)span;
    f = f * 0.01f;
    int k = (int)f;
    if (f - (float)k > 0.5f)
        k++;
    return *(int*)(c + 0x44) + k;
}

// ---- 0x48a210: reset every item kind's fresh flag ------------------------------------------------------
// @ 0x48a210
void sl_item_flag_all()
{
    for (int i = 0; i < g_item_count; i++)
        *(int*)((char*)g_items + i * 0x238 + 0x21c) = 0x3f800000;
}

// ---- 0x48a250: store one float in every item kind ------------------------------------------------------
// @ 0x48a250
void __stdcall sl_item_set_all(float v)
{
    for (int i = 0; i < g_item_count; i++)
        *(float*)((char*)g_items + i * 0x238 + 0x224) = v;
}

// ---- 0x48a290: party value from a record's troop size (0 when absent/invalid) ---------------------------
// @ 0x48a290
int World::sl_party_value(int party)
{
    if (party >= 0) {
        g_store0 = 0;
        g_store1 = 0;
        if (sl_query(7, party, cur_party) != 0)
            return g_store0;
        TroopRec* r = (TroopRec*)((char*)recs + party * 0x508);
        if (((__int64)r->flags30 & 0x10) != 0)
            return 0;
        return r->fb0 * r->fb0 / 20 + 25;
    }
    return 0;
}

// ---- 0x48a320: a party's value scaled to the price roll ------------------------------------------------
// @ 0x48a320
int World::sl_party_price(int party)
{
    if (party < 0)
        return 0;
    g_store0 = 0;
    g_store1 = 0;
    if (sl_query(9, party, cur_party))
        return g_store0;
    int v = sl_party_value(party);
    return (int)((float)v * 5.0f + 2.0f);
}

// ---- 0x48a380: set the two named strings of record (slot, index) ---------------------------------------
// @ 0x48a380
void World::set_names(int slot, const tstring* a)
{
    ((tstring*)((char*)this + (slot + 0x1a24) * 16))->assign(a);
}

// ---- 0x48a3a0: initialise a small record's two names and counters -------------------------------------
struct NamedRec {
    int f00, f04;
    char pad08[0x1c - 0x08];
    tstring name1c;                         // +0x1c
    int f2c, f30, f34, f38, f3c, f40, f44, f48;
    int f4c;                                // +0x4c
    int f50;                                // +0x50
    char pad54[0x68 - 0x54];
    tstring name68;                         // +0x68
    void setup(const tstring* a, const tstring* b);                                  // 0x48a3a0
    void setup2(const tstring* a, const tstring* b, const tstring* c, int v);       // 0x48a3d0
};

// @ 0x48a3a0
void NamedRec::setup(const tstring* a, const tstring* b)
{
    name68.assign(a);
    name1c.assign(b);
    f4c = 0;
    f50 = 0;
    f04 = 0;
}

// @ 0x48a3d0
void NamedRec::setup2(const tstring* a, const tstring* b, const tstring* c, int v)
{
    name68.clear();
    name1c.assign(a);
    ((tstring*)((char*)this + 0x78))->assign(b);
    ((tstring*)((char*)this + 0x88))->assign(c);
    f4c = v;
    f50 = 0;
    f04 = 1;
}
