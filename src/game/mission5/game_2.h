// game_2.h: declarations of src/game/mission5/game_2.cpp (TU 115 "game_2", 0x520b20..0x525160): the map and world-map scene
// builders (map.txt, the tree meshes of the campaign map), the module-file loader Game::load_game_data (factions.txt,
// skills.txt, troops.txt, scenes.txt, quests.txt, triggers.txt, simple_triggers.txt) and the record constructors / readers
// it calls. The shared classes are used where they exist (Game: src/game/party/game.h; the Data\*.txt record classes are
// the destructor-side classes of src/rgl/xmlcfg/records.h: RecS = troop, RecQ = quest, RecT = faction; the scene record's destructor is SceneFile's own);
// this header adds only views of their padding (named ...View) and the classes that exist nowhere else. Offsets cite the
// function that proves them.
#ifndef MISSION5_GAME_2_H
#define MISSION5_GAME_2_H
#include <stdio.h>
#include "../../include/layout32.h"
#include <stdlib.h>
#include <math.h>
#include <new>
#include "../party/game.h"
// std::vector<int>::_Insert_n stays visible in this module: cl only allocates the push_back callers as the original when it can see the body; the instance folds with 0x410610 by name
#include "../../include/iface.h"        // IfaceB (the polymorphic base of the scene record)
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh.h"            // rgl::mesh::compute_bounds (0x419300)
#include "../../rgl/stream.h"            // text_stream::open_read_text (0x415d60)
#include "../light.h"                    // Light::init (0x461480)
#include "../../rgl/scene/scene_entity.h"
#include "../terrain_b/cell_grid.h"      // CellGrid (the level, 0xa732b4), NavData, Wind
#include "../../rgl/scene/world.h"            // rgl::world

void __cdecl mb_error(const char* fmt, ...);        // 0x42cc80

// 0x415d60 is text_stream::open_read_text (rgl/stream.h); the readers here have the same file fields (fp +0x30, failed +0x34)
#define OPEN_READ_TEXT(s, path) ((s).open_read_text(path))

#define RSTR(p) ((rgl::string*)(p))                 // the stream's string slot takes rgl::string* (same 16-byte tstring)

// ---- the text reader the loaders build on their stack (vtable 0x54ac88, tstream3): its constructor and the user-declared
// empty destructor of text_stream2 are inlined there (stream_inline.h) ----
#include "../../rgl/stream_inline.h"

// ---- std::vector<tstring> internals (VC7.1, 0x10-byte element) ---------------------------------------------------
#include "../../include/ptr_tag.h"             // ptr_tag: std::_Nonscalar_ptr_iterator_tag (empty, passed by value)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../../include/render_flags.h"
// The out-of-line instances of std::vector<int> that other objects define (agent_combat.cpp 0x43e2a0, mesh_obj.cpp 0x410610) are called
// through their std:: names (an explicit specialization declared without a definition makes every use a call); the vector<tstring> ones
// (0x43e310, 0x42d360, 0x42d390) are declared in data_files.h.
#include "./data_files.h"             // TroopFile, QuestFile, FactionFile, tstring_vec
#include "../../include/vec_xlen.h"   // std::vector<int>::_Xlen 0x43e2a0
#include "../alloc_construct.h"   // alloc_tstring 0x436c90
void __stdcall vector_constructor_iterator(void* dst, unsigned size, int count, void (__fastcall *ctorfn)(void*));        // 0x401800


// ---- the data tables and their globals ---------------------------------------------------------------------------
#include "../str_table.h"       // StrTable g_strtable (0x5e2a18): find 0x4c6010
#define MODULE_PATH(sub) (module_path(sub))

// (item, modifier word) pairs of the statement / troop / scene records are the shared ItemRef (../item_ref.h; item -1 = none)


// skills.txt record: SkillRec (include/skill_rec.h), table g_skill_tbl 0x8ee3100
#include "../skill_cap.h"
__declspec(noinline) stream_base& __cdecl read_skill(stream_base& s, SkillRec& r);     // 0x5243c0

