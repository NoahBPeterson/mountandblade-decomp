// Slice s004902a0: helpers of the mission party-holder (vtable 0x5924a0). The object embeds the party
// manager (hash map + page table) and a vector of attached entities at +0x17c. 0x4902a0 builds a hash-like
// token from a party record; 0x4903e0 / 0x490430 / 0x4904a0 read script state; 0x490520 / 0x490930 are the
// two destructors; 0x490570 / 0x4905c0 place and bound an entity; 0x4906b0..0x4908c0 walk the entity
// vector; 0x490990 marks the parties of a given kind dirty. All names provisional.
#include <windows.h>
#include <stdlib.h>
#include <new>

struct Party {                                  // 0x5ac8-byte party record
    int f0;
    int type;                                   // +0x04 (== 1 tests the "slot" kind)
    char pad8[0x70 - 8];
    int kind70;                                 // +0x70 (kind tested by mark_kind)
    char pad74[0xac - 0x74];
    int groups[64][8];                          // +0xac, eight 32-byte groups
    char pad8ac[0x8b5 - 0x8ac];
    char flag8b5;                               // +0x8b5 (dirty flag)
    char pad8b6[0x5ac8 - 0x8b6];
};

struct Mgr;
struct VBaseE {
    virtual void v00(int f) = 0;                        // +0x00 destroy
    virtual void v01() = 0;
    virtual void v02(int p) = 0;                        // +0x08 on_event
    virtual void v03() = 0; virtual void v04() = 0; virtual void v05() = 0;
    virtual void v06() = 0; virtual void v07() = 0; virtual void v08() = 0;
    virtual void v09() = 0;
    virtual void v10(int* a, float* b, float* c, float* d) = 0;   // +0x28 on_bounds
    virtual void v11() = 0; virtual void v12() = 0; virtual void v13() = 0;
    virtual void v14() = 0; virtual void v15() = 0;
    virtual void v16() = 0;                             // +0x40 reset
    virtual void v17() = 0; virtual void v18() = 0; virtual void v19() = 0;
    virtual void v20() = 0; virtual void v21() = 0; virtual void v22() = 0;
    virtual void v23(int* p) = 0;                       // +0x5c other
    virtual void v24() = 0;                             // +0x60 update
};
struct VBaseM {
    virtual void m00(int f) = 0;
    virtual void m01() = 0; virtual void m02() = 0; virtual void m03() = 0;
    virtual void m04() = 0; virtual void m05() = 0; virtual void m06() = 0;
    virtual void m07() = 0; virtual void m08() = 0; virtual void m09() = 0;
    virtual void m10() = 0; virtual void m11() = 0; virtual void m12() = 0;
    virtual void m13() = 0; virtual void m14() = 0; virtual void m15() = 0;
    virtual void m16() = 0; virtual void m17() = 0; virtual void m18() = 0;
    virtual void m19() = 0; virtual void m20() = 0; virtual void m21() = 0;
    virtual void m22() = 0;
    virtual void m23(Mgr* e) = 0;                       // +0x5c
    virtual void m24() = 0;                             // +0x60
};

