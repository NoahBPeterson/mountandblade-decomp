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
// (../config/widget_mgr.h: WVt, ButtonBase, UiAInline, ContainerBase, DropDown, gui_mesh).
#ifndef GAME_CLS_MAP_CLS_MAP_H
#define GAME_CLS_MAP_CLS_MAP_H
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

// ---- 0x4d92c0..0x4d9450: the glyph hash table's helpers ---------------------------------------------------
// result of GlyphMap::insert (0x4d3c00): the node and whether it was new
struct HOut { GlyphNode* node; char inserted; };
// the value_type of the table: the key and the seven metric floats (a Glyph)
struct node_pair { int key; Glyph value; };

// ---- 0x4d9450: WinMain's globals and callees ---------------------------------------------------------------------
// fields of the application object g_app_obj (../config/core.h, 0x8ee3c48) that the framework class keeps protected
extern HINSTANCE g_hinstance;                    // 0x8ee3fa0 (g_app_obj + 0x358)
extern int       g_init_flag;                    // 0x8ee3f9c (g_app_obj + 0x354)
extern HWND      g_editor_hwnd;                  // 0x8ee3d30 (m_hWnd)
extern unsigned char g_fullscreen_flag;          // 0x8ee3f8e (m_bStartFullscreen)
extern float     g_res_scale;                    // 0x8ee3c50 (m_fMinFrameTime)
extern tstring   g_skin;                         // 0x5fe888 the language of the skin file name
extern unsigned char g_mt_inited;                // 0x5e2a09
extern void*     g_mt_event;                     // 0x5e28d8
extern CRITICAL_SECTION g_mt_cs;                 // 0x5e28dc
extern int       g_video_played;                 // 0x5e2a10
// the options dialog's method on the application object: the choice of the DirectX settings from the option block
struct OptionsDlgView {
    void choose_dx_settings(Config* cfg, bool overwrite);   // 0x4b9b90
};
FILE* __cdecl mb_open_user_file(const char* name, const char* mode);   // 0x415160
void __fastcall init_options_obj(void* self);    // 0x4c3a40 (ecx = the object at 0x5aa008)
void __cdecl worker_thread(void* arg);           // 0x4c8900
float __cdecl timer_op(int op);                  // 0x407660

// std::list<int>::_Buynode of the glyph table's list (0x4c4dc0; ecx = the list at Font + 0x48)
void* __fastcall list_int_buynode(void* self);   // 0x4c4dc0

// ---- 0x4d9e80: the object whose teardown calls 0x4c54f0 on its +0x78 member and then the editor selection handler 0x4d4b80
struct TearObj {
    char pad0[0x78];
    void* f78;                                  // +0x78
    void m4c54f0(void*);                        // 0x4c54f0
    void m4d4b80();                             // 0x4d4b80
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
    loading_config();                                           // (other object)
    ~loading_config();                                          // (other object)
    loading_config& operator=(const loading_config& src);     // 0x4da410
};
loading_config* copy_loading_config(loading_config* self, const loading_config* o);   // forces the implicit copy constructor 0x4dc030
loading_config* copy_configs_forward(loading_config* first, loading_config* last, loading_config* out);    // 0x4da880
loading_config* copy_configs_backward(loading_config* first, loading_config* last, loading_config* out);   // 0x4da8c0
void destroy_range_cfg(loading_config* first, loading_config* last, const loading_config* src);            // 0x4dc190

// ---- 0x4da840..0x4da9c0: element types of the global arrays the shutdown routine tears down ------------------------
// RecA is deleted through its vector deleting destructor 0x431960 (flags 3)
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
    void ensure_local();                        // 0x412ee0
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
struct LoadingScreen {
    char pad0[0x2c];
    ContainerBase* overlay;                     // +0x2c the window (vtable 0x592848)
    MissionUi* hint;                            // +0x30 the hint line (make_hint)
    MissionUi* text;                            // +0x34 the random "ui_hint" text line (flag 0x100)
    MissionUi* logo;                            // +0x38 the "warrider_logo" picture
    int f3c;                                    // +0x3c
    int f40;                                    // +0x40
    void init();                                // 0x4daa10
};
extern int g_hint_count;                        // 0xa18140 (number of "hint_N" strings; 0 = none)

