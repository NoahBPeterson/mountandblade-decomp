// Slice s004c46d0: the tstring-array loaders, the 0x34-byte record list loader, the two object types that
// share the "vtable 0x5947d8 / tstring at +0x1c / status 99" head, the generic list/vector helpers, and the
// std::fill instantiation over 0x2c-byte records.
//
//   0x4c46d0  load a count at +8 and a 0x20-byte tstring-pair array at +0
//   0x4c47d0  same for a count at +0xc and the array at +4
//   0x4c48d0  load a 0x34-byte record list (count at +4, success counter at +8)
//   0x4c49f0  particle-camera-like object ctor (frustum at +0x170, three vector4s, two floats)
//   0x4c4b70  same object's writer (vector4 parts, int at +0x2b0, two floats)
//   0x4c4c00  same object's reader
//   0x4c4c90  deleting dtor of the plain head object
//   0x4c4cc0  ctor of the second head object (frustum at +0x2ec)
//   0x4c4d10  two-pass destructor of the 0x40-byte record array (+4 array, +8 count, +0xc/+0x10 refcount)
//   0x4c4d80  std::list<int>::clear
//   0x4c4dc0  std::list<int>::_Buynode (+ its EH funclet 0x4c4e2f)
//   0x4c4e50  std::list<int>::_Insert-style node allocator (+ its EH funclet 0x4c4ebd)
//   0x4c4ee0  destructor of the 0x845e7c8-byte world object
//   0x4c4f40  ??_E deleting dtor of 0x60-byte elements (vector + object variants)
//   0x4c5010  std::fill over 0x2c-byte records through 0x4c1b20 (funclet 0x4c5074)
// Layouts and names are provisional; field offsets come from this slice's disassembly only.
#include <stdio.h>
#include <string.h>
#include <new>
#include <windows.h>
#include "rgl.h"

using rgl::vector4;

// ---- the engine string (same layout as the one used by s004c3430 / s004c3d90) -----------------------
struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                 // 0x42bbd0  empty
    tstring(const char* s);                    // 0x42bbe0
    ~tstring();                                // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
void underscores_to_spaces(tstring* s);        // 0x42bf80

// ---- the stream used by the loaders (virtual slots at +0x4c = r_int, +0x54 = r_tstr) ----------------
// Same shape as the local stream class of s004c3d90: slot 21 is the int reader (vtable +0x4c) and slot 22
// the tstring reader (vtable +0x54).
struct stream_base {
    virtual void s01();                                 // 1
    virtual void s02();                                 // 2
    virtual void s03();                                 // 3
    virtual void s04();                                 // 4
    virtual void s05();                                 // 5
    virtual void s06();                                 // 6
    virtual void s07();                                 // 7
    virtual void s08();                                 // 8
    virtual void s09();                                 // 9
    virtual void s10();                                 // 10
    virtual void s11();                                 // 11
    virtual void s12();                                 // 12
    virtual void s13();                                 // 13
    virtual void s14();                                 // 14
    virtual void s15();                                 // 15
    virtual void s16();                                 // 16
    virtual void s17();                                 // 17
    virtual void s18();                                 // 18
    virtual void s19();                                 // 19
    virtual void r_int(int* out);                       // 20  0x4c
    virtual void s21();                                 // 21  0x50
    virtual void r_tstr(tstring* out);                  // 22  0x54
};

// ---- the writer used by 0x4c4b70 / 0x4c4c00 (slot 4 = w_stream, slot 15 = w_float) ------------------
struct w_stream {
    virtual void v01();                                 // 0   0x04
    virtual void v02();                                 // 1   0x08
    virtual void v03();                                 // 2
    virtual w_stream* w_float(float* v);                // 3   0x0c, returns the next writer
    virtual void v04();                                 // 4   0x10
    virtual void v05();                                 // 5
    virtual void v06();                                 // 6
    virtual void v07();                                 // 7
    virtual void v08();                                 // 8
    virtual void v09();                                 // 9
    virtual void v10();                                 // 10
    virtual void v11();                                 // 11
    virtual void v12();                                 // 12
    virtual void v13();                                 // 13
    virtual w_stream* r_float(float* v);                // 14  0x38, returns the next reader
};

