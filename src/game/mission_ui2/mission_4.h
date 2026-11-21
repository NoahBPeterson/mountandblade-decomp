// mission_4.h: classes of mission_4.cpp (0x48ef40..0x48f790): the container frame, the rider table, the tableau stack and the
// widgets as the text / position setters see them. Names provisional; every offset cites the function that reads it. The
// view's (Camera) members are in party2/camera_class.h, the party's in src/include/party.h, the game data's in src/include/game_data.h,
// the widget manager's in src/game/config/widget_mgr.h.
#ifndef GAME_MISSION_UI2_MISSION_4_H
#define GAME_MISSION_UI2_MISSION_4_H
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../ui/mission_ui.h"
#include "../../include/frustum.h"
#include "../party2/camera_class.h"      // Camera: the scrolling view whose layout members these objects define
#include "../terrain_b/nav_data.h"        // NavData::shares 0x48efa0 (the cells of the ground / nav data)

// ---- 0x48ef40: the container widget's release is ContainerBase::update_children (ui/container.h) ----------------

// ---- 0x48efa0: NavData::shares (terrain_b/nav_data.h) ----

// ---- 0x48f450..0x48f5a0: the tableau on top of the stack of tableaus being rendered (TableauTbl::worlds, include/tableau_table.h) ----------
#include "../world_data/scene_object.h"     // SceneObject: the tableau scene (camera, background, ambient_light)

#endif // GAME_MISSION_UI2_MISSION_4_H
