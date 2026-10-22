// ===========================================================================
// mount&blade.exe (MSVC 7.1 / cl 13.10, x86-32) -- frozen batch ds1, slice 261
//
// The record constructors / record readers of the Data\*.txt tables, plus the
// tstring-vector reallocation path they use.  All eleven functions are called
// from Game::load_game_data (0x5236f0, slice s005236f0).
// ===========================================================================
#include <new>
#include <stdlib.h>
#include <math.h>
#include "../s00429890/tstring.h"

typedef unsigned int uint;

// the engine's fast float floor: bias non-positive values by (1 - 2^-24), then truncate
static int ftoi_floor(float t)
{
    if (!(t > 0.0f))
        t -= 0.99999988f;
    return (int)t;
}

// ---------------------------------------------------------------------------
// std::vector<tstring> internals (VC7.1, 0x10-byte element)
// ---------------------------------------------------------------------------
struct ptr_tag {};              // std::_Nonscalar_ptr_iterator_tag (empty, passed by value)
struct tstring_alloc {
    int pad;                                                    // +0 allocator subobject
    void destroy(tstring* f, tstring* l);                       // 0x42d360
};
// 0x438f00
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v,
                               tstring_alloc& al, ptr_tag tag);
// 0x524cd0
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d,
                                 tstring_alloc& al, ptr_tag tag);

// 0x401800
void __stdcall vec_ctor_iter(void* dst, unsigned size, unsigned count, void* ctorfn);

struct tstring_vec : tstring_alloc {
    tstring* first;                                             // +0
    tstring* last;                                              // +4
    tstring* end;                                               // +8
    tstring_vec() : first(0), last(0), end(0) {}
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    tstring* last_pos() const { return last; }
    void _Xlen() const;                                         // 0x43e2a0
    void insert_n(tstring* pos, unsigned n, const tstring& v);  // 0x43e310
    void reserve(unsigned n);                                   // 0x524bc0
    void push_back(const tstring& v)
    {
        if (size() < capacity())
        {
            ptr_tag t;
            tstring* p = last;
            uninit_fill_n_str(p, 1, v, *this, t);
            last = p + 1;
        }
        else
        {
            tstring* pos = last_pos();
            insert_n(pos, 1, v);
        }
    }
};

// ---------------------------------------------------------------------------
// the engine's binary stream interface (27 virtuals; only 0x38..0x54 matter)
// ---------------------------------------------------------------------------
struct stream_base {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13();
    virtual stream_base& r_float(float* p);                     // +0x38 slot 14
    virtual stream_base& r_bool(bool* p);                       // +0x3c slot 15
    virtual stream_base& r_uint(uint* p);                       // +0x40 slot 16
    virtual stream_base& r_u64(unsigned __int64* p);            // +0x44 slot 17
    virtual stream_base& r_i64(__int64* p);                     // +0x48 slot 18
    virtual stream_base& r_int(int* p);                         // +0x4c slot 19
    virtual stream_base& r_byte(unsigned char* p);              // +0x50 slot 20
    virtual stream_base& r_tstr(tstring* p);                    // +0x54 slot 21
    tstring name;                                               // +0x04
};

struct file_stream : stream_base {                              // 0x30 bytes
    int state;                                                  // +0x14
    char autoflush;                                             // +0x18
    char pad19[3];
    int f1c;                                                    // +0x1c
    tstring buf;                                                // +0x20
    file_stream() : autoflush(0), f1c(0) { state = 0; }
};

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------
extern int   g_level_lo[];          // 0x5937c8
extern int   g_level_hi[];          // 0x5937cc
extern float g_troop_rand_scale;    // 0x54a6bc
extern int   g_clamp_max;           // 0x8ee3ac4
struct NameMgr { tstring* find(tstring* key); };                // 0x4c6010
extern NameMgr g_name_mgr;          // 0x5e2a18

struct Pair { int a; int b; };

// ---------------------------------------------------------------------------
// triggers.txt statement (0x98-byte element, allocated with new[])
// ---------------------------------------------------------------------------
struct StmtRec {
    int kind;                       // +0x00
    char pad04[4];
    int f08;                        // +0x08
    char pad0c[4];
    __int64 f10;                    // +0x10
    char pad18[0x78];
    int f90;                        // +0x90
    char pad94[4];
    StmtRec() : f90(-1) {}
};

struct TriggerStmt {
    StmtRec* p;                     // +0
    int n;                          // +4
    char flag;                      // +8
    void init(int op, int idx);     // 0x524360
};

// ---------------------------------------------------------------------------
// skills.txt record (0x44 bytes)
// ---------------------------------------------------------------------------
struct SkillRec {
    tstring f0;                     // 0x00
    tstring f10;                    // 0x10
    tstring f20;                    // 0x20
    char pad30[4];
    int f34;                        // 0x34
    int f38;                        // 0x38
    char pad3c[4];
    int f40;                        // 0x40
};

