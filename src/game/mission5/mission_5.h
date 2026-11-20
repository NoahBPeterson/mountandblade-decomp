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
#include "../prop_kind.h"
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
#define g_game_s257f0 (*(Game_s257f0**)&g_game)
// Slice s005257f0: the dialogue "look at" update of the view object (0x5257f0) and its two agent lookups,
// find_speaker_agent (0x526070) and find_listener_agent (0x526130).  The lookup pair scans the mission's
// agent table for the agent whose troop id (+0x88) equals the current dialogue speaker / listener.
// The big function builds a look-at frame from the target and listener bodies' matrices, blends it into
// this+0x1194, copies it out, hands it to Mission_s257f0::camera_to and advances the blend timer.  Names provisional.
using rgl::vector4;
using rgl::matrix;



struct TroopRec_s257f0 {                                   // 0x508 bytes
    char pad0[0x30];
    unsigned flags;                                 // +0x30 (0x10 = a talking troop?)
    char pad34[0x508 - 0x34];
};
struct Game_s257f0 {
    char pad0[0xc8];
    int player;                                     // +0xc8
    char padcc[0x13a00 - 0xcc];
    TroopRec_s257f0* chars;                                // +0x13a00
};




                          // 0x526130 (defined in slice s00526130)


// ---- the view object (only the members this function touches) --------------------------------------------
struct ViewObj_s257f0 {                                   // the conversation screen Rec1208 (src/game/conv_screen.h)
    char pad0[0x30];
    void* window;                                   // +0x30 (Rec1208::window: its frustum at +0x1ac receives the 0x120 bytes of +0x34)
    char pad34[0xfc - 0x34];                        // +0x34 (Rec1208::fr, a rgl::frustum)
    matrix cam_frame;                               // +0xfc (Rec1208::fr.m: the camera frame, a copy of the look-at frame)
    char pad13c[0x1194 - 0x13c];
    matrix world;                                   // +0x1194 (Rec1208::world: the camera world transform; s, f, u, o)
    float blend;                                    // +0x11d4
    char started;                                   // +0x11d8
    __declspec(noinline) void update();                                  // 0x5257f0
    int find_speaker_agent();                                            // 0x526070 (thiscall: the callers load ecx)
    int find_listener_agent();                                           // 0x526130
};

struct vec3_s257f0 {
    float x, y, z;
    __forceinline void sub_assign(const vec3_s257f0& o);                 // 0x432770
};

static __forceinline float clamp01_s257f0(float x)
{
    if (x < 0.0f)
        x = 0.0f;
    else if (x >= 1.0f)
        x = 1.0f;
    return x;
}

// ======================================================= slice s00526130: classes, globals, helpers
#define g_game_s26130 (*(Game_s26130**)&g_game)
// Slice s00526130: the dialogue-listener agent lookup (0x526130).  It scans the mission's agent table for
// the used agent whose troop id (+0x88) equals DialogMgr_s26130::listener and returns the last match, or -1.
// Names provisional; every callee/global carries its VA.



struct TroopRec_s26130 {                                   // 0x508 bytes
    char pad0[0x30];
    unsigned flags;                                 // +0x30 (0x10 = a talking troop?)
    char pad34[0x508 - 0x34];
};
struct Game_s26130 {
    char pad0[0xc8];
    int player;                                     // +0xc8
    char padcc[0x13a00 - 0xcc];
    TroopRec_s26130* chars;                                // +0x13a00
};




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






// widgets (slice s004d5410 / s00494e40): the screens of the window manager as this slice reads them
struct Widget_s261e0 {
    char pad0[4];
    int  mode;                                          // +4 what the screen opens for (0, 2, 3, 5, the conversation's choice)
    char pad8[0x14 - 8];
    int  troop;                                         // +0x14 the troop the screen shows (the player, the dialog partner)
    int  back_state;                                    // +0x18 the window state to return to (0xe the mission, 0x10 the conversation)
};










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



// the mission template's bit 1 (flags at +0x20; shr; test r8,1)
static inline bool template_bit1_s261e0(int id)
{
    unsigned f = *(unsigned*)((char*)g_mission_templates + id * 0x58 + 0x20);
    return ((f >> 1) & 1) != 0;
}

static __forceinline int* rec_base_s261e0(int id)
{
    return (int*)((char*)g_mission_templates + id * 0x58 + 0x4c);
}

// ---- the HUD / mission-screen object: the record MissionBlocker (options_panel/mission_item_recs.h; tick is its slot 3) -------
#include "../options_panel/mission_item_recs.h"

// vtable of the object at Screen_s261e0+0x5c: only slot 0xc4 (index 49) is called here.
struct SrcVtbl_s261e0 {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void v24(); virtual void v25(); virtual void v26(); virtual void v27();
    virtual void v28(); virtual void v29(); virtual void v30(); virtual void v31();
    virtual void v32(); virtual void v33(); virtual void v34(); virtual void v35();
    virtual void v36(); virtual void v37(); virtual void v38(); virtual void v39();
    virtual void v40(); virtual void v41(); virtual void v42(); virtual void v43();
    virtual void v44(); virtual void v45(); virtual void v46(); virtual void v47();
    virtual void v48(); virtual void v49();
};

// ======================================================= slice s00527c90: classes, globals, helpers
#define g_game_s27c90 (*(GameData_s27c90**)&g_game)
// Slice s00527c90 (batch ds1): the scene/map .sco save path.
//
//   0x527c90  GameData_s27c90::save_scene      -- open Modules\<mod>\SceneObj_s27c90\<name>.sco and serialise the scene record
//   0x527da0  SceneData::save           -- write both object lists of a scene record
//   0x527ef0  SceneObj_s27c90::save            -- write one scene object
//   0x527f90  Mission::save_scene_data        -- reset both in-memory streams and serialise the live terrain layers
//   0x528000  TerrainDesc::save                -- write the layer count, dimensions and every layer
//   0x528060  TerrainDescEntry::save           -- run-length encode one layer
//   0x528410  MissionProps::release_item       -- free the entity behind one per-item slot
//   0x528470  Resource_s27c90::release         -- mark a resource's sub-resources for deletion
//
// Class layouts: tstring / stream_base / bin_writer come from mbbin.h (verified in s00417850..s00418a70);
// the GameData_s27c90 field offsets are those of s005261e0 (0x5e292c / 0x5e2930).  Names provisional.

// ---- engine string helpers -----------------------------------------------------------------------------
// tstring::c_str() is the inlined `buf ? buf : (char*)&buf` seen at 0x527cc1.
struct GameData_s27c90;

// The scene record (0x494 bytes) in the GameData_s27c90 scene table at +0x13a50.
struct SceneObjRec_s27c90;

struct GameData_s27c90 {
    char pad0[0x13a50];
    SceneObjRec_s27c90* levels;                    // +0x13a50 (GameData::levels)
    __forceinline tstring scene_path(int idx);            // 0x463950 (returns by hidden pointer; ret 8)
    __forceinline const char* module_path(const char* sub);   // 0x4639d0 (ret 4)
    __declspec(noinline) void save_scene(int idx);               // 0x527c90 (ret 4)
};




struct Mesh_s27c90;

// Resource_s27c90 with a sub-resource vector at +0x12c and a flag word at +8 (0x528470).
struct Resource_s27c90 {
    char pad0[8];
    unsigned flags;                         // +8
};

