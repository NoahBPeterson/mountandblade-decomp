//-----------------------------------------------------------------------------
// File: record_destructors.cpp  (0x431400..0x4320af)
//
// Desc: compiler-generated scalar/vector deleting destructors and destructor bodies of engine record classes
//       (each is a COMDAT the linker placed here), the std::vector<Item> helpers they use, and the reset of
//       a big per-mission object. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <new>      // declares operator delete[]: without it cl 13.10 frees arrays with plain operator delete
#include "game/data/trigger_entry.h"   // trigger_entry: the simple trigger (SimpleTriggerVec's element)
#include "game/data/record_destructors.h"
#include "game/agent/agent_obj.h"          // Agent (reset_path 0x432080)
#include "game/mission/placed_obj.h"         // PlacedObj (std::vector<PlacedObj>::_Destroy 0x431a30, _Tidy 0x431a60)
#include "game/campaign/data_files.h"   // the std::vector<tstring> instances
#include "game/data/troop.h"           // Troop (~Troop 0x431c40)
#include "game/data/game_recs.h"       // QuestRec / FactionRec (~QuestRec 0x431b10, ~FactionRec 0x432010)
#include "engine/physics/poly.h"      // (named here too: the resolution audit reads the annotations of directly included headers)
#include "game/common/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/rec_dtors.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/data/ptype.h"             // AnimBox (vector deleting destructor 0x431960)

// Only the two members the original has here (the vector of the mission's placed objects, Mission::props): instantiating
// the whole class also emitted PlacedObj's copy constructor, which the original does not have out of line. PlacedObj's
// destructor is implicit (the deleting one is 0x437870), reached through its vtable.
template void std::vector<PlacedObj>::_Destroy(PlacedObj*, PlacedObj*);   // 0x431a30
template void std::vector<PlacedObj>::_Tidy();                            // 0x431a60

// 0x431400 is the scalar deleting destructor of IfaceA; the compiler emits it with the vtable 0x5908ec of ~Party (game/campaign/game_data_party_quest.cpp, where its row is)

// 0x431440 / 0x431480 are TriggerRec's implicit destructor and vector deleting destructor (game/data/trigger_rec.h); the compiler
// emits them with TriggerVec::resize in game/app/options_dialog_and_module_tables.cpp, where their rows are
// @ 0x431540
void delete_recg_array(trigger_entry* p) { delete[] p; }
// @ 0x4315d0
void delete_rech_array(AnimDef* p) { delete[] p; }
// 0x431660 is menu_item's implicit destructor (game/data/menu_rec.h; row in game/app/options_dialog_and_module_tables.cpp)
// @ 0x4316b0
NameHolder::~NameHolder() {}

// The constructors only make cl emit the class's vtable and with it the vector deleting destructors
// (slot 0 of the real vtables).
// 0x4316c0 is Event's vector deleting destructor (game/campaign/event.h; row in game/campaign/game_data_party_quest.cpp)
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
// 0x431810 is menu_item's vector deleting destructor (row in game/app/options_dialog_and_module_tables.cpp)
// @ 0x4318c0
void delete_rec34_array(rec34* p) { delete[] p; }

// @ 0x431960
void delete_anim_boxes(AnimBox* p) { delete[] p; }   // AnimBox's vector deleting destructor (../../game/data/ptype.h)
// 0x431ab0 is rgl::cell's destructor (engine/scene/world.h; row in game/terrain/poly_mesh.cpp)
// @ 0x431b10
QuestRec::~QuestRec() {}

// 0x431b90 / 0x431cb0 are ~Party and its vector deleting destructor (game/campaign/game_data_party_quest.cpp: the compiler emits them with the constructor 0x4934d0)
// @ 0x431c40
Troop::~Troop() {}
// 0x431d30 is rgl::cell's vector deleting destructor (row in game/terrain/poly_mesh.cpp)

// 0x431e10 / 0x431ec0 are ~SceneFile and its vector deleting destructor (game/campaign/map_builder_and_data_load.cpp: the compiler emits them with the constructor 0x5248e0)

// 0x431f40 is the vector deleting destructor of the mission agent, MAgent (vtable 0x591848); the compiler emits it with MAgent's vtable
// in game/terrain/terrain_scene_renderer.cpp (MAgent::MAgent), where its row is.

// @ 0x432010
FactionRec::~FactionRec() {}

// @ 0x432080
void Agent::reset_path()
{
    ai.path_state = 0;
    ai.path_flag = 0;
    ai.path_count = 0;
    ai.path_active = 0;
    ai.goal_changed = 0;
}
