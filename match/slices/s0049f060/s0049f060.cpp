// Slice s0049f060: party bookkeeping of the mission object at 0x5e2930. 0x49f060 decides whether party `a` can
// attach/dismount onto party `b` (and where the join point is); 0x49f3d0 spawns the scene object of a party for a
// party trigger; 0x49f7c0 pops/replaces the party on top of the trigger stack; 0x49f980 and 0x49f9e0 enter a
// trigger block. Module System names: parties, party_tpl, triggers. Names provisional.
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include <new>
#include <vector>

// ---- engine strings (s004608d0/tstring.h) ----
struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4  (null while empty)
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
};
struct StrTable { tstring* find(const tstring* key); };                         // 0x418cf0 (ret 4, null when absent)
extern StrTable g_strtable;                                                     // 0x5d6fa0

// ---- the game data ---------------------------------------------------------------------------------------
struct Party;                                                                     // 0x5ac8 bytes
struct PartyMgr {                                                                 // embedded in the game data at +0xf8
    char pad0[0x138cc];
    std::vector<char*> pages;                                                     // +0x138cc (page pointers, 16 parties per page)
    int deflt;                                                                    // +0x138dc (never read here)
    int index_of(int id);                                                         // 0x48ad00 (thiscall, ret 4)
    Party* get(int id)                                                            // 0x51ee50 (thiscall, ret 4): out of line when the caller is too big
    {
        int k = index_of(id);
        return (Party*)((k & 0xf) * 0x5ac8 + *(pages.begin() + (k >> 4)));
    }
    void next_party_id(int* p);                                                   // 0x460ca0 (thiscall, ret 4)
};
struct KindRec {                                                                  // 0x98-byte scene object record
    int kind;                                                                     // +0
    char pad4[0x98 - 4];
};
struct Party {
    char pad0[0x30];
    float x, y, z, w;                                                             // +0x30 (position)
    char pad40[0x54 - 0x40];
    int f54;                                                                      // +0x54
    char pad58[0x78 - 0x58];
    int f78;                                                                      // +0x78 (the mounted/attached party)
    char pad7c[0x88 - 0x7c];
    int f88;                                                                      // +0x88 (scene object index, < 0 = not deployed)
    char pad8c[0x8b4 - 0x8c];
    char f8b4;                                                                    // +0x8b4
    char pad8b5[0x8bc - 0x8b5];
    int* f8bc;                                                                    // +0x8bc (item count table)
    char pad8c0[0x8c8 - 0x8c0];
    int f8c8;                                                                     // +0x8c8
    char f8cc;                                                                    // +0x8cc
    char pad8cd[0x5ac8 - 0x8cd];
};
struct TroopRec {                                                                 // 0x508 bytes
    char pad0[0x30];
    unsigned __int64 flags;                                                       // +0x30 tf_*
    char pad38[0x48 - 0x38];
    int level;                                                                    // +0x48
    char pad4c[0x508 - 0x4c];
};
struct ItemRef {                                                                  // 8 bytes
    int item; union { unsigned m4; struct { unsigned char amount; char pad[2]; signed char mod; } b; };
};
struct V2 {
    float x, y;
    V2(float a, float b) : x(a), y(b) {}
};
struct Receiver {                                                                 // a 0x98-byte scene object record
    char pad0[0x54];
    int f54;                                                                      // +0x54
    int f58;                                                                      // +0x58
    char pad5c[0x98 - 0x5c];
};
struct GameData {
    char pad0[0x78];
    int f78;                                                                      // +0x78 (player party)
    char pad7c[0xe0 - 0x7c];
    std::vector<char*> nodes;                                                     // +0xe0 (scene object pages, 16 records of 0x98 bytes)
    char padf0[0xf8 - 0xf0];
    PartyMgr parties;                                                             // +0xf8
    char pad_to_chars[0x13a00 - 0xf8 - sizeof(PartyMgr)];
    TroopRec* chars;                                                              // +0x13a00
    char pad13a04[0x13a0c - 0x13a04];
    int f13a0c;                                                                   // +0x13a0c
    int f13a10;                                                                   // +0x13a10
    int f13a14;                                                                   // +0x13a14
    char pad13a18[0x13ad0 - 0x13a18];
    float f13ad0;                                                                 // +0x13ad0
    int f13ad4;                                                                   // +0x13ad4
    char pad13ad8[0x14014 - 0x13ad8];
    unsigned char f14014;                                                         // +0x14014 (trigger block object)

