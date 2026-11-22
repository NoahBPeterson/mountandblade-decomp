// game_data_party_quest.h: the party / quest / experience members of the game object (g_game, 0x5e2930), the mission-side helpers
// that sit between them (0x490030..0x4958f0: translation units 83 world_2 and 84 game_data_2 of the TU map), and the
// small engine objects those functions reach (message log, sound player, window manager, scene list, panel family).
// Names provisional; every offset cites the function that proves it.
//
// The shared classes are included, not redefined: GameData / PartyMgr / Party / Stack / Troop / QuestRec / tstring /
// MsgLog / StrTable / Vec2 / MissionUi / ContainerBase / DropDown come from src/include and src/game. What those headers
// lack is added here as *views*: a derived class with no data members (`GameW : GameData`, ...) whose
// inline accessors read the raw offsets this module's functions use (the technique of script/script_interpreter.h).
// Pure additions to canonical headers: none.
#ifndef GAME_WORLD_DATA_H
#define GAME_WORLD_DATA_H
#include "platform/image_addrs.h"
#include <new>
#include <vector>
#include <deque>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "engine/core/tstring.h"
#include "game/common/game_globals.h"      // canonical g_module_settings.player_wounded_treshold
#include "game/data/str_table.h"
#include "engine/math/rgl_math.h"
#include "engine/math/vec.h"
#include "game/data/game_data.h"
#include "game/ui/gui_manager.h"
#include "game/ui/widgets/widget_manager.h"
#include "game/ui/widgets/mission.h"
#include "game/data/rec_dtors.h"
#include "engine/fx/fx.h"        // FxElem (add_group 0x4423f0)
#include "game/mission/mission.h"
#include "game/mission/placed_obj.h"

#include "game/data/raw_place.h"            // raw_place: constructing a canonical class in raw storage
#include "game/data/module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in game/app/options_dialog_and_module_tables.cpp)
#include "game/data/game_recs.h"
#include "engine/physics/shape.h"          // face (GameData::find_at)
#include "game/common/level_global.h"      // g_level (0xa732b4)
#include "engine/io/text_stream.h"

// raw field of a view class
#define WD_FLD(T, off) (*(T*)((char*)this + (off)))

// ---- the party manager embedded in the game object at +0xf8 (game/party/party.h PartyMgr) --------------------------------



// the container widget is game/ui/widgets/container.h's ContainerBase (its members are defined below, in game_data_party_quest.cpp)

inline int& troop_f54(Troop* t) { return t->upgrade_troop[1]; }   // +0x54 second upgrade troop (0x494e40)

// ---- the game object (game/data/game_data.h GameData) -----------------------------------------------------------------
// (the level object's face-sharing check is NavData::shares 0x48efa0, game/terrain/nav_data.h)

namespace rgl { struct strategic_entity; }
rgl::strategic_entity* __fastcall trigger_entity(int context);                     // 0x490430 (the scene entity of a trigger context)

#include "game/data/game_recs.h"   // PartyTemplate* g_party_templates (0x5e29f8)
extern float g_k15;                                             // 0x590da0 (15.0f)

// GameData's operations of 0x490030..0x4957d0 are declared on GameData (game/data/game_data.h).

// ---- the mission object (game/mission/mission.h Mission) and its agent records -----------------------------------------------



// ---- vectors of 8-byte integers, 4-byte-integer vectors (VC7.1 layout: allocator word first) -----------------------------
#include "engine/core/pod_elems.h"
struct FPair { float a, b; };

// (the record arrays the game object's teardown deletes (0x492a20) are TroopFile / FactionFile / QuestFile: game/campaign/data_files.h,
// and the scene records SceneFile: game/terrain/scene_file.h)

typedef void (__stdcall* PanelFn)(void* self, unsigned char v);

