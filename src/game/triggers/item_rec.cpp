//-----------------------------------------------------------------------------
// File: item_rec.cpp  (0x4ce870..0x4ce960)
//
// Desc: constructors of the two "item" head records and their shared deleting destructor. Names provisional.
//       Flags: /O2 /MT /Gy /TP. Boundary: see MIGRATION.md (class change only; no call evidence either way).
//-----------------------------------------------------------------------------
#include "./item_rec.h"

// @ 0x4ce870
ItemRec::ItemRec()
{
    w04 = 0;
    owned.begin = 0;
    owned.end = 0;
    owned.cap = 0;
    clock = 0;
    stamp = g_ticks[0];
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// @ 0x4ce8d0
ItemRec2::ItemRec2()
{
    w04 = 0;
    owned.begin = 0;
    owned.end = 0;
    owned.cap = 0;
    clock = 0;
    stamp = g_ticks[0];
    f5c = 0;
    f58 = 1.0f;
    f40 = -1;
}

// 0x4ce930 is the compiler-generated deleting destructor of ItemRec (ICF also serves ItemRec2): item_head's virtual
// destructor, the owned block's destructor 0x4c8bf0 and operator delete.