struct ResourceOwner_s27c90 {                      // the prop's scene entity (rgl::entity)
    char pad0[8];
    int id;                                        // +8 (rgl::entity::id: world::remove_entity 0x41f0d0 takes it)
    char pad0c[0x12c - 0xc];
    Resource_s27c90** first;                       // +0x12c (rgl::entity::items: the attached entities, first / last)
    Resource_s27c90** last;                        // +0x130
    __declspec(noinline) void release();    // 0x528470
};

struct SceneObj_s27c90;
struct SceneObjRec_s27c90 {
    char pad0[0x30];
    unsigned flags;                         // +0x30 (Level494::flags: 0x100 = an outdoor level with terrain layers)
    char pad34[0x104 - 0x34];
    std::vector<SceneObj_s27c90> objs;      // +0x104 (first at +0x108: 0x1c8-byte scene objects)
    char pad110[0x414 - 0x114];
    mem_stream stream_a;                    // +0x414 (0x30 bytes)
    mem_stream stream_b;                    // +0x444
    char pad474[0x494 - 0x474];
    void save(stream_base* s, int arg);     // 0x527da0 (ret 8)
};

// one 0x1c8-byte scene object
struct SceneObj_s27c90 {
    char pad0[0x10];
    int f10;                                // +0x10
    int f14;                                // +0x14
    char pad18[0x98 - 0x18];
    rgl::matrix frame;                      // +0x98 (PlacedObj::m98: the placement frame, init_placed 0x435860)
    char padd8[0x158 - 0xd8];               // +0xd8 (PlacedObj::md8, m118)
    float f158, f15c, f160;                 // +0x158
    char pad164[0x188 - 0x164];
    int kind;                               // +0x188 (PlacedObj::kind)
    int id;                                 // +0x18c (PlacedObj::id)
    tstring name;                           // +0x190
    int value;                              // +0x1a0 (PlacedObj::value)
    int f1a4;                               // +0x1a4
    char pad1a8[0x1b4 - 0x1a8];
    void* ent;                              // +0x1b4 (PlacedObj::ent: the prop's scene entity)
    char pad1b8[0x1c8 - 0x1b8];
    void save(stream_base* s);              // 0x527ef0 (ret 4)
};

stream_base& w_matrix(stream_base& s, rgl::matrix* m);     // 0x415ef0 (cdecl)

// Mission::release_item (0x528410, thiscall on the mission). Declared on this holder, not on Mission: a further Mission
// member declaration moves terrain_b.cpp's render_batch::render_scene (cl's numbering; nonmatching row, 203 -> 157).
struct MissionProps {
    __declspec(noinline) void release_item(int idx);             // 0x528410 (ret 4: drop the entity of prop idx)
};

// ---- world (Mission::scene) ---------------------------------------------------------------------------------
#include "../../rgl/scene/world.h"

// ======================================================= slice s005284a0: classes, globals, helpers
#define g_items_s284a0 (*(ItemKind_s284a0**)&g_items)
#define g_mission_s284a0 (*(Mission_s284a0**)&g_mission)
#define g_game_s284a0 (*(Game_s284a0**)&g_game)
#define g_key_names_s284a0 (*(KeyNames_s284a0*)&g_keyboard)
// Slice s005284a0: the mission player-view / aim controller's per-frame update (0x5284a0, 8192 bytes).
//
// It damps the view yaw/pitch (+0x268/+0x26c) from the look input, clamps the pitch, the look speed
// (+0x278) and the field of view (+0x288, 37 .. 90/97 depending on the sheathed flag), builds the view
// frame and hands its forward row to the local agent's look direction (+0x55d8), and (while the game is
// not paused) maps the bound keys into the agent's movement / action bits (+0x55d0), runs the slot
// cycling keys, toggles the drawn-weapon state, and turns the three "action" keys into Blow_s284a0 records for
// the agents under the crosshair.  `this` is the mission view object (same object as s004f7f00's
// MissionObj: +0x268 yaw, +0x2c8 mode, +0x2d4 aim target, +0x2fc snap counter); names are provisional.

using rgl::vector4;
using rgl::matrix;

// ---- item records ------------------------------------------------------------------------------------
struct ItemKind_s284a0 {                                       // 0x238 bytes, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned type_flags;                                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x238 - 0x1cc];
};


struct ItemRef; typedef ItemRef ItemRef_s284a0;   // sig s1: canonical ItemRef (src/game/item_ref.h: item, m4)

// ---- the blow record (0x58 bytes, ctor 0x4353a0) -----------------------------------------------------
struct Blow_s284a0 {
    __forceinline Blow_s284a0();                                             // 0x4353a0 (== Blow::init)
    char pad0[4];
    int attacker;                                       // +0x04
    vector4 pos;                                        // +0x08 (position of the agent hit)
    char pad18[0x38 - 0x18];
    int bone;                                           // +0x38 (Blow::bone: hit location / damage class)
    float dmg_in;                                       // +0x3c (Blow::dmg_in)
    char pad40[0x48 - 0x40];
    int kind;                                           // +0x48 (Blow::kind: 0 cut, 1 pierce, 2 blunt)
    char pad4c[0x58 - 0x4c];
};

// ---- the mission / agent objects (layouts reused from s004556d0, s004f7f00) --------------------------
struct Agent_s284a0;
struct AgentTable_s284a0 {
    int count;                                          // +0x00
    char pad4[0x18 - 4];
    Agent_s284a0** pages;                                      // +0x18 (first page pointer; mission+0x1c)
    __forceinline void next_used(unsigned* i);                        // 0x43b160
    __forceinline int first_used();                                   // 0x43b1b0
    __forceinline Agent_s284a0* get(unsigned id);              // 0x43b1f0
};

struct Agent_s284a0 {                                          // 0x5928 bytes
    char pad0[4];
    int used;                                           // +0x04
    char pad8[8];
    int kind;                                           // +0x10 (Agent::kind)
    int state;                                          // +0x14 (Agent::state)
    char pad18[0x2c - 0x18];
    int horse;                                          // +0x2c (Agent::horse: -1 on foot)
    char pad30[0x50 - 0x30];
    vector4 pos;                                        // +0x50
    vector4 vel;                                        // +0x60 (Agent::vel: the move direction the view snaps to)
    char pad70[0x7c - 0x70];
    int f7c;                                            // +0x7c (Agent::f7c: drawn-weapon state)
    char pad80[0x100 - 0x80];
    int anim1_cur;                                      // +0x100 (Agent::anim[1].cur: the upper-body animation)
    char pad104[0x118 - 0x104];
    float anim1_scale;                                  // +0x118 (Agent::anim[1].scale: its progress)
    char pad11c[0x22c - 0x11c];
    int wield[2];                                       // +0x22c (Agent::wield: the wielded item index per hand)
    char pad234[0x2c8 - 0x234];
    int simple_behavior;                                // +0x2c8 (Agent::simple_behavior)
    int f2cc;                                           // +0x2cc
    char pad2d0[0x2d4 - 0x2d0];
    int f2d4;                                           // +0x2d4
    char pad2d8[0x31c - 0x2d8];
    unsigned target;                                    // +0x31c (Agent::target)
    char pad320[0x55d0 - 0x320];
    unsigned in_flags;                                  // +0x55d0 (movement / action bits)
    char f55d4;                                         // +0x55d4
    char pad55d5[0x55d8 - 0x55d5];
    vector4 facing;                                     // +0x55d8 (Agent::fx..fw: the look direction)
    char pad55e8[0x55ec - 0x55e8];
    int snd_a;                                          // +0x55ec (Agent::snd_a)
    char pad55f0[0x560c - 0x55f0];
    int active;                                         // +0x560c (Agent::active)
    int f5610;                                          // +0x5610
    int f5614;                                          // +0x5614 (Agent::f5614: the block animation)
    char pad5618[0x5720 - 0x5618];
    float max_health;                                   // +0x5720 (Agent::max_health)
    float health;                                       // +0x5724 (Agent::health)
    char pad5728[0x5928 - 0x5728];

