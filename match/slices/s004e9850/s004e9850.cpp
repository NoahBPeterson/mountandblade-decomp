// Slice s004e9850 (batch ds1): the record-level serialisers that the game-state writer (0x4e8f10, slice
// s004e8f10) calls at the end: one GameRecord (0x4e9850), the fixed 96-pair array (0x4e99a0), the sparse
// page-table writer (0x4e99d0) with its per-entry writer (0x4e9ab0), the mission/party block (0x4e9b50) with
// its id table (0x4e9c50) and the big 0x5ac8-record walker (0x4e9d10), then the id-table compactor
// (0x4e9fe0) and the id removal (0x4ea030).
// Every offset below is read off the disassembly of this range; names are provisional. All nine are
// thiscall `ret 4`; the raw-pointer ones take the block they write as `this` and the stream as the only
// stack argument.
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

// ---- the writer (vtable slots as used in this range) -----------------------------------------------------
struct OutStream {
    virtual OutStream& v00();                       // +0x0
    virtual OutStream& v04();                       // +0x4
    virtual OutStream& v08();                       // +0x8
    virtual OutStream& w_int(int* p);               // +0xc
    virtual OutStream& w_int4(int* p);              // +0x10
    virtual OutStream& w_int5(int* p);              // +0x14
    virtual OutStream& w_int1(int* p);              // +0x18
    virtual OutStream& w_count(int* p);             // +0x1c
    virtual OutStream& w_raw(const void* p);        // +0x20
    virtual OutStream& v24();                       // +0x24
    virtual OutStream& w_cstr(const char* s);       // +0x28
    virtual OutStream& w_struct(const void* p);     // +0x2c
};

// ---- vectors --------------------------------------------------------------------------------------------
struct List8 {                                      // vector of 8-byte elements
    char* begin;                                    // +0x0
    char* end;                                      // +0x4
    char* endcap;                                   // +0x8
    void write8(OutStream* out);                    // 0x4e94b0 (thiscall, ret 4)
};
struct List4 {                                      // vector of 4-byte ints
    int* begin;                                     // +0x0
    int* end;                                       // +0x4
    int* endcap;                                    // +0x8
    int size() const { return begin ? (int)(end - begin) : 0; }
};
struct OutStream;
struct cPairs96 {                                   // the 0x60 8-byte pairs of a game record (+0xb4)
    char raw[8 * 0x60];
    __declspec(noinline) void write(OutStream* out); // 0x4e99a0 (ret 4)
};

// ---- game records (stride 0x508) ------------------------------------------------------------------------
#pragma pack(push, 4)
struct GameRecord {
    tstring name;                                   // +0x00
    tstring title;                                  // +0x10
    int     f20, f24, f28, f2c;                     // +0x20
    int     f30;                                    // +0x30 (bit 0x10: temporary)
    char    pad34[0xa0 - 0x34];
    int     fa0;                                    // +0xa0
    int     fa4, fa8, fac;                          // +0xa4
    int     fb0;                                    // +0xb0
    char    padb4[0x4f4 - 0xb4];
    List8   f4f4;                                   // +0x4f4 (vector of 8-byte elements)
    char    pad500[8];
    void write(OutStream* out);                     // 0x4e9850 (thiscall, ret 4)
};
#pragma pack(pop)

// ---- the sparse page table at Game+0xcc ----------------------------------------------------------------
#pragma pack(push, 4)
struct Entry {                                      // 0x98 stride 0x98, 0x10 entries per page
    char    pad00[4];
    int     f04;                                    // +0x04 (non-zero: the slot is used)
    int     f08;                                    // +0x08
    int     f0c;                                    // +0x0c
    int     f10;                                    // +0x10 (raw)
    char    f14[4];                                 // +0x14 (a struct)
    int     f18;                                    // +0x18 (int1)
    int     fa1;                                    // +0x1c
    int     fa2;                                    // +0x20
    int     fa3;                                    // +0x24
    int     fa4;                                    // +0x28
    int     fa5;                                    // +0x2c
    int     fa6;                                    // +0x30
    int     fa7;                                    // +0x34
    char    pad38[0x54 - 0x38];
    int     f54;                                    // +0x54 (raw)
    int     f58;                                    // +0x58 (raw)
    char    pad5c[0x68 - 0x5c];
    int     f68;                                    // +0x68 (int1)
    char    pad6c[4];
    int     f70;                                    // +0x70
    char    pad74[0x98 - 0x74];
    void write(OutStream* out);                     // 0x4e9ab0 (ret 4)
};
struct Table {
    int      count;                                 // +0x0
    char     pad04[0x18 - 0x04];
    Entry**  pages;                                 // +0x18
    char     pad1c[0x24 - 0x1c];
    void     write_table(OutStream* out);           // 0x4e99d0 (ret 4)
};
#pragma pack(pop)

