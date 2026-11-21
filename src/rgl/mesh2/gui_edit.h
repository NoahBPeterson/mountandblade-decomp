// gui_edit.h: classes of gui_edit.cpp (0x4c48d0..0x4ca600, TU map 102 "gui_manager_2" + 103 "item_page", merged):
// the record-list loader, the head objects, the std::list<int> / fill helpers, the editor's item records, the
// map editor dialogs and their record accessors, the mesh block reader, the layout/tableau group and the
// language-file exporter. The edited polygon mesh is PolyMesh (game/terrain_b/poly_mesh.h), the head records are head_rec.h.
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
#include "../xmlcfg/records.h"            // RecK (vector delete 0x4318c0), ~SimpleTriggerVec 0x4317b0
#include "./head_rec.h"
#include "../../game/terrain_b/poly_mesh.h"   // PolyMesh: the map editor's polygon mesh, whose members 0x4c53f0..0x4cb940 this module defines
#include "./screen_cfg.h"

using rgl::vector4;

// ---- engine entry points the slices declared (see mesh2_common.h for the allocator) -----------------------------
void __stdcall mb_error_fmt(const char* fmt, ...);

// ============================================================================================================
// 0x4c48d0: the 0x34-byte record list loader
// ============================================================================================================
#include "../../include/rec34.h"      // rec34 (shared with game/config/config.h; the loader 0x4be470 is rec34::read)


// ============================================================================================================
// 0x4c4d10..0x4c5074: the Tail2 record, std::list<int> helpers, the world object destructor, element deleters
// ============================================================================================================

#include "../../include/tail2.h"      // Tail2 (shared with rgl/xmlcfg/records.h)
#include "../../game/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)

#include "../../include/list_int.h"   // list_node_int, list_int (clear 0x4c4d80)

struct block_copier {
    void construct(void* dst, const void* src);   // 0x4c1b20 (this in ecx): 0x2c-byte record copy
};


// ============================================================================================================
// 0x4c5080..0x4c5150: the editor's item records
// ============================================================================================================
#include "../../include/crt.h"   // canonical: operator delete (0x535712)
#include "../../game/game_globals.h"
#include "../../game/string_table.h"
#include "../../include/item_mod.h"
#include "../../game/skill_cap.h"

struct item38 { char d[0x38]; };               // 0x38-byte record (its copy constructor is ArrSet's 0x4636d0, so the layout is ArrSet's: tstring at +0, five int arrays at +0x24)
#include "../../game/arr_set.h"   // ArrSet, allocArr (0x46b6c0: game/terrain_a/terrain_a.cpp, thiscall ret 8; the allocator in ecx is unused)

// The vertex-editor object: a tstring at +0x18, a cleared vector at +0x38, then two 0x4c-byte items
// (tstring at item+0x18, vector at item+0x38) at +0x88 and +0xd4.
struct editor {
    char d0[0x18];                              // +0
    tstring name;                               // +0x18
    char d28[0x10];                             // +0x28
    void** v38;                                 // +0x38 vector<int*> (allocator)
    void** v3c;                                 // +0x3c first
    void** v40;                                 // +0x40 last
    char d44[0x44];                             // +0x44
    char items[2 * 0x4c];                       // +0x88, +0xd4
};

// ============================================================================================================
// 0x4c5190..0x4c5330: view state and child dispatch of the UI root (gui_manager::set_view / wheel_step /
// clear_selection, src/game/strat/gui_manager.h)
// ============================================================================================================
extern const float g_f95;                        // 0x54b890 = 0.95
extern const float g_f05;                        // 0x54b4d0 = 0.05


// ============================================================================================================
// 0x4c6080..0x4c6860: the translated-name accessors of the record classes, and the reference-object page
// ============================================================================================================
// String constants of 0x4c6330..0x4c6620 ("{***}" is the placeholder appended to an untranslated name)

// ---- the record classes of the language export (save_language_files 0x4c8ca0) -----------------------------
// Every record class declares its own translated() (TroopRec, ItemKind, QuestRec, Party, FactionRec, menu_item, item_mod
// ...); gui_edit.cpp defines them, the linker folded the identical bodies.
#include "../../game/ptype.h"             // ptype (the 0x20-byte ground spec), g_ptypes 0x5e28f8, g_ptype_count 0x5e29e8

#include "../../include/item_kind.h"       // ItemKind (name / plural / key at +0 / +0x10 / +0x20), g_items 0x5e29d0
#include "../../include/item_mod.h"        // item_mod, g_item_mods 0xa75f70
inline const tstring* ItemKind::plural_name()
{
    const tstring* r;
    {
        tstring prefix("_pl");
        r = g_strtable.find(&(key + prefix));
    }
    return r ? r : &plural;
}
extern int g_item_kind_count;                           // 0x5e29d4

// (Skin::place 0x4c6b30 reads the head lod mesh's first mesh: rgl::lod_mesh / rgl::mesh, ../mesh.h)
#include "../mesh.h"

// the map editor labels its objects with PlacedObj::format 0x4c6620 (src/game/placed_obj.h)

