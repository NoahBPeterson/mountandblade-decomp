// Mount & Blade 1.011 - batch ds1, slice s004c2b20.
// Record classes of the module/script tables: constructors, destructors and their loaders.
// Class and record names are provisional (only vtable addresses and field offsets are known).
#include <windows.h>
#pragma warning(disable:4234)     // __thiscall on a free function (thunk reconstruction)

// tstring (same 0x10-byte layout as rgl::string in ../../include/rgl.h): {pos, buf, cap, len}.
// The real methods are declared here by address (ctor 0x42bbd0, dtor 0x42bc40, assign 0x42bc50,
// C-string ctor 0x42bbe0); they are declared-only, which is what the call sites need.
struct tstr {
    int   pos;   // +0
    char* p;     // +4  payload pointer
    int   cap;   // +8
    int   len;   // +0xc
    void ctor();                                // 0x42bbd0
    void dtor();                                // 0x42bc40
};
void __cdecl sub_42bc50(tstr* dst, const tstr* src);                 // 0x42bc50 operator=
void __cdecl sub_42bbe0(tstr* p, const char* s);                     // 0x42bbe0 ctor from C string
void __cdecl sub_42c110(tstr* p, const char* s);                     // 0x42c110
void __cdecl sub_42c1e0(tstr* dst, const tstr* src);                 // 0x42c1e0
#define TCTOR(x) (x).ctor()
#define TDTOR(x) (x).dtor()
#define TCPTR(x) ((tstr*)(x))->ctor()
#define TDPTR(x) ((tstr*)(x))->dtor()

// Half-initialised tstring used by the loader: type is set before the string methods are used.
struct tstr_ll {
    void*      typ;     // +0
    tstr       s;       // +4
    tstr_ll() : typ((void*)0x54a740) { TCTOR(s); }
    ~tstr_ll() { typ = (void*)0x54ad68; TDTOR(s); }
    const char* text() const { return s.p ? s.p : (const char*)&s.p; }
};

struct stream;
void __cdecl dtor_42bf80(void* s);                                   // 0x42bf80 (tstring clear/replace)
void* __cdecl operator new(size_t n);                                // 0x535482
void __cdecl operator delete(void* p);                               // 0x535712
void __cdecl alloc_ctor(void* dst, int size, int n, void* ctor);     // 0x401800
char* __cdecl sub_536f8f(char* buf, const char* fmt, ...);           // 0x536f8f (sprintf)
void __cdecl free_535712(void* p);                                   // 0x535712

void __cdecl sub_4607e0(void* dst, void* s);                         // 0x4607e0
void __cdecl sub_4bab70(void* self);                                 // 0x4bab70
void __cdecl sub_415290(void* dst, void* src);                       // 0x415290
void __cdecl sub_4be7c0(void* self);                                 // 0x4be7c0
void __cdecl sub_415d60(void* self, const char* s);                  // 0x415d60
void __cdecl sub_42cc80(const char* lit, const char* s);             // 0x42cc80

extern int    g_5e2958;               // 0x5e2958  clock snapshot source
extern int    g_5e295c;               // 0x5e295c
extern unsigned char g_5d8789;        // 0x5d8789
extern char   g_8ee3c48_obj[];        // 0x8ee3c48  global object
extern char   g_8ee3c28_obj[];        // 0x8ee3c28  global object

// =============================================================================================
// Loadable record: header, three tstrings, a 0x38 blob, a tstring at +0x44 and a 0x68-byte
// record vector at +0x54 (element ctor 0x4be420, element reader 0x4be310).
// =============================================================================================
struct VoiceRec {
    char body[0x68];
};
struct Vec68 {
    int       count;     // +0
    VoiceRec* data;      // +4
};
struct Rec2b20 {
    void*    vt;         // +0
    int      v4;         // +4
    char     pad8[8];    // +8
    tstr     name;       // +0x10
    tstr     t20;        // +0x20
    tstr     t28;        // +0x28
    char     pad34[4];   // +0x34
    char     big[0x1c];  // +0x38
    tstr     t44;        // +0x44
    Vec68    vec;        // +0x54
    int load(void* s);   // 0x4c2b20
};

