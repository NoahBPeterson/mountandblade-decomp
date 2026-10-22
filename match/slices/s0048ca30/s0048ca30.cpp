// Slice s0048ca30: random loot pickers over ItemKind, agent slot filling, small fastcall lookups into the
// world's current party/campaign records, ref-counted object assignment, AABB union helpers and a colour
// setter. Names provisional unless noted.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "rgl.h"

// ---------- shared declarations (layouts from neighbouring verified slices) ----------
// rgl string, 16 bytes (see s00429890 / s0044f3c0)
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
    unsigned type_flags;                // +0x1c8 (low byte = itp_type_*, bits 0x1000/0x10000 = flags)
    char pad1cc[8];
    int f1d4;                           // +0x1d4
    int f1d8;                           // +0x1d8 (value)
    char pad1dc[4];
    unsigned f1e0;                      // +0x1e0
    unsigned f1e4;                      // +0x1e4
    int f1e8;                           // +0x1e8
    char pad1ec[0x220 - 0x1ec];
    int f220;                           // +0x220
    float f224;                         // +0x224
    char pad228[0x238 - 0x228];
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
    unsigned stat_21x(int which);       // 0x434cb0
    int stat_1e8();                     // 0x434df0
    int stat_200();                     // 0x434eb0
    int stat_204();                     // 0x462a10
    int stat_1fc();                     // 0x462960
};

// The item-reference table that lives inside an agent at +0xb4: 0x48cc40 passes that address as
// `this` with `tier_base()` as the only argument (it walks 8-byte ItemRef entries and rewrites
// empty ones, so it is the table's own method, not the agent's).
struct SlotList {
    void finish(int count);             // 0x48be00
};

struct Agent;
struct World;

// An agent is 0x508 bytes (the array stride in 0x48ca30 and 0x48cc40); its item-reference table
// starts at +0xb4 with an 8-byte stride. Only the members this slice touches are named.
struct Agent {
    char pad0[0x30];
    unsigned __int64 flags30;           // +0x30
    char pad38[0x80 - 0x38];
    float stat[8];                      // +0x80
    unsigned fA0;                       // +0xa0
    char padA4[0x508 - 0xa4];           // sizeof(Agent) == 0x508
    int tier_base();                    // 0x462b60
};

// the agent container: the current troop index at +0x8c, the agent array at +0x13a00
struct World {
    char pad0[0x8c];
    int troop;                          // +0x8c (current agent index)
    char pad90[0x13a00 - 0x90];
    Agent* agents;                      // +0x13a00 (stride 0x508)
    char pad13a04[0x13a38 - 0x13a04];
    int f13a38;                         // +0x13a38 (argument to ItemRef::randomize_modifier)
    int pick_random_item(int t0, int t1, int t2, int t3);   // 0x48ca30
    void spawn_items(int type, int want, int troop);   // 0x48cc40
    void init_slots(int troop_idx, int want);   // 0x48c4c0
};
extern World* g_world;                  // 0x5e2930

// the mission object (0x5e292c): the scene at +0x490, a record table at +0x668c (stride 0x1c8)
struct Scene;
struct Mission {
    char pad0[0x490];
    Scene* scene;                       // +0x490
    char pad494[0x668c - 0x494];
    char* recs;                         // +0x668c
    void set_color(unsigned rgb);       // 0x48d350
};
extern Mission* g_mission;              // 0x5e292c

// 0xa7045c is one anonymous 16-byte object (see s0048c0c0)
struct StoreGlobal {
    char pad[0x10];
};
extern StoreGlobal g_store;             // 0xa7045c

