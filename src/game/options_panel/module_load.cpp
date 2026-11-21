// module_load.cpp (was slice s004d0c20): the module-system record loaders (item modifiers, flora kinds, ground specs, skyboxes,
// mission types) and the load subroutine that calls them. All of them drive the engine's text streams
// (stream_base vtable 0x54a740, tstream2 0x54a890, tstream3 0x54ac88; see mbbin.h) and the item-kind array.
// Names provisional; class names follow the Module System's record names.
#include <windows.h>
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include <new>
#include <memory>
#include <d3d9.h>
#include "./module_load.h"
#include "../../rgl/stream_inline.h"  // the readers' ctor/dtor are inlined here (0x4172d0 / 0x417310)
#include "./elem_calls.h"      // RagdollBone / E68 / FaceTexture: the copy constructors 0x4cd080 / 0x4cd100 / 0x4cd170
#include "../ptype.h"
#include "../../include/ground_type.h"   // GroundType, g_ground_types (0x8ed4df8)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
item_mod g_item_mods[43];                                   // 0xa75f70 (constructed and destroyed by the compiler: two tstrings per record)


// ---- string table (Data/strings.txt lookup, ctor 0x4230f0) ----
int find_string(const char* s);                  // 0x4230f0 (this = 0x8ed3c0)

// ---- module-system record arrays -------------------------------------------------------------------------
// flora kinds: count (0x5e29e8) + heap array of 0x20-byte records (+0 name, +0x18/+0x1c ints).
ptype* g_ptypes;                                 // 0x5e28f8 (flora_kinds.txt: the 0x20-byte ground / flora type records)
int g_ptype_count;                               // 0x5e29e8

// ground specs: count (0x5e29ec) + heap array of 0x4c-byte records.
EnvRow* g_env_rows;                              // 0x5e28f4
int g_num_ground_specs;                          // 0x5e29ec

// mission-type table: 0x20-byte records 0xa735d0..0xa73730.
mission_type g_mission_types[(0xa73730 - 0xa735d0) / 0x20];   // 0xa735d0 (.bss; built / destroyed by the compiler: see mission_type.h)

GroundType g_ground_types[11];                    // 0x8ed4df8 (the 11 records up to the mesh table at 0x8ed4f58; constructed and destroyed by the compiler: a name string each)

void load_skyboxes();                            // 0x4c3350

// std::allocator<FaceTexture>::destroy (0x4c8ab0, mesh_2.cpp; the unwind path of 0x4d12c0 calls it)
namespace std { template<> void allocator<FaceTexture>::destroy(FaceTexture*); }

// ---- std::vector<RagdollBone>'s helpers the linker placed here: the header bodies, explicitly instantiated ----
// @ 0x4d10f0 (the bone list's destroy: ~RagdollBone inline, the capsules' buffer then the name)
template void std::_Destroy_range(RagdollBone*, RagdollBone*, std::allocator<RagdollBone>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x4d11a0 (calls std::allocator<RagdollBone>::construct 0x4d1140, emitted with it, and destroy 0x4cd6a0)
template RagdollBone* std::_Uninit_copy(RagdollBone*, RagdollBone*, RagdollBone*, std::allocator<RagdollBone>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x4d1160 (std::vector<E68>'s allocator: the copy constructor 0x4cd100 behind placement new's null test)
template void std::allocator<E68>::construct(E68*, const E68&);

// @ 0x4d1180 (std::vector<FaceTexture>'s allocator: the copy constructor 0x4cd170 behind placement new's null test)
template void std::allocator<FaceTexture>::construct(FaceTexture*, const FaceTexture&);

// @ 0x4d1360
PolyMesh::PolyMesh()
{
    rgl::vector4 v(1.0f, 1.0f, 1.0f);
    stamp = 0;
    light = v;
    light.normalize();
    mode = 0;
    cur = -1;
    step = 1;
}

// @ 0x4d1230 (std::vector<E68>'s copy: construct 0x4d1160, the unwind path destroys through ~E68 0x4c8ba0: the header's
// inline destructor, which cl emits out of line for that path; that copy is the 0x4c8ba0 row, listed here)
template E68* std::_Uninit_copy(E68*, E68*, E68*, std::allocator<E68>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x4d12c0 (std::vector<FaceTexture>'s copy: construct 0x4d1180, the unwind path calls the allocator's destroy 0x4c8ab0)
template FaceTexture* std::_Uninit_copy(FaceTexture*, FaceTexture*, FaceTexture*, std::allocator<FaceTexture>&, std::_Nonscalar_ptr_iterator_tag);

// opens one of the engine's Data\*.txt tables; returns the reader as a base reference (so the callers'
// dispatch stays indirect, exactly as open_module_file 0x50d5e0 does)
static __forceinline stream_base& open_data_file(text_reader_l& ts, const char* name)
{
    if (!((text_file*)&ts)->open_read_text(name))
        mb_error("Unable to open %s", name);
    return ts;
}

// @ 0x4d0c20
// The engine's Data\*.txt tables (the module's own files are load_module_files 0x50d5e0, inventory_panel/module_files.cpp;
// the per-frame update 0x50ec40 and the loading screen 0x50f440 call this one first, then that one).
void load_data_files()
{
    text_reader_l ts1;
    stream_base& f1 = open_data_file(ts1, "Data\\item_modifiers.txt");
    for (int i = 0; i < 43; i++) {
        item_mod* m = &g_item_mods[i];
        f1.r_tstr(&m->key);
        f1.r_tstr(&m->name);
        m->name.underscores_to_spaces();
        f1.r_float(&m->price);
        f1.r_float(&m->rarity);
    }
    ts1.failed = 1;
    WL_MARKER(4)
    text_reader_l ts2;
    stream_base& f2 = open_data_file(ts2, "Data\\flora_kinds.txt");
    f2.r_int(&g_ptype_count);
    g_ptypes = new ptype[g_ptype_count];
    for (int i = 0; i < g_ptype_count; i++)
        g_ptypes[i].read(&ts2);                // 0x4cdfd0
    ts2.failed = 1;
    text_reader_l ts3;
    stream_base& f3 = open_data_file(ts3, "data\\ground_specs.txt");
    for (int i = 0; i < 11; i++)           // 11 (to 0x8ed4f58)
        g_ground_types[i].read(&ts3);      // 0x4be600
    ts3.failed = 1;
    // data\skyboxes.txt: a count, then that many 0x4c-byte records (the table at 0x5e28f4 / 0x5e29ec)
    text_reader_l ts4;
    stream_base& f4 = open_data_file(ts4, "data\\skyboxes.txt");
    f4.r_int(&g_num_ground_specs);
    g_env_rows = new EnvRow[g_num_ground_specs];
    for (int i = 0; i < g_num_ground_specs; i++)
        g_env_rows[i].load(ts4);                                  // 0x4bddf0
    ts4.failed = 1;
    text_reader_l ts5;
    stream_base& f5 = open_data_file(ts5, "Data\\mission_types.txt");
    for (int i = 0; i < 11; i++)           // 11 (to 0xa73730)
        ((Blk2dd0*)&g_mission_types[i])->read(&ts5);     // 0x4c2dd0
    ts5.failed = 1;
    WL_MARKER(5)
    load_skyboxes();                                        // 0x4c3350: the module's own skyboxes.txt
}
