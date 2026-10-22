// Slice s004832b0. Two members of the game object class (`Game`, at the 0x5e2930 global):
//
//   0x4832b0  Game::spawn(Agent*)      the shared "initialise one agent" routine. Called with the 0x5928-byte
//                                      agent record already allocated and partly filled; it applies the troop's
//                                      skin mesh, builds the collision capsule from the record's scale, runs the
//                                      equipment fillers and finishes with the corpse spawn.
//   0x4836e0  Game::create(...)        hands out an id from the embedded Pool<Agent> at Game+4 (alloc 0x478020),
//                                      resets the record (0x472f90), fills the spawn fields and calls 0x4832b0.
//
// ALL NAMES PROVISIONAL; offsets come from the two disassemblies. Flags: default /O2 /MT /Gy /TP.
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- the 0x5928-byte agent record -------------------------------------------------------------------------
struct Entity;                          // a 0x30c-byte script entity (0x428440)
struct Inv;                             // the carried-item container (auto_equip 0x4687e0)
struct ItemStack;                       // a 2-dword item reference (0x434df0 / 0x434eb0)
struct AgentSkel;                       // the 0x7c-byte per-agent animator (0x428820)

struct Agent {
    char pad0[8];
    int id;                             // +0x8
    char padc[0x10 - 0xc];
    int kind;                           // +0x10  0 human / 1 horse
    unsigned flags;                     // +0x14
    char pad18[0x50 - 0x18];
    vector4 pos;                        // +0x50  (the matrix' o row)
    vector4 dir;                        // +0x60  (the matrix' f row)
    char pad70[0x80 - 0x70];
    float scale;                        // +0x80
    char pad84[0x88 - 0x84];
    int troop;                          // +0x88
    int item_a;                         // +0x8c
    int item_b;                         // +0x90
    int f94;                            // +0x94
    char pad98[0x110 - 0x98];
    int f110;                           // +0x110
    char pad114[0x12c - 0x114];
    int f12c;                           // +0x12c
    int f130;                           // +0x130
    char pad134[0x13c - 0x134];
    char inv[0x180 - 0x13c];            // +0x13c (Inv)
    int body_a;                         // +0x180
    int body_b;                         // +0x184
    char pad188[0x194 - 0x188];
    int f194;                           // +0x194
    char pad198[0x1a4 - 0x198];
    int f1a4;                           // +0x1a4
    int f1a8;                           // +0x1a8
    char pad1ac[0x1c4 - 0x1ac];
    int f1c4;                           // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    struct Slot { int id; int pad; };
    Slot slots[10];                     // +0x1d0
    unsigned char slot_flag[10];        // +0x220
    char pad22a[0x240 - 0x22a];
    int f240;                           // +0x240
    int f244;                           // +0x244
    int f248;                           // +0x248
    float f24c;                         // +0x24c
    Entity* ent;                        // +0x250
    char pad254[0x258 - 0x254];
    int f258;                           // +0x258
    char pad25c[0x55d8 - 0x25c];
    vector4 userdata;                   // +0x55d8 (a copy of dir)
    char pad55e8[0x56f0 - 0x55e8];
    unsigned f56f0;                     // +0x56f0
    char pad56f4[0x5714 - 0x56f4];
    int f5714;                          // +0x5714
    char pad5718[0x5720 - 0x5718];
    float f5720;                        // +0x5720
    float f5724;                        // +0x5724
    char pad5728[0x5928 - 0x5728];

    void reset();                       // 0x472f90
    float equipment_value() const;      // 0x436050
    void spawn_corpse();                // 0x4554c0
};

// ---- the id pool embedded in the game object (Pool<Agent>, alloc 0x478020) ---------------------------------
template<class T> struct Pool {
    int next_id;                        // +0
    std::vector<int> free_ids;          // +4
    std::vector<T*> pages;              // +0x14
    int serial;                         // +0x24
    int alloc();                        // 0x478020 (thiscall, 16-record pages)
};

