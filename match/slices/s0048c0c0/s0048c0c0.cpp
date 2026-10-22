// Slice s0048c0c0: an agent's item-slot manager (8-byte item references gathered into 0x20-byte slot
// entries), its stable sort, insert/remove helpers, an "Event" placeholder object ctor and the material
// type id. Names provisional unless noted.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "rgl.h"

// ---------- shared declarations (layouts from neighbouring verified slices) ----------
// rgl string, 16 bytes (see s00429890 / s0044f3c0); it is the single global object's type too
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    void assign(const char* s);                 // 0x42bcb0
};

// item kind record (0x238 bytes, array at 0x5e29d0); see s0044f3c0 / s00462540
struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18
struct ItemKind {
    char pad0[0x20];
    tstring name;                       // +0x20
    char pad30[0x44 - 0x30];
    int mesh_count;                     // +0x44
    ItemMesh meshes[1];                 // +0x48
    char pad60[0x1c8 - 0x60];
    unsigned type_flags;                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[4];
    unsigned __int64 flags;             // +0x1d0 (itp_* bits)
    int f1d8;                           // +0x1d8
    char pad1dc[4];
    unsigned f1e0;                      // +0x1e0 (usage bits, low)
    unsigned f1e4;                      // +0x1e4 (usage bits, high)
    int f1e8;                           // +0x1e8
    char pad1ec[0x238 - 0x1ec];
};
extern ItemKind* g_items;               // 0x5e29d0
extern int g_item_count;                // 0x5e29d4
extern __int64 g_bit[];                 // 0x5a8e68 (g_bit[i] == 1 << i)

enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets, itp_type_animal, itp_type_book
};

struct ItemRef;

// an item reference with its packed amount / modifier word (see s00462540)
struct ItemRef {
    int item;
    union {
        unsigned m4;
        struct { unsigned char amount; unsigned char pad[2]; signed char mod; } b;
    };
    void reduce(int n);                 // 0x437510
    void set(int item_);                // 0x48c5a0
    void randomize_modifier(int kind);  // 0x48c5c0
};

int item_modifier_value(const ItemRef* r);      // 0x434df0 (result of a modifier lookup on the item)

// one slot entry: two item references plus bookkeeping, 0x20 bytes, table at agent +0xa8
struct ItemSlot {
    ItemRef a;                          // +0x00
    ItemRef b;                          // +0x08
    union {
        unsigned m10;
        struct { unsigned char f10; unsigned char pad11[3]; };
    };                                  // +0x10
    int f14;                            // +0x14
    int f18;                            // +0x18
    int f1c;                            // +0x1c
};

struct Agent;
struct World;

struct Agent {
    char pad0[0x30];
    unsigned __int64 flags30;           // +0x30
    char pad38[0x80 - 0x38];
    float stat[8];                      // +0x80
    unsigned fA0;                       // +0xa0 (bit 0x100000 = has custom slots)
    char padA4[4];
    ItemSlot slots[8];                  // +0xa8 (stride 0x20, only the first 8-byte reference is used)
    char pad1a8[0x4a4 - 0x1a8];
    int slot_count;                     // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    unsigned char dirty;                // +0x8b4 (set whenever the slot table changes)
    int slot_begin(int slot, unsigned mask, char remove);           // 0x489620
    int slot_add(int item, int amount, unsigned mask, int mod, char merge);   // 0x48c840
    void slot_remove(int idx, int amount, int extra, char peek);    // 0x489540
    int slot_consume(int idx, int amount, int extra, char merge);   // 0x48c3c0
    void remove_slot(int idx);          // 0x48c330
    void set_slot(int idx, int item, int mod);  // 0x48c7f0
    int tier_base();                    // 0x462b60
    void sort_slots();                  // 0x48c0c0
    void give_item(int item, int count);    // 0x48c9b0
};

// the agent container (only the troop array is used here)
struct World {
    char pad0[0x13a00];
    Agent* agents;                      // +0x13a00 (stride 0x508)
    void init_slots(int troop_idx, int want);   // 0x48c4c0
};
extern World* g_world;                  // 0x5e2930

