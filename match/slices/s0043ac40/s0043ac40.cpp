// Slice s0043ac40: the mission's agent table and the per-agent "nearby enemies" bookkeeping, 0x43ac40..0x43b5c9
// (all names provisional): a light/influence accumulator over the scene's point sources, small accessors of a
// 0x1f8-byte record table, the agent table's page-table iteration, an int hash-bucket lower_bound, two VC7.1
// <vector> helpers (_Ufill for tstring, _Ucopy for the 0x1c8 records) and the sorted list of the 16 nearest
// enemy agents with the "who to look at" picker.
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;

struct ptr_tag {};          // std::_Nonscalar_ptr_iterator_tag: an UNINITIALISED local pushes whatever EAX holds

struct tstring { int pos; char* buf; int cap; int len; };
struct E1c8 { virtual ~E1c8(); char body[0x1c8 - 4]; };
struct allocstr {};
struct allocE1c8 {};
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v, allocstr& al, ptr_tag tag);                  // 0x438f00
E1c8* __cdecl uninit_copy1c8(E1c8* f, E1c8* l, E1c8* d, allocE1c8& al, ptr_tag tag);                                   // 0x439060

// ---- scene light sources (provisional): 0x43ac40 accumulates direction / colour of the lights reaching a point --------------
struct Light {
    int type;                                          // +0 (2 = directional)
    float r, g, b;                                     // +4 / +8 / +0xc
    char pad10[0x24 - 0x10];
    vector4 pos;                                       // +0x24
    char pad34[0x44 - 0x34];
    vector4 dir;                                       // +0x44 (directional lights)
    float range;                                       // +0x54
    char pad58[0x6c - 0x58];
    unsigned flags;                                    // +0x6c (bit 0: half intensity)
};
struct Scene {
    char pad0[0xb8];
    int lights_alloc;                                  // +0xb8 (std::vector<Light*>: allocator, first, last, end)
    Light** lights_first;                              // +0xbc
    Light** lights_last;                               // +0xc0
    // vector4 arguments are passed by value (4 dwords each)
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);   // 0x43ac40 (thiscall, ret 0x30)
    char line_clear(const vector4* from, const vector4* to, float maxd, int a, void** hit);   // 0x439d60 (thiscall, ret 0x14)
};

// ---- 0x1f8-byte records (table object: records at +0x1c, count at +0x28) -------------------------------------------
struct Rec1f8 { char pad[0x1f0]; int f1f0; int f1f4; };
struct Table1f8 {
    char pad0[0x18];
    std::vector<Rec1f8> recs;                          // +0x18 (first at +0x1c)
    unsigned count;                                    // +0x28
    int get_f1f0(unsigned i);                          // 0x43b100
    int get_f1f4(unsigned i);                          // 0x43b130
};
// @ 0x43b100
int Table1f8::get_f1f0(unsigned i)
{
    if (i < count) return recs[i].f1f0;
    return 0;
}
// @ 0x43b130
int Table1f8::get_f1f4(unsigned i)
{
    if (i < count) return recs[i].f1f4;
    return 0;
}

// ---- the agent table (object at mission+4; agents are 0x5928-byte records in pages of 16) ------------------------------
struct Threat { int id; float dist; vector4 dir; };   // 0x18 bytes
struct Agent;
struct AgentTableExt {                                 // same object seen from another TU (no bodies: cl must not assume register use)
    int count;
    int first_used();                                  // 0x43b1b0
    void next_used(unsigned* i);                       // 0x43b160
};
struct AgentTable {
    int count;                                         // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                         // +0x14 (first at +0x18; id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                       // 0x43b160
    int first_used();                                  // 0x43b1b0
    Agent* get(unsigned id);                           // 0x43b1f0
};
struct Agent {
    char pad0[4];
    int used;                                          // +4
    int id;                                            // +8
    char pad0c[4];
    int f10;                                           // +0x10
    int f14;                                           // +0x14
    char pad18[0x50 - 0x18];
    vector4 pos;                                       // +0x50
    char pad60[0x1c4 - 0x60];
    int team;                                          // +0x1c4
    char pad1c8[0x53ec - 0x1c8];
    Threat threats[16];                                // +0x53ec (sorted by distance, nearest first)
    int nthreat;                                       // +0x556c
    char pad5570[0x55e8 - 0x5570];
    int f55e8;                                         // +0x55e8 (target's "weight")
    char pad55ec[0x5928 - 0x55ec];
    void collect_enemies(int);                         // 0x43b300 (thiscall, ret 4: one unused stack argument)
    unsigned pick_target();                                 // 0x43b4d0 (fastcall)
};
// @ 0x43b160
void AgentTable::next_used(unsigned* i)
{
    unsigned v = ++*i;
    if ((int)v < count) {
        do {
            if (pages[v >> 4][v & 0xf].used != 0) break;
        } while ((int)(*i = ++v) < count);
    }
}
// @ 0x43b1b0
int AgentTable::first_used()
{
    unsigned v = 0;
    if (0 < count) {
        Agent** pg = &pages[0];
        do {
            if (pg[v >> 4][v & 0xf].used != 0) break;
            v++;
        } while ((int)v < count);
    }
    return v;
}
// @ 0x43b1f0
Agent* AgentTable::get(unsigned id)
{
    return &pages[id >> 4][id & 0xf];
}

