// renderer.h: the Direct3D 9 render-state cache and matrix stack (global object g_render at 0xa17910). One definition
// for src/game/renderer.cpp (the state cache: 0x461630..0x461eb0), src/game/terrain_a (state calls), src/game/terrain_b
// (render_scene's cached flags / textures / lights) and src/rgl/shapes (the debug-draw matrix stack 0x408170..0x40ab10).
// Every offset cites the function that reads or writes it. Fields that two modules typed differently share an
// anonymous union (int sentinel in renderer.cpp's reset_state 0x4617b0 / pointer or index in terrain_b's render_scene).
// Needs rgl::vector4 / rgl::matrix (rgl_base.h picks the module's own) and the D3D9 headers.
#ifndef MB_RENDERER_H
#define MB_RENDERER_H
#include <windows.h>
#include <d3d9.h>
#include "./rgl_base.h"
#include "./mat4.h"

struct renderer {
    rgl::matrix stack[20];          // 0x000 matrix stack of the debug drawer (push 0x4081b0.., pop 0x4081b0)
    int sp;                         // 0x500 stack depth                          0x461630 (reset), 0x408170
    char pad504;                    // 0x504
    unsigned char b505;             // 0x505  D3DRS_FOGENABLE cache               0x4617b0
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
    unsigned tex_factor;            // 0x594  cached material colour 1
    unsigned tex_factor2;           // 0x598  cached material colour 2
    int stage_samp[3];              // 0x59c                                      0x461d90
    char pad5a8[0x5b4 - 0x5a8];
    int f5b4;                       // 0x5b4                                      0x4617b0
    union {
        int f5b8;                   // 0x5b8                                      0x4617b0
        void* bone_origin_cache;    //                                            0x46e590
    };
    int cull_setting;               // 0x5bc (0x4617b0 sets 2)
    int cull_cache;                 // 0x5c0 (0x4617b0 sets -1)
    union {
        void* tex_cache[5];         // 0x5c4 diffuse, diffuse2, normal, specular, env (0x46e590)
        int tex_id[5];              //       -457 sentinels (0x4617b0)
    };
    int light_count;                // 0x5d8                                      0x46e590 (0x4617b0 clears)
    int light_idx[4];               // 0x5dc                                      0x46e590 (0x4617b0 sets [0] = 0xffbd6370)
    float material_power;           // 0x5ec  "fMaterialPower" cache              0x4617b0, 0x461f80
    float tex_off[2];               // 0x5f0  (reset to sentinel values)          0x4617b0
    float f5f8, f5fc;
    float time_var;                 // 0x600                                      0x46e590
    float spec[3];                  // 0x604  "vSpecularColor" cache              0x4617b0, 0x461eb0
    int f610;
    unsigned char b614;             // 0x614                                      0x46e590
    char pad615[3];
    IDirect3DDevice9* dev;          // 0x618
    char pad61c[0x688 - 0x61c];
    int max_aniso;                  // 0x688                                      0x4617b0
    char pad68c[0x754 - 0x68c];
    D3DMATRIX view;                 // 0x754                                      0x461630
    rgl::matrix cur;                // 0x794 current world matrix (also written as the second view slot by 0x461630)

    void set_view(const rgl::mat4* p);                      // 0x461630
    void reset_state();                                     // 0x4617b0
    void set_lighting(bool on);                             // 0x461ba0
    void set_stage_color(int stage, int flags);             // 0x461bd0  SetTextureStageState(D3DTSS_COLOROP, flags & 0x1f)
    void set_stage_alpha(int stage, int flags);             // 0x461c60  D3DTSS_ALPHAOP
    void set_alpha_mode(int mode);                          // 0x461cf0  ALPHATESTENABLE + ALPHAREF 8/0x88/0xfb
    void set_stage_sampler(int stage, unsigned flags);      // 0x461d90  ADDRESSU/V wrap or clamp (bits 0, 1)
    void set_material(float power, const float* specular);  // 0x461e10
    void set_specular(const float* c);                      // 0x461eb0
    void set_blend_mode(int mode);                          // 0x40e940

    // the debug drawer's matrix stack and primitives (src/rgl/shapes/shapes.cpp)
    __declspec(noinline) void translate(const rgl::vector4& t);                  // 0x408170
    __declspec(noinline) void pop();                                             // 0x4081b0
    __declspec(noinline) void draw_sphere(float r);                              // 0x4081f0 5x8 wireframe lat/long sphere
    void apply(const rgl::matrix* m);                       // 0x40a870 cur = cur * m
    void draw_capsule(float a, float b, float r);           // 0x40aa20
    void tessellate(float* pts, int* n, float a, float b, float r, int stacks, int slices, int mode);  // 0x40ab10
    void draw_polygon(const rgl::vector4* pts, int n);      // 0x408e60 outline of a <= 4 point polygon
};
extern renderer g_render;           // 0xa17910

#endif // MB_RENDERER_H
