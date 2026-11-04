// render_batch.h: the batch renderer, ONE class for the 4 MB object g_batch (0x5fe898, 0x418830 bytes): the vertex
// declarations, the five vertex-format buffer pairs and the index pair (0x4bc5b0 init_formats, 0x4bc830 init_decls,
// 0x4bcfb0 init_shaders, 0x4bd0c0 release_decls in src/game/config/config.cpp), the queue of scenes (0x412040
// register_scene, 0x412100 add_mesh in src/rgl/particles/mesh_obj.cpp), the per-pass driver (0x47e520 render in
// src/game/mesh_slot/mesh_pass.cpp), the shadow-view draw (0x46d8c0 draw) and the shader-path scene renderer (0x46e590
// render_scene in src/game/terrain_b/terrain_b.cpp) with their device-state cache. The constructor is 0x4ebea0
// (src/game/char_sheet/screen.cpp), the destructor 0x4ca830 (src/rgl/mesh2/mesh_2.cpp). The global is defined in
// src/game/config/config.cpp, which owns the object's set-up.
#ifndef GAME_RENDER_BATCH_H
#define GAME_RENDER_BATCH_H
#include <stddef.h>
#include <vector>
#include "./dev_buffer.h"
#include "../rgl/mesh.h"                 // rgl::mesh (0x260), rgl::scene (0x1d8), material, skin
#include "../include/vec_table.h"
#include "../include/mat4.h"             // rgl::mat3

// a 4x4 matrix as the shader's matWorldArray takes it: three identity rows (mat3) and the translation column
struct xform {
    rgl::mat3 m;                        // +0 (identity3 output, 0x40cd60: 12 dwords)
    rgl::vector4 ex;                    // +0x30
};

struct view_t;                          // the shadow-view draw's scene (terrain_a/misc.h)

struct render_batch {
    IDirect3DVertexDeclaration9* decls[5];          // +0x00 0x4bc830, 0x4bd0c0
    buffer_pair vf[5];                              // +0x14 ten vertex records (two per format) 0x4bc5b0, 0x4bcfb0; terrain_b indexes them as dev_buffer[10]
    buffer_pair ib;                                 // +0x30c two index records 0x4bc5b0 (ib[0].ib at +0x350, ib[1].ib at +0x39c: SetIndices in render_scene 0x46e590)
    std::vector<rgl::scene*> scenes;                // +0x3a4 scenes queued for the next pass (0x412040)
    int cur_vf_a;                                   // +0x3b4 0x4bc5b0
    int cur_ib;                                     // +0x3b8
    int cur_vf_b;                                   // +0x3bc
    char scratch[0x418000];                         // +0x3c0 the staging area dev_buffer::upload (0x46d060) fills before it copies into the locked buffer
    int frame;                                      // +0x4183c0 the tag of the uploads (the save/load screen counts it up per frame: 0x4eb8e0)
    int f4183c4;                                    // +0x4183c4
    material* cur_mat;                              // +0x4183c8 the material bound by the last mesh of render_scene
    const char* cur_tech;                           // +0x4183cc its technique (D3DXHANDLE)
    skin* cur_part;                                 // +0x4183d0 the last mesh part
    xform* world_mats;                              // +0x4183d4 the 0x20 identity matrices of matWorldArray (new[], 0x4ebea0)
    vec_table bone_origins;                         // +0x4183d8 the 0x20 zero vectors of matBoneOriginArray (0x417cb0)
    rgl::mesh work_mesh;                            // +0x4183e0 (0x260)
    rgl::scene work_scene;                          // +0x418640 (0x1d8)
    material* sh0;                                  // +0x418818 0x4bcfb0 render_to_depth_shader
    material* sh1;                                  // +0x41881c render_to_depth_skin_shader
    material* sh2;                                  // +0x418820 render_character_shadow
    material* sh3;                                  // +0x418824 render_character_shadow_with_skin
    int counter;                                    // +0x418828 counts the shader reloads (init_shaders); the particle scenes compare theirs with it

    render_batch();                                 // 0x4ebea0
    ~render_batch();                                // 0x4ca830
    void init_formats();                            // 0x4bc5b0
    int init_decls(int* a0, int* a1, int* a2, int* a3, int* a4,
                   int* a5, int* a6, int* a7, int* a8, int* a9);   // 0x4bc830
    void init_shaders();                            // 0x4bcfb0
    int release_decls();                            // 0x4bd0c0
    void register_scene(rgl::scene* s);             // 0x412040
    void add_mesh(rgl::scene* s, rgl::mesh* m);     // 0x412100
    bool render(rgl::scene* only);                  // 0x47e520 render the queued scenes (only: that one scene; 0: all of them); false = try again
    void render_scene(rgl::scene* sc);              // 0x46e590 (terrain_b.cpp)
    void draw(view_t* v);                           // 0x46d8c0 (terrain_a.cpp)
};

extern render_batch g_batch; // 0x5fe898

#define RB_AT(m, off) typedef char render_batch_##m##_at[(offsetof(render_batch, m) == (off)) ? 1 : -1]
RB_AT(vf, 0x14);
RB_AT(ib, 0x30c);
RB_AT(scenes, 0x3a4);
RB_AT(cur_vf_a, 0x3b4);
RB_AT(scratch, 0x3c0);
RB_AT(frame, 0x4183c0);
RB_AT(cur_mat, 0x4183c8);
RB_AT(cur_part, 0x4183d0);
RB_AT(world_mats, 0x4183d4);
RB_AT(bone_origins, 0x4183d8);
RB_AT(work_mesh, 0x4183e0);
RB_AT(work_scene, 0x418640);
RB_AT(sh0, 0x418818);
RB_AT(counter, 0x418828);
typedef char render_batch_size[(sizeof(render_batch) == 0x418830) ? 1 : -1];   // 0x5fe898..0xa170c8 (g_input_a): the 8-aligned members pad the last member out by 4
#endif // GAME_RENDER_BATCH_H