    Receiver* node(unsigned idx)                                                  // scene object by index
    {
        return (Receiver*)((idx & 0xf) * 0x98 + *(nodes.begin() + (idx >> 4)));
    }
    int last_leader(int id);                                                      // 0x48fce0 (ret 4)
    float party_relation(int a, int b);                                           // 0x48ffa0 (ret 8)
    void attach_party(int a, int b);                                              // 0x4949d0 (ret 8)
    void attach_party_center(const V2& pt, int a, int b);                         // 0x49e9c0 (ret 0xc)
    bool deploy_party(int a, int b);                                              // 0x49f060 (ret 8)
};
extern GameData* g_game;                                                          // 0x5e2930


// ---- the mission -----------------------------------------------------------------------------------------
struct PartyStack { int id; int val; void reduce(int n); };                       // 8 bytes, mission+0x404
// PartyStack::reduce is 0x437510 (ItemStack::reduce: take `n` off the stack).
struct TriggerStack {                                                             // the party trigger stack
    PartyStack sp[0x80];                                                          // +0 (depth i reads sp[i-1])
    int count;                                                                    // +0x400 (stack depth)
};
struct NextUsedTbl { void next_used(int* p); };                                   // 0x43b160 (thiscall, ret 4)
struct Mission {
    KindRec* table;                                                               // +0x0 (0x98-byte scene object records)
    char pad4[0x5c - 4];
    int local_id;                                                                 // +0x5c
    char pad60[0x400 - 0x60];
    int sp_count;                                                                 // +0x400 (party trigger stack depth)
    PartyStack sp[0x80];                                                          // +0x404
    char pad804[0x13a0c - 0x804];
    int f13a0c;                                                                   // +0x13a0c
    int f13a10;                                                                   // +0x13a10
    int f13a14;                                                                   // +0x13a14
    char pad13a18[0x13a48 - 0x13a18];
    float* harvest;                                                               // +0x13a48 (party x party matrix)
    char pad13a4c[0x13ad0 - 0x13a4c];
    float f13ad0;                                                                 // +0x13ad0
    int f13ad4;                                                                   // +0x13ad4

    Party* first_party(int id);                                                   // 0x48fce0 (this = mission-4; ret 4)
    float party_relation(int a, int b);                                           // 0x48ffa0 (this = mission-4; ret 8)
    void attach_party(int a, int b);                                              // 0x4949d0 (this = mission-4; ret 8)
    void attach_party_center(const float* pt, int a, int b);                      // 0x49e9c0 (this = mission-4; ret 0xc)
    void set_trigger_value(int val, long long* out);                              // 0x49d500 (this = mission-4; ret 8)
    int add_party_to_trigger(int a, int* b, int* c);                              // 0x49d650 (this = mission-4; ret 0xc)
    bool trigger_step(int a, void* t);                                            // 0x49d7f0 (ret 8)
    void pop_party(int a, int t, int* out);                                       // 0x49f7c0 (ret 0xc)
    void enter_trigger(int val, int ip);                                          // 0x49f980 (ret 8)
    void exec_trigger(int val);                                                   // 0x49f9e0 (ret 4)
    // trigger-block builders (thiscall, the block handle is the first argument)
    void block_add(void* block, const void* pos, int one);                        // 0x4836e0 (ret 0xc)
    void block_exec(void* block, int val, int a, int b, int c, int d, int e, int f,
                    int g, char* out);                                            // 0x483870 (ret 0x28)
};
extern Mission* g_mission;                                                        // 0x5e292c

