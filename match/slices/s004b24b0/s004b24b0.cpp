// Slice s004b24b0: the "order" objects the script engine queues on a party (position and party/agent
// chase orders), the party's per-tick map refresh, and the 0xc-byte element / 0x5c-byte item-array
// copy helpers they are built from.
//
//   0x4b24b0  _Uninit_fill_n over 0xc-byte elements (dst, count by value; 5th pushed arg is the ptr_tag)
//   0x4b2520  std::_Push_heap sift-up for a 0xc-byte element (min-heap on the float at +8; comp by value)
//   0x4b25a0  fill range by copying elements (jumps to 0x4b2180)
//   0x4b25c0  Party::speed_factor from the party leader's skill 0xd (0x4b2c00.. wraps it in the script engine)
//   0x4b2610  std::__pop_heap_aux: *result = *first; adjust_heap(first, 0, last - first, value, comp)
//   0x4b2670  _Uninit_copy over the 0x5c-byte item records (through 0x4b2160)
//   0x4b26e0  ElemCopier member: uninitialized_fill_n(dst, count, val) and the end pointer
//   0x4b2710  std::push_heap tail: sift the last element up
//   0x4b2770  Rec98::bind_host  (the 0x98-byte map-agent record: agent under the position, height, clock stamp,
//             "Ship position" sanity checks)
//   0x4b2880  Party::place_on_map (agent under the party position, height, record mask, last-position copies)
//   0x4b2970/0x4b29d0/0x4b2ab0  Party::set_order / set_order_party / set_order_agent
//
// The party layout follows s004b3ce0 / s004b3420 (the order planner that consumes these fields): +0x30
// position (x, y, z, w), +0x5974 current map agent, +0x5978 map-record mask, +0x5998 order kind,
// +0x599c/+0x59a0/+0x59a4 order position and agent, +0x59a8 target party, +0x5984 current map-agent record,
// +0x5988 "needs repath" flag.  (An earlier version of this slice put the position at +0x20 and the
// agent at +0x30; the 0x4b2770 record is a different class, Rec98.)
#include <math.h>
#include <vector>
#include "rgl.h"

struct ptr_tag {};   // empty class passed by value (one 4-byte slot); same as src/include/ptr_tag.h

using rgl::vector4;

// ---- engine helpers -----------------------------------------------------------------------------------------
void __cdecl log_msg(const char* msg);                   // 0x42cb80  log a fixed message
void __cdecl log_fmt(const char* fmt, ...);              // 0x42cc80  sprintf-style log helper
void __stdcall throw_exception(void*, void*);            // 0x5368e6  _CxxThrowException
struct TickPair { int lo, hi; };
extern TickPair g_ticks[];                               // 0x5e2948  100 kHz clock entries (two words each)
extern float g_gk3;                                      // 0x54a710  3.0f
extern float g_gk0_05a;                                  // 0x549d74
extern float g_gk0_05b;                                  // 0x549d7c
extern float g_gk0_03;                                   // 0x549d70
extern float g_skill_speed_k;                            // 0xa18114

// ---- the 0xc-byte element and the 0x5c-byte item record ------------------------------------------------------
struct Elem12 { int a, b; float c; };
typedef char assert_elem12[sizeof(Elem12) == 0xc ? 1 : -1];

struct ElemCopier {                                     // "ctx" object: __thiscall copy
    void copy(Elem12* dst, Elem12* src);                // 0x436cf0
    void copy_items(void* dst, void* src);              // 0x4b2160
    Elem12* uninitialized_copy_n(Elem12* dst, unsigned count, Elem12* val);   // 0x4b26e0
    Elem12* fill_elems(Elem12* first, Elem12* last, Elem12* dst);             // 0x4b25a0
};

struct ItemRec {                                        // 0x5c-byte item record
    int type;                                           // +0
    int count;                                          // +4
    char pad8[0x5c - 8];
};
typedef char assert_itemrec[sizeof(ItemRec) == 0x5c ? 1 : -1];

