// party_2.h: classes and externs of party_2.cpp (0x4b1160..0x4b8f40): the tableau render callback, the map party's
// economy (stacks, skills, morale, speed), its orders and path following on the world map, the battle records between two
// parties, the world-map path finder and the STL helper instances the linker kept from this object.
// Shared classes are the canonical ones: Party (include/party.h, with the map-party fields of this module added there),
// GameData (include/game_data.h; the fields it lacks are reached by offset through GD()), Troop / ItemRef / ItemKind, NavData
// / NavCell / CellGrid (terrain_b/cell_grid.h; the 0x88-byte NavCell carries the path finder's accessors), face (shape.h).
// Everything declared here exists nowhere else in src/. Every offset cites the function that proves it. Names provisional.
#ifndef GAME_PARTY2_PARTY_2_H
#define GAME_PARTY2_PARTY_2_H
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <vector>
#include "../terrain_b/terrain_b.h"
#include "../../include/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical g_module_settings.skill_leadership_bonus
#include "../ui/mission_ui.h"
#include "../emit.h"
#include "../config/widget_mgr.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/pod_elems.h"
#include "../../include/ptr_tag.h"
#include "../../include/shape.h"
#include "../../include/dialog_rec.h"
#include "../../include/vec.h"
#include "../hnode.h"     // nav_node / node_gt: adjust_heap_node 0x438b10 (agent2/agent_stats.cpp) takes the open-list node and comparator under these names
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

using rgl::vector4;

#define MB_NOINLINE __declspec(noinline)    // (tools/difftest/slice.py reads the name before a '__declspec(' as the function name)

// a field of the game data that the shared GameData does not declare yet: GD(type, offset)
#define GD(T, off) (*(T*)((char*)this + (off)))

// ---- engine services and constants (other objects) ---------------------------------------------------------------------
void mb_warning(const char* fmt, ...);                  // 0x42cb80 (logs a message)
void mb_error(const char* fmt, ...);                    // 0x42cc80 (printf-style log)
float __cdecl frand();                                  // 0x40e400 (rand() % 15817 / 15817, in st0)

// ---- the callees' own classes: member-only stubs under the name of the class that defines them (the link needs the defining owner in the
// mangled name); the view classes below forward to them with `this` cast, which compiles to the same call ----------------------------
#include "../../rgl/mesh.h"      // rgl::mesh::clone 0x41c6d0 (the canonical class)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../alloc_construct.h"   // allocator<E12>::construct 0x436cf0 (agent/agent.cpp, ret 8)
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40)
struct vec2x {
    void rotate_a(float ang);   // 0x432600 (rgl/xmlcfg/game_util.cpp)
};
#include "../../rgl/scene/world.h"          // rgl::world: step 0x427f40, render_scene 0x429890, remove_entity 0x41f0d0, create_entity 0x428440, reset 0x421390, ground (NavData)

// ---- 0x5c-byte records and their helpers (vector<TrackRec>: the game's event / track records) ------------------------------
#include "../track_rec.h"                             // TrackRec: the 0x5c-byte event / track record (GameData::tracks, +0x13a60)
// a 0xc-byte heap element (min-heap on dist): the world-map search's open list, vector<E12>

// the allocators' construct: allocator<TrackRec>::construct 0x4b2160 is defined in party_2_copy.cpp (its own TU: it was once a stdcall
// copy, whose direct call is not spellable: cl would give the callers the TU-local callee's register knowledge); the open-list
// element has the layout of agent.cpp's E12 and its construct is that one body (0x436cf0, ICF folded)
namespace std {
#ifndef PARTY2_CONSTRUCT_DEFINED_HERE
template<> void allocator<TrackRec>::construct(TrackRec* p, const TrackRec& v);
#endif
template<> inline void allocator<nav_node>::construct(nav_node* p, const nav_node& v) { allocator<E12>().construct((E12*)p, (const E12&)v); }
}
struct vec2r : vec2 {
    void rotate_a(float ang) { ((vec2x*)this)->rotate_a(ang); }       // 0x432600 (replace by the unit vector at `ang`)
};

// the vector iterator of an TrackRec array, returned through a hidden pointer (non-POD)
struct ItemIter {
    TrackRec* p;
    ItemIter(TrackRec* q) : p(q) {}
};
struct ItemArray {                                      // a container whose first member is the array base
    TrackRec* base;
    ItemIter at(int idx);                               // 0x4b22b0
};

// ---- tableaus: the render callback of a tableau material and the object table that hands the materials out ------------------
// the material object 0x493df0 builds for a tableau record is an rgl skin (rgl/resource.h): tex[0] is its render target,
// tableau the record index, on_render scene_step
void scene_step(skin* material);                        // 0x4b1160 (the render callback stored in skin::on_render)

// (a tableau's private world is game/world_data/scene_object.h's SceneObject: TableauTbl::push 0x495650 appends one)
#include "../world_data/scene_object.h"

// ---- map-party visuals ---------------------------------------------------------------------------------------------
// ---- the map party's model (an rgl entity of type 9) and the meshes it carries ---------------------------------------------
#include "./party_model.h"                             // Poser; Party::model is a rgl::strategic_entity
// a std::vector of 0x5c-byte records (allocator word, first, last, end)
struct EventVec {
    int al;
    TrackRec* first;
    TrackRec* last;
    TrackRec* end;
    int size() const;                                   // 0x4b1af0
};

// ---- the world map's formation records and the battle records ---------------------------------------------------------
inline NavCell* nav_recs() { return &((rgl::world*)g_script.level)->ground->cells[0]; }       // the record array (+0x14 of the nav data)

#include "../event.h"                 // Event: the battle record (bind_host / detach / commit / update_nav_anim are its members)
inline Pool<Event>* battles() { return &g_game->events; }                  // the battle records: GameData::events
inline Event* battle_get(unsigned i) { return battles()->get(i); }         // 0x48ef20
inline Party* party_at(PartyMgr* m, int k) { return (Party*)((k & 0xf) * sizeof(Party) + (char*)*(m->pages.begin() + (k >> 4))); }
inline Party* party_of(PartyMgr* m, int id) { return party_at(m, m->lookup(id)); }

#include "../../include/sound_mgr.h"                      // SoundMgr (play 0x442650)
extern SoundMgr g_sound_mgr;                            // 0x5aa128

extern "C" void __stdcall FSOUND_StopSound(int id);                      // 0x5342a6 (FSOUND_StopSound@4)

// ---- callees of other objects, declared with the argument lists the call sites push (see MIGRATION.md) ----------------------

#endif // GAME_PARTY2_PARTY_2_H
