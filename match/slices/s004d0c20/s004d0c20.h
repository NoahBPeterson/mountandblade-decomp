// Declarations of slice s004d0c20 (module-system record loaders). Provisional names.
#ifndef S004D0C20_H
#define S004D0C20_H
#include <stdio.h>
#include "rgl.h"
#include "mbbin.h"

// item modifier record (0x28 bytes): name, value string, hidden flag, one id array and one value array.
struct item_modifier {
    tstring name;       // +0
    tstring value;      // +0x10
    char hidden;        // +0x20
    char pad21[3];
    int count;          // +0x24
    int* ids;           // +0x28
    int* values;        // +0x2c
};

// flora kind record (0x20 bytes): name, two ints.
struct flora_kind {
    tstring name;       // +0
    int f10;            // +0x10
    int f14;            // +0x14
    int f18;            // +0x18
    int f1c;            // +0x1c
    void copy_from(const flora_kind* src);  // 0x4cd080
};

// ground spec record (0x4c bytes): detail in s004bddf0.
struct ground_spec {
    char pad0[0x4c];
    void read(text_stream* s);              // 0x4bddf0
    void copy_from(const ground_spec* src); // 0x4cd100
    void destroy();                         // 0x4c8ba0
};

// mission type record (0x20 bytes): detail in 0x4c2dd0.
struct mission_type {
    char pad0[0x20];
    void read(text_stream* s);              // 0x4c2dd0
    void copy_from(const mission_type* src);    // 0x4cd170
    void destroy();                         // 0x4c8ab0
};

// The allocators of the three record vectors: construct (0x4d1140/0x4d1160/0x4d1180) copy-constructs one
// element in place through the record's copy_from.
struct flora_alloc { void construct(flora_kind* dst, const flora_kind& src); };
struct ground_alloc { void construct(ground_spec* dst, const ground_spec& src); };
struct mission_alloc { void construct(mission_type* dst, const mission_type& src); };

// "ttype" record (0x20 bytes, array at 0x8ed4df8).
struct ttype {
    char pad0[0x20];
    void read_ground_spec(text_stream* s);  // 0x4be600
};

// Destroy helpers (thiscall, `this` = the element).
void flora_kind_destroy(flora_kind* self);      // 0x4cd6a0
void ground_spec_destroy(ground_spec* self);    // 0x4c8ba0
void mission_type_destroy(mission_type* self);  // 0x4c8ab0

// A three-pointer range (the engine's arrays are built from these).
struct ptr_range {
    int* first;         // +0
    int* last;          // +4
    int* end;           // +8
};

// Object with one range at +8 and six 0x10-byte ranges at +0x1c (ctor 0x4d1360; the fourth dword of every
// range is left alone by the constructor, so the class is declared with a spare word).
struct growable_array {
    int f0;                         // +0
    int f4;                         // +4
    struct range10 { int* first; int* last; int* end; int spare; };
    range10 head;                   // +8
    int f18;                        // +0x18
    range10 items[6];               // +0x1c
    int f78;                        // +0x78
    int f7c;                        // +0x7c (-1)
    rgl::vector4 f80;               // +0x80 = (1,1,1) normalised
    int f90;                        // +0x90 = 1
    growable_array();
};
#endif
