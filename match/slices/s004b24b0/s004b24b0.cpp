// Slice s004b24b0: the "order" objects the script engine queues on a party (position and party/agent
// chase orders), the party's per-tick map refresh, and the 0xc-byte element / 0x5c-byte item-array
// copy helpers they are built from.
//
//   0x4b25c0 + 0x4b2c00..0x4b32e1  the script-side "agent N" wrapper (out of slice range, see below)
//   0x4b2520  heap sift for a 0xc-byte vector element (comp is passed on the stack)
//   0x4b25a0  fill range by copying elements (calls 0x4b2180)
//   0x4b25c0  agent speed factor from the party leader's skill 0xd
//   0x4b2610  insert a range of 0xc-byte elements at a position
//   0x4b2670  construct_items (0x5c-byte records through 0x4b2160)
//   0x4b26e0  uninitialized_copy_n over 0xc-byte elements
//   0x4b2710  push_back through the heap sift
//   0x4b2770/0x4b2880  Party::update_2 / update_3  (map agent, anim record, clock deadlines)
//   0x4b2970/0x4b29d0/0x4b2ab0  Party::set_order / set_order_party / set_order_agent
//
// The party layout follows s004b3ce0 (the order planner that consumes these fields): +0x20 position,
// +0x48 the 0x88-byte map record, +0x5998 order kind, +0x59a0/+0x59a4 chase target,
// +0x5984 current map-agent record, +0x5988 "in range" flag.
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- engine helpers -----------------------------------------------------------------------------------------
void __cdecl log_fmt(const char* fmt, ...);              // 0x42cb80 / 0x42cc80  sprintf-style log helpers
void __stdcall throw_exception(void*, void*);            // 0x5368e6  _CxxThrowException
extern __int64 g_ticks[];                                // 0x5e2948  100 kHz clock entries (two words each)
extern float g_gk3;                                      // 0x54a710  3.0f
extern float g_gk0_01;                                   // 0x54a6bc
extern float g_gk0_05a;                                  // 0x549d74
extern float g_gk0_05b;                                  // 0x549d7c
extern float g_gk0_03;                                   // 0x549d70
extern float g_skill_speed_k;                            // 0xa18114
extern int g_u_a181a8;                                   // 0xa181a8
extern float g_zoom_decay;                               // 0x54b8b4
extern float g_vital_decay;                              // 0x590c78
extern int g_clamp_max;                                  // 0x8ee3ac4
extern float g_k1;                                       // 0x549cdc
extern float g_k0;                                       // 0x549ca8
extern float g_k0_1;                                     // 0x54b8b0
extern float g_gk5;                                      // 0x54a704
extern float g_gk_mul;                                   // 0x54a6e4

// ---- the 0xc-byte element and the 0x5c-byte item record ------------------------------------------------------
struct Elem12 { int a, b; float c; };
typedef char assert_elem12[sizeof(Elem12) == 0xc ? 1 : -1];

struct ElemCopier {                                     // "ctx" object: __thiscall copy
    void copy(Elem12* dst, Elem12* src);                // 0x436cf0
    void copy_items(void* dst, void* src);              // 0x4b2160
};

struct ItemRec {                                        // 0x5c-byte item record
    int type;                                           // +0
    int count;                                          // +4
    char pad8[0x5c - 8];
};
typedef char assert_itemrec[sizeof(ItemRec) == 0x5c ? 1 : -1];

void __stdcall copy_item(void* dst, void* src);         // 0x4b2160
Elem12* construct_items_old(Elem12* first, Elem12* last, Elem12* dst, ElemCopier* ctx);  // 0x4b2180
void construct_elems(Elem12*& first, unsigned count, Elem12*& dst, ElemCopier* ctx);
void construct_elems2(Elem12*& first, unsigned count, Elem12*& dst, ElemCopier* ctx);
Elem12* fill_elems(Elem12* first, Elem12* last, Elem12* dst, ElemCopier* ctx);

// @ 0x4b24b0
void construct_elems2(Elem12*& first, unsigned count, Elem12*& dst, ElemCopier* ctx)
{
    try {
        while (count > 0) {
            ctx->copy(dst, first);
            count--;
            first++;
            dst++;
        }
    } catch (...) {
    }
}

// @ 0x4b2514
void throw_stub()
{
    throw_exception(0, 0);
    __assume(0);
}

// ---- engine objects -----------------------------------------------------------------------------------------
struct CharRec {
    char pad0[0x30];
    unsigned __int64 flags;                             // +0x30
    char pad38[0x50 - 0x38];
    int f50;                                            // +0x50
    char pad54[0x7a - 0x54];
    unsigned char f7a;                                  // +0x7a (food level clamp)
    char pad7b[0x9c - 0x7b];
    int f9c;                                            // +0x9c
    float fa0;                                          // +0xa0 (food)
    char pada4[0x508 - 0xa4];
};

