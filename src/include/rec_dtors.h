// rec_dtors.h: RecD (0x78 bytes) and RecI (0x68 bytes), engine records whose destructors sit at 0x431440 / 0x431660 and whose
// vector deleting destructors (??_E) are 0x431480 / 0x431810, with SubA / SubB, the (array, name) pairs they embed.
// ONE definition for src/rgl/xmlcfg (records.cpp: emits the destructors) and src/rgl/mesh2 (rec_tables.cpp: deletes a RecD
// through scalar_delete 0x431480; gui_edit.cpp: deletes a RecI through destroy 0x431810). The two members below are the
// out-of-line ??_E entries those objects call; they are only declared (their addresses are bound in symbols/slices).
#ifndef MB_REC_DTORS_H
#define MB_REC_DTORS_H
#include <new>
#include "../game/tstring.h"

// A heap array pointer followed by two words and a name (0x1c bytes). Two destructor flavours occur: a
// null-checked free (SubA: 0x431440, 0x431480) and a bare `delete[]` (SubB: 0x4315d0), so two source types.
struct SubA {
    int* ptr;           // +0
    int a, b;
    tstring name;       // +0xc
    ~SubA() { if (ptr) delete[] ptr; }
};
struct SubB {
    int* ptr;
    int a, b;
    tstring name;
    ~SubB() { delete[] ptr; }
};

// 0x78 bytes: two (array, name) pairs. ~RecD 0x431440, vector deleting destructor 0x431480.
struct RecD {
    int pad[4];
    SubA s1;            // +0x10
    SubA s2;            // +0x2c
    int tail[12];
    ~RecD();
    void* scalar_delete(unsigned flags);        // 0x431480 (??_ERecD, ret 4), called from rec_tables.cpp
};
// 0x68 bytes: three names, two pairs. ~RecI 0x431660, vector deleting destructor 0x431810.
struct RecI {
    tstring n0, n1, n2; // +0, +0x10, +0x20
    SubA s1;            // +0x30
    SubA s2;            // +0x4c
    ~RecI();
    void* destroy(unsigned flags);              // 0x431810 (??_ERecI deleting destructor), called from gui_edit.cpp
};

#endif // MB_REC_DTORS_H