    __forceinline ItemRef_s284a0 slot_pair(int idx);                         // 0x432790 (ret 8)
    __forceinline bool is_enemy(Agent_s284a0* o);                          // 0x432350 (ret 4)
    __forceinline bool is_friend(Agent_s284a0* o);                         // 0x4323a0 (ret 4)
    __forceinline void note_blow(Blow_s284a0* b);                            // 0x457af0 (ret 4)
    __forceinline void cycle_slot(int slot);                          // 0x44bc30 (ret 4)
    __forceinline void on_wield_reset();                              // 0x445a20
};

struct Mission_s284a0 {
    char pad0[4];
    AgentTable_s284a0 agents;                                  // +0x04
    char pad20[0x5c - 0x20];
    int player_id;                                      // +0x5c (Mission::player_id)
};


struct Game_s284a0 {
    char pad0[0x139e4];
    int f139e4;                                         // +0x139e4 (weapons sheathed)
    char pad139e8[0x1a090 - 0x139e8];
    int map_editor;                                     // +0x1a090 (GameData::f1a090: map editor active)
};


// ---- the view object (`this`) -----------------------------------------------------------------------
struct ViewCtrl_s284a0 {
    char pad0[0x44];
    unsigned char flags44;                              // +0x44 (bit 0: keyboard/analog look input)
    char pad45[0x268 - 0x45];
    float yaw;                                          // +0x268
    float pitch;                                        // +0x26c
    float yaw_rate;                                     // +0x270
    float pitch_rate;                                   // +0x274
    float look_speed;                                   // +0x278 (0.7 .. 10)
    char pad27c[0x288 - 0x27c];
    float fov;                                          // +0x288 (37 .. 90 / 97)
    char pad28c[0x2c8 - 0x28c];
    int mode;                                           // +0x2c8 (aim mode)
    int target;                                         // +0x2cc (aim target)
    char pad2d0[0x2d4 - 0x2d0];
    int aim_valid;                                      // +0x2d4
    char pad2d8[0x2fc - 0x2d8];
    int snap;                                           // +0x2fc (frames left to snap the view)

    __declspec(noinline) void update(float dt);                              // 0x5284a0
    __forceinline void update_equipment();                       // 0x4f8b40
};

// a view of rgl::matrix with the engine's rotation helpers (thiscall, ecx = matrix)
struct MatrixExt_s284a0 {
    __forceinline void rotate_sf(float* angle);                       // 0x40e770
    __forceinline void rotate_fu(float* angle);                       // 0x40e570
};

// ---- key tables -------------------------------------------------------------------------------------

struct KeyNames_s284a0 {                                       // 0xa170c8
    __forceinline char is_ready(int action);                             // 0x487900 (ret 4)
};


// ---- globals ----------------------------------------------------------------------------------------
#include "../../include/fx.h"
#include "../../rgl/resource_manager.h"

#define AGENT(id) (g_mission_s284a0->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))

// ======================================================= slice s00529130: classes, globals, helpers
// Slice s00529130: the mission HUD's per-frame marker update (0x529130), its per-frame tick
// (0x5295a0), the marker enable/disable pass (0x529600) and the party-stack hero reset (0x529670).
// Everything here is a __thiscall member of the HUD object; the marker widgets are reached through
// their vtables (slot 0x58 = set position, 0xb0 = hide, 0xb4 = show).  Names are provisional.

using rgl::vector4;

// ---- vtable-bearing marker widget (slots by byte offset: 0x58 pos, 0xb0 hide, 0xb4 show) ---------------
struct Widget_s29130 {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(const float* p);                     // +0x58
    virtual void v23(); virtual void v24(); virtual void v25(); virtual void v26();
    virtual void v27(); virtual void v28(); virtual void v29(); virtual void v30();
    virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38();
    virtual void v39(); virtual void v40(); virtual void v41(); virtual void v42();
    virtual void v43();
    virtual void hide();                                      // +0xb0
    virtual void show();                                      // +0xb4
};

// secondary widget with an ARGB colour dword at +0x3c
struct Mark_s29130 {
    char pad[0x3c];
    unsigned color;                                           // +0x3c
};

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
extern const float g_aim_range_tbl[14];                         // 0x59390c (lo/hi pairs)

struct vec2;
float __cdecl signed_angle(const vec2* a, const vec2* b);                 // 0x4381e0


// ======================================================= slice s00529840: classes, globals, helpers
#define g_scene_s29840 (*(Mission_s29840**)&g_mission)
#define g_game_s29840 (*(GameData_s29840**)&g_game)
// Slice s00529840: the mission object's per-agent update (0x529840) and the party-stack helpers it
// dispatches to (0x529b70 / 0x529bc0 / 0x529c30 / 0x529d00), plus the scene/mission record sync (0x529d80).
// Everything here is a member of the object at 0x5e292c (agent table at +4); 0x529bc0 is a free __stdcall.

using rgl::vector4;

// ---- agent record (0x5928 bytes; layout from s0043ac40 / s0043bdb0 / s0043b5d0) -------------------------
struct Agent_s29840;
struct AgentTable_s29840 {
    int count;                                                // +0x00
    char pad4[0x18 - 4];
    Agent_s29840** pages;                                            // +0x18 (id >> 4 -> page of 16)

    __forceinline unsigned first_used();                                    // 0x43b1b0 (thiscall)
    __forceinline void next_used(unsigned* p);                              // 0x43b160 (thiscall, ret 4)
};
struct Agent_s29840 {
    char pad0[0x10];
    int kind;                                                 // +0x10 (Agent::kind)
    int state;                                                // +0x14 (Agent::state: 1 alive, 4 wounded, 5 killed)
    char pad18[0x2c - 0x18];
    int horse;                                                // +0x2c (Agent::horse)
    char pad30[0x50 - 0x30];
    vector4 pos;                                              // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                                // +0x88
    char pad8c[0x94 - 0x8c];
    int f94;                                                  // +0x94 (the value 0x529c30 drops from the stack's history)
    char pad98[0x1bc - 0x98];
    int party;                                                // +0x1bc (Agent::party: party id)
    int party_stack;                                          // +0x1c0 index of the agent's troop stack in that party
    char pad1c4[0x55d8 - 0x1c4];
    vector4 facing;                                           // +0x55d8
    char pad55e8[0x571c - 0x55e8];
    float start_health;                                       // +0x571c health when the mission started (the hero is healed from it)
    float max_health;                                         // +0x5720 (Agent::max_health)
    float health;                                             // +0x5724 (Agent::health)
    int pad5728[2];                                           // +0x5728
    int xp_pool;                                              // +0x5730 (Agent::xp_pool)
    char pad5734[0x5928 - 0x5734];
};



