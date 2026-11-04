// trigger_vec.h: classes and externs of trigger_vec.cpp (0x4be742..0x4c2c50): the meta-mission and code tables, the
// vector copy helpers, the application object's option dialogs and device-object handlers, the module_info.txt
// parser, the trigger / game / mouse record vectors, the trigger-entry reader, the music monitor and the record
// loaders. ALL NAMES PROVISIONAL. Every offset cites the function whose disassembly proves it.
//
// Shared classes come from their canonical headers: tstring (../tstring.h), the streams and the engine log
// (../strat/gui_manager.h, ../../rgl/stream.h), the application object `Core` (../config/core.h: members added for
// 0x4bed90..0x4c1ad0, offsets unchanged), GameConfig / CreateGameOptionsPage / trigger_rec / trigger_entry /
// music_player / ini_file (../config/config.h), TriggerRec (../party/game.h: the full layout of the 0x78-byte
// record), RecH (../../rgl/xmlcfg/records.h: the 0x30-byte trigger entry), DialogMgr (../dialog_manager.h).
#ifndef GAME_TRIGGERS_TRIGGER_VEC_H
#define GAME_TRIGGERS_TRIGGER_VEC_H
#include <windows.h>
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "../tstring.h"
#include "../game_globals.h"      // canonical g_module_path
#include "../strat/gui_manager.h"
#include "../config/core.h"
#include "../config/config.h"
#include "../party/game.h"
#include "../../rgl/xmlcfg/records.h"
#define REC_TABLES_NO_GAME_VARS   // g_game_vars is declared below with the ini_file view
#include "../../rgl/mesh2/rec_tables.h"       // Alloc2c / Alloc204 (0x4c1b20 / 0x4c1b40), sound_engine
float clamp(const float* x, const float* lo, const float* hi);     // 0x40e3d0 (the definition's signature, rgl/shapes/shapes.h; rec_tables.h declares it by reference)
#include "../../rgl/resource.h"             // texture
#include "../shadow_map.h"
#include "../../include/ptr_tag.h"
#include "../shared_vars.h"
// The resource manager (object 0x5d6fa0) as rgl/core/resmgr.h's `resmgr` defines it; that header cannot be included next to
// rec_tables.h (g_log), so this is its declaration of the three members the module loader calls (resmgr.cpp 0x4195e0 / 0x419640 / 0x41e150).
// g_resmgr_full of resmgr.h is the same object as resource.h's g_resmgr.
struct resmgr {
    int invalidate();                                               // 0x4195e0 (device lost)
    void linearise_meshes();                                        // 0x419640
    void load_pack(const rgl::string& name, int user);              // 0x41e150
};
#define g_resmgr_full (*(resmgr*)&g_resmgr)
#include "../render_batch.h"

// ---- the options dialog's persistent values (0x4bf170 reads them, 0x4bee10 writes them back) -------------------------

// ---- meta mission (global object at 0xa73300, 0x4be742: called as MetaMission::init; the original brackets it with an
// 18-byte WinLicense marker pair at 0x4be730 / before 0x4be7c0) ---------------------------------------------------
#include "../inventory_panel/mission_ctx_rec.h"      // MissionCtx (0xa73300): init is the meta mission's (0x4be742)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)

// ---- script-code table (0x4a ints, every second one named: 0x4be7c0, also called as a bare function by rgl/mesh2) ---
struct code_table {
    int v[0x4a];
    void init();            // 0x4be7c0
};

// ---- vectors and copy helpers (0x4be950..0x4beb70) -----------------------------------------------------------------
// std::vector-shaped counted arrays (allocator word at +0, begin +4, end +8): element counts for 0x50 / 0x1f8 / 0xb0 byte elements
struct vec_e50  { int n; char* begin; char* end; int size() const; };   // 0x4be950
struct vec_e1f8 { int n; char* begin; char* end; int size() const; };   // 0x4be970
struct vec_eb0  { int n; char* begin; char* end; int size() const; };   // 0x4be9a0
struct clip_pair {          // list node, 0x28 bytes: two pointers and an eight-dword record (called from rgl/mesh2/gui_edit.cpp)
    void* a;                // +0x00
    void* b;                // +0x04
    int   d[8];             // +0x08
    clip_pair(void* a_, void* b_, const int* src);              // 0x4be9c0
};
struct blk2c   { int v[0x0b]; };    // 0x2c bytes
struct blk204  { int v[0x81]; };    // 0x204 bytes
blk2c*  copy_fwd_2c(blk2c* first, blk2c* last, blk2c* dst);                 // 0x4be9f0
void    fill_2c(blk2c* first, blk2c* last, const blk2c* val);               // 0x4bea20
blk2c*  copy_back_2c(blk2c* first, blk2c* last, blk2c* dst);                // 0x4bea50
blk204* copy_fwd_204(blk204* first, blk204* last, blk204* dst);             // 0x4bea80
void    fill_204(blk204* first, blk204* last, const blk204* val);           // 0x4beac0
blk204* copy_back_204(blk204* first, blk204* last, blk204* dst);            // 0x4beaf0
void    fill_int(int* first, int* last, const int* val);                    // 0x4beb30
int*    copy_back_int(int* first, int* last, int* dst);                     // 0x4beb50

