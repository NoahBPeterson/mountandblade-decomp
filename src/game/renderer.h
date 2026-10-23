// renderer.h: classes of renderer.cpp (0x461630..0x4625d0): the Direct3D 9 render-state cache (renderer, global
// object 0xa17910), the material / mesh-part state applied through it, the view frustum's matrix helpers, the
// device vertex / index buffer pair restored after a device reset, a 2D cell grid and an envelope parameter
// block. Engine (rgl) code by its content (D3D state cache, effect uniforms); placed under src/game for now.
// All names provisional; every offset is read or written by a function of this module (cited by address).
#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "rgl.h"
#include "./mat4.h"
#include "../include/frustum.h"

// ---- globals -----------------------------------------------------------------------------------------
extern int g_use_shaders;                   // 0x5e28fc
extern int g_use_wbuffer;                   // 0x5a9fdc
extern int g_aniso_filter;                  // 0x5aa040
extern float g_gamma;                       // 0x5aa088
extern ID3DXEffect* g_effect;               // 0x5e293c
extern unsigned g_stage_arg[8];             // 0x5a9c00 D3DTA_* by 3-bit code
extern IDirect3DDevice9* g_device;          // 0xa17f28
void mb_error(const char* fmt, ...);        // 0x42cc80

// ---- render-state cache (global object 0xa17910) ----------------------------------------------------------
struct renderer {
    char pad000[0x500];
    int f500;                       // 0x500                                      0x461630
    char pad504;                    // 0x504
    char b505;                      // 0x505  D3DRS_FOGENABLE cache               0x4617b0
    bool b506;                      // 0x506  D3DRS_LIGHTING cache                0x4617b0, 0x461ba0
    char light_on[64];              // 0x507  LightEnable cache                   0x4617b0
    unsigned char b547;             // 0x547  D3DRS_SPECULARENABLE cache          0x4617b0, 0x461f80
    unsigned char b548;             // 0x548  D3DRS_NORMALIZENORMALS cache        0x4617b0, 0x461f80
    unsigned char b549;             // 0x549  z-write disabled                    0x4617b0, 0x461f80
    unsigned char b54a;
    unsigned char b54b;             // 0x54b  depth test (ZFUNC LESSEQUAL)        0x4617b0, 0x461f80
    int f54c[3];                    // 0x54c  bound texture per stage             0x4617b0
    IDirect3DPixelShader9* cur_ps;  // 0x558                                      0x4617b0, 0x461f20
    int stage_color[3];             // 0x55c                                      0x461bd0
    int stage_alpha[3];             // 0x568                                      0x461c60
    int stage_xform[3];             // 0x574
    int stage_texgen[3];            // 0x580
    int blend_mode;                 // 0x58c
    int alpha_mode;                 // 0x590  0 off, 1 (ref 8), 2 (ref 0x88), 3 (ref 0xfb)   0x461cf0
    unsigned tex_factor;            // 0x594
    unsigned tex_factor2;           // 0x598
    int stage_samp[3];              // 0x59c                                      0x461d90
    char pad5a8[0x5b4 - 0x5a8];
    int f5b4, f5b8, f5bc, f5c0;     // 0x5b4                                      0x4617b0
    int f5c4, f5c8, f5cc, f5d0, f5d4, f5d8, f5dc;
    char pad5e0[0x5ec - 0x5e0];
    float material_power;           // 0x5ec  "fMaterialPower" cache              0x4617b0, 0x461f80
    float f5f0, f5f4, f5f8, f5fc, f600;   // 0x5f0 (reset to sentinel values)
    float spec[3];                  // 0x604  "vSpecularColor" cache              0x4617b0, 0x461eb0
    int f610;
    char pad614[4];
    IDirect3DDevice9* dev;          // 0x618
    char pad61c[0x688 - 0x61c];
    int max_aniso;                  // 0x688                                      0x4617b0
    char pad68c[0x754 - 0x68c];
    D3DMATRIX view;                 // 0x754                                      0x461630
    D3DMATRIX view2;                // 0x794                                      0x461630

