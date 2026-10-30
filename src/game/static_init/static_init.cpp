// static_init.cpp: the game's dynamic initializers (the C++ initializer table __xc_a..__xc_z at 0x5a8008..0x5a80bc)
// and the destructors they register with atexit (0x548130..0x548570).
//
// In the original these are compiler-generated: each is the constructor of a file-scope global of one of the game's
// TUs, emitted at the end of .text by the linker in link order. The globals themselves are not defined in src/ (the
// standalone link takes their storage from the original image's data, tools/relink/glue.py), so the initializers are
// written out here explicitly against that storage and registered in .CRT$XCU in the original table order. A
// constructor the original inlined (vtable store, member constructors, field stores) is written out as the stores it
// makes; the vtables it stores are the original's (their slots point at our functions after the glue relocates them).
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

// ---- storage (labels in the original data, by address) ----------------------------------------------------------
extern char g_5d6fa0[];     // 0x5d6fa0 the resource manager
extern char g_5d7080[];     // 0x5d7080 the gui manager
extern unsigned g_5a8e78;   // 0x5a8e78 (size mask)
extern unsigned g_5a8e88;   // 0x5a8e88 (type mask)
extern unsigned g_5e2464;   // 0x5e2464
extern unsigned g_5e2468;   // 0x5e2468
extern float g_549cdc;      // 0x549cdc
extern float g_5a9be0;      // 0x5a9be0
extern float g_5a9be8;      // 0x5a9be8
extern unsigned g_5e2584;   // 0x5e2584
extern unsigned g_5e2580;   // 0x5e2580
extern unsigned g_8ee41ec;  // 0x8ee41ec
extern unsigned g_8ee41e8;  // 0x8ee41e8
extern float g_5a9f20;      // 0x5a9f20
extern float g_5a9f28;      // 0x5a9f28
extern char g_5e2a18[];     // 0x5e2a18 the string tables
extern char g_5a9f30[];     // 0x5a9f30 the debug text log (vtable, two strings)
extern char g_5a9f60[];     // 0x5a9f60 the log file stream (vtable, two strings, flags, FILE*)
extern char g_5fe898[];     // 0x5fe898
extern char g_5e2948[];     // 0x5e2948 the tick counter (64-bit)
extern char g_a178f0[];     // 0xa178f0
extern char g_a17900[];     // 0xa17900
extern char g_a170d4[];     // 0xa170d4 the key state table (0x100 dwords)
extern char g_a178d4[];     // 0xa178d4
extern char g_a178e0[];     // 0xa178e0
extern char g_a17910[];     // 0xa17910 the display state
extern char g_a180e8[];     // 0xa180e8 the skeleton name vector (allocator, first, last, end)
extern char g_a180f8[];     // 0xa180f8 module.ini settings (name, then the values)
extern char g_a181f8[];     // 0xa181f8 the physics set (vector of E56)
extern char g_a18238[];     // 0xa18238 module.ini (ini file: 0x1000 entries of 0x2c)
extern char g_a44238[];     // 0xa44238 (end of its entries)
extern char g_a44240[];     // 0xa44240 the game config (ini file)
extern char g_a70240[];     // 0xa70240 (end of its entries)
extern char g_a70248[];     // 0xa70248
extern char g_a70260[];     // 0xa70260 the scene
extern char g_a70438[];     // 0xa70438
extern char g_a70440[];     // 0xa70440
extern char g_a73300[];     // 0xa73300 the mission context
extern char g_a735d0[];     // 0xa735d0 11 templates of 0x20
extern char g_a73730[];     // 0xa73730 the game keys (0x4a dwords)
extern char g_a7374c[];     // 0xa7374c (the key records' end)
extern char g_a73980[];     // 0xa73980 the main menu
extern char g_a739f8[];     // 0xa739f8
extern char g_a73a10[];     // 0xa73a10 the music player
extern char g_a75a28[];     // 0xa75a28 (end of 0x100 strings of 0x20)
extern char g_a75e8c[];     // 0xa75e8c
extern char g_a75ea0[];     // 0xa75ea0 the game strings
extern char g_a75eb0[];     // 0xa75eb0 the scripts
extern char g_a75f2c[];     // 0xa75f2c the presentations
extern char g_a75f40[];     // 0xa75f40 the tableaus
extern char g_a75f70[];     // 0xa75f70 0x2b item modifiers of 0x28
extern char g_a76628[];     // 0xa76628
extern char g_a76630[];     // 0xa76630 the terrain
extern char g_8ed4df8[];    // 0x8ed4df8 11 ground types of 0x20
extern char g_8ed4f58[];    // 0x8ed4f58 the meshes
extern char g_8ed4f78[];    // 0x8ed4f78
extern char g_8ed5090[];    // 0x8ed5090 the particle systems
extern char g_8ed50c8[];    // 0x8ed50c8 the dialogs
extern char g_8ee3120[];    // 0x8ee3120 0x2a records of 0x44 (three strings each, at -0x20, -0x10, 0)
extern char g_8ee3c28[];    // 0x8ee3c28 the module path
extern char g_8ee3c38[];    // 0x8ee3c38 the module name
extern char g_8ee3c48[];    // 0x8ee3c48 the application
extern char g_5aa128[];     // 0x5aa128 the sound manager
extern std::string g_5aa380;   // 0x5aa380
extern char g_59c490[];     // 0x59c490 (its 0x40-character initial value)
extern void* g_vt_54a7d0[]; // 0x54a7d0 the debug text log's vtable
extern void* g_vt_595990[]; // 0x595990 outfile's vtable
extern void* g_vt_595618[]; // 0x595618 outfile_b's vtable (its base)
extern FILE* g_5a9f90;      // 0x5a9f90 the log's FILE

