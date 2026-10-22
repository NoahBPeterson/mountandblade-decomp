// Slice s004893b0: a grab-bag of adjacent helper methods of several game records (a "party" stack
// list, a 3-slot selector record, a mission setup object, a troop-table counter). Names provisional.
#include <windows.h>
#include <stdlib.h>
#include <string.h>

// rgl string (16 bytes); assign is the non-virtual operator= at 0x42bc50, copy is the copy ctor 0x42c110.
struct tstring {
    int pos; char* buf; int cap; int len;
    void assign(const tstring* o);          // 0x42bc50
    void copy(const tstring* o);            // 0x42c110
};

extern int g_5a9f9c;                        // 0x5a9f9c (day/threshold counter)
struct Tick { int lo, hi; };
extern Tick g_ticks[];                      // 0x5e2948 clock table
static void copy4(int* d, const int* s) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3]; }
struct V4 { int a, b, c, d; };
struct Rec80 { int v[32]; };
struct Allocator { char* make(int a, int b, int c, int d); };
extern Allocator g_alloc;                   // 0x5d6fa0 global factory

// ---- free helper -----------------------------------------------------------------------------------------
// @ 0x4893b0
int sub_4893b0(unsigned x)
{
    int m = x & 7;
    if (m < 2 || m >= 7) {
        if (x != 7)
            return 0;
    }
    return 1;
}

// ---- small named record (tstring at +0xc, flag at +0x1c) -------------------------------------------------
struct RecA {
    int f0, f4, f8;
    tstring name;                           // +0x0c
    char flag1c;                            // +0x1c
    char pad1d[0x1c4 - 0x1d];
    int sel;                                // +0x1c4 (bounds-checked setter below)
};
void __fastcall sub_4893e0(RecA* thisptr);
void __fastcall sub_489400(RecA* thisptr, int, const tstring* s);

// @ 0x489a90
void __fastcall sub_489a90(RecA* thisptr, int, const int* src)
{
    int v = *src;
    if (v < 0 || v >= 8)
        return;
    thisptr->sel = v;
}

// @ 0x4893e0
void __fastcall sub_4893e0(RecA* thisptr)
{
    thisptr->f0 = 0;
    thisptr->f4 = -1;
    thisptr->f8 = 0;
    thisptr->name.cap = 0;
    thisptr->name.pos = 0;
    thisptr->name.buf = 0;
    thisptr->name.len = 0;
}

// @ 0x489400
void __fastcall sub_489400(RecA* thisptr, int, const tstring* s)
{
    thisptr->name.assign(s);
    thisptr->flag1c = 1;
}

// ---- named 16-byte record (tstring at +0, flag at +0x10) -------------------------------------------------
struct Named16 {
    tstring name;                           // +0x00
    char flag10;                            // +0x10
    void set(const tstring* s);             // 0x489ab0
};

// @ 0x489ab0
void Named16::set(const tstring* s)
{
    name.assign(s);
    flag10 = 1;
}

// ---- party stack list (stacks at +0xa8, flags at +0xa0) --------------------------------------------------
struct Stack20 {
    int count;                              // +0x00
    int troop;                              // +0x04
    int flags;                              // +0x08
    int f0c, f10, f14, f18, f1c;
};

struct Party {
    char pad0[0xa0];
    int flags_lo;                           // +0xa0
    int flags_hi;                           // +0xa4
    Stack20 stacks[64];                     // +0xa8
    int nstacks;                            // +0x8a8
    char pad8ac[0x8b4 - 0x8ac];
    char dirty;                             // +0x8b4
    char pad8b5[0x5ac8 - 0x8b5];

    void set_id(int v);                                      // 0x489420
    float scale();                                           // 0x489460
    int sum_all();                                           // 0x489490
    int sum_head();                                          // 0x4894b0
    int sum_flagged();                                       // 0x4894e0
    int count_tail();                                        // 0x489510
    void remove_row(int* out, int index, int* arr);          // 0x489540
    int find_or_add(unsigned troop, unsigned flag, char force);   // 0x489620

    int ext_4b22d0();                       // 0x4b22d0
    int ext_4b2330();                       // 0x4b2330
    int ext_4b1700();                       // 0x4b1700
    int ext_463190();                       // 0x463190
    __declspec(noinline) void set_u64(unsigned __int64 v);
    __declspec(noinline) unsigned __int64 get_u64();
};

void Party::set_u64(unsigned __int64 v)
{
    *(unsigned __int64*)&flags_lo = v;
}

unsigned __int64 Party::get_u64()
{
    return *(unsigned __int64*)&flags_lo;
}

