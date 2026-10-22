// Slice s00492230: battle order helpers on the mission object, the "refresh ratings" pass and the
// side-selection helper that calls it, the World's stack transfer, the VC7.1 vector<E100>::_Insert_n
// instantiation (with its cold tail) and the teardown of the World's four engine arrays.
//   0x492230  Mission3::set_weapon_order_1(team, value)  (writes +0x528c/+0x52f8/+0x5364 per team)
//   0x492350  Mission3::set_weapon_order_2(team, value)  (writes +0x5290/+0x52fc/+0x5368)
//   0x492470  Battle::refresh_rating()
//   0x492530  World::transfer_stack(a, b, flag)          (World = 0x5e2930)
//   0x492750  Battle::select_side(side)
//   0x4927d0  vector::_Insert_n (std::vector<E100>)   (+ cold tail 0x492930)
//   0x492a20  World::free_arrays()
#include <vector>
#include <windows.h>
#include <stdlib.h>

// ---- shared records ---------------------------------------------------------------------------------------
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
    char pad8[0xa0 - 0x8];
    int f_a0;                                    // +0xa0
    char padA4[0xa8 - 0xa4];
    Stack stacks[64];                            // +0xa8
    int nstacks;                                 // +0x8a8

    int add(int troop, int amount, int flag, int a4, int a5);  // 0x48c840 (ret 0x14)
    int sum(int what);                           // 0x4b1cf0 (ret 4)
};

struct Mgr {                                     // world+0xf8
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    int count;                                   // +0x138dc
    int index_of(int id);                        // 0x48ad00 (ret 4)
    Party* at(int i) { Party*& pp = pages[i >> 4]; return (Party*)((char*)pp + (i & 0xf) * 0x5ac8); }
};

// Battle record (0x5928 bytes, pages at self+0x1c); also handed to Agent's weapon calls as-is.
struct BRec {
    char pad0[0x4];
    int valid;                                   // +0x04
    char pad8[0x14 - 0x8];
    int type;                                    // +0x14
    char pad18[0x1c4 - 0x18];
    int f1c4;                                    // +0x1c4
    char pad1c8[0x56d4 - 0x1c8];
    int f56d4;                                   // +0x56d4
    char pad56d8[0x5928 - 0x56d8];
};

struct Pool {                                    // the mission's record pool, at +0x04
    int count;                                   // +0x00
    unsigned first();                            // 0x43b1b0 (ret)
    void next(unsigned* p);                      // 0x43b160 (ret 4)
};

// Element types whose destructors are out of line in other slices.
struct RecS {                                    // 0x508, dtor 0x431c40
    char body[0x508];
    ~RecS();                                     // 0x431c40 (ret)
};
struct RecT {                                    // 0x300, dtor 0x432010
    char body[0x300];
    ~RecT();                                     // 0x432010 (ret)
};
struct RecQ {                                    // 0x138, dtor 0x431b10
    char body[0x138];
    ~RecQ();                                     // 0x431b10 (ret)
};
struct Poly {                                    // polymorphic heap object with a virtual destructor
    virtual ~Poly();                             // deleting destructor in vtable slot 0
};

struct World {                                   // 0x5e2930
    char pad0[0x78];
    int gate;                                    // +0x78
    char pad7c[0xf8 - 0x7c];
    Mgr mgr;                                     // +0xf8
    char padA[0x1405c - 0x139d8];
    int arr1405c[1];                             // +0x1405c (flat 0x20-dword stack word table)

    int gate_rating(unsigned p1, unsigned p2, int p3);           // 0x491b20 (ret 0xc)
    void dissolve(int id);                       // 0x4915d0 (ret 4)
    void transfer_stack(int a, int b, char flag);                // 0x492530 (ret 0xc)
    void free_arrays();                          // 0x492a20
};

// The mission object (the one whose +0x1c page table holds 0x5928-byte battle records, count at +4).
struct Mission3 {
    char pad0[0x4];
    int count;                                   // +0x04 (record count; also the pool's count)
    char pad8[0x1c - 0x08];
    BRec** pages;                                // +0x1c (record page table, 0x5928-byte records)

    void set_weapon_order_1(int team, unsigned value);           // 0x492230 (ret 8)
    void set_weapon_order_2(int team, unsigned value);           // 0x492350 (ret 8)
};

