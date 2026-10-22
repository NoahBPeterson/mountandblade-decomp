// Slice s0051d920 (batch ds1): the game-state loader that mirrors s004e8f10's Game::save_state, plus the
// trigger-graph scoring helpers in front of it. `this` is the Game object (its layout is the one recovered
// in s004e8f10); the stream is the save-file reader: slot 0x38 reads one int, 0x3c an 8-byte int (the
// "int4" counterpart), 0x44 an 8-byte int, 0x48 a 4-byte int, 0x4c raw bytes and 0x54 a nested structure.
// Every offset below is read off the disassembly of this range; names are provisional.
#include <windows.h>
#include <new>
#include "rgl.h"

#define MB_NOTHROW throw()

// ---- the binary reader (mirror of s004e8f10's OutStream) -------------------------------------------------
struct LoadStream {
    virtual void s00() = 0;                              // +0x00
    virtual void s01() = 0;                              // +0x04
    virtual void s02() = 0;                              // +0x08
    virtual void s03() = 0;                              // +0x0c
    virtual void s04() = 0;                              // +0x10
    virtual void s05() = 0;                              // +0x14
    virtual void s06() = 0;                              // +0x18
    virtual void s07() = 0;                              // +0x1c
    virtual void s08() = 0;                              // +0x20
    virtual void s09() = 0;                              // +0x24
    virtual void s0a() = 0;                              // +0x28
    virtual void s0b() = 0;                              // +0x2c
    virtual void s0c() = 0;                              // +0x30
    virtual void s0d() = 0;                              // +0x34
    virtual void r_int(int* v) = 0;                      // +0x38  one int
    virtual void r_i64b(__int64* v) = 0;                 // +0x3c  the 8-byte "int4"
    virtual void s10() = 0;                              // +0x40
    virtual void r_i64(__int64* v) = 0;                  // +0x44  8 bytes
    virtual void r_int1(int* v) = 0;                     // +0x48  4 bytes
    virtual void r_raw(void* v) = 0;                     // +0x4c  raw bytes
    virtual void s20() = 0;                              // +0x50
    virtual void r_struct(void* v) = 0;                  // +0x54  nested structure
};

// ---- tstring (layout and members verified in s0042b790 / s004608d0) --------------------------------------
struct tstring {
    int   pos;          // +0x0
    char* buf;          // +0x4
    int   cap;          // +0x8
    int   len;          // +0xc
    void assign(const char* s);                          // 0x42bcb0
    char equals(const char* s) const;                    // 0x42bd10
    void underscores_to_spaces();                        // 0x42bf80
};

void __cdecl mb_error(const char* fmt, ...);             // 0x42cc80

// ---- records ---------------------------------------------------------------------------------------------
struct Rec78 {                                           // stride 0x78 (list at Game+0x139e8)
    char pad0[0xc];
    int  f0c;                                            // +0x0c
    char pad10[0x50 - 0x10];
    int  f50;                                            // +0x50
    char pad54[0x60 - 0x54];
    int  f60;                                            // +0x60
    char pad64[0x70 - 0x64];
    int  f70;                                            // +0x70
    char pad74[0x78 - 0x74];
    __declspec(noinline) void read(LoadStream* s);       // 0x51ddc0
};
struct Rec78Tag {                                        // at Game+0x139e8
    int     count;                                       // +0
    Rec78*  array;                                       // +4
    void read(LoadStream* s);                            // 0x51dd60
};
struct Rec30Tag {                                        // stride 0x30, one int at +0x28 (Game+0x139f4)
    int       count;                                     // +0
    Rec30Tag* array;                                     // +4
    void read(LoadStream* s);                            // 0x51de00
};
struct Rec138 {                                          // stride 0x138 (quests, list at Game+0x13a58)
    char  pad0[0x64];
    int   f64;                                           // +0x64
    char  pad68[0x114 - 0x68];
    int   f114;                                          // +0x114
    int   f118;                                          // +0x118
    int   f11c;                                          // +0x11c
    char  pad120[0x128 - 0x120];
    void* f128;                                          // +0x128 (a vector)
    char  pad12c[0x138 - 0x12c];
    void read(LoadStream* s);                            // 0x51e170
};