// ---- callees --------------------------------------------------------------------------------------------------
struct tstring { tstring(); ~tstring(); };          // 0x42bbd0, 0x42bc40
struct resmgr { resmgr(); ~resmgr(); };             // 0x41b000, 0x419e90
struct gui_manager { gui_manager(); ~gui_manager(); };  // 0x42d5c0, 0x42d3e0
struct LangTables { LangTables(); void release_strings(); };   // 0x4cda20, 0x4ca720
struct GuiScreen { GuiScreen(); };                  // 0x4ebea0
struct big_holder { void release(); };              // 0x4ca830
struct DisplayState { DisplayState(); };            // 0x4bc2d0
struct PhysSet { void reset(); };                   // 0x47dc50
struct ini_file { ini_file(); };                    // 0x4c1be0
namespace rgl { struct scene { scene(); ~scene(); }; }   // 0x429740, 0x41f4e0
struct model_a { model_a(); };                      // 0x4d3580
struct big_obj { void release(); };                 // 0x4cb9e0
struct MetaMission { void init(); };                // 0x4be742 (entered through the marker at 0x4be730)
struct music_player { music_player(); };            // 0x4c24c0
struct rec_table { rec_table(); };                  // 0x4be520
struct Terrain { Terrain(); };                      // 0x4ce720
struct GameApp { GameApp(); };                      // 0x4d3ec0
struct Tail2 { ~Tail2(); };                         // 0x4c4d10
struct RefList { void retire(); };                  // 0x4c6900
struct TwoVec { ~TwoVec(); };                       // 0x4da9c0
struct TriggerEntryVec { void dtor(); };            // 0x4c2200
struct ItemPage { void clear(); };                  // 0x4c6aa0
struct world_obj { ~world_obj(); };                 // 0x4c4ee0
struct BigArray { void destroy(); };                // 0x4eb810
struct loaded_psys_table { void clear(); };         // 0x4c1f20
struct scenefile_obj { ~scenefile_obj(); };         // 0x4c4090
struct RecK { void* vec_del(unsigned flags); };     // 0x4318c0 (the vector deleting destructor)
struct tstring_pair;
struct elem48;
struct Core;
struct NameVecRec;
void __fastcall fn_4c2ea0(void* keys);              // 0x4c2ea0
void* __fastcall conv_zero_node(void* p);           // 0x4c8c20
void* __fastcall dialog_mgr_ctor(void* p);          // 0x4c2470
void __fastcall destroy_tstring_pairs(tstring_pair** p);   // 0x4c4640
void __fastcall destroy_elem48_array(elem48** p);   // 0x4c3f60
void __fastcall core_dtor(Core* c);                 // 0x4bed90
void destroy_range(NameVecRec* first, NameVecRec* last);   // 0x4f6680
void destroy_range_e56(void* first, void* last, void* alloc, int tag);   // 0x43e6e0 (std::_Destroy_range<E56>)
void __fastcall nameholder_dtor(void* p);           // 0x4316b0 (NameHolder::~NameHolder)
void __stdcall vector_destructor_iterator(void* a, unsigned size, int n, void (__fastcall* dtor)(void*));   // 0x401830