// ---- the id table of 0x4e9b50 (stride 0x5ac8) -----------------------------------------------------------
#pragma pack(push, 4)
struct Rec5ac8 {
    int      f00;                                   // +0x00
    int      type;                                  // +0x04 (1 = live, 2 = dropped, 0 = free)
    int      id;                                    // +0x08
    char     pad0c[0x1c - 0x0c];
    int      f1c;                                   // +0x1c (int4)
    char     f20[4];                                // +0x20 (struct)
    char     pad24[0x30 - 0x24];
    int      f30, f34, f38;                         // +0x30 (three ints)
    char     pad3c[0x50 - 0x3c];
    int      f50;                                   // +0x50
    char     f54[4];                                // +0x54 (struct)
    char     pad58[0x6c - 0x58];
    int      f6c;                                   // +0x6c (raw)
    char     pad70[0x74 - 0x70];
    int      f74;                                   // +0x74 (raw)
    int      f78;                                   // +0x78
    int      f7c, f80, f84;                         // +0x7c (three ints)
    char     pad88[0x8c - 0x88];
    int      f8c;                                   // +0x8c (raw)
    int      f90;                                   // +0x90 (raw)
    int      f94;                                   // +0x94 (raw)
    char     pad98[0xa0 - 0x98];
    int      fa0;                                   // +0xa0
    char     pada4[0x8a8 - 0xa4];
    int      f8a8;                                  // +0x8a8 (count of the 0x20-byte entries at +0xa8)
    char     pad8ac[0x8bc - 0x8ac];
    List4    f8bc;                                  // +0x8bc
    int      f8c8;                                  // +0x8c8 (raw)
    int      f8cc;                                  // +0x8cc (int4)
    char     pad8d0[0x8e4 - 0x8d0];
    int      f8e4;                                  // +0x8e4 (raw)
    int      f8e8;                                  // +0x8e8 (raw)
    int      f8ec, f8f0;                            // +0x8ec
    int      f8f4;                                  // +0x8f4
    char     pad8f8[0x918 - 0x8f8];
    int      f918;                                  // +0x918 (raw)
    int      f91c;                                  // +0x91c (raw)
    char     pad920[0x924 - 0x920];
    int      f924, f928;                            // +0x924
    char     pad92c[0x958 - 0x92c];
    int      f958;                                  // +0x958
    char     pad95c[0x59d8 - 0x95c];
    List8    f59d8;                                 // +0x59d8 (vector of 8-byte elements)
    char     pad59e4[0x59f8 - 0x59e4];
    List4    f59f8;                                 // +0x59f8 (vector of ints)
    char     pad5a04[0x5a08 - 0x5a04];
    char     f5a08[0xb0];                           // +0x5a08 (the struct-of-arrays block)
    int      f5ab8;                                 // +0x5ab8
    char     pad5abc[0x5ac8 - 0x5abc];
    void write(OutStream* out);                     // 0x4e9d10 (ret 4)
};

typedef int Quad[4];                                // the 4-int runs written at +0x8ec and +0x924
#pragma pack(pop)

#pragma pack(push, 4)
struct RecList {                                    // the record list of the GameRecord object at +0xf8
    char     pad00[0x78];
    int      f78;                                   // +0x78 (the id whose record is walked)
    char     pad7c[0x138b8 - 0x7c];
    int      count;                                 // +0x138b8
    char     pad13bc[0x138d0 - 0x138bc];
    int      f138d0;                                // +0x138d0 (the current page of records)
    int      f138d4;                                // +0x138d4 (the next page)
    char     pad13d4[0x138dc - 0x13d4];
    int      f138dc;                                // +0x138dc
    char     pad13e0[0x14050 - 0x13e0];
    int      f14050[3];
    int      f1405c[0x20];
    void write_table(OutStream* out);               // 0x4e9c50 (ret 4)
    void flush_ids();                               // 0x4e9fe0 (ret 0)
    int  id_remove(int id);                         // 0x4ea030 (ret 4)
};
#pragma pack(pop)

