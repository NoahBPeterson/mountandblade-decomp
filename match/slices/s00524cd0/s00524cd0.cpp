// ===========================================================================
// mount&blade.exe (MSVC 7.1 / cl 13.10, x86-32) -- frozen batch ds1, slice 262
//
// The tstring-vector uninitialised-copy primitive used by every record reader's
// push_back/reserve path, its catch(...) handler, SceneRec::read and the scene
// record's passage/site serialiser Local::read.
// ===========================================================================
#include <new>
#include <stdlib.h>
#include "../s00429890/tstring.h"

typedef unsigned int uint;

// ---------------------------------------------------------------------------
// std::vector<tstring> internals (VC7.1, 0x10-byte element)
// ---------------------------------------------------------------------------
struct ptr_tag {};              // std::_Nonscalar_ptr_iterator_tag (empty, passed by value)
struct tstring_alloc {
    int pad;                                                    // +0 allocator subobject
    void construct(tstring* p, const tstring& v);               // 0x436c90
};

// ---------------------------------------------------------------------------
// the engine's binary stream interface (slots used by SceneRec::read)
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
    virtual stream_base& r_str(tstring* p);                     // +0x54 slot 21
};

// 0x42cc80
void __cdecl mb_error(const char* fmt, ...);

// tstring's character accessor: falls back to the buf field's own address when empty
static inline const char* cstr(const tstring& t)
{
    return t.buf ? t.buf : (const char*)&t.buf;
}

// ---------------------------------------------------------------------------
// the scene record's embedded passage/site stream (vtable slots 3, 7 and 8)
// ---------------------------------------------------------------------------
struct ser_stream {
    virtual void v00(); virtual void v01(); virtual void v02();
    virtual ser_stream& op(int* p);                             // +0x0c slot 3
    virtual void v04(); virtual void v05(); virtual void v06();
    virtual void wi(int* p);                                    // +0x1c slot 7
    virtual void wf(int* p);                                    // +0x20 slot 8
    char pad[0x40 - 0x24];
};

struct V0 { int a, b, c; char pad[0xc]; };                      // 0x18
struct V1 { int p0, p1, p2, p3, p4; char pad[0x20]; };          // 0x34
struct V2 {                                                     // 0x64
    char pad[0x28];
    int a[4];                                                   // +0x28
    int b[4];                                                   // +0x38
    int n;                                                      // +0x48
    char pad2[0x10];
    int c;                                                      // +0x5c
    char pad3[4];
};

// ---------------------------------------------------------------------------
// the scene record (0x494 bytes, vtable 0x590904)
// ---------------------------------------------------------------------------
struct Pair8 { int a; int b; };

struct SceneRec {
    char vt[0x10];                                              // +0x00
    tstring f10;                                                // +0x10
    tstring f20;                                                // +0x20
    int f30;                                                    // +0x30
    tstring f34;                                                // +0x34
    tstring f44;                                                // +0x44
    float f54, f58, f5c, f60, f64;                              // +0x54
    tstring f68;                                                // +0x68
    char padf[4];                                               // +0x78
    int f7c[0x10];                                              // +0x7c
    int fbc;                                                    // +0xbc
    int fc0[0x10];                                              // +0xc0
    int f100;                                                   // +0x100
    char pad1[0x114 - 0x104];
    Pair8 pairs114[0x40];                                       // +0x114
    int ints314[0x40];                                          // +0x314
    char pad2[0x444 - 0x414];
    char f444[0x40];                                            // ser_stream subobject at +0x444
    tstring f484;                                               // +0x484
    void read(stream_base& s);                                  // 0x524d60
};

// ---------------------------------------------------------------------------
// the temporary collection SceneRec::read fills through the embedded stream
// ---------------------------------------------------------------------------
struct Local {
    char pad0[0x1c];
    V0* v0_first; V0* v0_last;                                  // +0x1c / +0x20
    char pad1[8];
    V2* v2_first; V2* v2_last;                                  // +0x2c / +0x30
    char pad2[8];
    V1* v1_first; V1* v1_last;                                  // +0x3c / +0x40
    char pad3[0x94 - 0x44];
    unsigned size0() const { return v0_first == 0 ? 0 : (unsigned)(v0_last - v0_first); }
    unsigned size1() const { return v1_first == 0 ? 0 : (unsigned)(v1_last - v1_first); }
    unsigned size2() const { return v2_first == 0 ? 0 : (unsigned)(v2_last - v2_first); }
    Local();                                                    // 0x4d1360
    ~Local();                                                   // 0x4cb780
    void read(ser_stream& s);                                   // 0x524f00
};

// ===========================================================================
// @ 0x524cd0
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d,
                                 tstring_alloc& al, ptr_tag tag)
{
    tstring* s = d;
    try
    {
        for (; f != l; ++d, ++f)
            al.construct(d, *f);
    }
    catch (...)
    {
        for (; s != d; ++s)
            s->~tstring();
        throw;
    }
    return d;
}

// ===========================================================================
// @ 0x524d60
void SceneRec::read(stream_base& s)
{
    for (int i = 0; i < 0x40; ++i)
    {
        pairs114[i].a = -1;
        ints314[i] = 0;
    }
    s.r_str(&f10);
    s.r_str(&f20);
    s.r_int(&f30);
    s.r_str(&f34);
    s.r_str(&f44);
    s.r_float(&f54).r_float(&f58);
    s.r_float(&f5c).r_float(&f60);
    s.r_float(&f64);
    s.r_str(&f68);
    s.r_int(&fbc);
    if (fbc > 0x10)
        mb_error("Number of passages for site %s exceeds %d.", cstr(f10), 0x10);
    for (int i = 0; i < fbc; ++i)
        s.r_int(&f7c[i]);
    s.r_int(&f100);
    for (int i = 0; i < f100; ++i)
        if (i < 0x10)
            s.r_int(&fc0[i]);
    s.r_str(&f484);
    Local tmp;
    tmp.read(*(ser_stream*)f444);
}

// ===========================================================================
// @ 0x524f00
void Local::read(ser_stream& s)
{
    int n0 = (int)size0();
    s.wi(&n0);
    for (unsigned i = 0; i < size0(); ++i)
        s.op(&v0_first[i].a).op(&v0_first[i].b).op(&v0_first[i].c);

    int n1 = (int)size1();
    s.wi(&n1);
    for (unsigned i = 0; i < size1(); ++i)
    {
        V1& e = v1_first[i];
        s.wf(&e.p4);
        s.wf(&e.p0);
        s.wf(&e.p1);
        s.wf(&e.p2);
        s.wf(&e.p3);
    }

    int n2 = (int)size2();
    s.wi(&n2);
    for (unsigned i = 0; i < size2(); ++i)
    {
        V2& e = v2_first[i];
        s.wf(&e.n);
        for (int k = 0; k < e.n; ++k)
            s.wf(&e.a[k]);
        for (int k = 0; k < e.n; ++k)
            s.wf(&e.b[k]);
        s.wf(&e.c);
    }
}
