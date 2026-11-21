// cls_map.h: the first translation unit of 0x4d92c0..0x4e3eb0 (TU map 107 "cls_map"): the font definition object's
// glyph hash table, the application entry point, the font loader, the std::vector instantiations the linker
// placed here, the loading screen and its configuration record, the campaign windows (mission / meeting /
// done / debrief), the message window, the main and escape menus, the options screens and the meeting window.
// Layouts are read off the disassembly of the functions cited per field. ALL NAMES PROVISIONAL except where a
// vtable or a known class gives them.
//
// Canonical classes reused (never redefined): tstring (../tstring.h), Font / Glyph / GlyphMap (../font.h),
// gui_manager (../strat/gui_manager.h), LangTables (the string table; ../../rgl/mesh2/mesh2_common.h), FaceTexture
// (../../include/vec_elems.h), RagdollBone (../../rgl/core/bone.h), E68 (../../include/e68.h), Config / ini_file (../config/config.h), Core (the
// application object; ../config/core.h), MissionUi and its widget classes (../ui), the widget factories' classes
// (../config/widget_mgr.h: DropDown, rgl::mesh; ../ui/ui_widgets.h: UiA; ../ui/container.h: ContainerBase).
#ifndef GAME_CLS_MAP_CLS_MAP_H
#define GAME_CLS_MAP_CLS_MAP_H
#include "../../include/image_addrs.h"
#include <windows.h>
#include <new>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../config/config.h"
#include "../config/widget_mgr.h"
#include "../strat/gui_manager.h"
#include "../../rgl/mesh2/mesh2_common.h"
#include "../../include/e68.h"
#include "../../include/vec_elems.h"
#include "../../rgl/core/bone.h"          // RagdollBone (the 0x50-byte ragdoll bone: vector<RagdollBone> 0x4d9370..0x4dc1c0)
#include "../../include/ptr_tag.h"
#include "../font.h"
#include "../config/ui_widgets_c.h"
#include "../../include/game_data.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/render_flags.h"

// 0x43a1e0 is StrTable::lookup (agent2/agent_stats.cpp), which takes its key by reference: the one class, game/str_table.h
#include "../str_table.h"
#include "../../rgl/text_stream.h"
#define g_strtable_c g_strtable

// ---- 0x4d92c0..0x4d9450: the glyph hash table's helpers ---------------------------------------------------
// result of GlyphMap::insert (0x4d3c00): the node and whether it was new
struct HOut { GlyphNode* node; char inserted; };
// the value_type of the table: the key and the seven metric floats (a Glyph)
struct node_pair { int key; Glyph value; node_pair(const int& k, const Glyph& v) : key(k), value(v) {} };

// ---- 0x4d9450: WinMain's globals and callees ---------------------------------------------------------------------

extern unsigned char g_mt_inited;                // 0x5e2a09
extern void*     g_mt_event;                     // 0x5e28d8
extern CRITICAL_SECTION g_mt_cs;                 // 0x5e28dc
struct MapIcon;
extern MapIcon*  g_map_icons;                    // 0x5e29f0 (game_globals.h)
extern int       g_video_played;                 // 0x5e2a10
// (the options dialog's choose_dx_settings 0x4b9b90 is OptionsDlg's own member, ../config/options_dlg.h; DXUtil_Timer 0x407660 is dxframework/dxutil.h's)
FILE* __cdecl mb_open_user_file(const char* name, const char* mode);   // 0x415160
// 0x4c3a40 is options_record::load (rgl/mesh2/rec_tables.cpp: the record at 0x5aa008, ecx = the object) and 0x4c8900 the stdcall thread body of
// rgl/mesh2/gui_edit.cpp; 0x4c4dc0 is the fastcall std::list<int>::_Buynode of the glyph table's list (ecx = the list at Font + 0x48)
struct options_record {
    void load();   // 0x4c3a40
};
void __stdcall loader_thread_body();             // 0x4c8900
struct list_int;
void* __fastcall list_int_buynode(list_int* self);   // 0x4c4dc0

// ---- 0x4d9e80: PolyMesh::teardown (terrain_b/poly_mesh.h): re-marks the vertices of the current mode (calc_vertex_mode 0x4c54f0,
// rgl/mesh2/gui_edit.cpp) and runs the selection handler (on_selection_changed 0x4d4b80, options_panel/hud.cpp) on the editor's mesh
#include "../terrain_b/poly_mesh.h"
// the options-screen pages (options_panel/keys_page.cpp, quest_page.cpp, options_panel.cpp): the screens below are those objects; members only