// The battle/order object: two ids at +0/+4, flags +0x10, mode +0x14, side +0x24, tier +0x28,
// the active-id selector at +0x138, the rating at +0x1c.
struct Battle {
    char pad0[0x4];
    int ids[2];                                  // +0x04 / +0x08
    char padC[0x10 - 0x0c];
    int flags;                                   // +0x10
    int mode;                                    // +0x14
    char pad18[0x24 - 0x18];
    int side;                                    // +0x24
    int tier;                                    // +0x28
    char pad2c[0x138 - 0x2c];
    int sel;                                     // +0x138
    int rating;                                  // +0x1c  (kept by byte offset in the bodies)
    char pad140[0x144 - 0x13c];
    int f144;                                    // +0x144

    void refresh_rating();                       // 0x492470
    void select_side(int side);                  // 0x492750 (ret 4)
};

struct Agent {
    char pad0[0x88];
    char pad88[0x1c4 - 0x88];
    int f1c4;                                    // +0x1c4
    char pad1c8[0x56d4 - 0x1c8];
    int f56d4;                                   // +0x56d4
    int cls();                                   // 0x433140 (ret)
    void set_weapon_mode(int mode);              // 0x4461e0 (ret 4)
};

extern World* g_world;                           // 0x5e2930 (pointer to the World)

// @ 0x492230
void Mission3::set_weapon_order_1(int team, unsigned value)
{
    char* self = (char*)this;
    int base = team * 0x16c;
    if (*(int*)(self + base + 0x52c8) != 0)
        *(unsigned*)(self + base + 0x528c) = value;
    if (*(int*)(self + base + 0x5334) != 0)
        *(unsigned*)(self + base + 0x52f8) = value;
    if (*(int*)(self + base + 0x53a0) != 0)
        *(unsigned*)(self + base + 0x5364) = value;

    int n = *(int*)(self + 4);
    unsigned i = 0;
    if (n > 0) {
        do {
            if (*(int*)((char*)this->pages[i >> 4] + (i & 0xf) * 0x5928 + 4) != 0)
                break;
            i++;
        } while ((int)i < n);
    }
    Pool* pool = (Pool*)(self + 4);
    unsigned j = i;
    for (; j < pool->count; pool->next(&j)) {
        int off = (j & 0xf) * 0x5928;
        BRec** pg = this->pages;                        // read back each iteration (a call follows)
        BRec* r = (BRec*)((char*)pg[j >> 4] + off);
        if (r->type == 1) {
            int k = r->f1c4;
            if (k < 0 || k >= 8)
                k = 7;
            if (k == team) {
                Agent* a = (Agent*)r;
                int c = a->cls();
                if (*(int*)(self + base + c * 0x6c + 0x52c8) != 0) {
                    int w = a->f56d4;
                    a->f56d4 = 0;
                    a->set_weapon_mode(w);
                }
            }
        }
    }
}

// @ 0x492350
void Mission3::set_weapon_order_2(int team, unsigned value)
{
    char* self = (char*)this;
    int base = team * 0x16c;
    if (*(int*)(self + base + 0x52c8) != 0)
        *(unsigned*)(self + base + 0x5290) = value;
    if (*(int*)(self + base + 0x5334) != 0)
        *(unsigned*)(self + base + 0x52fc) = value;
    if (*(int*)(self + base + 0x53a0) != 0)
        *(unsigned*)(self + base + 0x5368) = value;

    int n = *(int*)(self + 4);
    unsigned i = 0;
    if (n > 0) {
        do {
            if (*(int*)((char*)this->pages[i >> 4] + (i & 0xf) * 0x5928 + 4) != 0)
                break;
            i++;
        } while ((int)i < n);
    }
    Pool* pool = (Pool*)(self + 4);
    unsigned j = i;
    for (; j < pool->count; pool->next(&j)) {
        int off = (j & 0xf) * 0x5928;
        BRec** pg = this->pages;                        // read back each iteration (a call follows)
        BRec* r = (BRec*)((char*)pg[j >> 4] + off);
        if (r->type == 1) {
            int k = r->f1c4;
            if (k < 0 || k >= 8)
                k = 7;
            if (k == team) {
                Agent* a = (Agent*)r;
                int c = a->cls();
                if (*(int*)(self + base + c * 0x6c + 0x52c8) != 0) {
                    int w = a->f56d4;
                    a->f56d4 = 0;
                    a->set_weapon_mode(w);
                }
            }
        }
    }
}