// ---- engine alloc/free helpers -----------------------------------------------------------------------
void* __cdecl engine_alloc(size_t size);                        // 0x535482  operator new[] (size at [p-4])
void* __cdecl engine_new(size_t size);                          // 0x53545e  operator new (scalar)
void  __cdecl engine_free(void* p);                             // 0x53570d  _free
void  __cdecl engine_delete(void* p);                           // 0x535712  operator delete (scalar)
struct RecI {
    void* destroy(unsigned flags);                              // 0x431810  ??_ERecI deleting destructor
};
void __fastcall rec34_free_array(void* p, void*, unsigned flags); // 0x4318c0  array free for 0x34 records
void __fastcall list_node_init_4be9c0(void* node, void*, void* a, void* b, void* c);  // 0x4be9c0
__declspec(noreturn) void __stdcall __CxxThrowException(void* p, void* t);   // 0x5368e6

// =====================================================================================================
// 0x4c46d0 / 0x4c47d0: the two 0x20-byte tstring pair arrays
// =====================================================================================================

struct tstring_pair {
    tstring a;       // +0
    tstring b;       // +0x10
};

// @ 0x4c46d0
// (ret 4, this in ecx) Reads the count at +8, destroys and rebuilds the 0x20-byte pair array at +0, then
// reads both strings of every element (b first, then a) and converts the underscores of `a` to spaces.
void __fastcall load_pair_array(void* self, void*, stream_base* s)
{
    s->r_int((int*)((char*)self + 8));
    tstring_pair* v = *(tstring_pair**)self;
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].b.~tstring();
            v[n].a.~tstring();
        }
        engine_free((char*)v - 4);
        *(void**)self = 0;
    }
    int n = *(int*)((char*)self + 8);
    tstring_pair* p = (tstring_pair*)engine_alloc(n * 0x20 + 4);
    if (p != 0) {
        *(int*)((char*)p - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&p[i].a) tstring();
            new (&p[i].b) tstring();
        }
    } else
        p = 0;
    *(tstring_pair**)self = p;
    int cnt = *(int*)((char*)self + 8);
    for (int i = 0; i < cnt; i++) {
        s->r_tstr(&p[i].b);
        s->r_tstr(&p[i].a);
        underscores_to_spaces(&p[i].a);
    }
}

// @ 0x4c47d0
// Same as 0x4c46d0 for a pair array at +4 whose count sits at +0xc (ret 4).
void __fastcall load_pair_array12(void* self, void*, stream_base* s)
{
    s->r_int((int*)((char*)self + 0xc));
    tstring_pair* v = *(tstring_pair**)((char*)self + 4);
    if (v != 0) {
        int n = *(int*)((char*)v - 4);
        while (--n >= 0) {
            v[n].b.~tstring();
            v[n].a.~tstring();
        }
        engine_free((char*)v - 4);
        *(void**)((char*)self + 4) = 0;
    }
    int n = *(int*)((char*)self + 0xc);
    tstring_pair* p = (tstring_pair*)engine_alloc(n * 0x20 + 4);
    if (p != 0) {
        *(int*)((char*)p - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&p[i].a) tstring();
            new (&p[i].b) tstring();
        }
    } else
        p = 0;
    *(tstring_pair**)((char*)self + 4) = p;
    int cnt = *(int*)((char*)self + 0xc);
    for (int i = 0; i < cnt; i++) {
        s->r_tstr(&p[i].b);
        s->r_tstr(&p[i].a);
        underscores_to_spaces(&p[i].a);
    }
}

// =====================================================================================================
// 0x4c48d0: the 0x34-byte record list loader
// =====================================================================================================

struct rec34 {
    tstring name;        // +0
    int     w10;         // +0x10
    int     w14;         // +0x14
    int     w18;         // +0x18
    tstring path;        // +0x1c
    char    flag;        // +0x2c
    char    pad2d[3];
    int     w30;         // +0x30
};
char __stdcall rec34_load(rec34* r, stream_base* s);       // 0x4be470 (ret 4, this in ecx): 1 on success
void __fastcall rec34_list_finish(void* self);             // 0x4be560