// @ 0x489420
void Party::set_id(int v)
{
    unsigned __int64* f = (unsigned __int64*)&flags_lo;
    ((char*)&flags_lo)[0] = 0;
    *f |= (long long)(v & 0xff);
}
// @ 0x489460
float Party::scale()
{
    unsigned u = (unsigned)flags_lo & 0x3000;
    if (u == 0)
        return 0.5f;
    if (u == 0x1000)
        return 0.75f;
    return 1.0f;
}

// @ 0x489490
int Party::sum_all()
{
    int s = 0;
    int n = nstacks;
    if (n > 0) {
        Stack20* p = stacks;
        do {
            s += p->count;
            p++;
        } while (--n);
    }
    return s;
}

// @ 0x4894b0
int Party::sum_head()
{
    int s = 0;
    int i = 0;
    int n = nstacks;
    if (n > 0) {
        Stack20* p = stacks;
        do {
            if (p->flags & 1)
                return s;
            s += p->count;
            i++;
            p++;
        } while (i < n);
    }
    return s;
}

// @ 0x4894e0
int Party::sum_flagged()
{
    int s = 0;
    int n = nstacks;
    if (n > 0) {
        Stack20* p = stacks;
        do {
            if (p->flags & 1)
                s += p->count;
            p++;
        } while (--n);
    }
    return s;
}

// @ 0x489510
int Party::count_tail()
{
    int n = nstacks;
    int lead = 0;
    int i = 0;
    if (n > 0) {
        char* q = (char*)stacks + 8;
        do {
            if (*q & 1)
                break;
            lead++;
            i++;
            q += 0x20;
        } while (i < n);
    }
    return n - lead;
}

// @ 0x489540
void Party::remove_row(int* out, int index, int* arr)
{
    Stack20 local = stacks[index];

    if (index < nstacks - 1) {
        int i = index;
        do {
            stacks[i] = stacks[i + 1];
            i++;
        } while (i < nstacks - 1);
    }

    *(int*)((char*)this + nstacks * 0x20 + 0x8c) = -1;
    *(int*)((char*)this + nstacks * 0x20 + 0x88) = 0;
    int cnt = nstacks - 1;
    nstacks = cnt;
    dirty = 1;

    if (arr != 0 && index < cnt) {
        int j = index;
        do {
            *(Rec80*)((char*)arr + j * 0x80) = *(Rec80*)((char*)arr + (j + 1) * 0x80);
            j++;
        } while (j < nstacks);
    }

    *(Stack20*)out = local;
}

// @ 0x489620
int Party::find_or_add(unsigned troop, unsigned flag, char force)
{
    int found = -1;
    if (*(unsigned __int64*)&flags_lo & 0x100000) {
        int have, cap;
        if (flag & 1) {
            have = 0;
            int m = nstacks;
            if (m > 0) {
                Stack20* p = stacks;
                do {
                    if (p->flags & 1)
                        have += p->count;
                    p++;
                } while (--m);
            }
            cap = ext_4b2330();
        } else {
            have = sum_head();
            cap = ext_4b22d0();
        }
        if (cap - have <= 0) {
            if (force)
                return found;
        }
    }

    int n = nstacks;
    int i = 0;
    if (n > 0) {
        Stack20* p = stacks;
        do {
            if (p->troop == (int)troop && ((p->flags ^ flag) & 1) == 0)
                found = i;
            i++;
            p++;
        } while (i < n);
        if (found != -1)
            return found;
    }
    if (n >= 0x40)
        return found;

    if (flag & 1) {
        if ((*(unsigned __int64*)&flags_lo & 0x100000) == 0)
            goto tail_count;
        if (count_tail() < ext_4b1700()) {
        tail_count: ;
            int lead = 0;
            int m = nstacks;
            for (int k = 0; k < m; k++) {
                if (stacks[k].flags & 1)
                    break;
                lead++;
            }
            if (m - lead < 0x20)
                return nstacks;
        }
    }
    if (!(flag & 1)) {
        if (*(unsigned __int64*)&flags_lo & 0x100000) {
            if (!(ext_463190() < ext_4b1700()))
                return found;
        }
        int lead = 0;
        int m = nstacks;
        for (int k = 0; k < m; k++) {
            if (stacks[k].flags & 1)
                break;
            lead++;
        }
        if (lead < 0x20)
            return nstacks;
    }
    return found;
}

// ---- 3-slot selector record (records of 0x6c bytes) ------------------------------------------------------
struct SelRec {
    char pad0[0xc];
    int x;                                  // +0x0c
    char pad10[0x18 - 0x10];
    int a[4];                               // +0x18
    int c[4];                               // +0x28
    int b;                                  // +0x38
    char pad3c[0x4c - 0x3c];
    int present;                            // +0x4c
    char pad50[0x60 - 0x50];
    int counter;                            // +0x60
    char pad64[0x6c - 0x64];
};

