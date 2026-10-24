#ifndef SAVE_RECORDS_H
#define SAVE_RECORDS_H
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
#include <vector>
#include "rgl.h"

#define MB_NOTHROW throw()

// ---- tstring (see s0042aaf0 / s004e2ea0) ----------------------------------------------------------------
#include "../tstring.h"

// ---- the writer / the 8-byte vector / the troop record (the slices' GameRecord) --------------------------
#include "../out_stream.h"
#include "../list8.h"
#include "../../include/troop.h"

// ---- vectors --------------------------------------------------------------------------------------------
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
    List8    f59d8;                                 // +0x59d8 (vector of 8-byte elements, 0x10 bytes with the allocator word)
    char     pad59e8[0x59f8 - 0x59e8];
    List4    f59f8;                                 // +0x59f8 (vector of ints)
    char     pad5a04[0x5a08 - 0x5a04];
    char     f5a08[0xb0];                           // +0x5a08 (the struct-of-arrays block)
    int      f5ab8;                                 // +0x5ab8
    char     pad5abc[0x5ac8 - 0x5abc];
    void write(OutStream* out);                     // 0x4e9d10 (ret 4)
};

struct Slot20 { int f0, f4, f8, fc, f10, f14, f18, f1c; };
typedef int Quad[4];                                // the 4-int runs written at +0x8ec and +0x924
#pragma pack(pop)

#pragma pack(push, 4)
struct RecList {                                    // the record list of the GameRecord object at +0xf8
    char     pad00[0x78];
    int      f78;                                   // +0x78 (the id whose record is walked)
    char     pad7c[0x138b8 - 0x7c];
    int      count;                                 // +0x138b8
    char     pad13bc[0x138cc - 0x138bc];
    std::vector<Rec5ac8*> pages;                    // +0x138cc (VC7.1 vector: allocator word, then first/last/end) (index >> 4 -> page of 16 records)
    int      f138dc;                                // +0x138dc
    char     pad13e0[0x14050 - 0x13e0];
    int      f14050[3];
    int      f1405c[0x20];
    int  index_of(int id);                          // 0x48ad00 (ret 4)
    void write_table(OutStream* out);               // 0x4e9c50 (ret 4)
    void flush_ids();                               // 0x4e9fe0 (ret 0)
    void id_remove(int id);                         // 0x4ea030 (ret 4)
};
#pragma pack(pop)

// ---- the object 0x4e9b50 actually receives (GameRecord + 0xf8) -------------------------------------------
// Its tail overlaps the Game object's own data: the record array at Game+0x13a00 sits at this-0x1c0.
#pragma pack(push, 4)
struct OutRecList {
    char      pad00[0x78];
    int       f78;                                  // +0x78
    char      pad7c[0x13a00 - 0x7c];
    Troop* records;                            // +0x13a00 (party records, stride 0x508)
    char      pad13a04[0x1405c - 0x13a04];
    int       pk[0x20];                             // +0x1405c (0x20-int blocks, one per entry)
    void write_missions(OutStream* out);            // 0x4e9b50 (ret 4)
};
#pragma pack(pop)

// ---- the outer game object (only the record array is touched here) --------------------------------------
struct OuterGame {
    char        pad00[0x13a00];
    Troop* records;                            // +0x13a00
};

// ---- callees (slice s004e8f10 / others) -----------------------------------------------------------------
OutStream* write_soa(OutStream* out, char* base);   // 0x4e9620 (cdecl, two stack args, returns out)
int        id_lookup(void* map, int id);            // 0x48ad00 (ret 4)
#include "../int_vec.h"


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
    for (unsigned i = 0; i < (!v->begin ? 0u : (unsigned)(v->end - v->begin)); i++)
        out->w_raw(&v->begin[i]);
}

// the record behind the id table. Page P's +0 word is where page P+1 lives, so walking hoists the page
// pointer and leans on the index's scale factor.
static Rec5ac8* rec_at(RecList* l, int i)
{
    return &l->pages[(unsigned)i >> 4][i & 0xf];
}

#endif
