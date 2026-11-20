// mission_obj.h: MissionObj (PropKind), the scene prop kind of scene_props.txt: the 0x78-byte record of the table g_propkinds
// (0x5e29dc, count g_propkind_count 0x5e29e0). Constructor 0x4ce2a0 and reader 0x4ce360 (rgl/mesh2/mesh_2.cpp); the inline
// destructor's out-of-line copy is 0x4c65d0 and its vector deleting destructor ??_EMissionObj 0x4c7ee0 (both emitted by
// `delete[] g_propkinds` in cls_map/cls_map.cpp's FinalCleanup; module_files.cpp's reload deletes the old table the same way).
// ONE definition for src/game and src/rgl: the name lookup (agent.cpp find_in_table78 0x4352a0), the hit points (troop_inv.cpp
// 0x4631c0), the spawn rules (terrain_b 0x472050: flag 0x4000), Mission::start_setup 0x484590 (flag 0x1000, the bounds),
// Mission::spawn_prop 0x44c210 (meshes, shape, triggers), the missile hit (agent_logic.cpp), the scene editor (gui_edit.cpp).
#ifndef MB_MISSION_OBJ_H
#define MB_MISSION_OBJ_H
#include <vector>
#include "../game/tstring.h"
#include "../rgl/rgl_math.h"           // rgl::vector4
#include "./shape.h"                   // shape_base (the body: deleted through its virtual destructor)
#include "../game/trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list
namespace rgl { struct mesh; }
struct body_part;                      // rgl/resource.h: the collision body resource (get_object60)
struct stream_base;

struct MissionObj {
    tstring name;                      // +0x00 the prop id ("spr_..." without the prefix in the editor list)
    int flags;                         // +0x10 low byte: the kind (5 chest, 8, 10 exit); 0x1000 jitter and snap to the ground,
                                       //       0x2000 / 0x10000 no shadow mask, 0x4000 invisible outside edit mode, 0x8000 destructible
    int hit_points;                    // +0x14 (0x4631c0: a destructible prop's hit points)
    rgl::vector4 v18;                  // +0x18 (0,0,0)
    rgl::vector4 frame;                // +0x28 (1,1,1; .w = 1.0f)
    int f38;                           // +0x38
    int f3c;                           // +0x3c
    std::vector<rgl::mesh*> meshes;    // +0x40 the meshes named by the prop's mesh prefix
    body_part* shape;                  // +0x50 the collision body resource
    shape_base* body;                  // +0x54 its shape copy (deleted by the destructor)
    rgl::vector4 center;               // +0x58 the centre of the bounds (0,0,0)
    float radius;                      // +0x68 the bounding radius (0.1f)
    SimpleTriggerVec triggers;                // +0x6c the prop's triggers ("Mission object %s"; update(-0x28) on spawn, -0x2a / -0x2b on a hit)
    MissionObj();                      // 0x4ce2a0
    ~MissionObj() { delete body; }     // 0x4c65d0 (the out-of-line copy the vector deleting destructor 0x4c7ee0 calls)
    void read(stream_base* s);         // 0x4ce360 (ret 4)
};
typedef MissionObj PropKind;

extern MissionObj* g_propkinds;        // 0x5e29dc

#endif // MB_MISSION_OBJ_H