struct Sel {
    SelRec rec[3];
    void set_v(int v);                                      // 0x489860
    void first(int* out);                                   // 0x489890
    void set_vec(const int* a, const int* b, const int* c); // 0x4898e0
    void bump(int d);                                       // 0x4899e0
};

// @ 0x489860
void Sel::set_v(int v)
{
    if (rec[0].present)
        rec[0].x = v;
    if (rec[1].present)
        rec[1].x = v;
    if (rec[2].present)
        rec[2].x = v;
}

// @ 0x489890
void Sel::first(int* out)
{
    int i = 0;
    int* p = &rec[0].present;
    do {
        if (*p != 0) {
            int k = i * 0x6c;
            V4* a = (V4*)((char*)this + k + 0x18);
            V4* o = (V4*)out;
            o->a = a->a;
            o->b = a->b;
            o->c = a->c;
            o->d = a->d;
            return;
        }
        i++;
        p += 0x1b;
    } while (i < 3);
    V4* zo = (V4*)out;
    zo->a = 0;
    zo->b = 0;
    zo->c = 0;
}

// @ 0x4898e0
void Sel::set_vec(const int* a, const int* b, const int* c)
{
    if (rec[0].present) {
        *(V4*)&rec[0].a[0] = *(const V4*)a;
        rec[0].b = *b;
        *(V4*)&rec[0].c[0] = *(const V4*)c;
    }
    if (rec[1].present) {
        *(V4*)&rec[1].a[0] = *(const V4*)a;
        rec[1].b = *b;
        *(V4*)&rec[1].c[0] = *(const V4*)c;
    }
    if (rec[2].present) {
        *(V4*)&rec[2].a[0] = *(const V4*)a;
        rec[2].b = *b;
        *(V4*)&rec[2].c[0] = *(const V4*)c;
    }
}

// @ 0x4899e0
void Sel::bump(int d)
{
    if (rec[0].present) {
        int v = rec[0].counter + d;
        rec[0].counter = v;
        if (v < 1)
            rec[0].counter = 1;
        else if (v > 4)
            rec[0].counter = 4;
    }
    if (rec[1].present) {
        int v = rec[1].counter + d;
        rec[1].counter = v;
        if (v < 1)
            rec[1].counter = 1;
        else if (v > 4)
            rec[1].counter = 4;
    }
    if (rec[2].present) {
        int v = rec[2].counter + d;
        rec[2].counter = v;
        if (v < 1)
            rec[2].counter = 1;
        else if (v > 4)
            rec[2].counter = 4;
    }
}

// ---- mission setup object (16-dword vectors at +0x18 / +0xd8 / +0x118) -----------------------------------
struct Obj1b4 {
    virtual void d0(); virtual void d1(); virtual void d2(); virtual void d3(); virtual void d4();
    virtual void d5(); virtual void d6(); virtual void d7(); virtual void d8(); virtual void d9();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22();
    virtual void slot23(void* src, void* dst);   // 0x5c
};

struct Mission {
    char pad0[0x18];
    int v18[16];                            // +0x18
    char pad58[0x64 - 0x58];
    int* p64;                               // +0x64
    char pad68[0xd8 - 0x68];
    int vd8[16];                            // +0xd8
    int v118[16];                           // +0x118
    int f158;                               // +0x158
    char pad15c[0x168 - 0x15c];
    int f168;                               // +0x168
    int f16c;                               // +0x16c
    int f170;                               // +0x170
    char pad174[0x178 - 0x174];
    int f178;                               // +0x178
    int f17c;                               // +0x17c
    int f180;                               // +0x180
    char pad184[0x1b4 - 0x184];
    Obj1b4* obj1b4;                         // +0x1b4

    void setup(void* src, int* b, int* c);  // 0x4897a0
    void set(void* src);                    // 0x489820
    __declspec(noinline) void quarter();    // 0x489ad0
    void* make(int* p);                     // 0x489b20
    void ext_467c50(int);                   // 0x467c50
    void ext_477b70();                      // 0x477b70
};

// @ 0x4897a0
void Mission::setup(void* src, int* b, int* c)
{
    ext_467c50(0);
    memcpy(v118, v18, sizeof(v18));
    memcpy(vd8, src, sizeof(vd8));
    f168 = *c;
    f16c = *b;
    int idx = f170;
    int lo = g_ticks[idx].lo;
    int hi = g_ticks[idx].hi;
    f178 = lo;
    f17c = hi;

    f180 = 0;
}

