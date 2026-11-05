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
#include "../../rgl/rgl_math.h"

#define MB_NOTHROW throw()

// ---- tstring (see s0042aaf0 / s004e2ea0) ----------------------------------------------------------------
#include "../tstring.h"

// ---- the writer / the 8-byte vector / the troop record (the slices' GameRecord) --------------------------
#include "../out_stream.h"
#include "../../include/party.h"                 // PartyMgr, Party (write_table, flush_ids, id_remove, write)
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
#include "./cs_common.h"                       // Map10007, std::vector<int>::push_back
#include "../int_vec.h"


// the int-vector writer the original inlines at +0x8b8 and +0x59f4: the count, then each int
static __forceinline void write_vec4(std::vector<int>& v, OutStream* out)
{
    int n;
    if (!v._Myfirst)
        n = 0;
    else
        n = (int)(v._Mylast - v._Myfirst);
    out->w_count(&n);
    for (unsigned i = 0; i < (!v._Myfirst ? 0u : (unsigned)(v._Mylast - v._Myfirst)); i++)
        out->w_raw(&v._Myfirst[i]);
}

#endif
