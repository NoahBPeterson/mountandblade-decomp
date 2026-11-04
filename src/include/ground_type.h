// ground_type.h: the ground-type record and its table (g_ground_types, 0x8ed4df8: 11 records of 0x20 bytes up to the mesh table at
// 0x8ed4f58). ONE definition for terrain_a (the sco terrain description matches a layer's name against the table and copies a
// type's values into the layer), mesh_slot/mesh_terrain.cpp (the terrain flag), agent5/editor_dialog.cpp (the brush list shows the
// names), config.cpp (the reader of data\ground_specs.txt, 0x4be600) and options_panel/module_load.cpp (which defines the table and
// reads it in, the compiler constructing and destroying the 11 records).
#ifndef MB_GROUND_TYPE_H
#define MB_GROUND_TYPE_H
#include "../game/tstring.h"

struct stream_base;
struct GroundType {                         // 0x20 bytes
    tstring name;                           // +0x00 (0x464200, 0x4656c0)
    void* mat;                              // +0x10 the material (copied into a layer: 0x464f10 fc <- f10; 0x467250)
    void* mat2;                             // +0x14 the second material, 0 when the file says "none"
    int f18;                                // +0x18 a float of the file, copied as bits (0x464f10: layer f14 <- f18)
    int f1c;                                // +0x1c flags (bit 1: the terrain mesh counts it, 0x481aa1; copied into layer f4)
    void read(stream_base* s);              // 0x4be600 (game/config/config.cpp)
};
extern GroundType g_ground_types[11];       // 0x8ed4df8 (defined in game/options_panel/module_load.cpp)

#endif // MB_GROUND_TYPE_H
