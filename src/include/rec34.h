// rec34.h: rec34, the 0x34-byte record of the module-file table loaded through rec34::read 0x4be470 (name at +4, a float at +0x14,
// then the script block DialogBlock at +0x18). ONE definition for src/game/config (config.cpp: read 0x4be470, the table loader)
// and src/rgl/mesh2 (gui_edit.cpp 0x4c48d0 builds the array: tstring ctor at +4, DialogBlock ctor at +0x18 inlined, calls 0x4be470
// on each element, vector deleting destructor 0x4318c0).
#ifndef MB_REC34_H
#define MB_REC34_H
#include "./dialog_rec.h"       // DialogBlock, Elem, tstring

struct rec34 {                                      // 0x34 bytes
    int     f00;                                    // +0x00
    tstring name;                                   // +0x04 (ctor at 0x4c4970)
    float   f14;                                    // +0x14
    DialogBlock body;                               // +0x18 (items +0x18, count +0x1c, done +0x20, text +0x24: 0x4c4980..0x4c498d)
    int read(stream_base* s);                       // 0x4be470
};

#endif // MB_REC34_H
