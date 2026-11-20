// mission_5.h: declarations of src/game/mission5/mission_5.cpp (TU 116 "mission_5", 0x525160..0x52cf60): the classes, globals
// and helpers the functions of the mission screen use. The slices of this stretch each carried their own partial view of the
// same few objects (the game-loop screen, the mission 0x5e292c, the game data 0x5e2930, the agents, the message log, the menu
// widgets); the views are kept as distinct classes with a per-slice suffix (Mission_s261e0 = the layout s005261e0 saw), and
// every global that was declared with a view type is declared once here as a raw pointer / byte array, the slices' names being
// cast macros over it. Merging the views into mission.h / agent_obj.h / game_data.h is open work (see MIGRATION.md).
// tstring, the streams and rgl::vector4 / rgl::matrix are the shared headers' (src/game/tstring.h, src/rgl/stream.h,
// src/rgl/rgl_math.h); offsets in the views cite the function that proves them, in each view's comments.
#ifndef MISSION5_MISSION_5_H
#define MISSION5_MISSION_5_H
#include "../../include/image_addrs.h"
#include "../../include/layout32.h"
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <new>
#include "../tstring.h"
#include "../../include/party.h"      // PartyMgr, Party (the game object's party manager at +0xf8)
#include "../../include/troop.h"      // Troop (the 0x508-byte troop records at GameData +0x13a00)
#include "../../rgl/rgl_math.h"
#include "../../include/timer.h"   // Timer (Mission::time_speed_timer)
#include "../../rgl/stream.h"
#include "../../rgl/mesh.h"
#include "../key_names.h"
using rgl::vector4;
using rgl::matrix;


// ---- the globals the slices saw with their own view types (the canonical declaration; the per-slice views below cast it)
struct Mission; struct GameData; struct ItemKind; struct MissionTemplate;
extern Mission* g_mission;                       // 0x5e292c
extern GameData* g_game;                         // 0x5e2930
extern ItemKind* g_items;                        // 0x5e29d0
extern MissionTemplate* g_mission_templates;     // 0x5e2a00
#include "../../include/dialog_rec.h"      // DialogBlock::play 0x4875e0
#include "../blow.h"                         // Blow::init 0x4353a0
#include "../dialog_mgr.h"           // DialogMgr g_dialogs (0x8ed50c8)

#include "../str_table.h"   // StrTable g_strtable (0x5e2a18): the one definition
#include "../game_keys.h"           // GameKeys g_game_keys (0xa73730)
#include "../slot.h"                        // Slot, g_keyboard (0xa170c8)
#include "../window_mgr.h"   // g_windows (0xa73980): the window manager, slot_as<T>(i) its screens, dq the id stack
#include "../map_flags.h"      // MapFlags, g_map_flags (0x5e291c): the one definition
#include "../inventory_panel/mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): the one definition
#include "../placed_obj.h"
#include "../../include/shape.h"
#include "../std_vec_out_of_line.h"
#include "../../include/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)
#include "../menu_rec.h"
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/render_flags.h"
#include "../config/config_block.h"
#include "../ui_globals.h"      // UI / core globals
#include "../terrain_a/terrain.h"   // Terrain, g_terrain (0xa76630)
struct GameApp;
extern GameApp g_app_obj;   // 0x8ee3c48 (config/core.h)


// ======================================================= slice s00525160: classes, globals, helpers

// ===========================================================================
// mount&blade.exe (MSVC 7.1 / cl 13.10, x86-32) -- frozen batch ds1, slice 263
//
// The game-loop event handler: one big frame function that drives the
// per-frame camera/sky update, the mission "show/hide" party switches, the
// per-frame scene item pass and the main-menu result dispatch.
// ===========================================================================
typedef unsigned int uint;

// ---------------------------------------------------------------------------
// the engine's generic polymorphic object: only the virtual slots this
// function calls are given real signatures (slot = byte offset / 4)
// ---------------------------------------------------------------------------
struct VObj_s25160 {
    virtual void v0();  virtual void v1();  virtual void v2();  virtual void v3();
    virtual void v4();                                          // 0x10
    virtual void v5();  virtual void v6();  virtual void v7();  virtual void v8();
    virtual void v9();  virtual void v10(); virtual void v11(); virtual void v12();
    virtual void v13(); virtual void v14(); virtual void v15(); virtual void v16();
    virtual char v17(int a);                                    // 0x44
    virtual void v18(int a);                                    // 0x48
    virtual void v19(); virtual void v20(); virtual void v21();
    virtual void v22(void* p);                                  // 0x58
    virtual void v23(); virtual void v24(); virtual void v25(); virtual void v26();
    virtual void v27(); virtual void v28(); virtual void v29(); virtual void v30();
    virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36();
    virtual void v37(void* p);                                  // 0x94
    virtual void v38(); virtual void v39(); virtual void v40();
    virtual void v41(void* a, void* b);                         // 0xa4
    virtual void v42(); virtual void v43();
    virtual void v44();                                         // 0xb0
    virtual void v45();                                         // 0xb4
};




