// Slice s004da410: the loading-screen module.  The 0x1f8-byte loading-configuration record (copy routine
// 0x4da410 and two std::vector range copies), the end-of-game shutdown routine (0x4da5b0), two array-container
// destructors (0x4da840, 0x4da9c0), the overlay builder (0x4da900) and the overlay initialiser (0x4daa10).
// Names and layouts are provisional; every offset is read off the disassembly of this slice.
#pragma warning(disable:4234)     // __thiscall on a free function
#include <new>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rgl.h"

// ---- engine string (16 bytes, see s0042a1f0/tstring.h) --------------------------------------------------
struct tstring {
    tstring& operator=(const tstring& o);       // 0x42bc50
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void append(const tstring& o);              // 0x42c1e0
};
struct E18   { int a, b, c, d, e, f; void dummy_dtor(); };  // 0x18 bytes; dtor 0x42bc40 with this = elem

// ---- record parts of the loading record (shapes read off 0x4da410's callees) ----------------------------
// std::vector<T> == { allocator, begin, end, end_of_storage }; the helpers below are the out-of-line copies
// the original calls (this in ecx, source pushed, ret 4).
struct vec1c {                                  // 0x10-byte elements (0x4cc910 / 0x4ccc00, asserted below)
    void* alloc; void* first; void* last; void* end;
    vec1c() {}
    vec1c& operator=(const vec1c& o);           // 0x4cc910
};
struct vec10 {                                  // 0x10-byte elements, memberwise copy ctor 0x4ccc00
    void* alloc; void* first; void* last; void* end;
    vec10() {}
    vec10& operator=(const vec10& o);           // 0x4ccc00
};
struct vec1c8 {                                 // 0x1c8-byte elements, copy ctor 0x4d9840
    void* alloc; void* first; void* last; void* end;
    vec1c8() {}
    vec1c8& operator=(const vec1c8& o);         // 0x4d9840
};
struct four   { int a, b, c, d; four& operator=(const four& o); };    // 0x10 bytes (copy 0x4cca70)
struct four2  { int a, b, c, d; four2& operator=(const four2& o); };  // 0x10 bytes (copy 0x4ccc00)
struct arr64  { int v[0x40]; };                                        // 0x100-byte inline table
struct E4r    { void* p; };                     // 4-byte element
struct E20    { tstring s; char* p10; int cap14, end18, last1c; }; // 0x20 bytes (s004d0c20 flora_kind)

// The out-of-line copies the loading record's members use.  They are __thiscall with the destination as the
// instance and the source as the only stack argument; a one-argument __fastcall is that calling convention.
void __fastcall copy_four(char* dst, four& out);       // 0x4cca70
void __fastcall copy_vec10(char* dst, int unused, const vec10& src);     // 0x4ccc00
void __fastcall copy_vec1c(char* dst, int unused, const vec1c& src);     // 0x4cc910
void __fastcall copy_vec1c8(char* dst, int unused, vec1c8& src);   // 0x4d9840

// The loading record (0x1f8 bytes).  Two engine strings, plain fields, three copies of the same 0x10-byte
// record, three std::vectors (0x10, 0x10 and 0x1c8-byte elements), a 0x100-byte inline table and a tail.
struct loading_config {
    tstring s0;                     // +0x00
    int v10, v14, v18, v1c, v20, v24;
    tstring s28;                    // +0x28
    int v38, v3c, v40, v44;
    four    r48;                    // +0x48 (0x4cca70)
    four    r58;                    // +0x58
    four    r68;                    // +0x68
    four2   r78;                    // +0x78 (0x4ccc00)
    int v88;                        // +0x88
    arr64   r8c;                    // +0x8c (0x100 bytes)
    int v18c, v190, v194, v198;     // +0x18c
    vec1c   r19c;                   // +0x19c (copy ctor 0x4cc910)
    vec1c   r1ac;                   // +0x1ac
    vec1c8  r1bc;                   // +0x1bc (0x1c8-byte elements, 0x4d9840)
    int v1cc, v1d0, v1d4, v1d8, v1dc, v1e0, v1e4, v1e8, v1ec, v1f0, v1f4;
    loading_config& operator=(const loading_config& src);     // 0x4da410
};