// =============================================================================================
// Small record classes: a vtable at +0, an int at +4 and a 0x63 constant at +8, then tstrings.
// =============================================================================================
struct Base0 {
    void* vt;            // +0
    int   v4;            // +4
    int   v8;            // +8  (0x63)
    char  padc[0x10];    // +0xc
    tstr  t1c;           // +0x1c
};

struct C4c2c50 : Base0 {     // vtable 0x595740
    char  pad2c[0x20];       // +0x2c
    int   v4c;               // +0x4c
    int   v50;               // +0x50
    tstr  t54;               // +0x54
    int   v64;               // +0x64
    tstr  t68;               // +0x68
    tstr  t78;               // +0x78
    tstr  t88;               // +0x88
    C4c2c50();               // 0x4c2c50
    __declspec(noinline) void dtor();   // 0x4c2cd0 (called, not inlined, by the deleting dtor 0x4c2cb0)
};

struct M44 { float f[16]; };
M44* identity_pos(M44* out);                         // 0x40cd00 (fills `out`, returns it)

// 0x140-byte record: four 0x40-byte matrices filled from 0x40cd00, counters at +0x100/+0x104,
// a mode int at +0x110 and a clock snapshot / rate at +0x118..+0x128.
struct C4c2d10 {
    M44    blk0;             // +0
    M44    blk1;             // +0x40
    M44    blk2;             // +0x80
    M44    blk3;             // +0xc0
    int    v100;             // +0x100
    int    v104;             // +0x104
    char   pad108[8];        // +0x108
    int    v110;             // +0x110
    int    v114;             // +0x114
    int    v118;             // +0x118
    int    v11c;             // +0x11c
    int    v120;             // +0x120
    float  f124;             // +0x124 (6.0f)
    int    v128;             // +0x128
    C4c2d10();               // 0x4c2d10
};

struct C4c2fb0 : Base0 {     // vtable 0x595798
    char  pad2c[0x1c];       // +0x2c
    tstr  t48;               // +0x48
    tstr  t58;               // +0x58
    tstr  t68;               // +0x68
    C4c2fb0();               // 0x4c2fb0
    __declspec(noinline) void dtor();   // 0x4c3020 (called, not inlined, by the deleting dtor 0x4c3000)
};

struct C4c3050 : Base0 {     // vtable 0x5957bc
    char  pad2c[0x24];       // +0x2c
    int   v50;               // +0x50 (set to -1)
    int   v54;               // +0x54
    int   v58;               // +0x58
    tstr  t5c;               // +0x5c
    tstr  t6c[6];            // +0x6c (6 entries, 0x10 apart)
    C4c3050();               // 0x4c3050
    void dtor();             // 0x4c30e0
};

struct Slot20 {              // 0x20 bytes
    int a, b, c, d, e, f, g, h;
};
struct C4c3160 : Base0 {     // vtable 0x595870
    char   body[0x8e8 - 0x2c];
    Slot20 slots[0x80];      // +0x8e8
    C4c3160();               // 0x4c3160
};

struct Pair30 {              // 0x30: tstring then a 0x10-byte pad (second ctor is +0x10)
    tstr a;
    char pad10[0x10];
};
struct C4c31c0 : Base0 {     // vtable 0x595924
    tstr t2c;                // +0x2c (0x30-byte pairs follow; the rest is reached by pointer)
    C4c31c0();               // 0x4c31c0
    void dtor();             // 0x4c3240
};

