// Slice s004e8f10 (batch ds1): the game-state serialiser. Game::save_state (0x4e8f10) walks the whole game
// object in a fixed order and hands each block to the binary writer; the rest of the range is the family of
// per-block writers it (and the next slice) uses: the 8-byte element vector writer (0x4e94b0), the
// count+pointer+stride lists (0x4e90d0 / 0x4e9150 / 0x4e9550 / 0x4e9670 / 0x4e96c0 / 0x4e9790), the fixed
// arrays (0x4e9510 / 0x4e95a0 / 0x4e97e0) and the struct-of-arrays helper (0x4e9620).
// Every offset below is read off the disassembly of this range; names are provisional.
// The stream is the binary writer of the save files: slot 0xc writes an int, 0x10 writes a pointer-sized
// int, 0x14 writes a 64-bit value, 0x18 writes an 8-byte element, 0x1c writes a count, 0x20 writes raw bytes,
// 0x28 writes a C string and 0x2c writes a nested structure. Every call returns the stream (eax).
#include <windows.h>
#include <new>
#include "rgl.h"

#define MB_NOTHROW throw()

// ---- tstring (see s0042aaf0 / s004e2ea0) ----------------------------------------------------------------
#pragma pack(push, 4)
struct tstring {
    int   pos;          // +0x0
    char* buf;          // +0x4
    int   cap;          // +0x8
    int   len;          // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    void operator=(const tstring& o) MB_NOTHROW;        // 0x42bc50
    void clear() MB_NOTHROW;                            // 0x42bfb0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
#pragma pack(pop)

// ---- the writer ------------------------------------------------------------------------------------------
struct OutStream {
    virtual OutStream& dec();                       // +0x0
    virtual OutStream& inc();                       // +0x4
    virtual OutStream& niy8();                      // +0x8
    virtual OutStream& w_int(int* p);               // +0xc
    virtual OutStream& w_int4(int* p);              // +0x10
    virtual OutStream& w_int8(int* p);              // +0x14
    virtual OutStream& w_int1(int* p);              // +0x18
    virtual OutStream& w_count(int* p);             // +0x1c
    virtual OutStream& w_raw(const void* p);        // +0x20
    virtual OutStream& niy24();                     // +0x24
    virtual OutStream& w_cstr(const char* s);       // +0x28
    virtual OutStream& w_struct(const void* p);     // +0x2c
};
extern __int64 g_time1;                             // 0x5e2950 (the 64-bit clock counter)

// the record the scalar block writes: five ints (all via w_int) then raw
struct Rec5c {
    int  f0, f4, f8;
    int  f10, f14;
    __declspec(noinline) void write(OutStream* s);  // 0x4e9460 (ret 4)
};

// ---- vectors / records -----------------------------------------------------------------------------------
struct Elem8 { int a, b; };
struct List8 {                                      // vector of 8-byte elements
    int   pad0;                                     // +0x0
    Elem8* begin;                                   // +0x4
    Elem8* end;                                     // +0x8
    Elem8* endcap;                                   // +0xc
    __declspec(noinline) void write8(OutStream* out);                    // 0x4e94b0 (ret 4)
};
void write_vec8(List8* v, OutStream* out);    // the alias the other slices call

struct GameRecord {                                 // stride 0x508 (game records at +0x13a00)
    char pad0[0x3c0];
    __int64 f3c0;                                   // +0x3c0
    int  f3c8;                                      // +0x3c8
    char pad3cc[0x508 - 0x3cc];
    __declspec(noinline) void write(OutStream* out);                     // 0x4e9850 (next slice; ret 4)
};

struct RecB0 {                                      // stride 0xb0 (the 0x5e29f8 array)
    char  pad0[0x34];
    int   f34, f38, f3c;
    char  pad40[0xa0 - 0x40];
    List8 f_a0;                                     // +0xa0
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e9510 (ret 4)
};

struct Rec138 {                                     // stride 0x138 (at +0x13a58)
    char  pad0[0x20];
    char  f20[0x10];
    char  f30[0x10];
    char  f40[0x24];
    int   f64;
    char  pad68[0x114 - 0x68];
    int   f114, f118, f11c;
    char  pad120[0x128 - 0x120];
    List8 f128;                                     // +0x128
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e95a0 (ret 4)
};

struct Rec300 {                                     // stride 0x300 (at +0x13a48)
    char  pad0[0x10];
    int   f10;
    char  pad14[0x28 - 0x14];
    int   f28;                                      // +0x28 (count)
    char  f2c[0x230 - 0x2c];                        // +0x2c (n ints)
    int   f230;
    char  pad234[0x24c - 0x234];
    List8 f24c;                                     // +0x24c
    char  f25c[0xa4];                               // +0x25c (the struct-of-arrays base)
    __declspec(noinline) void  write300(OutStream* out, int n);          // 0x4e9710 (ret 8)
};

struct Rec78 {                                      // stride 0x78 (at +0x139e8)
    char  pad0[0xc];
    int   f0c;
    char  pad10[0x50 - 0x10];
    int   f50;
    char  pad54[0x60 - 0x54];
    int   f60;
    char  pad64[0x70 - 0x64];
    int   f70;
    char  pad74[4];
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e9110 (ret 4)
};

struct Rec78Tag {                                   // count +0x0, array +0x4
    int   count;
    Rec78* array;
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e90d0 (ret 4)
};

struct Rec30Tag {                                   // count +0x0, array +0x4, stride 0x30
    int   count;
    char* array;
    __declspec(noinline) void  write(OutStream* out);                    // 0x4e9150 (ret 4)
};

// ---- the game object (see s00473a60 / s004e85d0) ---------------------------------------------------------
#pragma pack(push, 4)
struct Game {
    int          version;                           // +0x0
    char         pad04[0x8c - 0x4];
    int          f8c;                               // +0x8c (index of the current party)
    char         pad90[0x13a00 - 0x90];
    GameRecord*  records;                           // +0x13a00
    int          nrecords;                          // +0x13a04
    char         pad13a08[0x13a20 - 0x13a08];
    int          f13a20;
    char         pad13a24[0x13a48 - 0x13a24];
    Rec300*      f13a48;                            // +0x13a48 (array of 0x300-byte entries)
    int          n13a4c;                            // +0x13a4c
    RecB0*       parties;                           // +0x13a50 (array of 0x494-byte records)
    int          nparties;                          // +0x13a54
    Rec138*      f13a58;                            // +0x13a58 (array of 0x138-byte entries)
    int          n13a5c;                            // +0x13a5c
    char         pad13a60[0x13a64 - 0x13a60];
    char*        f13a64;                            // +0x13a64 (array of 0x5c-byte entries)
    char*        f13a68;                            // +0x13a68
    char         pad13a6c[4];
    char*        f13a70;                            // +0x13a70
    char         pad13a74[0x13a88 - 0x13a74];
    int          f13a88;
    char         pad13a8c[0x13aa0 - 0x13a8c];
    int          f13aa0;
    char         pad13aa4[0x13ad0 - 0x13aa4];
    int          f13ad0, f13ad4, f13ad8, f13adc;
    int          f13ae0, f13ae4, f13ae8, f13aec;
    int          f13af0;
    int          f13af4, f13af8, f13afc;
    char         pad13b00[0x13b20 - 0x13b00];
    int          f13b20;
    char         pad13b24[4];
    int          f13b28, f13b2c, f13b30, f13b34, f13b38, f13b3c, f13b40, f13b44, f13b48;
    char         pad13b4c[0x13b50 - 0x13b4c];
    tstring      f13b50;
    int          f13b60;
    char         pad13b64[0x1a1d8 - 0x13b64];
    int          f1a1d8, f1a1dc, f1a1e0, f1a1e4, f1a1e8, f1a1ec;
    int save_state(OutStream* out);                 // 0x4e8f10
    __declspec(noinline) int  write_clock(OutStream* out);               // 0x4e9190
    __declspec(noinline) void write_vec5c(OutStream* out);               // 0x4e93f0
    __declspec(noinline) void write_list138(OutStream* out);             // 0x4e9550
    __declspec(noinline) void write_list_party(OutStream* out);          // 0x4e9670
    __declspec(noinline) void write_list300(OutStream* out);             // 0x4e96c0
    __declspec(noinline) void write_records(OutStream* out);             // 0x4e97e0
    __declspec(noinline) void write_items(OutStream* out);               // 0x4e9790
    void write_other2(OutStream* out);              // 0x4e9b50 (next slice)
};

struct Table2 {                                     // the table at Game+0xcc
    void write(OutStream* out);                     // 0x4e99d0 (next slice)
};
#pragma pack(pop)

extern Game* g_game;                                // 0x5e2930

extern List8 g_list_a70480;                         // 0xa70480
extern RecB0* g_29f8;                               // 0x5e29f8 (array of 0xb0-byte records)
extern int   g_n29fc;                               // 0x5e29fc
struct ItemKind { char pad0[0x30]; List8 l30; char pad40[0x238 - 0x40]; };
extern ItemKind* g_items;                           // 0x5e29d0 (array of 0x238-byte item kinds)
extern int   g_n_items;                             // 0x5e29d4

// ---- the sub-writers ------------------------------------------------------------------------------------
OutStream* write_soa(OutStream* out, char* base);   // 0x4e9620 (free, two stack args)

// @ 0x4e8f10    int Game::save_state(OutStream* out)   (thiscall, ret 4)
int Game::save_state(OutStream* out)
{
    __int64 counter = g_time1;
    out->w_int8((int*)&counter);
    out->w_raw((char*)this + 4);
    out->w_raw((char*)this + 0xb4);
    write_clock(out);
    g_list_a70480.write8(out);
    ((Rec78Tag*)((char*)this + 0x139e8))->write(out);
    ((Rec30Tag*)((char*)this + 0x139f4))->write(out);
    write_list138(out);
    write_list_party(out);
    write_list300(out);
    write_vec5c(out);
    out->w_raw(&g_n29fc);
    for (int i = 0; i < g_n29fc; i++)
        g_29f8[i].write(out);
    write_other2(out);
    ((Table2*)((char*)this + 0xcc))->write(out);
    write_records(out);
    write_items(out);
    out->w_int8((int*)&records[f8c].f3c0);
    out->w_int8((int*)&records[f8c].f3c8);
    out->w_raw((char*)this + 0x98);
    out->w_raw((char*)this + 0xa0);
    out->w_raw((char*)this + 0x9c);
    out->w_raw((char*)this + 0xa4);
    return 0;
}

// @ 0x4e90d0    void Rec78Tag::write(OutStream* out)   (thiscall, ret 4)
void Rec78Tag::write(OutStream* out)
{
    out->w_raw(this);
    for (int i = 0; i < count; i++)
        array[i].write(out);
}

// @ 0x4e9110    void Rec78::write(OutStream* out)   (thiscall, ret 4)
void Rec78::write(OutStream* out)
{
    out->w_raw((char*)this + 0xc);
    out->w_int1(&f50);
    out->w_int1(&f60);
    out->w_int1(&f70);
}

// @ 0x4e9150    void Rec30Tag::write(OutStream* out)   (thiscall, ret 4)
void Rec30Tag::write(OutStream* out)
{
    out->w_raw(this);
    for (int i = 0; i < count; i++)
        out->w_int1((int*)(array + i * 0x30 + 0x28));
}

// @ 0x4e9190    int Game::write_clock(OutStream* out)   (thiscall, ret 4)
int Game::write_clock(OutStream* out)
{
    char* p = (char*)this;
    out->w_int1(&f13b20);
    out->w_raw(p + 0x13b28);
    out->w_raw(p + 0x13b2c);
    out->w_raw(p + 0x13b30);
    out->w_raw(p + 0x13b34);
    out->w_raw(p + 0x13b38);
    out->w_raw(p + 0x13b60);
    out->w_int(&f13b3c);
    out->w_int(&f13b40);
    out->w_int(&f13b44);
    out->w_int(&f13b48);
    out->w_struct(p + 0x13a70);
    out->w_int4(&f13aa0);
    out->w_raw(p + 0x13ae8);
    out->w_raw(p + 0x13a20);
    out->w_raw(p + 0x13a0c);
    out->w_raw(p + 0x13a10);
    out->w_raw(p + 0x13aec);
    out->w_raw(p + 0x13af4);
    out->w_raw(p + 0x13af8);
    out->w_raw(p + 0x13afc);
    out->w_raw(p + 0x13ae0);
    out->w_raw(p + 0x13ae4);
    tstring tmp = f13b50;
    tmp.clear();
    if (!tmp.len) {
        static const char kEmpty[] = "";
        out->w_cstr(kEmpty);
    } else {
        out->w_struct(&tmp);
    }
    out->w_raw(p + 0x1a1d8);
    out->w_raw(p + 0x1a1dc);
    out->w_raw(p + 0x1a1e0);
    out->w_raw(p + 0x1a1e4);
    out->w_raw(p + 0x1a1e8);
    out->w_raw(p + 0x1a1ec);
    out->w_int1(&f13a88);
    out->w_int(&f13ad0);
    out->w_raw(p + 0x13adc);
    out->w_raw(p + 0x13ad8);
    out->w_raw(p + 0x13ad4);
    return 0;
}

// @ 0x4e93f0    void Game::write_vec5c(OutStream* out)   (thiscall, ret 4)   vector at +0x13a64, stride 0x5c
void Game::write_vec5c(OutStream* out)
{
    int n;
    if (!f13a64)
        n = 0;
    else
        n = (int)(f13a68 - f13a64) / 0x5c;
    out->w_count(&n);
    for (char* r = f13a64; r < f13a68; r += 0x5c)
        ((Rec5c*)r)->write(out);
}

// @ 0x4e9460    void Rec5c::write(OutStream* s)   (thiscall, ret 4)
void Rec5c::write(OutStream* s)
{
    char* p = (char*)this;
    s->w_int((int*)(p + 0)).w_int((int*)(p + 4)).w_int((int*)(p + 8));
    s->w_int((int*)(p + 0x10));
    s->w_int((int*)(p + 0x14)).w_raw(p + 0x18);
}

// @ 0x4e94b0    void List8::write8(OutStream* out)   (thiscall, ret 4)
void List8::write8(OutStream* out)
{
    int n;
    if (!begin)
        n = 0;
    else
        n = (int)(end - begin);
    out->w_count(&n);
    for (unsigned i = 0; i < (!begin ? 0u : (unsigned)(end - begin)); i++)
        out->w_int1((int*)&begin[i]);
}
void write_vec8(List8* v, OutStream* out) { v->write8(out); }

// @ 0x4e9510    void RecB0::write(OutStream* out)   (thiscall, ret 4)
void RecB0::write(OutStream* out)
{
    out->w_raw(&f34);
    out->w_raw(&f38);
    out->w_raw(&f3c);
    f_a0.write8(out);
}

// @ 0x4e9550    void Game::write_list138(OutStream* out)   (thiscall, ret 4)
void Game::write_list138(OutStream* out)
{
    out->w_raw(&n13a5c);
    for (int i = 0; i < n13a5c; i++)
        f13a58[i].write(out);
}

// @ 0x4e95a0    void Rec138::write(OutStream* out)   (thiscall, ret 4)
void Rec138::write(OutStream* out)
{
    out->w_raw(&f64);
    out->w_raw(&f118);
    out->w_raw(&f114);
    out->w_int(&f11c);
    out->w_struct(&f40);
    out->w_struct(&f30);
    out->w_struct(&f20);
    write_soa(out, (char*)this + 0x68);
    f128.write8(out);
}

// @ 0x4e9620    OutStream* write_soa(OutStream* out, char* base)   (free function, ret 0)
// Eight iterations: the struct at base+0x20+i*0x10, the int at base+i*4 and the block at base+0xa0.
struct Soa8 { int v[8]; char s[8][16]; int blk[1]; };
OutStream* write_soa(OutStream* out, char* base)
{
    Soa8* b = (Soa8*)base;
    int* blk = b->blk;
    for (int i = 0; i < 8; i++) {
        out->w_struct(b->s[i]);
        out->w_raw(&b->v[i]);
        out->w_raw(blk);
    }
    return out;
}

// @ 0x4e9670    void Game::write_list_party(OutStream* out)   (thiscall, ret 4)
void Game::write_list_party(OutStream* out)
{
    out->w_raw(&nparties);
    for (int i = 0; i < nparties; i++)
        ((List8*)((char*)parties + i * 0x494 + 0x474))->write8(out);
}

// @ 0x4e96c0    void Game::write_list300(OutStream* out)   (thiscall, ret 4)
void Game::write_list300(OutStream* out)
{
    out->w_raw(&n13a4c);
    for (int i = 0; i < n13a4c; i++)
        f13a48[i].write300(out, n13a4c);
}

// @ 0x4e9710    void Rec300::write300(OutStream* out, int n)   (thiscall, ret 8)
void Rec300::write300(OutStream* out, int n)
{
    f24c.write8(out);
    for (int i = 0; i < n; i++)
        out->w_int((int*)(f2c + i * 4));
    out->w_struct(this);
    out->w_int4(&f10);
    out->w_count(&f28);
    out->w_raw((char*)this + 0x230);
    write_soa(out, (char*)this + 0x25c);
}

// @ 0x4e9790    void Game::write_items(OutStream* out)   (thiscall, ret 4)   item kinds at 0x5e29d0
void Game::write_items(OutStream* out)
{
    out->w_raw(&g_n_items);
    for (int i = 0; i < g_n_items; i++)
        write_vec8(&g_items[i].l30, out);
}

// @ 0x4e97e0    void Game::write_records(OutStream* out)   (thiscall, ret 4)
void Game::write_records(OutStream* out)
{
    out->w_raw(&nrecords);
    for (int i = 0; i < nrecords; i++)
        records[i].write(out);
    char* q = (char*)0x8ee3138;
    int   qend = 0x8ee3c60;
    while ((int)q < qend) {
        out->w_raw(q);
        q += 0x44;
    }
}