// ---- the object 0x4e9b50 actually receives (GameRecord + 0xf8) -------------------------------------------
// Its tail overlaps the Game object's own data: the record array at Game+0x13a00 sits at this-0x1c0.
#pragma pack(push, 4)
struct OutRecList {
    char      pad00[0x78];
    int       f78;                                  // +0x78
    char      pad7c[0x138b8 - 0x7c];
    int       count;                                // +0x138b8
    char      pad13bc[0x138d0 - 0x138bc];
    int       f138d0;                               // +0x138d0 (the current page of records)
    char      pad13d4[0x138dc - 0x13d4];
    int       f138dc;                               // +0x138dc
    char      pad13e0[0x14050 - 0x13e0];
    int       f14050[3];
    int       f1405c[0x20];                         // +0x1405c
    void write_missions(OutStream* out);            // 0x4e9b50 (ret 4)
};
#pragma pack(pop)

// ---- the outer game object (only the record array is touched here) --------------------------------------
struct OuterGame {
    char        pad00[0x13a00];
    GameRecord* records;                            // +0x13a00
};

// ---- callees (slice s004e8f10 / others) -----------------------------------------------------------------
void       write_soa(OutStream* out, char* base);   // 0x4e9620 (ret 0, two stack args)
int        id_lookup(void* map, int id);            // 0x48ad00 (ret 4)
struct IntVec { int int_remove(int& id); };         // 0x440b30 (thiscall, ret 4)


static const int kRecSize = sizeof(Rec5ac8);
// the int-vector writer the original inlines at +0x8bc and +0x59f8: the count, then each int
static __forceinline void write_vec4(List4* v, OutStream* out)
{
    int n;
    if (!v->begin)
        n = 0;
    else
        n = (int)(v->end - v->begin);
    out->w_count(&n);
    unsigned i = 0;
    for (;;) {
        int* b = v->begin;
        if (!b)
            break;
        if (i >= (unsigned)(v->end - b))
            break;
        out->w_raw(&b[i]);
        i++;
    }
}

// the record behind the id table. Page P's +0 word is where page P+1 lives, so walking hoists the page
// pointer and leans on the index's scale factor.
static Rec5ac8* rec_at(RecList* l, int i)
{
    char*  p = ((char**)l->f138d0)[(unsigned)i >> 4];
    int    slot = i & 0xf;
    Rec5ac8* r = (Rec5ac8*)(p + slot * (int)sizeof(Rec5ac8));
    return r;
}

// @ 0x4e9850    void GameRecord::write(OutStream* out)   (thiscall, ret 4)
void GameRecord::write(OutStream* out)
{
    char* p = (char*)this;
    f4f4.write8(out);
    for (int i = 0; i < 4; i++)
        out->w_raw(p + 0x58 + i * 4);
    for (int i = 0; i < 7; i++)
        out->w_int((int*)(p + 0x80 + i * 4));
    for (int i = 0; i < 6; i++)
        out->w_count((int*)(p + 0x68 + i * 4));
    out->w_int5((int*)(p + 0x30) + 0).w_raw(p + 0x3b8);
    out->w_raw(p + 0x3b4).w_raw(p + 0x9c);
    out->w_int((int*)(p + 0xa0));
    out->w_raw(p + 0x4c);
    out->w_raw(p + 0xa4).w_raw(p + 0xa8).w_raw(p + 0xac);
    out->w_raw(p + 0xb0);
    ((cPairs96*)(p + 0xb4))->write(out);
    char* q = p + 0x4a4;
    for (int i = 0; i < 10; i++, q += 8) {
        out->w_raw(q);
        out->w_raw(q + 4);
    }
    char* r5 = p + 0x3c0;
    for (int i = 0; i < 4; i++, r5 += 8)
        out->w_int5((int*)r5);
    write_soa(out, p + 0x400);
}

