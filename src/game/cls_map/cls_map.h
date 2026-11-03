// cls_map.h: the first translation unit of 0x4d92c0..0x4e3eb0 (TU map 107 "cls_map"): the font definition object's
// glyph hash table, the application entry point, the font loader, the std::vector instantiations the linker
// placed here, the loading screen and its configuration record, the campaign windows (mission / meeting /
// done / debrief), the message window, the main and escape menus, the options screens and the meeting window.
// Layouts are read off the disassembly of the functions cited per field. ALL NAMES PROVISIONAL except where a
// vtable or a known class gives them.
//
// Canonical classes reused (never redefined): tstring (../tstring.h), Font / Glyph / GlyphMap (../font.h),
// gui_manager (../strat/gui_manager.h), LangTables (the string table; ../../rgl/mesh2/mesh2_common.h), E50 / E176 / R44 /
// b16 (../../include/vec_elems.h), E68 (../../include/e68.h), Config / ini_file (../config/config.h), Core (the
// application object; ../config/core.h), MissionUi and its widget classes (../ui), the widget factories' classes
// (../config/widget_mgr.h: WVt, DropDown, gui_mesh; ../ui/ui_widgets.h: UiAInline; ../ui/container.h: ContainerBase).
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
#include "../../include/ptr_tag.h"
#include "../font.h"
#include "../config/ui_widgets_c.h"
#include "../../include/game_data.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/render_flags.h"

// 0x43a1e0 is StrTable::lookup (agent2/agent_stats.cpp), which takes its key by reference: the one class, game/str_table.h
#include "../str_table.h"
#define g_strtable_c g_strtable
struct Map10007 {
    int lookup(unsigned id);   // 0x48ad00 (ui/mission_ui.cpp: the id -> index table of the party pool, ret 4)
};

// ---- 0x4d92c0..0x4d9450: the glyph hash table's helpers ---------------------------------------------------
// result of GlyphMap::insert (0x4d3c00): the node and whether it was new
struct HOut { GlyphNode* node; char inserted; };
// the value_type of the table: the key and the seven metric floats (a Glyph)
struct node_pair { int key; Glyph value; };

// ---- 0x4d9450: WinMain's globals and callees ---------------------------------------------------------------------

extern unsigned char g_mt_inited;                // 0x5e2a09
extern void*     g_mt_event;                     // 0x5e28d8
extern CRITICAL_SECTION g_mt_cs;                 // 0x5e28dc
extern FILE*     g_log_file;                     // 0x5a9f90 (game_globals.h: the log stream's FILE*)
extern unsigned char g_log_flag94;               // 0x5a9f94 (game_globals.h)
extern unsigned char g_log_flag95;               // 0x5a9f95 (game_globals.h)
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

// ---- 0x4d9e80: the object whose teardown calls 0x4c54f0 on its +0x78 member and then the editor selection handler 0x4d4b80
// 0x4c54f0 is graph_obj::calc_vertex_mode (rgl/mesh2/gui_edit.cpp) and 0x4d4b80 EditMesh::on_selection_changed (options_panel/hud.cpp): this object
// is that editor (both members run on it); only the members are declared, under their classes
struct graph_obj {
    void calc_vertex_mode(int mode);   // 0x4c54f0
};
struct EditMesh {
    bool on_selection_changed();   // 0x4d4b80
};
// the options-screen pages (options_panel/keys_page.cpp, quest_page.cpp, options_panel.cpp): the screens below are those objects; members only
struct Widget;
struct options_panel { void key_name(int which, tstring* out); void draw_faction_report(); };      // 0x4d7370 / 0x4d7e50
struct KeysPage { void refresh(); void update_perf(); void set_enabled_text(Widget* w); };        // 0x4d7120 / 0x4d6b80 / 0x4d6dd0
struct QuestPage {
    void update_difficulty();   // 0x4d6300
    void update_days();         // 0x4d6270
};
struct TearObj {
    char pad0[0x78];
    void* f78;                                  // +0x78
    void calc_vertex_mode(void* mode) { ((graph_obj*)this)->calc_vertex_mode((int)mode); }       // 0x4c54f0
    void on_selection_changed() { ((EditMesh*)this)->on_selection_changed(); }               // 0x4d4b80
    void teardown();                            // 0x4d9e80
};