// ---- registry / module folder helpers, application object (0x4beb80..0x4bf640) -----------------------------------
void __cdecl open_keys_key(PHKEY out);                                      // 0x4b96d0 (src/game/config/options_dlg.cpp; its header cannot be mixed with config.h)
void __cdecl read_key_string(HKEY* key, LPCSTR name, tstring* out);         // 0x4b9720
void keys_write_value(HKEY* key, LPCSTR name, const tstring* v);            // 0x4beb80
bool module_ini_exists(const char* module);                                 // 0x4bebc0
void scan_module_folders(tstring* out, int* count);                         // 0x4bec60
void __fastcall core_dtor(Core* self);                                      // 0x4bed90 (the destructor of the application object, vtable 0x5948e0)
void* __fastcall core_deleting_dtor(Core* self, int, unsigned flags);       // 0x4bedf0
extern void* g_dlg_options;     // 0x8ee3fc4 the options dialog window (untyped so the call reloads it into eax), set by options_dlg_proc
INT_PTR CALLBACK options_dlg_proc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x4bf5f0

// ---- the module folder dialog (0x4c03b0; 0x4c3340 jumps to it with ecx = g_app_obj, unused) ----------------------------
INT_PTR CALLBACK folder_dlg_proc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x4c03b0
void load_game_variables();                                                 // 0x4c0520
void parse_module_info();                                                   // 0x4c05b0

// ---- globals of the device-object handlers and the module_info.txt parser -------------------------------------------------
#include "../../include/renderer.h"            // g_render 0xa17910
#include "../../include/render_flags.h"
#include "../../include/d3d_caps.h"
#include "../../rgl/options.h"
struct options;
extern options g_options;                   // 0x5aa008 (rgl/resource_manager.h clashes with the log declarations here)
extern ini_file g_module_info;          // 0xa18238 module_info.txt table (count member)
extern ini_file g_game_vars;            // 0xa44240 game_variables.txt table
// g_resmgr_x: the view of g_resmgr (0x5d6fa0) declared in config.h
extern int g_compatible_module_version;      // 0x5e29b8
extern int g_module_version;                 // 0x5e29c0
extern float g_map_min_x;                    // 0x5aa120
extern float g_map_min_y;                    // 0x5aa124
extern float g_map_max_x;                    // 0x5aa118
extern float g_map_max_y;                    // 0x5aa11c
extern int g_missile_damage_speed_power_int; // 0x5e29c8 (provisional)

HRESULT build_shader();                                                     // 0x4c17c0
HRESULT gfx_restore();                                                      // 0x4c1b00
char* uninit_copy_204(blk204* first, blk204* last, char* dst, Alloc204* a, ptr_tag tag); // 0x4c1b60 (catch funclet 0x4c1bcc)
void* __cdecl read_psys_rec(stream_base* s, char* o);                       // 0x4bdc00 (cdecl; config.h declares it as the static member psys_record::read)

// ---- FMOD stream calls of the music monitor (0x4c25e0), stdcall thunks into fmod.dll -----------------------------------------
extern "C" int   __stdcall FSOUND_Stream_GetOpenState(void* stream);    // 0x534318 (0 ready, -2 still loading)
extern "C" int   __stdcall FSOUND_Stream_Play(int channel, void* stream);   // 0x534312
extern "C" int   __stdcall FSOUND_Stream_GetLengthMs(void* stream);     // 0x53430c
extern "C" int   __stdcall FSOUND_Stream_GetTime(void* stream);         // 0x534306
extern "C" short __stdcall FSOUND_Stream_Close(void* stream);          // 0x5342e2 (config.cpp: FSOUND_Stream_Close)
// ---- record vectors (0x4c1c20..0x4c23f0) ------------------------------------------------------------------------------------
#include "../../include/vec.h"   // V2: 8-byte vector with an empty user ctor: arrays of it go through the compiler's array-ctor helper
typedef tstring mstr;       // a tstring as a MouseRec member: its ctor / dtor are the outlined 0x42bbd0 / 0x42bc40
// The 0xc0-byte particle-system record of config.h (psys_record) with the members its array constructor and destructor
// touch (inlined into 0x4c1f70 / 0x4c1f20); the exact bytes need the outlined tstring ctor / dtor of `mstr`.
struct MouseRec {
    mstr     s0;            // +0
    int      v10;           // +0x10
    mstr     s1;            // +0x14
    int      v24, v28, v2c, v30, v34, v38, v3c, v40;    // +0x24
    V2       m0[2];         // +0x44
    V2       m1[2];         // +0x54
    V2       m2[2];         // +0x64
    V2       m3[2];         // +0x74
    V2       m4[2];         // +0x84
    char     pad94[0x2c];
};
#include "../trigger_vecs.h"   // TriggerVec / GameVec
struct MapIcon {            // module-file map icon record (0x4c20c0): name, flags, mesh name, four numbers, trigger entries at +0x3c
    tstring name;           // +0x00
    int     flags;          // +0x10
    tstring mesh;           // +0x14
    float   f24;            // +0x24
    int     f28;            // +0x28
    float   f2c, f30, f34;  // +0x2c
    char    pad38[4];
    GameVec triggers;       // +0x3c
    void read(stream_base& s);                  // 0x4c20c0 (ret 4)
};
struct TriggerEntryVec {
    RecH* data;             // +0 (RecH: ../../rgl/xmlcfg/records.h, name +0, SubB pair +0x10 = records / count / flags / name, tail +0x2c)
    int   count;            // +4 element count (read by resize 0x4c2250)
    void dtor();                                // 0x4c2200
};
int __cdecl entry_vec_resize(stream_base* s, TriggerEntryVec* self);   // 0x4c2250 (cdecl: the stream is the first stack argument, the vector the second)
#include "../mode_rec.h"
struct PresentationRec {    // module-file presentation (0x4c23f0)
    tstring name;           // +0x00
    int     flags;          // +0x10
    GameVec triggers;       // +0x14
    int     f20;            // +0x20
    void read(stream_base& s);                  // 0x4c23f0 (ret 4)
};
#include "../menu_rec.h"      // MenuRec (module-file game menu 0x4c2b20)
#endif // GAME_TRIGGERS_TRIGGER_VEC_H