// ---- the game object ---------------------------------------------------------------------------------------
struct PartyMgr {                       // the object at Game+0xf8 (index_of 0x48ad00)
    char pad0[0x138d0];
    char* page_table;                   // +0x138d0
    int index_of(int id);               // 0x48ad00 (thiscall, ret 4)
};
struct TroopRec {
    char pad0[0x30];
    unsigned skin;                      // +0x30 (low nibble = skin, bit 4, bit 12)
    unsigned f34;                       // +0x34
    char pad38[0x508 - 0x38];
};
struct SndRec {                         // 0x58-byte record inside the mission sound table
    char pad0[0x4c];
    int count;                          // +0x4c
    char* slots;                        // +0x50
};
struct SndSlot {                        // a 0x78-byte sound slot at SndRec::slots
    int pad0[0x78 / 4];
    void update(int n);                 // 0x4625d0 (thiscall, ret 4)
};
struct SndBody {                        // the {count, slots} pair at SndRec+0x4c
    int num;
    SndSlot* slots;
};
struct ItemKind {                       // 0x1f8-byte entry of the item-kind table
    char pad0[0x1e8];
    int f1e8;                           // +0x1e8
    float f1ec;                         // +0x1ec
    char pad1f0[8];
};
struct Skeleton96 {                     // the 0x5ac8-byte record of the animation table
    char pad0[0x8a8];
    int count;                          // +0x8a8
    char pad8ac[4];
    int* items() { return (int*)((char*)this + 0xb0); }   // stride 0x20, low bit of the first dword
};
struct World {
    Entity* create_entity(int type);    // 0x428440 (thiscall, ret 4)
};
struct Game {
    World* world;                       // +0
    Pool<Agent> pool;                   // +4
    char pad2c[0x5c - 0x2c];
    int mission_id;                     // +0x5c
    char pad60[0x78 - 0x60];
    int sfx;                            // +0x78
    char pad7c[0x5df0 - 0x7c];
    int f5df0;                          // +0x5df0
    int f5df4;                          // +0x5df4
    char pad5df8[0x139c8 - 0x5df8];
    char* page_table;                   // +0x139c8
    char pad139cc[0x13a00 - 0x139cc];
    TroopRec* chars;                    // +0x13a00
    char pad13a04[0x14014 - 0x13a04];
    matrix frame;                       // +0x14014

    void spawn(Agent* a);                               // 0x4832b0
    void post_spawn(Agent* a);                          // 0x47a0c0
    int create(const matrix* m, const ItemStack* it, int troop);   // 0x4836e0
};

// ---- externals ---------------------------------------------------------------------------------------------
extern Game* g_game;                    // 0x5e2930
extern int g_item_count;                // 0x8ed4f88
extern ItemKind* g_item_table;          // 0x8ed4f7c
extern char* g_snd_recs;                // 0x5e2a00 (the mission sound table base)
extern int g_spawn_lo;                  // 0xa70448
extern int g_spawn_hi;                  // 0xa7044c
struct resmgr { void* get_object(const struct tstring& name); };   // 0x418cf0 (thiscall, ret 4)
extern resmgr g_resmgr;                 // 0x5d6fa0
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const char* s) throw();     // 0x42bbe0
    ~tstring() throw();                 // 0x42bc40
};
struct Inv {
    void auto_equip(int a, int b, int c, int d);   // 0x4687e0 (thiscall, ret 0x14)
};
struct ItemStack {
    unsigned a, b;
    int value() const;                  // 0x434df0 (thiscall, ret 0)
};
struct ItemStack2 {
    unsigned a, b;
    int value() const;                  // 0x434eb0 (thiscall, ret 0)
};
struct AgentSkel {
    char pad0[0x5c];
    int f5c;                            // +0x5c
    AgentSkel(int n) throw();           // 0x428820
    void set_object(int obj);           // 0x4288c0 (thiscall, ret 4)
    void set_env_count(int n);          // 0x46b9d0 (thiscall, ret 4)
};
struct Entity {
    char pad0[0xc];
    int id;                             // +0xc
    int type;                           // +0x10
    char pad14[0x98 - 0x14];
    void* shape;                        // +0x98
    char pad9c[0x304 - 0x9c];
    AgentSkel* skel;                    // +0x304
};
struct Body {
    void set_radius(float* r);          // 0x439000 (thiscall, ret 4)
};
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base();
};
struct capsule : shape_base {           // vtable 0x54a540, size 0x64
    vector4 p0, p1, v40, v50;
    float radius;                       // +0x60
    capsule(float r, const vector4* a, const vector4* b) throw();   // 0x433310 (thiscall, ret 0xc)
};
void AgentEq_pick_slots(Agent* a);      // 0x46b2d0 (__stdcall, ret 4)
void agent_equip_fill(Entity* e, int kind, int troop, int f94, void* s0, void* inv, void* f240, void* f194,
                      void* f1a4, void* f1a8, void* slots, int id);   // 0x482240 (__stdcall, ret 0x34)
void agent_build_body(Entity* e, int kind, int troop, void* inv, void* f130, void* slots, int id);  // 0x47cfe0
void snd_slot_update(void* slot, int n);                             // 0x4625d0 (thiscall, ret 4)