// ---- spawn helpers (other slices) ------------------------------------------------------------------------
struct SceneObj {
    virtual void f0();
    virtual void f4();
    virtual void f8();
    virtual void fc();
    virtual void f10();
    virtual void f14();
    virtual void set_pos(const float* arg);                                       // slot 6
    char pad4[0x58 - 4];
    char m[0x40];                                                                 // +0x58 (frame)
    char pad98[0xc4 - 0x98];
    int f_c4;                                                                     // +0xc4
    int f_c8;                                                                     // +0xc8
    float f_cc;                                                                   // +0xcc
    int f_d0;                                                                     // +0xd0
    char pad_d4[0x304 - 0xd4];
    struct FxDef2* skel;                                                          // +0x304
    char pad308[0x31c - 0x308];
};
struct FxDef2 {                                                                   // 0x7c bytes (created by world::create_entity(2))
    FxDef2(int kind);                                                             // 0x428820 (ret 4)
    char pad0[0x40];
    unsigned char* f40;                                                           // +0x40
    char pad44[0x5c - 0x44];
    int f5c;                                                                      // +0x5c
    char pad60[0x7c - 0x60];
    void set_env_count(int n);                                                    // 0x46b9d0 (ret 4)
    void set_skeleton(void* p);                                                   // 0x4288c0 (ret 4)
    void set_group0(int g, int id, int a, int b);                                 // 0x439fc0 (ret 0x14)
    void link_group(int g);                                                       // 0x4339d0 (ret 4)
    void set_blend(float f, const void* m);                                       // 0x421df0 (ret 8)
};
struct PickArg { char pad[0x400]; };                                              // the v[] argument of 0x482240
struct SiteRec {                                                                  // 0x1dc bytes, one per scene site
    char pad0[0x34];
    float x, y, z, w;                                                             // +0x34
    char pad44[0x1dc - 0x44];
};
struct Group {                                                                    // 0x30-byte records at 0xa76628
    char pad0[0x10];
    int* src;                                                                     // +0x10
    int count;                                                                    // +0x14
    char pad18[0x30 - 0x18];
};
extern Group* g_groups;                                                           // 0xa76628
extern int* g_body_table;                                                         // 0x8ed4f7c (vector<int>, entry ptr at +0x1e8)
extern unsigned g_body_count;                                                     // 0x8ed4f88

// @ 0x49f060
// Decide whether party `a` can be attached to `b`, and where the join point is. When the leader of `b` is not
// deployed yet the two parties join at the midpoint of their positions (the player's party is always the first
// one handed to attach_party_center); otherwise `a` adopts the leader's scene object (the player) or attaches to
// the party standing next to it.
bool GameData::deploy_party(int a, int b)
{
    int lead = last_leader(b);
    if (parties.get(lead)->f88 >= 0) {
        if (a == f78) {
            f13a0c = node(parties.get(lead)->f88)->f58;
            f13a10 = node(parties.get(lead)->f88)->f54;
            f13a14 = 0;
            parties.get(a)->f88 = parties.get(lead)->f88;
            return 1;
        }
        int x = node(parties.get(lead)->f88)->f58;
        if (x == lead)
            x = node(parties.get(lead)->f88)->f54;
        if (party_relation(x, a) < -0.01f)
            return 0;
        attach_party(a, x);
        parties.get(a)->f8cc = 1;
        return 1;
    }
    if (a == f78) {
        lead = last_leader(b);
        f13a0c = lead;
        f13a10 = -100000;
        f13a14 = 0;
        Party* pl = parties.get(lead);
        Party* pa = parties.get(a);
        attach_party_center(V2((pa->x + pl->x) * 0.5f, (pl->y + pa->y) * 0.5f), a, lead);
        return 1;
    }
    if (b == f78) {
        if (0.0f < f13ad0 && f13ad4 != 0)
            return 0;
        int la = last_leader(a);
        f13a0c = la;
        f13a10 = -100000;
        f13a14 = 1;
        Party* p1 = parties.get(la);
        Party* p2 = parties.get(b);
        attach_party_center(V2((p1->x + p2->x) * 0.5f, (p2->y + p1->y) * 0.5f), b, a);
        return 1;
    }
    Party* p1 = parties.get(a);
    Party* p2 = parties.get(lead);
    attach_party_center(V2((p2->x + p1->x) * 0.5f, (p2->y + p1->y) * 0.5f), a, lead);
    return 1;
}

