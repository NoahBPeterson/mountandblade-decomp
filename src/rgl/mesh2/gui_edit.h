// gui_edit.h: classes of gui_edit.cpp (0x4c48d0..0x4ca600, TU map 102 "gui_manager_2" + 103 "item_page", merged):
// the record-list loader, the head objects, the std::list<int> / fill helpers, the editor's item records, the
// map editor dialogs and their record accessors, the mesh block reader, the layout/tableau group and the
// language-file exporter. The mesh graph is graph_obj.h, the head records are head_rec.h.
// Merged from the slices s004c46d0, s004c5080, s004c5a20, s004c6330, s004c6ca0, s004c7290, s004c7bf0, s004c84f0,
// s004c8ca0 and s004ca550 (first function). Names are provisional unless a vtable maps them. Offsets cite the
// function that proves them.
#ifndef MESH2_GUI_EDIT_H
#define MESH2_GUI_EDIT_H
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <vector>
#include <d3d9.h>
#include "../rgl_math.h"
#include "../text_stream.h"
#include "../stream.h"
#include "../../game/tstring.h"
#include "../../game/strat/gui_manager.h"
#include "./mesh2_common.h"
#include "../xmlcfg/records.h"            // RecK (vector delete 0x4318c0), RecGList (clear 0x4317b0)
#include "./head_rec.h"
#include "./graph_obj.h"
#include "./screen_cfg.h"

using rgl::vector4;

// ---- engine entry points the slices declared (see mesh2_common.h for the allocator) -----------------------------
__declspec(noreturn) void __stdcall __CxxThrowException(void* p, void* t);   // 0x5368e6
void __stdcall mb_error_fmt(const char* fmt, ...);

// ============================================================================================================
// 0x4c48d0: the 0x34-byte record list loader
// ============================================================================================================
#include "../../include/rec34.h"      // rec34 (shared with game/config/config.h; the loader 0x4be470 is rec34::read)

struct rec34_list {
    rec34* v;        // +0
    int   count;     // +4
    int   nok;       // +8
    void load(stream_base& s);              // 0x4c48d0 (ret 4: thiscall, the stream on the stack)
};

// ============================================================================================================
// 0x4c4d10..0x4c5074: the Tail2 record, std::list<int> helpers, the world object destructor, element deleters
// ============================================================================================================
#include "../../include/rec_dtors.h"    // RecI (destroy 0x431810 = ??_ERecI deleting destructor), shared with rgl/xmlcfg/records.h

#include "../../include/tail2.h"      // Tail2 (shared with rgl/xmlcfg/records.h)

struct list_node_int {
    list_node_int* next;
    list_node_int* prev;
    int value;
};

struct list_int {
    list_node_int* head;   // +0
    list_node_int* w04;    // +4  sentinel (self-linked when empty)
    int            count;  // +8
    void clear();          // 0x4c4d80
};

// 0x28-byte record carrying a string first.
struct tstr28 {
    tstring s;                 // +0
    char    pad[0x18];
};

struct world_obj {
    char    head[0x18];        // +0
    tstr28  strs[0xf];         // +0x18  0x28 apart, up to +0x270
    char    pad[0x845e784 - 0x270];
    tstring name;              // +0x845e784
    char    pad2[0x845e7bc - 0x845e794];
    void*   blk;               // +0x845e7bc
    void*   blk2;              // +0x845e7c0
    void*   blk3;              // +0x845e7c4
    ~world_obj();              // 0x4c4ee0
};

struct elem60 {
    tstring s0;          // +0
    tstring s10;         // +0x10
    char    pad20[8];
    tstring s28;         // +0x28
    void*   p38;         // +0x38  freed with delete[]
    char    pad3c[8];
    tstring s44;         // +0x44
    char    pad54[4];
    void*   p58;         // +0x58  freed through its deleting destructor (flag 3)
    char    pad5c[4];
};

struct block_copier {
    void construct(void* dst, const void* src);   // 0x4c1b20 (this in ecx): 0x2c-byte record copy
};


// ============================================================================================================
// 0x4c5080..0x4c5150: the editor's item records
// ============================================================================================================
#include "../../include/crt.h"   // canonical: operator delete (0x535712)
struct item38 { char d[0x38]; };               // 0x38-byte record (tstring at +0x24)
struct ArrSet;
void __stdcall construct_arrset(ArrSet* p, const ArrSet& v);   // 0x46b6c0 (game/terrain_a/terrain_a.cpp; the call site loads the unused allocator into ecx)

// The vertex-editor object: a tstring at +0x18, a cleared vector at +0x38, then two 0x4c-byte items
// (tstring at item+0x18, vector at item+0x38) at +0x88 and +0xd4.
struct editor {
    char d0[0x18];
    tstring name;                               // +0x18
    char d28[0x10];
    void** v38;                                 // +0x38 vector<int*> (alloc)
    void** v3c;                                 // +0x3c first
    void** v40;                                 // +0x40 last
    char d44[0x44];
    char items[2 * 0x4c];                       // +0x88, +0xd4