struct GameData_s29840 {
    char pad0[0x78];
    int cur_party;                                            // +0x78
    char pad7c[0x8c - 0x7c];
    int cur_troop;                                            // +0x8c
    char pad90[0xf8 - 0x90];
    PartyMgr parties;                                         // +0xf8 (GameData::parties)
    struct Light* wind;                                       // +0x139dc (GameData::wind)
    int f139e0;                                               // +0x139e0
    char pad139e4[0x13a00 - 0x139e4];
    Troop* troops;                                            // +0x13a00 (GameData::troops, 0x508 bytes each)
    char pad13a04[0x13a50 - 0x13a04];
    char* levels;                                             // +0x13a50 (GameData::levels: 0x494-byte level records)
    char pad13a54[0x1405c - 0x13a54];
    int tbl[64][32];                                          // +0x1405c (GameData::tbl: per party stack, the last values of 0x529c30)
};

// ---- 0x1c8-byte mission/scene record (layout from s00440a30) -------------------------------------------

struct IfaceB_s29840 {
    int f4, f8, fc;
    IfaceB_s29840() {}
    virtual ~IfaceB_s29840() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
// sig s1: the 0x1c8 record is the canonical PlacedObj (ctor 0x437830, copy 0x437bb0, operator= 0x437d60 in agent_stats.cpp)
typedef PlacedObj PlacedObj;

// ---- scene record list (the std::vector is at scene+0x104) ---------------------------------------------
struct Scene_s29840 {
    char pad0[0x104];
    std::vector<PlacedObj> recs;                                 // +0x104
    char pad114[0x494 - 0x114];
};

// ---- the mission object at 0x5e292c --------------------------------------------------------------------
struct Mission_s29840 {
    rgl::world* scene;                                        // +0x00 (Mission::scene)
    AgentTable_s29840 agents;                                        // +0x04
    char pad20[0x5c - 0x20];
    unsigned player_id;                                       // +0x5c (Mission::player_id)
    char pad60[0x5df4 - 0x60];
    int scene_idx;                                            // +0x5df4 (Mission::scene_idx)
    char pad5df8[0x6688 - 0x5df8];
    std::vector<PlacedObj> props;                             // +0x6688 (Mission::props)

    __declspec(noinline) void fn_529840(int mode);                                 // 0x529840 (thiscall, ret 4)
    void fn_529b70(Agent_s29840* a);                                 // 0x529b70 (thiscall, ret 4)
    void fn_529c30(int* hist, int party, int slot, int value, int flag); // 0x529c30 (thiscall, ret 0x14)
    void fn_529d00(Agent_s29840* a);                                 // 0x529d00 (thiscall, ret 4)
    __declspec(noinline) void fn_529d80();                                         // 0x529d80 (thiscall)
};

// ---- externals -----------------------------------------------------------------------------------------



inline PartyMgr* party_mgr_s29840() { return &g_game_s29840->parties; }
inline Troop* troop_tbl_s29840() { return g_game_s29840->troops; }

__declspec(noinline) void __stdcall fn_529bc0(Agent_s29840* a, int add); // 0x529bc0 (ret 8)

// ======================================================= slice s00529ec0: classes, globals, helpers
#define g_strtable_s29ec0 g_strtable
#define g_items_s29ec0 (*(ItemKind_s29ec0**)&g_items)
#define g_game_s29ec0 (*(GameData_s29ec0**)&g_game)
#define g_prop_kinds_s29ec0 (*(PropKind_s29ec0**)&g_propkinds)
#define g_mapobjs_s29ec0 (*(MapObjList_s29ec0**)&g_menus)
#define g_areas_s29ec0 (*(Area_s29ec0**)&g_mission_templates)
// Slice s00529ec0: the mission screen's interaction probe, Screen_s29ec0::f_529ec0 (0x529ec0, 4011 bytes).
//
// Every frame the screen asks "what is the local agent looking at?":
//   1. cast a thin capsule_s29ec0 (1e-4 m) from the agent's eye 10.5 m forward and query the level grid for
//      face shapes (shape_type()==2); the nearest hit gives a preliminary distance `best`;
//   2. re-run the probe as a 0.1 m capsule_s29ec0 extended 0.1 m forward against the scene's *entities*
//      (mask 9) and keep the nearest entity within best+1.0 m: agents on foot (talk), ridden horses
//      (mount/dismount), scene props (chest / baggage / map-object / equipment) and the map edges
//      (leave area / retreat);
//   3. when nothing was hit, fall back to the mount/dismount test and to the four mission-border
//      tests, which use the agent's facing vector;
//   4. validate the selection (enemy/friend test only colours the label; talk and mount additionally
//      need a free look, a riding skill high enough and/or a short distance);
//   5. reset the HUD line cache, localise the label ("ui_talk", "ui_mount", ...) and draw the coloured
//      label at y=0.027 followed by a grey " <action>" at y=0.01/0.035;
//   6. publish {mode, selection, valid} to Screen_s29ec0+0x2c8/+0x2cc/+0x2d4, where the key handler
//      (s005261e0) consumes them.
//
// Layouts: shape/capsule_s29ec0/contact_s29ec0/level from s00439100, Agent_s29ec0/ItemRef_s29ec0/ItemKind_s29ec0 from s00434ad0 +
// s0043b5d0, tstring/gui_manager_s29ec0/StrTable_s29ec0 idioms from s0052ae70, Screen_s29ec0 from s005261e0, the map
// object helpers from s004c6ca0. Names are provisional; every offset is read off this disassembly.

using rgl::vector4;
using rgl::matrix;



// ---- engine string (0x10 bytes) ----------------------------------------------------------------------


// ---- localisation tables (0x5e2a18) ------------------------------------------------------------------


// ---- collision shapes (s00439100) --------------------------------------------------------------------
struct shape_base_s29ec0 {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base_s29ec0() {}                                    // slot 0
    virtual int shape_type();                                   // slot 1 (0 sphere, 1 capsule_s29ec0, 2 face, ...)
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9();
    virtual vector4* get_min(vector4* out);                     // slot 10
    virtual vector4* get_max(vector4* out);                     // slot 11
};
typedef capsule capsule_s29ec0;   // sig s1: the canonical capsule (src/include/shape.h; ctor 0x433310, transform 0x408770)
struct contact_s29ec0 {                                                // 0x20 bytes
    vector4 pos;                                                // 0x00
    vector4 point;                                              // 0x10
};

// ---- level / scene -----------------------------------------------------------------------------------
struct SceneEntity_s29ec0 {
    void* vtbl;                                                 // +0x00
    char pad4[0x0c - 0x04];
    int game_id;                                                // +0x0c (rgl::entity::game_id: agent / prop index)
    int game_kind;                                              // +0x10 (rgl::entity::game_kind: 0xb agent, 0xc prop, ...)
    char pad14[0x88 - 0x14];
    vector4 pos;                                                // +0x88
    shape_base_s29ec0* shape;                                          // +0x98
};


// ---- items (s00434ad0 / s0043b5d0) -------------------------------------------------------------------
struct ItemKind_s29ec0 {                                               // 0x238, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned char type_flags;                                   // +0x1c8 (low byte: itp_type_*)
    char pad1c9[0x1ec - 0x1c9];
    int difficulty;                                             // +0x1ec (the riding skill a horse needs; ItemRef::difficulty 0x434c50 adds the modifier)
    char pad1f0[0x238 - 0x1f0];
};


// an 8-byte item reference; the modifier sits in byte 7
struct ItemRef_s29ec0 {
    int item;                                                   // +0x0
    unsigned m4;                                                // +0x4 (modifier in byte +7)
    __forceinline int difficulty();                                             // 0x434c50 (thiscall)
    __forceinline void name(char* out);                                       // 0x512730 (thiscall, ret 4)
};

// ---- agents (0x5928 bytes) ---------------------------------------------------------------------------
struct Agent_s29ec0 {
    char pad0[0x10];
    int kind;                                                   // +0x10 (Agent::kind: 0 = human, 1 = horse)
    int state;                                                  // +0x14 (Agent::state: 1 = alive)
    char pad18[0x2c - 0x18];
    int horse;                                                  // +0x2c (Agent::horse: the ridden horse, -1 = on foot)
    int rider;                                                  // +0x30 (Agent::rider: -1 = no rider)
    char pad34[0x50 - 0x34];
    vector4 pos;                                                // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                                  // +0x88 troop record index
    ItemRef_s29ec0 item;                                               // +0x8c
    char pad94[0xac - 0x94];
    float vel_y;                                                // +0xac (Agent::vel_xy.y: the forward speed)
    char padb0[0x55d8 - 0xb0];
    vector4 facing;                                             // +0x55d8
    char pad55e8[0x5830 - 0x55e8];
    int f5830;                                                  // +0x5830 action state (2 = mounted)
    char pad5834[0x5928 - 0x5834];