void __stdcall copy_item(void* dst, void* src);         // 0x4b2160
Elem12* construct_items_old(Elem12* first, Elem12* last, Elem12* dst, ElemCopier* ctx, ptr_tag tag);  // 0x4b2180
// empty comparator functor passed by value (one 4-byte stack slot): min-heap on the float at +8
struct HeapGt { bool operator()(const Elem12& a, const Elem12& b) const { return a.c > b.c; } };
void adjust_heap(Elem12* first, int hole, int len, Elem12 value, HeapGt comp);       // 0x438b10
void construct_elems2(Elem12* dst, unsigned count, Elem12* val, ElemCopier* ctx, ptr_tag tag);

// @ 0x4b24b0
// std::uninitialized_fill_n(dst, count, *val): the original takes the pointer and count by value (they are
// advanced in their stack slots because the loop sits in a try block); a 5th pushed argument is unused.
void construct_elems2(Elem12* dst, unsigned count, Elem12* val, ElemCopier* ctx, ptr_tag tag)
{
    Elem12* s = dst;
    try {
        for (; 0 < count; --count, ++dst) ctx->copy(dst, val);
    } catch (...) {
        for (; s != dst; ++s) ;
        throw;
    }
}

// @ 0x4b2514
void throw_stub()
{
    throw_exception(0, 0);
    __assume(0);
}

// ---- engine objects -----------------------------------------------------------------------------------------
struct WorldAgent {                                     // 0x48ea80 result (the map agent / anim node)
    char pad0[4];
    int rec;                                            // +4  0x88-byte map record index
    char pad8[0x20 - 8];
    float x;                                            // +0x20
    float y;                                            // +0x24
    float z;                                            // +0x28
    char pad2c[0xa0 - 0x2c];
    float fa0;                                          // +0xa0
    float fa4;                                          // +0xa4
    float fa8;                                          // +0xa8
};

struct BigRec { char pad0[0x24]; int mask; char pad28[0x88 - 0x28]; };   // 0x88-byte map record
struct WorldMap {
    char pad0[0x14];
    BigRec* recs;                                       // +0x14
};
struct MapGlobal {                                      // object at 0xa732b4
    char pad0[0xb0];
    WorldMap* world;                                    // +0xb0
};
extern MapGlobal* g_scene;                              // 0xa732b4

struct tstring {                                        // the engine's string: c_str() is the inline pointer slot when empty
    char* p;
    const char* c_str() const { return p ? p : (const char*)&p; }
};

// 0x98-byte map-agent record (the mission's page table of army/settlement records, see s0049b630 Rec98)
struct Rec98 {
    char pad0[0x10];
    int type;                                           // +0x10
    char pad14[0x24 - 0x14];
    float x1, y1;                                       // +0x24 / +0x28  first position
    float x2, y2;                                       // +0x2c / +0x30  second position
    char pad34[0x3c - 0x34];
    float z;                                            // +0x3c
    char pad40[0x44 - 0x40];
    float f44;                                          // +0x44
    WorldAgent* agent1;                                 // +0x48
    WorldAgent* agent2;                                 // +0x4c
    char pad50[0x60 - 0x50];
    int clock;                                          // +0x60
    char pad64[0x68 - 0x64];
    int tick_lo, tick_hi;                               // +0x68 / +0x6c
    float f70;                                          // +0x70
    float f74;                                          // +0x74
    char pad78[0x90 - 0x78];
    int f90;                                            // +0x90
    char pad94[0x98 - 0x94];

    void bind_host();                                   // 0x4b2770
};

// the 0x5ac8-byte party record (layout as in s004b3ce0 / s004b3420)
struct Party {
    char pad0[0x10];
    tstring name;                                       // +0x10
    char pad14[0x30 - 0x14];
    vector4 pos;                                        // +0x30  map position
    char pad40[0x95c - 0x40];
    int f95c, f960, f964, f968;                         // +0x95c.. (float x/y pairs copied as raw words)
    char pad96c[0x5974 - 0x96c];
    WorldAgent* mesh;                                   // +0x5974  current map agent
    unsigned group;                                     // +0x5978  mask of the map record under the party
    float f597c;                                        // +0x597c
    int moving;                                         // +0x5980
    int f5984;                                          // +0x5984  map record index the party is on
    int f5988;                                          // +0x5988  "needs repath" flag
    char pad598c[0x5990 - 0x598c];
    float f5990;                                        // +0x5990
    float f5994;                                        // +0x5994
    int kind;                                           // +0x5998  order kind
    int target_pos[2];                                  // +0x599c / +0x59a0
    WorldAgent* tgt;                                    // +0x59a4
    int target;                                         // +0x59a8
    char pad59ac[0x59c8 - 0x59ac];
    vector4 pos59c8;                                    // +0x59c8  copy of the position
    char pad59d8[0x5ac8 - 0x59d8];

