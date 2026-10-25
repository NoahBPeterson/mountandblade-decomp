// mesh_game.cpp: TU 63 "mesh", part 7 (0x4832b0..0x484590): the mission's agent spawner (spawn, create, spawn_full,
// spawn_one, run_spawns) and the per-frame mission tick. The object is the Mission of src/game/mission.h. See MIGRATION.md.
#include "./mesh_slot.h"
#include "../terrain_b/mission.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

#define MAT(T, off) (*(T*)((char*)this + (off)))

// ---- the 0x5928-byte agent record as the spawner fills it --------------------------------------------------------------
// (Agent in src/game/agent_obj.h and MAgent in terrain_b/agent_rec.h name the same bytes; neither declares the spawn
// words 0x94, 0x13c.., 0x56f0.. that this code writes, so the record is spelled out here in the order of the offsets.)
struct AgentS {
    char pad0[4];
    int used;                           // +0x04 (0 = slot free)
    int id;                             // +0x08
    char padc[4];
    int kind;                           // +0x10 0 human / 1 horse
    int f14;                            // +0x14 state
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
    char pad1ac[0x1bc - 0x1ac];
    int f1bc;                           // +0x1bc
    int f1c0;                           // +0x1c0
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
    void* ent;                          // +0x250
    char pad254[0x258 - 0x254];
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
    float equipment_value() const;      // 0x436050
    void spawn_corpse();                // 0x4554c0
    void cycle_slot(int i);             // 0x44bc30
    void on_wield_reset();              // 0x445a20
    bool is_enemy(AgentS* a);  // 0x432350 (ecx = commander record)
    bool is_friend(AgentS* a);   // 0x4323a0 (ecx = commander record)
    void update(float dt);              // 0x45f660
    void tick();                        // 0x45ff30
};
typedef char agents_size_check[(sizeof(AgentS) == 0x5928) ? 1 : -1];

// ---- the id pool embedded in the mission object (terrain_b/pool.h's Pool, with this record) -------------------------------
template<class T> struct PoolS {
    int next_id;                        // +0
    std::vector<int> free_ids;          // +4
    std::vector<T*> pages;              // +0x14
    int serial;                         // +0x24
    int alloc();                        // 0x478020 (thiscall, 16-record pages)
};