// @ 0x4e99a0    void cPairs96::write(OutStream* out)   (thiscall, ret 4)   the 0x60 pairs at this
void cPairs96::write(OutStream* out)
{
    char* r = raw;
    for (int i = 0x60; i; i--, r += 8) {
        out->w_raw(r);
        out->w_raw(r + 4);
    }
}

// @ 0x4e99d0    void Table::write_table(OutStream* out)   (thiscall, ret 4)   the sparse page table
void Table::write_table(OutStream* out)
{
    int last = -1;
    int n = count;
    for (int i = 0; i < n; i++) {
        Entry* e = pages[(unsigned)i >> 4] + (i & 0xf);
        if (e->f04)
            last = i;
    }
    int w = last + 1;
    out->w_raw(&w);
    out->w_raw((int*)((char*)this + 0x24));
    int k = 0;
    while (k <= last) {
        Entry* e = pages[(unsigned)k >> 4] + (k & 0xf);
        if (e->f04) {
            int one = 1;
            out->w_raw(&one);
            out->w_raw(&e->f0c);
            e->write(out);
        } else {
            int zero = 0;
            out->w_raw(&zero);
        }
        k++;
    }
}

// @ 0x4e9ab0    void Entry::write(OutStream* out)   (thiscall, ret 4)
//   three chained pairs at +0x24, +0x2c and +0x34 (each: slot 0xc, then slot 0xc on its result at +4).
void Entry::write(OutStream* out)
{
    out->w_struct(&f14);
    out->w_raw(&f10);
    int* p;
    int* q;
    p = &fa3;
    OutStream* s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    p = &fa5;
    s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    p = &fa7;
    s = &out->w_int(p);
    q = p + 1;
    s->w_int(q);
    out->w_raw(&f54);
    out->w_raw(&f58);
    out->w_int1(&f68);
    out->w_int(&f70);
}

// @ 0x4e9b50    void OutRecList::write_missions(OutStream* out)   (thiscall, ret 4)
//   `this` is the outer object: the record list (RecList) is at this+0xf8, the id looked up is this+0x78,
//   the party-record array is the pointer stored at this+0x13a00 and the 0x20-int block table is this+0x1405c.
void OutRecList::write_missions(OutStream* out)
{
    RecList* l = (RecList*)((char*)this + 0xf8);
    l->flush_ids();
    l->write_table(out);
    int idx = id_lookup(l, ((RecList*)this)->f78);
    char** base = *(char***)((char*)l + 0x138d0);
    Rec5ac8* r = (Rec5ac8*)(base[idx >> 4] + (idx & 0xf) * (int)sizeof(Rec5ac8));
    GameRecord* records = *(GameRecord**)((char*)this + 0x13a00);
    int* pk = (int*)((char*)this + 0x1405c);
    char* e = (char*)r + 0xc0;
    for (int i = 0; i < r->f8a8; i++, e += 0x20) {
        int v = *(int*)(e - 0x14);
        // the party record (stride 0x508); bit 0x10 marks a temporary one
        if (records[v].f30 & 0x10)
            continue;
        out->w_int((int*)e);
        out->w_raw(e - 4);        for (int k = 0; k < 0x20; k++)
            out->w_raw(pk + k);
        pk += 0x20;
    }
}

// @ 0x4e9c50    void RecList::write_table(OutStream* out)   (thiscall, ret 4)
void RecList::write_table(OutStream* out)
{
    char* self = (char*)this;
    out->w_raw((int*)(self + 0x138b8));
    out->w_raw((int*)(self + 0x138dc));
    int i = 0;
    int* pi = &i;
    while (i < count) {
        Rec5ac8* r = rec_at(this, i);
        if (r->type == 1) {
            int one = 1;
            out->w_raw(&one);
            out->w_raw(pi);
            out->w_raw(&r->id);
            r->write(out);
        } else {
            int zero = 0;
            out->w_raw(&zero);
        }
        i++;
    }
}