// ---- the Data\*.txt record classes: TroopFile / QuestFile / FactionFile are data_files.h's; SceneFile (terrain_b/scene_file.h) below ---------
// the scene record's embedded passage / site writer stream (vtable slots 3, 7 and 8 of stream_base)
#include "../terrain_b/scene_file.h"       // SceneFile (the 0x494-byte scene record; shared with terrain_b/mission.cpp)
#include "../../rgl/stream_inline.h"   // the stream constructors / destructors inline (the scene record's embedded streams)
// the empty polygon mesh SceneRec::read writes into the embedded stream (PolyMesh::save 0x524f00)
#include "../terrain_b/poly_mesh.h"                          // PolyMesh (constructor 0x4d1360, destructor 0x4cb780, save 0x524f00)

// ---- script emitters (0xa75eb0 array, count 0xa75eb4) ------------------------------------------------------------
#include "../../include/rec34.h"                 // rec34_table, g_scripts (0xa75eb0)

#include "../../include/rag_model.h"            // RagModel, SkeletonBody (load 0x50fc60: inventory_panel/skeleton_load.cpp)
void __fastcall fix_skeletons(std::vector<RagModel>* list);          // 0x4cd850 (rgl/mesh2/mesh_2.cpp)
extern SkeletonBody g_skeletons;                           // 0xa180e8 skeleton-body table (rgl/mesh2/mesh_2.cpp)

// trigger vectors at Game +0x139e8 / +0x139f4 (game.h: TriggerVec / SimpleTriggerVec, whose padding these resize)
#include "../trigger_vecs.h"           // TriggerVec / SimpleTriggerVec


#include "../int_vec.h"             // IntVec
#include "../inventory_panel/game_tick.h"   // GameData::sun_color 0x5176e0, fog_color 0x517820
struct E92;
#include "../game_globals.h"   // canonical g_module_settings.map_tree_types (0xa1815c), g_module_settings.map_sea_direction (0xa18144), g_module_settings.map_river_direction (0xa18150)
// the slices read these as ints (g_module_settings.map_sea_direction/river_direction are floats, same words)
#define g_map_tree_types_a ((int*)&g_module_settings.map_tree_types)                  // 0xa1815c map, 0xa18160 snow, 0xa18164 steppe, 0xa18168 desert
#define g_map_sea_direction_i (*(int*)&g_module_settings.map_sea_direction)
#define g_map_river_direction_i (*(int*)&g_module_settings.map_river_direction)
extern unsigned __int64 g_bit[]; // 0x5a8e68
void     noise_fractal(float a, float b, int c, int d, const float* in, float* out); // 0x414fc0
int      rand();                                                               // 0x536a9f

// ---- 0x523360..0x523630: the nibble setter, the terrain material table, the chain builder, the array scan ---------------
// the mesh (rgl/mesh.h) under the name of its material-nibble setter: bits 24..27 of res->flags (skin +0x1c) are a 4-bit
// colour / alpha nibble
struct NibbleObj : rgl::mesh {
    __declspec(noinline) void set_nibble(int v);                                     // 0x523360 (thiscall, ret 4)
};
#include "../terrain_b/nav_data.h"                            // NavData::build_chain 0x5234e0 (NavCell, NavEdge)

// ---- thin views for build_scene (0x520d60) -------------------------------------------------------------------------
#include "../../rgl/scene/world.h"                                  // rgl::world: erase_entity 0x41f450 (ret 4)
namespace rgl { struct mesh; }
struct Poser { void add_extra(rgl::mesh* x); };                            // 0x442110 (agent3/agent_fx_sound.cpp; ret 4: the mesh pointer is its argument)
#include "../../include/party.h"                                    // PartyMgr (Game +0xf8, game.h's Scene): first_party_id 0x460d20, next_party_id 0x460ca0
#include "../event_pool.h"                  // EventPool::next 0x517960 (inventory_panel/game_tick.cpp; Game +0xcc is GameData::events)
#include "../../rgl/scene/scene_math.h"   // rgl::identity3 (0x40cd60)
// a tstring temporary handed to the resource manager as the rgl::string it is (rgl_string.h declares the same 16 bytes; its own constructor and
// destructor are not defined anywhere: tstring's, 0x42bbe0 / 0x42bc40, are)
struct RStr : tstring {
    RStr(const char* s) : tstring(s) {}
    operator const rgl::string&() const { return *this; }
};
#include "../env_row.h"                                      // EnvRow, g_env_rows (0x5e28f4)

#endif // MISSION5_GAME_2_H