// The party manager: one hash map at the object start (0x48ad00 indexes it), a page table of parties at
// +0x138d0, and the holder's own fields (entity vector, bounds, flags) interleaved.
struct Mgr {
    char pad0[0x104];
    unsigned flags;                             // +0x104 (bit 0 = disabled, 0x100 = track bounds)
    char pad108[0x154 - 0x108];
    void* owner;                                // +0x154 (owner object, when any)
    char pad158[0x17c - 0x158];
    void** entities;                            // +0x17c (vector of entity pointers)
    void** entities_end;                        // +0x180
    void** entities_cap;                        // +0x184
    char pad188[0x138d0 - 0x188];
    Party** pages;                              // +0x138d0 (page of 16 parties)
    Party** pages_end;                          // +0x138d4
    Party** pages_cap;                          // +0x138d8
    unsigned count;                             // +0x138dc (party id bound)
    int index_of(unsigned id);                  // 0x48ad00 (thiscall, ret 4)
    unsigned first_party_id();                  // 0x460d20 (thiscall, ret 0)
    void next_party_id(unsigned* io);           // 0x460ca0 (thiscall, ret 4)
    int paged_last();                           // 0x490310 (thiscall, ret 0; paged-vector last element)
    void clear_entities();                      // 0x490340 (thiscall, ret 0)
    int is_slot(unsigned id);                   // 0x4903e0 (thiscall, ret 4)
    int agent_of(int c);                        // 0x4904a0 (thiscall, ret 0)
    void dtor_common();                         // 0x490520 (thiscall, ret 0)
    void place(int* a);                         // 0x490570 (thiscall, ret 4)
    void track_bounds(void* p);                 // 0x4905c0 (thiscall, ret 4)
    void dtor_body();                           // 0x48ef40 (thiscall, ret 0)
    void dtor_base();                           // 0x487c00 (thiscall, ret 0)
    void bounds_init(int* a, float* p3, float* p4, float* p5); // 0x488720 (ret 0x10)
    void activate_all();                        // 0x4906b0 (thiscall, ret 0)
    void reset_all();                           // 0x4906e0 (thiscall, ret 0)
    void bounds_of(int* a, float* p3, float* p4, float* p5);  // 0x490710 (thiscall, ret 0x10)
    void update_all(int p);                     // 0x4908c0 (thiscall, ret 4)
    Mgr* delete_this(unsigned flag);            // 0x490930 (thiscall, ret 4)
};
inline Party* party_at(Mgr* m, int idx) { return m->pages[idx >> 4] + (idx & 0xf); }

// ---- global state -----------------------------------------------------------------------------------------
struct AgentRec {                               // 0x5928-byte record, page table at mission+0x1c
    char pad0[0x250];
    int f250;                                   // +0x250
    char pad254[0x5928 - 0x254];
};
struct Vehicle {                                // 0x1c8-byte record at mission+0x668c
    char pad0[0x1b4];
    int f1b4;                                   // +0x1b4
    char pad1b8[0x1c8 - 0x1b8];
};
struct Mission {
    char pad0[0x1c];
    AgentRec** agents;                          // +0x1c
    char pad20[0x668c - 0x20];
    char* parties;                              // +0x668c
};
struct GameData {
    char pad0[0x1a1f0];
    int f1a1f0;                                 // +0x1a1f0 (index into mission->parties)
    unsigned f1a1f4;                            // +0x1a1f4 (index into mission->agents)
    char pad1a1f8[0x1a204 - 0x1a1f8];
    int f1a204;                                 // +0x1a204 (free vehicle slot, -1 when none)
};
extern GameData* g_game;                        // 0x5e2930
extern Mission* g_mission;                      // 0x5e292c
extern void* g_agents;                          // 0x5e29d0
int agent_id(void* rec);                        // 0x433df0 (ret 4; ecx = agent record)
int __fastcall script_value(int c);             // 0x490430
void operator_delete(void* p);                  // 0x535712 (??3)


struct World {                                  // the game object; the manager at +0xf8
    char pad0[0xf8];
    Mgr mgr;
    int token(unsigned id, int group, int f14, int f18);  // 0x4902a0 (ret 0x10)
    void mark_kind(int kind);                             // 0x490990 (ret 4)
};

// @ 0x4902a0
int World::token(unsigned id, int group, int f14, int f18)
{
    int v = -1;
    if ((int)id >= 0) {
        Mgr* m = &mgr;
        int i = m->index_of(id);
        Party* p = m->pages[i >> 4];
        v = p[i & 0xf].groups[group][0];
    }
    return v * 0x1bf + (int)id * 0x7bb7 + f14 + 0x45db + f18;
}

// @ 0x490310
int Mgr::paged_last()
{
    int* v = (int*)((char*)this + 4);
    unsigned i = v[3] + v[2] - 1;
    unsigned q = i >> 2;
    unsigned rem = i - q * 4;
    if ((unsigned)v[1] <= q)
        q -= (unsigned)v[1];
    return *(int*)(v[0] + q * 4) + (int)rem * 4;
}

