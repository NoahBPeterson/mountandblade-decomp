// records.h: the engine record classes whose destructors / deleting destructors sit at 0x431400..0x432080,
// and the std::vector<Item> helpers they use. One header for records.cpp. Class and field names are
// provisional; only sizes, offsets and destruction orders are verified (by the byte-exact destructors cited at
// each class). The classes are only seen through `delete`/`delete[]` here: the objects are created by later
// objects (0x4c1c90, 0x4c3430, 0x4da5b0, ...), so most fields are padding.
//
// tstring is the shared src/game/tstring.h (16 bytes, dtor 0x42bc40). IfaceB/Missile/Mission-like classes of
// src/game/agent/agent.h are NOT reused: agent.h declares Missile's constructor only, but the vector deleting
// destructor 0x431750 is emitted here, from a definition that has the constructor. See MIGRATION.md.
#ifndef RGL_XMLCFG_RECORDS_H
#define RGL_XMLCFG_RECORDS_H
#include <new>
#include <vector>
#include <list>
#include "../../game/tstring.h"

#include "../../include/rec_dtors.h"   // OwnedRec48 (shared)
#include "../../include/rec34.h"       // rec34: the vector deleting destructor 0x4318c0

// IfaceA / IfaceB (the polymorphic bases; vtables 0x5908ec and 0x5908f8, slots 1 and 2 the empty function 0x477b70) are in include/iface.h.
#include "../../include/iface.h"        // IfaceB (shared; its constructor is inline, the vtable is emitted by the first user below)

// RecG, the 0x30-byte element with one pair at +4 (vector deleting destructor 0x431540), is the sound holder's trigger_entry
// (include/trigger_entry.h: interval, a DialogBlock, a Timer).
#include "../../game/trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list
// 0x30 bytes: name at +0, pair at +0x10 with an unconditional delete[]: 0x4315d0.
#include "../../include/anim_set.h"   // AnimDef (was RecH): the 0x30-byte animation whose vector deleting destructor is 0x4315d0
// A class whose only member is a tstring: its destructor is a tail jump to 0x42bc40 (0x4316b0).
struct NameHolder {
    tstring name;
    ~NameHolder();
};
// 0x4316c0 is the vector deleting destructor of game/event.h's Event (the slot 0 of its vtable 0x5922cc; world_data.cpp's
// `new Event[16]` emits it, where its row is)
#include "../../game/missile.h"   // Missile, the record of Pool<Missile> (Mission::poolJ)

// ---- from 0x431960 on --------------------------------------------------------------------------------------
// Polymorphic owned object (only its virtual destructor is called): src/include/poly.h
#include "../../include/poly.h"
// The std::vector helpers 0x431a30 / 0x431a60 are std::vector<PlacedObj>'s (game/placed_obj.h, records.cpp).
// out-of-line destructors of later objects, called on members
#include "../../include/tail2.h"             // Tail2 (0x20 bytes, ~Tail2 0x4c4d10)

// 0x431ab0 / 0x431d30 are rgl::cell's destructor and vector deleting destructor (rgl/scene/world.h: the grid's cell, 0x34
// bytes; the rows are game/terrain_b/poly_mesh.cpp's, which defines ~cell with the grid's destructor 0x475c60)
// (the 0x5ac8-byte IfaceA-derived record is Party, include/party.h: its ~Party 0x431b90 and vector deleting destructor 0x431cb0 are emitted with its constructor in game/world_data)
// (the 0x494-byte IfaceB-derived scene record, vtable 0x590904, is SceneFile, game/terrain_b/scene_file.h: its ~SceneFile 0x431e10 and vector deleting destructor 0x431ec0 are emitted with its constructor 0x5248e0, in game/mission5/game_2.cpp)
// 0x432080 is Agent::reset_path (game/agent_obj.h), defined in records.cpp

#endif