// @ 0x4e9d10    void Rec5ac8::write(OutStream* out)   (thiscall, ret 4)   the big record walk
void Rec5ac8::write(OutStream* out)
{
    char* p = (char*)this;
    out->w_struct(&f20);
    out->w_struct((char*)this + 0x0c);   // +0xc of the record body (the +0x20 struct is above it)
    out->w_int5((int*)(p + 0xa0));
    out->w_raw(p + 0x90);
    out->w_raw(p + 0x8c);
    out->w_raw(p + 0x70);
    out->w_raw(p + 0x74);
    out->w_raw(p + 0x8e4);
    out->w_raw(p + 0x918);
    out->w_raw(p + 0x8e8);
    out->w_raw(p + 0x91c);
    Quad& a0 = *(Quad*)(p + 0x8ec);
    int* pa = &a0[0];
    OutStream* sA = &out->w_int(pa);
    sA->w_int(&pa[1]);
    Quad& b0 = *(Quad*)(p + 0x924);
    int* pb = &b0[0];
    OutStream* sB = &out->w_int(pb);
    sB->w_int(&pb[1]);
    OutStream* s4b = &out->w_int((int*)(p + 0x30));
    s4b = &s4b->w_int((int*)(p + 0x34));
    s4b->w_int((int*)(p + 0x38));
    int* pq = (int*)((char*)this + 0x8a8);
    out->w_raw(pq);
    char* b = (char*)this + 0xa8;
    for (int j = 0; j < *pq; j++, b += 0x20) {
        out->w_raw(b + 4);
        out->w_raw(b);
        out->w_raw(b + 0x10);
        out->w_raw(b + 8);
    }
    // the tail (offsets per 0x4e94b0 / 0x4e9620): +0x50, +0x1c (int4), +0x54 (struct), the three ints at
    // +0x7c, +0x8f4, +0x958, the raw pair +0x6c/+0x78, +0x5ab8 (int1), +0x94/+0x8c8/+0x8cc (int4), then
    // the vectors at +0x8bc and +0x59f8, the struct-of-arrays block at +0x5a08 and the 8-byte vector +0x59d8.
    out->w_int((int*)((char*)this + 0x50));
    out->w_int4((int*)((char*)this + 0x1c));
    out->w_struct((char*)this + 0x54);
    OutStream* s3 = &out->w_int((int*)((char*)this + 0x7c));
    s3 = &s3->w_int((int*)((char*)this + 0x80));
    s3->w_int((int*)((char*)this + 0x84));
    out->w_int((int*)((char*)this + 0x8f4));
    out->w_int((int*)((char*)this + 0x958));
    out->w_raw((char*)this + 0x6c);
    out->w_int((int*)((char*)this + 0x78));
    out->w_int1((int*)((char*)this + 0x5ab8));
    out->w_raw((char*)this + 0x94);
    out->w_raw((char*)this + 0x8c8);
    out->w_int4((int*)((char*)this + 0x8cc));
    write_vec4((List4*)((char*)this + 0x8bc), out);
    write_vec4((List4*)((char*)this + 0x59f8), out);
    write_soa(out, (char*)this + 0x5a08);
    f59d8.write8(out);
    return;
}

// @ 0x4e9fe0    void RecList::flush_ids()   (thiscall, ret 0)   drop the records whose type is 2
void RecList::flush_ids()
{
    unsigned i = 0;
    if (count > 0) {
        do {
            char** base = (char**)f138d0;
            unsigned idx = i >> 4;
            char* page = base[idx];
            Rec5ac8* r = (Rec5ac8*)(page + (i & 0xf) * (int)sizeof(Rec5ac8));
            if (r->type == 2)
                id_remove(r->id);
            i++;
        } while ((int)i < count);
    }
}
// @ 0x4ea030    int RecList::id_remove(int id)   (thiscall, ret 4)   the 0x2717 open-addressed set
//   The receiver (ecx) is the set itself -- RecList + 0x138bc. The record behind the slot is type=0'd and the
//   slot index is passed on to int_remove (0x440b30) with the set pointer.
int RecList::id_remove(int id)
{
    int* set = (int*)this;
    int  probe = 0;
    int  slot;
    if (id < 0x2717) {
        slot = id;
    } else {
        slot = id % 0x2717;
        while (set[slot * 2] != id) {
            if (probe >= 0x2717)
                break;
            slot = (slot + 0x4f) % 0x2717;
            probe++;
        }
    }
    if (set[slot * 2] == id) {
        int idx = set[slot * 2 + 1];
        set[slot * 2] = -1;
        set[slot * 2 + 1] = -1;
        rec_at(this, idx)->type = 0;
        return ((IntVec*)((char*)this + 0x138bc))->int_remove(idx);
    }
}