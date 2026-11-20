// scene_object.h: SceneObject, the scene a tableau renders into (TableauTbl::worlds, include/tableau_table.h; 0x4a0 bytes,
// allocated by TableauTbl::push 0x495650, deleted by pop_back_delete 0x492ee0): an rgl::world (its default constructor
// 0x493ab0, game/world_data/world_data.cpp) with the tableau's camera, background colour and ambient light.
#ifndef GAME_WORLD_DATA_SCENE_OBJECT_H
#define GAME_WORLD_DATA_SCENE_OBJECT_H
#include "../../rgl/scene/world.h"
#include "../../include/frustum.h"

struct SceneObject : rgl::world {
    int override_flags;                     // +0x344 (cur_tableau_set_override_flags; spawn_party_scene 0x49f3d0 hands the
    int num_override_items;                 // +0x348  three to pick_slot_item 0x46b190)
    int override_items[8];                  // +0x34c (cur_tableau_add_override_item: no bound check)
    rgl::frustum camera;                    // +0x36c (set_camera_position 0x48f480, set_camera_parameters 0x48f4c0)
    int background;                         // +0x48c (set_background_color 0x48f570)
    rgl::vector4 ambient_light;             // +0x490 (set_ambient_light 0x48f5a0)
};
MB_LAYOUT32_ASSERT(scene_object_size_check, sizeof(SceneObject) == 0x4a0);

#endif // GAME_WORLD_DATA_SCENE_OBJECT_H