    __forceinline float speed_scale();                                             // 0x432450 (thiscall, no arg)
    __forceinline bool is_enemy(Agent_s29ec0* o);                                    // 0x432350 (ret 4)
    __forceinline bool is_friend(Agent_s29ec0* o);                                   // 0x4323a0 (ret 4)
};

// ---- game data (0x5e2930) ----------------------------------------------------------------------------
struct GameData_s29ec0 {
    char pad0[0x8c];
    int player;                                                 // +0x8c (GameData::player: the player's troop id)
    char pad90[0x13a00 - 0x90];
    char* chars;                                                // +0x13a00 troop records (0x508 bytes)
    char pad13a04[0x13a50 - 0x13a04];
    char* levels;                                               // +0x13a50 (GameData::levels: 0x494-byte level records)
    int nlevels;                                                // +0x13a54 their count (GameData::nparties)
    char pad13a58[0x13b10 - 0x13a58];
    char f13b10;                                                // +0x13b10
    char f13b11;                                                // +0x13b11
    __forceinline int skill_level(int troop, int skill);                      // 0x437b20 (ret 8)
};


// ---- mission (0x5e292c) ------------------------------------------------------------------------------
struct PropKind_s29ec0 {                                               // 0x78, array at 0x5e29dc
    char pad0[0x10];
    int flags;                                                  // +0x10 (low byte: 5 chest, 10 exit)
    char pad14[0x78 - 0x14];
};





// ---- map objects (0x5e29b0 / 0xa735c4, helpers in s004c6ca0) -----------------------------------------
struct MapObject_s29ec0 {                                              // 0x68 bytes
    char pad0[0x10];
    tstring name;                                               // +0x10
    tstring mesh;                                               // +0x20
    char pad30[0x68 - 0x30];
    __forceinline tstring* mesh_ptr();                                        // 0x4c6ca0 (thiscall)
};
MB_LAYOUT32_ASSERT(MapObject_s29ec0_size_check, sizeof(MapObject_s29ec0) == 0x68);
struct MapObjList_s29ec0 {                                             // 0x60 bytes
    char pad0[0x54];
    int count;                                                  // +0x54
    MapObject_s29ec0* objs;                                            // +0x58
    char pad5c[4];
};
MB_LAYOUT32_ASSERT(MapObjList_s29ec0_size_check, sizeof(MapObjList_s29ec0) == 0x60);   // indexed by g_mapobjs[i]: 0x60 stride (0x52a4ec: lea ecx,[ecx+ecx*2]; shl ecx,5)


// ---- area records (0x5e2a00, 0x58 bytes) --------------------------------------------------------------
struct Area_s29ec0 {                                                   // 0x58 bytes
    char pad0[0x20];
    unsigned flags;                                             // +0x20
    char pad24[0x58 - 0x24];
};


// ---- HUD globals -------------------------------------------------------------------------------------


// ---- the mission screen ------------------------------------------------------------------------------
struct Screen_s29ec0 {
    char pad0[0x26c];
    float pitch;                                                // +0x26c the view pitch (ViewCtrl_s284a0::pitch)
    char pad270[0x2c8 - 0x270];
    int sel_mode;                                               // +0x2c8 interaction mode (ViewCtrl_s284a0::mode)
    int sel_target;                                             // +0x2cc selection id (ViewCtrl_s284a0::target)
    char pad2d0[0x2d4 - 0x2d0];
    int sel_valid;                                              // +0x2d4 selection valid (ViewCtrl_s284a0::aim_valid)
    __declspec(noinline) void f_529ec0();                                            // 0x529ec0
};

// ======================================================= slice s0052ae70: classes, globals, helpers
#define g_strtable_s2ae70 g_strtable
// Slice s0052ae70: the mission order markers.
//   0x52ae70  build (once) the two "order" entities: the round/tetrahedron marker built from the shape
//             at Screen_s2ae70+0x6698 into three meshes, and the ground rectangle marker (four points, four
//             vertices, two triangles) of the second entity.
//   0x52b300  order tick: derive the current order id from the local agent's kind, hand it to
//             Mission_s2ae70::order, and when the "give order" state is 1 arm the 7.5 s timeout and repaint.
//   0x52b3c0  paint the five order labels (group / hold position / follow / charge / mount / dismount)
//             through the gui manager, tinting the selected one.
// The Screen_s2ae70 layout (+0x29c.. / +0x6698.. / +0x67xx) is shared with slice s005261e0; names are
// provisional and every offset is read off this disassembly.



// ---- engine string (0x10 bytes, ctor 0x42bbe0, dtor 0x42bc40) ------------------------------------------


// ---- resolved material_s2ae70 ---------------------------------------------------
struct material_s2ae70;

// ---- localisation table (0x5e2a18) ---------------------------------------------------------------------


// ---- rgl::mesh_s2ae70 (0x260 bytes; ctor 0x412a90) ------------------------------------------------------------
namespace rgl {
struct vec2_s2ae70 { float x, y; vec2_s2ae70() {} vec2_s2ae70(float a, float b) : x(a), y(b) {} };

struct mesh_s2ae70 {
    char pad0[0x2c];
    material_s2ae70* res;                                      // +0x2c
    char pad30[0x138 - 0x30];
    char f138;                                          // +0x138 (dirty flag)
    char pad139[0x164 - 0x139];
    char f164;                                          // +0x164 (dirty flag)
    char pad165[0x260 - 0x165];
    __forceinline int add_point(const vector4* p);                    // 0x412f50
    __forceinline int add_vertex(int index, const vec2_s2ae70* uv, const unsigned* color, const vector4* pos,
                   const int* f5c);                     // 0x413c50 (ret 0x14)
    __forceinline int add_face(int a, int b, int c);                  // 0x411920
};
} // namespace rgl

// ---- entity (created by World_s2ae70::create_entity(9)) --------------------------------------------------------
struct Entity_s2ae70 {
    virtual void v0(); virtual void v1(); virtual void v2();
    virtual void v3(); virtual void v4(); virtual void v5();
    virtual void set_scale(const float* s);             // vtable +0x18
    char pad[0x140 - 4];
    unsigned char visible;                              // +0x140 (rgl::entity::visible)
    __forceinline void add_extra(rgl::mesh_s2ae70* m);                        // 0x442110 (thiscall, ret 4)
};


// the shape owned by the Screen_s2ae70 at +0x6698 (0x94 bytes; see s004e65e0 / s004e6a70)
struct Shape_s2ae70 {
    char pad[0x94];
    __forceinline void polygon_to_mesh(rgl::mesh_s2ae70* m);                       // 0x4e75a0 (thiscall, ret 4)
    __forceinline void build_ribbons(rgl::mesh_s2ae70* m);                    // 0x4e6a70 (thiscall, ret 4)
    __forceinline void build_spikes(rgl::mesh_s2ae70* m);                     // 0x4e65e0 (thiscall, ret 4)
};

// ---- mission (0x5e292c) and the game clock -------------------------------------------------------------



// ---- the mission screen --------------------------------------------------------------------------------
struct Screen_s2ae70 {
    rgl::world* world;                                         // +0
    char pad4[0x29c - 4];
    int order_state;                                    // +0x29c (MissionBlocker::f29c: 1 = the order menu is up)
    int order;                                          // +0x2a0 (MissionBlocker::f2a0: the selected order)
    char pad2a4[0x2a8 - 0x2a4];
    Timer order_timer;                                  // +0x2a8 the order menu's 7.5 s timeout
    char pad2b8[0x654c - 0x2b8];
    float base_z;                                       // +0x654c (Mission::base_z: the ground height)
    char pad6550[0x6698 - 0x6550];
    Shape_s2ae70 shape;                                        // +0x6698 (Mission::nav)
    rgl::mesh_s2ae70* marker_poly;                             // +0x672c
    rgl::mesh_s2ae70* marker_spikes;                           // +0x6730
    rgl::mesh_s2ae70* marker_ribbons;                          // +0x6734
    Entity_s2ae70*    marker_entity;                           // +0x6738
    rgl::mesh_s2ae70* marker_ground;                           // +0x673c
    Entity_s2ae70*    special;                                 // +0x6740 (Mission::special: the ground marker's entity)