    void clear_vec38();                          // 0x4c5110
    void free_items();                           // 0x4c5150
};

// ============================================================================================================
// 0x4c5190..0x4c5330: view state and child dispatch of the UI root at 0x5d7080. src/game/strat/gui_manager.h
// declares the class but no names for +0x16e8..+0x16f4 (view x/y/scale) and +0x1708; these three methods are
// declared on a layout-only view of it (offsets as in that header: objs +0, f10 +0x10, o568 +0x568, f1709).
// ============================================================================================================
struct gui_child {                               // polymorphic child (deleted through slot 0, events through slot 35)
    virtual ~gui_child();                        // slot 0 (deleting dtor)
    virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10();
    virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19(); virtual void s20();
    virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24(); virtual void s25();
    virtual void s26(); virtual void s27(); virtual void s28(); virtual void s29(); virtual void s30();
    virtual void s31(); virtual void s32(); virtual void s33(); virtual void s34();
    virtual char on_event(unsigned mask);        // slot 35 (0x8c)
};

struct mesh_bnd {};   // the scene node whose min/max corners come from mnode::calc_bounds (0x438d80, thiscall, ret 8)
extern const float g_f95;                        // 0x54b890 = 0.95
extern const float g_f05;                        // 0x54b4d0 = 0.05
extern const float g_f011;                       // 0x595c98 = 0.011
extern const float g_f989;                       // 0x595c94 = 0.989
extern const float g_f739;                       // 0x595c90 = 0.739
extern const float g_f728;                       // 0x595c8c = 0.728

struct gui_view {
    std::vector<gui_child*> kids;   // +0 (begin +4, end +8)
    gui_child* active;       // +0x10
    char d14[0x568 - 0x14];
    mesh_bnd* o568;         // +0x568
    char d56c[0x16e8 - 0x56c];
    float fx;                // +0x16e8 view x (clamped to [0.05, 0.95])
    float fy;                // +0x16ec view y
    float fs;                // +0x16f0 view y scale
    float f16f4;             // +0x16f4
    char d16f8[0x1708 - 0x16f8];
    char f1708;              // +0x1708
    char f1709;              // +0x1709
    char d170a[2];
    int f170c;               // +0x170c

    void set_view(const float* v);               // 0x4c5190
    void wheel_step(int mask);                   // 0x4c52f0
    void clear_selection();                      // 0x4c5330
};


// ============================================================================================================
// 0x4c6080..0x4c6860: the translated-name accessors of the record classes, and the reference-object page
// ============================================================================================================
// String constants of 0x4c6330..0x4c6620 ("{***}" is the placeholder appended to an untranslated name)
extern const char s__pl[];                      // 0x5923b0 "_pl"
extern const char s__text[];                    // 0x5923b4 "_text"
extern const char s__door[];                    // 0x595c9c "{***}"
extern const char s_suffix_door[];              // 0x595d64 "_door"
extern const char s_prefix_desc[];              // 0x595ca4 "_desc"
extern const char s_invalid_item[];             // 0x595cac "INVALID ITEM"
extern const char s_item_name[];                // 0x595cbc "ui_item_itemname"
extern const char s_unknown[];                  // 0x595cd0 "ui_unknown_unknownname"
extern const char s_prop[];                     // 0x595ce8 "ui_prop_propname"
extern const char s_invalid_plant[];            // 0x595cfc "INVALID PLANT"
extern const char s_plant[];                    // 0x595d0c "ui_plant_plantname"
extern const char s_passage[];                  // 0x595d20 "ui_passage_menu_item_passagename"
extern const char s_entry_point[];              // 0x595d44 "ui_entry_point_entrypointname"

struct RefRecord;                                                // below

// ---- the record classes of the language export (save_language_files 0x4c8ca0) -----------------------------
// Every `translated()` of these classes is one of the accessors defined in gui_edit.cpp; the ones with equal
// field layouts share code in the original (RecA = TroopRec/ItemKind/QuestRec, RecB = PartyRec, RecC = FactionRec).
struct GroundSpec {             // 0x20-byte record (array pointer at 0x5e28f8, count at 0x5e29e8)
    char pad0[0x4];
    char* name;                 // +0x4
    char pad8[0x20 - 0x8];
};
extern GroundSpec* g_ptypes;              // 0x5e28f8
extern int g_ptype_count;                 // 0x5e29e8

#include "../../include/item_kind.h"       // ItemKind (name / plural / key at +0 / +0x10 / +0x20), g_items 0x5e29d0
inline const tstring* ItemKind::plural_name() { return suffix_or(key, "_pl", plural); }
extern int g_item_kind_count;                           // 0x5e29d4