float slot_weight(float* out, float* in, int slot);     // 0x488ff0
extern float g_imod_hi[];               // 0xa75f90 pool (stride 0x28)
extern float g_imod_lo[];               // 0xa75f94

// ---- 0x48c0c0: sort the agent's slot entries by (modifier-adjusted) slot value -------------------------
// @ 0x48c0c0
void Agent::sort_slots()
{
    int* const table = (int*)((char*)this + 0xb4);
    int count = tier_base();
    slot_begin(count, (unsigned)count, 0);
    int passes = count;
    int limit = count - 1;
    do {
        int i = limit;
        for (int* p = table; i > 0; p += 2, i--) {
            int l, r;
            if (*p < 0) {
                l = 0;
            } else {
                float v = (float)g_items[*p].f1d8 * *(float*)((char*)g_bit + *((char*)p + 7) * 0x28);
                int n = (int)floor(v);
                if (!(v - (float)floor(v) <= 0.5f))
                    n++;
                l = n;
            }
            if (p[2] < 0) {
                r = 0;
            } else {
                float v = (float)g_items[p[2]].f1d8 * *(float*)((char*)g_bit + *((char*)p + 0xf) * 0x28);
                int n = (int)floor(v);
                if (!(v - (float)floor(v) <= 0.5f))
                    n++;
                r = n;
            }
            if (l < r || (l == r && p[0] > p[2])) {
                int t = p[0]; p[0] = p[2]; p[2] = t;
                t = p[1]; p[1] = p[3]; p[3] = t;
            }
        }
        passes--;
    } while (passes != 0);
}

// ---- 0x48c210 / 0x48c260: "Event" placeholder object ------------------------------------------------
struct Event {
    void* vtable;                       // +0x00 (0x5922cc)
    char pad04[0x10 - 4];
    int f10;                            // +0x10
    tstring name;                       // +0x14
    int f24;                            // +0x24
    float f28;                          // +0x28
    char pad2c[0x34 - 0x2c];
    float f34;                          // +0x34
    float f38;                          // +0x38
    char pad3c[0x40 - 0x3c];
    float f40;                          // +0x40
    float f44;                          // +0x44
    char pad48[0x50 - 0x48];
    int f50;                            // +0x50
    char pad54[0x60 - 0x54];
    int f60;                            // +0x60
    char pad64[4];
    int f68;                            // +0x68
    int f6c;                            // +0x6c
    char pad70[0x80 - 0x70];
    int f80;                            // +0x80
    char pad84[4];
    int f88;                            // +0x88
    int f8c;                            // +0x8c
    int f90;                            // +0x90
    Event();                            // 0x48c210
    int init();                         // 0x48c260
};
extern int g_cfg2950;                   // 0x5e2950
extern int g_cfg2954;                   // 0x5e2954
extern int g_cfg2948;                   // 0x5e2948
extern int g_cfg294c;                   // 0x5e294c

// @ 0x48c210
Event::Event()
{
    vtable = (void*)0x5922cc;
    name.assign("");
    f60 = 1;
    f68 = g_cfg2950;
    f6c = g_cfg2954;
    f80 = 0;
    f88 = g_cfg2948;
    f8c = g_cfg294c;
}

// @ 0x48c260
int Event::init()
{
    int seed;
    f10 = 0;
    name.assign("An Event");
    f24 = 0;
    f28 = 0.0f;
    f50 = 0;
    float s = 0.0f;
    float c = 1.0f;
    f34 = s;
    f38 = c;
    seed = rand();
    float a = (float)(seed % 15817) * 0.00039724255f;
    float ca = cos(a);
    float sa = sin(a);
    f38 = sa * f34 + ca * f38;
    f34 = ca * s - sa * c;
    f40 = 0.3f;
    f44 = -10000.0f;
    f90 = -1;
    return seed;
}

