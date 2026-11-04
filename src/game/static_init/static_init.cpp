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
#include "../../include/ptr_tag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
#include "../../include/clock_table.h"
#include "../../include/rec_dtors.h"   // RecK (vector deleting destructor 0x4318c0)
#include "../animator.h"       // Animator g_anim (0x8ed4f60)
#include "../float_consts.h"

// ---- storage (labels in the original data, by address) ----------------------------------------------------------


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





extern char g_a181f8[];     // 0xa181f8 the physics set (vector of E56)

extern char g_8ed4df8[];    // 0x8ed4df8 11 ground types of 0x20
#include "../../include/mesh_table.h"   // MeshTable g_meshes (0x8ed4f58)

struct SoundMgr;
extern SoundMgr g_sound_mgr;   // 0x5aa128 the sound player (defined in agent_fx_sound.cpp; no constructor, only the destructor registered below)

// ---- callees --------------------------------------------------------------------------------------------------
#include "../tstring.h"                       // tstring::tstring 0x42bbd0, ~tstring 0x42bc40
struct TStrRec : tstring { char rest[0x10]; };   // the 0x20-byte table record the string heads
struct tstring11 { TStrRec a[11]; };            // a table of 11 such records, constructed the way the compiler does a global array
struct ini_file { ini_file(); };                    // 0x4c1be0
struct ItemPage { void clear(); };                  // 0x4c6aa0
struct BigArray { void destroy(); };                // 0x4eb810
void __fastcall nameholder_dtor(void* p);           // 0x4316b0 (NameHolder::~NameHolder)
void __stdcall vector_destructor_iterator(void* a, unsigned size, int n, void (__fastcall* dtor)(void*));   // 0x401830

#define AT(type, p, off) (*(type*)((char*)(p) + (off)))

// ---- destructors (atexit) -------------------------------------------------------------------------------------
// @ 0x548510
void __cdecl static_dtor_548510() { ((BigArray*)&g_anim.skins_alloc)->destroy(); }   // the skin vector at 0x8ed4f78

// ---- initializers, in the original table order -----------------------------------------------------------------
// @ 0x548030
void __cdecl static_init_548030() { atexit(static_dtor_548510); }

// The table, as __xc_a..__xc_z held it (0x5a8008..0x5a80bc); the CRT runs .CRT$XCU after its own initializers.
typedef void (__cdecl* init_fn)();
#pragma data_seg(".CRT$XCU")
init_fn g_static_init_table[] = {
    static_init_548030,
};
#pragma data_seg()
