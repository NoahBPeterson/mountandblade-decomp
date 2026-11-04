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
#include "../slot.h"
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx, g_mission_ctx (0xa73300)
#include "../music_player.h"   // music_player, g_music (0xa73a10)
#include "../../include/ptr_tag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "../../include/clock_table.h"
#include "../animator.h"       // Animator g_anim (0x8ed4f60)
#include "../float_consts.h"

// ---- storage (labels in the original data, by address) ----------------------------------------------------------
extern char g_5d6fa0[];     // 0x5d6fa0 the resource manager
extern char g_5d7080[];     // 0x5d7080 the gui manager


extern unsigned g_5e2464;   // 0x5e2464
extern unsigned g_5e2468;   // 0x5e2468
extern float g_5a9be0;      // 0x5a9be0
extern float g_5a9be8;      // 0x5a9be8
extern unsigned g_5e2584;   // 0x5e2584
extern unsigned g_5e2580;   // 0x5e2580
extern unsigned g_8ee41ec;  // 0x8ee41ec
extern unsigned g_8ee41e8;  // 0x8ee41e8
extern float g_5a9f20;      // 0x5a9f20
extern float g_5a9f28[2];   // 0x5a9f28 (defined in cls_map.cpp: {x, y})
extern char g_5a9f30[];     // 0x5a9f30 the debug text log (vtable, two strings)
extern char g_5a9f60[];     // 0x5a9f60 the log file stream (vtable, two strings, flags, FILE*)





extern char g_a181f8[];     // 0xa181f8 the physics set (vector of E56)

extern char g_a73730[];     // 0xa73730 the game keys (0x4a dwords)
extern char g_a7374c[];     // 0xa7374c (the key records' end)
extern char g_a73980[];     // 0xa73980 the main menu
extern char g_a739f8[];     // 0xa739f8
extern char g_a75ea0[];     // 0xa75ea0 the game strings
extern char g_a75eb0[];     // 0xa75eb0 the scripts
extern char g_a75f2c[];     // 0xa75f2c the presentations
extern char g_a75f40[];     // 0xa75f40 the tableaus
extern char g_a76628[];     // 0xa76628
extern char g_8ed4df8[];    // 0x8ed4df8 11 ground types of 0x20
extern char g_8ed4f58[];    // 0x8ed4f58 the meshes
extern char g_8ed5090[];    // 0x8ed5090 the particle systems

extern char g_8ee3c48[];    // 0x8ee3c48 the application
struct SoundMgr;
extern SoundMgr g_sound_mgr;   // 0x5aa128 the sound player (defined in agent_fx_sound.cpp; no constructor, only the destructor registered below)
extern void* g_vt_54a7d0[]; // 0x54a7d0 the debug text log's vtable
extern void* g_vt_595990[]; // 0x595990 outfile's vtable
extern void* g_vt_595618[]; // 0x595618 log_stream's vtable (its base)
extern FILE* g_5a9f90;      // 0x5a9f90 the log's FILE