// @ 0x490340
void Mgr::clear_entities()
{
    Party** i = pages;
    while (i < pages_end) {
        Party* e = *i;
        if (e != 0) {
            int* p = (int*)((char*)e - 4);
            if (*p != 0)
                ((VBaseE*)e)->v00(3);
            else
                operator delete[](p);
        }
        *i = 0;
        i++;
    }
    if (pages != 0)
        operator delete(pages);
    pages = 0;
    pages_end = 0;
    pages_cap = 0;
    void** base = (void**)((char*)this + 0x138c0);
    if (*base != 0)
        operator delete(*base);
    base[0] = 0;
    base[1] = 0;
    base[2] = 0;
}

// @ 0x4903e0
int Mgr::is_slot(unsigned id)
{
    if (id < count) {
        int i = index_of(id);
        if (i >= 0)
            return party_at(this, i)->type == 1;
    }
    return 0;
}

// @ 0x490430
int __fastcall script_value(int c)
{
    int v = 0;
    if (c == -0x28 || c == -0x2b || c == -0x2a) {
        char* base = g_mission->parties;
        v = *(int*)(base + g_game->f1a1f0 * 0x1c8 + 0x1b4);
    } else if (c == -0x32 || c == -0x33) {
        unsigned idx = g_game->f1a1f4;
        unsigned page_idx = idx >> 4;
        unsigned rem = idx & 0xf;
        AgentRec* page = *(AgentRec**)((char*)g_mission->agents + page_idx * 4);
        return *(int*)((char*)page + rem * 0x5928 + 0x250);
    }
    return v;
}

// @ 0x4904a0
int Mgr::agent_of(int c)
{
    if (script_value(c) == 0)
        return -1;
    if (c != -0x32 && c != -0x33)
        return -1;
    if (g_game->f1a204 < 0)
        return -1;
    unsigned idx = g_game->f1a1f4;
    int off = (idx & 0xf) * 0xb25 + g_game->f1a204;
    int id = *(int*)((char*)g_mission->agents[idx >> 4] + off * 8 + 0x1d0);
    if (id < 0)
        return -1;
    return agent_id((char*)g_agents + id * 0x238);
}

// @ 0x490520
void Mgr::dtor_common()
{
    *(void**)this = (void*)0x5924a0;
    dtor_body();
    if (entities != 0)
        operator_delete(entities);
    entities = 0;
    entities_end = 0;
    entities_cap = 0;
    dtor_base();
}

// @ 0x490570
void Mgr::place(int* a)
{
    *(int*)((char*)this + 0xe8) = a[0];
    *(int*)((char*)this + 0xec) = a[1];
    VBaseM* self = (VBaseM*)this;
    if (owner != 0) {
        ((VBaseM*)owner)->m23(this);
        self->m24();
        return;
    }
    *(int*)((char*)this + 0xe0) = a[0];
    *(int*)((char*)this + 0xe4) = a[1];
    self->m24();
}

// @ 0x4905c0
void Mgr::track_bounds(void* p)
{
    char* e = (char*)p;
    float n[2];
    volatile float* ep = (volatile float*)(e + 0xe8);
    n[0] = ep[0] + *(float*)((char*)this + 0xe0);
    *(int*)(e + 0xe0) = *(int*)&n[0];
    n[1] = ep[1] + *(float*)((char*)this + 0xe4);
    *(int*)(e + 0xe4) = *(int*)&n[1];
    ((VBaseE*)e)->v24();
    if (flags & 0x100) {
        float v;
        v = *(float*)(e + 0xf0) + *(float*)(e + 0xe8);
        if (v > *(float*)((char*)this + 0xf0))
            *(float*)((char*)this + 0xf0) = v;
        v = *(float*)(e + 0xf8) + *(float*)(e + 0xe8);
        if (v < *(float*)((char*)this + 0xf8))
            *(float*)((char*)this + 0xf8) = v;
        v = *(float*)(e + 0xf4) + *(float*)(e + 0xec);
        if (v > *(float*)((char*)this + 0xf4))
            *(float*)((char*)this + 0xf4) = v;
        v = *(float*)(e + 0xfc) + *(float*)(e + 0xec);
        if (v < *(float*)((char*)this + 0xfc))
            *(float*)((char*)this + 0xfc) = v;
    }
}