// ---- generic widget views: only the virtual slots used here are named -----------------------------------
struct WidgetA {
    virtual void a00(int); virtual void a01(int); virtual void a02(int); virtual void a03(int);
    virtual void a04(int); virtual void a05(int); virtual void a06(int); virtual void a07(int);
    virtual void a08(int); virtual void a09(int); virtual void a10(int); virtual void a11(int);
    virtual void a12(int); virtual void a13(int); virtual void a14(int); virtual void a15(int);
    virtual void a16(int); virtual void a17(int); virtual void a18(int); virtual void a19(int);
    virtual void a20(int); virtual void a21(int); virtual void a22(int); virtual void a23(int);
    virtual void a24(int); virtual void a25(int); virtual void a26(int); virtual void a27(int);
    virtual void a28(int); virtual void a29(int); virtual void a30(int); virtual void a31(int);
    virtual void a32(int); virtual void a33(int); virtual void a34(int); virtual void a35(int);
    virtual void a36(int); virtual void a37(int); virtual void a38(int); virtual void a39(int);
    virtual void a40(int); virtual void a41(int); virtual void a42(int); virtual void a43(int);
    virtual void a44(int); virtual void a45(int); virtual void a46(int); virtual void a47(int);
    virtual void a48(int); virtual void a49(int); virtual void a50(int); virtual void a51(int);
    virtual void a52(int); virtual void a53(int); virtual void a54(int); virtual void a55(int);
    virtual void a56(int); virtual void a57(int); virtual void a58(int); virtual void a59(int);
    virtual void a60(int); virtual void a61(int); virtual void a62(int); virtual void a63(int);
    virtual void a64(int); virtual void a65(int); virtual void a66(int); virtual void a67(int);
    virtual void a68(int); virtual void a69(int); virtual void a70(int); virtual void a71(int);
    virtual void a72(int); virtual void a73(int); virtual void a74(int); virtual void a75(int);
    virtual void a76(int); virtual void a77(int); virtual void a78(int); virtual void a79(int);
    virtual void a80(int);
};

// widget slot views: the same objects reached through different bases in the two functions
struct WidgetB {                                // 0x4daa10 slots: 0x58, 0x64, 0x74, 0x7c, 0x98, 0xa4, 0xc8
    virtual void b00(int); virtual void b01(int); virtual void b02(int); virtual void b03(int);
    virtual void b04(int); virtual void b05(int); virtual void b06(int); virtual void b07(int);
    virtual void b08(int); virtual void b09(int); virtual void b10(int); virtual void b11(int);
    virtual void b12(int); virtual void b13(int); virtual void b14(int); virtual void b15(int);
    virtual void b16(int); virtual void b17(int); virtual void b18(int); virtual void b19(int);
    virtual void b20(int); virtual void b21(int);
    virtual void set_pos(const float* p);       // +0x58
    virtual void b23(int); virtual void b24(int);
    virtual void set_scale(const float* p);     // +0x64
    virtual void b26(int); virtual void b27(int);
    virtual void b28(int); virtual void b29(int);
    virtual void set_flag(const int* f);        // +0x74
    virtual void b31(int); virtual void b32(int); virtual void b33(int); virtual void b34(int);
    virtual void b35(int); virtual void b36(int);
    virtual void set_mesh(void* m, int zero);   // +0x7c
    virtual void b38(int); virtual void b39(int); virtual void b40(int); virtual void b41(int);
    virtual void b42(int); virtual void b43(int); virtual void b44(int); virtual void b45(int);
    virtual void b46(int); virtual void b47(int); virtual void b48(int); virtual void b49(int);
    virtual void b50(int);
    virtual void set_color(const int* c);       // +0x98
    virtual void b52(int); virtual void b53(int); virtual void b54(int); virtual void b55(int);
    virtual void b56(int);
    virtual void set_text(const void* a, const void* b);    // +0xa4
    virtual void b58(int); virtual void b59(int); virtual void b60(int); virtual void b61(int);
    virtual void b62(int); virtual void b63(int); virtual void b64(int); virtual void b65(int);
    virtual void b66(int); virtual void b67(int); virtual void b68(int); virtual void b69(int);
    virtual void b70(int); virtual void b71(int); virtual void b72(int); virtual void b73(int);
    virtual void b74(int); virtual void b75(int); virtual void b76(int); virtual void b77(int);
    virtual void b78(int);
    virtual void add_child(void* c);            // +0xc8
};

