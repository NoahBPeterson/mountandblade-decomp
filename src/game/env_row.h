// env_row.h: the 0x4c-byte rows of the environment table at g_env_rows (0x5e28f4; data\skyboxes.txt, count g_num_ground_specs
// 0x5e29ec, loaded by options_panel/module_load.cpp): the name of the sky mesh, the time-of-day class and sun angle, the
// camera focus / wind vector and the ambient base vector, the fog colour and the ambient-sound selection score. ONE definition
// for terrain_b (setup_ambient, post_spawn), mesh_slot (start_setup), agent3 (effect spawn), agent5 (the lens flare), script
// (set_fog_distance), mission5 (the map object) and options_panel (the loader). The two vectors are plain float arrays: the
// loader allocates the table with array new, which must construct nothing but the name.
#ifndef GAME_ENV_ROW_H
#define GAME_ENV_ROW_H
#include "./tstring.h"
namespace rgl { struct vector4; }
struct EnvRow {
    tstring name;                                       // +0x00 the sky mesh (resmgr get_object)
    int f10;                                            // +0x10 bits 0..1 time-of-day class, 4..7 season (start_setup), 0x10000000 no wind (setup_ambient)
    float f14;                                          // +0x14 the sun's base angle in degrees (start_setup)
    float sun_angle;                                    // +0x18 degrees (get_dir)
    float size;                                         // +0x1c lens flare / sky size
    float v20[4];                                       // +0x20 camera focus (x, y, z) / wind strength per axis
    float base[4];                                      // +0x30 ambient base vector (the 4th lane is read by post_spawn)
    float f40;                                          // +0x40
    unsigned f44;                                       // +0x44 ambient / fog colour
    float rank;                                         // +0x48 the ambient-sound selection score (start_setup)
    rgl::vector4 get_dir(int hour, int day);            // 0x463610 (returns by hidden pointer; ui/troop_inv.cpp)
};
typedef char assert_EnvRow_size[sizeof(EnvRow) == 0x4c ? 1 : -1];
extern EnvRow* g_env_rows;                              // 0x5e28f4
#endif // GAME_ENV_ROW_H
