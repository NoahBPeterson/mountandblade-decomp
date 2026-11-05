// records.h: the engine record classes whose destructors / deleting destructors sit at 0x431400..0x432080,
// and the std::vector<Item> helpers they use. One header for records.cpp. Class and field names are
// provisional; only sizes, offsets and destruction orders are verified (by the byte-exact destructors cited at
// each class). The classes are only seen through `delete`/`delete[]` here: the objects are created by later
// objects (0x4c1c90, 0x4c3430, 0x4da5b0, ...), so most fields are padding.
//
// tstring is the shared src/game/tstring.h (16 bytes, dtor 0x42bc40). IfaceB/RecJ/Mission-like classes of
// src/game/agent/agent.h are NOT reused: agent.h declares RecJ's constructor only, but the vector deleting
// destructor 0x431750 is emitted here, from a definition that has the constructor. See MIGRATION.md.
#ifndef RGL_XMLCFG_RECORDS_H
#define RGL_XMLCFG_RECORDS_H
#include <new>
#include <vector>
#include <list>
#include "../../game/tstring.h"

#include "../../include/rec_dtors.h"   // SubA, SubB (the (array, name) pairs), RecD, RecI, Tail, RecQ, RecS, RecT (shared)

// IfaceA / IfaceB (the polymorphic bases; vtables 0x5908ec and 0x5908f8, slots 1 and 2 the empty function 0x477b70) are in include/iface.h.
#include "../../include/iface.h"        // IfaceB (shared; its constructor is inline, the vtable is emitted by the first user below)

// RecG, the 0x30-byte element with one pair at +4 (vector deleting destructor 0x431540), is the sound holder's TimerChild
// (game/ui/timer_list.h: period, a DialogBlock channel, a Timer).
#include "../../game/ui/timer_list.h"
// 0x30 bytes: name at +0, pair at +0x10 with an unconditional delete[]: 0x4315d0.
struct RecH {
    tstring name;       // +0
    SubB s;             // +0x10
    int tail;
};
// A class whose only member is a tstring: its destructor is a tail jump to 0x42bc40 (0x4316b0).
struct NameHolder {
    tstring name;
    ~NameHolder();
};
// IfaceB-derived, 0x98 bytes (vector deleting destructor 0x4316c0).
struct RecF : IfaceB {
    int pad;
    tstring name;       // +0x14
    int tail[29];
    RecF();
};
#include "../../include/recj.h"        // RecJ (IfaceB-derived, 0xb0 bytes, vector deleting destructor 0x431750; shared)

// ---- from 0x431960 on --------------------------------------------------------------------------------------
// Polymorphic owned object (only its virtual destructor is called): src/include/poly.h
#include "../../include/poly.h"
// 0x1c8-byte polymorphic record in a std::vector (helpers 0x431a30 / 0x431a60): game/placed_obj.h's PlacedObj (vtable
// 0x590b84), whose destructor is implicit (deleting destructor 0x437870); only reached through the vtable here.
struct Item {
    virtual ~Item();
    char body[0x1c4];
};
// out-of-line destructors of later objects, called on members
#include "../../include/tail2.h"             // Tail2 (0x20 bytes, ~Tail2 0x4c4d10)

// 0x34 bytes: vector<int> at +0, list<int> at +0x10, vector<int> at +0x20. ~RecP 0x431ab0, ??_E 0x431d30.
struct RecP {
    std::vector<int> a;         // +0
    std::list<int> l;           // +0x10
    int pad1c;
    std::vector<int> b;         // +0x20
    int tail;
    ~RecP();
};
// (the 0x5ac8-byte IfaceA-derived record is Party, include/party.h: its ~Party 0x431b90 and vector deleting destructor 0x431cb0 are emitted with its constructor in game/world_data)
// (the 0x494-byte IfaceB-derived scene record, vtable 0x590904, is SceneFile, game/terrain_b/scene_file.h: its ~SceneFile 0x431e10 and vector deleting destructor 0x431ec0 are emitted with its constructor 0x5248e0, in game/mission5/game_2.cpp)
// 0x5340-byte object: the reset at 0x432080 clears five fields (offsets as stored there).
struct BigObj {
    char pad[0x5340];
    void reset();                   // 0x432080
};

#endif
