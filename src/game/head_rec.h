// head_rec.h: the base of the "head record" family (editor / module / trigger tables): vtable 0x5947d8, a status int at +4 (0),
// the constant 0x63 at +8 and a name tstring at +0x1c (ctor 0x4c2c50 inlined; layout evidence in src/rgl/mesh2/head_rec.h).
// ONE definition for src/game (options_panel/trigger_records.h) and src/rgl (mesh2/head_rec.h).
#ifndef MB_GAME_HEAD_REC_H
#define MB_GAME_HEAD_REC_H
#include "./tstring.h"

struct head_rec {
    virtual void vfunc();      // slot 0 (only the vptr store matters)
    // The original table at 0x5947d8 (and every record table derived from it) has NINE slots; the relink patches only
    // the slots our compiled table has, so a one-slot declaration left slots 1..8 out of reach (slot 1 is `release`,
    // 0x4cb680, the rest are the record's other virtuals). The names are placeholders: the relink writes the original's
    // own functions into the slots, and slots whose original target has no symbol stay as these no-ops.
    virtual void d01() {}
    virtual void d02() {}
    virtual void d03() {}
    virtual void d04() {}
    virtual void d05() {}
    virtual void d06() {}
    // the relink can only patch slots that exist in our compiled table, so a one-slot declaration left slots 1..8
    // unreachable (several 0x4xxxxx functions were never installed or called). Declare the remaining eight; the names are
    // placeholders, the relink writes the original's own functions into the slots.
    int     w04;               // +4     0x4c2c50
    int     w08;               // +8     0x63
    char    pad0c[0x10];       // +0xc
    tstring name;              // +0x1c
    head_rec() { w04 = 0; w08 = 0x63; }
};

#endif // MB_GAME_HEAD_REC_H