    void set_view(const rgl::mat4* p);                      // 0x461630
    void reset_state();                                     // 0x4617b0
    void set_lighting(bool on);                             // 0x461ba0
    void set_stage_color(int stage, int flags);             // 0x461bd0
    void set_stage_alpha(int stage, int flags);             // 0x461c60
    void set_alpha_mode(int mode);                          // 0x461cf0
    void set_stage_sampler(int stage, unsigned flags);      // 0x461d90
    void set_material(float power, const float* specular);  // 0x461e10
    void set_specular(const float* c);                      // 0x461eb0
};
extern renderer g_render;                                   // 0xa17910

// ---- material / mesh part ------------------------------------------------------------------------------
struct material {
    char pad00[0x20];
    unsigned flags;                 // 0x20  bit 28: has a pixel shader, bit 29: shader set elsewhere   0x461f20
    char pad24[0xbc - 0x24];
    IDirect3DPixelShader9* ps;      // 0xbc                                       0x461f20
    void bind_pixel_shader();       // 0x461f20
};

struct mat_rec { char pad00[0x20]; unsigned flags; };      // bit 5: specular   0x461f80
struct mesh_part {
    char pad00[0x1c];
    unsigned flags;                 // 0x1c  bit 3: z-write off, bit 4: depth test off, bit 11: normalize normals,
                                    //       bits 12..13: alpha-test mode           0x461f80
    char pad20[0x30 - 0x20];
    mat_rec* mat;                   // 0x30
    float spec_color[3];            // 0x34
    char pad40[4];
    float power;                    // 0x44
    char pad48[0x64 - 0x48];
    int refs;                       // 0x64  number of mesh instances using this part   0x4620b0
    void set_state();               // 0x461f80
};

struct mesh_inst {
    char pad00[0x2c];
    mesh_part* part;                // 0x2c                                       0x4620b0
    void set_part(mesh_part* p);    // 0x4620b0
};

// ---- view frustum: rgl::frustum of src/include/frustum.h (projection 0x4620d0, view_matrix 0x462170, to_clip 0x4621f0)
using rgl::frustum;
using rgl::plane;
// view_matrix is declared there with a D3DXMATRIX result (terrain_b reads it that way); renderer.cpp builds a mat4 and
// defines the function through this view of the same record
// mat4 from a rotation and an origin as renderer::set_view (0x461630) builds it (rgl::mat4 itself has no constructor)
struct mat4_ro : rgl::mat4 {
    mat4_ro(const rgl::rot3& r, const rgl::vector4& origin) { *(rgl::rot3*)this = r; o = origin; }
};
struct frustum_view : rgl::frustum {
    rgl::mat4 view_matrix();                // 0x462170 (hidden return pointer)
};

// ---- render-queue record (key at +0x218) ------------------------------------------------------------------
struct sort_rec { char pad[0x218]; __int64 key; };

// ---- device vertex / index buffer pair (restore after reset)
#include "./dev_buffer.h"

// ---- 2D cell grid (rows of 16-byte cells; dtor 0x41ecc0 in an earlier object) ------------------------------
struct grid_cell {
    float h; int flags; int id, id2;
    grid_cell() : flags(0), id(-1), id2(-1) {}
};
struct cell_rows {
    grid_cell** rows;               // +0
    int pad[4];
    int last;                       // +0x14  highest column index                0x462420
    int rows_n;                     // +0x18  highest row index
    ~cell_rows();                   // 0x41ecc0
    void alloc(const int* dim);     // 0x462420
};

// ---- envelope / fade parameter block --------------------------------------------------------------------
struct envelope {
    char pad00[0xdc];
    int fdc;                        // 0xdc                                       0x4624c0
    float inv_time;                 // 0xe0
    int fe4;                        // 0xe4
    char dirty;                     // 0xe8                                       0x4624c0, 0x462540
    char pade9[3];
    float fade_time;                // 0xec                                       0x4624c0
    float f0;                       // 0xf0                                       0x462540
    float time;                     // 0xf4                                       0x462540
    int f8;                         // 0xf8
    int fc;                         // 0xfc
    float smooth;                   // 0x100
    void set_fade(const float* t, const int* v);                    // 0x4624c0
    void set_params(float a, float t, const int* b, const int* c);  // 0x462540
};

#endif // GAME_RENDERER_H