// @ 0x492470
void Battle::refresh_rating()
{
    char* self = (char*)this;
    Mgr* mgr = &g_world->mgr;
    Party* pp = mgr->at(mgr->index_of(*(int*)(self + *(int*)(self + 0x138) * 4)));
    int tier = pp->sum(0x10);
    if (tier <= 3)
        *(int*)(self + 0x28) = 0;
    else if (tier <= 0xa)
        *(int*)(self + 0x28) = 1;
    else
        *(int*)(self + 0x28) = (tier > 0x32) + 2;

    int s = *(int*)(self + 0x138);
    int r = g_world->gate_rating(g_world->gate, *(unsigned*)(self + s * 4),
                                 *(int*)(self + (1 - s) * 4));
    *(int*)(self + 0x1c) = r;
    if (r > 6)
        *(int*)(self + 0x1c) = 6;
    if (*(int*)(self + 0x1c) < -0x20)
        *(int*)(self + 0x1c) = -0x20;
}

// @ 0x492530
void World::transfer_stack(int a, int b, char flag)
{
    char* w = (char*)this;
    Mgr* mgr = &this->mgr;
    Party* pa = mgr->at(mgr->index_of(a));
    Party* pb = mgr->at(mgr->index_of(b));
    int* wstacks = (int*)(w + 0x1405c);
    for (int k = 0; k < pa->nstacks; k++) {
        Stack* s = &pa->stacks[k];
        int f = s->f08;
        if (flag) {
            if (f & 1)
                f &= ~1;
            else
                f |= 1;
        }
        int r = pb->add(s->troop, s->count, f, 0, 0);
        if (b == this->gate && r >= 0) {
            int shift = s->count;
            if (shift < 0x20) {
                int* d = wstacks + r * 0x20 + shift;
                int* p = wstacks + r * 0x20;
                for (int j = 0x20 - shift; j; j--)
                    *d++ = *p++;
            } else {
                shift = 0x20;
            }
            int* out = wstacks + r * 0x20;
            for (int j = 0; j < shift; j++) {
                int v;
                if (a == this->gate) {
                    v = wstacks[k * 0x20 + j];
                } else {
                    v = -1;
                    if (a >= 0) {
                        Party* px = mgr->at(mgr->index_of(a));
                        v = *(int*)((char*)px + k * 0x20 + 0xac);
                    }
                    v = v * 0x1bf + 0x45db + a * 0x7bb7 + j;
                }
                out[j] = v;
            }
        }
    }
    this->dissolve(a);
}

// @ 0x492750
void Battle::select_side(int side)
{
    char* self = (char*)this;
    int mode = *(int*)(self + 0x14);
    if (mode != 4)
        *(int*)(self + 0x24) = side;
    if (*(int*)(self + (1 - side) * 4) >= 0) {
        int flags = *(int*)(self + 0x10) & ~0x10;
        int flip = 1 - side;
        *(int*)(self + 0x138) = flip;
        *(int*)(self + 0x10) = flags;
        *(int*)(self + 0x144) = 1;
        if (mode == 4 && flip == 1)
            *(int*)(self + 0x10) = flags | mode;
        refresh_rating();
    }
}

// ---- std::vector<int> with the engine's leading dword (allocator) at +0 ------------------------------------
struct E100 { int v; };

// @ 0x4927d0
template void std::vector<E100>::_Insert_n(std::vector<E100>::iterator, size_t, const E100&);

// @ 0x492a20
void World::free_arrays()
{
    char* self = (char*)this;
    RecS* s = *(RecS**)(self + 0x13a00);
    *(char*)(self + 0x1a05c) = 0;
    if (s != 0) {
        delete[] s;
        *(void**)(self + 0x13a00) = 0;
    }

    Poly* p = *(Poly**)(self + 0x13a50);
    if (p != 0) {
        delete[] p;
        *(void**)(self + 0x13a50) = 0;
    }

    RecT* t = *(RecT**)(self + 0x13a48);
    if (t != 0) {
        delete[] t;
        *(void**)(self + 0x13a48) = 0;
    }

    RecQ* q = *(RecQ**)(self + 0x13a58);
    if (q != 0) {
        delete[] q;
        *(void**)(self + 0x13a58) = 0;
    }
}