#include "../../include/game_recs.h"       // PartyTemplate (translated 0x4c6080), QuestRec, FactionRec: the shared record classes
#include "../../include/dialog_rec.h"      // DialogRec (translated 0x4c6080): the shared dialog record
#include "../../include/skin.h"                  // Skin, FaceKey, g_anim.skin_vec._Myfirst (shared with src/game/char_sheet)
#include "../../include/skill_rec.h"                         // SkillRec (shared with src/game/char_sheet/cs_detail.h)


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
// numbers, the target. The stride bounds every field but the one place that names one (the string at +0x18); the
// whole layout is src/game/placed_obj.h's PlacedObj (0x1c8: frame at +0x18, scale at +0x158, kind +0x188, the label
// at +0x190, the value +0x1a0, the scene entity +0x1b4), and the 0x204-byte copies keep that header's tail.
struct MapRecord {
    char pad0[0x18];
    tstring mesh_name;                                  // +0x18 (0x4c7290 reads the first 0x40 bytes from here as a matrix: PlacedObj::frame)
    char pad28[0x158 - 0x28];                           // +0x28
    void* view;                                         // +0x158 the map view the name is given through (Target::set_name)
    char pad15c[0x1a0 - 0x15c];                         // +0x15c
    int entry_no;                                       // +0x1a0
    int var_no;                                         // +0x1a4
    char pad1a8[0x1b4 - 0x1a8];                         // +0x1a8
    Target* target;                                     // +0x1b4 the object shown at the record's place
    char pad1b8[0x1c8 - 0x1b8];                         // +0x1b8 (the insert helpers copy 0x204 bytes)
};

// selection list: a COM-style interface, its methods __stdcall with `this` pushed on the stack (0x4c6fd8: push the
// object, call [vtbl+0x10], no ecx argument, callee pops)
struct SelList {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual int __stdcall get_count();                  // [0x10]
    virtual void v14(); virtual void v18(); virtual void v1c(); virtual void v20();
    virtual void v24(); virtual void v28();
    virtual int __stdcall get_sel(int a, int value, int b, int c, int d, int* flag);    // [0x2c]
};

struct ListObj;                                         // the inserted map objects

struct MapDialog;
// (the _Uninit_copy helpers 0x4c3d90 .. 0x4c3ef0, 0x4c1b60 are declared by gui_edit.cpp from rec_tables.h's declarations: their definitions
// have no trailing tag parameter, which MapDialog's callers still push)
#include "../../include/ptr_tag.h"      // ptr_tag: std::_Nonscalar_ptr_iterator_tag (empty, passed by value)
struct tstring_alloc;
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, tstring_alloc& al, ptr_tag tag);       // 0x524cd0 (game/mission5/game_2.h)
void __cdecl uninit_fill_n(void* first, size_t n, const void* src, block_copier* cop, ptr_tag);   // 0x4c5010 (gui_edit.cpp)

struct MapDialog {
    char pad0[0x4];                                     // +0x0
    std::vector<void*> raw;                             // +0x4 (the dialog's raw block: allocator +4, first +8)
    std::vector<ListObj*> objects;                      // +0x14 (the inserted map objects: allocator +0x14, first +0x18)
    char pad24[0xf4 - 0x24];                            // +0x24
    SelList* list;                                      // +0xf4
    char padf8[0x380 - 0xf8];                           // +0xf8
    HWND hwnd;                                          // +0x380
    char pad384[0x59c - 0x384];                         // +0x384
    int mode;                                           // +0x59c
    char pad5a0[0x668c - 0x5a0];                        // +0x5a0
    MapRecord* records;                                 // +0x668c (the map's record array)

    MapRecord* insert_a(MapRecord* arr, int index, int z);       // 0x4c6eb0
    int* insert_b(int* arr, int index, int z);                   // 0x4c6ee0 (scalar elements: the insert helpers run with `this` as the allocator)
    void place_a(MapRecord* arr, int index, int z);              // 0x4c6f10
    void place_b(MapRecord* arr, int index, int z);              // 0x4c6f30
    void place_c(MapRecord* arr, int index, int z);              // 0x4c6f50
    void place_d(MapRecord* arr, int index, int z);              // 0x4c6f70
};


// ============================================================================================================
// 0x4c7290..0x4c7f90: the configuration dialog and the map editor's object dialog
// ============================================================================================================
// the renderer configuration as read from / written to the user's rgl_config.txt
struct RenderConfig {                                   // game/config/config.h `Config`, 0x70 bytes
    int first_time;                                     // +0 (set when the file was missing or fresh)
    char data[0x70 - 4];
};   // the methods are Config's init_defaults 0x4bc020 / load_file 0x4bbf70 and rgl_config::save 0x415fb0 (called through casts in gui_edit.cpp)

extern char g_skin_path[];                              // 0x595e14 (".\Skins\mb_skin_%s.smf")

// (the settings / configuration dialog's members are GameApp's: game/config/core.h init_main_page, refresh_multisample_list)