// @ 0x4c48d0
// (ret 4, this in ecx) Reads two strings and one int into locals, the count into +4, rebuilds the 0x34-byte
// records, loads every one of them through 0x4be470 (counting the successes in +8) and finishes the list.
void __fastcall rec34_list_load(void* self, void*, stream_base* s)
{
    tstring a;
    tstring b;
    int dummy;
    s->r_tstr(&a);
    s->r_tstr(&b);
    s->r_int(&dummy);
    s->r_int((int*)((char*)self + 4));
    void* old = *(void**)self;
    if (old != 0) {
        rec34_free_array(old, 0, 3);
        *(void**)self = 0;
    }
    int n = *(int*)((char*)self + 4);
    *(int*)((char*)self + 8) = 0;
    rec34* r = (rec34*)engine_alloc(n * 0x34 + 4);
    if (r != 0) {
        *(int*)((char*)r - 4) = n;
        for (int i = 0; i < n; i++) {
            new (&r[i].name) tstring();
            new (&r[i].path) tstring();
            r[i].w18 = 0;
            r[i].w14 = 0;
            r[i].flag = 0;
        }
    } else
        r = 0;
    *(rec34**)self = r;
    int cnt = *(int*)((char*)self + 4);
    for (int i = 0; i < cnt; i++) {
        if (rec34_load(&r[i], s))
            ++*(int*)((char*)self + 8);
    }
    rec34_list_finish(self);
}

// =====================================================================================================
// 0x4c49f0 .. 0x4c4cc0: the two "head record" objects
// =====================================================================================================

struct frustum_head {
    char perspective;          // +0
    char pad1[3];
    char pad4[0xc8 - 4];
    float m[0x10];             // +0xc8  sixteen dwords (identity_pos)
    float left;                // +0x108
    float right;               // +0x10c
    float top;                 // +0x110
    float bottom;              // +0x114
    float nearz;               // +0x118
    float farz;                // +0x11c
};

void __fastcall frustum_ctor(frustum_head* f);                                        // 0x41ef60
void __fastcall frustum_planes(frustum_head* f);                                      // 0x41e4f0
void __fastcall tstring_ctor(tstring* s);                                             // 0x42bbd0

// The 0x4c4c90 head record: vptr 0x5947d8, w04/w08 and the name at +0x1c.
struct head_rec {
    virtual void vfunc();      // slot 0 (stub: only the vptr store matters)
    int     w04;               // +4
    int     w08;               // +8     99
    char    pad0c[0x1c - 0xc];
    tstring name;              // +0x1c
    head_rec() { w04 = 0; w08 = 0x63; }
};

// The camera-like object of 0x4c49f0 (vtable 0x595bd8).
struct head_cam : head_rec {
    char    pad30[0x64 - 0x2c];
    char    flag64;            // +0x64
    char    pad65[0x170 - 0x65];
    frustum_head fr;           // +0x170 (0x120 bytes; the vector4 at +0x268 overlaps it in the original)
    vector4 p290;              // +0x290
    vector4 p2a0;              // +0x2a0
    int     w2b0;              // +0x2b0
    float   f2b4;              // +0x2b4
    float   f2b8;              // +0x2b8
    head_cam();
    virtual void vfunc();
    void write_stream(w_stream* w);
    void read_stream(w_stream* r);
};


// @ 0x4c49f0
head_cam::head_cam()
{
    *(void**)this = (void*)0x595bd8;
    frustum_ctor(&fr);
    fr.perspective = 1;
    fr.nearz = 0.2f;
    fr.farz = 300.0f;
    fr.left = -0.1f;
    fr.right = 0.1f;
    fr.bottom = -0.07f;
    fr.top = 0.07f;
    frustum_planes(&fr);
    *(vector4*)((char*)this + 0x268) = vector4(0, 0, 10);
    p290 = vector4(0, 0, 0);
    p2a0 = vector4(0, 0, 0);
    w2b0 = 0;
    flag64 = 0;
    f2b4 = 1.0f;
    f2b8 = 5.0f;
}

// @ 0x4c4b70
// (ret 4, this in ecx) Writes the vector4s part by part, each through the writer returned by the previous
// call (p290, p2a0, then the int at +0x2b0, the two floats).
void head_cam::write_stream(w_stream* w)
{
    vector4* p = &p290;
    w->w_float(&p->x)->w_float(&p->y)->w_float(&p->z);
    p = &p2a0;
    w->w_float(&p->x)->w_float(&p->y)->w_float(&p->z);
    w->w_float((float*)&w2b0);
    w->w_float(&f2b4);
    w->w_float(&f2b8);
}