// the hint bar made by 0x4dada0 (slots 0x74 and 0xa4 are used by 0x4daa10).  0x60 bytes, the position string
// first; 0x4e43e0 copies a whole hint into the manager's table (at +2c*0x60) and moves the string there.
struct WidgetC {
    virtual void c00(int); virtual void c01(int); virtual void c02(int); virtual void c03(int);
    virtual void c04(int); virtual void c05(int); virtual void c06(int); virtual void c07(int);
    virtual void c08(int); virtual void c09(int); virtual void c10(int); virtual void c11(int);
    virtual void c12(int); virtual void c13(int); virtual void c14(int); virtual void c15(int);
    virtual void c16(int); virtual void c17(int); virtual void c18(int); virtual void c19(int);
    virtual void c20(int); virtual void c21(int); virtual void c22(int); virtual void c23(int);
    virtual void c24(int); virtual void c25(int); virtual void c26(int); virtual void c27(int);
    virtual void c28(int); virtual void c29(int);
    virtual void set_text(const void* a, const void* b);    // +0xa4
};

// the overlay window (vptr 0x592848, ctor 0x4931a0, 0x190 bytes; see s0049aca0)
struct Overlay {
    virtual void n00(int); virtual void n01(int); virtual void n02(int); virtual void n03(int);
    virtual void n04(int); virtual void n05(int); virtual void n06(int); virtual void n07(int);
    virtual void n08(int); virtual void n09(int); virtual void n10(int); virtual void n11(int);
    virtual void n12(int); virtual void n13(int); virtual void n14(int); virtual void n15(int);
    virtual void n16(int); virtual void n17(int); virtual void n18(int); virtual void n19(int);
    virtual void n20(int); virtual void n21(int);
    virtual void set_pos(const float* p);       // +0x58
    virtual void n23(int); virtual void n24(int);
    virtual void set_scale(const float* p);     // +0x64
    virtual void n26(int); virtual void n27(int); virtual void n28(int); virtual void n29(int);
    virtual void set_flag(const int* f);        // +0x74
    virtual void n31(int);
    virtual void set_mesh(void* m, int zero);   // +0x7c
    virtual void n33(int); virtual void n34(int); virtual void n35(int); virtual void n36(int);
    virtual void n37(int); virtual void n38(int); virtual void n39(int); virtual void n40(int);
    virtual void n41(int); virtual void n42(int); virtual void n43(int); virtual void n44(int);
    virtual void n45(int); virtual void n46(int); virtual void n47(int); virtual void n48(int);
    virtual void n49(int);
    virtual void set_color(const int* c);       // +0x98
    virtual void n51(int); virtual void n52(int); virtual void n53(int); virtual void n54(int);
    virtual void n55(int); virtual void n56(int); virtual void n57(int); virtual void n58(int);
    virtual void n59(int); virtual void n60(int); virtual void n61(int); virtual void n62(int);
    virtual void n63(int); virtual void n64(int); virtual void n65(int); virtual void n66(int);
    virtual void n67(int); virtual void n68(int); virtual void n69(int); virtual void n70(int);
    virtual void n71(int); virtual void n72(int); virtual void n73(int); virtual void n74(int);
    virtual void n75(int); virtual void n76(int); virtual void n77(int); virtual void n78(int);
    virtual void n79(int); virtual void n80(int); virtual void n81(int); virtual void n82(int);
    virtual void n83(int); virtual void n84(int); virtual void n85(int); virtual void n86(int);
    virtual void n87(int); virtual void n88(int); virtual void n89(int); virtual void n90(int);
    virtual void n91(int); virtual void n92(int); virtual void n93(int); virtual void n94(int);
    virtual void n95(int); virtual void n96(int); virtual void n97(int);
    virtual void add_child(void* c);            // +0xc8
};

// the manager's hint table is laid out in 0x50-entry groups: entry i of a group starts at group+0x10*i, so each
// record is 0x10 bytes with its position string at +0 and the widget itself behind it
struct HintRec {
    tstring pos;                    // +0x00 (copied from the hint's own string, 0x42bc50)
};

