// Slice s004918d0: party/battle helpers. 0x4918d0 transfer check, 0x491b20 gate rating,
// 0x491e50 set position, 0x491f20 token lookup, 0x491f50..0x4921b0 battle (0x5928 records) helpers.
// Layouts recovered from this slice's disassembly.
#include <windows.h>
#include <stdlib.h>
#include <math.h>

// ---- shared records ---------------------------------------------------------------------------------------
struct TroopRec {                                // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    unsigned flags;                              // +0x30
    char pad34[0xa0 - 0x34];
    float hp;                                    // +0xa0
    char pada4[0xb0 - 0xa4];
    int value;                                   // +0xb0
    char padb4[0x508 - 0xb4];
};
struct Stack {                                   // 0x20 bytes, array at party+0xa8
    int count;                                   // +0x00
    int troop;                                   // +0x04
    int f08;                                     // +0x08
    int f0c;                                     // +0x0c
    int wounded;                                 // +0x10
    int pad14[3];
};
struct Party {                                   // 0x5ac8 bytes
    void* vtbl;                                  // +0x00
    int type;                                    // +0x04
    char pad8[0x30 - 0x8];
    float f30;                                   // +0x30
    float f34;                                   // +0x34
    char pad38[0x8c - 0x38];
    int owner;                                   // +0x8c
    char pad90[0xa0 - 0x90];
    unsigned f_a0;                               // +0xa0
    char pada4[0xa8 - 0xa4];
    Stack stacks[64];                            // +0xa8
    int nstacks;                                 // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];

    int add(int troop, int amount, int flag, int a4, int a5);  // 0x48c840 (ret 0x14)
    int sum(int what);                           // 0x4b1cf0 (ret 4)
    int find_stack(int amount, int what);        // 0x4b1d30 (ret 8)
    int f4894b0();                               // 0x4894b0 (ret)
    int f4894e0();                               // 0x4894e0 (ret)
    int f4b22d0();                               // 0x4b22d0 (ret)
    int f4b2330();                               // 0x4b2330 (ret)
    int f463190();                               // 0x463190 (ret)
    int f4b1700();                               // 0x4b1700 (ret)
    int f489510();                               // 0x489510 (ret)
    void f4b2880();                              // 0x4b2880 (ret)
    int f4b2370(int a);                          // 0x4b2370 (ret 4)
};

struct Mgr {                                     // game+0xf8
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    int count;                                   // +0x138dc
    char pad138e0[0x13908 - 0x138e0];
    TroopRec* chars;                             // +0x13908
    int index_of(int id);                        // 0x48ad00 (ret 4)
    bool is_slot(int id);                        // 0x4903e0 (ret 4)
    Party* at(int i) { return &pages[i >> 4][i & 0xf]; }
    Party* atu(unsigned i) { return &pages[i >> 4][i & 0xf]; }
};

struct World {                                   // global game object (0x5e2930)
    char pad0[0x78];
    int gate;                                    // +0x78
    char pad7c[0x8c - 0x7c];
    int player;                                  // +0x8c
    char pad90[0xf8 - 0x90];
    Mgr mgr;                                     // +0xf8
    char padA[0x1405c - 0x13a04];
    int arr1405c[1];                             // +0x1405c
    char padE[0x1a05c - 0x14060];
    char b1a05c;                                 // +0x1a05c

    void get_prisoners(int id, int* a, int* b, int* c, int* d);  // 0x490ce0 (ret 0x14)
    void dissolve(int id);                       // 0x4915d0 (ret 4)
    void pick_near(float x, float y, float radius, float* out);  // 0x490030 (ret 0x10)
    int token(int a, int b, int c, int d);       // 0x4902a0 (ret 0x10)
    bool agent_low(int idx);                     // 0x4638f0 (ret 4)

    bool can_transfer(unsigned id, int other, char flag);        // 0x4918d0 (ret 0xc)
    int gate_rating(unsigned p1, unsigned p2, int p3);           // 0x491b20 (ret 0xc)
    void set_pos(float x, float y, int id, float radius);        // 0x491e50 (ret 0x10)
    int token_lookup(int a, int b, int c, int d);                // 0x491f20 (ret 0x10)
};

