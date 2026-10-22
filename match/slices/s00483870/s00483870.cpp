// Slice s00483870. Three members of the mission object (the object 0x483870/0x483de0/0x483eb0 are called on;
// the same object slice s004832b0 calls `Game`, reached as [0x5e2930] + 0x14014):
//
//   0x483870  Game::spawn_full(...)   a 10-argument agent creator: allocates a record from the embedded
//                                     Pool<Agent>, resets it, fills the spawn fields from a matrix and a
//                                     0x4c-byte wave record, applies the skin-based scale/height, runs
//                                     Game::spawn, then optionally creates a carried-item agent through
//                                     pick_slot_item + Game::create and announces the wave on the ui queue.
//   0x483de0  Game::spawn_one(int)    reads a wave record through 0x473a60 and forwards it to spawn_full,
//                                     then stores the record index on the created agent (+0x5718).
//   0x483eb0  Game::run_spawns()      up to 100 passes over the current wave set, spawning every pending
//                                     entry whose per-entry flag at Game+0x6824 is clear.
//
// ALL NAMES PROVISIONAL; offsets come from the three disassemblies. Flags: default /O2 /MT /Gy /TP.
#include <new>
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- the 0x5928-byte agent record -------------------------------------------------------------------------
struct Entity;                          // a 0x30c-byte script entity (0x428440)
struct ItemStack;                       // a 2-dword item reference (0x434df0 / 0x434eb0)

struct Agent {
    char pad0[8];
    int id;                             // +0x8
    char padc[4];
    int kind;                           // +0x10
    int f14;                            // +0x14
    unsigned flags;                     // +0x18
    char pad1c[0x2c - 0x1c];
    int f2c;                            // +0x2c
    int f30;                            // +0x30
    char pad34[0x50 - 0x34];
    vector4 pos;                        // +0x50
    vector4 dir;                        // +0x60
    char pad70[0x7c - 0x70];
    int f7c;                            // +0x7c
    float scale;                        // +0x80
    char pad84[0x88 - 0x84];
    int troop;                          // +0x88
    int f8c;                            // +0x8c
    int f90;                            // +0x90
    int f94;                            // +0x94
    char pad98[0x110 - 0x98];
    int f110;                           // +0x110
    char pad114[0x1bc - 0x114];
    int f1bc;                           // +0x1bc
    int f1c0;                           // +0x1c0
    int f1c4;                           // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    struct Slot { int id; int pad; };
    Slot slots[10];                     // +0x1d0
    unsigned char slot_flag[10];        // +0x220
    char pad22a[0x258 - 0x22a];
    int f258;                           // +0x258
    char pad25c[0x2c4 - 0x25c];
    int f2c4;                           // +0x2c4
    int f2c8;                           // +0x2c8
    char pad2cc[0x55d8 - 0x2cc];
    vector4 userdata;                   // +0x55d8 (a copy of dir)
    char pad55e8[0x56f0 - 0x55e8];
    unsigned f56f0;                     // +0x56f0
    int f56f4[8];                       // +0x56f4
    int f5714;                          // +0x5714
    int f5718;                          // +0x5718
    int f571c;                          // +0x571c
    float f5720;                        // +0x5720
    float f5724;                        // +0x5724
    char pad5728[0x5928 - 0x5728];

    void reset();                       // 0x472f90
    void cycle_slot(int i);             // 0x44bc30
    void on_wield_reset();              // 0x445a20
    bool check_new_enemies(Agent* a);   // 0x432350 (ecx = commander record)
    bool check_reinforcements(Agent* a);   // 0x4323a0 (ecx = commander record)
};

// ---- the id pool embedded in the mission object (Pool<Agent>, alloc 0x478020) ------------------------------
template<class T> struct Pool {
    int next_id;                        // +0
    std::vector<int> free_ids;          // +4
    std::vector<T*> pages;              // (begin at +0x18)
    int serial;                         // +0x24
    int alloc();                        // 0x478020
};

// ---- the two 0x58-byte tables addressed through [0x5e2a00] -------------------------------------------------
struct SndSlot {                        // a 0x78-byte sound slot
    int pad0[0x78 / 4];
    void update(int n);                 // 0x4625d0 (thiscall, ret 4)
};
struct SpawnRec {                       // a 0x4c-byte wave entry
    int f0;                             // +0
    int f4;                             // +4
    int f8;                             // +8
    int fc;                             // +0xc
    int f10, f14;
    int cur;                            // +0x18
    int next;                           // +0x1c
    int f20;                            // +0x20
    int f24;                            // +0x24
    int arr[9];                         // +0x28
};
struct SpawnSet {                       // the 0x58-byte per-set record
    char pad0[0x34];
    SpawnRec* recs;                     // +0x34
    int count;                          // +0x38
    char pad3c[0x4c - 0x3c];
    int num;                            // +0x4c
    SndSlot* slots;                     // +0x50
};

// ---- the object at Game+0xf8 (PartyMgr, index_of 0x48ad00) -------------------------------------------------
struct Skeleton96 {                     // the 0x5ac8-byte character record of the page table
    char pad0[0x80];
    float f80;                          // +0x80
    char pad84[0x5ac8 - 0x84];
};
struct PartyMgr {
    char pad0[0x138d0];
    char** page_table;                  // +0x138d0
    int index_of(int id);               // 0x48ad00 (thiscall, ret 4)
};