// ---- callees --------------------------------------------------------------------------------------------------
struct TStrRec { TStrRec(); ~TStrRec(); char rec[0x20]; };   // 0x42bbd0, 0x42bc40; sized as the 0x20-byte table record the string heads
struct tstring11 { TStrRec a[11]; };            // a table of 11 such records, constructed the way the compiler does a global array
struct resmgr { resmgr(); ~resmgr(); };             // 0x41b000, 0x419e90
struct gui_manager { gui_manager(); ~gui_manager(); };  // 0x42d5c0, 0x42d3e0
struct ini_file { ini_file(); };                    // 0x4c1be0
struct rec_table { rec_table(); };                  // 0x4be520
struct GameApp { GameApp(); };                      // 0x4d3ec0
struct Tail2 { ~Tail2(); };                         // 0x4c4d10
struct RefList { void retire(); };                  // 0x4c6900
struct TwoVec { ~TwoVec(); };                       // 0x4da9c0
struct TriggerEntryVec { void dtor(); };            // 0x4c2200
struct ItemPage { void clear(); };                  // 0x4c6aa0
struct BigArray { void destroy(); };                // 0x4eb810
struct loaded_psys_table { void clear(); };         // 0x4c1f20
struct RecK { void* vec_del(unsigned flags); };     // 0x4318c0 (the vector deleting destructor)
struct tstring_pair;
struct elem48;
struct Core;
void __fastcall fn_4c2ea0(void* keys);              // 0x4c2ea0
void* __fastcall conv_zero_node(void* p);           // 0x4c8c20
void __fastcall destroy_tstring_pairs(tstring_pair** p);   // 0x4c4640
void __fastcall destroy_elem48_array(elem48** p);   // 0x4c3f60
void __fastcall core_dtor(Core* c);                 // 0x4bed90
void __fastcall nameholder_dtor(void* p);           // 0x4316b0 (NameHolder::~NameHolder)
void __stdcall vector_destructor_iterator(void* a, unsigned size, int n, void (__fastcall* dtor)(void*));   // 0x401830

#define AT(type, p, off) (*(type*)((char*)(p) + (off)))