// a mesh's keyframe track (a 0x24-byte record's +0x14 points at an array of these)
struct kf_track {               // (not the 12-byte rgl::keyframes of src/include/keyframes.h: times at +8, count at +0xc)
    char pad0[0x8];
    int* times;                 // +0x8
    int count;                  // +0xc
};
struct kf_node98 {              // 0x98-byte record with a keyframe track at +0x84
    char pad0[0x84];
    kf_track kf;                // +0x84
};

struct RefItem {                // 0x24-byte entry of the record array
    tstring name;               // +0
    void* refs;                 // +0x10 (left uninitialised by the array constructor)
    void* kf;                   // +0x14 (an array of keyframes, freed with the item)
    int f18, f1c, f20;          // +0x18, +0x1c, +0x20 (f20 left uninitialised)
    RefItem() { kf = 0; f18 = 0; f1c = 0; }   // the array-new expansion 0x4c6a20: tstring ctor, then these three
    ~RefItem() { ((RecGList*)&kf)->clear(); }   // 0x4317b0: the keyframe track array is a RecGList-style owner
};
struct RefList {                // the object whose record array sits at +0 (fill_records 0x4c6950 / retire 0x4c6900)
    RefItem* records;           // +0
    void retire();              // 0x4c6900
};
struct Pad98 {                  // a 0x98-byte record whose head is a string (see clear)
    tstring name;               // +0
    char pad10[0x98 - 0x10];
};

struct RefRecord {              // the source record: [0x4c] gives two strings, [0x54] two more
    virtual void deleting_dtor(unsigned flags);   // slot 0
    virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48();
    virtual void v4c(void* out);
    virtual void v50();
    virtual void v54(void* out);
};

// Class A: name +0, text +0x10, field +0x20, record key +0x54 (the troop/item/quest records, array at +0x13a00)
struct RecA {
    tstring name;               // +0
    tstring text;               // +0x10
    tstring field;              // +0x20
    char pad30[0x54 - 0x30];
    tstring key;                // +0x54

    tstring translated_plural();                     // 0x4c6330
    tstring translated();                         // 0x4c63f0
    tstring translated_text();                      // 0x4c6470
};

// Class C: a one-byte flag and the string share the +0x10 word (the faction records, array at +0x13a48)
struct RecC {
    tstring name;               // +0
    char flag;                  // +0x10
    tstring field;              // +0x14

    tstring translated();                       // 0x4c6860
};

// Class B: the grid the caller walks (a string at +0xc, a one-byte flag at +0x1c and a second string at +0x20)
struct RecB {
    char pad0[0xc];
    tstring field;              // +0xc
    char flag;                  // +0x1c
    tstring name;               // +0x20

    tstring translated();                    // 0x4c6530
};

// ---- the morph-target layout used by ItemPage::place (0x4c6b30); these mirror rgl::mesh / rgl::keyframes of
// src/rgl/particles/mesh_obj.h (point20 vector at mesh+0x70, keyframes at mesh+0xa4), kept local because that header
// cannot be mixed with the rest of this module.
struct PFrame {                 // 0x20: a morph key frame (vertices at +4, normals at +0x14, std::vector layout)
    int a0;
    vector4* v;
    vector4* vend;
    vector4* vcap;
    char norms[0x10];
};
struct PTrack {                 // keyframes (12 bytes)
    PFrame* frames;
    int* times;
    int count;
    // find is rgl::keyframes::find (0x40f0d0, thiscall, ret 0x10): PTrack is a view of keyframes (frames, times, count)
};
struct PPoint { vector4 a, b; };   // 0x20 (rest, current)
struct PMesh {
    char pad0[0x70];
    int pa0;                    // +0x70 (allocator lane)
    PPoint* pts;                // +0x74
    PPoint* pts_end;            // +0x78
    PPoint* pts_cap;            // +0x7c
    char pad80[0xa4 - 0x80];
    PTrack kf;                  // +0xa4
    unsigned npts() const { return pts ? (unsigned)(pts_end - pts) : 0; }
};
struct POwner { char pad0[0x38]; PMesh** mesh; };       // +0x38
struct PoseRec {                // 0x34
    char pad0[0x20];
    int key;                    // +0x20 key time
    char pad24[0xc];
    vector4* vecs;              // +0x30
};

// the editor page: the message formatter and the buffers it owns
struct ItemPage {
    tstring name;               // +0
    tstring text;               // +0x10
    char pad20[0x38 - 0x20];
    POwner* owner;              // +0x38 (place 0x4c6b30)
    char pad3c[4];
    PoseRec* poses;             // +0x40 (place)
    char pad44[0x188 - 0x44];
    int kind;                   // +0x188
    int index;                  // +0x18c
    tstring field;              // +0x190
    char* text_ptr;             // +0x1a0 (kind 1: the entry-point name)
    char* text_ptr2;            // +0x1a4 (kind 5: the passage name)
    char pad1a8[0x24c - 0x1a8];
    void* extra;                // +0x24c (free_mem'd)
    void* extra_end;            // +0x250
    void* extra_cap;            // +0x254