// (the handler's `this` is the conversation screen Rec1208, src/game/conv_screen.h)

// ===========================================================================
// ======================================================= slice s005257f0: classes, globals, helpers
// Slice s005257f0: the conversation camera (Rec1208::update_camera 0x5257f0) and its two agent lookups, find_speaker_agent
// (0x526070) and find_listener_agent (0x526130): the used agent whose troop is the dialogue's speaker / listener.
static __forceinline float clamp01(float x)
{
    if (x < 0.0f)
        x = 0.0f;
    else if (x >= 1.0f)
        x = 1.0f;
    return x;
}

// ======================================================= slice s005261e0: classes, globals, helpers
#define g_strtable_s261e0 g_strtable
// Slice s005261e0: the mission screen / game-loop tick (0x5261e0, 6818 bytes, thiscall, ret 4).
//
// The `this` object is the mission HUD/controller: its fields +0x2c8 (mode), +0x2cc (selection),
// +0x2d4 (flag), +0x2d8/+0x2dc, +0x2f1/+0x2f2/+0x2f4 and +0x324 are read/written here; the item source
// object at +0x30 is MissionSource (0x4d5410/0x4dd920, see slice s004d5410).  clobbered globals are the
// mission (0x5e292c), the game data (0x5e2930) and the GUI/message log (0x5d7080).
//
// One frame: lock, key toggles, scene time-scale, renderer "ready" rate, HUD/camera update, the no-camera
// position save, the mission-editor key block (modes 0x15..0x24 and the three group flags), the region
// change/timers (0x29c..0x2c8), the item-use keys (kinds 1/4/5/6/7), the sound-slot ticks, menu actions
// (0xa73980), the conversation auto-close timer, and finally the menu-option / quick-save / dialog handlers.
//
// Offsets were read from this function; names are provisional.  Declarations reuse the verified layouts
// from slices s004d5410 (MissionSource, tstring, MsgLog_s261e0), s004f1390 (MsgLog_s261e0), s0043ac40 (AgentTable_s261e0),
// s004532f0 (GameData_s261e0/Mission_s261e0) and s004d5410 (Widget_s261e0), kept local to this slice.

// -----------------------------------------------------------------------------------------------------
// rgl string table (16-byte tstring: read cursor, buffer, capacity, length); ctor 0x42bbe0, copy
// 0x42c110, dtor 0x42bc40.  c_str() is the inline `buf ? buf : (char*)&buf` seen at 0x526b7f.




// -----------------------------------------------------------------------------------------------------
// message log (global object 0x5d7080) -- see s004f1390 / s004529e0


// -----------------------------------------------------------------------------------------------------
// key tables: two 256-entry int arrays at 0xa170d4 (down) and 0xa174d4 (up); 0x480 apart.





// 2-entry binding arrays of the mission-editor actions

// -----------------------------------------------------------------------------------------------------
// agents / mission / game data






// The dialog manager is a fixed object at 0x8ed50c8 (outside the tool's annotation range, so it is
// named through a literal cast here).




// ---- plain globals ----------------------------------------------------------------------------------
extern unsigned char g_mt_inited;                    // 0x5e2a09
extern CRITICAL_SECTION g_mt_cs;                           // 0x5e28dc
extern HANDLE g_mt_event;                                  // 0x5e28d8
extern int  g_pending_action;                                  // 0x5a9f18
extern int  g_vert_budget;                                  // 0x5a9c34
#include "../config/config_block.h"   // Config g_cfg (0x5a9f98)
#include "../../include/renderer.h"
#define g_clamp_max g_skill_tbl[36].max_rank             // 0x8ee3ac4

#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)



// float constants in .rdata
extern const float g_f549ca8;                           // 0x549ca8 = 0.0f
extern const float g_f549cdc;                           // 0x549cdc = 1.0f
extern const float g_f54a6d0;                           // 0x54a6d0 = 1e-05f
extern const float g_f54b8b0;                           // 0x54b8b0 = 1.5f
extern const float g_f590b38;                           // 0x590b38 = 8.0f

