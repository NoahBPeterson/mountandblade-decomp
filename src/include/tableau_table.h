// tableau_table.h: TableauTbl, the tableau table (g_tableaus 0xa75f40, 0x24 bytes): the 0x68-byte tableau records of
// tableau_materials.txt (a vector<E68>, first at +4), the stack of scene worlds the tableaus are rendered into (a vector of
// SceneObject*, first at +0x14; the last one is the tableau being rendered) and the alpha-mask flag (0xa75f60) the render callback
// 0x4b1160 clears and the script operation cur_tableau_render_as_alpha_mask sets.
// ONE definition for every module that reaches the table (the script interpreter, the tableau spawners, the party / inventory
// screens, the render callback, the loader). Defined in game/cls_map/cls_map.cpp, next to its destructor.
#ifndef MB_TABLEAU_TABLE_H
#define MB_TABLEAU_TABLE_H
#include <vector>
#include "./e68.h"

struct SceneObject;                                             // the world a tableau renders into (game/world_data/world_data.h, 0x4a0 bytes)
struct TabCurrent;                                              // the SceneObject as the camera setters see it (game/mission_ui2/mission_4.h)
struct TabSpawn;                                                // the material a record hands out (game/party2/party_2.h)
struct SlotReader;
namespace rgl { struct mesh; struct matrix; struct vector4; }

struct TableauTbl {                                             // 0x24 bytes
    std::vector<E68> recs;                                      // +0x00 the tableau records (first at +4)
    std::vector<SceneObject*> worlds;                           // +0x10 the worlds being rendered (first at +0x14)
    int alpha_mask;                                             // +0x20 (0xa75f60)
    ~TableauTbl();                                              // 0x4da9c0 (game/cls_map/cls_map.cpp): the atexit destructor

    void read(SlotReader* s);                                   // 0x4e7a70 (game/cls_map/progress_bar.cpp, ret 4): reads the records
    // the camera / colour setters of the tableau on top of the stack (game/mission_ui2/mission_4.cpp)
    TabCurrent* current();                                      // 0x48f450
    void set_camera_position(const rgl::matrix* m);             // 0x48f480 (ret 4)
    void set_camera_parameters(const char* perspective, const float* w, const float* h, const float* near_, const float* far_);   // 0x48f4c0 (ret 0x14)
    void set_background_color(int c);                           // 0x48f570 (ret 4)
    void set_ambient_light(const float* c);                     // 0x48f5a0 (ret 4)
    // world_data/world_data.cpp
    void pop_back_delete();                                     // 0x492ee0: destroys and removes the last world
    void purge_all();                                           // 0x493f30: purges the materials of every record
    void add_point_light(const rgl::matrix* p, const rgl::vector4& c);   // 0x493f80 (ret 8)
    void add_sun_light(const rgl::matrix* p, const rgl::vector4& c);     // 0x494020 (ret 8)
    SceneObject* push();                                        // 0x495650: appends a fresh world
    // world_data/obj_tbl.cpp
    rgl::mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0 (ret 8)
    void spawn(int index, int from, const rgl::matrix* frame, const rgl::vector4* scale);   // 0x4992b0 (ret 0x10)
    void spawn_scaled(int index, const rgl::matrix* frame, const rgl::vector4* scale, const float* alpha, const unsigned* color);   // 0x499390 (ret 0x14)
    void spawn_scaled2(int index, const rgl::matrix* frame, const float* scale);   // 0x499540 (ret 0xc)
    // party2/party_2.cpp
    TabSpawn* add_spawned(const int* tableau, const int* instance);   // 0x4b14b0 (ret 8): the material of the record for the instance
};
extern TableauTbl g_tableaus;                                   // 0xa75f40 (defined in game/cls_map/cls_map.cpp)

#endif // MB_TABLEAU_TABLE_H