// ---- the per-troop 0x508-byte record at Game+0x13a00 ------------------------------------------------------
struct TroopRec {
    char pad0[0x30];
    unsigned skin;                      // +0x30
    unsigned f34;                       // +0x34
    char pad38[0x48 - 0x38];
    int f48;                            // +0x48
    char pad4c[0x58 - 0x4c];
    int f58;                            // +0x58
    char pad5c[0x7a - 0x5c];
    unsigned char f7a;                  // +0x7a
    char pad7b[0xa0 - 0x7b];
    int fa0;                            // +0xa0
    char pada4[0x508 - 0xa4];
};

// ---- the mission object -----------------------------------------------------------------------------------
struct World { Entity* create_entity(int type); };   // 0x428440
struct Game {
    World* world;                       // +0
    Pool<Agent> pool;                   // +4
    char pad2c[0x5c - 0x2c];
    int mission_id;                     // +0x5c
    int f60;                            // +0x60
    char pad64[0x78 - 0x64];
    int sfx;                            // +0x78
    char pad7c[0x8c - 0x7c];
    int f8c;                            // +0x8c
    char pad90[0x5df0 - 0x90];
    int f5df0;                          // +0x5df0
    int f5df4;                          // +0x5df4
    char pad5df8[0x6528 - 0x5df8];
    char f6528;                         // +0x6528
    char pad6529[0x6598 - 0x6529];
    vector4 f6598;                      // +0x6598
    char pad65a8[0x6668 - 0x65a8];
    char f6668;                         // +0x6668
    char f6669;                         // +0x6669
    char pad666a[0x6824 - 0x666a];
    char f6824[0x100];                  // +0x6824
    char pad6924[0x139c8 - 0x6924];
    char* page_table;                   // +0x139c8
    char pad139cc[0x13a00 - 0x139cc];
    TroopRec* chars;                    // +0x13a00
    char pad13a04[0x14014 - 0x13a04];
    matrix frame;                       // +0x14014

    void spawn(Agent* a);                               // 0x4832b0
    int create(const matrix* m, const ItemStack* it, int troop);   // 0x4836e0
    __declspec(noinline) bool spawn_one(int id);        // 0x483de0
    void run_spawns();                                  // 0x483eb0
    __declspec(noinline) int spawn_full(const matrix* m, int troop, int a, int b, int c, int d, int e, int f, int cnt, int* arr);  // 0x483870
    void wave_info(int idx, int* a, int* b, int* c, int* d, int* e, int* f);   // 0x473a60
};

// ---- externals ---------------------------------------------------------------------------------------------
extern Game* g_game;                    // 0x5e2930
extern char* g_snd_recs;                // 0x5e2a00 (the wave/sound set table base)
extern int g_skin_cap;                  // 0x8ee3ac4
extern int g_spawn_lo;                  // 0xa70448
extern int g_spawn_hi;                  // 0xa7044c
extern char g_msg_enabled;              // 0x5d8794
extern int g_msg_count;                 // 0x5e2138
extern int g_msg_base;                  // 0x5e213c
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const char* s) throw();     // 0x42bbe0
    tstring& operator=(const tstring& o);   // 0x42bc50
    ~tstring() throw();                 // 0x42bc40
};
extern tstring g_msg_text[32];          // 0x5e2140
extern int g_msg_id[32];                // 0x5e2340
struct StrTable { tstring* lookup(const tstring& key); };   // 0x43a1e0
extern StrTable g_strtable;             // 0x5e2a18
tstring* t2;                            // reused temporary for the strtable lookup
struct ItemStack { unsigned a, b; };
long long pick_slot_item(int a, int b, int c, int d, int* e, char f, char* g);   // 0x46b190
void __stdcall f472160(int v);          // 0x472160