    void reset();                               // 0x4c65d0
    void format(char* out);                     // 0x4c6620
    void fill_records(RefRecord* rec);          // 0x4c6950 (declared void: the original sets no eax)
    void clear();                               // 0x4c6aa0
    void place(int e);                          // 0x4c6b30
};

struct QuickString { tstring name; tstring key; tstring translated(); };   // translated 0x4c6080
struct GameString { tstring name; tstring key; tstring translated(); };    // translated 0x4c6080
#include "../../include/game_recs.h"       // PartyTemplate (translated 0x4c6080), QuestRec, FactionRec: the shared record classes
#include "../../include/dialog_rec.h"      // DialogRec (translated 0x4c6080): the shared dialog record
#include "../../include/skin_set.h"              // SkinSet, SkinElem, g_skins_begin (shared with src/game/char_sheet)
struct ModRec {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    char pad20[0x28 - 0x20];
    tstring translated();                                               // 0x4c6080
};
#include "../../include/skill_rec.h"                         // SkillRec (shared with src/game/char_sheet/cs_detail.h)
struct MenuDoor {
    tstring name;                                                       // +0x00
    tstring text;                                                       // +0x10
    tstring key;                                                        // +0x20
    char pad30[0x68 - 0x30];
    tstring translated();                                               // 0x4c62b0
    tstring translated_door();                                          // 0x4c6d00
};
struct GameMenu {
    tstring name;                                                       // +0x00
    tstring key;                                                        // +0x10
    char pad20[0x54 - 0x20];
    int n_doors;                                                        // +0x54
    MenuDoor* doors;                                                    // +0x58
    char pad5c[4];
};


// ============================================================================================================
// 0x4c6ca0..0x4c6f90: the map screen (world-map object labels, object insertion, map-menu reset and refresh)
// ============================================================================================================

struct MapObject {                                      // a placed location / entry of the map
    char pad0[0x10];
    tstring name;                                       // +0x10
    tstring mesh;                                       // +0x20
    tstring* mesh_ptr();                                // 0x4c6ca0
};

struct Target {                                         // vtable slot [0x5c] = set_name
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54(); virtual void v58();
    virtual void set_name(tstring* name, void** view);  // [0x5c]
};

// An entry of the map's record array (stride 0x1c8; 0x204 for the insert helpers). 0x4c6dc0 / 0x4c77c0 /
// 0x4c7e20 / 0x4c6620 prove the fields: the mesh name string, the position matrix, the view, the entry and var
// numbers, the target.
struct MapRecord {
    char pad0[0x18];
    tstring mesh_name;                                  // +0x18 (0x4c7290 reads the first 0x40 bytes from here as a matrix)
    char pad28[0x158 - 0x28];
    void* view;                                         // +0x158
    char pad15c[0x1a0 - 0x15c];
    int entry_no;                                       // +0x1a0
    int var_no;                                         // +0x1a4
    char pad1a8[0x1b4 - 0x1a8];
    Target* target;                                     // +0x1b4
    char pad1b8[0x1c8 - 0x1b8];
};

struct SelList {                                        // selection list (vtable slot [0x10] = get_count)
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual int get_count(void* owner);                 // [0x10]
    virtual void v14(); virtual void v18(); virtual void v1c(); virtual void v20();
    virtual void v24(); virtual void v28();
    virtual int get_sel(void* owner, int a, int value, int b, int c, int d, int* flag);  // [0x2c]
};

// objects inserted into the map: vtable + payload
struct ListObj {
    virtual ~ListObj();                                 // scalar deleting destructor at vtable[0]
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
};

struct MapDialog;
// (the _Uninit_copy helpers 0x4c3d90 .. 0x4c3ef0, 0x4c1b60 are declared by gui_edit.cpp from rec_tables.h's declarations: their definitions
// have no trailing tag parameter, which MapDialog's callers still push)
#include "../../include/ptr_tag.h"      // ptr_tag: std::_Nonscalar_ptr_iterator_tag (empty, passed by value)
struct tstring_alloc;
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, tstring_alloc& al, ptr_tag tag);       // 0x524cd0 (game/mission5/game_2.h)

