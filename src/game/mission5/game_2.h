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
struct GameData { const char* module_path(const char* sub); void init_event(struct E92* r); char emit_event(int id, int a, int b); };   // 0x4639d0, 0x4b36a0 (include/game_data.h: Game is the game-data object)
#define MODULE_PATH(sub) (((GameData*)this)->module_path(sub))

// (item, modifier word) pairs of the statement / troop / scene records are the shared ItemRef (../item_ref.h; item -1 = none)

// triggers.txt statement (0x98-byte element, allocated with new[])
struct StmtRec {
    int kind;                       // +0x00
    char pad04[4];
    int f08;                        // +0x08
    char pad0c[4];
    __int64 f10;                    // +0x10
    char pad18[0x78];
    int f90;                        // +0x90
    char pad94[4];
    StmtRec() : f90(-1) {}
};
struct TriggerStmt {                // the 0x2c-byte statement of a simple trigger (at +4 of its 0x30-byte record)
    StmtRec* p;                     // +0
    int n;                          // +4
    char flag;                      // +8
    char pad9[0x2c - 9];
    __declspec(noinline) void init(int op, int idx);     // 0x524360 (ret 8)
};
typedef char TriggerStmt_size_check[sizeof(TriggerStmt) == 0x2c ? 1 : -1];
struct SimpleTrigger {              // 0x30 bytes: SimpleRec of game.h
    int f0;                         // +0 (the emitter delay, bit-copied)
    TriggerStmt stmt;               // +4
};
typedef char SimpleTrigger_size_check[sizeof(SimpleTrigger) == 0x30 ? 1 : -1];

// skills.txt record: SkillRec (include/skill_rec.h), table g_skill_tbl 0x8ee3100
#include "../skill_cap.h"
__declspec(noinline) stream_base& __cdecl read_skill(stream_base& s, SkillRec& r);     // 0x5243c0

// ---- the Data\*.txt record classes: TroopFile / QuestFile / FactionFile are data_files.h's; SceneFile (terrain_b/scene_file.h) below ---------
// the scene record's embedded passage / site writer stream (vtable slots 3, 7 and 8 of stream_base)
struct V0 { int a, b, c; char pad[0xc]; };                      // 0x18
struct V1 { int p0, p1, p2, p3, p4; char pad[0x20]; };          // 0x34
struct Rec64 {                                                  // 0x64
    char pad[0x28];
    int a[4];                                                   // +0x28
    int b[4];                                                   // +0x38
    int n;                                                      // +0x48
    char pad2[0x10];
    int c;                                                      // +0x5c
    char pad3[4];
};
#include "../terrain_b/scene_file.h"       // SceneFile (the 0x494-byte scene record; shared with terrain_b/mission.cpp)
// the temporary collection SceneRec::read fills through the embedded stream (0x524f00)
#include "../../rgl/mesh2/graph_obj.h"                         // graph_obj::clear_vecs 0x4cb780
struct growable_array { growable_array(); };                  // 0x4d1360 (options_panel/module_load.h: the same 0x94-byte object; only its constructor is called here)
struct Local : growable_array {
    __forceinline Local() {}
    char pad0[0x1c];
    V0* v0_first; V0* v0_last;                                  // +0x1c / +0x20
    char pad1[8];
    Rec64* v2_first; Rec64* v2_last;                                  // +0x2c / +0x30
    char pad2[8];
    V1* v1_first; V1* v1_last;                                  // +0x3c / +0x40
    char pad3[0x94 - 0x44];
    unsigned size0() const { return v0_first == 0 ? 0 : (unsigned)(v0_last - v0_first); }
    unsigned size1() const { return v1_first == 0 ? 0 : (unsigned)(v1_last - v1_first); }
    unsigned size2() const { return v2_first == 0 ? 0 : (unsigned)(v2_last - v2_first); }
    __forceinline ~Local() { ((graph_obj*)this)->clear_vecs(); }   // 0x4cb780: the object is the editor's mesh graph
    void read(stream_base& s);                                  // 0x524f00
};

// ---- script emitters (0xa75eb0 array, count 0xa75eb4) ------------------------------------------------------------
#include "../../include/rec34.h"                 // rec34_table, g_scripts (0xa75eb0)

struct SkeletonBody { void load(const tstring& path); };           // 0x50fc60 (inventory_panel/skeleton_load.cpp; ret 4)
struct RegSkeleton;
void __fastcall fix_skeletons(std::vector<RegSkeleton>* list);       // 0x4cd850 (rgl/mesh2/mesh_2.cpp)
extern std::vector<RegSkeleton> g_skeletons;               // 0xa180e8 skeleton-body table (rgl/mesh2/mesh_2.cpp)

// trigger vectors at Game +0x139e8 / +0x139f4 (game.h: TriggerTag / SimpleTag, whose padding these resize)
#include "../trigger_vecs.h"           // TriggerVec / GameVec