// ---- 0x48c330: remove slot entry idx, shifting the list down -----------------------------------------
// @ 0x48c330
void Agent::remove_slot(int idx)
{
    if ((slots[idx].f10 & 1) || idx < 1) {
        this->dirty = 1;
        return;
    }
    int i = idx;
    ItemSlot* p = &slots[idx - 1];
    while (1) {
        ItemSlot t = *p;
        *p = p[1];
        p[1] = t;
        i--;
        if (i <= 1)
            break;
        p--;
    }
    this->dirty = 1;
}

// ---- 0x48c3c0: count a slot entry down; drop it when it runs out --------------------------------------
// @ 0x48c3c0
int Agent::slot_consume(int idx, int amount, int extra, char merge)
{
    ItemSlot* e = &slots[idx];
    int v = e->a.b.amount - amount;
    e->a.b.amount = v;
    if (merge) {
        int t = e->f10 - amount;
        e->f10 = t;
        t = t > 0 ? t : 0;
        e->f10 = t;
    } else {
        if (e->f10 > v)
            e->f10 = v;
    }
    if (e->f18 > v)
        e->f18 = v;
    if (v <= 0) {
        ItemSlot out;
        slot_remove(idx, amount, extra, (char)merge);
        this->dirty = 1;
        return 1;
    }
    this->dirty = 1;
    return 0;
}

// ---- 0x48c460: material type id ----------------------------------------------------------------------
// @ 0x48c460
int material_type()
{
    return 4;
}

// ---- 0x48c470: a name built from an id (0x18-byte records) --------------------------------------------
struct IdRecord {                       // 0x18 bytes; only the index is used
    int id;
    char pad4[0x14];
};
struct IdStore {
    IdRecord* records;                  // 0x00
    int count;                          // +0x04
    tstring name;                       // +0x08 (the store starts at 0xa7045c)
    IdRecord* record(int index);        // 0x435790
    void* record3(int a, int b, int c); // 0x435790 (3-arg form)
};
struct StoreGlobal {
    char pad0[0x0c];
    IdStore* store;                     // +0x0c
};
extern IdStore g_idstore;               // 0xa7045c (name at 0xa70464)
extern StoreGlobal g_store;             // 0xa7045c

// @ 0x48c470
IdRecord* __stdcall id_name(int id, IdRecord* rec)
{
    g_idstore.name.assign("");
    IdRecord* found = g_idstore.record(id / 0x18);
    found->id = id;
    found->pad4[0] = 0;
    *rec = *found;
    return rec;
}
/* RETIRED
struct IdRecord {                       // 0x18 bytes; only the index is used
    int id;
    char pad4[0x14];
};
*/

// ---- 0x48c4c0: materialise an agent's item slots from its troop record --------------------------------
// @ 0x48c4c0
void World::init_slots(int troop_idx, int want)
{
    Agent* ag = &agents[troop_idx];
    ItemRef* s = (ItemRef*)((char*)ag + 0xb4);
    int n = ag->tier_base();
    int filled = 0, i;
    for (i = 0; i < n; i++)
        if (((ItemRef*)((char*)this + 0xb4))[i * 4].item == -1)
            filled++;
    for (i = 0; i < 200 && filled < want; i++) {
        int pick = n > 1 ? rand() % n : 0;
        int item = s[pick * 4].item;
        if (item != -1 && !(g_items[item].type_flags & 0x10)) {
            s[pick * 4].item = -1;
            filled++;
        }
    }
    ag->slot_begin((int)this, (unsigned)ag->tier_base(), 0);}

// ---- 0x48c570: copy 8-byte records forward, returning the end ----------------------------------------
// @ 0x48c570
void* __stdcall copy_records8(void* first, void* last, void* dst)
{
    int n = (int)((char*)last - (char*)first) >> 3;
    int bytes = n * 8;
    return (char*)memmove(dst, first, (size_t)bytes) + bytes;
}

// ---- 0x48c5a0 -----------------------------------------------------------------------------------------
// @ 0x48c5a0
void ItemRef::set(int item_)
{
    item = item_;
    m4 = 0;
    reduce(0);
}

