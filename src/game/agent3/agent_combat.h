// agent_combat.h: the classes of agent_combat.cpp (0x43e190..0x441360): the game font's glyph metrics, the
// element records of the std::vector / std::deque instantiations the linker kept from this object, the pool of
// 0xb0-byte records, and (in agent_types.h) the agent's combat helpers. Offsets are the ones read by a function
// of this module (the address is cited at each field). Names provisional.
#ifndef AGENT3_COMBAT_H
#define AGENT3_COMBAT_H
#include <vector>
#include <deque>
#include <memory>
#include <new>
#include <algorithm>
#include <math.h>
#include <hash_map>
#include "./agent_types.h"

// ---- collision shapes used by the hit tests: src/include/shape.h (capsule: the default constructor is inlined into
// 0x43f6d0, the (radius, a, b) one is 0x433310; contact is 0x24 bytes, point at +0x10) -------------------------------
#include "../font.h"

// the font's glyph table (GlyphMap, font.h) is a stdext::hash_map<int, Glyph> whose lookup is the library's own
// _Hash::lower_bound that src/game/agent2/agent_ai.cpp instantiates (0x43b210, with the mapped type its slice named HashVal)
struct HashVal { int v; };
typedef stdext::_Hash<stdext::_Hmap_traits<int, HashVal, stdext::hash_compare<int, std::less<int> >,
                                          std::allocator<std::pair<const int, HashVal> >, false> > IntHashBase;

// Callees the original shares with another object through identical-code folding: the linker serves both with the one
// function, whose definition is under the other class (declared here with the defining class's name, so the call binds
// its symbol; the object is cast at the call).
struct FxAnim { void update_poses(); void update_poses2(); };   // 0x422080 / 0x422130: src/rgl/dynamic/fx_anim.h (Skeleton::update_poses of skeleton.h is the same code)
struct level {
    void floor_at(const float* p, float* out_z, struct face** out_face);   // 0x439be0
    float floor_z(const float* p);                       // 0x439d40
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);   // 0x43ac40
    bool trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused);   // 0x439d60: agent2/agent_types.h (world::trace_ray of world.h)
};
#include "../../include/mat4.h"   // pos (transform_point 0x408460; posm's frame is the same 64-byte pos)

// ---- element types of the container instantiations -----------------------------------------------------------
#include "../../include/pod_elems.h"   // E8 / E12 / E100 / rec36 / rec136 (shared)
struct E48 { int v[12]; };                           // 0x30 (vector<E48>)
// 0x1c8-byte polymorphic record (vtable 0x590b84, copy ctor 0x437bb0, deleting dtor 0x437870): the shared Rec1c8
// (src/include/rec1c8.h; src/game/agent/agent.h calls it PlacedObj).
#include "../../include/rec1c8.h"

// ---- id pool: records handed out from pages of 16 (the agent table's allocator) ----------------------------------
// the 0xb0-byte pool record is RecJ (src/include/recj.h; IfaceB: f4 in-use flag, f8 id, fc serial number; ctor 0x4377d0)
#include "../../include/recj.h"
typedef RecJ PoolRec;           // the former name (agent4/agent_logic.cpp reads the pool pages through it)
struct Pool {
    int next_id;                // +0: ids handed out to pages so far
    std::vector<int> free_ids;  // +4
    std::vector<RecJ*> pages; // +0x14 (first at +0x18)
    int serial;                 // +0x24
    int alloc();                // 0x440c90 (thiscall, no args; returns the id)
};

// ---- blend / range tables and helpers called by the agent code ------------------------------------------------------
struct RangePair { float lo, hi; };
extern RangePair g_range_tbl[];                         // 0x590aac (indexed by Agent::variant)
float __cdecl signed_angle(const vec2* a, const vec2* b);   // 0x4381e0 (agent2/agent_types.h)
int __stdcall id_matches_dir(int id, int dir);          // 0x4326d0 (src/rgl/xmlcfg/game_util.h: static, this unused; Agent::id_matches_dir of agent_obj.h is the thiscall spelling)
float __cdecl accel_factor(const float* a, const float* b, const float* c);   // 0x435ae0
extern int g_ctl_mode;                                  // 0x5aa074 (0 = target, 1 = mouse, 2 = mirrored mouse)
extern int g_stick_mode;                                // 0x5aa078 (0 = none, 1 = stick)
extern int g_invert;                                    // 0x5aa01c
extern float g_stick_x;                                 // 0xa178e0
extern float g_stick_y;                                 // 0xa178e4
int __cdecl group_base(int id);                         // 0x432130
int __cdecl not_group_base(int id);                     // 0x4320b0

#endif // AGENT3_COMBAT_H
