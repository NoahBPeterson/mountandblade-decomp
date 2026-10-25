// module_load.cpp (was slice s004d0c20): the module-system record loaders (item modifiers, flora kinds, ground specs, skyboxes,
// mission types) and the load subroutine that calls them. All of them drive the engine's text streams
// (stream_base vtable 0x54a740, tstream2 0x54a890, tstream3 0x54ac88; see mbbin.h) and the item-kind array.
// Names provisional; class names follow the Module System's record names.
#include <windows.h>
#include <new>
#include <d3d9.h>
#include "./module_load.h"
#include "./elem_calls.h"      // E50 / E68 / E176: the copy constructors 0x4cd080 / 0x4cd100 / 0x4cd170

// ---- string table (Data/strings.txt lookup, ctor 0x4230f0) ----
int find_string(const char* s);                  // 0x4230f0 (this = 0x8ed3c0)

// ---- module-system record arrays -------------------------------------------------------------------------
// item modifiers: 0x28-byte records, 0xf02e slots at 0xa75f80; name at +0, modifier value at +0x10,
// hidden flag at +0x20, one id array at +0x24 and a value array at +0x28.
item_modifier g_item_modifiers[0xf02e];          // 0xa75f80
int g_num_item_modifiers;                        // 0xa76042

// flora kinds: count (0x5e29e8) + heap array of 0x20-byte records (+0 name, +0x18/+0x1c ints).
flora_kind* g_flora_kinds;                       // 0x5e28f8
int g_num_flora_kinds;                           // 0x5e29e8

// ground specs: count (0x5e29ec) + heap array of 0x4c-byte records.
ground_spec* g_ground_specs;                     // 0x5e28f4
int g_num_ground_specs;                          // 0x5e29ec

// mission-type table: 0x20-byte records 0xa735d0..0xa73730.
mission_type g_mission_types[(0xa73730 - 0xa735d0) / 0x20];   // 0xa735d0

ttype g_ttypes[(0x8ed4f58 - 0x8ed4df8) / 0x20];  // 0x8ed4df8

void load_skyboxes();                            // 0x4c3350

// @ 0x4d10f0
void item_modifier_array_dtor(item_modifier* first, item_modifier* last)
{
    if (first != last) {
        do {
            if (first->ids) {
                operator delete(first->ids);   // 0x535712
                first->ids = 0;
                first->values = 0;
                first->count = 0;
            }
            first->name.~tstring();
            first++;
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

// @ 0x4d1360
growable_array::growable_array()
{
    head.first = 0;
    head.last = 0;
    head.end = 0;
    items[0].first = 0; items[0].last = 0; items[0].end = 0;
    items[1].first = 0; items[1].last = 0; items[1].end = 0;
    items[2].first = 0; items[2].last = 0; items[2].end = 0;
    items[3].first = 0; items[3].last = 0; items[3].end = 0;
    items[4].first = 0; items[4].last = 0; items[4].end = 0;
    items[5].first = 0; items[5].last = 0; items[5].end = 0;
    rgl::vector4 v(1.0f, 1.0f, 1.0f);
    f80 = v;
    f0 = 0;
    f80.normalize();
    items[5].spare = 0;
    f7c = -1;
    f90 = 1;
}

// @ 0x4d11a0 (returns the advanced destination, as std::_Uninit_copy does)
flora_kind* uninitialized_copy_flora(flora_kind* first, flora_kind* last, flora_kind* dst, flora_alloc& al)
{
    for (; first != last; first++, dst++)
        al.construct(dst, *first);
    return dst;
}

// @ 0x4d1230 (returns the advanced destination, as std::_Uninit_copy does)
ground_spec* uninitialized_copy_ground(ground_spec* first, ground_spec* last, ground_spec* dst, ground_alloc& al)
{
    for (; first != last; first++, dst++)
        al.construct(dst, *first);
    return dst;
}

// @ 0x4d120a
// EH catch block: destroys the elements already constructed in [first, last) of a partly built range,
// then rethrows with `throw;`. (The address also sits inside text_stream's ctor as an EH funclet.)
void destroy_range_and_rethrow(ground_spec* first, ground_spec* last)
{
    for (; first != last; first++)
        conv_list_dtor((conv_rec*)first);
    throw;
}

// @ 0x4d129a
void destroy_range_ground(ground_spec* first, ground_spec* last)
{
    for (; first != last; first++)
        conv_list_dtor((conv_rec*)first);       // 0x4c8ba0 takes the element as its only argument
}

// @ 0x4d12c0 (returns the advanced destination, as std::_Uninit_copy does)
mission_type* uninitialized_copy_mission(mission_type* first, mission_type* last, mission_type* dst, mission_alloc& al)
{
    for (; first != last; first++, dst++)
        al.construct(dst, *first);
    return dst;
}

// @ 0x4d1330
void destroy_range_mission(mission_type* first, mission_type* last)
{
    for (; first != last; first++)
        conv_sub_dtor((void**)first);           // 0x4c8ab0 takes the element as its only argument
}

// @ 0x4d0c20
void load_module_files()
{
    {
        text_reader_l ts;
        if (!((text_stream*)&ts)->open_read_text("Data\\item_modifiers.txt"))
            mb_error("Unable to open %s", "Data\\item_modifiers.txt");
        g_num_item_modifiers = 0;
        for (item_modifier* m = g_item_modifiers; m < &g_item_modifiers[0xf02e]; m++) {
            ts.r_tstr((rgl::string*)&m->name);
            ts.r_tstr((rgl::string*)&m->value);
            m->value.underscores_to_spaces();
            ts.r_int(m->ids);
            ts.r_int(m->values);
        }
    }
    {
        text_reader_l ts;
        if (!((text_stream*)&ts)->open_read_text("Data\\flora_kinds.txt"))
            mb_error("Unable to open %s", "Data\\flora_kinds.txt");
        ts.r_int(&g_num_flora_kinds);
        g_flora_kinds = new flora_kind[g_num_flora_kinds];
        for (int i = 0; i < g_num_flora_kinds; i++)
            ((ObjectList*)&g_flora_kinds[i])->read(&ts);   // 0x4cdfd0
    }
    {
        text_reader_l ts;
        if (!((text_stream*)&ts)->open_read_text("data\\ground_specs.txt"))
            mb_error("Unable to open %s", "data\\ground_specs.txt");
        for (ttype* t = g_ttypes; t < &g_ttypes[6]; t++)
            ((rec20*)t)->read(&ts);           // 0x4be600
        g_ground_specs = new ground_spec[g_num_ground_specs];
        for (int i = 0; i < g_num_ground_specs; i++)
            ((prsnt_record*)&g_ground_specs[i])->load_extras(ts);        // 0x4bddf0
    }
    {
        text_reader_l ts;
        if (!((text_stream*)&ts)->open_read_text("data\\skyboxes.txt"))
            mb_error("Unable to open %s", "data\\skyboxes.txt");
        {
            text_stream2 ts2;
            load_skyboxes();
        }
    }
    {
        text_reader_l ts;
        if (!((text_stream*)&ts)->open_read_text("Data\\mission_types.txt"))
            mb_error("Unable to open %s", "Data\\mission_types.txt");
        for (mission_type* m = g_mission_types; m < &g_mission_types[12]; m++)
            ((Blk2dd0*)m)->read(&ts);                       // 0x4c2dd0
    }
}