#define AT(type, p, off) (*(type*)((char*)(p) + (off)))

// ---- destructors (atexit) -------------------------------------------------------------------------------------
// @ 0x548130
void __cdecl static_dtor_548130() { ((resmgr*)g_5d6fa0)->~resmgr(); }
// @ 0x548140
void __cdecl static_dtor_548140() { ((gui_manager*)g_5d7080)->~gui_manager(); }
// @ 0x548150
void __cdecl static_dtor_548150() { ((tstring*)g_8ee3c28)->~tstring(); }
// @ 0x548160
void __cdecl static_dtor_548160() { ((tstring*)g_8ee3c38)->~tstring(); }
// @ 0x548170
void __cdecl static_dtor_548170()
{
    ((tstring*)(g_5a9f30 + 0x20))->~tstring();
    ((tstring*)(g_5a9f30 + 0x4))->~tstring();
}
// @ 0x548190
void __cdecl static_dtor_548190() { ((tstring*)g_a180f8)->~tstring(); }
// @ 0x5481a0
void __cdecl static_dtor_5481a0()
{
    char* p = g_a44238;
    for (int n = 0x1000; n; n--) {
        p -= 0x2c;
        ((tstring*)(p + 0x1c))->~tstring();
        ((tstring*)p)->~tstring();
    }
}
// @ 0x5481d0
void __cdecl static_dtor_5481d0()
{
    char* p = g_a70240;
    for (int n = 0x1000; n; n--) {
        p -= 0x2c;
        ((tstring*)(p + 0x1c))->~tstring();
        ((tstring*)p)->~tstring();
    }
}
// @ 0x548200
void __cdecl static_dtor_548200() {}
// @ 0x548210
void __cdecl static_dtor_548210() {}
// @ 0x548220
void __cdecl static_dtor_548220()
{
    char* p = g_a76628;
    for (int n = 0x2b; n; n--) {
        p -= 0x28;
        ((tstring*)(p + 0x10))->~tstring();
        ((tstring*)p)->~tstring();
    }
}
// @ 0x548250
void __cdecl static_dtor_548250()
{
    char* p = g_8ed4f58;
    for (int n = 0xb; n; n--) {
        p -= 0x20;
        ((tstring*)p)->~tstring();
    }
}
// @ 0x548270
void __cdecl static_dtor_548270()
{
    char* p = g_8ee3c38;
    for (int n = 0x2a; n; n--) {
        p -= 0x44;
        ((tstring*)(p + 0x10))->~tstring();
        ((tstring*)p)->~tstring();
        ((tstring*)(p - 0x10))->~tstring();
    }
}
// @ 0x5482a0
void __cdecl static_dtor_5482a0() { core_dtor((Core*)g_8ee3c48); }
// @ 0x5482b0   the log stream's destructor: back to the base vtable, close the file if it is open, then the strings
void __cdecl static_dtor_5482b0()
{
    AT(void*, g_5a9f60, 0) = g_vt_595618;
    if (g_5a9f60[0x35]) {
        typedef int (__fastcall* close_fn)(void* self);   // a thiscall slot with no arguments
        if (((close_fn)g_vt_595618[0x1a])(g_5a9f60) == 0)
            fclose(g_5a9f90);
        g_5a9f60[0x34] = 1;
        g_5a9f60[0x35] = 0;
    }
    ((tstring*)(g_5a9f60 + 0x20))->~tstring();
    ((tstring*)(g_5a9f60 + 0x4))->~tstring();
}
// @ 0x548310
void __cdecl static_dtor_548310()
{
    ((tstring*)g_a75e8c)->~tstring();
    char* p = g_a75a28;
    for (int n = 0x100; n; n--) {
        p -= 0x20;
        ((tstring*)p)->~tstring();
    }
}
// @ 0x548340
void __cdecl static_dtor_548340() { ((TriggerEntryVec*)g_a76628)->dtor(); }
// @ 0x548350
void __cdecl static_dtor_548350() { ((loaded_psys_table*)g_8ed5090)->clear(); }
// @ 0x548360   11 key records of 0x20 below 0xa7374c: an owned NameHolder array (delete[]) and a string each
void __cdecl static_dtor_548360()
{
    char* e = g_a7374c;
    for (int n = 0xb; n; n--) {
        int used = AT(int, e, -0x24);
        e -= 0x20;
        if (used) {
            char* a = AT(char*, e, 0);
            if (a) {
                vector_destructor_iterator(a, 0x18, AT(int, a, -4), nameholder_dtor);
                operator delete[](a - 4);
            }
        }
        ((tstring*)(e - 0x1c))->~tstring();
    }
}
// @ 0x5483b0
void __cdecl static_dtor_5483b0() { destroy_tstring_pairs((tstring_pair**)g_a75ea0); }
// @ 0x5483c0
void __cdecl static_dtor_5483c0()
{
    RecK* r = AT(RecK*, g_a75eb0, 0);
    if (r) {
        r->vec_del(3);
        AT(RecK*, g_a75eb0, 0) = 0;
    }
}
// @ 0x5483e0
void __cdecl static_dtor_5483e0() { destroy_elem48_array((elem48**)g_8ed4f58); }
// @ 0x5483f0
void __cdecl static_dtor_5483f0() { ((scenefile_obj*)g_8ed50c8)->~scenefile_obj(); }
// @ 0x548400
void __cdecl static_dtor_548400() { ((RefList*)g_a75f2c)->retire(); }
// @ 0x548410
void __cdecl static_dtor_548410() { ((ItemPage*)g_5aa128)->clear(); }
// @ 0x548420
void __cdecl static_dtor_548420() { ((world_obj*)g_a76630)->~world_obj(); }
// @ 0x548430
void __cdecl static_dtor_548430() { g_5aa380.~basic_string(); }
// @ 0x548460
void __cdecl static_dtor_548460() { ((Tail2*)g_a739f8)->~Tail2(); }
// @ 0x548470
void __cdecl static_dtor_548470() { ((LangTables*)g_5e2a18)->release_strings(); }
// @ 0x548480
void __cdecl static_dtor_548480() { ((big_holder*)g_5fe898)->release(); }
// @ 0x548490
void __cdecl static_dtor_548490() { ((rgl::scene*)g_a70260)->~scene(); }
// @ 0x5484a0
void __cdecl static_dtor_5484a0() { ((big_obj*)g_a70440)->release(); }
// @ 0x5484b0   std::vector<E56>::~vector of the physics set (allocator at +0, first/last/end at +4/+8/+0xc)
void __cdecl static_dtor_5484b0()
{
    void* first = AT(void*, g_a181f8, 4);
    if (first) {
        destroy_range_e56(first, AT(void*, g_a181f8, 8), g_a181f8, 0);
        operator delete(AT(void*, g_a181f8, 4));
    }
    AT(void*, g_a181f8, 4) = 0;
    AT(void*, g_a181f8, 8) = 0;
    AT(void*, g_a181f8, 0xc) = 0;
}
// @ 0x548500
void __cdecl static_dtor_548500() { ((TwoVec*)g_a75f40)->~TwoVec(); }
// @ 0x548510
void __cdecl static_dtor_548510() { ((BigArray*)g_8ed4f78)->destroy(); }
// @ 0x548520   std::vector<NameVecRec>::~vector of the skeleton names
void __cdecl static_dtor_548520()
{
    NameVecRec* first = AT(NameVecRec*, g_a180e8, 4);
    if (first) {
        destroy_range(first, AT(NameVecRec*, g_a180e8, 8));
        operator delete(AT(void*, g_a180e8, 4));
    }
    AT(void*, g_a180e8, 4) = 0;
    AT(void*, g_a180e8, 8) = 0;
    AT(void*, g_a180e8, 0xc) = 0;
}