// ---- widgets: this module's call sites go through vt() (../config/widget_mgr.h): slot n of the widget vtable is dNN at +4n ----
// The slots and the call sites that name them (offset: role: callers in this module):
//   +0x0c d03 rebuild: 0x4dc630   +0x2c d11 select(index): 0x4e05c0   +0x58 d22 position pair   +0x64 d25 scale pair
//   +0x6c d27 size pair   +0x70 d28 text offset pair   +0x78 d30 text flags: 0x4dc630   +0x7c d31 install mesh(kind): 0x4dada0
//   +0x80 d32 install mesh(slot)   +0x98 d38 colour   +0x9c d39 highlight colour   +0xa4 d41 set text (out, text)
//   +0xac d43 set text (text, pos): 0x4db090   +0xb8 d46 hide: 0x4dcef0   +0xc8 d50 add child
// The overlay window (vtable 0x592848, 0x190 bytes): the base constructor 0x4931a0 (ContainerBase), then the vptr and the byte at +0x188.
void __fastcall ctor_4931a0(void* p);           // 0x4931a0 (ecx = object)
#define NEW_OVERLAY_WINDOW(var) \
    ContainerBase* var; \
    { ContainerBase* np_ = (ContainerBase*)operator new(0x190); \
      var = np_ != 0 ? (ctor_4931a0(np_), *(void**)np_ = (void*)0x592848, np_->f188 = 0, np_) : 0; }
// the resource manager's get_object (0x419540) for tstring keys, and the UI globals
inline gui_mesh* res_get(const tstring& name) { return (gui_mesh*)g_resmgr_full.get_object((const rgl::string*)&name); }
extern gui_manager g_gui;                       // 0x5d7080
extern unsigned char g_ui_changed;              // 0x5d8788 (set to 1 whenever a window was installed)

// ---- 0x4dada0..0x4db660: the campaign diary screens (mission window, meeting window, "done" window) --------------------
// The screen manager (`this` of 0x4db020..0x4db660, 0x4de1b0..): one of the screen objects, a window pointer table.
struct Screen {
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
    void init_mission_window();                 // 0x4dae40
    void init_meeting_window();                 // 0x4db1f0
    void fill_mission_row(int side);            // 0x4db440
    void init_done_window();                    // 0x4db660
    void dynamic_pass();                        // 0x4d7e50
};
// the hint bar / label maker (0x4dada0) and the 0x4db810 button maker
MissionUi* __cdecl make_hint(ContainerBase* o, const tstring& text, float x, float y, float w, float h);   // 0x4dada0
UiAInline* __cdecl new_button(ContainerBase* o, const tstring& text, float px, float py, float sx, float sy);   // 0x4db810

// globals of the campaign diary screens (addresses; names provisional)
extern GameData* g_game;                        // 0x5e2930
extern void* g_mission_obj;                     // 0x5e292c (the mission object; +0x5df0 = the selected mission)
extern void* g_5e2a00;                          // 0x5e2a00 (mission table, 0x58-byte records; +0x24 = name)
extern int g_a73314;                            // 0xa73314 (mission stage)
extern int g_a73430;                            // 0xa73430 (grid rows)
extern int g_a73428;                            // 0xa73428 (grid columns)
extern int g_a7342c;                            // 0xa7342c (grid depth)
extern int g_a73348[];                          // 0xa73348 (grid of unit indices)
extern int g_a73438;                            // 0xa73438 (selected index, -1 = none)
extern int g_a73300[];                          // 0xa73300 (selection table)
extern int g_a732fc[];                          // 0xa732fc (per-side fallback ids)
extern char g_a181b8;                           // 0xa181b8 (hide-names flag)
extern int g_skill_clamp;                       // 0x8ee3ac4
extern float g_5a9f28[];                        // 0x5a9f28 {0.0, 0.2145}
void __fastcall stop_sounds_494310(void* p);    // 0x494310 (ecx = 0x5aa128: the main mission UI)

