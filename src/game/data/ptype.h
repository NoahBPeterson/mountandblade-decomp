// ptype.h: the particle / ground-spec type record (table g_ptypes at *0x5e28f8, count 0x5e29e8, 0x20 bytes each) and the
// 0x48-byte box record each of its frames points to. ONE definition for src/game: terrain_a (terrain.h), terrain_b
// (scene_container_grid.cpp spawn_bodies 0x479da0) and char_sheet (dust_particles.h, update_particles 0x4f3e10 / emit_cell 0x4f4060) saw it as
// partial views. The header needs tstring and shape.h; the mesh and resource types are forward-declared so that both header
// universes (rgl_math.h and rgl.h) can include it.
#ifndef GAME_PTYPE_H
#define GAME_PTYPE_H
#include <vector>
#include "engine/core/tstring.h"

namespace rgl { struct mesh; }
struct body_part;
#include "engine/physics/shape.h"                   // shape_base: AnimBox deletes its body through the virtual destructor
struct stream_base;
// One frame of a type (ptype::read 0x4cdfd0 fills it).
struct AnimBox {                                // 0x48
    tstring name;                               // +0x00 the frame's mesh name (gen_set_init 0x47df80: empty = no meshes)
    std::vector<rgl::mesh*> meshes;             // +0x10 the meshes of the name (first at +0x14; char_sheet dust_particles.cpp 0x4f3e10)
    body_part* res;                             // +0x20 the collision body resource, 0 for "0" (terrain_b 0x479da0)
    shape_base* body;                           // +0x24 its shape copy
    rgl::vector4 mn;                            // +0x28 the bounds of the meshes (1e7f, merged with each mesh's bounds 0x438d80)
    rgl::vector4 mx;                            // +0x38 (-1e7f, merged likewise)
    AnimBox() : res(0), body(0) {}              // the reader's new[] (0x4cdfd0) runs it per element
    ~AnimBox() { delete body; }                 // inline: the vector deleting destructor 0x431960 (game/data/record_destructors.cpp) expands it
};
struct ptype {
    tstring name;                               // +0 (0x4650f0)
    unsigned __int64 flags;                     // +0x10 (read as one 64-bit word) (0x400000 / 0x80000 / 0x1000000 / 0x20000 ...; the high dword's low 16 bits are a density)
    unsigned short density() const { return ((const unsigned short*)&flags)[2]; }   // the high dword's low word (read as a word: 0x4650f0)
    int nframes;                                // +0x18 (0x466cd0, 0x46a270; terrain_b 0x479da0 and dust 0x4f3e10: count of boxes)
    AnimBox* boxes;                             // +0x1c: nframes boxes of 0x48 bytes
    ptype() { nframes = 0; boxes = 0; }         // the flora table's new[] (load_data_files 0x4d0c20) runs it per element
    ~ptype() { delete[] boxes; }                // inline (GameApp::FinalCleanup 0x4da5b0 expands the table's delete[])
    void read(stream_base* s);                  // 0x4cdfd0 (game/editor/poly_mesh_edge_face_ops.cpp: flora_kinds.txt's record; ret 4)
};
extern ptype* g_ptypes;                         // 0x5e28f8
extern int g_ptype_count;                       // 0x5e29e8
extern int g_num_ground_specs;                  // 0x5e29ec count of the skyboxes.txt ground specs (module_record_loaders.cpp loads them; mesh_start walks them)

#endif // GAME_PTYPE_H