// the loading-screen manager (`this` of 0x4da900 and 0x4daa10)
struct LoadingScreen {
    char pad0[0x2c];
    Overlay* overlay;               // +0x2c
    WidgetC* hint;                  // +0x30
    WidgetB* text;                  // +0x34
    WidgetB* logo;                  // +0x38
    int f3c;                        // +0x3c
    int f40;                        // +0x40
    char pad44[0x568 - 0x44];
    void* f568;                     // +0x568 (loading mesh)
    char pad56c[0x7d0 - 0x56c];
    HintRec pos[0x50];              // +0x7d0 (per-hint table entry, 0x10 bytes each)
    unsigned char flags[0x50];      // +0xcd0
    int ids[0x40];                  // +0xd20
    int id2[0x40];                  // +0xe60
    char padEA0[0x7c0];             // 0xe60 + 0x7c0 = 0x1620
    int f16e0;                      // +0x16e0
    int f16e4;                      // +0x16e4
    void rebuild();                 // 0x4da900
    void init();                    // 0x4daa10
};

struct E20Vec { char pad0[4]; char* first; char* last; char* end; };    // vector with an 0x20-byte element
struct var_mesh {
    char pad[0x260];
    var_mesh();                                     // 0x412a90
};
struct RecVec { char pad0[4]; char* first; char* last; char* end; char pad10[4]; char* f14; int f18, f1c; };

// ---- callees -------------------------------------------------------------------------------------------
namespace rgl {
struct mesh {
    char pad[0x260];
    mesh();                                     // 0x412a90
};
}
// 0x4e43e0: append one hint (its position string, id, flag) to the loading window (a member, ret 0x10)
struct ScreenRef {
    void refresh(tstring* pos, int id, int id2, char flag);   // 0x4e43e0
};
void rec20_dtor(E20* first, E20* last, E20Vec& a, E20Vec& b);         // 0x4d10f0 (reads first/last only)
void __fastcall four_mem3(const four& src, int unused, four& out);    // 0x4cca70
void __fastcall four_mem3b(int dummy, four src, four& dst);   // 0x4ccc00
void rec4_dtor(void* first, void* last, RecVec& a, RecVec& b);   // 0x4cd570 (array dtor of 4-byte records)
void str_list_clear(void* p);                   // 0x4c4d80
void item_array_dtor(void* p, int flags);       // 0x4c4f40
void flora_array_dtor(void* p, int flags);      // 0x4c3430
void ground_array_dtor(void* p, int flags);     // 0x4c7ee0
void mission_array_dtor(void* p, int flags);    // 0x4c8980
void audio_close();                             // winmm!FSOUND_Close
void crt_exit();                                // 0x535458
struct Game  { ~Game(); };                      // 0x492f40
struct World { ~World(); };                     // 0x4d51d0
void options_save();                            // 0x4181c0 (this = 0x5aa008)
void game_destroy();                            // 0x492f40 (this = g_game)
void world_destroy();                           // 0x4d51d0 (this = g_5e292c)
void shutdown_3();                              // 0x535458
void gui_add_root(void* g, void* p);            // 0x49b120
void* resmgr_get(void* r, const tstring& n);    // 0x419540 (this = 0x5d6fa0)
const tstring* strtab_lookup(void* t, const tstring& n);    // 0x43a1e0 (this = 0x5e2a18)
char* crt_itoa(int v, char* buf, int radix);    // 0x5371f7 (__itoa)
void __fastcall ctor_4931a0(void* p);                      // 0x4931a0
void ctor_48ae20(void* p);                      // 0x48ae20
void* clone_41c6d0(void* m);                    // 0x41c6d0
WidgetC* __fastcall make_hint(tstring& text, int unused, Overlay* o, float a, float b, float c, float d);  // 0x4dada0
extern "C" __declspec(dllimport) unsigned long __stdcall timeGetTime(void);   // 0x5493e8
int crt_rand();                                 // 0x536a9f
void crt_srand(unsigned seed);                  // 0x536a92
void tstring_dtor(char* p);                     // 0x42bc40 (this = p)