// The work area 0x49f3d0 shares with the appearance/agent builders. The member names are the frame offsets of
// the original (local_bc .. local_54): `b8` is local_b8, `f60` is local_60, and `f74`/`f70` receive the mesh
// node lists the builders hand back.
struct SpawnArea {
    int bc; int b8; int b4; int b0; int ac; int a8; int a4; int a0;
    int x9c; int x98; int x94; int x90; int x8c; int x88; int x84; int x80; int x7c; int x78;
    int f74; int f70; int x6c; int x68; int x64; int f60; int x5c; int x58; int x54;
    // the original zeroes every member with its own store
    __forceinline SpawnArea()
        : bc(0), b8(0), b4(0), b0(0), ac(0), a8(0), a4(0), a0(0), x9c(0), x98(0), x94(0), x90(0), x8c(0), x88(0),
          x84(0), x80(0), x7c(0), x78(0), f74(0), f70(0), x6c(0), x68(0), x64(0), f60(0), x5c(0), x58(0), x54(0)
    {
    }
};
struct V4c {                                                                      // w is left uninitialised (a stack slot)
    float x, y, z, w;
    V4c(float a, float b, float c) : x(a), y(b), z(c) {}
};
struct World {
    char pad0[0x344];
    int f344;                                                                     // +0x344
    int f348;                                                                     // +0x348
    int f34c;                                                                     // +0x34c
    SceneObj* create_entity(int kind);                                            // 0x428440 (ret 4)
};
struct PtrVec {                                                                   // a vector of pointers (begin/end)
    void** begin;                                                                 // +0 (0xa75f54)
    void** end;                                                                   // +4 (0xa75f58)
    int size() { return (int)(end - begin); }
};
extern PtrVec g_world_list;                                                       // 0xa75f54
struct MeshNodeList { void cap_all(float* v); };                                  // 0x438f90 (ret 4)

// appearance / agent builders (other slices; the argument lists are taken from the disassembly)
__int64 pick_slot_item(int a, int b, int c, int d, int* e, char f, char* g);       // 0x46b190 (cdecl, ret 0x1c)
void build_appearance(SceneObj* e, int count, int party, int arg, PartyStack* s, void* b8, void* name,
                      void* sixty, float* dist, void* bc, ItemRef* items, int one); // 0x482240 (cdecl)
void build_agent(SceneObj* e, int count, int party, void* b8, float* dist, ItemRef* items,
                 int one);                                                        // 0x47cfe0 (cdecl, ret 0x1c)
void link_appearance(SceneObj* e, ItemRef* items, void* b8);                       // 0x495430 (ret 0xc)

// @ 0x49f3d0
// Spawn the scene object of party `party` (negative = its horse) and build its full appearance: create the
// entity in the current world, copy the 0x40-byte frame matrix into it, resolve the skeleton (a skin for a
// troop, "skel_horse" for a horse), fill the ten item slots from the party's chosen troop, run the
// appearance/agent builders, link the animation groups and set the position. The builders themselves
// (0x482240, 0x47cfe0, 0x495430, 0x46b190, 0x438f90, 0x439fc0, 0x4339d0, 0x421df0, 0x428440, 0x428820,
// 0x4288c0, 0x46b9d0) live in other slices.
void spawn_party_scene(int party, int kind, int* frame, int* group, int* args)
{
    if (g_world_list.begin == 0 || g_world_list.size() == 0)
        return;
    World* world = (World*)g_world_list.begin[g_world_list.size() - 1];

    SceneObj* e = world->create_entity(9);
    memcpy(e->m, frame, 0x40);

    PartyStack s;
    s.id = kind;
    s.val = 0;
    s.reduce(0);

    SpawnArea area;
    float dist = 0.0f;
    char scratch = 0;

    FxDef2* def = new FxDef2(2);
    e->skel = def;
    def->set_env_count(2);
    def->f5c = 0;

    ItemRef items[10];
    int count;
    if (party < 0) {
        def->set_skeleton(g_strtable.find(&tstring("skel_horse")));
        count = 1;
    } else {
        unsigned t = (unsigned)g_game->chars[party].flags & 0xf;
        def->set_skeleton(t < (unsigned)g_body_count
                              ? *(void**)((char*)g_body_table + 0x1e8 + t * 0x1f8) : 0);
        int level = g_game->chars[party].level;
        for (int i = 0; i < 10; i++)
            *(unsigned __int64*)&items[i] = pick_slot_item(party, i, world->f344, world->f348,
                                                           &world->f34c, 0, &scratch);
        count = level;
    }
    char name[0x10];
    build_appearance(e, count, party, *args, &s, &area.b8, name, &area.f60, &dist, &area.bc, items, -1);
    if (party >= 0) {
        build_agent(e, count, party, &area.b8, &dist, items, -1);
        link_appearance(e, items, &area.b8);
        if (area.f74 != 0) {
            dist = 20.0f;
            for (int i = 0; i < *(int*)(area.f74 + 0x84); i++)
                ((MeshNodeList*)(area.f74 + 0x34 + i * 0x10))->cap_all(&dist);
        }
        if (area.f70 != 0) {
            dist = 20.0f;
            for (int i = 0; i < *(int*)(area.f70 + 0x84); i++)
                ((MeshNodeList*)(area.f70 + 0x34 + i * 0x10))->cap_all(&dist);
        }
    }
    Group* g = &g_groups[*group];
    if (g->count > 0) {
        int* src = g->src;
        e->skel->set_group0(0, src[0], src[4], src[5]);
        e->skel->link_group(0);
        *(int*)e->skel->f40 = 0x3f800000;
        e->skel->set_blend(0.05f, e->m);
    }
    *(V4c*)&e->f_c4 = V4c(0.0f, 0.0f, 1.1f);
    dist = 100000.0f;
    e->set_pos(&dist);
}