struct C4c3290 : Base0 {     // vtable 0x595a20
    char  pad2c[0x24];       // +0x2c
    tstr  t50;               // +0x50
    tstr  t60;               // +0x60
    tstr  t70;               // +0x70
    C4c3290();               // 0x4c3290
    __declspec(noinline) void dtor();   // 0x4c3300 (called, not inlined, by the deleting dtor 0x4c32e0)
};

// 0x18-byte entry of the block read by 0x4c2dd0 (tstring, two ints).
struct Entry18 {
    tstr s0;                 // +0
    int  v10;                // +0x10
    int  v14;                // +0x14
};
// Block with tstrings at +0/+0x10 and an entry vector at +0x18.
struct Blk2dd0 {
    tstr     s0;             // +0
    tstr     s10;            // +0x10
    int      count;          // +0x18
    Entry18* entries;        // +0x1c
};

// Stream vtable slots (record reader / int reader).
struct stream { void* const vt; };
typedef void (__fastcall *vt_rec)(void* self, void* dst);
typedef void (__fastcall *vt_int)(void* self, int* dst);
inline void st_rec(void* s, void* dst) { ((vt_rec*)(*(char**)s))[0](s, dst); }
inline void st_str(void* s, void* dst) { ((vt_rec*)(*(char**)s))[0x54 / 4](s, dst); }
inline void st_int(void* s, int* dst) { ((vt_rec*)(*(char**)s))[0x4c / 4](s, dst); }

// =============================================================================================
// Functions
// =============================================================================================

// @ 0x4c30b0
void set_flag_5d8789()
{
    g_5d8789 = 1;
}

// @ 0x4c2c50
C4c2c50::C4c2c50()
{
    vt = (void*)0x5947d8;
    TCTOR(t1c);
    v4 = 0;
    v8 = 0x63;
    vt = (void*)0x595740;
    TCTOR(t54);
    TCTOR(t68);
    TCTOR(t78);
    TCTOR(t88);
    v4c = 0;
    v50 = 0;
}

// @ 0x4c2cd0
void C4c2c50::dtor()
{
    vt = (void*)0x595740;
    TDTOR(t88);
    TDTOR(t78);
    TDTOR(t68);
    TDTOR(t54);
    vt = (void*)0x5947d8;
    TDTOR(t1c);
}

