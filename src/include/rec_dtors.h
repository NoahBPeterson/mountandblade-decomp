// rec_dtors.h: RecD (0x78 bytes), RecI (0x68), RecK (0x34) and OwnedRec48 (0x48), engine records whose vector deleting
// destructors (??_E) are 0x431480 / 0x431810 / 0x4318c0 / 0x431960, with SubA / SubB, the (array, name) pairs they embed.
// ONE definition for src/rgl/xmlcfg (records.cpp: emits the destructors), src/rgl/mesh2 (rec_tables.cpp deletes a RecD,
// gui_edit.cpp a RecI), src/game/static_init (RecK) and src/game/cls_map (OwnedRec48): each user writes `delete[]`, which
// calls the one ??_E.
#ifndef MB_REC_DTORS_H
#define MB_REC_DTORS_H
#include <new>
#include <vector>
#include "../game/tstring.h"
#include "./poly.h"          // Poly: the owned object of OwnedRec48

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
    ~RecD();   // 0x431440
    void* scalar_delete(unsigned flags);        // 0x431480 (??_ERecD, ret 4): rec_tables.cpp (elem58) and world_data.cpp call it by address; `delete[]` there changes the registers of the matched functions
};
// 0x68 bytes: three names, two pairs. ~RecI 0x431660, vector deleting destructor 0x431810.
struct RecI {
    tstring n0, n1, n2; // +0, +0x10, +0x20
    SubA s1;            // +0x30
    SubA s2;            // +0x4c
    ~RecI();
    void* destroy(unsigned flags);              // 0x431810 (??_ERecI): gui_edit.cpp (elem60), same reason
};

// 0x34 bytes: a name at +4 and a pair at +0x18; vector deleting destructor 0x4318c0.
struct RecK {
    int head;
    tstring name;       // +4
    int pad;
    SubA s;             // +0x18
};
struct Owner { Poly* p; ~Owner() { delete p; } };

// 0x48 bytes: name, vector<int> at +0x10, owned polymorphic object at +0x24. Vector deleting dtor 0x431960.
struct OwnedRec48 {
    tstring name;               // +0
    std::vector<int> v;         // +0x10
    int pad20;
    Owner owned;                // +0x24
    char tail[0x20];
};

// A std::vector<tstring>-shaped tail (allocator, first, last, end): 0x10 bytes, destructor 0x42d390 (the tidy of gui_manager).
struct Tail { int a, b, c, d; ~Tail(); };

// Records of the game object's teardown (0x492a20): only their out-of-line destructors are called. ONE definition for
// src/rgl/xmlcfg (records.cpp emits the destructors) and src/game/world_data (world_data.h, which kept opaque stand-ins).
// Non-virtual destructor body 0x431b10: vector at +0x128, eight names at +0x88, five names, name at +0.
struct RecQ {
    tstring n0, n1, n2, n3, n4;     // +0 .. +0x40
    int pad50;
    tstring n5;                     // +0x54
    char pad64[0x88 - 0x64];
    tstring names[8];               // +0x88
    char pad108[0x128 - 0x108];
    std::vector<int> v;             // +0x128
    ~RecQ();
};
// Non-virtual destructor body 0x431c40: vector at +0x4f4, eight names at +0x420, two names, name at +0.
struct RecS {
    tstring n0;                     // +0
    tstring n1;                     // +0x10
    tstring n2;                     // +0x20
    char pad30[0x420 - 0x30];
    tstring names[8];               // +0x420
    char pad4a0[0x4f4 - 0x4a0];
    std::vector<int> v;             // +0x4f4
    char pad504[0x508 - 0x504];     // troop record, 0x508 bytes
    ~RecS();
};
// Non-virtual destructor body 0x432010: eight names at +0x27c, vector at +0x24c, Tail at +0x234, names at +0x14, +0.
struct RecT {
    tstring n0;                     // +0
    int pad10;
    tstring n1;                     // +0x14
    char pad24[0x234 - 0x24];
    Tail t;                         // +0x234 (0x10 bytes: world_app.cpp keeps two of them 0x10 apart at Mission+0x19c / +0x1ac)
    char pad244[0x24c - 0x244];
    std::vector<int> v;             // +0x24c
    char pad25c[0x20];
    tstring names[8];               // +0x27c
    char pad2fc[0x300 - 0x2fc];     // faction record, 0x300 bytes (0x51af4e indexes them; the old 0x5330 pad was wrong)
    ~RecT();
};

#endif // MB_REC_DTORS_H
