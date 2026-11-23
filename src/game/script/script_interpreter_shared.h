//-----------------------------------------------------------------------------
// File: script_interpreter_shared.h  (shared by operation_execute.cpp and script_support.cpp)
//
// Desc: forward declarations and the slice namespaces (c49b630: the sound manager view, c49bea0: the vector / matrix views of the
//       order code) that both parts of the old script_interpreter.cpp used, kept once.
//-----------------------------------------------------------------------------
#pragma once

namespace rgl { struct mesh; }

namespace rgl { struct matrix; } typedef rgl::matrix pos;

// ======================================================================================
// slice s0049b630 (namespace c49b630)
// ======================================================================================
namespace c49b630 {
// Slice s0049b630: six Mission methods from one translation unit.
//   [0x49b630] rebuild the mission's UI "font mesh" (material lookup + layout_text)
//   [0x49b720] add "value" to an army slot and, for slot 0, log the gained amount
//   [0x49b820] subtract "value" from an army slot (clamped at 0) and log the lost amount
//   [0x49b930] mark a quest record as taken (write +0x118/+0x11c, log ui_new_quest_taken)
//   [0x49baa0] layout one text row into a mesh with the non-wrap/scale preset
//   0x49bbb0: the battle icon for one 0x98-byte army record (Mission::spawn_battle_icon)
// Names and field names are provisional; every offset is read off the disassembly.
// The class layout is shared with the neighbouring slices s0049bea0 / s0049c2c0 (Mission).
// (tstring.h inlined below)


using rgl::vector4;
using rgl::matrix;
using rgl::mesh;

// ---- engine tables / globals ---------------------------------------------------------------------

struct SoundMgr { int play(int idx, int flags, const vector4* pos, float freq, int* out); };  // 0x442650


struct TickPair {                                   // g_ticks[i].lo + g_ticks[i].hi * 2^32
    int lo, hi;
    operator __int64() const { return ((long long)hi << 32) | (unsigned)lo; }
};

extern "C" int __cdecl sprintf(char*, const char*, ...);        // 0x536f8f

// ---- scene objects -------------------------------------------------------------------------------

// the map entity world::create_entity(9) builds: rgl::strategic_entity (engine/scene/scene_entity.h)
typedef rgl::strategic_entity EntityFull;



} // namespace c49b630

namespace c49b630 {
using rgl::vector4;
using rgl::matrix;
using rgl::mesh;

} // namespace c49b630

// ======================================================================================
namespace c49bea0 {
// Slice s0049bea0: 0x49bea0 (order arrow creation) and 0x49c170 (reinforcement slot check).
// 0x49bea0: look an agent/order slot up in the mission's agent table, create a scene object
//   (type 9) for it, give it a 0.3-radius sphere shape and an identity frame, refresh the order's
//   text entries and then attach the "ui_..." arrow label through 0x49baa0.
// 0x49c170: validate a reinforcement packet id against the current set and apply its flag bits
//   (reset the unit's copied frame, clear two counters, run 0x4677a0, then set the team slot flags).
// Names are provisional; every offset comes from these two functions and the neighbouring mission slices.

#define MB_NOTHROW throw()


struct vector4 { float x, y, z, w; };
struct matrix { float m[4][4]; };
inline matrix* identity_pos(matrix* out) { return (matrix*)rgl::identity_pos((rgl::matrix*)out); }   // 0x40cd00 (cdecl; returns out)



// scene object for one order entry (the mission order contour): rgl::strategic_entity (engine/scene/scene_entity.h)
typedef rgl::strategic_entity OrderObj;


struct OrderObjHost {
    virtual OrderObj* create_entity(int type);       // 0x428440 (ret 4)
};


// 0xa732b4 is g_level; the original calls the *direct* create_entity (0x49bea0: mov ecx,[0xa732b4]; push 9;
// call 0x428440), not a virtual slot - the object at g_level is not polymorphic (its first dword is data).
static OrderObj* order_create_entity(int type)
{
    return (OrderObj*)g_script.level->create_entity(type);
}

} // namespace c49bea0

// ======================================================================================
// slice s0049c2c0
// ======================================================================================
// Slice s0049c2c0: Mission::order (0x49c2c0, ~2.4 KB, `ret 8`). One `case` per army order: each of
// the three order slots belonging to the selected order leader is armed with a per-order state,
// the order's localised caption is fetched from the string table, and (only for the local agent's
// own order group) the caption plus the leader's order text is pushed to the message log. Finally
// every agent in the same order group gets its three order-parameter fields re-seeded from the
// global order table. `index` selects a leader (0..2), `order` the order (0..0xd).
// Names, and the two frame slots the compiler reads before writing on the advance/fall-back paths,
// are provisional.

#define MB_NOTHROW throw()

// the order leaders are the mission's TeamRec records (mission.h): cls[k] are the three order slots (ClassInfo; kind is the armed state,
// use the slot-in-use flag, pos the move target, target the extra word set by set_vec, nav the nearest face, dir the per-slot offset, sum the resting target)

// the agents are ::Agent (agent_obj.h): pos +0x50, team +0x1c4 (the order group, clamped to 7), the AI timer ai_ta() +0x2b0,
// ai_ta_interval +0x2c0, v56b4 (the follow direction); the table at mission+4 is ::AgentTable (pages: first at mission+0x1c)


struct TickPair { int a; int b; };

extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f