// ---- globals (addresses; names provisional) ------------------------------------------------------------
struct MsgLog {
    virtual void m00(int); virtual void m01(int); virtual void m02(int); virtual void m03(int);
    virtual void m04(int); virtual void m05(int); virtual void m06(int); virtual void m07(int);
    virtual void m08(int); virtual void m09(int);
    virtual void message(const char* s);        // +0x28
    virtual void m11(int); virtual void m12(int); virtual void m13(int); virtual void m14(int);
    virtual void m15(int); virtual void m16(int); virtual void m17(int); virtual void m18(int);
    virtual void m19(int); virtual void m20(int); virtual void m21(int); virtual void m22(int);
    virtual void m23(int); virtual void m24(int); virtual void m25(int);
    virtual int  m26();                         // +0x68
};

struct GuiMgr { void reset(); void add_root(void* p); };
struct ResMgr { void* get_object(const tstring& n); };
struct StrTab { const tstring* lookup(const tstring& n); };

#define g_gui     (*(GuiMgr*)0x5d7080)
#define g_resmgr  (*(ResMgr*)0x5d6fa0)
#define g_strtab  (*(StrTab*)0x5e2a18)
#define g_a18140  (*(int*)0xa18140)
#define g_5d8788  (*(char*)0x5d8788)
extern void* g_game;                            // 0x5e2930
extern void* g_5e292c;                          // 0x5e292c
extern char* g_5e29d0;                          // 0x5e29d0
extern void* g_5e29b0;                          // 0x5e29b0
extern char* g_5e2928;                          // 0x5e2928
extern void* g_5e2a00;                          // 0x5e2a00
extern void* g_5e29dc;                          // 0x5e29dc
extern char* g_5e28f8;                          // 0x5e28f8
extern char* g_5e28f4;                          // 0x5e28f4
extern char* g_5e29f0;                          // 0x5e29f0
extern void* g_5e29f8;                          // 0x5e29f8
extern WidgetA* g_5e2934;                       // 0x5e2934 (font object)
struct Timer;
extern Timer* g_timers[30];                     // 0xa73980 .. 0xa739f8
extern char g_5a9f94;                           // 0x5a9f94
extern char g_5a9f95;                           // 0x5a9f95
extern FILE* g_5a9f90;                          // 0x5a9f90

// ---- element types of the global arrays torn down by 0x4da5b0 ------------------------------------------
struct E24   { int a, b; tstring s; };                          // 0x18 bytes, string at +8
struct RecA  { virtual void r0(int); virtual ~RecA(); };        // vector deleting dtor 0x431960
struct DelA  {
    void del3(int flags);                           // 0x431960
};
struct E20b  { tstring s; char pad[0xc]; RecA* p;               // 0x20 bytes, RecA* at +0x1c
               ~E20b() { if (p) ((DelA*)p)->del3(3); } };
// @ 0x4da840  (std::vector<E20b>::_Tidy)
template class std::vector<E20b>;                                // emits _Tidy == 0x4da840
struct E4C   { tstring s; char pad[0x3c]; };                    // 0x4c bytes
struct S3c   { char pad[0xb]; ~S3c(); };                        // 0xc bytes at +0x3c (dtor 0x4317b0)
struct E48   { tstring s0; char pad[4]; tstring s14; char pad2[0x18]; S3c m3c; };  // 0x48 bytes
struct Item238 { char pad[0x238]; ~Item238(); };                // dtor 0x4c8a40
struct Del   { void del3(int flags); };                         // 0x4c4f40 (ecx = object)
struct DelB  {
    void del3(int flags);                                       // 0x4c3430
};
struct DelC  {
    void del3(int flags);                                       // 0x4c7ee0
};
struct DelD  {
    void del3(int flags);                                       // 0x4c8980
};
struct L     {
    void clear();                                               // 0x4c4d80
};
struct Timer { virtual ~Timer(); };                             // delete -> vtable slot 0, flags 1
struct Font {
    char pad0[0x44];
    struct TT {
        int   f0;                   // +0x44
        char  lst[4];               // +0x48 (std::list<int>, 0x4c4d80)
        void* p4c;                  // +0x4c
        int   f50;                  // +0x50
        int   f54;                  // +0x54
        void* b58;                  // +0x58 (vector begin; 0x58/0x5c/0x60)
        void* e5c;
        void* c60;
        ~TT() {
            if (b58)
                operator delete(b58);
            b58 = 0;
            e5c = 0;
            c60 = 0;
            ((L*)lst)->clear();
            operator delete(p4c);
            p4c = 0;
        }
    } t;
};