// ---- battle (0x5928-record manager) ------------------------------------------------------------------------
struct BRec {                                    // 0x5928 bytes
    char pad0[0x4];
    int valid;                                   // +0x04
    char pad8[0x14 - 0x8];
    int type;                                    // +0x14
    char pad18[0x88 - 0x18];
    int f88;                                     // +0x88
    char pad8c[0x1c4 - 0x8c];
    int f1c4;                                    // +0x1c4
    char pad1c8[0x56d4 - 0x1c8];
    int f56d4;                                   // +0x56d4
    char pad56d8[0x5928 - 0x56d8];
};
struct MRec16c {                                 // 0x16c bytes, array at battle+0x53c8
    int f00, f04, f08, f0c, f10, f14, f18, f1c;
    char pad[0x16c - 0x20];
};
struct Hex18 {                                   // 0x18 bytes, array at battle+0x6220
    int v;                                       // +0x00
    char pad4[0x10];
    float f;                                     // +0x14
};
struct Pool {                                    // battle+0x04 (subobject passed in ecx)
    int count;                                   // +0x00
    unsigned first();                            // 0x43b1b0 (ret)
    void next(unsigned* p);                      // 0x43b160 (ret 4)
};
struct Mission3 {
    char pad0[0x4];
    Pool pool;                                   // +0x04
    char pad8[0x10 - 0x08];
    unsigned flags;                              // +0x10
    char pad14[0x1c - 0x14];
    BRec** pages;                                // +0x1c (index >> 4 -> page)
    char pad20[0x5c - 0x20];
    int cur;                                     // +0x5c
    char pad60[0x53c8 - 0x60];
    MRec16c rec8[8];                             // +0x53c8 (stride 0x16c)
    char padA[0x6220 - (0x53c8 + 8 * 0x16c)];
    Hex18 hex[8];                                // +0x6220 (stride 0x18)
    char padB[0x6520 - (0x6220 + 8 * 0x18)];
    int f6520;                                   // +0x6520

    MRec16c* recs() { return (MRec16c*)((char*)this + 0x53c8); }
    BRec* rec(unsigned i) { return &pages[i >> 4][i & 0xf]; }
    float speed_factor(float x);                 // 0x467740 (ret 4)
    int rating(int idx);                         // 0x491f50 (ret 4)
    bool check(int a);                           // 0x4920e0 (ret 4)
    bool is_active();                            // 0x492170 (ret)
    unsigned find(int id);                       // 0x4921b0 (ret 4)
};

extern __int64 g_5e2958;                         // 0x5e2958 (int64, fild qword)
extern int rand();

// @ 0x4918d0
bool World::can_transfer(unsigned id, int other, char flag) {
    if (id >= (unsigned)mgr.count) return false;
    int idx = mgr.index_of(id);
    if (idx < 0) return false;
    if (mgr.atu((unsigned)idx)->type != 1) return false;
    if (!mgr.is_slot(other)) return false;
    int ia = mgr.index_of(id);
    Party* a = mgr.at(ia);
    int ib = mgr.index_of(other);
    Party* b = mgr.at(ib);
    if (b->f_a0 & 0x100000) {
        if (!flag) {
            if (b->f4894b0() + a->f4894b0() > b->f4b22d0()) return false;
            if (b->f4894e0() + a->f4894e0() > b->f4b2330()) return false;
        } else {
            if (b->f4894b0() + a->f4894e0() > b->f4b22d0()) return false;
            if (b->f4894e0() + a->f4894b0() > b->f4b2330()) return false;
        }
    }
    int p190 = b->f463190();
    int d1 = b->f4b1700() - p190;
    int p510 = b->f489510();
    int d2 = b->f4b1700() - p510;
    int na = a->nstacks;
    int c1 = 0, c2 = 0;
    TroopRec* chars = mgr.chars;
    for (int k = 0; k < na; k++) {
        Stack* s = &a->stacks[k];
        bool matched = false;
        if (!(chars[s->troop].flags & 0x10)) {
            for (int j = 0; j < b->nstacks; j++) {
                if (b->stacks[j].troop == s->troop) {
                    if (!flag) {
                        if (((s->f08 ^ b->stacks[j].f08) & 1) == 0) matched = true;
                    } else {
                        if (((s->f08 ^ b->stacks[j].f08) & 1) != 0) matched = true;
                    }
                }
            }
            if (matched) continue;
        }
        if (!flag) { if (s->f08 & 1) c2++; else c1++; }
        else       { if (s->f08 & 1) c1++; else c2++; }
    }
    if (c1 > d1) return false;
    if (c2 > d2) return false;
    return true;
}