    __declspec(noinline) void init_order_markers();                          // 0x52ae70
 __forceinline int vtx(int p, const rgl::vec2_s2ae70& uv, const unsigned& color, const rgl::vector4& n, const int& zero);
    __declspec(noinline) void order_tick();                                  // 0x52b300
    void draw_order_menu();                             // 0x52b3c0
};

// shape builders (all take the shape in ecx and the target mesh_s2ae70 on the stack)

// ======================================================= slice s0052b5c0: classes, globals, helpers
#define g_strtable_s2b5c0 g_strtable
#define g_items_s2b5c0 (*(ItemKind_s2b5c0**)&g_items)
#define g_game_s2b5c0 (*(CampaignGame_s2b5c0**)&g_game)
#define g_mission_s2b5c0 (*(Mission_s2b5c0**)&g_mission)
#define g_editor_s2b5c0 (*(Editor_s2b5c0*)&g_app_obj)
// Slice s0052b5c0: the mission order and cursor/terrain helpers.
//   0x52b5c0  Mission_s2b5c0::order_agent   -- give an item/order to one of an agent's four order slots
//   0x52ba10  Mission_s2b5c0::update_ui     -- per-frame UI/cursor state machine
//   0x52ba1d  interior continuation of 0x52ba10 (EBX/ZF preset) -- translated in full
//   0x52bc80  Mission_s2b5c0::update_cursor -- ray-pick the ground and drive the terrain brush
//   0x52bdb0  Mission_s2b5c0::find_position -- screen ray -> world position through the collision world
// Layouts and callee signatures are the ones verified in the neighbouring slices
// (s0052ae70 Mission_s2b5c0/Screen, s0052f250 MapEditor, s005191e0 collision shapes, s0052c2b0 brush).

using rgl::vector4;
using rgl::matrix;



// ---- engine string (16 bytes) -------------------------------------------------------------------------




// ---- item kinds / order references --------------------------------------------------------------------
struct ItemKind_s2b5c0 {
    char pad[0x1c8];
    unsigned kind;                                      // +0x1c8
    char pad2[0x238 - 0x1cc];
};


struct ItemRef_s2b5c0 {
    int id;                                             // +0x00
    int count;                                          // +0x04
    __forceinline int max_amount();                                   // 0x434b70 (thiscall)
    __forceinline void name(char* out);                               // 0x512730 (thiscall, ret 4)
};

// ---- agents -------------------------------------------------------------------------------------------
struct AgentFrame_s2b5c0 {
    char pad[0x58];
    matrix m;                                           // +0x58
};

struct Agent_s2b5c0 {
    char pad0[0x1d0];
    ItemRef_s2b5c0 orders[4];                                  // +0x1d0
    char pad1f0[0x30];
    char active[4];                                     // +0x220
    char pad224[8];
    int wield[2];                                       // +0x22c (Agent::wield: the wielded item index per hand)
    char pad234[0x1c];
    AgentFrame_s2b5c0* frame;                                  // +0x250
    char pad254[0x5928 - 0x254];
    __forceinline void on_wield_reset();                              // 0x445a20 (thiscall)
};

// ---- collision world (mission->f0) --------------------------------------------------------------------
struct shape_base_s2b5c0 {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base_s2b5c0() : m4(-1), m8(8), mc(-1),
                   min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base_s2b5c0() {}
    virtual int shape_type();                           // slot 1, 0x408020
    __forceinline void intersect(vector4* out, const vector4* a, const vector4* b);   // 0x51a2d0
};
typedef capsule capsule_s2b5c0;


struct Camera_s2b5c0 {
    __forceinline void screen_to_world(vector4* near_pt, vector4* far_pt);    // 0x4bd2f0
};



// ---- the mission / screen object (0x5e292c) -----------------------------------------------------------
#include "../../rgl/scene/scene_entity.h"
typedef rgl::entity UiObj_s2b5c0;                       // the mission's scene entities (+0x65d0 / +0x65dc / +0x65e4 / +0x6738)

struct CampaignGame_s2b5c0 {
    char pad0[0x1a094];
    int game_mode;                                      // +0x1a094
    int f1a098;                                         // +0x1a098
    vector4 color;                                      // +0x1a09c
    int f1a0ac;                                         // +0x1a0ac
    char pad1a0b0[0x1a0c8 - 0x1a0b0];
    char f1a0c8;                                        // +0x1a0c8
    char pad1a0c9[0x1a0d4 - 0x1a0c9];
    float brush_radius[3];                              // +0x1a0d4 the terrain brush radius of edit modes 1, 2, 3 (game_mode - 1)
    char pad1a0e0[0x1a1cc - 0x1a0e0];
    int f1a1cc;                                         // +0x1a1cc
    char pad1a1d0[0x1a20c - 0x1a1d0];
    int f1a20c;                                         // +0x1a20c
};


struct Mission_s2b5c0 {
    rgl::world* scene;                                         // +0x00 (Mission::scene)
    char pad4[0x18 - 4];
    std::vector<Agent_s2b5c0*> pages;                          // +0x18 (data pointer at +0x1c)
    char pad28[0x60 - 0x28];
    void* view;                                         // +0x60
    char pad64[0x2b8 - 0x64];
    // +0x2b8 .. +0x2f0: the MissionBlocker members (update_ui and update_cursor run with this = the HUD object)
    Timer pick_timer;                                   // +0x2b8 the entity pick runs every 0.05 s
    int f2c8;                                           // +0x2c8 (MissionBlocker::f2c8: the interaction mode)
    int sel_target;                                     // +0x2cc (MissionBlocker::f2cc: -1 = nothing selected)
    char pad2d0[0x2f0 - 0x2d0];
    char scene_dirty;                                   // +0x2f0 the terrain brush changed the ground (rebuilt by conv_build_scene)
    char pad2f1[0x63d0 - 0x2f1];
    char terrain[0x651c - 0x63d0];                      // +0x63d0 (Mission::terrain, a TerrainDesc)
    int f651c;                                          // +0x651c
    char pad6520[0x65d0 - 0x6520];
    UiObj_s2b5c0* f65d0;                                       // +0x65d0
    char pad65d4[8];
    UiObj_s2b5c0* f65dc;                                       // +0x65dc
    char pad65e0[4];
    UiObj_s2b5c0* f65e4;                                       // +0x65e4
    char pad65e8[0x6738 - 0x65e8];
    UiObj_s2b5c0* marker_entity;                               // +0x6738 (Mission::marker_entity)

