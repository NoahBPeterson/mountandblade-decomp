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
#include "./elem_calls.h"      // E50 / E68 / E176: the copy constructors 0x4cd080 / 0x4cd100 / 0x4cd170
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

// std::allocator<E50>::destroy (0x4cd6a0, mesh_2.cpp; the unwind path of 0x4d11a0 calls it)
namespace std { template<> void allocator<E50>::destroy(E50*); }

// @ 0x4d10f0
void item_modifier_array_dtor(item_modifier* first, item_modifier* last)
{
    if (first != last) {
        void** b = (void**)&first->block;
        do {
            if (*b)
                operator delete(*b);             // 0x535712
            b[0] = 0;                            // zeroed whether or not there was a block (0x4d1116..0x4d111b)
            b[1] = 0;
            b[2] = 0;
            first->name.~tstring();
            first++;
            b = (void**)((char*)b + sizeof(item_modifier));
        } while (first != last);
    }
}

// @ 0x4d1140 (the allocator's construct: thiscall, `this` = the allocator, unused; callee pops 8)
__declspec(noinline) void flora_alloc::construct(flora_kind* dst, const flora_kind& src)
{
    if (dst != 0)
        new ((void*)dst) E50(*(const E50*)&src);   // 0x4cd080
}

// @ 0x4d1160 (the allocator's construct: thiscall, `this` = the allocator, unused; callee pops 8)
__declspec(noinline) void ground_alloc::construct(ground_spec* dst, const ground_spec& src)
{
    if (dst != 0)
        new ((void*)dst) E68(*(const E68*)&src);   // 0x4cd100
}

// @ 0x4d1180 (the allocator's construct: thiscall, `this` = the allocator, unused; callee pops 8)
__declspec(noinline) void mission_alloc::construct(mission_type* dst, const mission_type& src)
{
    if (dst != 0)
        new ((void*)dst) E176(*(const E176*)&src); // 0x4cd170
}

static __forceinline void zero_range(growable_array::range10& r)
{
    r.first = 0;
    r.last = 0;
    r.end = 0;
}

// @ 0x4d1360
growable_array::growable_array()
{
    zero_range(head);
    zero_range(items[0]);
    zero_range(items[1]);
    zero_range(items[2]);
    zero_range(items[3]);
    zero_range(items[4]);
    zero_range(items[5]);
    rgl::vector4 v(1.0f, 1.0f, 1.0f);
    f0 = 0;
    f80 = v;
    f80.normalize();
    items[5].spare = 0;
    f7c = -1;
    f90 = 1;
}

// the allocator wrappers of the three copy loops: their construct/destroy are the vector element's
// (flora_alloc::construct 0x4d1140 / ground_alloc 0x4d1160 / mission_alloc 0x4d1180 and the elements'
// destructors) as world_app.cpp's fill helpers spell them.
struct flora_copy_alloc {
    __forceinline void construct(E50* p, const E50& v) { ((flora_alloc*)(void*)this)->construct((flora_kind*)p, *(const flora_kind*)&v); }
    __forceinline void destroy(E50* p) { ((std::allocator<E50>*)(void*)this)->destroy(p); }
};



// @ 0x4d11a0 (std::_Uninit_copy's allocator variant, 0x50-byte element E50)
flora_kind* uninitialized_copy_flora(flora_kind* first, flora_kind* last, flora_kind* dst, flora_alloc& al)
{
    E50* next = (E50*)dst;
    try {
        for (; (E50*)first != (E50*)last; ++(E50*&)dst, ++(E50*&)first)
            ((flora_copy_alloc*)&al)->construct((E50*)dst, *(const E50*)first);
    } catch (...) {
        for (; next != (E50*)dst; ++next)
            ((flora_copy_alloc*)&al)->destroy(next);
        throw;
    }
    return dst;
}

// @ 0x4d1230 (std::_Uninit_copy's allocator variant, 0x68-byte element E68)
ground_spec* uninitialized_copy_ground(ground_spec* first, ground_spec* last, ground_spec* dst, ground_alloc& al)
{
    E68* next = (E68*)dst;
    try {
        for (; (E68*)first != (E68*)last; ++(E68*&)dst, ++(E68*&)first)
            al.construct((ground_spec*)dst, *(const ground_spec*)first);
    } catch (...) {
        for (; next != (E68*)dst; ++next)
            conv_list_dtor((conv_rec*)next);
        throw;
    }
    return dst;
}

// @ 0x4d12c0 (returns the advanced destination, as std::_Uninit_copy does)
mission_type* uninitialized_copy_mission(mission_type* first, mission_type* last, mission_type* dst, mission_alloc& al)
{
    // the vector's element is E176 (stride 0xb0), not the 0x20-byte table record
    for (E176* f = (E176*)first, *l = (E176*)last, *d = (E176*)dst; f != l; f++, d++, dst = (mission_type*)d)
        al.construct((mission_type*)d, *(const mission_type*)f);
    return dst;
}

// the flora-kind element `new[]` constructs: flora_kind's records have an inline constructor (name default-constructed,
// +0x10/+0x14 left as read, nframes/f1c zeroed), which is what the original's new[] calls per element
// (g_ptypes is read as ptype, so f18/f1c are ptype's nframes/boxes)
struct flora_kind_init : flora_kind {
    flora_kind_init() { f18 = 0; f1c = 0; }
};

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
    g_ptypes = (ptype*)new flora_kind_init[g_ptype_count];
    for (int i = 0; i < g_ptype_count; i++)
        ((ObjectList*)&g_ptypes[i])->read(&ts2);   // 0x4cdfd0
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
        ((prsnt_record*)&g_env_rows[i])->load_extras(ts4);        // 0x4bddf0
    ts4.failed = 1;
    text_reader_l ts5;
    stream_base& f5 = open_data_file(ts5, "Data\\mission_types.txt");
    for (int i = 0; i < 11; i++)           // 11 (to 0xa73730)
        ((Blk2dd0*)&g_mission_types[i])->read(&ts5);     // 0x4c2dd0
    ts5.failed = 1;
    WL_MARKER(5)
    load_skyboxes();                                        // 0x4c3350: the module's own skyboxes.txt
}
