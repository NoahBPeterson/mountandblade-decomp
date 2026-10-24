// head_rec.h: the base of the "head record" family (editor / module / trigger tables): vtable 0x5947d8, a status int at +4 (0),
// the constant 0x63 at +8 and a name tstring at +0x1c (ctor 0x4c2c50 inlined; layout evidence in src/rgl/mesh2/head_rec.h).
// ONE definition for src/game (options_panel/trigger_records.h) and src/rgl (mesh2/head_rec.h).
#ifndef MB_GAME_HEAD_REC_H
#define MB_GAME_HEAD_REC_H
#include "./tstring.h"

struct head_rec {
    virtual void vfunc();      // slot 0 (only the vptr store matters)
    int     w04;               // +4     0x4c2c50
    int     w08;               // +8     0x63
    char    pad0c[0x10];       // +0xc
    tstring name;              // +0x1c
    head_rec() { w04 = 0; w08 = 0x63; }
};

#endif // MB_GAME_HEAD_REC_H