// @ 0x489820
void Mission::set(void* src)
{
    memcpy(v18, src, sizeof(v18));
    if (obj1b4 != 0)
        obj1b4->slot23(src, &f158);
    ext_477b70();
}

// @ 0x489ad0
void Mission::quarter()
{
    if (g_5a9f9c >= 0x3c) {
        v18[2] = v18[0];
        v18[3] = v18[1];
        return;
    }
    if (g_5a9f9c >= 0x19) {
        v18[2] = v18[0] / 2;
        v18[3] = v18[1] / 2;
        return;
    }
    v18[2] = v18[0] / 4;
    v18[3] = v18[1] / 4;
}

// @ 0x489b20
void* Mission::make(int* p)
{
    quarter();
    char* o = g_alloc.make(v18[2], v18[3], 0x15, 1);
    *(int*)(o + 0x64) = 0;
    *(int*)(o + 0x60) = *p;
    *(int*)(*(int*)(o + 0x48) + 0x4c) = 0;
    if (p64 != 0) {
        *(int*)(o + 0x30) = *(int*)((char*)p64 + 0x30);
        ((tstring*)(o + 0x20))->assign((tstring*)((char*)p64 + 0x20));
        *(int*)(o + 0x1c) = *(int*)((char*)p64 + 0x1c);
        ((tstring*)(o + 0x04))->assign((tstring*)((char*)p64 + 0x04));
        *(V4*)(o + 0x34) = *(V4*)((char*)p64 + 0x34);
        *(int*)(o + 0x44) = *(int*)((char*)p64 + 0x44);
        *(int*)(o + 0x50) = *(int*)((char*)p64 + 0x50);
        *(int*)(o + 0x4c) = *(int*)((char*)p64 + 0x4c);
        *(int*)(o + 0x54) = *(int*)((char*)p64 + 0x54);
        *(int*)(o + 0x58) = *(int*)((char*)p64 + 0x58);
    }
    return o;
}

// ---- 30-pointer record -----------------------------------------------------------------------------------
struct Ptr30 {
    int* p[30];                             // +0x00
    void clear_all();                       // 0x489be0
    struct E { int h[4]; };
};

// @ 0x489be0
void Ptr30::clear_all()
{
    int zero = 0, cnt = 0x63;
    int** e = &p[2];
    int i = 5;
    do {
        int* q;
        q = e[-2]; q[1] = zero; q[2] = cnt;
        q = e[-1]; q[1] = zero; q[2] = cnt;
        q = e[0];  q[1] = zero; q[2] = cnt;
        q = e[1];  q[1] = zero; q[2] = cnt;
        q = e[2];  q[1] = zero; q[2] = cnt;
        q = e[3];  q[1] = zero; q[2] = cnt;
        e += 6;
    } while (--i);
}

// ---- troop table (0x508-byte records at +0x13a00) --------------------------------------------------------
struct Pair { int id; int pad; };
struct Troop508 {
    char pad0[0xb4];
    char padb4[0x4a4 - 0xb4];
    Pair slots[10];                         // +0x4a4
    int count();                            // 0x462b60
};

struct Troop508;

struct GameData {
    char pad0[0xe028];
    tstring name;                           // +0xe028
    char pad_e03c[0x13a00 - 0xe038];
    Troop508* troops;                       // +0x13a00 (array base, records of 0x508)

    tstring* get_name(tstring* dst);                    // 0x489c30
    int count_in(int idx, int id, char flag);           // 0x489c50
};

// @ 0x489c30
tstring* GameData::get_name(tstring* dst)
{
    dst->copy(&name);
    return dst;
}

// @ 0x489c50
int GameData::count_in(int id, int idx, char flag)
{
    Troop508* base = troops;
    int c = 0;
    Troop508* t = (Troop508*)((char*)base + idx * 0x508);
    int n = t->count();
    if (n > 0) {
        Pair* e = (Pair*)((char*)t + 0xb4);
        do {
            if (e->id == id)
                c++;
            e++;
        } while (--n);
    }
    if (flag) {
        char* b = (char*)base + idx * 0x508;
        if (*(int*)(b + 0x4a4) == id) c++;
        if (*(int*)(b + 0x4ac) == id) c++;
        if (*(int*)(b + 0x4b4) == id) c++;
        if (*(int*)(b + 0x4bc) == id) c++;
        if (*(int*)(b + 0x4c4) == id) c++;
        if (*(int*)(b + 0x4cc) == id) c++;
        if (*(int*)(b + 0x4d4) == id) c++;
        if (*(int*)(b + 0x4dc) == id) c++;
        if (*(int*)(b + 0x4e4) == id) c++;
        if (*(int*)(b + 0x4ec) == id) c++;
    }
    return c;
}