// ---- 0x4da410..0x4dc1c0: the skin record's copy helpers ---------------------------------------------------------------
// Skin (../../include/skin.h) is the 0x1f8-byte record: operator= 0x4da410 (this module), copy constructor 0x4dc030
// (implicit; this module emits it out of line).
#include "../../include/skin.h"
Skin* copy_skin(Skin* self, const Skin* o);   // forces the implicit copy constructor 0x4dc030
Skin* copy_configs_forward(Skin* first, Skin* last, Skin* out);    // 0x4da880
Skin* copy_configs_backward(Skin* first, Skin* last, Skin* out);   // 0x4da8c0
void destroy_range_cfg(Skin* first, Skin* last, const Skin* src);            // 0x4dc190

// ---- 0x4da840..0x4da9c0: element types of the global arrays the shutdown routine tears down ------------------------
// RecA is deleted through its vector deleting destructor 0x431960 (flags 3)
// 0x431960 is OwnedRec48's scalar vector deleting destructor (rgl/xmlcfg/records.cpp, thiscall, ret 4)
#include "../../include/rec_dtors.h"      // OwnedRec48 (vector deleting destructor 0x431960)
struct E20b {                                   // 0x20 bytes: a name and a deletable record at +0x1c (the slice's E20b)
    tstring s;                                  // +0x00
    char pad10[0xc];
    OwnedRec48* p;                              // +0x1c
    ~E20b() { if (p) delete[] p; }
};

struct VDel { virtual void del(int flags); };   // slot 0 of any polymorphic object: the deleting destructor

// ---- 0x4daa10: the loading-screen object (a screen like the menus below: the overlay window at +0x2c, three child widgets) ----
// (0x4da900 is gui_manager::rebuild, ../strat/gui_manager.h; this object only shares the offsets +0x568.. with it by accident of the slice)
#include "../triggers/item_rec.h"       // ItemRec / ItemRec2: the main menu and the escape menu (slot 2 is create)
#include "../screen_records.h"        // C4c2fb0 (the "creating game" panel; slot 2 is the loading-window builder 0x4daa10) and C4c3290 (the start-up loading screen)

// ---- widgets: this module calls MissionUi's virtuals by name (../ui/mission_ui.h): move_a (position pair), set_xy (scale
// pair), set_size / set_offset, set148 (text flags), set_mesh / set_track, set_color / set_target (colour, highlight colour),
// set_text, layout_text_to, flag2_set (hide), ContainerBase::add_child and DropDown::append (a row).
// The overlay window (vtable 0x592848, 0x190 bytes) is ui/container.h's NodeA: `new NodeA(0)` (the byte at +0x188 cleared).
// the resource manager's get_object (0x419540) for tstring keys, and the UI globals
// 0x419540 is resmgr::get_object (rgl/core/resmgr.h), which cannot be included next to resource.h: only the member is declared
#include "../../rgl/resource.h"                  // resmgr g_resmgr (0x5d6fa0): get_object 0x419540, get_lod_mesh 0x41cdc0
inline rgl::mesh* res_get(const tstring& name) { return (rgl::mesh*)g_resmgr.get_object(&name); }

// ---- 0x4dada0..0x4db660: the campaign diary screens (mission window, meeting window, "done" window) --------------------
// The screen manager (`this` of 0x4db020..0x4db660, 0x4de1b0..): one of the screen objects, a window pointer table.
struct MissionScreen {
    virtual void s00(int);                      // (vptr at +0)
    int f4;                                     // +0x04 dynamic pass selector (0x4db660: 0 runs 0x4d7e50)
    char pad8[0x20 - 8];
    int f20;                                    // +0x20
    char pad24[0x2c - 0x24];
    ContainerBase* overlay;                     // +0x2c the window (0x4dae40, 0x4db1f0, 0x4db660)
    MissionUi* row[16];                         // +0x30 unit widgets of the grid; row[2] = the mission text (0x4dae40); row[0..2] are f30/f34/f38 of the done window
    ContainerBase* col[3];                      // +0x70 per-side column windows (0x4db1f0)
    char pad7c[0x184 - 0x7c];
    int f184;                                   // +0x184
};
// the hint bar / label maker (0x4dada0) and the 0x4db810 button maker
MissionUi* __cdecl make_hint(ContainerBase* o, const tstring& text, float x, float y, float w, float h);   // 0x4dada0
UiA* __cdecl new_button(ContainerBase* o, const tstring& text, float px, float py, float sx, float sy);   // 0x4db810