// ---- VC7.1 <vector> helpers -----------------------------------------------------------------------------------------
struct vecstr : allocstr { tstring* first; tstring* last; tstring* end; tstring* ufill(tstring* f, unsigned n, const tstring& v); };   // 0x43b2b0
struct vecE1c8 : allocE1c8 { E1c8* first; E1c8* last; E1c8* end; E1c8* ucopy(E1c8* f, E1c8* l, E1c8* d); };                             // 0x43b2e0
// @ 0x43b2b0
tstring* vecstr::ufill(tstring* f, unsigned n, const tstring& v)
{
    ptr_tag t;
    uninit_fill_n_str(f, n, v, *this, t);
    return f + n;
}
// @ 0x43b2e0
E1c8* vecE1c8::ucopy(E1c8* f, E1c8* l, E1c8* d) { ptr_tag t; return uninit_copy1c8(f, l, d, *this, t); }

// ---- int hash buckets (bucket i spans buckets[i] .. buckets[i+1] in the node list) ---------------------------------
struct HNode { HNode* next; int pad4; int key; };
struct HIter {                                         // non-POD (user ctors), so it is returned through a hidden pointer
    HNode* p;
    HIter() {}
    HIter(HNode* q) : p(q) {}
    HIter(const HIter& o) : p(o.p) {}
    bool operator==(const HIter& o) const { return p == o.p; }
};
struct IntHash {
    char pad0[8];
    HNode* end_;                                       // +8 (list sentinel)
    char pad0c[8];
    HNode** buckets;                                   // +0x14
    char pad18[8];
    unsigned mask;                                     // +0x20
    unsigned maxidx;                                   // +0x24
    HIter end() const { return HIter(end_); }
    HIter find(const int& key);                        // 0x43b210 (ret 8: hidden result pointer + key)
};
// @ 0x43b210
HIter IntHash::find(const int& key)
{
    int k = key;
    unsigned b = (k ^ 0xdeadbeef) & mask;
    if (maxidx <= b) b += (0xffffffffu - (mask >> 1));
    HIter w(buckets[b]);
    HIter hi(buckets[b + 1]);
    for (; !(w == hi); w.p = w.p->next) {
        if (!(w.p->key < k)) return (k < w.p->key) ? end() : w;
    }
    return end();
}

// ---- per-agent list of the 16 nearest enemies, sorted by distance (records of 0x18 bytes at agent+0x53ec) ---------------
struct Mission {
    int f0;
    char pad4[0x18 - 4];
    std::vector<Agent*> agents;                        // +0x18 (page table; the AgentTable at +4 has the same vector at +0x14)
    AgentTable* table() { return (AgentTable*)((char*)this + 4); }
    char pad28[0x53c8 - 0x28];
    int relation[8 * 0x5b];                            // +0x53c8 (team x team, stride 0x5b)
    bool hostile(int a, int b)
    {
        if (a < 0 || a >= 8 || b < 0 || b >= 8) return false;
        return relation[a * 0x5b + b] == -1;
    }
};
extern Mission* g_mission;                             // 0x5e292c
extern "C" double __cdecl sqrt(double);

static __forceinline bool hostile_to(Mission* m, int a, Agent* o)
{
    if (a < 0 || a >= 8) return false;
    int b = o->team;
    if (b < 0 || b >= 8) return false;
    return m->relation[a * 0x5b + b] == -1;
}
// @ 0x43b300
void Agent::collect_enemies(int)
{
    nthreat = 0;
    Mission* m = g_mission;
    AgentTableExt* t = (AgentTableExt*)m->table();
    unsigned i = t->first_used();
    if (i < (unsigned)t->count) {
        do {
            Agent* o = &m->agents[i >> 4][i & 0xf];
            if (o->f10 == 0 && o->f14 == 1 && (int)i != id && hostile_to(m, team, o)) {
                vector4 d;
                d.x = o->pos.x - pos.x;
                d.y = o->pos.y - pos.y;
                d.z = o->pos.z - pos.z;
                float len = d.normalize();
                int k = 0;
                int n = nthreat;
                if (0 < n) {
                    while (k < nthreat) {
                        if (!(threats[k].dist < len)) break;
                        k++;
                    }
                    if (k >= 16) goto next;
                }
                if (n < 16) nthreat = n + 1;
                for (int j = nthreat - 1; j > k; j--) threats[j] = threats[j - 1];
                threats[k].id = o->id;
                threats[k].dist = len;
                threats[k].dir = d;
                m = g_mission;
            }
        next:
            t = (AgentTableExt*)m->table();
            t->next_used(&i);
        } while (i < (unsigned)t->count);
    }
}

