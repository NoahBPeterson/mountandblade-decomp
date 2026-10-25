// module_load.h (was slice s004d0c20): (module-system record loaders). Provisional names.
#ifndef OPTIONS_PANEL_MODULE_LOAD_H
#define OPTIONS_PANEL_MODULE_LOAD_H
#include <stdio.h>
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../tstring.h"

// vtable 0x54ac88 (stream.h: tstream3) with its constructor inline, as 0x4d0c20 builds it on the stack (the
// same device as text_reader in src/rgl/core/resmgr.h). open_read_text (0x415d60) is text_stream's member.
// (derived without overrides: its vtable holds tstream3's members, which src/rgl/core/resource_io.cpp defines)
struct text_reader_l : tstream3 {};

// the item modifier table (Data\item_modifiers.txt): 43 records of 0x28 bytes at 0xa75f70, read by 0x4d0c20; the
// game reads name / key as src/rgl/mesh2/gui_edit.h's ModRec, the two factors from +0x20 (item_ref.h's g_sort_val)
struct item_mod {
    tstring name;       // +0     (underscores shown as spaces)
    tstring key;        // +0x10  read first
    float price;        // +0x20
    float rarity;       // +0x24
};
extern item_mod g_item_mods[43];     // 0xa75f70

// the record type of item_modifier_array_dtor (0x4d10f0, shared by identical-code folding): a name, a string, a
// flag and an id / value array pair. Not the item modifier table above.
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
};

// ground spec record (0x4c bytes, the table data\skyboxes.txt fills; detail in s004bddf0): a name at +0 that the
// array new of 0x4d0c20 constructs (with the element count stored before the array)
struct ground_spec {
    tstring name;       // +0
    char pad10[0x4c - 0x10];
};

// mission type record (0x20 bytes): detail in 0x4c2dd0.
struct mission_type {
    char pad0[0x20];
};

// The allocators of the three record vectors: construct (0x4d1140/0x4d1160/0x4d1180) copy-constructs one
// element in place through the record's copy_from.
struct flora_alloc { void construct(flora_kind* dst, const flora_kind& src); };
struct ground_alloc { void construct(ground_spec* dst, const ground_spec& src); };
struct mission_alloc { void construct(mission_type* dst, const mission_type& src); };

// "ttype" record (0x20 bytes, array at 0x8ed4df8).
struct ttype {
    char pad0[0x20];
};

// The record classes of the readers the loaders call (the engine's own names; mesh_2.cpp / rec_tables.cpp / config.cpp define
// them, headers that cannot be included next to this one's stream.h): the mission-type table entry is rec_tables.cpp's Blk2dd0
// (read 0x4c2dd0), the ttype entry config.cpp's rec20 (read 0x4be600), the ground spec config.cpp's prsnt_record (load_extras
// 0x4bddf0), and the flora kind mesh_2.cpp's ObjectList (read 0x4cdfd0).
struct Blk2dd0 { void read(stream_base* s); };
struct rec20 { void read(stream_base* s); };
struct prsnt_record { void load_extras(stream_base& s); };
struct ObjectList { void read(stream_base* s); };

// Destroy helpers: 0x4c8ba0 is gui_edit.cpp's conv_list_dtor (fastcall, the element in ecx), 0x4c8ab0 its conv_sub_dtor
// (stdcall, the element pushed).
struct conv_rec;
void __fastcall conv_list_dtor(conv_rec* self);     // 0x4c8ba0
void __stdcall conv_sub_dtor(void** self);          // 0x4c8ab0

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
    int f7c;                        // +0x7c (-1)
    rgl::vector4 f80;               // +0x80 = (1,1,1) normalised
    int f90;                        // +0x90 = 1
    growable_array();
};
#endif
