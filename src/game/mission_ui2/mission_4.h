// mission_4.h: classes of mission_4.cpp (0x48ef40..0x48f790): the container frame, the rider table, the tableau stack and the
// widgets as the text / position setters see them. Names provisional; every offset cites the function that reads it. The
// panel's members are in field_panel.h, the party's in src/include/party.h, the game data's in src/include/game_data.h,
// the widget manager's in src/game/config/widget_mgr.h.
#ifndef GAME_MISSION_UI2_MISSION_4_H
#define GAME_MISSION_UI2_MISSION_4_H
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../ui/mission_ui.h"
#include "../../include/frustum.h"
#include "./field_panel.h"
#include "../terrain_b/nav_data.h"        // NavData::shares 0x48efa0 (the cells of the ground / nav data)

// ---- 0x48ef40: the container widget's release is ContainerBase::update_children (ui/container.h) ----------------

// ---- 0x48efa0: NavData::shares (terrain_b/nav_data.h) ----

// ---- 0x48f450..0x48f5a0: the tableau on top of the stack of tableaus being rendered (TableauTbl::worlds, include/tableau_table.h) ----------
struct TabCurrent {                         // the tableau on top of the stack
    char pad0[0x36c];
    rgl::frustum fr;                        // +0x36c the camera (0x48f480, 0x48f4c0)
    int background;                         // +0x48c (0x48f570)
    float ambient[4];                       // +0x490 (0x48f5a0)
};

#endif // GAME_MISSION_UI2_MISSION_4_H