// the 8-byte element vector used by the party/item lists (its _Myfirst sits at +4)
struct E8 { int a, b; };
struct Vec8 {
    int  pad0;                                           // +0
    E8*  _Myfirst;                                       // +4
    E8*  _Mylast;                                        // +8
    E8*  _Myend;                                         // +0xc
    void resize(unsigned n, E8 val);                     // 0x493090 (thiscall, ret 0xc)
    __declspec(noinline) void read(LoadStream* s);        // 0x51e0b0
};
extern Vec8 g_list_a70480;                               // 0xa70480

struct Table;                                            // the table at Game+0xcc
struct Game {
    int    version;                                      // +0x00
    char   pad4[0x14 - 4];
    char*  f14;                                          // +0x14 trigger records (stride 0x88)
    char*  f18;                                          // +0x18 end of the trigger records
    char   pad1c[0x8c - 0x1c];
    int    f8c;                                          // +0x8c current party
    char   pad90[0x13a00 - 0x90];
    char*  records;                                      // +0x13a00
    int    nrecords;                                     // +0x13a04
    char   pad13a08[0x13a48 - 0x13a08];
    char*  f13a48;                                       // +0x13a48
    int    n13a4c;                                       // +0x13a4c
    char*  parties;                                      // +0x13a50
    int    nparties;                                     // +0x13a54
    char*  quests;                                       // +0x13a58
    int    nquests;                                      // +0x13a5c

    int  find_trigger(int* idp, int* a2, int* arr, int* cnt, unsigned* m1, unsigned* m2, int* flag);
                                                         // 0x51d920 (thiscall, ret 0x1c)
    void score_triggers(int* a, int* b, int* c);         // 0x51dac0 (ret 0xc)
    __declspec(noinline) float trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c);   // 0x51db70 (ret 0x14)
    void load_state(LoadStream* s);                      // 0x51dbd0
    void load_clock(LoadStream* s);                      // 0x51de70
    void load_quests(LoadStream* s);                     // 0x51e100
    void load_parties(LoadStream* s);                    // 0x51e240
    /* the next slices */
    void load_300(LoadStream* s);                        // 0x51e2b0
    void load_vec5c(LoadStream* s);                      // 0x51e6c0
    void load_295fc(LoadStream* s);                      // 0x51e3a0
    void load_other(LoadStream* s);                      // 0x51eaa0
    void load_records(LoadStream* s);                    // 0x51e4c0
    void load_items(LoadStream* s);                      // 0x51e450
};
struct Table {
    void read(LoadStream* s, int a);                     // 0x51e840 (thiscall, ret 8)
};

extern int g_tick_lo;                                    // 0x5e2950
extern int g_tick_hi;                                    // 0x5e2954
extern Game* g_game;                                     // 0x5e2930

LoadStream* read_soa(LoadStream* out, char* base);       // 0x51e1f0 (free function, ret 0)

// =========================================================================================================
// @ 0x51d920    int Game::find_trigger(...)   (thiscall, ret 0x1c)
// Walks the trigger graph from the record *idp: `arr`/`*cnt` is the visited stack, m1/m2 the flag masks and
// *flag the flag that decides whether to recurse.
int Game::find_trigger(int* idp, int* a2, int* arr, int* cnt, unsigned* m1, unsigned* m2, int* flag)
{
    char* f14 = this->f14;
    if (*(int*)(f14 + *idp * 0x88) < 1)
        return -1;
    int off = 0x28;
    for (int i = 0; i < *(int*)(f14 + *idp * 0x88); i++, off += 4) {
        int v = *(int*)(f14 + *idp * 0x88 + off);
        int n = *cnt;
        if (n >= 0) {
            for (int k = 0; k <= *cnt; k++)
                if (v == arr[k])
                    goto next;
        }
        if (v >= 0) {
            char* r2 = f14 + v * 0x88;
            if (*(int*)(r2 + 0x48) >= 0 || *(int*)(r2 + 0x24) == *flag) {
                if (*(int*)(r2 + 0x24) == *flag) {
                    *cnt = n + 1;
                    arr[n + 1] = *idp;
                    int res = find_trigger(&v, a2, arr, cnt, m1, m2, flag);
                    *cnt = n;
                    if (res >= 0)
                        return res;
                } else {
                    unsigned x = *m1;
                    unsigned y = *m2;
                    char* r3 = f14 + *a2 * 0x88;
                    if (((*(unsigned*)(r3 + 0x24) & x) == y && (*(unsigned*)(r2 + 0x24) & (x == y)))
                        || ((*(unsigned*)(r3 + 0x24) & x) != y
                            && (*(unsigned*)(r2 + 0x24) & (x != y)))) {
                        if (*(int*)(r2 + 0x48) >= 0)
                            return *(int*)(f14 + v * 0x88 + 0x48);
                    }
                }
            }
        }
next:
        ;
    }
    return -1;
}