// @ 0x491b20
int World::gate_rating(unsigned p1, unsigned p2, int p3) {
    int a = 0, b = 0, c = 0, d = 0;
    if (p2 < (unsigned)mgr.count) {
        int i = mgr.index_of(p2);
        if (i >= 0 && mgr.atu((unsigned)i)->type == 1) {
            get_prisoners(p2, &a, &b, &c, &d);
        }
    }
    float f1 = (float)(a - b);
    float f2 = 0.0f;
    if (p1 < (unsigned)mgr.count) {
        int i = mgr.index_of(p1);
        if (i >= 0 && mgr.atu((unsigned)i)->type == 1) {
            int j = mgr.index_of(p1);
            f2 = (float)mgr.at(j)->sum(0x110);
        }
    }
    float f3 = 0.0f;
    if (p3 >= 0 && mgr.is_slot(p3)) {
        a = b = c = d = 0;
        get_prisoners(p3, &a, &b, &c, &d);
        f3 = (float)(a - b);
    }
    float n = f3 + f2;
    float ratio = n / f1;
    if (ratio >= 1.0f) ratio = ratio - 1.0f;
    else ratio = 1.0f - 1.0f / ratio;
    float mag = (n + f1) * 0.02f;
    if (mag < 0.5f) mag = 0.5f;
    else if (mag > 1.0f) mag = 1.0f;
    float prod = mag * ratio;
    float fl = floor(prod);
    int r = (int)fl;
    if (prod - fl > 0.5f) r++;
    if (r < 1) r = 1;
    if (p1 < (unsigned)mgr.count) {
        int i = mgr.index_of(p1);
        if (i >= 0 && mgr.atu((unsigned)i)->type == 1) {
            int j = mgr.index_of(p1);
            r += mgr.atu((unsigned)j)->f4b2370(0xf) / 2;
        }
    }
    if (p2 < (unsigned)mgr.count) {
        int i = mgr.index_of(p2);
        if (i >= 0 && mgr.atu((unsigned)i)->type == 1) {
            r -= mgr.atu((unsigned)i)->f4b2370(0xf) / 2;
        }
    }
    if (r < -0x14) return -0x14;
    if (r > 0x14) return 0x14;
    return r;
}

// @ 0x491e50
void World::set_pos(float x, float y, int id, float radius) {
    pick_near(x, y, radius, &x);
    if ((unsigned)id >= (unsigned)mgr.count) return;
    Mgr* m = &mgr;
    int i = m->index_of(id);
    if (i < 0) return;
    if (m->atu((unsigned)i)->type != 1) return;
    i = m->index_of(id);
    Party* p = m->at(i);
    p->f30 = x;
    p->f34 = y;
    i = m->index_of(id);
    m->at(i)->f4b2880();
}

// @ 0x491f20
int World::token_lookup(int a, int b, int c, int d) {
    if (a == gate) return arr1405c[b * 0x20 + c];
    return token(a, b, c, d);
}

// @ 0x491f50
int Mission3::rating(int idx) {
    int total = 0;
    if (rec8[idx].f00 > 0) total = hex[0].v;
    if (rec8[idx].f04 > 0) total += hex[1].v;
    if (rec8[idx].f08 > 0) total += hex[2].v;
    if (rec8[idx].f0c > 0) total += hex[3].v;
    if (rec8[idx].f10 > 0) total += hex[4].v;
    if (rec8[idx].f14 > 0) total += hex[5].v;
    if (rec8[idx].f18 > 0) total += hex[6].v;
    if (rec8[idx].f1c > 0) total += hex[7].v;
    if (total == 0) return 0;
    int cu = cur;
    if (cu < 0) return 0;
    BRec* rr = rec((unsigned)cu);
    if (rr->f1c4 < 0) return 0;
    int n = ((int*)&rec8[idx].f00)[rr->f1c4];
    int s = f6520;
    if (n < 0) s = -s;
    float v = total * (1.0f / speed_factor((float)s));
    double fl = floor(v);
    int res = (int)fl;
    if (v - fl > 0.5) res++;
    if (res < 1) res = 1;
    return res;
}

// @ 0x4920e0
bool Mission3::check(int a) {
    if (cur < 0) return false;
    int* p = &recs()[rec((unsigned)cur)->f1c4].f00;
    Hex18* h = hex;
    for (int k = 0; k < 8; k++, p++, h++) {
        if (*p == -1 && h->v > 0) return false;
    }
    return (float)g_5e2958 * 1e-05f - hex[7].f >= (float)a;
}

// @ 0x492170
bool Mission3::is_active() {
    unsigned cu = (unsigned)cur;
    if (cu == 0xffffffff) return false;
    return rec(cu)->type != 1;
}

// @ 0x4921b0
unsigned Mission3::find(int id) {
    unsigned found = 0xffffffff;
    unsigned n = (unsigned)pool.count;
    for (unsigned i = pool.first(); i < n; pool.next(&i)) {
        if (rec(i)->f88 == id) found = i;
    }
    return found;
}