// ---- 0x48ca30: pick a random usable item kind matching one of four types --------------------------------
// A candidate's chance starts at 1 and is zeroed for low-quality candidates.
// @ 0x48ca30
int World::pick_random_item(int t0, int t1, int t2, int t3)
{
    for (;;) {
        float prob = 1.0f;
        int idx;
        if (g_item_count <= 1)
            idx = 0;
        else
            idx = rand() % g_item_count;
        ItemKind* k = &g_items[idx];
        ItemRef r;
        r.item = idx;
        r.m4 = 0;
        if (idx > 0) {
            unsigned t = k->type_flags & 0xff;
            if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts ||
                t == itp_type_pistol || t == itp_type_musket || t == itp_type_crossbow ||
                t == itp_type_bullets)
                r.m4 = 0;
            else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal)
                r.m4 = (unsigned)r.stat_1e8();
        }
        unsigned tf = k->type_flags;
        unsigned t = tf & 0xff;
        unsigned f = tf >> 12;
        if ((tf & 0x10000) != 0 && (f & 1) == 0 &&
            (t == (unsigned)t0 || t == (unsigned)t1 || t == (unsigned)t2 || t == (unsigned)t3)) {
            float v = (float)*(int*)((char*)&this->agents[this->troop] + 0xb0);
            prob = (v * 30.0f + 200.0f) / ((float)k->f1d8 + 200.0f);
            if (t == itp_type_head_armor || t == itp_type_body_armor ||
                t == itp_type_foot_armor || t == itp_type_hand_armor) {
                int a = (int)r.stat_204();
                int b = (int)r.stat_1fc();
                int c = (int)r.stat_200();
                if (c > b) {
                    if (c > a)
                        a = c;
                } else if (b > a) {
                    a = b;
                }
                if (a <= 4)
                    prob = 0.0f;
            } else {
                unsigned u = k->type_flags & 0xff;
                if (u == itp_type_one_handed_wpn || u == itp_type_two_handed_wpn ||
                    u == itp_type_polearm) {
                    int a = (int)r.stat_21x(1);
                    int b = (int)r.stat_21x(0);
                    if (b <= a)
                        b = a;
                    if (b <= 0xa)
                        prob = 0.0f;
                }
            }
        } else {
            idx = -1;
        }
        float roll = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        if (roll >= prob || idx == -1)
            continue;
        return idx;
    }
}

// ---- 0x48cc40: spawn a number of random items of one type into an agent's slots -------------------------
// Args are (type, want, troop); `ret 0xc` — the agent index is the third stack argument.
// @ 0x48cc40
void World::spawn_items(int type, int want, int troop)
{
    Agent* ag = &this->agents[troop];
    int got = 0;
    for (int tries = 0; tries < 10000 && got < want; tries++) {
        int idx;
        if (g_item_count <= 1)
            idx = 0;
        else
            idx = rand() % g_item_count;
        ItemKind* k = &g_items[idx];
        unsigned tf = k->type_flags;
        if ((tf & 0xff) != (unsigned)type || (tf & 0x1000) != 0 || (tf & 0x10000) == 0)
            continue;
        float chance = (float)k->f220 * k->f224 * 0.01f;
        if (chance < 0.0f)
            chance = 0.0f;
        else if (chance >= 20.0f)
            chance = 20.0f;
        float roll = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        for (; chance > roll && got < want; got++) {
            chance -= 1.0f;
            int n = ag->tier_base();
            int slot = n > 1 ? rand() % n : 0;
            ItemRef* e = &((ItemRef*)((char*)ag + 0xb4))[slot];
            if (e->item == -1 || (g_items[e->item].type_flags & 0x1000) == 0) {
                e->item = idx;
                e->m4 = 0;
                if (idx > 0) {
                    unsigned t = k->type_flags & 0xff;
                    if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts ||
                        t == itp_type_pistol || t == itp_type_musket || t == itp_type_crossbow ||
                        t == itp_type_bullets) {
                        e->b.amount = 0;
                    } else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal) {
                        e->b.amount = 0;
                        e->m4 |= (unsigned)e->stat_1e8();
                    }
                }
                e->randomize_modifier(this->f13a38);
            }
        }
    }
    ((SlotList*)((char*)ag + 0xb4))->finish(ag->tier_base());
}

// ---- 0x48ce90: copy n 8-byte item references, returning the end of the destination ----------------------
// The original copies the same source reference n times (the source pointer is not advanced), computes the
// result as first + n * 8, guards with n > 0 (unsigned) and runs a do/while over a copy of n.
// @ 0x48ce90
ItemRef* __stdcall copy_refs(ItemRef* dst, unsigned n, const ItemRef* src)
{
    ItemRef* p = dst;
    if (n > 0) {
        unsigned i = n;
        do {
            *p = *src;
            p++;
        } while (--i);
    }
    return dst + n;
}