// ---- plain callees ----------------------------------------------------------------------------------
void __cdecl restore_defaults();                        // 0x4d3a50
void __cdecl quick_save_msg();                                // 0x516a70 (quick save)
void quick_save_msg(); // 0x516a70
void __cdecl operator delete(void* p);                        // 0x535712 (operator delete, ??3@YAXPAX@Z)
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f



// ---- the HUD / mission-screen object: the record MissionBlocker (options_panel/mission_item_recs.h; tick is its slot 3) -------
#include "../options_panel/mission_item_recs.h"

// ======================================================= slice s00527c90: classes, globals, helpers
// Slice s00527c90: the scene .sco save path: GameData::save_scene 0x527c90 (open Modules\<mod>\SceneObj\<name>.sco and write the
// scene record), SceneFile::save 0x527da0 (one group of props, then the two embedded streams), PlacedObj::save 0x527ef0,
// Mission::save_scene_data 0x527f90 (the live terrain layers and the nav mesh into the record's streams), TerrainDesc::save
// 0x528000, TerrainDescEntry::save 0x528060 (run-length encoded layer), Mission::release_item 0x528410 (drop a prop's entity)
// and rgl::entity::release_items 0x528470 (mark the entity's attached shapes for deletion).
stream_base& w_matrix(stream_base& s, rgl::matrix* m);     // 0x415ef0 (cdecl)

// ---- world (Mission::scene) ---------------------------------------------------------------------------------
#include "../../rgl/scene/world.h"

// ======================================================= slice s005284a0: classes, globals, helpers
// Slice s005284a0: MissionBlocker::update_view (0x5284a0, 8192 bytes), the mission screen's per-frame view and input update:
// it damps yaw / pitch from the look input, clamps the pitch, the camera distance and the zoom (37 .. 90 / 97 with the weapons
// sheathed), hands the view's forward row to the player's agent, maps the bound keys into the agent's input bits, runs the
// slot cycling keys and the cheat keys (Blow records for the agents under the crosshair).

// ---- globals ----------------------------------------------------------------------------------------
#include "../../include/fx.h"
#include "../../rgl/resource_manager.h"


// ======================================================= slice s00529130: classes, globals, helpers
// Slice s00529130: the mission HUD's per-frame marker update (0x529130), its per-frame tick
// (0x5295a0), the marker enable/disable pass (0x529600) and the party-stack hero reset (0x529670).
// Everything here is a __thiscall member of the HUD object; the marker widgets are reached through
// their vtables (slot 0x58 = set position, 0xb0 = hide, 0xb4 = show).  Names are provisional.

using rgl::vector4;



// ---- agent record (0x5928 bytes; layout from s0043ac40 / s0043bdb0 / s0043b5d0) -------------------------

// ---- mission (the object at 0x5e292c: agent table at +4, current agent at +0x5c) ------------------------


// ---- the HUD object -----------------------------------------------------------------------------------

// ---- externals -----------------------------------------------------------------------------------------



extern const float g_half;                                          // 0x549d70 (0.5f)
extern float g_deg2rad;                                       // 0x590ce4 (0.017453292)
extern float g_0375;                                          // 0x59c310 (0.375f)
extern float g_2p5;                                           // 0x590c4c (2.5f)
extern float g_255;                                           // 0x590cec (255.0f)
extern float g_0;                                             // 0x549ca8 (0.0f)
extern float g_1;                                             // 0x549cdc (1.0f)

struct vec2;
float __cdecl signed_angle(const vec2* a, const vec2* b);                 // 0x4381e0


// ======================================================= slice s00529840: classes, globals, helpers
// Slice s00529840: the mission's end-of-battle bookkeeping: Mission::apply_agent_results 0x529840 (xp to the party stacks, the
// heroes' hit points, the killed / wounded counts), record_killed 0x529b70, remove_from_stack 0x529bc0 (a free __stdcall),
// drop_stack_seed 0x529c30, record_wounded 0x529d00 and sync_scene_props 0x529d80 (the live props back into the scene record).
struct Agent;
__declspec(noinline) void __stdcall remove_from_stack(Agent* a, int killed); // 0x529bc0 (ret 8)