// ---------------------------------------------------------------------------
// troops.txt record (0x508 bytes)
// ---------------------------------------------------------------------------
struct TroopRec {
    tstring f0;                     // 0x000
    tstring f10;                    // 0x010
    tstring f20;                    // 0x020
    unsigned __int64 u30;           // 0x030
    int f38;                        // 0x038
    int f3c;
    int f40;
    int f44;
    int f48;
    int f4c;
    int f50;
    int f54;
    int f58[4];                     // 0x058
    uint f68[6];                    // 0x068
    float f80[7];                   // 0x080
    int f9c;                        // 0x09c
    float fa0;                      // 0x0a0
    int fa4;
    int fa8;
    int fac;
    int fb0;
    Pair pairs[0x60];               // 0x0b4 .. 0x3b3
    int f3b4;                       // 0x3b4
    int f3b8;                       // 0x3b8
    unsigned __int64 u3c0[4];       // 0x3c0
    unsigned __int64 u3e0[4];       // 0x3e0
    char pad400[0x20];              // 0x400
    char names_raw[0x80];           // 0x420 tstring[8]
    int f4a0;                       // 0x4a0
    Pair p10[10];                   // 0x4a4
    tstring_vec v;                  // 0x4f4
    TroopRec();
    void read(stream_base& s);      // 0x524420
};

// ---------------------------------------------------------------------------
// quests.txt record (0x138 bytes)
// ---------------------------------------------------------------------------
struct QuestRec {
    tstring f0;                     // 0x000
    tstring f10;                    // 0x010
    tstring f20;                    // 0x020
    tstring f30;                    // 0x030
    tstring f40;                    // 0x040
    int f50;                        // 0x050
    tstring f54;                    // 0x054
    int f64;                        // 0x064
    char pad68[0x20];               // 0x068
    char names_raw[0x80];           // 0x088 tstring[8]
    int f108;                       // 0x108
    char pad10c[0xc];
    int f118;                       // 0x118
    int f11c;                       // 0x11c
    char pad120[0xc];
    int f12c;                       // 0x12c
    int f130;                       // 0x130
    int f134;                       // 0x134
    QuestRec();
    void read(stream_base& s);      // 0x524690
    tstring* default_name();        // 0x48d750
};

// ---------------------------------------------------------------------------
// scenes.txt record (0x494 bytes)
// ---------------------------------------------------------------------------
struct RecY {
    virtual void rv0();             // vtable 0x590904
    virtual void rv1();
    virtual void rv2();
    char pad04[0xc];                // 0x004
    tstring f10;                    // 0x010
    tstring f20;                    // 0x020
    char pad30[4];
    tstring f34;                    // 0x034
    tstring f44;                    // 0x044
    char pad54[0x14];
    tstring f68;                    // 0x068
    char b78;                       // 0x078
    char pad79[0x8b];
    tstring_vec v104;               // 0x104
    Pair pairs114[0x40];            // 0x114
    int ints314[0x40];              // 0x314
    file_stream s414;               // 0x414
    file_stream s444;               // 0x444
    tstring_vec v474;               // 0x474
    tstring f484;                   // 0x484
    RecY();
    void read(stream_base& s);      // 0x524d60 (slice 262)
};

// ---------------------------------------------------------------------------
// factions.txt record (0x300 bytes)
// ---------------------------------------------------------------------------
struct Faction {
    tstring name;                   // 0x000
    char b10;                       // 0x010
    char pad11[3];
    tstring f14;                    // 0x014
    int f24;                        // 0x024
    uint color;                     // 0x028
    float arr[0x80];                // 0x02c .. 0x22b
    int f22c;                       // 0x22c
    int f230;                       // 0x230
    tstring_vec v;                  // 0x234
    float f244;                     // 0x244
    float f248;                     // 0x248
    tstring_vec v2;                 // 0x24c
    char pad25c[0x20];              // 0x25c
    char names_raw[0x80];           // 0x27c tstring[8]
    int f2fc;                       // 0x2fc
    Faction();
    void read(stream_base& s, int count);   // 0x524a70
};

// ===========================================================================
// @ 0x524360
void TriggerStmt::init(int op, int idx)
{
    flag = 0;
    if (p)
        delete[] p;
    p = new StmtRec[1];
    p->kind = op;
    p->f08 = 1;
    p->f10 = idx;
    n = 1;
}

// ===========================================================================
// @ 0x5243c0
stream_base& read_skill(stream_base& s, SkillRec& r)
{
    s.r_tstr(&r.f20);
    s.r_tstr(&r.f10).r_int(&r.f40).r_int(&r.f34).r_tstr(&r.f0);
    r.f10.underscores_to_spaces();
    r.f0.underscores_to_spaces();
    r.f38 = r.f34;
    return s;
}

