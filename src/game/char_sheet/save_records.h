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
#include "../../rgl/stream.h"
#include "../../include/party.h"                 // PartyMgr, Party (write_table, flush_ids, id_remove, write)
#include "../slot_array.h"
#include "../../include/troop.h"

// ---- vectors --------------------------------------------------------------------------------------------
struct List4 {                                      // vector of 4-byte ints
    int* begin;                                     // +0x0
    int* end;                                       // +0x4
    int* endcap;                                    // +0x8
    int size() const { return begin ? (int)(end - begin) : 0; }
};
struct stream_base;
struct cPairs96 {                                   // the 0x60 8-byte pairs of a game record (+0xb4)
    ItemRef pair[0x60];
    __declspec(noinline) void write(stream_base* out); // 0x4e99a0 (ret 4)
};

// ---- the event pool's writer is Pool<Event>::write_table (include/pool.h), its record Event::write (game/event.h)
#include "../event.h"
#include "../../include/pool.h"

// ---- the object 0x4e9b50 actually receives (GameRecord + 0xf8) -------------------------------------------
// Its tail overlaps the Game object's own data: the record array at Game+0x13a00 sits at this-0x1c0.
#pragma pack(push, 4)
#pragma pack(pop)

// ---- the outer game object (only the record array is touched here) --------------------------------------
struct OuterGame {
    char        pad00[0x13a00];
    Troop* records;                            // +0x13a00
};

// ---- callees (slice s004e8f10 / others) -----------------------------------------------------------------
struct SoaBlock;
stream_base* write_soa(stream_base* out, SoaBlock* b);   // 0x4e9620 (cdecl, two stack args, returns out)
int        id_lookup(void* map, int id);            // 0x48ad00 (ret 4)
#include "./cs_common.h"                       // Map10007, std::vector<int>::push_back
#include "../int_vec.h"


// the int-vector writer the original inlines at +0x8b8 and +0x59f4: the count, then each int
static __forceinline void write_vec4(std::vector<int>& v, stream_base* out)
{
    unsigned n;
    if (!v._Myfirst)
        n = 0;
    else
        n = (int)(v._Mylast - v._Myfirst);
    out->w_uint(&n);
    for (unsigned i = 0; i < (!v._Myfirst ? 0u : (unsigned)(v._Mylast - v._Myfirst)); i++)
        out->w_int(&v._Myfirst[i]);
}

#endif