// @ 0x51dac0    void Game::score_triggers(int* a, int* b, int* c)   (thiscall, ret 0xc)
// For every trigger record and every neighbour slot that is set, stores the compatibility score.
void Game::score_triggers(int* a, int* b, int* c)
{
    int i = 0;
    int off = 0;
    while (this->f14) {
        if ((unsigned)i >= (unsigned)((this->f18 - this->f14) / 0x88))
            break;
        for (int k = 0; k < *(int*)(this->f14 + off); k++) {
            if (*(int*)(this->f14 + off + 0x28 + k * 4) != -1)
                *(float*)(this->f14 + off + 0x38 + k * 4) =
                    trigger_score(i, k, *a, *b, *c);
        }
        i++;
        off += 0x88;
    }
}

// @ 0x51db70    float Game::trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c)   (ret 0x14)
float Game::trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c)
{
    char* rec = this->f14 + row * 0x88;
    char* oth = this->f14 + *(int*)(rec + 0x28 + idx * 4) * 0x88;
    unsigned m = *(unsigned*)(rec + 0x24) & a;
    unsigned n = *(unsigned*)(oth + 0x24) & a;
    if (m == c)
        return 1000000000.0f;
    if (n == c)
        return 1000000000.0f;
    if (m == b) {
        if (n != b)
            return 1000000000.0f;
    } else if (n == b)
        return 1000000000.0f;
    return 2.0f;
}

// @ 0x51dbd0    void Game::load_state(LoadStream* s)   (thiscall, ret 4)
void Game::load_state(LoadStream* s)
{
    char* p = (char*)this;
    __int64 tick;
    s->r_i64(&tick);
    g_tick_hi = *((int*)&tick + 1);
    g_tick_lo = *(int*)&tick;
    s->r_raw(p + 4);
    s->r_raw(p + 0xb4);
    load_clock(s);
    g_list_a70480.read(s);
    ((Rec78Tag*)(p + 0x139e8))->read(s);
    ((Rec30Tag*)(p + 0x139f4))->read(s);
    load_quests(s);
    load_parties(s);
    load_300(s);
    load_vec5c(s);
    load_295fc(s);
    load_other(s);
    ((Table*)(p + 0xcc))->read(s, 0);
    load_records(s);
    load_items(s);
    s->r_i64((__int64*)(records + f8c * 0x508 + 0x3c0));
    s->r_i64((__int64*)(records + f8c * 0x508 + 0x3c8));
    s->r_raw(p + 0x98);
    s->r_raw(p + 0xa0);
    s->r_raw(p + 0x9c);
    s->r_raw(p + 0xa4);
}

// @ 0x51dd60    void Rec78Tag::read(LoadStream* s)   (thiscall, ret 4)
void Rec78Tag::read(LoadStream* s)
{
    int n;
    s->r_raw(&n);
    if (n > count)
        mb_error("Load error: Number of triggers in file: %d, exceeds %d", n, count);
    for (int i = 0; i < n; i++)
        array[i].read(s);
}

// @ 0x51ddc0    void Rec78::read(LoadStream* s)   (thiscall, ret 4)
void Rec78::read(LoadStream* s)
{
    char* p = (char*)this;
    s->r_raw(p + 0xc);
    s->r_int1(&f50);
    s->r_int1(&f60);
    s->r_int1(&f70);
}

// @ 0x51de00    void Rec30Tag::read(LoadStream* s)   (thiscall, ret 4)
void Rec30Tag::read(LoadStream* s)
{
    int n;
    s->r_raw(&n);
    if (n > count)
        mb_error("Load error: Number of simple triggers in file: %d, exceeds %d", n, count);
    for (int i = 0; i < n; i++)
        s->r_int1((int*)((char*)array + 0x28 + i * 0x30));
}