// the settings snapshot taken by the dialog: 0x230 bytes + 0x20 = a 0x250-byte frame
struct SettingsSnapshot {
    char data[0x230];
    RenderConfig cfg;                                   // 0x230
};

#include "../../game/mission.h"   // canonical: g_mission (0x5e292c)

#include "../../include/vec.h"


#include "../../include/game_data.h"      // GameData (save_language_files 0x4c8ca0), TroopRec, Party, PartyMgr, QuestRec, FactionRec: the shared classes

// (the map editor's object is Mission::edit_obj, an entity (its local frame); the placed records are Mission::props, PlacedObj)


// the localized text of `key` (the table stores a missing key as an empty string)
inline const char* text(const char* key)
{
    tstring k(key);
    return g_strtable.lookup(k)->c_str();
}

// the list/combo messages of the object dialog (0x4c7bf0)
#define WM_USER_LIST 0x150


// ============================================================================================================
// 0x4c7ee0..0x4c83a0: the mesh block reader (0x4c7ee0 is ??_EMissionObj: include/mission_obj.h)
// ============================================================================================================
#include "../resource.h"                                // resmgr g_resmgr (0x5d6fa0), clear_f14, ReleaseFn

#include "../../include/e68.h"                          // E68: the map block (free_materials 0x4c8330, read_terrain 0x4c83a0)
#include "../../include/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)

// The image's `Block` is the item-kind record: its reader 0x4c8110 reads the item_kinds.txt fields in ItemKind's order
// (module_files.cpp calls the same function as ItemKind::read). The name is the original's mangled symbol.
#include "../../include/item_kind.h"                    // ItemKind (0x238): the record Block::read fills
struct Block : ItemKind {
    void read(stream_base* stream, int version);        // 0x4c8110 (ret 8: the file version gates the triggers)
};
void mb_error(const char* fmt, ...);                    // 0x42cc80 (src/game/strat/gui_manager.h)


// ============================================================================================================
// 0x4c84f0..0x4c8c20: the layout/tableau group (code table, layout record destructors, scene builder, loader thread)
// ============================================================================================================
// ---- the tableau-code names (0x5a9c98; three const char* in a row) ------------------------------------
extern const char* g_conv_names[3];                          // 0x5a9c98

// ---- the party template record (PartyTemplate) as its destructors see it -------------------------------------------------------
struct BufTriple {                                        // a vector's _Myfirst / _Mylast / _Myend (the allocator word precedes it)
    char* first;                                          // +0
    char* last;                                           // +4
    char* end;                                            // +8
    void release() { if (first) operator delete(first); first = 0; last = 0; end = 0; }
};
struct Rec0xb0Tail {                                     // the sub-object at +0x10 of the element (inline destructor)
    tstring key;                                         // element+0x10
    char pad20[0x84];
    BufTriple buf;                                       // element+0xa4 owned buffer (operator delete), end, cap
    ~Rec0xb0Tail() { buf.release(); }
};
struct Rec0xb0 {                                         // element of the array destroyed by 0x4c8980: include/game_recs.h's PartyTemplate
    tstring name;                                        // +0
    Rec0xb0Tail tail;                                    // +0x10
};
void __fastcall conv_free_one(Rec0xb0* self);            // 0x4c85e0 (the element's destructor)
struct edit_vert;

// (Terrain::conv_build_scene 0x4c8630 works on rgl::world's height field and ground: NavData, NavCell, face)


#include "../../include/phys_set.h"                      // PhysSet (update 0x472dc0), g_phys 0xa181f8

extern char g_loading;                                       // 0x5a9c38 (loader thread runs while set)
extern HANDLE g_mt_event;                                  // 0x5e28d8
extern CRITICAL_SECTION g_mt_cs;                           // 0x5e28dc
extern float g_paused_time;                                      // 0x5e2a0c (pending physics step)

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


#include "../../include/ptr_vec.h"

// the 0x38-byte record vector the copy helper belongs to
struct item38_vec {
    char pad0[8];
    void copy_range(const item38* first, const item38* last, item38* dst);   // 0x4c88e0
};



// ============================================================================================================
// 0x4c8ca0: GameData::save_language_files and the records it walks
// ============================================================================================================
inline const tstring* QuestRec::text_name()
{
    const tstring* r;
    {
        tstring prefix("_text");
        r = g_strtable.find(&(key + prefix));
    }
    return r ? r : &desc;
}

extern int g_party_template_count;                                         // 0x5e29fc
#include "../../game/menu_rec.h"   // MenuRec* g_menus (0x5e29b0)
extern int g_menu_count;                                                   // 0x5e29b4


// the text log is written through its virtual w_cstr slot (+0x28), as the call sites do
inline stream_base& log_cstr(const char* s) { return ((stream_base&)g_log).w_cstr(s); }

#include "../../game/presentation.h"   // widget_mgr (destructor 0x4c8bf0)
struct TableauTbl;
void __fastcall conv_map_codes(TableauTbl* self); // 0x4c84f0 (TableauTbl::read: maps the three game tableau codes)
#endif