// ---- 0x4dc630..0x4dce70: the message (result) window and its button makers ----------------------------------------------
// the option table (0xa44240): ini_file, entries of 0x2c bytes with the value string at +0x1c of each entry (from +0x1c: 0xa4425c)
extern ini_file g_ini;                          // 0xa44240
extern float g_549cdc, g_590b6c, g_54b3b0, g_549d70, g_549d68;   // button metrics (rdata floats)
extern MissionUi* g_ui_active_edit;             // 0x5d7090 (the widget that has the keyboard focus)
extern unsigned char g_msg_flag;                // 0x5d75e0
struct ResultWindow {
    char pad0[4];
    int outcome;                                // +0x04
    char pad8[0x1c - 8];
    tstring s1c;                                // +0x1c
    ContainerBase* win;                         // +0x2c
    MissionUi* panel30;                         // +0x30
    MissionUi* panel34;                         // +0x34
    UiE* box38;                                 // +0x38
    UiAInline* b3c;                             // +0x3c
    UiAInline* b40;                             // +0x40
    char pad44[0x54 - 0x44];
    tstring s54;                                // +0x54
    unsigned char f64;                          // +0x64
    char pad65[3];
    tstring s68;                                // +0x68
    tstring s78;                                // +0x78
    tstring s88;                                // +0x88
    void init();                                // 0x4dc630
};
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
extern MenuKindRec* g_menu_kinds;               // 0xa75f2c
extern int g_menu_kind_count;                   // 0xa75f38
// the menu block: a widget (vtable 0x5924a0, 0x188 bytes) that owns a vector (alloc +0x178, first +0x17c, last, end). The UiFrame of
// ../party/party_window.h (destroying destructor 0x5021f0); that header conflicts with game_data.h on g_game, so this is its view here.
struct UiFrame : MissionUi {
    int f178;                                   // +0x178 (the vector's allocator word)
    int f17c, f180, f184;                       // +0x17c (0x4dd920 clears them)
};
struct MissionUiCtor { void ctor(); };          // 0x48ae20 (MissionUi's constructor on raw memory)
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

struct MainMenu {
    char pad0[0x2c];
    ContainerBase* win;                         // +0x2c
    BlockOwner block;                           // +0x30
    MissionUi* row_tutorial;                    // +0x60
    MissionUi* row_new_game;                    // +0x64
    MissionUi* row_restore;                     // +0x68
    MissionUi* row_custom;                      // +0x6c
    MissionUi* row_options;                     // +0x70
    MissionUi* row_credits;                     // +0x74
    MissionUi* row_exit;                        // +0x78
    MissionUi* version;                         // +0x7c
    MissionUi* logo;                            // +0x80
    MissionUi* pic;                             // +0x84
    int first_free;                             // +0x88
    void create();                              // 0x4dcef0
};

struct EscapeMenu {
    char pad0[0x2c];
    ContainerBase* win;                         // +0x2c
    BlockOwner block;                           // +0x30
    MissionUi* row0;                            // +0x60
    MissionUi* row1;                            // +0x64
    MissionUi* row2;                            // +0x68
    MissionUi* row3;                            // +0x6c
    MissionUi* row4;                            // +0x70
    MissionUi* row5;                            // +0x74
    MissionUi* logo;                            // +0x78
    char pad7c[0x88 - 0x7c];
    int count;                                  // +0x88
    void create();                              // 0x4dda40
};

// the object built by 0x4ddf80: a message window with up to six longer buttons
struct MessageMenu {
    char pad0[0x1c];
    tstring text;                               // +0x1c
    ContainerBase* win;                         // +0x2c
    char pad30[0x34 - 0x30];
    MissionUi* image;                           // +0x34
    MissionUi* rows[6];                         // +0x38
    char pad50[0x54 - 0x50];
    int count;                                  // +0x54
    char f58;                                   // +0x58
    char pad59[0x6c - 0x59];
    tstring texts[1];                           // +0x6c
    void create_message();                      // 0x4ddf80
};

struct DebriefMenu {
    char pad0[0x10];
    int f10;                                    // +0x10
    char pad14[0x2c - 0x14];
    ContainerBase* win;                         // +0x2c
    MissionUi* f30;                             // +0x30
    MissionUi* f34;                             // +0x34
    void create_report();                       // 0x4de1b0
    void create_debrief();                      // 0x4de5b0
};