// ======================================================= slice s00529ec0: classes, globals, helpers
// Slice s00529ec0: MissionBlocker::probe_interaction (0x529ec0, 4011 bytes): every frame, what is the player's agent looking at?
//   1. a thin capsule (1e-4 m) from the agent's eye 10.5 m forward against the level's faces: the nearest hit is `best`;
//   2. the probe again as a 0.1 m capsule against the scene entities (mask 9): agents on foot (talk), free horses
//      (mount), scene props (chest / baggage / passage / equipment) and the map edges (leave area / retreat);
//   3. nothing hit: the dismount test and the four mission-border tests on the agent's facing;
//   4. validation (riding skill, distances), then the HUD label ("ui_talk", "ui_mount", ...) and its grey action text;
//   5. {mode, target, valid} into sel_mode / sel_target / sel_valid, which the screen tick 0x5261e0 consumes.

// ======================================================= slice s0052ae70: classes, globals, helpers
// Slice s0052ae70: the mission order markers. Mission::init_order_markers 0x52ae70 builds (once) the two marker entities: the
// nav mesh's polygon / ribbons / spikes meshes and the ground rectangle (four points, four vertices, two faces) on `special`.
// MissionBlocker::order_tick 0x52b300 gives the selected order to the player's team and, when the order menu is up, arms its
// 7.5 s timeout and repaints it; draw_order_menu 0x52b3c0 paints the order labels, the selected one tinted.

// ======================================================= slice s0052b5c0: classes, globals, helpers
// Slice s0052b5c0: the mission order and cursor/terrain helpers.
//   0x52b5c0  Mission::order_agent          -- give an item/order to one of an agent's four order slots
//   0x52ba10  MissionBlocker::update_ui     -- per-frame UI/cursor state machine of the map editor
//   0x52ba1d  interior continuation of 0x52ba10 (EBX/ZF preset) -- translated in full
//   0x52bc80  MissionBlocker::update_cursor -- ray-pick the ground and drive the terrain brush
//   0x52bdb0  MissionBlocker::find_position -- screen ray -> world position through the collision world
// Layouts and callee signatures are the ones verified in the neighbouring slices
// (s0052ae70 Screen, s0052f250 MapEditor, s005191e0 collision shapes, s0052c2b0 brush).

using rgl::vector4;
using rgl::matrix;



// ---- engine string (16 bytes) -------------------------------------------------------------------------




// ---- item kinds / order references --------------------------------------------------------------------



// ---- agents -------------------------------------------------------------------------------------------


// ---- the mission / screen object (0x5e292c) -----------------------------------------------------------
#include "../../rgl/scene/scene_entity.h"









// =====================================================================================================
//  Mission_s2b5c0::order_agent
// =====================================================================================================
// ======================================================= slice s0052bf40: classes, globals, helpers
// Slice s0052bf40: the terrain brush/grid update.
//   0x52bf40  Terrain_s2bf40::update   -- clear the brush grid, drive the brush over the grid cells around
//                                 the cursor (0x52c2b0), regenerate the mode-0/1/2 objects and
//                                 rebuild every layer chunk of the touched cells (0x47ff60).
//   0x52c290  Terrain_s2bf40::clear    -- zero the 0x177-byte brush grid at this+0x845e030.
// Layouts follow the neighbouring slices (s0052c2b0 brush, s0052f250 map editor, s0052b5c0 mission).

using rgl::vector4;
using rgl::matrix;







// =====================================================================================================
//  Terrain_s2bf40::update
// =====================================================================================================
// ======================================================= slice s0052c2b0: classes, globals, helpers

// Slice s0052c2b0: the terrain "brush" of the world-map editor (0x52c2b0, 2887 bytes, __thiscall,
// ret 0x1c = 7 stack args). It maps the brush amount (arg4) through the per-map-mode response curve
// selected by g_game->editor.mode, then either rewrites the height/colour values of one or
// many map tiles (the 24-byte "element" records reached from arg1, whose per-cell value array at +0x14
// is a 0xac440-byte lazily allocated block) or marks the touched grid cells in this[0x845e030].
// Reconstruction of the original control flow (verified against the disassembly); NOT a byte-exact
// match: the original's x87 evaluation order and the compiler's 4x unrolling of the 0.98 scan are
// reproduced only structurally. See nonmatching.txt.

typedef unsigned int uint;

// ---------------------------------------------------------------------------------------------------
// globals (addresses from the annotations in the disassembly)
extern float g_00;                   // 0x549ca8  0.0f
extern float g_098;                  // 0x5911d8  0.98f (scan threshold)
extern double g_001;                 // 0x59c480  unused here (other slices)