// @ 0x43b4d0
unsigned Agent::pick_target()
{
    int n = nthreat;
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned id = (unsigned)-1;
    if (0 < n) {
        Threat* r = threats;
        Agent** pg = &g_mission->agents[0];
        do {
            id = r->id;
            if (pg[id >> 4][id & 0xf].f14 == 1) {
                const vector4& p = pg[id >> 4][id & 0xf].pos;
                float dx = p.x - pos.x;
                float dy = p.y - pos.y;
                float dz = p.z - pos.z;
                float v = 3.0f / (float)sqrt(dy * dy + (dx * dx + dz * dz));
                if (!(v < 4.0f)) v = 4.0f;
                v = v / ((float)pg[id >> 4][id & 0xf].f55e8 + 7.0f);
                if (v > bestv) { bestv = v; best = id; }
            }
            r++;
            n--;
            id = best;
        } while (n != 0);
    }
    return id;
}

// inline vector helpers of the original TU (rgl's operators are out-of-line in other TUs)
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 vneg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }
static __forceinline float vdot(const vector4& a, const vector4& b) { return a.x * b.x + (a.y * b.y + a.z * b.z); }
static __forceinline vector4 vmul(const vector4& a, float s) { return vector4(a.x * s, a.y * s, a.z * s); }

// @ 0x43ac40
void Scene::light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale)
{
    *out_dir = vector4(0.0f, 0.0f, 0.0f);
    *out_col = vector4(0.0f, 0.0f, 0.0f);
    Light** it = lights_first;
    if (it < lights_last) {
        unsigned skip_dir = flags & 2;
        do {
            if (skip_dir && (*it)->type == 2) goto next;
            if ((flags & 4) && (*it)->type != 2) goto next;
            {
                Light* l = *it;
                int type = l->type;
                double dist;
                vector4 d;
                float k;
                if (type == 2) {
                    dist = 1000.0f;
                    d = l->dir;
                    k = scale;
                } else {
                    d = vsub(p, l->pos);
                    dist = d.normalize();
                    k = 1.0f;
                }
                float dp = vdot(n, vneg(d));
                if (0.0f < dp && (type == 2 || dist < l->range)) {
                    vector4 hp = l->pos;
                    if (type == 2) {
                        vector4 t = vmul(d, (float)dist);
                        hp = vsub(p, t);
                    } else {
                        k = (float)(1.0 / (dist * dist) * k);
                    }
                    bool nothing = (flags == 0);
                    if (nothing) {
                        vector4 p0 = vsub(p, vmul(d, 0.05f));
                        if (!line_clear(&p0, &hp, 150.0f, 0, 0)) goto next;
                    }
                    l = *it;
                    if (l->flags & 1) k = k * 0.5f;
                    float t = dp * 0.9f + 0.1f;
                    float s = k * t;
                    float cr = s * l->r;
                    float cg = s * l->g;
                    float cb = s * l->b;
                    out_col->x = cr + out_col->x;
                    out_col->y = cg + out_col->y;
                    out_col->z = cb + out_col->z;
                    l = *it;
                    float br = (l->b + l->g) + l->r;
                    float ax = (d.x * k) * t;
                    float ay = (d.y * k) * t;
                    float az = (d.z * k) * t;
                    float bx = ax * 0.33f;
                    float by = ay * 0.33f;
                    float bz = az * 0.33f;
                    out_dir->x = bx * br + out_dir->x;
                    out_dir->y = by * br + out_dir->y;
                    out_dir->z = br * bz + out_dir->z;
                }
            }
        next:
            it++;
        } while (it < lights_last);
    }
    float len = (float)sqrt(out_dir->x * out_dir->x + (out_dir->y * out_dir->y + out_dir->z * out_dir->z));
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        out_dir->x = inv * out_dir->x;
        out_dir->y = inv * out_dir->y;
        out_dir->z = inv * out_dir->z;
    } else {
        out_dir->x = 0.0f; out_dir->y = 1.0f; out_dir->z = 0.0f;
    }
}