    int skill(int skill);                               // 0x4b2370
    float speed_factor();                               // 0x4b25c0
    void place_on_map();                                // 0x4b2880
    void set_order(int* pos);                           // 0x4b2970
    void set_order_party(int* t);                       // 0x4b29d0
    void set_order_agent(int* t);                       // 0x4b2ab0
};
typedef char assert_party[sizeof(Party) == 0x5ac8 ? 1 : -1];

struct PartyMgr {                                       // g_game + 0xf8
    char pad0[0x138cc];
    std::vector<char*> pages;                           // +0x138cc (begin pointer at +0x138d0)
    int index_of(int id);                               // 0x48ad00 (thiscall, ret 4)
    Party* get(int id);
};
inline Party* party_at(PartyMgr* m, int k)
{
    return (Party*)((k & 0xf) * 0x5ac8 + *(m->pages.begin() + (k >> 4)));
}
inline Party* party_of(PartyMgr* m, int id) { return party_at(m, m->index_of(id)); }


struct GameData {
    char pad0[0xf8];
    PartyMgr parties;                                   // +0xf8
    char pad_[0x13ad0 - 0xf8 - sizeof(PartyMgr)];
    bool not_afternoon();                               // 0x435740
    WorldAgent* find_at(const float* p);                // 0x48ea80
};
extern GameData* g_game;                                // 0x5e2930

// @ 0x4b2520
// std::_Push_heap (min-heap on the float at +8): sift `value` up from `hole` while the parent is greater.
// The comparator is an empty functor passed by value (one 4-byte slot).
void push_heap_elem(Elem12* first, int hole, int top, Elem12 value, HeapGt pred)
{
    for (int i = (hole - 1) / 2; top < hole && pred(first[i], value); i = (hole - 1) / 2) {
        first[hole] = first[i];
        hole = i;
    }
    Elem12* d = &first[hole];
    d->a = value.a;
    d->b = value.b;
    d->c = value.c;
}

// @ 0x4b25a0
// ElemCopier member (this = ctx): std::_Uninit_copy over the 0xc-byte elements; the 5th pushed argument is the
// uninitialised ptr_tag (eax at the call).
Elem12* ElemCopier::fill_elems(Elem12* first, Elem12* last, Elem12* dst)
{
    ptr_tag tag;
    return construct_items_old(first, last, dst, this, tag);
}

// @ 0x4b2610
// std::__pop_heap_aux: *result = *first; adjust_heap(first, 0, last - first, value, comp)
void pop_heap_elem(Elem12* first, Elem12* last, Elem12* result, Elem12 value, HeapGt comp)
{
    *result = *first;
    adjust_heap(first, 0, (int)(last - first), value, comp);
}

// @ 0x4b2670
ItemRec* construct_items(ItemRec* first, ItemRec* last, ItemRec* dst, ElemCopier* ctx, ptr_tag tag)
{
    ItemRec* s = dst;
    try {
        for (; first != last; ++dst, ++first) ctx->copy_items(dst, first);   // 0x4b2160
    } catch (...) {
        for (; s != dst; ++s) ;
        throw;
    }
    return dst;
}

// @ 0x4b26d6
void throw_stub2()
{
    throw_exception(0, 0);
    __assume(0);
}

// @ 0x4b26e0
// ElemCopier member (this = ctx): uninitialized_fill_n(dst, count, val) and the end pointer
Elem12* ElemCopier::uninitialized_copy_n(Elem12* dst, unsigned count, Elem12* val)
{
    ptr_tag tag;   // uninitialised: pushes whatever eax holds (val)
    construct_elems2(dst, count, val, this, tag);
    return dst + count;
}

// @ 0x4b2710
// std::push_heap tail: sift up the last element
void push_back_elem(Elem12* first, Elem12* last, HeapGt comp)
{
    if (first != last) {
        Elem12* l = last - 1;
        int n = (int)(l - first);
        if (n > 0) {
            push_heap_elem(first, n, 0, *l, comp);
        }
    }
}