struct MapDialog {
    char pad0[0x8];
    void* raw;                                          // +0x8
    void* raw_end;                                      // +0xc
    void* raw_cap;                                      // +0x10
    char pad14[0x4];                                    // +0x14
    ListObj** list_start;                               // +0x18
    ListObj** list_end;                                 // +0x1c
    ListObj** list_cap;                                 // +0x20
    char pad24[0x2c - 0x24];                            // +0x24
    int f2c;                                            // +0x2c (reset by reset_state)
    char pad30[0xf4 - 0x30];                            // +0x30
    SelList* list;                                      // +0xf4
    char padf8[0x380 - 0xf8];                           // +0xf8
    HWND hwnd;                                          // +0x380
    char pad384[0x59c - 0x384];                         // +0x384
    int mode;                                           // +0x59c
    char pad5a0[0x668c - 0x5a0];                        // +0x5a0
    MapRecord* records;                                 // +0x668c (the map's record array)

    void reset_state();                                 // 0x4c6e10
    void clear_objects();                               // 0x4c6e30
    void set_object_target(int n);                      // 0x4c6dc0
    MapRecord* insert_a(MapRecord* arr, int index, int z);       // 0x4c6eb0
    int* insert_b(int* arr, int index, int z);                   // 0x4c6ee0
    void place_a(MapRecord* arr, int index, int z);              // 0x4c6f10
    void place_b(MapRecord* arr, int index, int z);              // 0x4c6f30
    void place_c(MapRecord* arr, int index, int z);              // 0x4c6f50
    void place_d(MapRecord* arr, int index, int z);              // 0x4c6f70
    void refresh_list(char use_sel);                             // 0x4c6f90
};

extern int g_dlg_display_bits;                                   // 0x5aa0fc (selection of the map dialog)
extern int g_dlg_multisample;                                 // 0x5aa100

// ============================================================================================================
// 0x4c7290..0x4c7f90: the configuration dialog and the map editor's object dialog
// ============================================================================================================
// the renderer configuration as read from / written to the user's rgl_config.txt
struct RenderConfig {                                   // game/config/config.h `Config`, 0x70 bytes
    int first_time;                                     // +0 (set when the file was missing or fresh)
    char data[0x70 - 4];
};   // the methods are Config's init_defaults 0x4bc020 / load_file 0x4bbf70 and rgl_config::save 0x415fb0 (called through casts in gui_edit.cpp)

extern tstring g_skin_name;                             // 0x5fe888 (the skin name; buf at 0x5fe88c)
extern int g_trial_mode;                             // 0x5e29c4 (show the download button)
extern char g_skin_path[];                              // 0x595e14 (".\Skins\mb_skin_%s.smf")
extern char g_rgl_name[];                               // 0x54b048 ("rgl_config.txt")
extern const char g_mode_rb[];                          // 0x54a8fc ("rb")
extern const char g_mode_wb[];                          // 0x54b058 ("wb")

// the settings / configuration dialog
struct ConfigDialog {
    char pad[0x364];                                    // (+0x354 is the main.bmp logo handle)
    HWND hwnd;                                          // 0x364
    char pad368[0x380 - 0x368];
    void init();                                        // 0x4c7290
    // choose_dx_settings (0x4b9b90) is OptionsDlg's (game/config/options_dlg.h): this dialog is the same object
};

// the settings snapshot taken by the dialog: 0x230 bytes + 0x20 = a 0x250-byte frame
struct SettingsSnapshot {
    char data[0x230];
    RenderConfig cfg;                                   // 0x230
};

// a placed object of the map (the pawn the editor moves): its +0x58 is a 4x4 matrix
struct Pawn {
    char pad[0x58];
    float matrix[0x10];                                 // +0x58
};
#include "../../game/mission.h"   // canonical: g_mission (0x5e292c)

#include "../../include/vec.h"

// the map editor's object dialog
namespace rgl { struct mesh; struct mat3; }
struct MapEditorDialog {
    char pad[0x248];
    float cam_x;                                        // 0x248 camera direction (0x5305b0 negates the three into the editor frame)
    float cam_y;                                        // 0x24c
    float cam_z;                                        // 0x250
    char pad254[0x2cc - 0x254];
    int picked;                                         // 0x2cc picked record index (0x5305b0)
    char pad2d0[0x2f8 - 0x2d0];
    char f2f8;                                          // 0x2f8 (0x5305b0: cleared before handle_input, which sets it)
    char f2f9;                                          // 0x2f9 (0x5305b0: selection markers need a rebuild)
    char pad2fa[0x374 - 0x2fa];
    HWND hwnd;                                          // 0x374
    void refresh();                                     // 0x4c77c0
    void update_object_type();                          // 0x4c7bf0
    void update_objects();                              // 0x4c7e20
    // ---- src/game/map_editor (map_editor.cpp, 0x5305b0..0x531590); the argument types are the callers' views ----
    void update();                                      // 0x5305b0
    void add_selected(rgl::mesh* m, const rgl::matrix* frame, void* rec, unsigned color);   // 0x5314e0 (ret 0x10)
    void select_entity(rgl::mesh* m, void* rec, unsigned color);    // 0x4ea0c0 (ret 0xc)
    void apply(bool& moved, rgl::vector4& pos, bool& rotated, rgl::mat3& frame, bool& drag, rgl::vector4& dir, char& flag);   // 0x52fe40 (ret 0x1c)
};

