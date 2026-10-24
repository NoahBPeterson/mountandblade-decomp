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
#include "../strat/gui_manager.h"
#include "../config/core.h"
#include "../config/config.h"
#include "../party/game.h"
#include "../../rgl/xmlcfg/records.h"
#include "../../rgl/mesh2/rec_tables.h"       // Alloc2c / Alloc204 (0x4c1b20 / 0x4c1b40), sound_engine
#include "../../rgl/resource.h"             // texture
#include "../shadow_map.h"
#include "../render_batch.h"

// ---- the options dialog's persistent values (0x4bf170 reads them, 0x4bee10 writes them back) -------------------------
extern int g_opt_5aa0ac;    // 0x5aa0ac  first combo's selection (control 0x43e)
extern int g_opt_5aa0b0;    // 0x5aa0b0  0x445
extern int g_opt_5aa0b4;    // 0x5aa0b4  fps limit (10..999), edit control 0x448
extern int g_opt_5aa0b8;    // 0x5aa0b8  0x444
extern int g_opt_5aa0c0;    // 0x5aa0c0  0x44c
extern int g_opt_5aa0c4;    // 0x5aa0c4  0x44a
extern int g_opt_5aa0cc;    // 0x5aa0cc  0x442
extern int g_opt_5aa0d0;    // 0x5aa0d0  0x443 (inverted)
extern int g_opt_5aa0d8;    // 0x5aa0d8  0x44d (inverted)
extern int g_opt_5aa0dc;    // 0x5aa0dc  0x468
extern int g_opt_5aa0e0;    // 0x5aa0e0  0x441
extern int g_opt_5aa0e4;    // 0x5aa0e4  0x446
extern int g_opt_5aa0e8;    // 0x5aa0e8  0x452
extern int g_opt_5aa0f4;    // 0x5aa0f4  resolution width
extern int g_opt_5aa0f8;    // 0x5aa0f8  resolution height
extern int g_opt_5aa0fc;    // 0x5aa0fc  colour depth (0x10 / 0x20)
extern int g_opt_5aa100;    // 0x5aa100  control 0x465 item data
extern int g_opt_5aa104;    // 0x5aa104  multisample type found by the probe (0x4bf170)
extern int g_opt_5aa108;    // 0x5aa108  0x487
extern int g_opt_5aa110;    // 0x5aa110  control 0x467 item data

// ---- meta mission (global object at 0xa73300, 0x4be742: called as MetaMission::init; the original brackets it with an
// 18-byte WinLicense marker pair at 0x4be730 / before 0x4be7c0) ---------------------------------------------------
#include "../meta_mission.h"      // MetaMission (0xa73300): the one definition (triggers and party)

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
extern ID3DXEffect* g_effect;           // 0x5e293c
extern int g_use_shaders;               // 0x5e28fc
extern D3DCAPS9 g_caps;                 // 0xa17f2c (GetDeviceCaps fills it)
extern int  g_renderer_vsync;           // 0xa180e4
extern char g_renderer_flag;            // 0xa17e14
extern unsigned char g_cursor_flag;     // 0x5d75e0 ShowCursor argument
extern float g_gamma;                   // 0x5aa088
extern tstring g_path_prefix;           // 0x8ee3c28 shader directory prefix
extern tstring g_module_name;           // 0x8ee3c38 the selected module (load_game_variables)
extern tstring g_module_info_name;      // 0xa180f8 module_name directive of module_info.txt
extern render_batch g_batch;            // 0x5fe898
extern ShadowMap g_shadow_map;          // 0xa70248 (w at +0x10 = 0xa70258, surf at +0xc = 0xa70254)
extern ini_file g_module_info;          // 0xa18238 module_info.txt table (count at 0xa44238)
extern ini_file g_game_vars;            // 0xa44240 game_variables.txt table (rec_tables.h views it as ini_file_x g_ini)
extern resource_manager_x g_resmgr_x;   // declared in config.h (0x5d6fa0)
extern int g_display_wp_one_handed;          // 0x5e2900
extern int g_display_wp_two_handed;          // 0x5e2904
extern int g_display_wp_polearms;            // 0x5e2908
extern int g_display_wp_archery;             // 0x5e290c
extern int g_display_wp_crossbows;           // 0x5e2910
extern int g_display_wp_throwing;            // 0x5e2914
extern int g_display_wp_firearms;            // 0x5e2918
extern int g_compatible_module_version;      // 0x5e29b8
extern int g_module_version;                 // 0x5e29c0
extern int g_scan_module_textures;           // 0xa18108
extern int g_scan_module_sounds;             // 0xa1810c
extern float g_time_multiplier;              // 0xa18110
extern float g_seeing_range;                 // 0xa18114
extern float g_track_spotting;               // 0xa18118
extern float g_blood_multiplier;             // 0xa1811c
extern int g_skill_leadership_bonus;         // 0xa18120
extern int g_skill_prisoner_management_bonus;// 0xa18124
extern int g_base_companion_limit;           // 0xa18128
extern float g_player_xp_multiplier;         // 0xa1812c
extern float g_hero_xp_multiplier;           // 0xa18130
extern float g_regulars_xp_multiplier;       // 0xa18134
extern float g_hero_wounded_treshold;        // 0xa18138
extern float g_player_wounded_treshold;      // 0xa1813c
extern int g_num_hints;                      // 0xa18140
extern float g_map_sea_direction;            // 0xa18144
extern float g_map_sea_speed_x;              // 0xa18148
extern float g_map_sea_speed_y;              // 0xa1814c
extern float g_map_river_direction;          // 0xa18150
extern float g_map_river_speed_x;            // 0xa18154
extern float g_map_river_speed_y;            // 0xa18158
extern int g_map_tree_types;                 // 0xa1815c
extern int g_map_snow_tree_types;            // 0xa18160
extern int g_map_steppe_tree_types;          // 0xa18164
extern int g_map_desert_tree_types;          // 0xa18168
extern int g_starting_year;                  // 0xa1816c
extern int g_starting_month;                 // 0xa18170
extern int g_starting_day;                   // 0xa18174
extern float g_damage_interrupt_attack_treshold;      // 0xa18178
extern float g_armor_soak_factor_against_cut;         // 0xa1817c
extern float g_armor_soak_factor_against_pierce;      // 0xa18180
extern float g_armor_soak_factor_against_blunt;       // 0xa18184
extern float g_armor_reduction_factor_against_cut;    // 0xa18188
extern float g_armor_reduction_factor_against_pierce; // 0xa1818c
extern float g_armor_reduction_factor_against_blunt;  // 0xa18190
extern float g_missile_damage_speed_power;   // 0xa18194
extern float g_melee_damage_speed_power;     // 0xa18198
extern float g_horse_charge_damage_multiplier;   // 0xa1819c
extern float g_couched_lance_damage_multiplier;  // 0xa181a0
extern int g_main_menu_font_color_red;       // 0xa181bc
extern int g_main_menu_font_color_green;     // 0xa181c0
extern int g_main_menu_font_color_blue;      // 0xa181c4
extern int g_works_with_version_min;         // 0xa181c8
extern int g_works_with_version_max;         // 0xa181cc
extern float g_map_min_x;                    // 0x5aa120
extern float g_map_min_y;                    // 0x5aa124
extern float g_map_max_x;                    // 0x5aa118
extern float g_map_max_y;                    // 0x5aa11c
extern int g_xa181a4;                        // 0xa181a4
extern int g_xa181a8;                        // 0xa181a8
extern int g_xa181ac;                        // 0xa181ac
extern int g_xa181b0;                        // 0xa181b0
extern int g_xa181b4;                        // 0xa181b4
extern unsigned char g_xa181b8;              // 0xa181b8
extern int g_missile_damage_speed_power_int; // 0x5e29c8 (provisional)