struct AnimObj {                                        // 0x48ea80 result (map agent / anim node)
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

struct Scene {
    char pad0[0xb0];
    void* anims;                                        // +0xb0 (0x88-byte record store)
};
extern Scene* g_scene;                                  // 0xa732b4
struct Rec88 { char pad0[0x24]; unsigned mask; char pad28[0x88 - 0x28]; };

struct PartyRec {                                       // one 0x5ac8-byte party record
    char pad0[0x30];
    AnimObj* anim;                                      // +0x30  map agent of this party
    char pad34[0x5974 - 0x34];
    int f5974;                                          // +0x5974
    char pad5978[0x5ac8 - 0x5978];
};
typedef char assert_partyrec[sizeof(PartyRec) == 0x5ac8 ? 1 : -1];

struct PartyPool {
    char pad0[0x138d0];
    PartyRec** recs;                                    // +0x138d0

    PartyRec* get(int handle);                          // 0x48ad00 + record lookup
};

struct Party {
    char pad0[8];
    int id;                                             // +0x8
    char padc[0x10 - 0xc];
    int type;                                           // +0x10
    char pad14[0x20 - 0x14];
    vector4 pos;                                        // +0x20  map position
    AnimObj* agent;                                     // +0x30  current chase target
    char pad34[0x40 - 0x34];
    float f40;                                          // +0x40
    float f44;                                          // +0x44
    Rec88* f48;                                         // +0x48
    float f4c;                                          // +0x4c
    vector4 f50;                                        // +0x50  direction
    int f60;                                            // +0x60  clock index
    char pad64[0x68 - 0x64];                            // +0x64 (route vector, unused here)
    int f68;                                            // +0x68
    int f6c;                                            // +0x6c
    float f70;                                          // +0x70
    float f74;                                          // +0x74
    vector4 pos78;                                      // +0x78
    int f88;                                            // +0x88
    char pad8c[0x90 - 0x8c];
    int f90;                                            // +0x90
    char pad94[0xa0 - 0x94];
    unsigned __int64 flagsa0;                           // +0xa0
    char pada8[0x8a8 - 0xa8];
    int inv_count;                                      // +0x8a8
    float fac;                                          // +0x8ac
    float f8b0;                                         // +0x8b0
    char pad8b4[0x5980 - 0x8b4];
    int moving;                                         // +0x5980
    int f5984;                                          // +0x5984
    int f5988;                                          // +0x5988
    char pad598c[0x5990 - 0x598c];
    float f5990;                                        // +0x5990
    float f5994;                                        // +0x5994
    int kind;                                           // +0x5998
    int target_pos[2];                                  // +0x599c / +0x59a0
    AnimObj* tgt;                                       // +0x59a4
    int target;                                         // +0x59a8
    char pad59ac[0x59c0 - 0x59ac];
    void* f59c0;                                        // +0x59c0

    int skill(int skill);                               // 0x4b2370
    int total_value(unsigned flags);                    // 0x4b1cf0
    void update_2();                                    // 0x4b2770
    void update_3();                                    // 0x4b2880
    void set_order(int* pos);                           // 0x4b2970
    void set_order_party(int* t);                       // 0x4b29d0
    void set_order_agent(int* t);                       // 0x4b2ab0
};

struct GameData {
    char pad0[0x78];
    int main_party;                                     // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                         // +0x8c
    char pad90[0xf8 - 0x90];
    PartyPool parties;                                  // +0xf8
    char padfc[0x13a00 - 0x138d4];
    CharRec* chars;                                     // +0x13a00
    char pad13a04[0x13ad0 - 0x13a04];
    float zoom;                                         // +0x13ad0
    int f13ad4;                                         // +0x13ad4