// a game frame's camera: +0x1a098 the edited map-object index, +0x1a09c the eye position, +0x1a0ac the
// camera's z, +0x1a0b8 the view / edit mode, +0x1a1b4 the selected record index
struct Frame {
    char pad[0x1a098];
    int object_index;                                   // 0x1a098
    Vec3 eye;                                           // 0x1a09c
    char pad1a0a8[0x1a0ac - 0x1a0a8];
    float keep;                                         // 0x1a0ac (read by the menu screen)
    char pad1a0b0[0x8];
    int mode;                                           // 0x1a0b8
    int sel;                                            // 0x1a0bc selected list item (0x4c7bf0)
    char pad1a0c0[0x1a1b4 - 0x1a0c0];
    int record;                                         // 0x1a1b4
};
#include "../../include/game_data.h"      // GameData (save_language_files 0x4c8ca0), TroopRec, Party, PartyMgr, QuestRec, FactionRec: the shared classes

struct Map {
    char pad[0x65e4];
    Pawn* object;                                       // +0x65e4
    char pad65e8[0x668c - 0x65e8];
    MapRecord* records;                                 // +0x668c
    MapRecord* records_end;                             // +0x6690
};

extern tstring g_module_path;                           // 0x8ee3c28 (global file-name prefix)
extern char g_ui_play[];                                // 0x595e0c
extern char g_ui_configure[];                           // 0x595dfc
extern char g_ui_check_for_new_version[];               // 0x595de0
extern char g_ui_download_new_settings[];               // 0x595dc4
extern char g_ui_website[];                             // 0x595db8
extern char g_ui_cancel2[];                             // 0x595dac
extern char g_ui_current_setting[];                     // 0x595d98
extern char g_bmp_main[];                               // 0x595d8c ("main.bmp")
extern char g_empty[];                                  // 0x54a480 ("")

// the localized text of `key` (the table stores a missing key as an empty string)
inline const char* text(const char* key)
{
    tstring k(key);
    tstring* r = g_strtable.lookup(k);
    return r->buf ? r->buf : (char*)r + 4;
}

// the list/combo messages of the object dialog (0x4c7bf0)
#define WM_USER_LIST 0x150


// ============================================================================================================
// 0x4c7ee0..0x4c83a0: record array deleter and the mesh block reader
// ============================================================================================================
// the resource manager (global at 0x5d6fa0)
struct res_manager {
    void* get_material(const tstring& name);            // 0x418a70
    void release(void* res);                            // 0x4175f0 (the call shape only: the definition is the stdcall clear_f14, see below)
};
// 0x4175f0 is rgl/mem_stream.cpp's stdcall clear_f14(named_object*), which ignores ecx; the original loads ecx = the manager before the call:
// the call goes to that symbol through the member type of the original's call
struct named_object;
void __stdcall clear_f14(named_object* r);              // 0x4175f0
union ReleaseFn { void (res_manager::*as_member)(void*); void (__stdcall *clear_f14)(named_object*); };
extern res_manager g_res_manager;                       // 0x5d6fa0

// the array's elements are 0x78 bytes: +0x00 the pointer to the element's name, +0x18 the owner, +0x28 the
// cleared sub-object, +0x48/+0x4c/+0x50 three owned pointers, and the element's 16-byte name string sits at
// its very start, 4 bytes *before* the first of those fields
struct RecordArrayHead {
    int count;                                          // -4
};
struct Rec78Buf {                                       // three owned pointers (begin, end, cap)
    char* p;
    char* e;
    char* c;
    void release() { if (p) operator delete(p); p = 0; e = 0; c = 0; }
};
struct Rec78Tail {                                      // the sub-object at +0x44 of the element (inline destructor)
    Rec78Buf buf;                                       // +0 owned buffer (operator delete), end, cap
    char pad0c[4];
    RefRecord* owner;                                   // +0x10 (deleted through slot 0)
    char pad14[0x14];
    char groups[4];                                     // +0x28 (record_array_clear)
    char pad2c[8];
    ~Rec78Tail()
    {
        Rec78Buf* b = &buf;
        if (owner)
            owner->deleting_dtor(1);
        ((RecGList*)groups)->clear();   // 0x4317b0 (thiscall)
        b->release();
    }
};
#include "../../include/rec78.h"                        // Rec78: the 0x78-byte element of the array (shared); its raw tail[] at +0x44 is a Rec78Tail
inline Rec78Tail& rec78_tail(Rec78* r) { return *(Rec78Tail*)r->tail; }

