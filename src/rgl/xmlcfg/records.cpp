//-----------------------------------------------------------------------------
// File: records.cpp  (0x431400..0x4320af)
//
// Desc: compiler-generated scalar/vector deleting destructors and destructor bodies of engine record classes
//       (each is a COMDAT the linker placed here), the std::vector<Item> helpers they use, and the reset of
//       a big per-mission object. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <new>      // declares operator delete[]: without it cl 13.10 frees arrays with plain operator delete
#include "../../include/trigger_entry.h"   // trigger_entry: the simple trigger (SimpleTriggerVec's element)
#include "records.h"
#include "../../game/agent_obj.h"          // Agent (reset_path 0x432080)
#include "../../game/placed_obj.h"         // PlacedObj (std::vector<PlacedObj>::_Destroy 0x431a30, _Tidy 0x431a60)
#include "../../game/mission5/data_files.h"   // TroopFile / QuestFile / FactionFile (~TroopFile 0x431c40, ~QuestFile 0x431b10, ~FactionFile 0x432010)
#include "../../include/poly.h"      // (named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/rec_dtors.h"      // (listed directly: the resolution audit scans only the source's own includes)

// Only the two members the original has here (the vector of the mission's placed objects, Mission::props): instantiating
// the whole class also emitted PlacedObj's copy constructor, which the original does not have out of line. PlacedObj's
// destructor is implicit (the deleting one is 0x437870), reached through its vtable.
template void std::vector<PlacedObj>::_Destroy(PlacedObj*, PlacedObj*);   // 0x431a30
template void std::vector<PlacedObj>::_Tidy();                            // 0x431a60

// 0x431400 is the scalar deleting destructor of IfaceA; the compiler emits it with the vtable 0x5908ec of ~Party (game/world_data/world_data.cpp, where its row is)

// 0x431440 / 0x431480 are TriggerRec's implicit destructor and vector deleting destructor (include/trigger_rec.h); the compiler
// emits them with TriggerVec::resize in game/triggers/trigger_vec.cpp, where their rows are
// @ 0x431540
void delete_recg_array(trigger_entry* p) { delete[] p; }
// @ 0x4315d0
void delete_rech_array(AnimDef* p) { delete[] p; }
// 0x431660 is menu_item's implicit destructor (game/menu_rec.h; row in game/triggers/trigger_vec.cpp)
// @ 0x4316b0
NameHolder::~NameHolder() {}

// The constructors only make cl emit the class's vtable and with it the vector deleting destructors
// (slot 0 of the real vtables).
RecF::RecF() {}
RecF* make_recf_array(int n) { return new RecF[n]; }     // makes cl emit ??_E (needs a `new[]`)
// @ 0x4316c0
void delete_recf_array(RecF* p) { delete[] p; }

Missile::Missile() {}
Missile* make_recj_array(int n) { return new Missile[n]; }
// @ 0x431750
void delete_recj_array(Missile* p) { delete[] p; }

// @ 0x4317b0
SimpleTriggerVec::~SimpleTriggerVec()
{
    if (data) {
        delete[] data;
        data = 0;
    }
}
// 0x431810 is menu_item's vector deleting destructor (row in game/triggers/trigger_vec.cpp)
// @ 0x4318c0
void delete_reck_array(RecK* p) { delete[] p; }

// @ 0x431960
void delete_reca_array(OwnedRec48* p) { delete[] p; }
// @ 0x431ab0
RecP::~RecP() {}
// @ 0x431b10
QuestFile::~QuestFile() {}

// 0x431b90 / 0x431cb0 are ~Party and its vector deleting destructor (game/world_data/world_data.cpp: the compiler emits them with the constructor 0x4934d0)
// @ 0x431c40
TroopFile::~TroopFile() {}
// @ 0x431d30
void delete_recp_array(RecP* p) { delete[] p; }

// 0x431e10 / 0x431ec0 are ~SceneFile and its vector deleting destructor (game/mission5/game_2.cpp: the compiler emits them with the constructor 0x5248e0)

// 0x431f40 is the vector deleting destructor of the mission agent, MAgent (vtable 0x591848); the compiler emits it with MAgent's vtable
// in game/terrain_b/terrain_b.cpp (MAgent::MAgent), where its row is.

// @ 0x432010
FactionFile::~FactionFile() {}

// @ 0x432080
void Agent::reset_path()
{
    ai.path_state = 0;
    ai.path_flag = 0;
    ai.path_count = 0;
    ai.path_active = 0;
    ai.goal_changed = 0;
}
