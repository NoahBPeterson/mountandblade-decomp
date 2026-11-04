// mesh_table.h: MeshTable, the mesh records of meshes.txt (g_meshes 0x8ed4f58, 8 bytes): the {records, count} pair of rec_table.
// The records are 0x48 bytes (prsnt_record in game/config/config.h: a tstring at +0 and one at +0x14; the editor reads them as
// SceneRec48, set_background_mesh as MeshRec). The count (0x8ed4f5c) is the bound of the scripts' check_mesh 0x49cdf0.
#ifndef MB_MESH_TABLE_H
#define MB_MESH_TABLE_H
#include "./rec_table.h"

struct stream_base;
namespace rgl { struct mesh; }
struct MeshTable : rec_table {                      // 8 bytes
    ~MeshTable();                                   // 0x4c3f60 (rgl/mesh2/rec_tables.cpp): deletes the record array; the atexit destructor
    void load(stream_base* s);                      // 0x4c3fb0 (rgl/mesh2/rec_tables.cpp: reads the count and every record), ret 4
    rgl::mesh* placed(int index);                   // 0x499050 (game/world_data/obj_tbl.cpp: the clone of record `index`, placed)
};
extern MeshTable g_meshes;                          // 0x8ed4f58 (defined in rgl/mesh2/rec_tables.cpp)

#endif // MB_MESH_TABLE_H
