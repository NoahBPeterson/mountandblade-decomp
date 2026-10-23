// records.h: the engine record classes whose destructors / deleting destructors sit at 0x431400..0x432080,
// and the std::vector<Item> helpers they use. One header for records.cpp. Class and field names are
// provisional; only sizes, offsets and destruction orders are verified (by the byte-exact destructors cited at
// each class). The classes are only seen through `delete`/`delete[]` here: the objects are created by later
// objects (0x4c1c90, 0x4c3430, 0x4da5b0, ...), so most fields are padding.
//
// tstring is the shared src/game/tstring.h (16 bytes, dtor 0x42bc40). IfaceB/RecJ/Mission-like classes of
// src/game/agent/agent.h are NOT reused: agent.h declares RecJ's constructor only, but the vector deleting
// destructor 0x431750 is emitted here, from a definition that has the constructor. See MIGRATION.md.
#ifndef RGL_XMLCFG_RECORDS_H
#define RGL_XMLCFG_RECORDS_H
#include <new>
#include <vector>
#include <list>
#include "../../game/tstring.h"

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

// Two abstract-looking bases (vtables 0x5908ec and 0x5908f8; slots 1 and 2 are the empty function 0x477b70).
// Their scalar deleting destructors are slot 0 (0x431400, 0x431420).
struct IfaceA {
    IfaceA();
    virtual ~IfaceA() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct IfaceB {
    IfaceB();
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};

// 0x78 bytes: two (array, name) pairs. ~RecD 0x431440, vector deleting destructor 0x431480.
struct RecD {
    int pad[4];
    SubA s1;            // +0x10
    SubA s2;            // +0x2c
    int tail[12];
    ~RecD();
};
// 0x30 bytes, one pair at +4: vector deleting destructor 0x431540.
struct RecG {
    int head;
    SubA s;             // +4
    int tail[4];
};
// 0x30 bytes: name at +0, pair at +0x10 with an unconditional delete[]: 0x4315d0.
struct RecH {
    tstring name;       // +0
    SubB s;             // +0x10
    int tail;
};
// 0x68 bytes: three names, two pairs. ~RecI 0x431660, vector deleting destructor 0x431810.
struct RecI {
    tstring n0, n1, n2; // +0, +0x10, +0x20
    SubA s1;            // +0x30
    SubA s2;            // +0x4c
    ~RecI();
};
// A class whose only member is a tstring: its destructor is a tail jump to 0x42bc40 (0x4316b0).
struct NameHolder {
    tstring name;
    ~NameHolder();
};
// IfaceB-derived, 0x98 bytes (vector deleting destructor 0x4316c0).
struct RecF : IfaceB {
    int pad[4];
    tstring name;       // +0x14
    int tail[29];
    RecF();
};
// IfaceB-derived, 0xb0 bytes (vector deleting destructor 0x431750).
struct RecJ : IfaceB {
    int body[0x2b];
    RecJ();
};
// Owner of a heap array of RecG: frees it and clears the pointer (0x4317b0).
struct RecGList {
    int head;
    RecG* items;        // +4
    void clear();
};
// 0x34 bytes: a name at +4 and a pair at +0x18; vector deleting destructor 0x4318c0.
struct RecK {
    int head;
    tstring name;       // +4
    int pad;
    SubA s;             // +0x18
};

// ---- from 0x431960 on --------------------------------------------------------------------------------------
// Polymorphic owned object (only its virtual destructor is called): src/include/poly.h
#include "../../include/poly.h"
// 0x1c8-byte polymorphic record in a std::vector (helpers 0x431a30 / 0x431a60).
struct Item {
    virtual ~Item();
    char body[0x1c4];
};
// out-of-line destructors of later objects, called on members
struct Tail  { char body[0x18]; ~Tail(); };     // 0x42d390 (the std::vector<tstring> tidy of gui_manager)
struct Tail2 { char body[0x20]; ~Tail2(); };    // 0x4c4d10

struct Owner { Poly* p; ~Owner() { delete p; } };

// 0x48 bytes: name, vector<int> at +0x10, owned polymorphic object at +0x24. Vector deleting dtor 0x431960.
struct RecA {
    tstring name;               // +0
    std::vector<int> v;         // +0x10
    int pad20;
    Owner owned;                // +0x24
    char tail[0x20];
};
// 0x34 bytes: vector<int> at +0, list<int> at +0x10, vector<int> at +0x20. ~RecP 0x431ab0, ??_E 0x431d30.
struct RecP {
    std::vector<int> a;         // +0
    std::list<int> l;           // +0x10
    int pad1c;
    std::vector<int> b;         // +0x20
    int tail;
    ~RecP();
};
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
// IfaceA-derived, 0x5ac8 bytes. ~RecR 0x431b90 (implicit), vector deleting destructor 0x431cb0.
struct RecR : IfaceA {
    char pad4[8];
    tstring n0;                     // +0xc
    char pad1c[4];
    tstring n1;                     // +0x20
    char pad30[0x54 - 0x30];
    tstring n2;                     // +0x54
    char pad64[0x8b8 - 0x64];
    std::vector<int> v0;            // +0x8b8
    char pad8c8[0x59d8 - 0x8c8];
    std::vector<int> v1;            // +0x59d8
    char pad59e8[0xc];
    std::vector<int> v2;            // +0x59f4
    char pad5a04[0x5a28 - 0x5a04];
    tstring names[8];               // +0x5a28
    char tail[0x5ac8 - 0x5aa8];
    RecR();
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
    ~RecS();
};
// IfaceB-derived, 0x494 bytes, own vtable 0x590904. ~RecY 0x431e10, vector deleting dtor 0x431ec0.
struct RecY : IfaceB {
    char pad4[0xc];
    tstring n0;                     // +0x10
    tstring n1;                     // +0x20
    char pad30[4];
    tstring n2;                     // +0x34
    tstring n3;                     // +0x44
    char pad54[0x68 - 0x54];
    tstring n4;                     // +0x68
    char pad78[0x104 - 0x78];
    std::vector<Item> items;        // +0x104
    char pad114[0x418 - 0x114];
    tstring n5;                     // +0x418
    char pad428[0xc];
    tstring n6;                     // +0x434
    char pad444[4];
    tstring n7;                     // +0x448
    char pad458[0xc];
    tstring n8;                     // +0x464
    std::vector<int> v;             // +0x474
    tstring n9;                     // +0x484
    RecY();
    virtual ~RecY();
};
// IfaceB-derived, 0x5928 bytes: vector<int> at +0x1c, Tail2 at +0x53c0 (vector deleting dtor 0x431f40).
struct RecZ : IfaceB {
    char pad4[0x18];
    std::vector<int> v;             // +0x1c
    char pad2c[0x53c0 - 0x2c];
    Tail2 t;                        // +0x53c0
    char tail[0x5928 - 0x53e0];
    RecZ();
};
// Non-virtual destructor body 0x432010: eight names at +0x27c, vector at +0x24c, Tail at +0x234, names at +0x14, +0.
struct RecT {
    tstring n0;                     // +0
    int pad10;
    tstring n1;                     // +0x14
    char pad24[0x234 - 0x24];
    Tail t;                         // +0x234
    std::vector<int> v;             // +0x24c
    char pad25c[0x20];
    tstring names[8];               // +0x27c
    char pad2fc[0x5330 - 0x2fc];
    ~RecT();
};
// 0x5340-byte object: the reset at 0x432080 clears five fields (offsets as stored there).
struct BigObj {
    char pad[0x5340];
    void reset();                   // 0x432080
};

#endif