// ---- destructors (atexit) -------------------------------------------------------------------------------------
// @ 0x548130
void __cdecl static_dtor_548130() { ((resmgr*)g_5d6fa0)->~resmgr(); }
// @ 0x548140
void __cdecl static_dtor_548140() { ((gui_manager*)g_5d7080)->~gui_manager(); }
// @ 0x548170
void __cdecl static_dtor_548170()
{
    ((TStrRec*)(g_5a9f30 + 0x20))->~TStrRec();
    ((TStrRec*)(g_5a9f30 + 0x4))->~TStrRec();
}
// @ 0x548200
void __cdecl static_dtor_548200() {}
// @ 0x548210
void __cdecl static_dtor_548210() {}
// @ 0x548250
void __cdecl static_dtor_548250()
{
    char* p = g_8ed4f58;
    for (int n = 0xb; n; n--) {
        p -= 0x20;
        ((TStrRec*)p)->~TStrRec();
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
    ((TStrRec*)(g_5a9f60 + 0x20))->~TStrRec();
    ((TStrRec*)(g_5a9f60 + 0x4))->~TStrRec();
}
// @ 0x548310
void __cdecl static_dtor_548310()
{
    ((TStrRec*)&g_music.root)->~TStrRec();
    char* p = (char*)&g_music.start[4];   // the end of the 0x100 track name strings (+0x2018)
    for (int n = 0x100; n; n--) {
        p -= 0x20;
        ((TStrRec*)p)->~TStrRec();
    }
}
// @ 0x548340
void __cdecl static_dtor_548340() { ((TriggerEntryVec*)g_a76628)->dtor(); }
// @ 0x548350
void __cdecl static_dtor_548350() { ((loaded_psys_table*)g_8ed5090)->clear(); }
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
// @ 0x548400
void __cdecl static_dtor_548400() { ((RefList*)g_a75f2c)->retire(); }
// @ 0x548410
void __cdecl static_dtor_548410() { ((ItemPage*)&g_sound_mgr)->clear(); }
// @ 0x548460
void __cdecl static_dtor_548460() { ((Tail2*)g_a739f8)->~Tail2(); }
// @ 0x548500
void __cdecl static_dtor_548500() { ((TwoVec*)g_a75f40)->~TwoVec(); }
// @ 0x548510
void __cdecl static_dtor_548510() { ((BigArray*)&g_anim.skins_alloc)->destroy(); }   // the skin vector at 0x8ed4f78

// ---- initializers, in the original table order -----------------------------------------------------------------
// @ 0x5479a0
void __cdecl static_init_5479a0() { new (g_5d6fa0) resmgr; atexit(static_dtor_548130); }
// @ 0x5479c0
void __cdecl static_init_5479c0() { new (g_5d7080) gui_manager; atexit(static_dtor_548140); }
// @ 0x547aa0   the debug text log: its two strings, flags, and vtable
void __cdecl static_init_547aa0()
{
    new (g_5a9f30 + 0x4) TStrRec;
    new (g_5a9f30 + 0x20) TStrRec;
    g_5a9f30[0x18] = 0;
    AT(int, g_5a9f30, 0x1c) = 0;
    AT(void*, g_5a9f30, 0) = g_vt_54a7d0;
    AT(int, g_5a9f30, 0x14) = 0;
    atexit(static_dtor_548170);
}
// @ 0x547ae0   the log file stream (outfile): two strings, flags, vtable
void __cdecl static_init_547ae0()
{
    new (g_5a9f60 + 0x4) TStrRec;
    new (g_5a9f60 + 0x20) TStrRec;
    g_5a9f60[0x18] = 0;
    AT(int, g_5a9f60, 0x1c) = 0;
    g_5a9f60[0x34] = 1;
    g_5a9f60[0x35] = 0;
    AT(int, g_5a9f60, 0x14) = 0;
    AT(void*, g_5a9f60, 0) = g_vt_595990;
    atexit(static_dtor_5482b0);
}
// @ 0x547b50   input state: both tick snapshots, the key table, the axes
void __cdecl static_init_547b50()
{
    __int64 now = g_ticks[0];
    g_keyboard.t828 = now;
    g_keyboard.t838 = now;
    memset(g_keyboard.down, 0, 0x400);
    g_keyboard.f818 = 0;
    g_keyboard.f81c = 0;
    g_keyboard.axis[0] = 0;
    g_keyboard.axis[1] = 0;
}
// @ 0x547e40
void __cdecl static_init_547e40() { g_mission_ctx.init(); atexit(static_dtor_548200); }
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
void __cdecl static_init_547ee0() { new (&g_music) music_player; atexit(static_dtor_548310); }
// @ 0x547f00
void __cdecl static_init_547f00() { atexit(static_dtor_5483b0); }
// @ 0x547f10
void __cdecl static_init_547f10() { new (g_a75eb0) rec_table; atexit(static_dtor_5483c0); }
// @ 0x547f30
void __cdecl static_init_547f30() { new (g_a75f2c) rec_table; atexit(static_dtor_548400); }
// @ 0x547f50
void __cdecl static_init_547f50() { atexit(static_dtor_548500); }
// @ 0x547fa0
void __cdecl static_init_547fa0() { atexit(static_dtor_548340); }
// @ 0x547fb0
void __cdecl static_init_547fb0() { atexit(static_dtor_548410); }
// @ 0x547fe0
void __cdecl static_init_547fe0()
{
    new (g_8ed4df8) tstring11;
    atexit(static_dtor_548250);
}
// @ 0x548010
void __cdecl static_init_548010() { new (g_8ed4f58) rec_table; atexit(static_dtor_5483e0); }
// @ 0x548030
void __cdecl static_init_548030() { atexit(static_dtor_548510); }
// @ 0x548040
void __cdecl static_init_548040() { atexit(static_dtor_548350); }
// @ 0x548110
void __cdecl static_init_548110() { new (g_8ee3c48) GameApp; atexit(static_dtor_5482a0); }

// The table, as __xc_a..__xc_z held it (0x5a8008..0x5a80bc); the CRT runs .CRT$XCU after its own initializers.
typedef void (__cdecl* init_fn)();
#pragma data_seg(".CRT$XCU")
init_fn g_static_init_table[] = {
    static_init_5479a0, static_init_5479c0, 
    
    static_init_547aa0, static_init_547ae0, static_init_547b50,
    static_init_547e40,
    static_init_547e90, static_init_547ec0, static_init_547ee0, static_init_547f00, static_init_547f10,
    static_init_547f30, static_init_547f50, static_init_547fa0, static_init_547fb0,
    static_init_547fe0, static_init_548010, static_init_548030, static_init_548040,
    static_init_548110,
};
#pragma data_seg()
