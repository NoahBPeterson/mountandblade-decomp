// e68.h: E68, the 0x68-byte element of the vector at src/game/editor/poly_mesh_edge_face_ops.cpp (vector<E68>, destructor calls the out-of-line
// free_materials 0x4c8330) and of the table walked by src/game/ui/widgets/widget_base.cpp (Vec68::count 0x48ace0: (end - begin) / 0x68).
// ONE definition for src/rgl/mesh2 and src/game/ui. The member at +0x38 is the shared DialogBlock (copy constructor 0x4bd7a0); the
// vector at +0x54 holds the block's materials (rgl skin*: a scalar element, its operator= is the memmove one at 0x43f0a0).
#ifndef MB_E68_H
#define MB_E68_H
#include <vector>
#include "engine/core/tstring.h"
#include "game/dialog/dialog_rec.h"
struct stream_base;
struct skin;


struct E68 {                                // 0x68: the map (tableau) block
    tstring s;                              // 0x00
    int b0, b1;                             // 0x10
    int b2, b3;                             // 0x18 the render size (width, height)
    int b4, b5;                             // 0x20 the render target size (quarter 0x489ad0: by the texture detail)
    float b6, b7, b8, b9;                   // 0x28 (metres: read in millimetres, 0x4c83a0)
    DialogBlock m;                          // 0x38
    std::vector<skin*> v;                   // 0x54 the tableau materials (key, refcount; free_materials releases them; copy constructor 0x4cc280)
    skin* t;                                // 0x64 the tableau material (0x4c83a0)
    E68() { b0 = -1; t = 0; }
    E68(const E68& o);                      // 0x4cd100 (poly_mesh_edge_face_ops.cpp)
    E68& operator=(const E68& o);           // 0x4cd2f0 (poly_mesh_edge_face_ops.cpp)
    void free_materials();                  // 0x4c8330 (src/game/editor/editor_selection_and_translation.cpp)
    void read_terrain(stream_base* stream); // 0x4c83a0 (src/game/editor/editor_selection_and_translation.cpp)
    skin* find_or_create(const int* key);   // 0x493df0 (game/campaign/game_data_party_quest.cpp, ret 4: the material of instance *key)
    void purge();                           // 0x493e90 (game/campaign/game_data_party_quest.cpp: releases the unreferenced materials)
    void quarter();                         // 0x489ad0 (game/ui/widgets/widget_base.cpp: b4 / b5 from b2 / b3 by g_cfg.texture_detail)
    skin* make_material(const int* key);    // 0x489b20 (game/ui/widgets/widget_base.cpp: a render target material copying t's look)
    ~E68() { free_materials(); }
};

#endif // MB_E68_H
