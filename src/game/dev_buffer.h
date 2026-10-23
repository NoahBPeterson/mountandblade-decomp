// dev_buffer.h: the 0x4c-byte device vertex / index buffer record of the render batch and the pair of them that the
// batch keeps per vertex format (0x4bc540..0x4bd0c0 in src/game/config, 0x4622e0 / 0x4623c0 in src/game/renderer.cpp).
#ifndef GAME_DEV_BUFFER_H
#define GAME_DEV_BUFFER_H
#include <windows.h>
#include <d3d9.h>
#include "./tstring.h"

struct dev_buffer {
    int type;                                       // 0x00 0 vertex buffer, 1 index buffer   0x4bc540, 0x4622e0
    int elem_size;                                  // 0x04   0x4bc5b0
    IDirect3DVertexBuffer9* vb;                     // 0x08   0x4bc540
    unsigned fvf;                                   // 0x0c   0x4bc5b0
    int f10;                                        // 0x10
    int generation;                                 // 0x14   0x4bc540
    tstring name;                                   // 0x18   0x4bc5b0 ("vdt_...")
    int count;                                      // 0x28   capacity requested   0x4bc540, 0x4bc830, 0x4622e0
    int loaded;                                     // 0x2c   capacity currently on the device   0x4bc540
    char pad30[0x44 - 0x30];
    IDirect3DIndexBuffer9* ib;                      // 0x44   0x4bc540
    IDirect3DVertexDeclaration9* decl;              // 0x48   0x4bc830
    HRESULT restore();                              // 0x4622e0
};

struct buffer_pair {
    dev_buffer bufs[2];                             // stride 0x4c
    void release();                                 // 0x4bc540
    void restore_all();                             // 0x4623c0
    void restore() { bufs[0].restore(); bufs[1].restore(); }   // inline (0x4bcfb0 loop body)
};

#endif // GAME_DEV_BUFFER_H
