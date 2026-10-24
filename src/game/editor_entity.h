// editor_entity.h: the current editor entity (pointer at 0xa739b8). One definition for inventory_panel/game_app.cpp
// (0x50f130 calls pick and sets f2f9) and map_editor/map_editor.h (the per-frame update 0x5305b0 reads f2f9);
// both name the same object by its global 0xa739b8 and agree on the flag at +0x2f9.
#ifndef GAME_EDITOR_ENTITY_H
#define GAME_EDITOR_ENTITY_H
struct EditorEntity {
    void pick_entity();                                         // 0x4ed8d0
    char pad2f9[0x2f9];
    char f2f9;                                                  // 0x2f9
};
#endif