// @ 0x4da410
loading_config& loading_config::operator=(const loading_config& src)
{
    s0 = src.s0;
    v10 = src.v10;
    v14 = src.v14;
    v18 = src.v18;
    v1c = src.v1c;
    v20 = src.v20;
    v24 = src.v24;
    s28 = src.s28;
    v38 = src.v38;
    v3c = src.v3c;
    v40 = src.v40;
    v44 = src.v44;
    r48 = src.r48;
    r58 = src.r58;
    r68 = src.r68;
    r78 = src.r78;
    v88 = src.v88;
    for (int i = 0; i < 0x40; i++)
        r8c.v[i] = src.r8c.v[i];
    v18c = src.v18c;
    v190 = src.v190;
    v194 = src.v194;
    v198 = src.v198;
    r19c = src.r19c;
    r1ac = src.r1ac;
    r1bc = src.r1bc;
    v1cc = src.v1cc;
    v1d0 = src.v1d0;
    v1d4 = src.v1d4;
    v1d8 = src.v1d8;
    v1dc = src.v1dc;
    v1e0 = src.v1e0;
    v1e4 = src.v1e4;
    v1e8 = src.v1e8;
    v1ec = src.v1ec;
    v1f0 = src.v1f0;
    v1f4 = src.v1f4;
    return *this;
}

// @ 0x4da5b0
int shutdown()
{
    audio_close();
    (*(MsgLog*)0x5a9f60).message("Saving options...");
    options_save();
    if (g_game) {
        delete (Game*)g_game;
        g_game = 0;
    }
    if (g_5e292c) {
        delete (World*)g_5e292c;
        g_5e292c = 0;
    }
    if (g_5e29d0) {
        delete[] (Item238*)g_5e29d0;
    }
    if (g_5e29b0) {
        ((Del*)g_5e29b0)->del3(3);              // ??_E, flags 3 (0x4c4f40)
    }
    if (g_5e2928) {
        delete[] (E24*)g_5e2928;
    }
    if (g_5e2a00) {
        ((DelB*)g_5e2a00)->del3(3);            // 0x4c3430
    }
    if (g_5e29dc) {
        ((DelC*)g_5e29dc)->del3(3);            // 0x4c7ee0
    }
    if (g_5e28f8) {
        delete[] (E20b*)g_5e28f8;
    }
    if (g_5e28f4) {
        delete[] (E4C*)g_5e28f4;
    }
    if (g_5e29f0) {
        delete[] (E48*)g_5e29f0;
    }
    if (g_5e29f8) {
        ((DelD*)g_5e29f8)->del3(3);            // 0x4c8980
    }
    for (int i = 0; i < 30; i++) {
        delete g_timers[i];
        g_timers[i] = 0;
    }
    if (g_5e2934) {
        Font* f = (Font*)g_5e2934;
        f->t.~TT();
        operator delete(f);
    }
    crt_exit();
    if (g_5a9f95) {
        if ((*(MsgLog*)0x5a9f60).m26() == 0)
            fclose(g_5a9f90);
        g_5a9f94 = 1;
        g_5a9f95 = 0;
    }
    return 0;
}

// @ 0x4da880
loading_config* copy_configs_forward(loading_config* first, loading_config* last, loading_config* out)
{
    if (first != last) {
        do {
            *out = *first;
            first++;
            out++;
        } while (first != last);
    }
    return out;
}

// @ 0x4da8c0
loading_config* copy_configs_backward(loading_config* first, loading_config* last, loading_config* out)
{
    if (first != last) {
        do {
            last--;
            out--;
            *out = *last;
        } while (last != first);
    }
    return out;
}