// @ 0x51de70    void Game::load_clock(LoadStream* s)   (thiscall, ret 4)
void Game::load_clock(LoadStream* s)
{
    char* p = (char*)this;
    s->r_int1((int*)(p + 0x13b20));
    s->r_raw(p + 0x13b28);
    s->r_raw(p + 0x13b2c);
    s->r_raw(p + 0x13b30);
    s->r_raw(p + 0x13b34);
    s->r_raw(p + 0x13b38);
    s->r_raw(p + 0x13b60);
    s->r_int((int*)(p + 0x13b3c));
    s->r_int((int*)(p + 0x13b40));
    s->r_int((int*)(p + 0x13b44));
    s->r_int((int*)(p + 0x13b48));
    s->r_struct(p + 0x13a70);
    s->r_i64b((__int64*)(p + 0x13aa0));
    s->r_raw(p + 0x13ae8);
    s->r_raw(p + 0x13a20);
    s->r_raw(p + 0x13a0c);
    s->r_raw(p + 0x13a10);
    s->r_raw(p + 0x13aec);
    s->r_raw(p + 0x13af4);
    s->r_raw(p + 0x13af8);
    s->r_raw(p + 0x13afc);
    s->r_raw(p + 0x13ae0);
    s->r_raw(p + 0x13ae4);
    tstring& ts = *(tstring*)(p + 0x13b50);
    s->r_struct(&ts);
    ts.underscores_to_spaces();
    if (ts.equals("-1"))
        ts.assign("");
    s->r_raw(p + 0x1a1d8);
    s->r_raw(p + 0x1a1dc);
    s->r_raw(p + 0x1a1e0);
    s->r_raw(p + 0x1a1e4);
    s->r_raw(p + 0x1a1e8);
    s->r_raw(p + 0x1a1ec);
    s->r_int1((int*)(p + 0x13a88));
    s->r_int((int*)(p + 0x13ad0));
    s->r_raw(p + 0x13adc);
    s->r_raw(p + 0x13ad8);
    s->r_raw(p + 0x13ad4);
}

// @ 0x51e0b0    void Vec8::read(LoadStream* s)   (thiscall, ret 4)
void Vec8::read(LoadStream* s)
{
    int n;
    s->r_raw(&n);
    resize((unsigned)n, E8());
    for (int i = 0; i < n; i++)
        s->r_int1(&_Myfirst[i].a);
}

// @ 0x51e100    void Game::load_quests(LoadStream* s)   (thiscall, ret 4)
void Game::load_quests(LoadStream* s)
{
    char* p = (char*)this;
    int n;
    s->r_raw(&n);
    if (n > *(int*)(p + 0x13a5c))
        mb_error("Load error: Number of quests in file: %d, exceeds %d", n, *(int*)(p + 0x13a5c));
    for (int i = 0; i < n; i++)
        ((Rec138*)(*(char**)(p + 0x13a58)))[i].read(s);
}

// @ 0x51e170    void Rec138::read(LoadStream* s)   (thiscall, ret 4)
void Rec138::read(LoadStream* s)
{
    char* p = (char*)this;
    s->r_raw(p + 0x64);
    s->r_raw(p + 0x118);
    s->r_raw(p + 0x114);
    s->r_int(&f11c);
    s->r_struct(p + 0x40);
    s->r_struct(p + 0x30);
    s->r_struct(p + 0x20);
    read_soa(s, p + 0x68);
    ((Vec8*)(p + 0x128))->read(s);
}

// @ 0x51e1f0    LoadStream* read_soa(LoadStream* s, char* base)   (free function, cdecl, ret 0)
// Eight iterations: the structure at base+0x20+i*0x10, the int at base+i*4 and the block at base+0xa0.
LoadStream* read_soa(LoadStream* s, char* base)
{
    char* a = base + 0xa0;
    char* p = base + 0x20;
    for (int i = 8; i; i--, p += 0x10, base += 4) {
        s->r_struct(p);
        s->r_raw(base);
        s->r_raw(a);
    }
    return s;
}

// @ 0x51e240    void Game::load_parties(LoadStream* s)   (thiscall, ret 4)
void Game::load_parties(LoadStream* s)
{
    char* p = (char*)this;
    int n;
    s->r_raw(&n);
    if (n != nparties)
        mb_error("Load error: Number of scenes in file: %d, exceeds %d", n, nparties);
    for (int i = 0; i < nparties; i++)
        ((Vec8*)(parties + i * 0x494 + 0x474))->read(s);
}