// Re-seat a map-agent record on the world map: agent under the first position, its height above the
// agent's plane, the clock stamp, and (type 2) a sanity check of the record masks under both positions.
// @ 0x4b2770
void Rec98::bind_host()
{
    agent1 = g_game->find_at(&x1);
    z = agent1->z - ((x1 - agent1->x) * agent1->fa0 + (y1 - agent1->y) * agent1->fa4) / agent1->fa8;
    int clk = clock;
    tick_lo = g_ticks[clk].lo;
    tick_hi = g_ticks[clk].hi;
    f70 = 1.0f;
    f44 = -10000.0f;
    f74 = 0.0f;
    f90 = -1;
    if (type == 2) {
        int m = g_scene->world->recs[agent1->rec].mask;
        if ((m & 7) != 0 && m != 7) {
            log_msg("Ship position not in water");
        }
        WorldAgent* b = g_game->find_at(&x2);
        agent2 = b;
        m = g_scene->world->recs[b->rec].mask;
        if (((m & 7) < 2 || (m & 7) >= 7) && m != 7) {
            log_msg("Ship land position not on land");
        }
    }
}

// @ 0x4b25c0
float Party::speed_factor()
{
    float v = (skill(0xd) * g_gk0_05b + 1.0f) * g_skill_speed_k;
    if (g_game->not_afternoon()) {
        return g_gk0_03 * v;
    }
    return v;
}

// Place the party on the map: agent under (x, y), height z over the agent's plane, record mask, and the
// "last position" copies.
// @ 0x4b2880
void Party::place_on_map()
{
    float* p = &pos.x;
    mesh = g_game->find_at(p);
    if (!mesh) {
        log_fmt("Unable to position party %s at (%2.2f, %2.2f).", name.c_str(), (double)pos.x, (double)pos.y);
    }
    WorldAgent* m = mesh;
    pos.z = m->z - ((p[1] - m->y) * m->fa4 + (p[0] - m->x) * m->fa0) / m->fa8;
    group = g_scene->world->recs[m->rec].mask;
    int x = ((int*)p)[0];
    int y = ((int*)p)[1];
    f964 = x;
    f968 = y;
    f95c = x;
    f960 = y;
    f5984 = -1;
    f597c = 5.0f;
    pos59c8 = *(vector4*)p;
}

// @ 0x4b2970
void Party::set_order(int* pos)
{
    kind = 1;
    target_pos[0] = pos[0];
    target_pos[1] = pos[1];
    tgt = g_game->find_at((const float*)pos);
    if (f5984 != tgt->rec) {
        moving = 0;
        f5988 = 1;
    }
}

// @ 0x4b29d0
void Party::set_order_party(int* t)
{
    kind = 2;
    target = t[0];
    PartyMgr* mgr = &g_game->parties;
    if (f5984 != party_of(mgr, t[0])->mesh->rec) {
        const vector4* pp = &party_of(mgr, target)->pos;
        float dx = pp->x - pos.x;
        float dy = pp->y - pos.y;
        float dz = pp->z - pos.z;
        moving = 0;
        if (sqrtf(dx * dx + dy * dy + dz * dz) > g_gk3) {
            f5988 = 1;
        }
    }
}

// @ 0x4b2ab0
void Party::set_order_agent(int* t)
{
    kind = 3;
    int id = t[0];
    target = id;
    PartyMgr* mgr = &g_game->parties;
    const vector4* pp = &party_of(mgr, id)->pos;
    vector4 d;
    d.x = pp->x - pos.x;
    d.y = pp->y - pos.y;
    d.z = pp->z - pos.z;
    const vector4* qq = &party_of(mgr, id)->pos;
    float ex = qq->x - f5990;
    float ey = qq->y - f5994;
    if (ex * ex + ey * ey > g_gk0_05a) {
        moving = 0;
        if (d.z * d.z + d.y * d.y + d.x * d.x > g_gk0_05a) {
            f5988 = 1;
        }
        vector4& rp = party_of(&g_game->parties, target)->pos;
        f5990 = rp.x;
        f5994 = rp.y;
    }
}