// ---- initializers, in the original table order -----------------------------------------------------------------
// @ 0x5479a0
void __cdecl static_init_5479a0() { new (g_5d6fa0) resmgr; atexit(static_dtor_548130); }
// @ 0x5479c0
void __cdecl static_init_5479c0() { new (g_5d7080) gui_manager; atexit(static_dtor_548140); }
// @ 0x5479e0
void __cdecl static_init_5479e0() { g_5e2464 = g_5a8e78; }
// @ 0x5479f0
void __cdecl static_init_5479f0() { g_5e2468 = g_5a8e88; }
// @ 0x547a00
void __cdecl static_init_547a00() { g_5a9be8 = g_549cdc - g_5a9be0; }
// @ 0x547a20
void __cdecl static_init_547a20() { g_5e2584 = g_5a8e78; }
// @ 0x547a30
void __cdecl static_init_547a30() { g_5e2580 = g_5a8e88; }
// @ 0x547a40
void __cdecl static_init_547a40() { g_8ee41ec = g_5a8e78; }
// @ 0x547a50
void __cdecl static_init_547a50() { g_8ee41e8 = g_5a8e88; }
// @ 0x547a60
void __cdecl static_init_547a60() { g_5a9f28 = g_549cdc - g_5a9f20; }
// @ 0x547a80
void __cdecl static_init_547a80() { new (g_5e2a18) LangTables; atexit(static_dtor_548470); }
// @ 0x547aa0   the debug text log: its two strings, flags, and vtable
void __cdecl static_init_547aa0()
{
    new (g_5a9f30 + 0x4) tstring;
    new (g_5a9f30 + 0x20) tstring;
    g_5a9f30[0x18] = 0;
    AT(int, g_5a9f30, 0x1c) = 0;
    AT(void*, g_5a9f30, 0) = g_vt_54a7d0;
    AT(int, g_5a9f30, 0x14) = 0;
    atexit(static_dtor_548170);
}
// @ 0x547ae0   the log file stream (outfile): two strings, flags, vtable
void __cdecl static_init_547ae0()
{
    new (g_5a9f60 + 0x4) tstring;
    new (g_5a9f60 + 0x20) tstring;
    g_5a9f60[0x18] = 0;
    AT(int, g_5a9f60, 0x1c) = 0;
    g_5a9f60[0x34] = 1;
    g_5a9f60[0x35] = 0;
    AT(int, g_5a9f60, 0x14) = 0;
    AT(void*, g_5a9f60, 0) = g_vt_595990;
    atexit(static_dtor_5482b0);
}
// @ 0x547b30
void __cdecl static_init_547b30() { new (g_5fe898) GuiScreen; atexit(static_dtor_548480); }
// @ 0x547b50   input state: both tick snapshots, the key table, the axes
void __cdecl static_init_547b50()
{
    int lo = AT(int, g_5e2948, 0), hi = AT(int, g_5e2948, 4);
    AT(int, g_a178f0, 0) = lo;
    AT(int, g_a178f0, 4) = hi;
    AT(int, g_a17900, 0) = lo;
    AT(int, g_a17900, 4) = hi;
    memset(g_a170d4, 0, 0x400);
    AT(int, g_a178e0, 0) = 0;
    AT(int, g_a178e0, 4) = 0;
    AT(int, g_a178d4, 0) = 0;
    AT(int, g_a178d4, 4) = 0;
}
// @ 0x547ba0
void __cdecl static_init_547ba0() { new (g_a17910) DisplayState; }
// @ 0x547bb0
void __cdecl static_init_547bb0() { atexit(static_dtor_548520); }
// @ 0x547bc0   module.ini settings: the defaults before module.ini is read
void __cdecl static_init_547bc0()
{
    char* s = g_a180f8;                          // settings block, 0xa180f8..0xa181f4
    new (s) tstring;
    AT(int, s, 0xd8) = 10000;  AT(int, s, 0xdc) = 10000;  AT(int, s, 0xe4) = 10000;  AT(int, s, 0xec) = 10000;
    AT(int, s, 0xf0) = 10000;  AT(int, s, 0xf4) = 10000;  AT(int, s, 0xf8) = 10000;  AT(int, s, 0xfc) = 10000;
    AT(int, s, 0xe0) = 200000;
    AT(int, s, 0xe8) = 0x5399;
    AT(int, s, 0xc4) = 0;  AT(int, s, 0xc8) = 0;  AT(int, s, 0xcc) = 0;   // main menu font colour
    AT(int, s, 0xd0) = 0;                        // works_with_version_min
    AT(int, s, 0xd4) = 1000000;                  // works_with_version_max
    AT(int, s, 0x10) = 0;  AT(int, s, 0x14) = 0; // scan_module_textures / sounds
    AT(float, s, 0x18) = 0.15f;                  // time_multiplier
    AT(float, s, 0x1c) = 6.5f;                   // seeing_range
    AT(float, s, 0x20) = 0.8f;                   // track_spotting
    AT(float, s, 0x24) = 1.0f;                   // blood_multiplier
    AT(int, s, 0x28) = 5;  AT(int, s, 0x2c) = 5;  AT(int, s, 0x30) = 5;   // leadership / prisoner bonus, companions
    AT(float, s, 0x34) = 1.0f;  AT(float, s, 0x38) = 1.0f;  AT(float, s, 0x3c) = 1.0f;   // xp multipliers
    AT(float, s, 0x40) = 15.0f;                  // hero_wounded_treshold
    AT(float, s, 0x44) = 5.0f;                   // player_wounded_treshold
    for (int o = 0x48; o <= 0x70; o += 4)        // hints, sea/river direction and speeds, tree types
        AT(int, s, o) = 0;
    AT(int, s, 0x74) = 1200;  AT(int, s, 0x78) = 2;  AT(int, s, 0x7c) = 12;   // starting date
    AT(int, s, 0x80) = 0;                        // damage_interrupt_attack_treshold
    AT(float, s, 0x84) = 1.0f;  AT(float, s, 0x88) = 0.5f;  AT(float, s, 0x8c) = 0.5f;   // armor soak
    AT(int, s, 0x90) = 0;  AT(int, s, 0x94) = 0;  AT(int, s, 0x98) = 0;                  // armor reduction
    AT(float, s, 0x9c) = 1.0f;  AT(float, s, 0xa0) = 1.0f;  AT(float, s, 0xa4) = 1.0f;  AT(float, s, 0xa8) = 1.0f;
    for (int o = 0xac; o <= 0xbc; o += 4)        // quick battles, food slot, hair colours, faction colour, notes
        AT(int, s, o) = 0;
    s[0xc0] = 0;                                 // show_party_ids_instead_of_names
    atexit(static_dtor_548190);
}
// @ 0x547d80
void __cdecl static_init_547d80() { ((PhysSet*)g_a181f8)->reset(); atexit(static_dtor_5484b0); }
// @ 0x547da0
void __cdecl static_init_547da0() { new (g_a18238) ini_file; atexit(static_dtor_5481a0); }
// @ 0x547dc0
void __cdecl static_init_547dc0() { new (g_a44240) ini_file; atexit(static_dtor_5481d0); }
// @ 0x547de0
void __cdecl static_init_547de0()
{
    new (g_a70260) rgl::scene;
    AT(int, g_a70248, 0) = 0;  AT(int, g_a70248, 4) = 0;  AT(int, g_a70248, 8) = 0;  AT(int, g_a70248, 0xc) = 0;
    AT(int, g_a70438, 0) = 0;
    atexit(static_dtor_548490);
}
// @ 0x547e20
void __cdecl static_init_547e20() { new (g_a70440) model_a; atexit(static_dtor_5484a0); }
// @ 0x547e40
void __cdecl static_init_547e40() { ((MetaMission*)g_a73300)->init(); atexit(static_dtor_548200); }
// @ 0x547e60
void __cdecl static_init_547e60()
{
    char* p = g_a735d0;
    for (int n = 0xb; n; n--, p += 0x20)
        new (p) tstring;
    atexit(static_dtor_548360);
}
// @ 0x547e90
void __cdecl static_init_547e90()
{
    memset(g_a73730, 0xff, 0x4a * 4);
    fn_4c2ea0(g_a73730);
    atexit(static_dtor_548210);
}
// @ 0x547ec0
void __cdecl static_init_547ec0() { conv_zero_node(g_a73980); atexit(static_dtor_548460); }
// @ 0x547ee0
void __cdecl static_init_547ee0() { new (g_a73a10) music_player; atexit(static_dtor_548310); }
// @ 0x547f00
void __cdecl static_init_547f00() { atexit(static_dtor_5483b0); }
// @ 0x547f10
void __cdecl static_init_547f10() { new (g_a75eb0) rec_table; atexit(static_dtor_5483c0); }
// @ 0x547f30
void __cdecl static_init_547f30() { new (g_a75f2c) rec_table; atexit(static_dtor_548400); }
// @ 0x547f50
void __cdecl static_init_547f50() { atexit(static_dtor_548500); }
// @ 0x547f60
void __cdecl static_init_547f60()
{
    char* p = g_a75f70;
    for (int n = 0x2b; n; n--, p += 0x28) {
        new (p) tstring;
        new (p + 0x10) tstring;
    }
    atexit(static_dtor_548220);
}
// @ 0x547fa0
void __cdecl static_init_547fa0() { atexit(static_dtor_548340); }
// @ 0x547fb0
void __cdecl static_init_547fb0() { atexit(static_dtor_548410); }
// @ 0x547fc0
void __cdecl static_init_547fc0() { new (g_a76630) Terrain; atexit(static_dtor_548420); }
// @ 0x547fe0
void __cdecl static_init_547fe0()
{
    char* p = g_8ed4df8;
    for (int n = 0xb; n; n--, p += 0x20)
        new (p) tstring;
    atexit(static_dtor_548250);
}
// @ 0x548010
void __cdecl static_init_548010() { new (g_8ed4f58) rec_table; atexit(static_dtor_5483e0); }
// @ 0x548030
void __cdecl static_init_548030() { atexit(static_dtor_548510); }
// @ 0x548040
void __cdecl static_init_548040() { atexit(static_dtor_548350); }
// @ 0x548050
void __cdecl static_init_548050() { dialog_mgr_ctor(g_8ed50c8); atexit(static_dtor_5483f0); }
// @ 0x548070
void __cdecl static_init_548070()
{
    char* p = g_8ee3120;
    for (int n = 0x2a; n; n--, p += 0x44) {
        new (p - 0x20) tstring;
        new (p - 0x10) tstring;
        new (p) tstring;
    }
    atexit(static_dtor_548270);
}
// @ 0x5480b0
void __cdecl static_init_5480b0() { new (g_8ee3c28) tstring; atexit(static_dtor_548150); }
// @ 0x5480d0
void __cdecl static_init_5480d0() { new (g_8ee3c38) tstring; atexit(static_dtor_548160); }
// @ 0x5480f0
void __cdecl static_init_5480f0() { g_5aa380.assign(g_59c490, 0x40); atexit(static_dtor_548430); }
// @ 0x548110
void __cdecl static_init_548110() { new (g_8ee3c48) GameApp; atexit(static_dtor_5482a0); }