// ---- 0x48c5c0: assign a random item modifier to this reference ---------------------------------------
// @ 0x48c5c0
void ItemRef::randomize_modifier(int kind)
{
    if (item < 0)
        return;
    if ((float)(rand() % 15817) * 6.322312e-05f < 0.1f)
        return;
    int found = -1;
    float base_in = 1.0f, base_out = 1.0f;
    float total = slot_weight(&base_out, &base_in, kind);
    float ref = total;
    unsigned lo = g_items[item].f1e0;
    unsigned hi = g_items[item].f1e4;
    for (int i = 0; i < 0x2b; i++) {
        if ((lo & (unsigned)g_bit[i]) != 0 || (hi & (unsigned)(g_bit[i] >> 32)) != 0)
            total = slot_weight(&g_imod_lo[i * 5], &g_imod_hi[i * 5], kind) + total;
    }
    float target = (float)(rand() % 15817) * total * 6.322312e-05f;
    if (target > ref) {
        total = ref;
        for (int i = 0; i < 0x2b; i++) {
            if (found != -1)
                break;
            if ((g_items[item].f1e0 & (unsigned)g_bit[i]) != 0 ||
                (g_items[item].f1e4 & (unsigned)(g_bit[i] >> 32)) != 0) {
                total = slot_weight(&g_imod_lo[i * 5], &g_imod_hi[i * 5], kind) + total;
                if (target < total)
                    found = i;
            }
        }
    }
    if (found < 0)
        return;
    b.mod = 0;
    m4 |= (unsigned)found << 24;
    reduce(0);
    if (item > 0) {
        unsigned t = g_items[item].type_flags & 0xff;
        if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts ||
            t == itp_type_pistol || t == itp_type_musket || t == itp_type_crossbow ||
            t == itp_type_bullets) {
            b.amount = 0;
        } else if (t == itp_type_bow || t == itp_type_one_handed_wpn || t == itp_type_book) {
            int v = item_modifier_value(this);
            *(unsigned short*)&m4 = 0;
            m4 |= (unsigned)v;
        }
    }
}

// ---- 0x48c7f0: set slot idx's item and modifier ------------------------------------------------------
// @ 0x48c7f0
void Agent::set_slot(int idx, int item, int mod)
{
    ItemRef* r = &slots[idx].a;
    r->set(item);
    if (mod >= 0 && mod < 0x2b) {
        r->b.mod = 0;
        r->m4 |= (unsigned)mod << 24;
        r->reduce(0);
    }
}

// ---- 0x48c840: add an item to the agent's slots (merged with an equal entry, or a new one) ------------
// @ 0x48c840
int Agent::slot_add(int item, int amount, unsigned mask, int mod, char merge)
{
    if (amount == 0)
        return 0;
    int idx = slot_begin((int)this, mask, merge == 0);
    if (idx < 0) {
        this->dirty = 1;
        return -1;
    }
    if (idx == this->slot_count) {
        ItemSlot* e = &slots[idx];
        e->a.item = 0;
        e->a.m4 = 0;
        e->b.item = 0;
        e->b.m4 = 0;
        e->f10 = 0;
        e->f14 = 0;
        e->f18 = 0;
        e->f1c = 0;
        e->a.item = item;
        e->a.b.amount = -1;
        e->a.item = item;
        e->b.item = (int)mask;
        e->f14 = 0;
        e->b.item = mod;
        e->f14 = 0;
        e->f18 = amount;
        e->f1c = 0;
        e->f10 = mask;
        int k = slot_count++;
        if (!(mask & 1)) {
            remove_slot(k + 1);
            this->dirty = 1;
            return idx;
        }
    } else {
        slots[idx].f18 += amount;
        slots[idx].f14 += mod;
    }
    this->dirty = 1;
    return idx;
}

// ---- 0x48c9b0: give an agent copies of an item (first free slots) ------------------------------------
// @ 0x48c9b0
void Agent::give_item(int item, int count)
{
    int n = tier_base();
    ItemRef* s = (ItemRef*)((char*)this + 0xb4);
    for (int i = 0; i < n; i++) {
        if (count < 1)
            return;
        if (s[i * 4].item == -1) {
            count--;
            s[i * 4].set(item);
            s[i * 4].reduce(0);
        }
    }
}