// @ 0x4906b0
void Mgr::activate_all()
{
    void** i = entities;
    while (i != entities_end) {
        ((VBaseM*)this)->m23((Mgr*)*i);
        i++;
    }
}

// @ 0x4906e0
void Mgr::reset_all()
{
    void** i = entities;
    while (i != entities_end) {
        ((VBaseE*)*i)->v16();
        i++;
    }
}

// @ 0x490710
void Mgr::bounds_of(int* a, float* p3, float* p4, float* p5)
{
    if (flags & 1)
        return;
    struct __declspec(align(8)) Locals {
        float z, y0, hy, ly, mx, my;
    } l;
    bounds_init(a, p3, p4, p5);
    l.mx = p4[0];
    l.y0 = p3[0];
    l.my = p4[1];
    l.ly = p5[1];
    l.hy = p5[0];
    float v = *(float*)((char*)this + 0xf8) + *(float*)((char*)this + 0xe0);
    if (l.mx < v) l.mx = v;
    v = *(float*)((char*)this + 0xf0) + *(float*)((char*)this + 0xe0);
    if (l.hy > v) l.hy = v;
    v = *(float*)((char*)this + 0xfc) + *(float*)((char*)this + 0xe4);
    if (l.my < v) l.my = v;
    v = *(float*)((char*)this + 0xf4) + *(float*)((char*)this + 0xe4);
    if (l.ly > v) l.ly = v;
    void** i = entities;
    while (i != entities_end) {
        char* e = (char*)*i;
        if (!(*p5 < *(float*)(e + 0xf8) + *(float*)(e + 0xe0)) &&
            !(p5[1] < *(float*)(e + 0xfc) + *(float*)(e + 0xe4)) &&
            !(*(float*)(e + 0xf0) + *(float*)(e + 0xe0) < p4[0]) &&
            !(*(float*)(e + 0xf4) + *(float*)(e + 0xe4) < p4[1])) {
            l.z = p3[0];
            ((VBaseE*)e)->v10(a, &l.z, &l.mx, &l.hy);
            if (l.z > l.y0) l.y0 = l.z;
        }
        i++;
    }
    p3[0] = l.y0;
}

// @ 0x4908c0
void Mgr::update_all(int p)
{
    if (flags & 1)
        return;
    void** i = entities;
    while (i != entities_end) {
        *(int*)((char*)*i + 0x100) = 0;
        i++;
    }
    i = entities;
    while (i != entities_end) {
        ((VBaseE*)*i)->v02(p);
        i++;
    }
}

// @ 0x490930
Mgr* Mgr::delete_this(unsigned flag)
{
    *(void**)this = (void*)0x5924a0;
    dtor_body();
    if (entities != 0)
        operator_delete(entities);
    entities = 0;
    entities_end = 0;
    entities_cap = 0;
    dtor_base();
    if (flag & 1)
        operator_delete(this);
    return this;
}

// @ 0x490990
void World::mark_kind(int kind)
{
    Mgr* m = &mgr;
    unsigned i = m->first_party_id();
    while (i < *(unsigned*)((char*)this + 0x139d4)) {
        if (party_at(m, m->index_of(i))->kind70 == kind)
            party_at(m, m->index_of(i))->flag8b5 = 1;
        m->next_party_id(&i);
    }
}

// @ 0x48ef40 (destructor body shared by 0x490520 / 0x490930)
void dtor_body(Mgr* m)
{
    void** i = m->entities;
    while (i < m->entities_end) {
        void* e = *i;
        if (e != 0)
            ((VBaseE*)e)->v00(1);
        i++;
    }
    if (m->entities != 0)
        operator_delete(m->entities);
    if (*(void**)((char*)m + 0x180) != 0)
        operator_delete(*(void**)((char*)m + 0x180));
}