// globals of the menus
extern int g_window_flags;                      // 0xa73a10 (set to 0x400000)
extern int g_tutorial_enabled;                  // 0xa181a4
extern int g_menu_font_red;                     // 0xa181bc
extern int g_menu_font_green;                   // 0xa181c0
extern int g_menu_font_blue;                    // 0xa181c4
extern int g_main_menu_kind;                    // 0xa75f34
extern char* g_save_prefix;                     // 0x8ee3c3c (the c_str idiom falls back to its own address)
extern BlockOwner* g_active_menu;               // 0x5e299c
extern float g_rand_range;                      // 0x54a6bc
extern float g_pic_split1;                      // 0x54b868
extern float g_pic_split2;                      // 0x549d70
extern float g_pic_split3;                      // 0x54b8ac
extern char g_key_return[];                     // 0x59736c "ui_return_to_game"
extern char g_key_options[];                    // 0x597234 "ui_options"
extern char g_key_save_exit[];                  // 0x597358 "ui_save_and_exit"
extern char g_key_save[];                       // 0x597350 "ui_save"
extern char g_key_save_as[];                    // 0x597344 "ui_save_as"
extern char g_key_quit[];                       // 0x59732c "ui_quit_without_saving"
extern char g_img_logo[];                       // 0x597020 "warrider_logo"
extern char g_img_warrior[];                    // 0x597318 "pic_mb_warrior_1"
extern char g_img_warrior2[];                   // 0x597304
extern char g_img_warrior3[];                   // 0x5972f0
extern char g_img_warrior4[];                   // 0x5972dc
extern char g_img_load[];                       // 0x597030 "load_window"
extern char g_img_message[];                    // 0x5971b0 "message_window"
extern char g_key_button_down[];                // 0x5971d0 "longer_button_down"
extern char g_key_button[];                     // 0x5971c0 "longer_button"

// 0x4de3e0: builds a textured quad (four points, four vertices, two faces) in a mesh; a member of the mesh class
struct PanelMesh : rgl::mesh {
    void build_panel(const float* base, const float* size, const float* uv0, const float* uv1, int* col);   // 0x4de3e0
};
rgl::mesh* make_background(float* v, int color);   // 0x4de320

// ---- 0x4dfd50 / 0x4dfec0: the horizontal slider and the drop-down button makers ----------------------------------------------
UiB* make_slider(MissionUi* parent, float x, float y, float k);                          // 0x4dfd50
DropDown* make_dropdown(MissionUi* parent, const tstring* name, float x, float y);       // 0x4dfec0

// ---- 0x4de6f0: the main-menu options screen (the screen object; fields are widget pointers) ------------------------------------
struct OptionsScreen {
    char pad0[0x2c];
    ContainerBase* root;                        // +0x2c options_window container
    MissionUi* header;                          // +0x30 main_options container
    MissionUi* s34;                             // +0x34
    MissionUi* b38;                             // +0x38 ui_controls
    MissionUi* b3c;                             // +0x3c ui_video_options
    MissionUi* b40;                             // +0x40 ui_done
    MissionUi* box44;                           // +0x44
    MissionUi* box48;                           // +0x48
    MissionUi* box4c;                           // +0x4c
    MissionUi* box50;                           // +0x50
    MissionUi* box54;                           // +0x54
    MissionUi* box58;                           // +0x58
    MissionUi* l5c, *l60, *l64, *l68, *l6c, *l70, *l74;   // +0x5c..+0x74
    MissionUi* d78;                             // +0x78 framerate slider
    MissionUi* s7c, *s80, *s84, *t88;           // +0x7c..+0x88
    MissionUi* s8c, *s90, *s94, *s98, *s9c, *sa0, *sa4, *sa8, *sac, *sb0, *sb4, *sb8, *sbc, *sc0, *sc4, *sc8, *scc, *sd0, *sd4;   // +0x8c..+0xd4
    void create();                              // 0x4d6300
    void open();                                // 0x4de6f0
};