// A record with two vector members: a non-scalar vector at +0 and a scalar vector at +0x10.
// Its compiler-generated destructor tears down the scalar member first, then the non-scalar one,
// which is exactly 0x4da9c0.
struct TwoVec {
    std::vector<E20b> v1;       // +0x00 (allocator, first +4, last +8, end +0xc)
    std::vector<int>  v2;       // +0x10 (allocator, first +0x14, last +0x18, end +0x1c)
    ~TwoVec();
};

// @ 0x4da9c0
TwoVec::~TwoVec() {}

// @ 0x4da900
void LoadingScreen::rebuild()
{
    char* self = (char*)this;
    int count = *(int*)(self + 0xfa0);
    var_mesh* m = new var_mesh;
    if (f568)
        (*(Overlay**)&f568)->n00(1);
    if (m == 0) {
        f16e0 = -1;
        f16e4 = 0xff101010;
    }
    *(int*)(self + 0xfa0) = 0;
    f568 = m;
    *(int*)(self + 0x1700) = 0;
    if (count > 0) {
        int i;
        for (i = 0; i < count; i++)
            ((ScreenRef*)this)->refresh(&pos[i].pos, ids[i], *(int*)((char*)ids + 0x140 + i * 4), (char)flags[i]);
    }
}

// @ 0x4daa10
void LoadingScreen::init()
{
    g_gui.reset();

    Overlay* o;
    {
        Overlay* p = (Overlay*)operator new(0x190);
        if (p != 0) {
            ctor_4931a0(p);
            *(void**)p = (void*)0x592848;
            ((char*)p)[0x188] = 0;
        }
        o = p;
    }
    overlay = o;

    float pos[2];
    pos[0] = -0.001f;
    pos[1] = -0.001f;
    o->set_pos(pos);

    float scale[2];
    scale[0] = 1.0f;
    scale[1] = 0.75f;
    o->set_scale(scale);

    tstring name("load_window");
    o->set_mesh(clone_41c6d0(resmgr_get(&g_resmgr, name)), 0);
    name.~tstring();

    int color[1];
    color[0] = 0xff442323;
    o->set_color(color);

    WidgetB* logo_ = (WidgetB*)operator new(0x178);
    if (logo_ != 0)
        ctor_48ae20(logo_);
    else
        logo_ = 0;
    logo = logo_;

    float lpos[2];
    lpos[0] = 0.05f;
    lpos[1] = 0.6f;
    logo_->set_pos(lpos);

    float lscale[2];
    lscale[0] = 0.4f;
    lscale[1] = 0.05f;
    logo_->set_scale(lscale);

    tstring lname("warrider_logo");
    logo_->set_mesh(clone_41c6d0(resmgr_get(&g_resmgr, lname)), 0);
    lname.~tstring();

    o->add_child(logo_);

    WidgetC* hint_;
    WidgetB* text_;
    {
        tstring hintpos("");
        hint = hint_ = make_hint(hintpos, 0, o, 0.1404162f, 0.1f, 0.3f, 0.04f);
        hintpos.~tstring();
    }
    {
        tstring textpos("");
        text = text_ = (WidgetB*)make_hint(textpos, 0, o, 0.5872804f, 0.3f, 0.3375f, 0.03f);
        textpos.~tstring();
    }

    int flag[1];
    flag[0] = 0x100;
    text->set_flag(flag);

    g_5d8788 = 1;
    Overlay* root = overlay;
    gui_add_root(&g_gui, &root);

    if (g_a18140 != 0) {
        crt_srand(timeGetTime());
        int idx;
        if (g_a18140 <= 1)
            idx = 0;
        else
            idx = crt_rand() % g_a18140;

        tstring prefix("hint_");
        char num[0x40];
        crt_itoa(idx + 1, num, 10);
        tstring suffix(num);
        prefix.append(suffix);
        suffix.~tstring();

        tstring key("ui_hint");
        tstring full(*strtab_lookup(&g_strtab, key));
        key.~tstring();
        tstring sep(" #");
        full.append(sep);
        sep.~tstring();
        tstring num2(num);
        full.append(num2);
        num2.~tstring();
        tstring trail(": ");
        full.append(trail);
        trail.~tstring();
        full.append(*strtab_lookup(&g_strtab, suffix));
        hint->set_text(&suffix, &full);
        full.~tstring();
    }
    f3c = 0;
    f40 = 0;
}