// @ 0x4832b0
__declspec(noinline) void Game::spawn(Agent* a)
{
    if (a->kind == 0) {
        a->f12c = 0xa76628;
        a->f110 = 7;
        TroopRec* t = &g_game->chars[a->troop];
        if (*(unsigned long long*)&t->skin & 0x10) {
            PartyMgr* pm = (PartyMgr*)((char*)g_game + 0xf8);
            int n = 1;
            do {
                int idx = pm->index_of(g_game->sfx);
                char* page = ((char**)pm->page_table)[idx >> 4];
                Skeleton96* rec = (Skeleton96*)(page + (idx & 0xf) * 0x5ac8);
                int cnt = rec->count;
                int found = 0, i = 0;
                if (cnt > 0) {
                    int* p = rec->items();
                    do {
                        if (*p & 1)
                            break;
                        found++;
                        i++;
                        p += 8;
                    } while (i < cnt);
                }
                if (n >= found)
                    break;
                n++;
            } while (1);
        }
        if (a->id != this->mission_id) {
            unsigned long long f = *(unsigned long long*)&t->skin;
            if (!(f & 0x10) || !(f & 0x1000))
                ((Inv*)t)->auto_equip(a->f94, 0x100e, (a->f56f0 >> 28) & 0xffffff01, (a->f56f0 >> 9) & 0xffffff01);
        }
    } else if (a->kind == 1) {
        a->f12c = 0xa76628;
    }
    Entity* e = this->world->create_entity(9);
    e->id = a->id;
    e->type = 0xb;
    a->ent = e;
    void* mem = operator new(0x7c);
    e->skel = mem ? new (mem) AgentSkel(2) : 0;
    e->skel->set_env_count(2);
    e->skel->f5c = 0;
    shape_base* shp;
    if (a->kind == 0) {
        unsigned skin = g_game->chars[a->troop].skin & 0xf;
        int obj = 0;
        if (skin < (unsigned)g_item_count)
            obj = g_item_table[skin].f1e8;
        e->skel->set_object(obj);
        skin = g_game->chars[a->troop].skin & 0xf;
        float h;
        if (skin < (unsigned)g_item_count)
            h = g_item_table[skin].f1ec;
        else
            h = 1.0f;
        a->scale = h;
        if ((mem = operator new(0x64)) != 0) {
            vector4 b(a->scale * 0.0f, a->scale * 0.0f, a->scale * 1.3f);
            vector4 c(a->scale * 0.0f, a->scale * 0.01f, a->scale * 2.3f);
            shp = new (mem) capsule(a->scale * 0.3f, &c, &b);
        } else
            shp = 0;
    } else if (a->kind == 1) {
        tstring name("skel_horse");
        e->skel->set_object((int)g_resmgr.get_object(name));
        name.~tstring();
        if ((mem = operator new(0x64)) != 0) {
            vector4 b(0.0f, 0.8f, 0.8f);
            vector4 c(0.0f, -0.3f, 0.7f);
            shp = new (mem) capsule(0.4f, &c, &b);
        } else
            shp = 0;
    } else {
        return;
    }
    e->shape = shp;
    AgentEq_pick_slots(a);
    agent_equip_fill(e, a->kind, a->troop, a->f94, &a->item_a, &a->inv[0], &a->f240, &a->f194, &a->f1a4,
                     &a->f1a8, &a->slots[0], a->id);
    agent_build_body(e, a->kind, a->troop, &a->inv[0], &a->f130, &a->slots[0], a->id);
    a->f24c = a->equipment_value();
    this->post_spawn(a);
    if (a->kind == 0) {
        if (a->body_a) {
            float r = 20.0f;
            ((Body*)a->body_a)->set_radius(&r);
        }
        if (a->body_b) {
            float r = 20.0f;
            ((Body*)a->body_b)->set_radius(&r);
        }
    }
    a->spawn_corpse();
}

// @ 0x4836e0
int Game::create(const matrix* m, const ItemStack* it, int troop)
{
    int own = pool.alloc();
    Agent* a = &pool.pages[(unsigned)own >> 4][own & 0xf];
    a->reset();
    a->item_a = it->a;
    a->item_b = it->b;
    a->troop = troop;
    a->pos = m->o;
    a->f1c4 = -1;
    a->kind = 1;
    a->f258 = 0;
    a->dir = m->f;
    a->userdata = a->dir;
    a->flags |= 1;
    this->spawn(a);
    for (int i = 0; i < 10; i++) {
        a->slots[i].id = -1;
        a->slot_flag[i] = 1;
    }
    int v = ((ItemStack*)((char*)a + 0x8c))->value();
    a->f5720 = (float)v;
    a->f5724 = (float)v;
    a->f240 = ((ItemStack2*)it)->value();
    a->f244 = ((ItemStack2*)it)->value();
    a->f248 = ((ItemStack2*)it)->value();
    *(long long*)&g_spawn_lo = own;
    SndBody* p = (SndBody*)(g_snd_recs + this->f5df0 * 0x58 + 0x4c);
    for (int i = 0; i < p->num; i++)
        p->slots[i].update(-25);
    return own;
}