// ---- the campaign map's tree builder (0x5213d0) -------------------------------------------------------------------
// Views of the ground graph (CellGrid::core, a NavData): NavVert / NavCell of cell_grid.h with the flat field names this
// function reads, and the third vector (+0x20, 0x24-byte edge records) that NavData declares as ints.
struct MapNavVert {                    // 0x24 bytes = NavVert
    float x, y, z;                   // +0x00
    char  pad0c[4];
    float nx, ny, nz;                // +0x10 normal
    char  pad1c[4];
    float s;                         // +0x20 dot with the view direction
};
struct MapNode {                     // 0x88 bytes = NavCell
    int   count;                     // +0x00
    int   idx[8];                    // +0x04 vertex indices
    int   type;                      // +0x24 terrain mask (NavRec::mask)
    char  pad[0x88 - 0x28];
};
struct MapEdge {                     // 0x24 bytes
    char  pad0c[0xc];
    int   a;                         // +0x0c
    int   b;                         // +0x10
    char  pad14[0x24 - 0x14];        // the tail after b is 0x10 bytes: `0x24 - 0x18` made the record 0x20,
};                                   // so the 0x24-stride edge loops drifted 4 bytes per record and read an `a` of -1
typedef char chk_map_edge_size[sizeof(MapEdge) == 0x24 ? 1 : -1];
struct MapNav {                      // NavData
    int      pad0;
    MapNavVert* pts;                   // +0x04
    MapNavVert* pts_end;               // +0x08
    char     padc[8];
    MapNode* nodes;                  // +0x14
    MapNode* nodes_end;              // +0x18
    char     pad1c[8];
    MapEdge* edges;                  // +0x24
    MapEdge* edges_end;              // +0x28
};
#include "../int_vec.h"             // RawVec: std::vector<T*> triple with its allocator dword (stride 0x10, g_mat_vec)
// The parts of Game that game.h leaves as padding and the build_scene / build_map_trees members that use them.
struct GameMapView {
    char   pad0[0x2c];
    rgl::mesh* map_track;            // +0x2c  0x520d60
    rgl::entity* track_entity;       // +0x30
    std::vector<int> ents;           // +0x34  std::vector<mesh*> (0x5213d0)
    rgl::mesh* map_obj;              // +0x44
    rgl::vector4 fog1;               // +0x48
    rgl::vector4 fog2;               // +0x58
    rgl::vector4 fog3;               // +0x68
    char   pad78[0x139dc - 0x78];
    Light* wind;                     // +0x139dc (the fog / sun object; +0x44 = its direction)
    char   pad139e0[0x1a2a8 - 0x139e0];
    rgl::entity* compass;            // +0x1a2a8
    // the map builder's members whose bodies never read `this`: the call sites load ecx (0x5217a2, 0x522ffe), both end in ret N
    __declspec(noinline) void scan_arrays(void* a, void* b);                              // 0x523630
    __declspec(noinline) skin* terrain_material(int type);                                // 0x523390
};
struct GameG { rgl::vector4* sun_color(rgl::vector4* out); rgl::vector4* fog_color(rgl::vector4* out); };   // 0x5176e0, 0x517820 (inventory_panel/game_tick.h: the game object)
namespace c49bea0 { struct MissionAgents { void add_order_arrow(int id); }; }     // 0x49bea0 (ret 4: one int arg; party2/camera.cpp)
namespace c49b630 { struct Mission { void spawn_battle_icon(unsigned id); }; }              // 0x49bbb0 (party2/camera.cpp)
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
struct FlagNode { char pad[0x1c]; unsigned f1c; };              // +0x1c: bits 24..27 are a 4-bit colour / alpha nibble
struct NibbleObj {
    char pad[0x2c];
    FlagNode* f2c;                                              // +0x2c
    __declspec(noinline) void set_nibble(int v);                                     // 0x523360 (thiscall, ret 4)
};
struct Rec88 { int count; char pad4[0x14 - 4]; int nb[1]; char pad18[0x88 - 0x18]; };
typedef char Rec88_size_check[sizeof(Rec88) == 0x88 ? 1 : -1];    // the 0x88-byte graph node (NavCell): neighbour ids at +0x14
struct Node24 { int f00, f04, f08, f0c, f10; char pad14[0x24 - 0x14]; };   // the 0x24-byte edge record
struct ChainGraph {
    char pad0[0x14];
    Rec88* f14;                                                 // +0x14
    char pad18[0x24 - 0x18];
    Node24* f24;                                                // +0x24
    void build_chain(int target, int start, int* out, int* count);   // 0x5234e0 (ret 0x10)
};
struct SceneArrays {
    char pad0[4];
    char* a_first;                                              // +0x04 (elements of 0x24)
    char* a_last;                                               // +0x08
    char padc[0x14 - 0xc];
    char* b_first;                                              // +0x14 (elements of 0x88)
    char* b_last;                                               // +0x18
    int a_size() const { return a_first ? (int)((a_last - a_first) / 0x24) : 0; }
    int b_size() const { return b_first ? (int)((b_last - b_first) / 0x88) : 0; }
};

// ---- thin views for build_scene (0x520d60) -------------------------------------------------------------------------
#include "../../rgl/scene/world.h"                                  // rgl::world: erase_entity 0x41f450 (ret 4)
struct Poser { void add_extra(int x); };                            // 0x442110 (agent3/agent_fx_sound.cpp; ret 4: the mesh pointer is its argument)
#include "../../include/party.h"                                    // PartyMgr (Game +0xf8, game.h's Scene): first_party_id 0x460d20, next_party_id 0x460ca0
struct GamePool { void next(int* idx); };                           // 0x517960 (inventory_panel/game_tick.h; ret 4: SpriteTable at Game +0xcc)
#include "../../rgl/scene/scene_math.h"   // rgl::identity3 (0x40cd60)
// a tstring temporary handed to the resource manager as the rgl::string it is (rgl_string.h declares the same 16 bytes; its own constructor and
// destructor are not defined anywhere: tstring's, 0x42bbe0 / 0x42bc40, are)
struct RStr : tstring {
    RStr(const char* s) : tstring(s) {}
    operator const rgl::string&() const { return *this; }
};
#include "../env_row.h"                                      // EnvRow, g_env_rows (0x5e28f4)

#endif // MISSION5_GAME_2_H