// a mission record's vector at +0x108 (begin/end at +0x10c/+0x110 and +0x11c/+0x120)
struct RecVec {
    void** b10c;                        // +0x108 (int* [2])
};

// ---- 0x48cec0: fastcall lookup into the current mission's records (keys are negative ids) ---------------
// @ 0x48cec0
int __fastcall lookup_a(int key)
{
    int r = 0;
    if (key == -0x28 || key == -0x2b || key == -0x2a) {
        int i = *(int*)((char*)g_world + 0x1a1f0);
        if (i >= 0) {
            char* rec = *(char**)((char*)g_mission + 0x668c) + i * 0x1c8;
            rec = *(char**)(rec + 0x1b4);
            int* first = *(int**)(rec + 0x10c);
            if (first != 0) {
                int n = (int)(*(int**)(rec + 0x110) - first) >> 2;
                if (n > 0)
                    r = *first;
            }
        }
        return r;
    }
    if (key == -0x32 || key == -0x33)
        return *(int*)((char*)g_world + 0x1a1fc);
    return r;
}

// ---- 0x48cf40: another fastcall lookup (mission record +0x11c pointer list) ------------------------------
// @ 0x48cf40
int __fastcall lookup_b(int key)
{
    int r = 0;
    if (key == -0x28) {
        int i = *(int*)((char*)g_world + 0x1a1f0);
        if (i >= 0) {
            char* rec = *(char**)((char*)g_mission + 0x668c) + i * 0x1c8;
            rec = *(char**)(rec + 0x1b4);
            int* first = *(int**)(rec + 0x11c);
            if (first != 0) {
                int n = (int)(*(int**)(rec + 0x120) - first) >> 2;
                if (n > 0)
                    r = *first;
            }
        }
        return r;
    }
    if (key == -0x32 || key == -0x46)
        return *(int*)((char*)g_world + 0x1a200);
    return r;
}

// the container whose +0x38 holds an array of begin/end list pairs and whose +0x84 is the list count
struct ListHolder {
    char pad0[0x38];
    int* lists;                         // +0x38 (begin/end pairs, stride 0x10)
    char pad3c[0x84 - 0x3c];
    int count;                          // +0x84
    void set_owner(void* owner);        // 0x48cfc0
};
struct Owned {
    char pad0[0x2c];
    Owned* owner;                       // +0x2c
    char pad30[0x64 - 0x30];
    int refs;                           // +0x64
};
// ---- 0x48cfc0: give every object in a container's lists the same owner pointer --------------------------
// @ 0x48cfc0
void ListHolder::set_owner(void* owner)
{
    int i = 0;
    if (this->count > 0) {
        char* p = (char*)this + 0x3c;
        do {
            int** it = *(int***)(p - 4);
            for (; it < *(int***)p; it++) {
                Owned* obj = *(Owned**)(it);
                if (obj->owner != 0)
                    obj->owner->refs--;
                obj->owner = (Owned*)owner;
                if (owner != 0)
                    ((Owned*)owner)->refs++;
            }
            i++;
            p += 0x10;
        } while (i < this->count);
    }
}

// the shape whose bounds are unioned in 0x48d020 (bounds at +0x22c: min x/y, max x/y)
struct Shape {
    float* vptr;                        // +0x00
    char pad4[0x22c - 4];
    float x0;                           // +0x22c
    float y0;                           // +0x230
    char pad234[0x23c - 0x234];
    float x1;                           // +0x23c
    float y1;                           // +0x240
    void compute_bounds();              // 0x419300
};