// ---- 0x4e05c0: the options screen ("Win" of the slice) ---------------------------------------------------------------------------------
// a display mode of the mode list (0x4cf100): "WxH ... 16 bits / 32 bits"
struct DisplayMode { tstring name; int w, h, bpp; };
void list_modes(std::vector<DisplayMode>* out);   // 0x4cf100
struct OptionsWin {
    char pad0[0x2c];
    ContainerBase* window;                      // +0x2c
    MissionUi* done;                            // +0x30
    UiB* ui_scale;                              // +0x34
    UiB* y_scale;                               // +0x38
    UiB* z_scale;                               // +0x3c
    UiB* days;                                  // +0x40
    UiB* ragdoll_limit;                         // +0x44
    DropDown* corpses;                          // +0x48
    DropDown* ragdolls;                         // +0x4c
    DropDown* blood;                            // +0x50
    DropDown* plant_shadows;                    // +0x54
    DropDown* resolution;                       // +0x58
    DropDown* shadow_quality;                   // +0x5c
    MissionUi* change;                          // +0x60
    UiAInline* quality[6];                      // +0x64..0x78
    MissionUi* texture_quality;                 // +0x7c
    MissionUi* shadow_quality_label;            // +0x80
    MissionUi* texture_detail;                  // +0x84
    MissionUi* vertex_shaders;                  // +0x88
    MissionUi* refresh_rate;                    // +0x8c
    MissionUi* screen_size;                     // +0x90
    MissionUi* gamma;                           // +0x94
    MissionUi* anisotropic_filtering;           // +0x98
    MissionUi* grass_density;                   // +0x9c
    MissionUi* ragdoll_quality;                 // +0xa0
    MissionUi* character_detail;                // +0xa4
    MissionUi* char_detail_low;                 // +0xa8
    MissionUi* char_detail_high;                // +0xac
    MissionUi* char_shadow_detail;              // +0xb0
    MissionUi* char_shadow_low;                 // +0xb4
    MissionUi* char_shadow_high;                // +0xb8
    MissionUi* dynamic_lighting;                // +0xbc
    MissionUi* character_shadows;               // +0xc0
    MissionUi* environment_shadows;             // +0xc4
    MissionUi* realistic_shadows;               // +0xc8
    MissionUi* particle_systems;                // +0xcc
    MissionUi* antialiasing;                    // +0xd0
    MissionUi* fast_water;                      // +0xd4
    MissionUi* corpses_list;                    // +0xd8
    MissionUi* ragdolls_list;                   // +0xdc
    MissionUi* blood_list;                      // +0xe0
    MissionUi* quality_label;                   // +0xe4
    int day_saved;                              // +0xe8
    char f_ec;                                  // +0xec
    char f_ed;                                  // +0xed
    void init();                                // 0x4e05c0
};
extern int g_day;                   // 0x5a9f9c
extern int g_res_w;                 // 0x5a9fe4
extern int g_res_h;                 // 0x5a9fe8
extern int g_bpp;                   // 0x5a9fec
extern int g_shadow_q;              // 0x5a9fc4
extern int g_dx9;                   // 0x5a9fcc
extern int g_corpses;               // 0x5aa004
extern int g_ragdolls;              // 0x5aa008
extern int g_blood;                 // 0x5aa00c
extern int g_char_detail;           // 0x5aa024
extern int g_char_shadow;           // 0x5aa028
extern int g_ragdoll_q;             // 0x5aa030
extern int g_plant_shadow;          // 0x5aa034
extern int g_ragdoll_lim;           // 0x5aa03c
extern int g_particle;              // 0x5aa040
extern float g_scale_x;             // 0x5aa088
extern float g_scale_y;             // 0x5aa08c
extern float g_scale_z;             // 0x5aa090
extern int g_dinput_enabled;        // 0x5e28fc
extern const float g_row_step;      // 0x597944 = 0.00528f

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
    void read_keys();                           // 0x4d7120 (thiscall)
    void read_key_name(int i, tstring* out);    // 0x4d7370 (thiscall, ret 8)
    tstring* key_text(tstring* out, int i);     // 0x4e2ce0 (ret 8)
    tstring* slot_text(tstring* out, int i);    // 0x4e2ea0 (ret 8)
};
extern OptionsMenu g_menu;                      // 0xa73730

#endif // GAME_CLS_MAP_CLS_MAP_H