// @ 0x4c2cb0
void* __fastcall destroy_C4c2c50(C4c2c50* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c2fb0
C4c2fb0::C4c2fb0()
{
    vt = (void*)0x5947d8;
    TCTOR(t1c);
    v4 = 0;
    v8 = 0x63;
    vt = (void*)0x595798;
    TCTOR(t48);
    tstr* p = (tstr*)((char*)this + 0x58);
    TCPTR(p);
    TCPTR(p + 1);
}

// @ 0x4c3020
void C4c2fb0::dtor()
{
    tstr* p = (tstr*)((char*)this + 0x58);
    TDPTR(p + 1);
    TDPTR(p);
    TDTOR(t48);
    vt = (void*)0x5947d8;
    TDTOR(t1c);
}

// @ 0x4c3000
void* __fastcall destroy_C4c2fb0(C4c2fb0* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c3050
C4c3050::C4c3050()
{
    vt = (void*)0x5947d8;
    TCTOR(t1c);
    v4 = 0;
    v8 = 0x63;
    vt = (void*)0x5957bc;
    TCTOR(t5c);
    tstr* p = t6c;
    for (int i = 6; i != 0; i--) {
        TCPTR(p);
        p++;
    }
    v50 = -1;
    v54 = 0;
}

// @ 0x4c30e0
void C4c3050::dtor()
{
    vt = (void*)0x5957bc;
    char* p = (char*)this + 0xcc;
    for (int i = 6; i > 0; i--) {
        p -= 0x10;
        TDPTR(p);
    }
    TDPTR((char*)this + 0x5c);
    vt = (void*)0x5947d8;
    TDPTR((char*)this + 0x1c);
}

// @ 0x4c30c0
void* __fastcall destroy_C4c3050(C4c3050* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c3160
C4c3160::C4c3160()
{
    vt = (void*)0x5947d8;
    TCTOR(t1c);
    v4 = 0;
    v8 = 0x63;
    vt = (void*)0x595870;
    Slot20* s = slots;
    for (int i = 0; i < 0x80; i++, s++) {
        s->a = 0;
        s->b = -1;
        s->c = 0;
        s->f = 0;
        s->d = 0;
        s->e = 0;
        s->g = 0;
    }
}

// @ 0x4c31c0
C4c31c0::C4c31c0()
{
    vt = (void*)0x5947d8;
    TCTOR(t1c);
    v4 = 0;
    v8 = 0x63;
    vt = (void*)0x595924;
    char* p = (char*)&t2c;
    for (int i = 0; i < 9; i++, p += 0x30) {
        TCPTR(p);
        TCPTR(p + 0x10);
    }
    TCPTR((char*)this + 0x224);
}

// @ 0x4c3240
void C4c31c0::dtor()
{
    TDPTR((char*)this + 0x224);
    char* p = (char*)this + 0x2c + 9 * 0x30;
    for (int i = 9; i > 0; i--) {
        p -= 0x30;
        TDPTR(p + 0x10);
        TDPTR(p);
    }
    vt = (void*)0x5947d8;
    TDTOR(t1c);
}

// @ 0x4c3220
void* __fastcall destroy_C4c31c0(C4c31c0* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c3290
C4c3290::C4c3290()
{
    vt = (void*)0x5947d8;
    TCTOR(t1c);
    v4 = 0;
    v8 = 0x63;
    vt = (void*)0x595a20;
    TCTOR(t50);
    tstr* p = (tstr*)((char*)this + 0x60);
    TCPTR(p);
    TCPTR(p + 1);
}

// @ 0x4c3300
void C4c3290::dtor()
{
    char* p = (char*)this + 0x60;
    TDPTR(p + 0x10);
    TDPTR(p);
    TDPTR((char*)this + 0x50);
    vt = (void*)0x5947d8;
    TDPTR((char*)this + 0x1c);
}

// @ 0x4c32e0
void* __fastcall destroy_C4c3290(C4c3290* self, int, int flags)
{
    self->dtor();
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c3330
// Tail-call thunks: `mov ecx, 0x8ee3c48; jmp 0x4bf870` -> a thiscall member call on the editor singleton.
struct EditorObj {
    void unk_4bf870();                          // 0x4bf870
    void unk_4c03b0();                          // 0x4c03b0
};
void global_4bf870()
{
    ((EditorObj*)0x8ee3c48)->unk_4bf870();
}

// @ 0x4c3340
void global_4c03b0()
{
    ((EditorObj*)0x8ee3c48)->unk_4c03b0();
}

// @ 0x4c2d10
C4c2d10::C4c2d10()
{
    M44 tmp;
    v110 = 2;
    v118 = g_5e2958;
    v11c = g_5e295c;
    v120 = 0;
    v128 = 0;
    f124 = 6.0f;
    blk0 = *identity_pos(&tmp);
    blk1 = *identity_pos(&tmp);
    blk3 = *identity_pos(&tmp);
    blk2 = *identity_pos(&tmp);
    v100 = 0;
    v104 = 0;
}

// @ 0x4c3120
// Deleting destructor of the class with vtable 0x595804 (implicit dtor: tstring at +0x84 and the base's
// tstring at +0x1c). Declared as a __fastcall free function: `self` lands in ECX and the dummy int in EDX
// (unused), leaving `flags` at [esp+4] so the epilogue is the original's `ret 4`.
void* __fastcall destroy_C4c3120(void* self, int, int flags)
{
    TDPTR((char*)self + 0x84);
    *((void**)self) = (void*)0x5947d8;
    TDPTR((char*)self + 0x1c);
    if (flags & 1)
        operator delete(self);
    return self;
}

// @ 0x4c2b20
int Rec2b20::load(void* s)
{
    char buf[0x200];
    tstr_ll tmp;
    st_str(s, &name);
    st_rec(s, &t20);            // vt+0x48
    st_str(s, this);
    dtor_42bf80(this);
    st_str(s, &t28);
    sub_4607e0(big, s);
    sub_536f8f(buf, "???%s", name.p ? name.p : (char*)&name.p);
    sub_42bbe0(&tmp.s, buf);
    sub_42bc50(&t44, &tmp.s);
    tmp.~tstr_ll();
    st_int(s, &vec.count);
    int n = vec.count;
    if (n != 0) {
        Vec68* p = (Vec68*)operator new(n * 0x68 + 4);
        if (p != 0) {
            p->count = n;
            VoiceRec* r = (VoiceRec*)((char*)p + 4);
            alloc_ctor(r, 0x68, n, (void*)0x4be420);
            vec.data = (VoiceRec*)r;
        } else {
            vec.data = 0;
        }
    }
    for (int i = 0; i < n; i++)
        ((void (__cdecl*)(char*, void*, char*))0x4be310)((char*)vec.data + i * 0x68, s, (char*)this + 0x10);
    return 0;
}

// @ 0x4c2dd0
int __fastcall read_block_2dd0(Blk2dd0* self, void* s)
{
    st_str(s, self);
    dtor_42bf80(self);
    st_int(s, &self->count);
    int n = self->count;
    if (n != 0) {
        void* p = operator new(n * 0x18 + 4);
        if (p != 0) {
            *(int*)p = n;
            Entry18* e = (Entry18*)((char*)p + 4);
            alloc_ctor(e, 0x18, n, (void*)0x4be720);
            self->entries = e;
        } else {
            self->entries = 0;
        }
    }
    for (int i = 0; i < self->count; i++) {
        Entry18* e = &self->entries[i];
        st_int(s, &e->v10);
        st_str(s, e);
        st_int(s, &e->v14);
        dtor_42bf80(e);
    }
    st_int(s, (int*)&self->s10);
    st_int(s, (int*)((char*)self + 0x14));
    return 0;
}

// @ 0x4c2ea0
void __fastcall fn_4c2ea0(void* self)
{
    tstr_ll a;
    tstr_ll b;
    char buf[0x28];
    sub_42bbe0(&a.s, (const char*)0x595788);
    sub_42bbe0(&b.s, (const char*)0x595788);
    sub_536f8f(buf, (const char*)0x595788);
    ((void (__cdecl*)(char*))0x4bab70)(buf);
    ((void (__cdecl*)(tstr*, char*))0x415290)(&a.s, buf);
    if (((unsigned char*)&b)[0x14] != 0) {
        sub_4be7c0(self);
    } else {
        int v = 0;
        st_int(&a, &v);
        if (v != 0x25) {
            sub_4be7c0(self);
        } else {
            char* d = (char*)self;
            for (int i = 0; i < 0x25; i++)
                for (int j = 0; j < 2; j++, d += 4)
                    st_int(&a, (int*)d);
        }
        ((unsigned char*)&b)[0x14] = 1;
    }
    b.~tstr_ll();
    a.~tstr_ll();
}

// @ 0x4c3350
void fn_4c3350()
{
    tstr_ll a;
    tstr_ll b;
    sub_42c110(&a.s, (const char*)0x8ee3c28);
    sub_42bbe0(&b.s, (const char*)0x595a44);
    sub_42c1e0(&a.s, &b.s);
    TDTOR(b.s);
    const char* nm = a.s.p ? a.s.p : (const char*)&a.s.p;
    ((char (__cdecl*)(tstr*, const char*))0x415d60)(&a.s, nm);
    sub_42cc80((const char*)0x594580, nm);
    ((int (__cdecl*)(void*, void*))0x4c2250)(&a.s, (void*)0xa76628);
    a.~tstr_ll();
}