    int party_handle(int id);                           // 0x48ad00
    bool not_afternoon();                               // 0x435740
    AnimObj* find_at(const float* p);                   // 0x48ea80
};
extern GameData* g_game;                                // 0x5e2930

// @ 0x4b2520
// Generator-generated heap sift for a vector of 0xc-byte elements.  The caller passes
// (first, hole, last, value, comparator) and the comparator lives at [esp+0x24]; the sift compares the
// elements' last float and walks parents up while the hole is not empty.
void push_heap_elem(Elem12* first, int hole, int last, Elem12 value, const void* comp)
{
    (void)comp;
    int top = (hole - 1) / 2;
    Elem12 tmp;
    while (last < hole) {
        if (((const float*)first)[top * 3 + 2] <= value.c) {
            break;
        }
        tmp = first[top];
        first[hole] = tmp;
        hole = top;
        top = (hole - 1) / 2;
    }
    first[hole] = value;
}

// @ 0x4b25c0
float agent_speed_factor(Party* party)
{
    float v = (party->skill(0xd) * g_gk0_05b + 1.0f) * g_skill_speed_k;
    if (!g_game->not_afternoon()) {
        return v;
    }
    return v * g_gk0_03;
}

// @ 0x4b25a0
Elem12* fill_elems(Elem12* begin, Elem12* last, Elem12* dst, ElemCopier* ctx)
{
    return construct_items_old(begin, last, dst, ctx);
}

// @ 0x4b2610
Elem12* insert_elems(Elem12* pos, Elem12* first, Elem12* last, Elem12 value, void* ctx)
{
    *pos = value;
    construct_elems(first, (unsigned)((last - first) / 3), pos, (ElemCopier*)ctx);
    return pos;
}

// @ 0x4b2670
ItemRec* construct_items(ItemRec* first, ItemRec* last, ItemRec* dst, ElemCopier* ctx)
{
    try {
        while (first != last) {
            ctx->copy_items(dst, first);                // 0x4b2160
            dst++;
            first++;
        }
    } catch (...) {
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
Elem12* uninitialized_copy_n(Elem12* first, unsigned count, Elem12* dst, ElemCopier* ctx)
{
    Elem12* src = first;
    Elem12* out = dst;
    construct_elems2(src, count, out, ctx);
    return out + count;
}

// @ 0x4b2710
Elem12* push_back_elem(Elem12* first, Elem12* last, Elem12 value, const void* comp)
{
    (void)value;
    Elem12 tmp = last[-1];
    if (first != last) {
        push_heap_elem(first, (last - first) / 3, 0, tmp, comp);
    }
    return last;
}

// Advance the map agent / anim record / clock deadlines of a party.
// @ 0x4b2770
void Party::update_2()
{
    AnimObj* a = g_game->find_at(&pos.x);
    agent = a;
    f4c = a->z - ((pos.z - a->y) * a->fa4 + (pos.y - a->x) * a->fa0) / a->fa8;
    int clk = f60;
    f68 = (int)g_ticks[clk * 2];
    f6c = (int)(g_ticks[clk * 2] >> 32);
    f70 = 1.0f;
    f44 = -10000.0f;
    f74 = 0.0f;
    f90 = -1;
    if (type == 2) {
        Rec88* r = &((Rec88*)g_scene->anims)[agent->rec];
        if ((r->mask & 7) != 0 && r->mask != 7) {
            log_fmt((const char*)0x5935f4);
        }
        AnimObj* b = g_game->find_at(&pos78.x);
        tgt = b;
        unsigned m = ((Rec88*)g_scene->anims)[b->rec].mask;
        if (((m & 7) < 2 || (m & 7) >= 7) && m != 7) {
            log_fmt((const char*)0x5935d4);
        }
    }
}

// @ 0x4b2880
void Party::update_3()
{
    AnimObj* a = g_game->find_at(&pos.x);
    agent = a;
    if (!a) {
        log_fmt((const char*)0x593610, pos.x, pos.y);
    }
    float ax = a->x, ay = a->y, az = a->z;
    f4c = az - ((pos.z - ay) * a->fa4 + (pos.y - ax) * a->fa0) / a->fa8;
    f40 = ((pos.x - ax) * a->fa0 + (pos.y - ay) * a->fa4) / a->fa8;
    f5990 = pos.x;
    f5994 = pos.y;
    f48 = (Rec88*)g_scene->anims + a->rec;
    f44 = -1.0f;
    f70 = 5.0f;
    int v = (int)f40;
    f68 = v;
    f6c = (int)f5990;
    pos78.x = f5990;
    pos78.y = f5994;
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
    PartyRec* p = g_game->parties.get(t[0]);
    if (f5984 != p->anim->rec) {
        PartyRec* q = g_game->parties.get(target);
        AnimObj* b = q->anim;
        float dx = b->x - pos.x;
        float dy = b->y - pos.y;
        float dz = b->z - pos.z;
        moving = 0;
        if (!(sqrtf(dx * dx + dy * dy + dz * dz) >= g_gk3)) {
            f5988 = 1;
        }
    }
}

// @ 0x4b2ab0
void Party::set_order_agent(int* t)
{
    kind = 3;
    target = t[0];
    PartyRec* r1 = g_game->parties.get(target);
    AnimObj* a = r1->anim;
    float dx = a->x - pos.x;
    float dy = a->y - pos.y;
    float dz = a->z - pos.z;
    PartyRec* r2 = g_game->parties.get(target);
    AnimObj* b = r2->anim;
    float ex = (b->x - f5990) * (b->x - f5990);
    float ey = (b->y - f5994) * (b->y - f5994);
    if (!(ex + ey >= g_gk0_05a)) {
        moving = 0;
        if (!(dx * dx + dy * dy + dz * dz >= g_gk0_05a)) {
            f5988 = 1;
        }
        PartyRec* r3 = g_game->parties.get(target);
        f5990 = r3->anim->x;
        f5994 = r3->anim->y;
    }
}