// ---- the objects behind the game pointer ---------------------------------------------------------------------------
struct PartyMgrS {                      // the object at g_game+0xf8 (index_of 0x48ad00)
    char pad0[0x138d0];
    char** page_table;                  // +0x138d0
    int index_of(int id);               // 0x48ad00 (thiscall, ret 4)
};
struct Skeleton96 {                     // the 0x5ac8-byte character record of the party page table
    char pad0[0x80];
    float f80;                          // +0x80
    char pad84[0x8a8 - 0x84];
    int count;                          // +0x8a8
    char pad8ac[4];
    int* items() { return (int*)((char*)this + 0xb0); }   // stride 0x20, low bit of the first dword
    char padb4[0x5ac8 - 0x8b4];
};
struct TroopRecS {                      // the 0x508-byte troop record (g_game+0x13a00)
    char pad0[0x30];
    unsigned skin;                      // +0x30 (low nibble = skin, bit 4, bit 12)
    unsigned f34;                       // +0x34
    char pad38[0x48 - 0x38];
    int f48;                            // +0x48
    char pad4c[0x58 - 0x4c];
    int f58;                            // +0x58
    char pad5c[0x7a - 0x5c];
    unsigned char f7a;                  // +0x7a
    char pad7b[0xa0 - 0x7b];
    int fa0;                            // +0xa0
    char pad_a4[0x508 - 0xa4];
    void auto_equip_dummy();
};
struct SndSlotS {
    int pad0[0x78 / 4];
    void update(int n);                 // 0x4625d0 (thiscall, ret 4)
};
struct SpawnRecG {                      // a 0x4c-byte wave entry
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
struct SpawnSetG {                      // the 0x58-byte per-set record at g_mission_templates
    char pad0[0x34];
    SpawnRecG* recs;                    // +0x34
    int count;                          // +0x38
    char pad3c[0x4c - 0x3c];
    int num;                            // +0x4c
    SndSlotS* slots;                    // +0x50
};
struct ItemKindT {                      // the item-kind table entry the spawner reads (g_skins_begin at 0x8ed4f7c, 0x1f8 bytes)
    char pad0[0x1e8];
    int f1e8;                           // +0x1e8
    float f1ec;                         // +0x1ec
    char pad1f0[8];
};
struct Inv {
    void auto_equip(int a, int b, int c, int d);    // 0x4687e0 (thiscall, ret 0x14)
};
struct ItemStackG {
    unsigned a, b;
    int value() const;                  // 0x434df0
};
struct ItemStack2 {
    unsigned a, b;
    int value() const;                  // 0x434eb0
};
struct AgentSkel {
    char pad0[0x5c];
    int f5c;                            // +0x5c
    AgentSkel(int n) throw();           // 0x428820 (FxDef::FxDef)
    void init(int obj);           // 0x4288c0
    void set_env_count(int n);          // 0x46b9d0
};
struct EntityG {                        // the 0x30c-byte script entity (rgl::strategic_entity)
    char pad0[0xc];
    int id;                             // +0xc
    int type;                           // +0x10
    char pad14[0x98 - 0x14];
    void* shape;                        // +0x98
    char pad9c[0x304 - 0x9c];
    AgentSkel* skel;                    // +0x304
};
struct Body {
    void cap_all(float* r);          // 0x439000
};
void AgentEq_pick_slots(AgentS* a);      // 0x46b2d0 (__stdcall, ret 4)
void agent_equip_fill(EntityG* e, int kind, int troop, int f94, void* s0, void* inv, void* f240, void* f194,
                      void* f1a4, void* f1a8, void* slots, int id);   // 0x482240 (__stdcall, ret 0x34)
void build_equipment(EntityG* e, int kind, int troop, void* inv, void* f130, void* slots, int id);  // 0x47cfe0
extern char* g_mission_templates;                // 0x5e2a00 (the mission mode table base)
extern char g_script[]; // 0xa70440
extern int g_spawn_hi;                  // 0xa7044c
extern int g_skin_count;                // g_char_count 0x8ed4f88
extern ItemKindT* g_skins_begin;         // 0x8ed4f7c
#include "../skill_cap.h"
extern char g_gui[]; // 0x5d7080
extern int g_msg_count;                 // 0x5e2138
extern int g_msg_base;                  // 0x5e213c
extern tstring g_msg_text[32];          // 0x5e2140
extern int g_msg_id[32];                // 0x5e2340
extern tstring* t2;                     // reused temporary for the strtable lookup
struct StrTableG {
    tstring* lookup(const tstring& key);    // 0x43a1e0
};
extern StrTableG g_strtable;            // 0x5e2a18
long long pick_slot_item(int a, int b, int c, int d, int* e, char f, char* g);   // 0x46b190
void __stdcall replace_mark(int v);          // 0x472160

// the mission object as the spawner sees it
struct MissionG : Mission {
    void spawn(AgentS* a);                                  // 0x4832b0
    int create(const matrix* m, const ItemStackG* it, int troop);   // 0x4836e0
    NOINL bool spawn_one(int id);                           // 0x483de0
    void run_spawns();                                      // 0x483eb0
    NOINL int spawn_full(const matrix* m, int troop, int a, int b, int c, int d, int e, int f, int cnt, int* arr);  // 0x483870
    void pick_spawn(int idx, int* a, int* b, int* c, int* d, int* e, int* f);   // 0x473a60
    void frame(float dt, bool spawn);                       // 0x483f60
    void post_spawn(AgentS* a);                           // 0x47a0c0
};
#define POOL (*(PoolS<AgentS>*)&MAT(char, 4))

// @ 0x4832b0 -- the shared "initialise one agent" routine: applies the troop's skin mesh, builds the collision capsule from
// the record's scale, runs the equipment fillers and finishes with the corpse spawn
NOINL void MissionG::spawn(AgentS* a)
{
    if (a->kind == 0) {
        a->f12c = 0xa76628;
        a->f110 = 7;
        TroopRecS* t = &(*(TroopRecS**)(((char*)g_game) + 0x13a00))[a->troop];
        if (*(unsigned long long*)&t->skin & 0x10) {
            PartyMgrS* pm = (PartyMgrS*)(((char*)g_game) + 0xf8);
            int n = 1;
            do {
                int idx = pm->index_of(*(int*)(((char*)g_game) + 0x78));
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
        if (a->id != this->f5c) {
            unsigned long long f = *(unsigned long long*)&t->skin;
            if (!(f & 0x10) || !(f & 0x1000))
                ((Inv*)t)->auto_equip(a->f94, 0x100e, (a->f56f0 >> 28) & 0xffffff01, (a->f56f0 >> 9) & 0xffffff01);
        }
    } else if (a->kind == 1) {
        a->f12c = 0xa76628;
    }
    EntityG* e = (EntityG*)((world_x*)scene_world)->create_entity(9);
    e->id = a->id;
    e->type = 0xb;
    a->ent = e;
    void* mem = operator new(0x7c);
    e->skel = mem ? new (mem) AgentSkel(2) : 0;
    e->skel->set_env_count(2);
    e->skel->f5c = 0;
    shape_base* shp;
    if (a->kind == 0) {
        unsigned skin_i = (*(TroopRecS**)(((char*)g_game) + 0x13a00))[a->troop].skin & 0xf;
        int obj = 0;
        if (skin_i < (unsigned)g_skin_count)
            obj = g_skins_begin[skin_i].f1e8;
        e->skel->init(obj);
        skin_i = (*(TroopRecS**)(((char*)g_game) + 0x13a00))[a->troop].skin & 0xf;
        float h;
        if (skin_i < (unsigned)g_skin_count)
            h = g_skins_begin[skin_i].f1ec;
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
        e->skel->init((int)g_resmgr.get_object70(*(rgl::string*)&name));
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
    build_equipment(e, a->kind, a->troop, &a->inv[0], &a->f130, &a->slots[0], a->id);
    a->f24c = a->equipment_value();
    this->post_spawn(a);
    if (a->kind == 0) {
        if (a->body_a) {
            float r = 20.0f;
            ((Body*)a->body_a)->cap_all(&r);
        }
        if (a->body_b) {
            float r = 20.0f;
            ((Body*)a->body_b)->cap_all(&r);
        }
    }
    a->spawn_corpse();
}

// @ 0x4836e0 -- hand out an id from the embedded pool, reset the record, fill the spawn fields and spawn it
int MissionG::create(const matrix* m, const ItemStackG* it, int troop)
{
    int own = POOL.alloc();
    AgentS* a = &POOL.pages[(unsigned)own >> 4][own & 0xf];
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
    a->f14 |= 1;
    this->spawn(a);
    for (int i = 0; i < 10; i++) {
        a->slots[i].id = -1;
        a->slot_flag[i] = 1;
    }
    int v = ((ItemStackG*)((char*)a + 0x8c))->value();
    a->f5720 = (float)v;
    a->f5724 = (float)v;
    a->f240 = ((ItemStack2*)it)->value();
    a->f244 = ((ItemStack2*)it)->value();
    a->f248 = ((ItemStack2*)it)->value();
    *(long long*)&(*(int*)((char*)&g_script + 0x8)) = own;
    struct SndBody { int num; SndSlotS* slots; };
    SndBody* p = (SndBody*)(g_mission_templates + this->mode * 0x58 + 0x4c);
    for (int i = 0; i < p->num; i++)
        p->slots[i].update(-25);
    return own;
}

// @ 0x483870 -- a 10-argument agent creator: allocates a record from the pool, resets it, fills the spawn fields from a matrix
// and a wave record, applies the skin-based scale/height, runs spawn, then optionally creates a carried-item agent through
// pick_slot_item + create and announces the wave on the ui queue
NOINL int MissionG::spawn_full(const matrix* m, int troop, int a1, int a2, int a3, int a4, int a5, int a6, int cnt, int* arr)
{
    int own = POOL.alloc();
    if (troop == *(int*)(((char*)g_game) + 0x8c)) {
        this->f5c = own;
        this->f6598 = m->o;
    }
    AgentS* a = &POOL.pages[own >> 4][own & 0xf];
    a->reset();
    a->f1bc = a1;
    a->f94 = a4;
    a->f56f0 = a5;
    a->f1c0 = a2;
    a->troop = troop;
    a->item_a = -1;
    a->f5714 = cnt;
    for (int i = 0; i < cnt; i++)
        a->f56f4[i] = arr[i];
    a->f1c4 = a3;
    a->pos = m->o;
    a->pos.z = a->pos.z + 0.3f;
    a->dir = m->f;
    a->userdata = a->dir;

    TroopRecS* t = &(*(TroopRecS**)(((char*)g_game) + 0x13a00))[troop];
    int v = t->f7a & 0xf;
    if (v < 0) v = 0;
    if (v > g_skill_tbl[36].cap) v = g_skill_tbl[36].cap;
    float h = (float)(t->f58 + 0x23 + v * 2);
    a->f5720 = h;
    if (t->skin & 0x10) {
        a->f5724 = t->fa0;
    } else {
        a->f5724 = h;
        if (a1 == *(int*)(((char*)g_game) + 0x78)) {
            PartyMgrS* pm = (PartyMgrS*)(((char*)g_game) + 0xf8);
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
    if (a->id == this->f5c) {
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
        int nid = this->create(m, (const ItemStackG*)&item, troop);
        AgentS* na = &POOL.pages[(unsigned)nid >> 4][nid & 0xf];
        na->f30 = own;
        a->f2c = nid;
        na->f1bc = a1;
        na->f1c0 = a2;
        if (troop == *(int*)(((char*)g_game) + 0x8c))
            this->f60 = nid;
    }
    if (a->id == this->f5c) {
        if ((MAT(char, 0x6528) & 1) || ((*(unsigned*)(g_mission_templates + this->mode * 0x58 + 0x20) >> 1) & 1) != 0) {
            a->cycle_slot(0);
            a->cycle_slot(1);
        }
    }
    a->on_wield_reset();
    int s = a->f1c4;
    int k = (s < 0 || s >= 8) ? 7 : s;
    if (*(int*)((char*)this + 0x5280 + k * 0x16c) < 0 || a->id == this->f5c) {
        *(int*)((char*)this + 0x5280 + k * 0x16c) = a->id;
    }
    if (this->f5c >= 0) {
        const char* lit = 0;
        if (MAT(char, 0x6669) != 0) {
            AgentS* cmdr = &POOL.pages[(unsigned)this->f5c >> 4][this->f5c & 0xf];
            if (cmdr->is_enemy(a)) {
                MAT(char, 0x6669) = 0;
                lit = "ui_new_enemies_have_arrived";
            }
        }
        if (lit == 0 && MAT(char, 0x6668) != 0) {
            AgentS* cmdr = &POOL.pages[(unsigned)this->f5c >> 4][this->f5c & 0xf];
            if (cmdr->is_friend(a)) {
                MAT(char, 0x6668) = 0;
                lit = "ui_reinforcements_have_arrived";
            }
        }
        if (lit != 0) {
            tstring msg(lit);
            t2 = g_strtable.lookup(msg);
            if ((*(char*)((char*)&g_gui + 0x1714)) && g_msg_count < 0x20) {
                int k3 = (g_msg_base + g_msg_count) % 0x20;
                g_msg_text[k3] = *t2;
                g_msg_id[k3] = -1;
                g_msg_count++;
            }
            msg.~tstring();
        }
    }
    *(long long*)&(*(int*)((char*)&g_script + 0x8)) = own;
    SpawnSetG* p = (SpawnSetG*)(g_mission_templates + this->mode * 0x58);
    for (int i = 0; i < p->num; i++)
        p->slots[i].update(-25);
    return own;
}

// @ 0x483de0 -- read a wave record through 0x473a60 and forward it to spawn_full, then store the record index on the agent
NOINL bool MissionG::spawn_one(int id)
{
    int o7, o5, o3, o2, o6;
    int orig = id;
    pick_spawn(id, &o2, &o3, &id, &o5, &o6, &o7);
    if (id < 0)
        return false;
    SpawnRecG* r = (SpawnRecG*)(*(int*)(g_mission_templates + this->mode * 0x58 + 0x34) + orig * 0x4c);
    int nid = spawn_full((matrix*)((char*)this + r->f0 * 0x130 + 0x6b0), id, o2, o3, o5, o7,
                         r->f8, r->f20, r->f24, r->arr);
    if (nid >= 0)
        POOL.pages[(unsigned)nid >> 4][nid & 0xf].f5718 = orig;
    return true;
}

// @ 0x483eb0 -- up to 100 passes over the current wave set, spawning every pending entry whose done flag is clear
void MissionG::run_spawns()
{
    SpawnSetG* p = (SpawnSetG*)(g_mission_templates + this->mode * 0x58);
    char done = 0;
    int iter = 0;
    do {
        if (done)
            break;
        done = 1;
        for (int i = 0; i < p->count; i++) {
            SpawnRecG* r = &p->recs[i];
            if (r->cur > 0 && this->done[i] == 0 && this->spawn_one(i)) {
                r->cur--;
                r->next++;
                ::replace_mark(r->f0);
                done = 0;
            }
        }
        iter++;
    } while (iter < 100);
}

// ---- 0x483f60: the per-frame mission tick ----------------------------------------------------------------------------------
// the 0xb0-byte prop record of the pool at Mission+0x30 (RecJ, src/include/recj.h, as this tick sees it)
struct PropRecF {
    char pad0[4];
    int used;                           // +0x04 (0 = the prop retires this frame)
    char pad8[0x28 - 8];
    float f28;                          // +0x28 (a life value; < -100 frees the prop)
    char pad2c[0x50 - 0x2c];
    char* f50;                          // +0x50 (the prop's entity; 0 = none)
    char pad54[0x68 - 0x54];
    int state;                          // +0x68 (1/2/3 age out)
    char pad6c[0x70 - 0x6c];
    int clock;                          // +0x70 (index into g_ticks)
    char pad74[4];
    long long start;                    // +0x78
    char pad80[0xb0 - 0x80];
    void update(float dt);              // 0x47d3e0 (thiscall, ret 4)
    void next_used(unsigned* i);        // 0x474300 (thiscall)
};
struct PtrTableF {                      // the paged pointer table (count; the page vector's first member sits at +0x18)
    int count;                          // +0
    char pad4[0x14];
    char** pages;                       // +0x18
    char pad1c[4];
};
typedef char assert_ptrtable[(sizeof(PtrTableF) == 0x20) ? 1 : -1];
struct PropAnim {                       // the 0x1c8-byte prop tween record (Rec1c8)
    char pad0[0x1c8];
    void update(float dt);              // 0x467c50 (thiscall, ret 4)
};
struct CarryRec {                       // the carried-item records copied by the frame&0xf sweep (0x130 bytes, 0x40 of them at +0x6b0)
    matrix m0;                          // +0x00
    matrix m1;                          // +0x40
    matrix m2;                          // +0x80
    int f_c0;                           // +0xc0 (zeroed)
    int f_c4;                           // +0xc4 (zeroed)
    char padc8[0xe8 - 0xc8];
    int f_e8;                           // +0xe8 (zeroed)
    char padec[0x130 - 0xec];
};
struct SndSetF { char pad0[0x4c]; int count; SndSlotS* slots; };   // the 0x58-byte mode record (+0x4c count, +0x50 slots)
struct SkyF {                           // the sky record at Mission+0x65a8
    char pad0[0x48];
    float f48;                          // +0x48 (the mission clock in seconds)
    char pad4c[0x1f8 - 0x4c];
    float f1f8;                         // +0x1f8 (the sky time)
    char pad1fc[0x208 - 0x1fc];
    float f208;                         // +0x208 (the sky time again)
};
struct psys_rec {                       // the shadow object's target skin (ShadowMap::target, g_shadow_map): a texture reference at +0x48
    char pad0[0x48];
    char* scene;
};
extern long long g_tick_cur;            // 0x5e2980 (the last frame's tick count)
extern char g_mt_inited;                   // 0x5e2a09
extern float g_paused_time;             // 0x5e2a0c
#include "../../include/fx.h"
extern int g_psys_scene;                // 0xa170c0
extern char g_phys[];             // 0xa181f8 (the time-scale object 0x472dc0 is called on)
struct TimeScale {
    void update(float dt);               // 0x472dc0
};
extern "C" int rand();                 // 0x536a9f
struct WorldF {
    void step(float dt);                // 0x427f40
    void remove_entity(int id);                // 0x41f0d0
};
struct Terrain_w {
    void destroy_water(int* w);         // 0x4814a0 (TerrainX::destroy_water)
};

// @ 0x483f60 -- the per-frame mission tick: the shadow-object upkeep, the 100 kHz frame clock, the carried-item sweep, the
// spawn pass, the wave sound slots, the prop pool (per-entry update + retire, list overflow cull), the prop tweens, three agent
// passes (non-horses, horses, ticks), the mission timers/sky clock and finally world::step + the frame counter
void MissionG::frame(float dt, bool spawn)
{
    psys_rec* ps = (psys_rec*)g_shadow_map.target;
    bool scene_changed = ps != 0 && *(int*)(ps->scene + 0x50) != g_psys_scene;
    if (scene_changed) {
        unsigned flags = *(unsigned*)(*(char**)(((char*)g_game) + 0x13a50) + f5df4 * 0x494 + 0x30);
        int* water = (int*)&MAT(int, 0x63c0);
        if (flags & 0x100) {
            ((Terrain_w*)&g_terrain)->destroy_water(water);
        } else if (!(flags & 1)) {
            ((ShadowMapP*)ps)->render();
        }
    }

    if (!g_mt_inited && dt > 0.0f)
        ((TimeScale*)g_phys)->update(dt);

    long long t = (long long)(dt * 100000.0f);
    g_tick_cur = t;
    g_ticks[2] += t;
    g_step_tbl[2] = dt;

    // every sixteenth frame the carried-item records restart from their first matrix
    if ((MAT(unsigned, 0x6550) & 0xf) == 1) {
        CarryRec* c = (CarryRec*)((char*)this + 0x6b0);
        int n = 0x40;
        do {
            normalize(c->m0.s, c->m1.s);
            c->f_c4 = 0;
            c->f_c0 = 0;
            c->f_e8 = 0;
            normalize(c->m2.s, c->m1.s);
            c++;
        } while (--n);
    }

    if (spawn)
        run_spawns();

    if (MAT(unsigned, 0x6550) != 0) {
        char* set = g_mission_templates + this->mode * 0x58;
        int n = *(int*)(set + 0x4c);
        char* slot = *(char**)(set + 0x50);
        for (int i = 0; i < n; i++, slot += 0x78)
            ((SndSlotS*)slot)->update(0x10);
    }

    // the prop pool: per-entry update, retire the finished ones, and steady the used slots
    PtrTableF& props = MAT(PtrTableF, 0x30);
    PtrTableF& agents = MAT(PtrTableF, 4);
    std::vector<char*>& retire_list = *(std::vector<char*>*)&MAT(char, 0x5dfc);
    std::vector<int>& cull_list = *(std::vector<int>*)&MAT(char, 0x5e0c);
    unsigned i;
    for (i = 0; i < (unsigned)props.count; i++)
        if (((PropRecF*)props.pages[i >> 4])[i & 0xf].used != 0)
            break;
    while (i < (unsigned)props.count) {
        PropRecF* p = (PropRecF*)props.pages[i >> 4] + (i & 0xf);
        p->update(dt);
        bool dead = false;
        if (p->state == 2 || p->state == 3 || p->state == 1)
            dead = (g_ticks[p->clock] - p->start) * 1e-5f > 1.0f;
        if (!dead && p->f28 < -100.0f)
            dead = true;
        if (dead) {
            if (p->f50 != 0) {
                unsigned n = (unsigned)retire_list.size();
                if (n < retire_list.capacity())
                    retire_list.push_back(p->f50);
                else
                    retire_list.insert(retire_list.end(), p->f50);
            }
            p->used = 0;
            unsigned n2 = (unsigned)cull_list.size();
            if (n2 < cull_list.capacity())
                cull_list.push_back((int)i);
            else {
                int idx = (int)i;
                cull_list.insert(cull_list.end(), idx);
            }
        }
        p->next_used(&i);
    }

    // the retire list overflows: drop a random prop and compact
    if (retire_list.size() > 0x300) {
        unsigned n = (unsigned)retire_list.size();
        unsigned k = n > 1 ? (unsigned)rand() % n : 0;
        int id = *(int*)(retire_list[k] + 8);
        ((WorldF*)scene_world)->remove_entity(id);
        unsigned last = (unsigned)retire_list.size();
        retire_list[k] = retire_list[last - 1];
        retire_list.resize(last - 1);
    }

    std::vector<PropAnim>& anims = *(std::vector<PropAnim>*)&MAT(char, 0x6688);
    for (unsigned k = 0; k < (unsigned)anims.size(); k++)
        anims[k].update(dt);

    // agents: everything but the horses, then the horses, then every agent's tick
    for (i = 0; i < (unsigned)agents.count; i++)
        if (((AgentS*)agents.pages[i >> 4])[i & 0xf].used != 0)
            break;
    while (i < (unsigned)agents.count) {
        AgentS* a = (AgentS*)agents.pages[i >> 4] + (i & 0xf);
        if (a->kind != 1)
            a->update(dt);
        ((PropRecF*)a)->next_used(&i);
    }
    for (i = 0; i < (unsigned)agents.count; i++)
        if (((AgentS*)agents.pages[i >> 4])[i & 0xf].used != 0)
            break;
    while (i < (unsigned)agents.count) {
        AgentS* a = (AgentS*)agents.pages[i >> 4] + (i & 0xf);
        if (a->kind == 1)
            a->update(dt);
        ((PropRecF*)a)->next_used(&i);
    }
    for (i = 0; i < (unsigned)agents.count; i++)
        if (((AgentS*)agents.pages[i >> 4])[i & 0xf].used != 0)
            break;
    while (i < (unsigned)agents.count) {
        AgentS* a = (AgentS*)agents.pages[i >> 4] + (i & 0xf);
        a->tick();
        ((PropRecF*)a)->next_used(&i);
    }

    update_prop_flags();

    // the sky clock and the mission timers
    SkyF* sky = MAT(SkyF*, 0x65a8);
    float& sky_time = MAT(float, 0x65ac);
    if (sky != 0) {
        sky_time = sky_time + dt * 0.04f;
        if (sky_time > 100.0f)
            sky_time = (float)fmod((double)sky_time, 100.0);
        sky->f1f8 = sky_time;
        sky->f208 = sky_time;
        sky->f48 = (float)(g_ticks[2] % 1000000000) * 7.5e-6f;
    }

    update_team_stats();
    ((WorldF*)scene_world)->step(dt);
    update_team_counts();
    MAT(unsigned, 0x6550)++;
    if (g_mt_inited)
        g_paused_time += dt;
}
