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
#include "../xmlcfg/records.h"            // ~SimpleTriggerVec 0x4317b0
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

#include "../../game/glyph_hash.h"   // GlyphList (clear 0x4c4d80, _Buynode 0x4c4dc0 / 0x4c4e50)



// ============================================================================================================
// 0x4c5080..0x4c5150: the editor's item records
// ============================================================================================================
#include "../../include/crt.h"   // canonical: operator delete (0x535712)
#include "../../game/game_globals.h"
#include "../../game/string_table.h"
#include "../../include/item_mod.h"
#include "../../game/skill_cap.h"

#include "../../game/arr_set.h"   // ArrSet (std::vector<ArrSet>'s copy helpers 0x4c5080 / 0x4c88e0 are gui_edit.cpp's)

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
#include "../../include/skin.h"                  // Skin, FaceKey, g_anim.skin_vec (shared with src/game/char_sheet)
#include "../../include/skill_rec.h"                         // SkillRec (shared with src/game/char_sheet/cs_detail.h)


// ============================================================================================================
// 0x4c6ca0..0x4c6f90: the map screen (world-map object labels, object insertion, map-menu reset and refresh)
// ============================================================================================================

// 0x4c6eb0 .. 0x4c6f70 are std::vector<T>::_Ufill / _Ucopy of mesh_2.cpp's vector instantiations (gui_edit.cpp)
#include "../../include/constraint.h"      // Constraint (0x204 bytes)
#include "../../game/glyph_hash.h"         // GlyphIt
#include "../core/bone.h"                  // CapDef (0x2c bytes)


// ============================================================================================================
// 0x4c7290..0x4c7f90: the configuration dialog and the map editor's object dialog
// ============================================================================================================
extern char g_skin_path[];                              // 0x595e14 (".\Skins\mb_skin_%s.smf")

// (the settings / configuration dialog's members are GameApp's: game/config/core.h init_main_page, refresh_multisample_list)


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
#include "../resource.h"                                // resmgr g_resmgr (0x5d6fa0), release 0x4175f0

#include "../../include/e68.h"                          // E68: the map block (free_materials 0x4c8330, read_terrain 0x4c83a0)
#include "../../include/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)

#include "../../include/item_kind.h"                    // ItemKind (0x238): its reader 0x4c8110 is defined in gui_edit.cpp
void mb_error(const char* fmt, ...);                    // 0x42cc80 (src/game/strat/gui_manager.h)


// ============================================================================================================
// 0x4c84f0..0x4c8c20: the layout/tableau group (code table, layout record destructors, scene builder, loader thread)
// ============================================================================================================
// ---- the tableau-code names (0x5a9c98; three const char* in a row) ------------------------------------
extern const char* g_conv_names[3];                          // 0x5a9c98

// ---- the party template table's element destructors (PartyTemplate, include/game_recs.h) ----------------------------------------
void __fastcall conv_free_one(PartyTemplate* self);      // 0x4c85e0 (the element's destructor)

// (Terrain::conv_build_scene 0x4c8630 works on rgl::world's height field and ground: NavData, NavCell, face)


#include "../../include/phys_set.h"                      // PhysSet (update 0x472dc0), g_phys 0xa181f8

extern char g_loading;                                       // 0x5a9c38 (loader thread runs while set)
extern HANDLE g_mt_event;                                  // 0x5e28d8
extern CRITICAL_SECTION g_mt_cs;                           // 0x5e28dc
extern float g_paused_time;                                      // 0x5e2a0c (pending physics step)

namespace std {   // 0x4331e0: the forward copy of tstring ranges is the instance std::_Copy_opt<tstring*, tstring*> of rgl/xmlcfg/game_util.cpp
template<> tstring* _Copy_opt<tstring*, tstring*>(tstring*, tstring*, tstring*, _Nonscalar_ptr_iterator_tag);   // 0x4331e0
}






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
