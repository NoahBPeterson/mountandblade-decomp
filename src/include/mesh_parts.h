// mesh_parts.h: the plain records of the mesh object (rgl::mesh in src/rgl/mesh.h, ::mesh of src/game/strat/strat_ext.h): the
// 0x60-byte vertex, the morph key frame, the attachment object at mesh+0xc0, the 0x2c-byte render slot and the
// render_info out parameter of the entity render slot. Namespace rgl; needs rgl::vector4 (rgl_base.h picks the module's own).
#ifndef MB_MESH_PARTS_H
#define MB_MESH_PARTS_H
#include <vector>
#include <string.h>
#include "./rgl_base.h"
#include "./vec.h"

namespace rgl {

struct vertex60 {                // 0x60 bytes, vector at mesh+0x50 (index read by build_skin 0x411d20)
    vector4 pos;                 // 0x00
    vector4 f10, f20;
    vector4 normal;              // 0x30
    vec2 uv;                     // 0x40
    int f48, f4c;
    int index;                   // 0x50: point index
    unsigned color;              // 0x54: packed ARGB
    int f58;                     // 0x58: linear colour (0x419360)
    int f5c;
};

// 0x20 bytes: one morph key frame (two vector4 arrays). Out-of-line ctor 0x411780, copy 0x411c10.
struct frame_t {
    std::vector<vector4> verts, norms;
    frame_t();                                      // 0x411780
    void copy_from(const frame_t& o);               // 0x411c10
};

} // namespace rgl
#include "./keyframes.h"      // rgl::keyframes (frame_t above)
namespace rgl {

// object attached at mesh+0xc0, deleted with the mesh (~mesh 0x4127f0)
struct obj_c0 { char pad[0x24]; vector4 out; vector4 in; };


struct slot {                    // 0x2c bytes, array of 3 at mesh+0x110 (ctor loop in mesh::mesh 0x412a90)
    int f0, f4, f8, fc;
    int f10, f14;
    int pa, pb;                  // 0x18
    int f20, f24;
    char f28, f29;               // slots[0].f28 (0x138) and slots[1].f28 (0x164) are dirty flags
    slot()
    {
        f0 = 0; f4 = 0; f8 = 0; fc = 0;
        f28 = 1; f29 = 0;
        memset(&pa, 0, 8);
        f20 = 0; f24 = -1;
    }
};

// out parameter of the entity render slot: a hit flag and the view depth of the first point (0x420310)
struct render_info { char valid; float depth; };

} // namespace rgl

#endif // MB_MESH_PARTS_H
