// dev_buffer.h: the 0x4c-byte device vertex / index buffer record of the render batch and the pair of them that the
// batch keeps per vertex format (0x4bc540..0x4bd0c0 in src/game/config, 0x4622e0 / 0x4623c0 in src/game/renderer.cpp,
// the uploader 0x46d060 in src/game/terrain_a). One class each; the constructors and destructors are the compiler's.
#ifndef GAME_DEV_BUFFER_H
#define GAME_DEV_BUFFER_H
#include <windows.h>
#include <d3d9.h>
#include <vector>
#include "./tstring.h"

namespace rgl { struct mesh; }

struct dev_buffer {
    int type;                                       // 0x00 0 vertex buffer, 1 index buffer (-1 until the batch sets it)   0x4bc540, 0x4622e0, 0x46d060
    int elem_size;                                  // 0x04 bytes per element (2 for indices)   0x4bc5b0
    IDirect3DVertexBuffer9* vb;                     // 0x08   0x4bc540
    unsigned fvf;                                   // 0x0c vertex field flags   0x4bc5b0
    int cursor;                                     // 0x10 elements already used   0x46d060
    int generation;                                 // 0x14 rebuild generation   0x4bc540
    tstring name;                                   // 0x18   0x4bc5b0 ("vdt_...")
    int count;                                      // 0x28   capacity requested   0x4bc540, 0x4bc830, 0x4622e0
    int loaded;                                     // 0x2c   capacity currently on the device   0x4bc540
    int is_dynamic;                                 // 0x30 the pair's second buffer is the dynamic one   0x46d060
    std::vector<rgl::mesh*> meshes;                 // 0x34 the meshes uploaded into it
    IDirect3DIndexBuffer9* ib;                      // 0x44   0x4bc540
    IDirect3DVertexDeclaration9* decl;              // 0x48   0x4bc830
    dev_buffer()
    {
        type = -1;
        is_dynamic = 1;
        count = 0; loaded = 0;
        cursor = 0; generation = 1;
        decl = 0;
        name.assign("unknown buffer pair");         // 0x59607c
    }
    HRESULT restore();                              // 0x4622e0
    int upload(void* scratch, void* tag);           // 0x46d060 (src/game/terrain_a)
};

struct buffer_pair {
    dev_buffer bufs[2];                             // stride 0x4c
    buffer_pair()                                   // 0x4cd720
    {
        bufs[0].is_dynamic = 0;
        bufs[1].is_dynamic = 1;
    }
    void release();                                 // 0x4bc540
    void restore_all();                             // 0x4623c0
    void restore() { bufs[0].restore(); bufs[1].restore(); }   // inline (0x4bcfb0 loop body)
};

#endif // GAME_DEV_BUFFER_H