// @ 0x4c4c00
// Reader counterpart of 0x4c4b70 (ret 4, this in ecx): the same chained calls through slot 15 (+0x38).
void head_cam::read_stream(w_stream* w)
{
    vector4* p = &p290;
    w->r_float(&p->x)->r_float(&p->y)->r_float(&p->z);
    p = &p2a0;
    w->r_float(&p->x)->r_float(&p->y)->r_float(&p->z);
    w->r_float((float*)&w2b0);
    w->r_float(&f2b4);
    w->r_float(&f2b8);
}

// @ 0x4c4c90
// Deleting destructor (ret 4): the head vtable, the string at +0x1c, then the object itself for bit 0.
void* __fastcall head_rec_delete(head_rec* self, void*, unsigned flags)
{
    *(void**)self = (void*)0x5947d8;
    self->name.~tstring();
    if (flags & 1)
        engine_delete(self);
    return self;
}

// The second head object of 0x4c4cc0 (vtable 0x595c20, frustum at +0x2ec).
struct head_obj2 : head_rec {
    char    pad30[0x2dc - 0x2c];
    int     w2dc;              // +0x2dc  -1
    int     w2e0;              // +0x2e0
    int     w2e4;              // +0x2e4  0
    char    pad2e8[4];
    frustum_head fr;           // +0x2ec
    head_obj2();
    virtual void vfunc();
};

// @ 0x4c4cc0
head_obj2::head_obj2()
{
    *(void**)this = (void*)0x595c20;
    frustum_ctor(&fr);
    w2dc = -1;
    w2e4 = 0;
}

// =====================================================================================================
// 0x4c4d10: the 0x40-byte record array destructor
// =====================================================================================================

struct rec40 {
    tstring name;        // +0
    int     w10;         // +0x10
    int     w14;         // +0x14
    tstring path;        // +0x18
    char    flag;        // +0x28
    char    pad29[3];
    int     w2c;         // +0x2c
    void*   w30;         // +0x30  owning pointer freed with operator delete
    char    pad34[0xc];
};

struct Tail2 {
    void*        vec;      // +0
    void**       arr;      // +4  array of owned pointers
    unsigned int count;    // +8
    int          refbase;  // +0xc
    int          refcap;   // +0x10
    void release() { if (refcap != 0) { refcap = refcap - 1; if (refcap == 0) refbase = 0; } }
    ~Tail2();
};

// @ 0x4c4d10
// Tail2::~Tail2 (0x54a?? vtable owner): first drains the +0x10 reference window through the inlined
// release() (clearing +0xc when it reaches zero), then frees the owned pointers back to front through
// operator delete and drops the array.
Tail2::~Tail2()
{
    while (refcap != 0)
        release();
    unsigned int i = count;
    while (i > 0) {
        void* p = arr[i - 1];
        --i;
        if (p != 0)
            engine_delete(p);
    }
    if (arr != 0)
        engine_delete(arr);
    count = 0;
    arr = 0;
}

// =====================================================================================================
// 0x4c4d80: std::list<int>::clear
// =====================================================================================================

struct list_node_int {
    list_node_int* next;
    list_node_int* prev;
    int value;
};

struct list_int {
    list_node_int* head;   // +0
    list_node_int* w04;    // +4  sentinel (self-linked when empty)
    int            count;  // +8
    void clear();
};

// @ 0x4c4d80
void list_int::clear()
{
    list_node_int* h = w04;
    list_node_int* first = h->next;
    h->next = h;
    w04->prev = w04;
    count = 0;
    while (first != w04) {
        list_node_int* nx = first->next;
        engine_delete(first);
        first = nx;
    }
}

// =====================================================================================================
// 0x4c4dc0 / 0x4c4e50: the list node allocators and their funclets
// =====================================================================================================

// @ 0x4c4dc0
// std::list<int>::_Buynode: allocates a 0x28-byte node and links it to itself on both sides.
void* __fastcall list_int_buynode(list_int* self)
{
    list_node_int* node = (list_node_int*)engine_new(0x28);
    if (node != 0)
        node->next = node;
    if ((list_node_int*)((char*)node + 4) != 0)
        node->prev = node;
    return node;
}

// @ 0x4c4e2f
// EH funclet of 0x4c4dc0: frees the node under construction and rethrows.
void list_int_buynode_funclet(list_node_int* node)
{
    engine_delete(node);
    __CxxThrowException(0, 0);
}