// ---- 0x4da410..0x4dc1c0: the loading-screen configuration record --------------------------------------------------
// 0x1f8 bytes. Members (each a std::vector, copied by the out-of-line instantiations of src/rgl/mesh2: symbols/slices/mesh_2.txt):
// operator= 0x4da410 (this module), copy constructor 0x4dc030 (implicit; this module emits it out of line).
struct loading_config {
    tstring s0;                     // +0x00
    int v10, v14, v18, v1c, v20, v24;
    tstring s28;                    // +0x28
    int v38, v3c, v40, v44;
    std::vector<NamePtrs56> r48;    // +0x48 (copy ctor 0x4cc750, operator= 0x4cca70)
    std::vector<NamePtrs56> r58;    // +0x58
    std::vector<NamePtrs56> r68;    // +0x68
    std::vector<E516> r78;          // +0x78 (copy ctor 0x4cc830, operator= 0x4ccc00)
    int v88;                        // +0x88
    int r8c[0x40];                  // +0x8c (0x100 bytes)
    int v18c, v190, v194, v198;     // +0x18c
    std::vector<tstring> r19c;      // +0x19c (copy ctor 0x4cc680, operator= 0x4cc910)
    std::vector<tstring> r1ac;      // +0x1ac
    std::vector<E176> r1bc;         // +0x1bc (copy ctor 0x4da300, operator= 0x4d9840)
    struct Ints7 { int v1cc, v1d0, v1d4, v1d8, v1dc, v1e0, v1e4; } t1cc;    // +0x1cc (the copy constructor copies these seven as one block: rep movsd)
    int v1e8, v1ec, v1f0, v1f4;
    loading_config();                                           // model_b::model_b (src/game/model_b.h: the same 0x1f8-byte record; progress_bar.cpp forwards to it)
    ~loading_config();                                          // 0x4d3720 (model_b::~model_b)
    loading_config& operator=(const loading_config& src);     // 0x4da410
};
loading_config* copy_loading_config(loading_config* self, const loading_config* o);   // forces the implicit copy constructor 0x4dc030
loading_config* copy_configs_forward(loading_config* first, loading_config* last, loading_config* out);    // 0x4da880
loading_config* copy_configs_backward(loading_config* first, loading_config* last, loading_config* out);   // 0x4da8c0
void destroy_range_cfg(loading_config* first, loading_config* last, const loading_config* src);            // 0x4dc190

// ---- 0x4da840..0x4da9c0: element types of the global arrays the shutdown routine tears down ------------------------
// RecA is deleted through its vector deleting destructor 0x431960 (flags 3)
// 0x431960 is OwnedRec48's scalar vector deleting destructor (rgl/xmlcfg/records.cpp, thiscall, ret 4)
struct OwnedRec48 {
    void* vdel(unsigned flags);   // ??_EOwnedRec48 0x431960 (mangled as the destructor below)
};
struct DelA { void del3(int flags); };          // 0x431960
struct E20b {                                   // 0x20 bytes: a name and a deletable record at +0x1c (the slice's E20b)
    tstring s;                                  // +0x00
    char pad10[0xc];
    DelA* p;                                    // +0x1c
    ~E20b() { if (p) p->del3(3); }
};
// a record with two vector members: a non-scalar vector at +0 and a scalar vector at +0x10. Its compiler-generated
// destructor tears down the scalar member first, then the non-scalar one (0x4da9c0)
struct TwoVec {
    std::vector<E20b> v1;       // +0x00 (allocator, first +4, last +8, end +0xc)
    std::vector<int>  v2;       // +0x10 (allocator, first +0x14, last +0x18, end +0x1c)
    ~TwoVec();
};