// ---- the event pool (GameData +0xcc: Pool<Event>, engine/core/pool.h; alloc 0x493700) and the cache group (E68 members) -----------
// Its record is game/campaign/event.h's Event (constructor 0x48c210, vtable 0x5922cc, an IfaceB of 0x98 bytes); the pages are `new Event[16]`,
// which is where the vector deleting destructor in the vtable is emitted. The party pages are `new Party[16]` (PartyMgr::grow).
#include "game/data/troop_inventory_stats.h"
#include "engine/resource/resource.h"                               // resmgr g_resmgr 0x5d6fa0
// the tableau record's materials: E68::find_or_create 0x493df0 / E68::purge 0x493e90 (game/data/e68.h) on rgl skin (key f60, refcount)
#include "engine/scene/view_params.h"                       // ViewParams (ctor 0x46b940)

// ---- the tableau table (g_tableaus 0xa75f40; the shared header tableau.h has the whole class) -------------------------------------
#include "game/data/tableau.h"

#include "game/ui/widgets/widget_manager.h"

// ---- the window manager (g_windows 0xa73980: 30 screen objects, the open-screen deque at +0x78) --------------------------------
#include "game/data/head_rec.h"                                      // head_rec: the screens' base (slot 0 = deleting destructor)
#include "game/ui/window_mgr.h"                                   // WindowMgrW g_windows (0xa73980)

// ---- the sound player (g_sound_mgr 0x5aa128): engine/audio/sound_mgr.h ----------------
#include "engine/audio/sound_mgr.h"


#include "game/config/config.h"                                  // g_cfg 0x5a9f98
#include "engine/render/renderer_include.h"                              // g_render 0xa17910 (dev at +0x618 = 0xa17f28)
struct RotRows { rgl::vector4 s, f, u; };                       // the rotation part of a frame (0x30 bytes)
#include "engine/math/mat4_include.h"                              // rgl::mat3
extern "C" void __stdcall FSOUND_StopSound(int handle);                            // 0x5342a6 _FSOUND_StopSound@4
#include "engine/audio/fmod_api.h"   // FMOD 3 (one declaration per entry point)

// ---- the carried-item placement of an agent body (0x495430): views of the item kind, mesh, skeleton ---------------------------------------
#include "game/data/item_ref.h"                                      // ItemRef: the carried item refs
namespace rgl { struct lod_mesh; }
struct CarryCtx {                                               // the agent's display slots (Agent::slot_mesh .., build_appearance's ap)
    rgl::lod_mesh* held[4];                                    // [0..3] the held-item meshes (not read here)
    rgl::lod_mesh* c1[4];                                      // [4..7]  (Agent::links)
    rgl::lod_mesh* c2[4];                                      // [8..11] (Agent::B)
    rgl::lod_mesh* c3[4];                                      // [12..15] (Agent::disp)
};
// The placement calls FxDef::remove_group / FxElem::add_group (engine/fx/fx.h) on the body's skeleton (rgl::strategic_entity::fx),
// ItemKind::get_carry_pos (game/data/item_kind_include.h) and Agent::agent_class (game/agent/agent_obj.h).
#include "engine/scene/scene_entity.h"                      // rgl::strategic_entity: the body that carries the items
void __cdecl place_scene_agents(rgl::strategic_entity* holder, ItemRef* table, CarryCtx* ctx);   // 0x495430

// ---- 0x4952f0: the effect object the mission's scene container hands out (the wind / light record, 0x7c bytes) ----------------------------

// ---- the particle-system table (Party::add_emitter, 0x4955e0, is in game_data_party_quest.cpp) -----------------------------------------------------------------------------
#include "game/data/emit.h"

// ---- 0x495650: the scene list ------------------------------------------------------------------------------------------------------
#include "engine/scene/frustum.h"                              // rgl::frustum (0x120 bytes; the ctor 0x41ef60); the stand-in "Frustum" was 0x134 bytes with 0x14 unknown after it
#include "game/data/scene_object.h"                             // SceneObject: the tableau scene (an rgl::world)


#endif // GAME_WORLD_DATA_H