#if MB_LP64
void* __cdecl operator new[](size_t size);      // 0x535482  ??_U@YAPAXI@Z operator new[] (port: LP64 spelling)
#else
void* __cdecl operator new[](uint size);      // 0x535482  ??_U@YAPAXI@Z operator new[]
#endif
namespace rgl { struct vector4; unsigned pack_color(const vector4* c); }   // 0x40e440 (rgl/rgl_math.h)

// ---------------------------------------------------------------------------------------------------


// ======================================================= slice s0052ce00: classes, globals, helpers

// Slice s0052ce00: the per-cell value block helpers of the world-map terrain brush.
//   0x52ce00  clear the 0xac440-byte block (0x1a4 x 0x1a4 floats) of one brush element.
//   0x52ce70  lazily allocate that block (filled with -1) and store one cell.
//   0x52cef0  scale a grid coordinate pair into the scene's cell range (divide by the cell size at
//             this+0x843bd70, clamp to [0, size-1] using this+0x845d984 / this+0x845d988).
// The element record is 24 bytes; its block pointer lives at +0x14 and the block is read as float
// cells by the brush (0x52c2b0) and written as int -1 when it is first allocated. Layout and offsets
// are read off this slice's disassembly.

#if MB_LP64
void* __cdecl operator new[](size_t size);              // 0x535482  operator new[] (??_U@YAPAXI@Z, port: LP64 spelling)
#else
void* __cdecl operator new[](unsigned _Size);          // 0x535482  operator new[] (??_U@YAPAXI@Z)
#endif






// ---- sig s1: canonical declarations (minimal) and forwarders from the per-slice view classes to them.
namespace rgl {
vector4 unpack_color(const unsigned* argb);   // 0x40e4e0
}
struct UiD {
    void set_value(float);   // 0x48baa0
};

struct Agent; struct AgentTable; struct AgentX; struct Blow; struct ContainerBase; struct DialogMgr; struct ForceLabel; struct Game; struct GameData; struct ItemRef; struct LangTables; struct MapEditorDialog; struct MapFlags; struct MapObject; struct Mission; struct NamedRec; struct Party; struct Pickable; struct Picker; struct PlacedObj; struct PolyMesh; struct Poser; struct Slot; struct StrTable; struct Timer; struct UiD; struct WindowMgrW; struct camera_ray; struct capsule; struct contact; struct level; namespace rgl { struct matrix; } typedef rgl::matrix pos; struct shape_base; struct skin; struct stream_base; struct tstring; struct vec2;
namespace rgl { struct entity; }

struct ItemRef;
#include "../mission.h"   // the real Agent, AgentTable, Mission (agent_obj.h)
struct AgentX {
    bool f100_is_0x48_and_charged() const;   // 0x432470
    bool is_enemy(Agent const*) const;   // 0x432350
    float speed_scale() const;   // 0x432450
};
struct ForceLabel {
    void make_label(char*);   // 0x48d940
};
struct Game {
    void more_start();   // 0x51f800
};
#include "../terrain_b/scene_file.h"   // SceneFile: GameData::scenes
#include "../../rgl/stream_inline.h"   // the stream constructors / destructors inline (the scene record's embedded streams)
#include "../../include/game_data.h"   // GameData (g_game 0x5e2930): module_path 0x4639d0, skill_level 0x437b20, scene_path 0x463950
#include "../ui/item_ref.h"   // ItemRef, snd_bank, snd_slot
#include "../../include/trigger_rec.h"   // TriggerRec (snd_slot): update 0x4625d0
#include "../trigger_vecs.h"             // TriggerVec (snd_bank): update_all 0x462760
struct MapEditorDialog {
    void refresh();   // 0x4c77c0
    void update();   // 0x5305b0
};
struct MapObject {
    tstring* mesh_ptr();   // 0x4c6ca0
};
#include "../terrain_b/poly_mesh.h"   // PolyMesh (Mission::nav): the preview builders 0x4e65e0 / 0x4e6a70 / 0x4e75a0 (cls_map/progress_bar.cpp)
#include "../../include/timer.h"   // Timer (elapsed 0x432c10)
namespace c49c2c0 {
struct Mission {
    void order(int, int);   // 0x49c2c0
};
}
#include "../map_widget.h"            // MapWidget::screen_to_world 0x4bd2f0








#endif // MISSION5_MISSION_5_H