// ---- 0x4dc350: the frame of the loading screen (a view of gui_manager) -----------------------------------------------
// A mesh's position array (+0x74) holds 0x20-byte records (a vector4 pair); +0x2c is its material, +0x3c a colour / id word,
// +0x138 a dirty flag; +0xfc / +0x100 are the uv corner this code moves. (A partial view of rgl::mesh.)
struct rec32 { rgl::vector4 p; rgl::vector4 q; };
struct FrameMesh {
    char pad0[0x2c];
    void* material;                             // +0x2c
    char pad30[0x3c - 0x30];
    int f3c;                                    // +0x3c
    char pad40[0x74 - 0x40];
    rec32* pos;                                 // +0x74
    char pad78[0xfc - 0x78];
    float uv_x;                                 // +0xfc
    float uv_y;                                 // +0x100
    char pad104[0x138 - 0x104];
    char dirty;                                 // +0x138
    char pad139[0x22c - 0x139];
    float c22c, c230;                           // +0x22c, +0x230
    char pad234[0x23c - 0x234];
    float c23c, c240;                           // +0x23c, +0x240
    char pad244[0x260 - 0x244];
    void ensure_local();                        // 0x412ee0 (rgl::mesh::ensure_local: defined as the call of it in cls_map.cpp)
};

// A view of gui_manager (g_gui): the quad mfb0 at +0xfb0, the corner meshes o1210..o121c, the colour word f16e4.
struct FrameGui {
    char pad0[0xfb0];
    FrameMesh quad;                             // +0xfb0
    FrameMesh* corner[4];                       // +0x1210
    char pad1220[0x16e4 - 0x1220];
    int f16e4;                                  // +0x16e4
    void set_rect(const float* a, const float* b);   // 0x4dc350 (ret 8)
};


struct VDel { virtual void del(int flags); };   // slot 0 of any polymorphic object: the deleting destructor

// ---- 0x4daa10: the loading-screen object (a screen like the menus below: the overlay window at +0x2c, three child widgets) ----
// (0x4da900 is gui_manager::rebuild, ../strat/gui_manager.h; this object only shares the offsets +0x568.. with it by accident of the slice)
#include "../triggers/item_rec.h"       // ItemRec / ItemRec2: the main menu and the escape menu (slot 2 is create)
#include "../screen_records.h"        // C4c2fb0 (the "creating game" panel; slot 2 is the loading-window builder 0x4daa10) and C4c3290 (the start-up loading screen)

// ---- widgets: this module's call sites go through vt() (../config/widget_mgr.h): slot n of the widget vtable is dNN at +4n ----
// The slots and the call sites that name them (offset: role: callers in this module):
//   +0x0c d03 rebuild: 0x4dc630   +0x2c d11 select(index): 0x4e05c0   +0x58 d22 position pair   +0x64 d25 scale pair
//   +0x6c d27 size pair   +0x70 d28 text offset pair   +0x78 d30 text flags: 0x4dc630   +0x7c d31 install mesh(kind): 0x4dada0
//   +0x80 d32 install mesh(slot)   +0x98 d38 colour   +0x9c d39 highlight colour   +0xa4 d41 set text (out, text)
//   +0xac d43 set text (text, pos): 0x4db090   +0xb8 d46 hide: 0x4dcef0   +0xc8 d50 add child
// The overlay window (vtable 0x592848, 0x190 bytes): the base constructor 0x4931a0 (ContainerBase), then the vptr and the byte at +0x188.
// The two sites that build it (DebriefMenu::create_debrief 0x4de5b0, SaveWinCtl::create 0x4e5d90) call the constructor out of line;
// the other sites (text_box columns, MeetingWindow::init) have it inlined (`new text_box`).
struct ContainerW {
    void* construct();   // 0x4931a0: the base constructor of the overlay window (world_data/world_data.cpp ContainerW::construct, returns this)
};
#define NEW_OVERLAY_WINDOW(var) \
    ContainerBase* var; \
    { ContainerBase* np_ = (ContainerBase*)::operator new(0x190); \
      var = np_ != 0 ? (((ContainerW*)np_)->construct(), *(void**)np_ = (void*)vtbl_592848, ((NodeA*)np_)->f188 = 0, np_) : 0; }
