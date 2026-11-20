// game_parts.h: member sub-objects of the game object shared by its two class definitions (party/game.h Game, the
// constructor's and loaders' view, and include/game_data.h GameData, the readers' view), so both lay them out alike at 64 bits.
#ifndef MB_GAME_PARTS_H
#define MB_GAME_PARTS_H
#include "../game/tstring.h"
namespace rgl { struct mesh; struct lod_mesh; }

// The script debugger's state (+0x1a060..+0x1a090): the F2 "Debug" / "Stop" toggle and the breakpoint the emitters test
// (GameData::emit_event 0x435790 tags a call (id << 16) | line when its script is the one named here).
struct ScriptDebugger {
    int enabled;                                // +0x1a060 debug mode is on: the debugger window is wanted (game_app.cpp sets it; trigger_vec.cpp 0x4bf870 clears it when ID_scripts.py is missing)
    int shown;                                  // +0x1a064 the debugger window is shown (game_app.cpp)
    int active;                                 // +0x1a068 the "Debug" / "Stop" toggle (trigger_vec.cpp 0x4bf870; the script interpreter's F2 key)
    tstring script;                             // +0x1a06c the script to break in (emit_event 0x435790; config.cpp 0x4bf870: the selected script)
    int line;                                   // +0x1a07c the selected operation index (CB_GETITEMDATA, 0x4bf870): the low word of the breakpoint tag
    tstring text;                               // +0x1a080 the whole script text (DialogBlock::renumber 0x49d930 searches it; config.cpp 0x4bb3f0 lists it)
};

// The context of the event a sound / particle trigger runs for (+0x1a1f0..+0x1a208): Mission::spawn_prop 0x44c210, the agent
// sounds (agent4/agent_logic.cpp), the item meshes (mesh_slot/mesh_scene.cpp), the party window (party_window.cpp 0x502b80);
// the trigger lookups 0x48cec0 / 0x48cf40 (mission_3.cpp) read it.
struct EventContext {
    int prop;                                   // +0x1a1f0 index of the current prop record, < 0 when none (0x48cec0)
    int agent;                                  // +0x1a1f4 agent id
    int troop;                                  // +0x1a1f8 its troop
    rgl::lod_mesh* key_a;                      // +0x1a1fc the item's lod mesh (0x48cec0 trigger_item: contexts -0x32, -0x33)
    rgl::mesh* mesh;                            // +0x1a200 the entity's first mesh (0x48cf40: contexts -0x32, -0x46)
    int slot;                                   // +0x1a204 the item slot
};

#endif // MB_GAME_PARTS_H