// a block of the map's mesh: name (+0x00), a second name (+0x10), variation count (+0x44), material
// variations (+0x58 array of 0x18-byte entries), the read pose (+0x1c8..), flags (+0x21c)
struct BlockMat { char pad0[0x64]; void* owned; };      // a material entry; +0x64 the owning block (cleared on release)
struct Block {
    int first;                                          // +0x00
    char pad0[0x18];
    float* verts;                                       // +0x1c (0x18-byte vertices)
    char* verts_end;                                    // +0x20
    char* verts_cap;                                    // +0x24
    float terrain[4];                                   // +0x28
    float pad38;                                        // +0x38
    char* attaches;                                     // +0x3c (0x34-byte entries)
    float scale;                                        // +0x40
    int variation_count;                                // +0x44 (an int, read from the stream)
    char* pad48;                                        // +0x48
    char* pad4c;                                        // +0x4c
    char pad50[8];                                      // +0x50 (+0x54: the material vector, mats())
    char* variations;                                   // +0x58 (array of pointers)
    char* variations_end;                               // +0x5c
    char* variations_cap;                               // +0x60
    void* material;                                     // +0x64

    std::vector<BlockMat*>& mats() { return *(std::vector<BlockMat*>*)((char*)this + 0x54); }   // allocator lane +0x54
    void clear();                                       // 0x4c80c0
    void read(stream_base* stream, int version);        // 0x4c8110 (ret 8: the file version gates the triggers)
    void free_materials();                              // 0x4c8330
    void read_terrain(stream_base* stream);             // 0x4c83a0
};
void mb_error(const char* fmt, ...);                    // 0x42cc80 (src/game/strat/gui_manager.h)


// ============================================================================================================
// 0x4c84f0..0x4c8c20: the layout/tableau group (code table, layout record destructors, scene builder, loader thread)
// ============================================================================================================
// ---- the tableau-code names (0x5a9c98; three const char* in a row) ------------------------------------
extern const char* g_conv_names[3];                          // 0x5a9c98
extern const char g_empty_b[];                               // 0x54b118 (an empty string literal)

// ---- the layout record (conv_rec) and its pieces -------------------------------------------------------
// +0x1c is a std::vector-like triple of pointers into an array of 0x68-byte entries.
struct ConvItem16 {                                          // a 0x68-byte entry (tstring first, data at +0x10)
    tstring name;                                            // +0
    char pad10[0x94];
    int a4;                                                  // +0xa4
    int a8;                                                  // +0xa8
    int ac;                                                  // +0xac
};
struct BufTriple {
    char* p;
    char* e;
    char* c;
    void release() { if (p) operator delete(p); p = 0; e = 0; c = 0; }
};
struct Rec0xb0Tail {                                     // the sub-object at +0x10 of the element (inline destructor)
    tstring key;                                         // +0x10
    char pad20[0x84];
    BufTriple buf;                                       // +0xa4 owned buffer (operator delete), end, cap
    ~Rec0xb0Tail() { buf.release(); }
};
struct Rec0xb0 {                                         // element of the array destroyed by 0x4c8980 (destructor = 0x4c85e0)
    tstring name;                                        // +0
    Rec0xb0Tail tail;                                    // +0x10
};
struct ConvOwner {
    char pad0[4];
    ConvItem16* begin;                                       // +4
    ConvItem16* end;                                         // +8
    int count() { return begin ? (int)((char*)end - (char*)begin) / 0x68 : 0; }
};

// 0x2c-byte vertex-editor record of the +0x1c vector (layout from s004c5080).
struct grid_slot_rec;
struct edit_vert;
struct conv_rec {
    char pad0[0x1c];
    edit_vert* begin;                                        // +0x1c  (vector<edit_vert>-style triple)
    edit_vert* end;                                          // +0x20
    char pad24[0xc3b0dc - 0x24];
    void free_materials();                                     // 0x4c8330 (the block's material release)
    void clear_groups();                                     // 0x4317b0 (release of the 16 variation tracks)
};

// 0x24 bytes: a scene-graph vertex.
struct scene_vert {
    vector4 pos;                                             // +0
    int f10;                                                 // +0x10
    char pad14[0x10];
};

// one cell of the tableau grid (conv_build_scene 0x4c8630): the cells of a column are 0x94 bytes apart, the
// columns 0x911c bytes apart, starting at conv_rec+0x288
struct tab_cell {
    vector4 v;                                               // +0 (v.z holds the code stored in the scene cell)
    int f10;                                                 // +0x10
    int vert;                                                // +0x14 (scene vertex index, < 0 = none)
};