// the resource manager's get_object (0x419540) for tstring keys, and the UI globals
// 0x419540 is resmgr::get_object (rgl/core/resmgr.h), which cannot be included next to resource.h: only the member is declared
// 0x49b120 is NodeVec::push_back (config/widget_node.cpp: the UI root vector at the start of g_gui / g_ui_sound), not gui_manager::add_root:
// only the member is declared under the defining class, the root window pointer is taken by NodeA* const&
struct NodeVec {
    void push_back(NodeA* const& x);   // 0x49b120
};
#define ADD_ROOT_REF(w) ((NodeVec*)&g_gui)->push_back((NodeA* const&)(w))   // a void* variable passed by reference as the original does
#define ADD_ROOT(w) do { NodeA* root_ = (NodeA*)(w); ((NodeVec*)&g_gui)->push_back(root_); } while (0)
struct lod_group;
struct resmgr { rgl::mesh* get_object(const rgl::string* name);      // 0x419540
                lod_group* get_lod_mesh(const rgl::string& name); };   // 0x41cdc0 (rgl/core/resmgr.h)
inline gui_mesh* res_get(const tstring& name) { return (gui_mesh*)((resmgr*)&g_resmgr)->get_object((const rgl::string*)&name); }
extern gui_manager g_gui;                       // 0x5d7080
extern unsigned char g_gui_dirty;               // 0x5d8788 (set to 1 whenever a window was installed; game_globals.h declares it, which clashes with this header's table views)

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
    void fill_single_mission();                 // 0x4db020
    void fill_mission_grid();                   // 0x4db090
    void fill_mission_row(int side);            // 0x4db440
    void draw_faction_report() { ((options_panel*)this)->draw_faction_report(); }      // 0x4d7e50 (options_panel/options_panel.cpp)
};
// the hint bar / label maker (0x4dada0) and the 0x4db810 button maker
MissionUi* __cdecl make_hint(ContainerBase* o, const tstring& text, float x, float y, float w, float h);   // 0x4dada0
UiAInline* __cdecl new_button(ContainerBase* o, const tstring& text, float px, float py, float sx, float sy);   // 0x4db810

// globals of the campaign diary screens (addresses; names provisional)
extern GameData* g_game;                        // 0x5e2930
#include "../mission.h"   // canonical: g_mission (0x5e292c)
#include "../skill_cap.h"   // canonical: g_skill_tbl (0x8ee3100)
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): the one definition
extern float g_5a9f24;                          // 0x5a9f24 0.65 (the meeting window columns' top)
extern float g_5a9f28[];                        // 0x5a9f28 {0.0, 0.2145} ([1] is 0.21450001, not the literal 0.2145f)
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128: stop_all 0x494310

// ---- 0x4dc630..0x4dce70: the message (result) window and its button makers ----------------------------------------------
// the option table (0xa44240): ini_file, entries of 0x2c bytes with the value string at +0x1c of each entry (from +0x1c: 0xa4425c)
extern ini_file g_game_vars;                          // 0xa44240
#include "../result_window.h"                    // ResultWindow: the head_rec record of vtable 0x595740 (its slot 2 is the window maker below)
UiAInline* new_long_button(MissionUi* parent, const tstring* text, float a, float b, float c);     // 0x4dcbd0
UiAInline* new_medium_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4dce70

// ---- 0x4dcef0..0x4de6f0: the main menu, the in-game (escape) menu, the message menu and the debrief menu -------------------
// the menu-kind record (stride 0x24; table g_menu_kinds 0xa75f2c): the menu's background image count and its sound holder
struct SoundHolder { void play(int id); };      // 0x48add0 (ret 4: the sound holder is a TimerList, ../ui/mission_ui.h)
struct MenuKindRec {
    char pad0[0x10];
    int a;                                      // +0x10 (stored into the block owner's f14)
    SoundHolder sound;                          // +0x14
    char pad18[0x20 - 0x18];
    int first;                                  // +0x20 background image id (0 = none)
};
extern MenuKindRec* g_presentations;               // 0xa75f2c
extern int g_menu_kind_count;                   // 0xa75f38
// the menu block: a widget (vtable 0x5924a0, 0x188 bytes) that owns a vector (alloc +0x178, first +0x17c, last, end). The UiFrame of
// ../party/party_window.h (destroying destructor 0x5021f0); that header conflicts with game_data.h on g_game, so this is its view here.
#include "../ui/ui_frame.h"                  // UiFrame (vtable 0x5924a0): the one definition
// a menu block owner: the selected kind, its value and a tick (embedded at +0x30 of the main / escape menu)
struct BlockOwner {
    char pad0[0x10];
    int index;                                  // +0x10 the menu kind (-1 = none)
    int f14;                                    // +0x14 = g_menu_kinds[index].a
    Timer timer;                                // +0x18 clock index, +0x20 the stamp (activate 0x4dd9e0)
    char pad28[4];
    UiFrame* block;                           // +0x2c
    void init(ContainerBase* parent, int which);   // 0x4dd920 (ret 8)
    void activate();                            // 0x4dd9e0
};
UiAInline* build_row(ContainerBase* parent, const tstring* key, float x, float y, float w, float h);   // 0x4dd830
__declspec(noinline) UiAInline* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
                            const tstring& name, const tstring& down, float k1, float k2, float k3);   // 0x4dcc50
