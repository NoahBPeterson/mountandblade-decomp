// container_and_tableau_misc.h: classes of container_and_tableau_misc.cpp (0x48ef40..0x48f790): the container frame, the rider table, the tableau stack and the
// widgets as the text / position setters see them. Names provisional; every offset cites the function that reads it. The
// view's (Camera) members are in game/mission/camera_class.h, the party's in src/game/party/party.h, the game data's in src/game/data/game_data.h,
// the widget manager's in src/game/ui/widgets/widget_manager.h.
#ifndef GAME_MISSION_UI2_MISSION_4_H
#define GAME_MISSION_UI2_MISSION_4_H
#include "engine/core/tstring.h"
#include "engine/math/rgl_math.h"
#include "engine/render/mesh.h"
#include "game/ui/widgets/widget_base.h"
#include "engine/scene/frustum.h"
#include "game/mission/camera_class.h"      // Camera: the scrolling view whose layout members these objects define
#include "game/terrain/nav_data.h"        // NavData::shares 0x48efa0 (the cells of the ground / nav data)

// ---- 0x48ef40: the container widget's release is ContainerBase::update_children (game/ui/widgets/container.h) ----------------

// ---- 0x48efa0: NavData::shares (game/terrain/nav_data.h) ----

// ---- 0x48f450..0x48f5a0: the tableau on top of the stack of tableaus being rendered (TableauTbl::worlds, game/data/tableau_table.h) ----------
#include "game/data/scene_object.h"     // SceneObject: the tableau scene (camera, background, ambient_light)

#endif // GAME_MISSION_UI2_MISSION_4_H
