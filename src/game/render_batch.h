// render_batch.h: the batch renderer object (a ~4 MB object; 0x4bc5b0 init_formats, 0x4bc830 init_decls, 0x4bcfb0
// init_shaders, 0x4bd0c0 release_decls in src/game/config/config.cpp; 0x46e590 render_scene in
// src/game/terrain_b/terrain_b.cpp). One definition for both modules: terrain_b indexes the ten vertex records as a flat
// dev_buffer[10] (vf + (format + 2 * stream)) and reads the index buffer of record i at +0x350 + 0x4c * i.
#ifndef GAME_RENDER_BATCH_H
#define GAME_RENDER_BATCH_H
#include "./dev_buffer.h"

struct scene;

struct render_batch {                               // ~4 MB object (config.cpp: format / decl / shader set-up; terrain_b.cpp: render_scene)
    IDirect3DVertexDeclaration9* decls[5];          // +0x00 0x4bc830, 0x4bd0c0
    buffer_pair vf[5];                              // +0x14 ten vertex records (two per format) 0x4bc5b0, 0x4bcfb0
    buffer_pair ib;                                 // +0x30c two index records 0x4bc5b0
    char pad3a4[0x3b4 - 0x3a4];                      // (ib[0].ib at +0x350, ib[1].ib at +0x39c: SetIndices in render_scene 0x46e590)
    int cur_vf_a;                                   // +0x3b4 0x4bc5b0
    int cur_ib;                                     // +0x3b8
    int cur_vf_b;                                   // +0x3bc
    char pad3c0[0x418818 - 0x3c0];
    void* sh0;                                      // +0x418818 0x4bcfb0
    void* sh1;                                      // +0x41881c
    void* sh2;                                      // +0x418820
    void* sh3;                                      // +0x418824
    int counter;                                    // +0x418828
    void init_formats();                            // 0x4bc5b0
    int init_decls(int* a0, int* a1, int* a2, int* a3, int* a4,
                   int* a5, int* a6, int* a7, int* a8, int* a9);   // 0x4bc830
    void render_scene(scene* sc);                   // 0x46e590 (terrain_b.cpp; sc is terrain_b/render_batch.h's scene)
    void init_shaders();                            // 0x4bcfb0
    int release_decls();                            // 0x4bd0c0
};

#endif // GAME_RENDER_BATCH_H