// @ 0x483870
__declspec(noinline) int Game::spawn_full(const matrix* m, int troop, int a1, int a2, int a3, int a4, int a5, int a6, int cnt, int* arr)
{
    int own = pool.alloc();
    if (troop == g_game->f8c) {
        this->mission_id = own;
        this->f6598 = m->o;
    }
    Agent* a = &pool.pages[own >> 4][own & 0xf];
    a->reset();
    a->f1bc = a1;
    a->f94 = a4;
    a->f56f0 = a5;
    a->f1c0 = a2;
    a->troop = troop;
    a->f8c = -1;
    a->f5714 = cnt;
    for (int i = 0; i < cnt; i++)
        a->f56f4[i] = arr[i];
    a->f1c4 = a3;
    a->pos = m->o;
    a->pos.z = a->pos.z + 0.3f;
    a->dir = m->f;
    a->userdata = a->dir;

    TroopRec* t = &g_game->chars[troop];
    int v = t->f7a & 0xf;
    if (v < 0) v = 0;
    if (v > g_skin_cap) v = g_skin_cap;
    float h = (float)(t->f58 + 0x23 + v * 2);
    a->f5720 = h;
    if (t->skin & 0x10) {
        a->f5724 = t->fa0;
    } else {
        a->f5724 = h;
        if (a1 == g_game->sfx) {
            PartyMgr* pm = (PartyMgr*)((char*)g_game + 0xf8);
            Skeleton96* rec = (Skeleton96*)(pm->page_table[pm->index_of(a1) >> 4]);
            int idx = pm->index_of(a1) & 0xf;
            if (rec[idx].f80 > 0.5f) {
                a->f5724 = (1.5f - rec[idx].f80) * h;
            }
        }
    }
    a->f571c = a->f5724;
    a->kind = t->f48;
    a->f14 = 1;
    a->f258 = a6;
    a->f2c4 = 0;
    if (a6 & 0x10)
        a->f2c4 = 1;
    a->f56f0 = a5;
    if (a->id == this->mission_id) {
        a->flags |= 4;
        a->f7c = 2;
    }
    a->f14 = 1;
    for (int i = 0; i < 10; i++) {
        a->slots[i].id = -1;
        a->slot_flag[i] = 1;
    }
    this->spawn(a);
    a->f2c8 = a6;
    long long item = pick_slot_item(troop, 8, a6, cnt, arr, 0, (char*)&a6);
    if ((int)item >= 0) {
        int nid = this->create(m, (const ItemStack*)&item, troop);
        Agent* na = &pool.pages[(unsigned)nid >> 4][nid & 0xf];
        na->f30 = own;
        a->f2c = nid;
        na->f1bc = a1;
        na->f1c0 = a2;
        if (troop == g_game->f8c)
            this->f60 = nid;
    }
    if (a->id == this->mission_id) {
        if ((this->f6528 & 1) || ((*(unsigned*)(g_snd_recs + this->f5df0 * 0x58 + 0x20) >> 1) & 1) != 0) {
            a->cycle_slot(0);
            a->cycle_slot(1);
        }
    }
    a->on_wield_reset();
    int s = a->f1c4;
    int k = (s < 0 || s >= 8) ? 7 : s;
    if (*(int*)((char*)this + 0x5280 + k * 0x16c) < 0 || a->id == this->mission_id) {
        *(int*)((char*)this + 0x5280 + k * 0x16c) = a->id;
    }
    if (this->mission_id >= 0) {
        const char* lit = 0;
        if (this->f6669 != 0) {
            Agent* cmdr = &pool.pages[(unsigned)this->mission_id >> 4][this->mission_id & 0xf];
            if (cmdr->check_new_enemies(a)) {
                this->f6669 = 0;
                lit = "ui_new_enemies_have_arrived";
            }
        }
        if (lit == 0 && this->f6668 != 0) {
            Agent* cmdr = &pool.pages[(unsigned)this->mission_id >> 4][this->mission_id & 0xf];
            if (cmdr->check_reinforcements(a)) {
                this->f6668 = 0;
                lit = "ui_reinforcements_have_arrived";
            }
        }
        if (lit != 0) {
            tstring msg(lit);
            t2 = g_strtable.lookup(msg);
            if (g_msg_enabled && g_msg_count < 0x20) {
                int k3 = (g_msg_base + g_msg_count) % 0x20;
                g_msg_text[k3] = *t2;
                g_msg_id[k3] = -1;
                g_msg_count++;
            }
            msg.~tstring();
        }
    }
    *(long long*)&g_spawn_lo = own;
    SpawnSet* p = (SpawnSet*)(g_snd_recs + this->f5df0 * 0x58);
    for (int i = 0; i < p->num; i++)
        p->slots[i].update(-25);
    return own;
}

// @ 0x483de0
bool Game::spawn_one(int id)
{
    int o7, o5, o3, o2, o6;
    int orig = id;
    wave_info(id, &o2, &o3, &id, &o5, &o6, &o7);
    if (id < 0)
        return false;
    SpawnRec* r = (SpawnRec*)(*(int*)(g_snd_recs + this->f5df0 * 0x58 + 0x34) + orig * 0x4c);
    int nid = spawn_full((matrix*)((char*)this + r->f0 * 0x130 + 0x6b0), id, o2, o3, o5, o7,
                         r->f8, r->f20, r->f24, r->arr);
    if (nid >= 0)
        pool.pages[(unsigned)nid >> 4][nid & 0xf].f5718 = orig;
    return true;
}

// @ 0x483eb0
void Game::run_spawns()
{
    SpawnSet* p = (SpawnSet*)(g_snd_recs + this->f5df0 * 0x58);
    char done = 0;
    int iter = 0;
    do {
        if (done)
            break;
        done = 1;
        for (int i = 0; i < *(int*)(g_snd_recs + this->f5df0 * 0x58 + 0x38); i++) {
            SpawnRec* r = (SpawnRec*)(*(int*)(g_snd_recs + this->f5df0 * 0x58 + 0x34) + i * 0x4c);
            if (r->cur > 0 && this->f6824[i] == 0 && this->spawn_one(i)) {
                r->cur--;
                r->next++;
                f472160(r->f0);
                done = 0;
            }
        }
        iter++;
    } while (iter < 100);
}