// the object holding four shapes (e.g. a ground/field container); bounds centre at +0xf8
struct FieldVt {
    virtual void s00(); virtual void s04(); virtual void s08(); virtual void s0c();
    virtual void s10(); virtual void s14(); virtual void s18(); virtual void s1c();
    virtual void s20(); virtual void s24(); virtual void s28(); virtual void s2c();
    virtual void s30(); virtual void s34(); virtual void s38(); virtual void s3c();
    virtual void s40(); virtual void s44(); virtual void s48(); virtual void s4c();
    virtual void s50(); virtual void s54(); virtual void s58(); virtual void s5c();
    virtual void s60();
    virtual void set_offset(float* c);  // +0x64
};
struct Field : FieldVt {
    char pad4[0xf8 - 4];
    float cx;                           // +0xf8
    float cy;                           // +0xfc
    char pad100[0x108 - 0x100];
    Shape* s0;                          // +0x108
    Shape* s1;                          // +0x10c
    Shape* s2;                          // +0x110
    Shape* s3;                          // +0x114
    void update();                      // 0x48d020
};

// ---- 0x48d020: union of the four shapes' bounds; if the origin lies outside, recentre --------------
// The max accumulators start at 0 and the min accumulators at 999999; the result centre is the min pair.
// @ 0x48d020
void Field::update()
{
    float x1 = 0.0f, y1 = 0.0f;
    float x0 = 999999.0f, y0 = 999999.0f;
    if (s0) {
        s0->compute_bounds();
        if (y1 < s0->y1)
            y1 = s0->y1;
        if (x1 < s0->x1)
            x1 = s0->x1;
        if (y0 > s0->y0)
            y0 = s0->y0;
        if (x0 > s0->x0)
            x0 = s0->x0;
    }
    if (s2) {
        s2->compute_bounds();
        if (y1 < s2->y1)
            y1 = s2->y1;
        if (x1 < s2->x1)
            x1 = s2->x1;
        if (y0 > s2->y0)
            y0 = s2->y0;
        if (x0 > s2->x0)
            x0 = s2->x0;
    }
    if (s1) {
        s1->compute_bounds();
        if (y1 < s1->y1)
            y1 = s1->y1;
        if (x1 < s1->x1)
            x1 = s1->x1;
        if (y0 > s1->y0)
            y0 = s1->y0;
        if (x0 > s1->x0)
            x0 = s1->x0;
    }
    if (s3) {
        s3->compute_bounds();
        if (y1 < s3->y1)
            y1 = s3->y1;
        if (x1 < s3->x1)
            x1 = s3->x1;
        if (y0 > s3->y0)
            y0 = s3->y0;
        if (x0 > s3->x0)
            x0 = s3->x0;
    }
    if (sqrt(x1 * x1 + y1 * y1) > 0.0f) {
        float c[2];
        c[0] = x1;
        c[1] = y1;
        set_offset(c);
        cx = x0;
        cy = y0;
    }
}

// The colour object (the camera / view object): a vtable whose +0x94 / +0x98 slots take a colour
// pointer, and a colour dword at +0x14c.
struct VisualVt {
    virtual void s00(); virtual void s04(); virtual void s08(); virtual void s0c();
    virtual void s10(); virtual void s14(); virtual void s18(); virtual void s1c();
    virtual void s20(); virtual void s24(); virtual void s28(); virtual void s2c();
    virtual void s30(); virtual void s34(); virtual void s38(); virtual void s3c();
    virtual void s40(); virtual void s44(); virtual void s48(); virtual void s4c();
    virtual void s50(); virtual void s54(); virtual void s58(); virtual void s5c();
    virtual void s60(); virtual void s64(); virtual void s68(); virtual void s6c();
    virtual void s70(); virtual void s74(); virtual void s78(); virtual void s7c();
    virtual void s80(); virtual void s84(); virtual void s88(); virtual void s8c();
    virtual void s90();
    virtual void get_color(unsigned* out);          // +0x94
    virtual void set_color(unsigned* c);            // +0x98
};
struct Visual : VisualVt {
    char pad4[0x14c - 4];
    unsigned color;                     // +0x14c
};
// the scene object at +0x490 of the mission; +0x17c points at a slot holding the view object
struct Scene {
    char pad0[0x17c];
    Visual** view;                      // +0x17c
};

// ---- 0x48d350: set the view object's colour, keeping the current alpha ---------------------------------
// @ 0x48d350
void Mission::set_color(unsigned rgb)
{
    Visual* v = scene->view[0];
    rgb = rgb | (v->color & 0xff000000);
    scene->view[0]->set_color(&rgb);
    unsigned out[2];
    scene->view[0]->get_color(out);
}