HRESULT build_shader();                                                     // 0x4c17c0
HRESULT gfx_restore();                                                      // 0x4c1b00
char* uninit_copy_204(blk204* first, blk204* last, char* dst, Alloc204* a); // 0x4c1b60 (catch funclet 0x4c1bcc)
void __fastcall tstring_ctor(tstring* p);                                   // 0x42bbd0
void __fastcall tstring_dtor(tstring* p);                                   // 0x42bc40
void* __fastcall dialog_mgr_ctor(void* self);                               // 0x4c2470 (DialogMgr::DialogMgr)
void* __cdecl read_psys_rec(stream_base* s, char* o);                       // 0x4bdc00 (cdecl; config.h declares it as a member)

// ---- FMOD stream calls of the music monitor (0x4c25e0), stdcall thunks into fmod.dll -----------------------------------------
extern "C" int   __stdcall FSOUND_Stream_GetOpenState(void* stream);    // 0x534318 (0 ready, -2 still loading)
extern "C" int   __stdcall FSOUND_Stream_Play(int channel, void* stream);   // 0x534312
extern "C" int   __stdcall FSOUND_Stream_GetLengthMs(void* stream);     // 0x53430c
extern "C" int   __stdcall FSOUND_Stream_GetTime(void* stream);         // 0x534306
extern "C" short __stdcall FSOUND_Stream_Close_(void* stream);          // 0x5342e2 (config.cpp: FSOUND_Stream_Close)
extern int g_music_request;             // 0xa70474 track requested by the scripts (-1 none)
extern int g_music_request_kind;        // 0xa70478
extern int g_music_event;               // 0xa7047c one-shot jingle request (-1 none)
extern float g_music_volume_floor;      // 0x590cd0 (0.02f) volume below which the monitor idles
// ---- record vectors (0x4c1c20..0x4c23f0) ------------------------------------------------------------------------------------
#include "../../include/vec.h"   // V2: 8-byte vector with an empty user ctor: arrays of it go through the compiler's array-ctor helper
struct mstr {               // tstring as a MouseRec member: the real ctor / dtor are the outlined 0x42bbd0 / 0x42bc40
    int pos; char* buf; int cap; int len;
    mstr() throw();
    ~mstr() throw();
};
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
// the vector deleting destructor of RecH (??_ERecH, ret 4), emitted in rgl/xmlcfg/records.cpp
struct RecHVdel {
    void* vdel(unsigned flags);                 // 0x4315d0
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
