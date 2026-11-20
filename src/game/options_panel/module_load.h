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
#include "../../include/item_mod.h"         // item_mod, g_item_mods (0xa75f70)

#include "../env_row.h"           // EnvRow: the 0x4c-byte record of the table data\skyboxes.txt fills (g_env_rows 0x5e28f4)
// the element the std::vector allocator of 0x4d1160 / 0x4d1230 / 0x4d129a copies (0x68 bytes in the vector; a name at +0)
struct ground_spec {
    tstring name;       // +0
    char pad10[0x4c - 0x10];
};

// mission type record (0x20 bytes): detail in 0x4c2dd0.
#include "../mission_type.h"            // mission_type (0xa735d0 table): a name, two ints and the owned options

// The allocator of the 0x68-byte record vector (the 0x50- and 0xb0-byte ones are std::allocator<RagdollBone> / <FaceTexture>,
// whose construct 0x4d1140 / 0x4d1180 module_load.cpp instantiates): construct (0x4d1160) copy-constructs one element in place.
struct ground_alloc { void construct(ground_spec* dst, const ground_spec& src); };


// The record classes of the readers the loaders call (the engine's own names; mesh_2.cpp / rec_tables.cpp / config.cpp define
// them, headers that cannot be included next to this one's stream.h): the mission-type table entry is rec_tables.cpp's Blk2dd0
// (read 0x4c2dd0), the ground type entry GroundType (include/ground_type.h; read 0x4be600) and the flora kind mesh_2.cpp's
// ObjectList (read 0x4cdfd0); the skybox row EnvRow (env_row.h) reads itself (EnvRow::load 0x4bddf0, config.cpp).
struct Blk2dd0 { void read(stream_base* s); };
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

#include "../terrain_b/poly_mesh.h"   // PolyMesh (constructor 0x4d1360)
#endif