// ===========================================================================
// @ 0x524420
void TroopRec::read(stream_base& s)
{
    int n;
    s.r_tstr(&f20);
    s.r_tstr(&f0);
    f0.underscores_to_spaces();
    s.r_tstr(&f10);
    f10.underscores_to_spaces();
    s.r_u64(&u30);
    s.r_int(&f3b8);
    s.r_int(&f38);
    s.r_int(&f4c);
    s.r_int(&f50);
    s.r_int(&f54);
    int* pa = &pairs[0].a;
    for (int i = 0; i < 0x40; ++i, pa += 2)
    {
        s.r_int(pa);
        s.r_int(pa + 1);
    }
    for (int i = 0; i < 10; ++i)
    {
        p10[i].a = -1;
        p10[i].b = 0;
    }
    for (int i = 0; i < 4; ++i)
        s.r_int(&f58[i]);

    s.r_int(&n);
    if (n < 1)
        n = 1;
    fb0 = n;
    int base = g_level_lo[n];
    int r = rand() % 0x3dc9;
    float t = (float)r * (float)(g_level_hi[n] - base) * g_troop_rand_scale;
    f9c = base + ftoi_floor(t);

    for (int i = 0; i < 7; ++i)
    {
        int v;
        s.r_int(&v);
        f80[i] = (float)v;
    }
    for (int i = 0; i < 6; ++i)
        s.r_uint(&f68[i]);

    int b = (f68[4] >> 16) & 0xf;
    if (b < 0)
        b = 0;
    if (b > g_clamp_max)
        b = g_clamp_max;
    fa0 = (float)(f58[0] + 0x23 + b * 2);

    for (int i = 0; i < 4; ++i)
        s.r_u64(&u3c0[i]);
    for (int i = 0; i < 4; ++i)
        s.r_u64(&u3e0[i]);
}

// ===========================================================================
// @ 0x524690
void QuestRec::read(stream_base& s)
{
    s.r_tstr(&f54);
    s.r_tstr(&f0);
    s.r_int(&f50);
    s.r_tstr(&f10);
    f0.underscores_to_spaces();
    f10.underscores_to_spaces();
    f30 = *default_name();
    tstring* p = g_name_mgr.find(&f54);
    f40 = p ? *p : f0;
}

// ===========================================================================
// @ 0x524710
TroopRec::TroopRec()
{
    for (int i = 0; i < 0x60; ++i)
        ((int*)pairs)[i * 2] = -1;
    vec_ctor_iter(names_raw, 0x10, 8, (void*)0x42bbd0);
    f4a0 = -1;
    for (int i = 0; i < 10; ++i)
    {
        p10[i].a = -1;
        p10[i].b = 0;
    }
    u30 = 0;
    f3b8 = -1;
    f48 = 0;
    f44 = 0;
    f3b4 = 0;
    f9c = 0;
    fa4 = 0;
    fac = 0;
    fa8 = 0;
    fb0 = 1;
    for (int i = 0; i < 4; ++i)
        f58[i] = 0;
    for (int i = 0; i < 7; ++i)
        f80[i] = 0;
}

// ===========================================================================
// @ 0x524860
QuestRec::QuestRec()
{
    vec_ctor_iter(names_raw, 0x10, 8, (void*)0x42bbd0);
    f108 = -1;
    f12c = 0;
    f130 = 0;
    f134 = 0;
    f50 = 0;
    f64 = 0;
    f11c = 0;
    f118 = -1;
}

// ===========================================================================
// @ 0x5248e0
RecY::RecY()
{
    b78 = 0;
    for (int i = 0; i < 0x40; ++i)
    {
        pairs114[i].a = -1;
        ints314[i] = 0;
    }
}

// ===========================================================================
// @ 0x5249f0
Faction::Faction()
{
    vec_ctor_iter(names_raw, 0x10, 8, (void*)0x42bbd0);
    f2fc = -1;
    f230 = 0;
    b10 = 0;
    f244 = -1.0f;
    f248 = 1.0f;
}

// ===========================================================================
// @ 0x524a70
void Faction::read(stream_base& s, int count)
{
    s.r_tstr(&f14);
    s.r_tstr(&name);
    name.underscores_to_spaces();
    s.r_int(&f24);
    s.r_uint(&color);
    color |= 0xff000000;
    f248 = (float)(100 - ((f24 >> 8) & 0xff)) * 0.01f;

    if (count > 0)
    {
        float* p = arr;
        for (int i = count; i > 0; --i)
        {
            s.r_float(p);
            ++p;
        }
    }
    s.r_int(&f22c);
    if (f22c != 0)
    {
        tstring tmp;
        v.reserve(f22c);
        for (int i = 0; i < f22c; ++i)
        {
            s.r_tstr(&tmp);
            v.push_back(tmp);
        }
    }
}

// ===========================================================================
// @ 0x524bc0
void tstring_vec::reserve(unsigned n)
{
    if (n > 0x0fffffff)
        _Xlen();
    if (capacity() >= n)
        return;
    tstring* p = (tstring*)::operator new(n * 0x10);
    try
    {
        ptr_tag t;
        uninit_copy_str(first, last, p, *this, t);
    }
    catch (...)
    {
        ::operator delete(p);
        throw;
    }
    unsigned s = first == 0 ? 0 : (unsigned)(last - first);
    if (first)
    {
        destroy(first, last);
        ::operator delete(first);
    }
    end = p + n;
    last = p + s;
    first = p;
}