// globals of the campaign diary screens (addresses; names provisional)
extern GameData* g_game;                        // 0x5e2930
#include "../mission.h"   // canonical: g_mission (0x5e292c)
#include "../skill_cap.h"   // canonical: g_skill_tbl (0x8ee3100)
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): the one definition
extern float g_5a9f24;                          // 0x5a9f24 0.65 (the meeting window columns' top)
extern vec2 g_5a9f28;                           // 0x5a9f28 {0.0, 0.2145} ([1] is 0.21450001, not the literal 0.2145f)
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128: stop_all 0x494310

// ---- 0x4dc630..0x4dce70: the message (result) window and its button makers ----------------------------------------------
// the option table (0xa44240): ini_file, entries of 0x2c bytes with the value string at +0x1c of each entry (from +0x1c: 0xa4425c)
extern ini_file g_game_vars;                          // 0xa44240
#include "../result_window.h"                    // ResultWindow: the head_rec record of vtable 0x595740 (its slot 2 is the window maker below)
UiA* new_long_button(MissionUi* parent, const tstring* text, float a, float b, float c);     // 0x4dcbd0
UiA* new_medium_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4dce70

// ---- 0x4dcef0..0x4de6f0: the main menu, the in-game (escape) menu, the message menu and the debrief menu -------------------
#include "../trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list
#include "../game_globals.h"                  // g_presentations (0xa75f2c, canonical PresentationTable: the PresentationRec records)
// the menu block: a widget (vtable 0x5924a0, 0x188 bytes) that owns a vector (alloc +0x178, first +0x17c, last, end). The UiFrame of
// ../party/party_window.h (destroying destructor 0x5021f0); that header conflicts with game_data.h on g_game, so this is its view here.
#include "../ui/ui_frame.h"                  // UiFrame (vtable 0x5924a0): the one definition
#include "../presentation.h"                // widget_mgr: the presentation the menus embed at +0x30 (init 0x4dd920, activate 0x4dd9e0)
UiA* build_row(ContainerBase* parent, const tstring* key, float x, float y, float w, float h);   // 0x4dd830
__declspec(noinline) UiA* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
                            const tstring& name, const tstring& down, float k1, float k2, float k3);   // 0x4dcc50
void scan_saves(int* ids, int* count, int* first, const char* prefix);   // 0x4dd6e0
void save_path(char* out);                                              // 0x4dd7b0

#include "../screen_records.h"          // C4c3050: the message window record (slot 2 is the window maker 0x4ddf80)

// globals of the menus: g_presentations.used[0] (prsnt_game_start) is the main menu kind


// 0x4de3e0: builds a textured quad (four points, four vertices, two faces) in a mesh; a member of the mesh class
struct PanelMesh : rgl::mesh {
    void build_panel(const float* base, const float* size, const float* uv0, const float* uv1, int* col);   // 0x4de3e0
};
rgl::mesh* make_background(float* v, int color);   // 0x4de320

// ---- 0x4dfd50 / 0x4dfec0: the horizontal slider and the drop-down button makers ----------------------------------------------
UiB* make_slider(MissionUi* parent, float x, float y, float k);                          // 0x4dfd50
DropDown* make_dropdown(MissionUi* parent, const tstring* name, float x, float y);       // 0x4dfec0

// ---- 0x4de6f0: the main-menu options screen (the screen object; fields are widget pointers) ------------------------------------
typedef Recd8 OptionsScreen;

// ---- 0x4e05c0: the options screen ("Win" of the slice) ---------------------------------------------------------------------------------
// a display mode of the mode list (0x4cf100): "WxH ... 16 bits / 32 bits"
struct DisplayMode { tstring name; int w, h, bpp; };
std::vector<tstring> list_modes();                // 0x4cf100 (options_panel/config_dialog.cpp; hidden return pointer; a DisplayMode starts with its name)
typedef Recf0 OptionsWin;      // the record of screen_records.h (vtable class); its members live there
extern int g_use_shaders;        // 0x5e28fc

#include "../game_keys.h"                    // GameKeys g_game_keys (0xa73730)
// (0x4e25a0..0x4e3780: the key-binding page, screen_records.h's Rec1bc; the table it edits is GameKeys)

#endif // GAME_CLS_MAP_CLS_MAP_H
