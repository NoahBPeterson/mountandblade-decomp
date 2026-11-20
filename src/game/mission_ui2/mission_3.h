// mission_3.h: classes of mission_3.cpp (0x48c330..0x48e5d0): views of the objects whose members sit in that object file and
// that no shared header declares (the game data, party, troop, item, quest and faction members are declared in the shared
// headers src/include/{game_data,party,troop,item_kind,game_recs}.h and src/game/{item_ref,slot_array}.h; the widget
// setters in src/game/config/widget_mgr.h). Names provisional; every offset cites the function that reads it.
#ifndef GAME_MISSION_UI2_MISSION_3_H
#define GAME_MISSION_UI2_MISSION_3_H
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../ui/mission_ui.h"

// ---- 0x48cec0 / 0x48cf40: the current site's records read the trigger event context, GameData::event_ctx (include/game_data.h) ----

// ---- 0x48cfc0: a container of object lists ------------------------------------------------------------------------------
#include "../../rgl/core/resmgr.h"     // lod_group::set_material 0x48cfc0 (defined here)

#include "./field_panel.h"

// ---- 0x48d5f0, 0x48d690: containers of 0x1dc-byte elements and of 8-byte records ---------------------------------------------
struct ArrInner {
    char pad0[0xb0];
    std::vector<int> vals;                  // +0xb0 (first at +0xb4; 0x48d636)
};
struct ArrElem {
    virtual int v00();
    virtual int v04();
    virtual int v08(int arg);
    char pad8[0x19c - 4];
    std::vector<ArrInner*> list;            // +0x19c (first at +0x1a0, last at +0x1a4)
    char pad1ac[0x1dc - 0x1ac];
};
struct ArrHolder {
    char pad0[4];
    ArrElem* base;                          // +4
    int lookup(int index, int arg);         // 0x48d5f0
};
struct Pair690 { int a; int b; };

struct FieldGroup { int n; char pad[0x68]; };     // a 0x6c-byte force-group record: the member count first (0x48d940)
// ---- 0x48d940: a force label ---------------------------------------------------------------------------------------------
struct ForceLabel {
    char pad0[0x4c];
    int n0;                                 // +0x4c member count of the first 0x6c-byte force group (infantry)
    char pad50[0xb8 - 0x50];
    int n1;                                 // +0xb8 second group (archers)
    char padbc[0x124 - 0xbc];
    int n2;                                 // +0x124 third group (cavalry)
    void make_label(char* out);             // 0x48d940
};

// ---- 0x48db20..0x48e590: the overlay setters take the manager's MissionUi* (tracks[], color14c/150, v168, f16c/f170 range)
// and, by kind(), the derived widget: 3 UiC (f194, mask), 8 XyzButton (f180, f184), 5 DropDown (append, slot 50).


#endif // GAME_MISSION_UI2_MISSION_3_H