// @ 0x4c4e50
// std::list<int> node constructor allocator (ret 0xc): allocates a 0x28-byte node and initialises it through
// 0x4be9c0 with the three arguments of the caller.
void* __fastcall list_int_make_node(list_int* self, void*, void* a, void* b, void* c)
{
    void* node = engine_new(0x28);
    if (node != 0)
        list_node_init_4be9c0(node, 0, a, b, c);
    return node;
}

// @ 0x4c4ebd
// EH funclet of 0x4c4e50: frees the node under construction and rethrows.
void list_int_make_node_funclet(void* node)
{
    engine_delete(node);
    __CxxThrowException(0, 0);
}

// =====================================================================================================
// 0x4c4ee0: destructor of the big world object
// =====================================================================================================

// 0x28-byte record carrying a string first.
struct tstr28 {
    tstring s;                 // +0
    char    pad[0x18];
};

struct world_obj {
    char    head[0x18];        // +0
    tstr28  strs[0xf];         // +0x18  0x28 apart, up to +0x270
    char    pad[0x845e784 - 0x270];
    tstring name;              // +0x845e784
    char    pad2[0x845e7bc - 0x845e794];
    void*   blk;               // +0x845e7bc
    void*   blk2;              // +0x845e7c0
    void*   blk3;              // +0x845e7c4
    ~world_obj();
};

// @ 0x4c4ee0
// Frees the owned block at +0x845e7bc, clears the three words there; the member destructors then destroy the
// string at +0x845e784 and the fifteen 0x28-byte string records from +0x248 back down to +0x18.
world_obj::~world_obj()
{
    if (blk != 0)
        engine_delete(blk);
    blk = 0;
    blk2 = 0;
    blk3 = 0;
}

// =====================================================================================================
// 0x4c4f40: ??_E deleting destructor of the 0x60-byte element
// =====================================================================================================

struct elem60 {
    tstring s0;          // +0
    tstring s10;         // +0x10
    char    pad20[8];
    tstring s28;         // +0x28
    void*   p38;         // +0x38  freed with _free
    char    pad3c[8];
    tstring s44;         // +0x44
    char    pad54[4];
    void*   p58;         // +0x58  freed through its deleting destructor (flag 3)
    char    pad5c[4];
};

// @ 0x4c4f40
// Deleting destructor (ret 4). Bit 1 destroys a vector of elements back to front from the count stored before
// `this` (plus the array itself for bit 0); without bit 1 it destroys the single element.
void* __fastcall elem60_vector_delete(elem60* self, void*, unsigned flags)
{
    if (flags & 2) {
        void* base = (char*)self - 4;
        int n = *(int*)base;
        elem60* e = (elem60*)((char*)self + n * 0x60);
        while (--n >= 0) {
            e = (elem60*)((char*)e - 0x60);
            if (e->p58 != 0)
                ((RecI*)e->p58)->destroy(3);
            if (e->p38 != 0)
                engine_free(e->p38);
            e->s44.~tstring();
            e->s28.~tstring();
            e->s10.~tstring();
            e->s0.~tstring();
        }
        if (flags & 1)
            engine_free(base);
        return base;
    }
    if (self->p58 != 0)
        ((RecI*)self->p58)->destroy(3);
    if (self->p38 != 0)
        engine_free(self->p38);
    self->s44.~tstring();
    self->s28.~tstring();
    self->s10.~tstring();
    self->s0.~tstring();
    if (flags & 1)
        engine_delete(self);
    return self;
}

// =====================================================================================================
// 0x4c5010: std::fill over 0x2c-byte records
// =====================================================================================================

struct block_copier {
    void copy(void* dst, const void* src);   // 0x4c1b20 (this in ecx): 0x2c-byte record copy
};

struct records_filler {
    void fill(void* first, size_t n, const void* src, block_copier* cop);
};

// @ 0x4c5010
// Fills `n` 0x2c-byte records with the value `src` through 0x4c1b20 (the record copy); `cop` is the record
// that owns the copies. The loop is wrapped in the EH frame that guards the copy.
void __cdecl fill_records(void* first, size_t n, const void* src, block_copier* cop)
{
    try {
        while (n > 0) {
            cop->copy(first, src);
            --n;
            first = (char*)first + 0x2c;
        }
    } catch (...) {
        throw;
    }
}

// @ 0x4c5074
// EH funclet of 0x4c5010.
void fill_records_funclet()
{
    __CxxThrowException(0, 0);
}