// 0x88-byte record of the scene's list at +0x14: n (+0) vertex indices (+4), the scene object at +0x84
struct scene_obj {                                           // polymorphic scene object, vertices (vector4) at +0x60
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void s4(); virtual void s5(); virtual void s6(); virtual void s7();
    virtual void set_frame(const rgl::matrix* m);                 // slot 8 (0x20), ret 4
    char pad4[0x60 - 4];
    vector4 pts[1];                                          // +0x60
};
struct var_rec {
    int n;                                                   // +0
    int idx[32];                                             // +4
    scene_obj* obj;                                          // +0x84
};

struct matrix3 { vector4 s, f, u; };                         // identity3's result (0x30 bytes)
matrix3* __cdecl identity3(matrix3* out);                    // 0x40cd60

// the scene graph global (at +0xc3b0dc of the layout record)
struct world_sub {
    char pad0[4];
    scene_vert* verts;                                       // +4
    char pad8[8];
    std::vector<var_rec> items;                              // +0x10 (0x88-byte records)
};
struct code_tab { int** cols; };                             // cols[x] -> 16-byte cells (one per row)
struct world_root {
    char pad0[0x64];
    code_tab* codes;                                         // +0x64
    char pad68[0x48];
    world_sub* sub;                                          // +0xb0
};


#include "../../include/phys_set.h"                      // PhysSet (update 0x472dc0), g_phys 0xa181f8

extern char g_loading;                                       // 0x5a9c38 (loader thread runs while set)
extern HANDLE g_mt_event;                                  // 0x5e28d8
extern CRITICAL_SECTION g_mt_cs;                           // 0x5e28dc
extern float g_paused_time;                                      // 0x5e2a0c (pending physics step)
extern void* g_presentation;                                 // 0x5e299c

// the +0x2c-byte record of the +0x1c vector
struct edit_vert {
    vector4 pos;                                             // +0
    vector4 nrm;                                             // +0x10
    char used;                                               // +0x20
    char pad21[3];
    int flag;                                                // +0x24
    char pad28[4];
};


struct edit_vec;
struct edit_vec {                                       // the vector<edit_vert> whose _Ufill is 0x4c8810 (this = allocator)
    edit_vert* ufill(edit_vert* dst, int n, const edit_vert* src);   // 0x4c8810 (thiscall, ret 0xc)
};
namespace std {   // 0x4331e0: the forward copy of tstring ranges is the instance std::_Copy_opt<tstring*, tstring*> of rgl/xmlcfg/game_util.cpp
template<> tstring* _Copy_opt<tstring*, tstring*>(tstring*, tstring*, tstring*, _Nonscalar_ptr_iterator_tag);   // 0x4331e0
}
void __fastcall conv_free_one(conv_rec* self);            // 0x4c85e0


// vector<tstring>::erase (0x4c8840): end pointer at +8
struct tstr_iter { tstring* p; };
struct tstr_vec {
    char pad0[8];
    tstring* last;                                           // +8
    tstr_iter erase_range(tstring* first, tstring* last);   // 0x4c8840 (thiscall, ret 0xc, hidden return)
};


#include "../../include/ptr_vec.h"

// the 0x38-byte record vector the copy helper belongs to
struct item38_vec {
    char pad0[8];
    void copy_range(item38* dst, const item38* first, const item38* last);   // 0x4c88e0
};



// ============================================================================================================
// 0x4c8ca0: GameData::save_language_files and the records it walks
// ============================================================================================================
inline const tstring* QuestRec::text_name() { return suffix_or(key, "_text", desc); }

extern PartyTemplate* g_party_templates;                                // 0x5e29f8
extern int g_party_template_count;                                         // 0x5e29fc
extern GameString* g_strings;                                      // 0xa75ea0
extern int g_n_game_strings;                                            // 0xa75ea8
extern QuickString* g_quick_strings;                                    // 0xa75ea4
extern int g_n_quick_strings;                                           // 0xa75eac
extern DialogRec* g_dialogs;                                            // 0x8ed50cc
extern int g_n_dialogs;                                                 // 0x8ed50c8
extern ModRec g_item_modifiers[];                                            // 0xa75f70 (43 x 0x28)
extern ModRec g_item_modifiers[]; // 0xa75f70 (43 x 0x28)
extern SkillRec g_skill_tbl[];                                             // 0x8ee3100 (42 x 0x44)
extern GameMenu* g_menus;                                               // 0x5e29b0
extern int g_menu_count;                                                   // 0x5e29b4
extern char g_show_party_ids_instead_of_names;                                                  // 0xa181b8


// the text log is written through its virtual w_cstr slot (+0x28), as the call sites do
inline stream_base& log_cstr(const char* s) { return ((stream_base&)g_log).w_cstr(s); }

void __fastcall conv_clear(void** self); // 0x4c8bf0
void __fastcall conv_build_scene(conv_rec* self); // 0x4c8630
void __fastcall conv_map_codes(conv_rec* self); // 0x4c84f0
#endif