    __declspec(noinline) bool order_agent(unsigned agent_id, ItemRef_s2b5c0* order); // 0x52b5c0
    __declspec(noinline) void update_ui();                                   // 0x52ba10
    __declspec(noinline) void update_ui_continue();                          // 0x52ba1d (interior continuation)
    void update_cursor();                               // 0x52bc80
    __declspec(noinline) void find_position(char* ok, vector4* pos);         // 0x52bdb0
    __forceinline int add_placed(const int* kind, const int* id, const int* value,
                   const matrix* m, int* spawn_id);     // 0x44f870
    __forceinline void update_map();                                  // 0x52f250
    __forceinline void update_dialog();                               // 0x5305b0
    __forceinline void update_mode4();                                // 0x52cf60
    __forceinline void pick_entity();                                 // 0x4ed8d0 (fastcall free function pick_entity(GuiCtx*))
};


struct Editor_s2b5c0 {
    __forceinline void update_checks();                        // 0x4bf640
    __forceinline void refresh();                                     // 0x4c77c0
};




// =====================================================================================================
//  Mission_s2b5c0::order_agent
// =====================================================================================================
// ======================================================= slice s0052bf40: classes, globals, helpers
#define g_game_s2bf40 (*(CampaignGame_s2bf40**)&g_game)
// Slice s0052bf40: the terrain brush/grid update.
//   0x52bf40  Terrain_s2bf40::update   -- clear the brush grid, drive the brush over the grid cells around
//                                 the cursor (0x52c2b0), regenerate the mode-0/1/2 objects and
//                                 rebuild every layer chunk of the touched cells (0x47ff60).
//   0x52c290  Terrain_s2bf40::clear    -- zero the 0x177-byte brush grid at this+0x845e030.
// Layouts follow the neighbouring slices (s0052c2b0 brush, s0052f250 map editor, s0052b5c0 mission).

using rgl::vector4;
using rgl::matrix;



struct CampaignGame_s2bf40 {
    char pad0[0x1a094];
    int game_mode;                                      // +0x1a094
    char pad1a098[0x1a0d4 - 0x1a098];
    float brush_radius[3];                              // +0x1a0d4 the terrain brush radius of edit modes 1, 2, 3 (game_mode - 1)
};




// =====================================================================================================
//  Terrain_s2bf40::update
// =====================================================================================================
// ======================================================= slice s0052c2b0: classes, globals, helpers

// Slice s0052c2b0: the terrain "brush" of the world-map editor (0x52c2b0, 2887 bytes, __thiscall,
// ret 0x1c = 7 stack args). It maps the brush amount (arg4) through the per-map-mode response curve
// selected by g_game_c->game_mode (+0x1a094), then either rewrites the height/colour values of one or
// many map tiles (the 24-byte "element" records reached from arg1, whose per-cell value array at +0x14
// is a 0xac440-byte lazily allocated block) or marks the touched grid cells in this[0x845e030].
// Reconstruction of the original control flow (verified against the disassembly); NOT a byte-exact
// match: the original's x87 evaluation order and the compiler's 4x unrolling of the 0.98 scan are
// reproduced only structurally. See nonmatching.txt.

typedef unsigned int uint;

// ---------------------------------------------------------------------------------------------------
// globals (addresses from the annotations in the disassembly)
#define g_game_c ((char*)g_game)
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
struct conv_rec; struct GuiCtx;
void __fastcall pick_entity(GuiCtx*);   // 0x4ed8d0
struct UiD {
    void set_value(float);   // 0x48baa0
};

struct Agent; struct AgentTable; struct AgentX; struct BlockOwner; struct Blow; struct ContainerBase; struct CreateGameOptionsPage; struct DialogMgr; struct ForceLabel; struct Game; struct GameData; struct GuiCtx; struct ItemRef; struct LangTables; struct Local; struct MapEditorDialog; struct MapEditor_s2cf60; struct MapEditor_s2f250; struct MapFlags; struct MapObject; struct Mission; struct MissionSource; struct NamedRec; struct Party; struct Pickable; struct Picker; struct PlacedObj; struct PolyMesh; struct Poser; struct Shape; struct Slot; struct StrTable; struct Timer; struct UiD; struct WindowMgrW; struct camera_ray; struct capsule; struct contact; struct level; namespace rgl { struct matrix; } typedef rgl::matrix pos; struct shape_base; struct skin; struct snd_bank; struct snd_slot; struct stream_base; struct tstring; struct vec2;
namespace rgl { struct entity; }

struct ItemRef;
#include "../mission.h"   // the real Agent, AgentTable, Mission (agent_obj.h)
struct AgentX {
    bool f100_is_0x48_and_charged() const;   // 0x432470
    bool is_enemy(Agent const*) const;   // 0x432350
    float speed_scale() const;   // 0x432450
};
struct BlockOwner {
    void init(ContainerBase*, int);   // 0x4dd920
};
struct CreateGameOptionsPage {
    void update_checks();   // 0x4bf640
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
struct Local {
    void read(stream_base&);   // 0x524f00
};
struct MapEditorDialog {
    void refresh();   // 0x4c77c0
    void update();   // 0x5305b0
};
struct MapEditor_s2cf60 {
    void update();   // 0x52cf60
};
struct MapEditor_s2f250 {
    void update();   // 0x52f250
};
struct MapObject {
    tstring* mesh_ptr();   // 0x4c6ca0
};
struct MissionSource {
    void pick(int*);   // 0x4d5410
};
struct Picker {
    void collect(unsigned, Pickable**, int*);   // 0x51a3a0
};
struct Poser {
    void add_extra(rgl::mesh*);   // 0x442110
};
struct Shape {
    void build_ribbons(rgl::mesh*);   // 0x4e6a70
    void build_spikes(rgl::mesh*);   // 0x4e65e0
    void polygon_to_mesh(rgl::mesh*);   // 0x4e75a0
};
#include "../../include/timer.h"   // Timer (elapsed 0x432c10)
namespace c49c2c0 {
struct Mission {
    void order(int, int);   // 0x49c2c0
};
}
#include "../map_widget.h"            // MapWidget::screen_to_world 0x4bd2f0

__forceinline void Entity_s2ae70::add_extra(rgl::mesh_s2ae70* a0) { ((rgl::entity*)this)->add_mesh((rgl::mesh*)a0); }
__forceinline int rgl::mesh_s2ae70::add_face(int a0, int a1, int a2) { return ((rgl::mesh*)this)->add_face(a0, a1, a2); }
__forceinline int rgl::mesh_s2ae70::add_point(rgl::vector4 const* a0) { return ((rgl::mesh*)this)->add_point(a0); }
__forceinline void Shape_s2ae70::build_ribbons(rgl::mesh_s2ae70* a0) { ((Shape*)this)->build_ribbons((rgl::mesh*)a0); }
__forceinline void Shape_s2ae70::build_spikes(rgl::mesh_s2ae70* a0) { ((Shape*)this)->build_spikes((rgl::mesh*)a0); }


__forceinline void Agent_s284a0::cycle_slot(int a0) { ((Agent*)this)->cycle_slot(a0); }
__forceinline Agent_s284a0* AgentTable_s284a0::get(unsigned id) { return (Agent_s284a0*)((AgentTable*)this)->get(id); }
__forceinline int AgentTable_s284a0::first_used() { return ((AgentTable*)this)->first_used(); }
__forceinline unsigned AgentTable_s29840::first_used() { return (unsigned)((AgentTable*)this)->first_used(); }
__forceinline void shape_base_s2b5c0::intersect(rgl::vector4* a0, rgl::vector4 const* a1, rgl::vector4 const* a2) { ((face*)this)->intersect(a0, a1, a2); }   // 0x51a2d0 face::intersect
__forceinline bool Agent_s284a0::is_enemy(Agent_s284a0* a0) { return ((AgentX*)this)->is_enemy((Agent const*)a0); }
__forceinline bool Agent_s29ec0::is_enemy(Agent_s29ec0* a0) { return ((AgentX*)this)->is_enemy((Agent const*)a0); }
__forceinline bool Agent_s284a0::is_friend(Agent_s284a0* a0) { return ((Agent*)this)->is_friend((Agent const*)a0); }
__forceinline bool Agent_s29ec0::is_friend(Agent_s29ec0* a0) { return ((Agent*)this)->is_friend((Agent const*)a0); }
__forceinline char KeyNames_s284a0::is_ready(int a0) { return (char)((Slot*)this)->is_ready(a0); }
__forceinline int ItemRef_s2b5c0::max_amount() { return ((ItemRef*)this)->max_amount(); }
__forceinline tstring* MapObject_s29ec0::mesh_ptr() { return ((MapObject*)this)->mesh_ptr(); }
__forceinline char const* GameData_s27c90::module_path(char const* a0) { return ((GameData*)this)->module_path(a0); }
__forceinline void ItemRef_s29ec0::name(char* a0) { ((ItemRef*)this)->name(a0); }
__forceinline void ItemRef_s2b5c0::name(char* a0) { ((ItemRef*)this)->name(a0); }
__forceinline void AgentTable_s284a0::next_used(unsigned* a0) { ((AgentTable*)this)->next_used(a0); }
__forceinline void AgentTable_s29840::next_used(unsigned* a0) { ((AgentTable*)this)->next_used(a0); }
__forceinline void Agent_s284a0::note_blow(Blow_s284a0* a0) { ((Agent*)this)->note_blow((Blow*)a0); }
__forceinline void Agent_s284a0::on_wield_reset() { ((Agent*)this)->on_wield_reset(); }
__forceinline void Agent_s2b5c0::on_wield_reset() { ((Agent*)this)->on_wield_reset(); }
__forceinline void Shape_s2ae70::polygon_to_mesh(rgl::mesh_s2ae70* a0) { ((Shape*)this)->polygon_to_mesh((rgl::mesh*)a0); }
__forceinline void Editor_s2b5c0::refresh() { ((MapEditorDialog*)this)->refresh(); }

__forceinline void MatrixExt_s284a0::rotate_fu(float* a0) { ((rgl::matrix*)this)->rotate_about_s((float const*)a0); }
__forceinline void MatrixExt_s284a0::rotate_sf(float* a0) { ((rgl::matrix*)this)->rotate_about_u((float const*)a0); }
__forceinline tstring GameData_s27c90::scene_path(int a0) { return ((GameData*)this)->scene_path(a0); }
__forceinline void Camera_s2b5c0::screen_to_world(rgl::vector4* a0, rgl::vector4* a1) { ((MapWidget*)this)->screen_to_world(a0, a1); }
__forceinline int GameData_s29ec0::skill_level(int a0, int a1) { return ((GameData*)this)->skill_level(a0, a1); }
__forceinline float Agent_s29ec0::speed_scale() { return ((AgentX*)this)->speed_scale(); }
__forceinline int ItemRef_s29ec0::difficulty() { return ((ItemRef*)this)->difficulty(); }
__forceinline void vec3_s257f0::sub_assign(vec3_s257f0 const& a0) { ((Vec3*)this)->sub_assign((Vec3 const&)a0); }
__forceinline void Editor_s2b5c0::update_checks() { ((CreateGameOptionsPage*)this)->update_checks(); }
__forceinline void Mission_s2b5c0::update_dialog() { ((MapEditorDialog*)this)->update(); }
__forceinline void ViewCtrl_s284a0::update_equipment() { ((MissionBlocker*)this)->update_equipment(); }
__forceinline void Mission_s2b5c0::update_map() { ((MapEditor_s2f250*)this)->update(); }
__forceinline void Mission_s2b5c0::update_mode4() { ((MapEditor_s2cf60*)this)->update(); }

__forceinline ItemRef Agent_s284a0::slot_pair(int a0) { return ((Agent*)this)->slot_pair(a0); }
__forceinline Blow_s284a0::Blow_s284a0() { ((Blow*)this)->init(); }
__forceinline int rgl::mesh_s2ae70::add_vertex(int a0, const vec2_s2ae70* a1, const unsigned* a2, const vector4* a3, const int* a4) { return ((rgl::mesh*)this)->add_vertex(a0, (const vec2*)a1, a2, a3, a4); }
__forceinline int Mission_s2b5c0::add_placed(const int* a0, const int* a1, const int* a2, const matrix* a3, int* a4) { return ((Mission*)this)->add_placed(a0, a1, a2, a3, a4); }


__forceinline void Mission_s2b5c0::pick_entity() { ::pick_entity((GuiCtx*)this); }

#endif // MISSION5_MISSION_5_H