// @ 0x49f7c0
// Pop / replace the party on top of the trigger stack. `n` is the depth read once at entry; the top entry's
// kind decides what happens to its value (kind 4/5 just unwind, kind 6/7/0xb/0xf/0xc shift it along one), then the
// trigger step runs and the (possibly re-mapped) value is written back as a 64-bit script value.
void Mission::pop_party(int a, int t, int* out)
{
    int val;
    int idx;
    (*out)++;
    int n = *(int*)(t + 0x400);
    if (n == 0)
        return;
    if (n > 0 && n < 0x80) {
        idx = *(int*)(t + n * 8 - 8);
        val = *(int*)(t + n * 8 - 4);
    }
    int kind = table[idx].kind;
    if (kind == 4 || kind == 5) {
        if (n < 1)
            return;
        *(int*)(t + 0x400) = n - 1;
        return;
    }
    if (kind == 6 || kind == 7 || kind == 0xb || kind == 0xf || kind == 0xc) {
        if (kind == 0xc) {
            ((NextUsedTbl*)((char*)g_mission + 4))->next_used(&val);
        } else if (kind == 7) {
            val--;
        } else if (kind == 0xb) {
            ((PartyMgr*)((char*)g_game + 0xf8))->next_party_id(&val);
        } else {
            val++;
        }
    } else {
        return;
    }
    if (n > 0 && n < 0x80)
        *(int*)(t + n * 8 - 4) = val;
    if (!trigger_step(a, (void*)t)) {
        int c = *(int*)(t + 0x400);
        if (c > 0)
            *(int*)(t + 0x400) = c - 1;
        return;
    }
    if (kind == 0xf) {
        int one = 1;
        int p = add_party_to_trigger(a, &one, &a);
        val = ((PartyMgr*)((char*)g_game + 0xf8))->get(p)->f8bc[val];
    }
    long long wide = val;
    set_trigger_value(a, &wide);
    *out = idx + 1;
}

// @ 0x49f980
// Enter the trigger block `val` at statement `ip`: pack `ip` (0..0x2a) into the top byte of `val`, then
// register the entry with the trigger block object at g_game+0x14014.
void Mission::enter_trigger(int val, int ip)
{
    PartyStack s;
    s.id = val;
    if (ip < 0x2b && ip >= 0) {
        s.val = (s.val & 0xffffff) | (ip << 24);
        s.reduce(0);
    }
    block_add((void*)((char*)g_game + 0x14014), &s, 1);
}

// @ 0x49f9e0
void Mission::exec_trigger(int val)
{
    char out[0x20];
    block_exec((void*)((char*)g_game + 0x14014), val, -1, 0, 0, -1, 0, 0, 0, out);
}
