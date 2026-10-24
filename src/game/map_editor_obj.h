// map_editor_obj.h: the call view of the application object at 0x8ee3c48 (g_app_obj, the Core of src/game/config/core.h;
// the map editor's dialog class of that same object is MapEditorDialog, src/rgl/mesh2/gui_edit.h). ONE definition of the
// slices' "MapEditor" for src/game/char_sheet (facegen_misc: the editor restart and the device-object handlers
// 0x4c1a60 / 0x4c1940, called on the global) and src/game/inventory_panel (game_app: the dialog refreshers). The full
// classes cannot be included next to these TUs' local universes (gui_edit.h's Game / Widget views clash), so the
// members the callers use are declared here, each at the address that proves it.
#ifndef MB_GAME_MAP_EDITOR_OBJ_H
#define MB_GAME_MAP_EDITOR_OBJ_H

struct MapEditor {                                      // instance at 0x8ee3c48
    void restart();                                     // 0x4c0520 (load_game_variables, trigger_vec.cpp)
    void func_4c1a60();                                 // 0x4c1a60 (Core::InvalidateDeviceObjects)
    void func_4c1940();                                 // 0x4c1940 (Core::RestoreDeviceObjects)
    void update_object_type();                          // 0x4c7bf0 (MapEditorDialog::update_object_type)
    void update_objects();                              // 0x4c7e20 (MapEditorDialog::update_objects)
};

#endif // MB_GAME_MAP_EDITOR_OBJ_H