void mb_snprintf(char* dest, unsigned size, const char* fmt, ...);   // 0x415100
void scan_saves(int ids, int* count, int* first, const char* prefix);   // 0x4dd6e0
void save_path(char* out);                                              // 0x4dd7b0

#include "../screen_records.h"          // C4c3050: the message window record (slot 2 is the window maker 0x4ddf80)

// globals of the menus
extern int g_music_situation;                      // 0xa73a10 (set to 0x400000)
extern int g_main_menu_kind;                    // 0xa75f34


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
inline void Recd8::update_difficulty() { ((QuestPage*)this)->update_difficulty(); } // 0x4d6300 (options_panel/quest_page.cpp)      // the record of screen_records.h (vtable class); its members live there

// ---- 0x4e05c0: the options screen ("Win" of the slice) ---------------------------------------------------------------------------------
// a display mode of the mode list (0x4cf100): "WxH ... 16 bits / 32 bits"
struct DisplayMode { tstring name; int w, h, bpp; };
std::vector<tstring> list_modes();                // 0x4cf100 (options_panel/config_dialog.cpp; hidden return pointer; a DisplayMode starts with its name)
typedef Recf0 OptionsWin;      // the record of screen_records.h (vtable class); its members live there
extern int g_use_shaders;        // 0x5e28fc

// ---- 0x4e25a0..0x4e3780: the key-binding page of the options menu (global at 0xa73730; the key table and the page share it) -----------
struct OptionsMenu {
    int f00;                                    // +0x00 (first key word; copy_slots 0x4e2cb0 copies it with the rest)
    int f04, f08, f0c, f10, f14, f18, f1c, f20, f24, f28;   // +0x04
    ContainerBase* quad;                        // +0x2c the options_window container
    MissionUi* f30;                             // +0x30 ui_done button
    MissionUi* f34;                             // +0x34 ui_undo_changes
    MissionUi* f38;                             // +0x38 ui_set_default_keys
    MissionUi* slots[0x25];                     // +0x3c .. +0xd0 the 37 key-row labels
    MissionUi* slot_buttons[0x25];              // +0xd0 .. +0x164 their buttons
    MissionUi* f164;                            // +0x164 "press a key" label
    MissionUi* f168;                            // +0x168 battle controls caption
    MissionUi* f16c;                            // +0x16c map controls caption
    MissionUi* captions[7];                     // +0x170 .. +0x18c the 7 key names
    MissionUi* key_values[7];                   // +0x18c .. +0x1a8 their bound keys
    MissionUi* f1a8;                            // +0x1a8 general controls caption
    MissionUi* f1ac;                            // +0x1ac ui_take_screenshot
    MissionUi* f1b0;                            // +0x1b0 "Ctrl + Insert"
    char f1b4;                                  // +0x1b4 modified
    char pad1b5[3];
    int f1b8;                                   // +0x1b8 the row being assigned, -1 for none
    void init();                                // 0x4e25a0
    void copy_slots();                          // 0x4e2cb0
    void refresh() { ((KeysPage*)this)->refresh(); }                           // 0x4d7120 (thiscall; options_panel/keys_page.cpp)
    void key_name(int i, tstring* out) { ((options_panel*)this)->key_name(i, out); }    // 0x4d7370 (thiscall, ret 8; options_panel/options_panel.cpp)
    tstring key_text(int i);                    // 0x4e2ce0 (returns a tstring: hidden out pointer, ret 8)
    tstring slot_text(int i);                   // 0x4e2ea0 (same convention)
};
extern OptionsMenu g_game_keys;                      // 0xa73730

#endif // GAME_CLS_MAP_CLS_MAP_H