// The table, as __xc_a..__xc_z held it (0x5a8008..0x5a80bc); the CRT runs .CRT$XCU after its own initializers.
typedef void (__cdecl* init_fn)();
#pragma data_seg(".CRT$XCU")
init_fn g_static_init_table[] = {
    static_init_5479a0, static_init_5479c0, static_init_5479e0, static_init_5479f0, static_init_547a00,
    static_init_547a20, static_init_547a30, static_init_547a40, static_init_547a50, static_init_547a60,
    static_init_547a80, static_init_547aa0, static_init_547ae0, static_init_547b30, static_init_547b50,
    static_init_547ba0, static_init_547bb0, static_init_547bc0, static_init_547d80, static_init_547da0,
    static_init_547dc0, static_init_547de0, static_init_547e20, static_init_547e40, static_init_547e60,
    static_init_547e90, static_init_547ec0, static_init_547ee0, static_init_547f00, static_init_547f10,
    static_init_547f30, static_init_547f50, static_init_547f60, static_init_547fa0, static_init_547fb0,
    static_init_547fc0, static_init_547fe0, static_init_548010, static_init_548030, static_init_548040,
    static_init_548050, static_init_548070, static_init_5480b0, static_init_5480d0, static_init_5480f0,
    static_init_548110,
};
#pragma data_seg()
