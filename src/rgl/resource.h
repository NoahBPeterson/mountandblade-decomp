// resource.h: the engine's named resources and the resource manager (one header for every src/rgl module).
// Class names are the slices' provisional ones, kept because symbols/slices/*.txt maps the vtables by them.
// In BRF terms, `material` (vtable 0x54ab30: alternatives, pixel shader) is a *shader* and `skin` (vtable
// 0x54abf0: shader name, five textures, colour) is a *material*; the resource manager's lists say so.
#ifndef RGL_RESOURCE_H
#define RGL_RESOURCE_H
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "./rgl_string.h"
#include "./rgl_math.h"
#include "./stream.h"

// Root of the resources (and of rgl::mesh), vtable 0x54a6c8: slot 0 is the pure virtual destructor
// (_purecall), slot 1 the type id (0x408020, returns 0). Ctor layout from 0x417010 / 0x4171c0 / 0x417e10:
// vptr, name ctor, f14 = 0, then the derived class's fields and vptr.
struct named_object {
    rgl::string name;                          // +0x04
    int f14;                                   // +0x14
    named_object() { f14 = 0; }
    virtual ~named_object() = 0;               // body: 0x4150f0
    virtual int type();                        // 0x408020
};
// The derived classes' deleting destructors (0x417600, 0x417f20, ...) inline this body. An out-of-line copy
// sits at 0x4150f0 in text_stream.cpp, which defines RGL_NAMED_OBJECT_DTOR_OUT_OF_LINE to emit it there.
#ifndef RGL_NAMED_OBJECT_DTOR_OUT_OF_LINE
inline named_object::~named_object() {}
#endif

// ---- textures -------------------------------------------------------------------------------------
// vtable 0x54aaac (slot 0 0x417f20, slot 1 0x48b590 returns 2), 0x54 bytes. Fields from 0x418520.
struct texture : named_object {
    char loaded;                               // +0x18
    int f1c;                                   // +0x1c
    int kind;                                  // +0x20 0 file texture, 1 render-target texture, 2 depth-stencil
    unsigned flags;                            // +0x24
    int owns_texture;                          // +0x28
    int width;                                 // +0x2c
    int height;                                // +0x30
    int mipmaps;                               // +0x34
    int format;                                // +0x38 D3DFORMAT
    int used_format;                           // +0x3c
    IDirect3DTexture9** d3d_tex;               // +0x40
    IDirect3DSurface9* surface;                // +0x44
    IDirect3DSurface9* level0;                 // +0x48
    int f4c;                                   // +0x4c
    int f50;                                   // +0x50
    // inline body; the deleting destructor is 0x417f20
    virtual ~texture() { unload(); }
    virtual int type();                        // 0x48b590
    texture();                                 // (another object; not located)
    int load(int check_exists, rgl::string* root);   // 0x416540 (thiscall, ret 8)
    int unload();                              // 0x4167c0
    int create();                              // 0x418520
};

// ---- shaders (`material`) -------------------------------------------------------------------------------
// One texture-stage record of a shader (0x14 bytes).
struct stage_t {
    int a, b, c, d, e;
};
// vtable 0x54ab30, 0xc8 bytes (ctor 0x416830, dtor 0x4168f0).
struct material : named_object {
    int f18;                                   // +0x18
    unsigned flags1c;                          // +0x1c (bit 12: needs pixel shader 1.1)
    unsigned flags20;                          // +0x20 (bit 16 disabled, bit 28 has pixel shader, bit 29 vertex shader)
    int nalt;                                  // +0x24
    rgl::string alts[3];                       // +0x28 alternative shader names
    rgl::string ps_file;                       // +0x58
    stage_t stages[4];                         // +0x68
    int nstages;                               // +0xb8
    IDirect3DPixelShader9* ps;                 // +0xbc
    int fc0;                                   // +0xc0
    char ps_done;                              // +0xc4
    material();                                // 0x416830
    // inline (the deleting destructor 0x417690 inlines it on its array path); out-of-line copy:
    virtual ~material() {}                     // 0x4168f0
    virtual int type();                        // 0x48c460
    virtual void load(stream_base* s);         // 0x4169a0
    virtual void save(stream_base* s);         // 0x416b10
    bool supported();                          // 0x416930
    int get_flags();                           // 0x40eab0
};

// ---- materials (`skin`) ------------------------------------------------------------------------------
// vtable 0x54abf0, 0x6c bytes (ctor 0x416cd0): what rgl::mesh renders with (mesh+0x2c). Fields from
// 0x4190d0 (load), 0x416d50 (save) and mesh::render 0x413750.
struct skin : named_object {
    int f18;                                   // +0x18
    unsigned flags;                            // +0x1c
    rgl::string shader_name;                   // +0x20
    material* shader;                          // +0x30
    rgl::vector4 color;                        // +0x34 (x, y, z read by load; the 4th lane unused)
    float f44;                                 // +0x44
    texture* tex[5];                           // +0x48 ("none" = no texture)
    int f5c, f60, f64;                         // +0x5c
    void (*on_render)(skin*);                  // +0x68 (called by mesh::render 0x413750)
    skin();                                    // 0x416cd0
    // inline body; deleting destructor 0x417600
    virtual ~skin() {}
    virtual int type();                        // 0x48b880
    virtual void load(stream_base* s);         // 0x4190d0
    virtual void save(stream_base* s);         // 0x416d50
};

// ---- effects ----------------------------------------------------------------------------------------------
// vtable 0x54ac08 (slot 0 0x417fb0, slot 1 0x417040), 0x2c bytes: an ID3DXEffect technique set.
struct effect_set : named_object {
    char resolved;                             // +0x18
    D3DXHANDLE h[4];                           // +0x1c name, name_SHDW, name_SHDWNVIDIA, name_PRESHADED
    effect_set();                              // 0x417010
    // inline body; deleting destructor 0x417fb0
    virtual ~effect_set()
    {
        for (int i = 0; i < 4; i++)
            h[i] = 0;
        resolved = 0;
    }
    virtual int type();                        // 0x417040
    int resolve(ID3DXEffect* fx);              // 0x417050
};

// ---- body parts ---------------------------------------------------------------------------------------------
// Collision-shape base (vtables 0x54a4b8 sphere, 0x54a540 capsule, 0x54a588 face, ...; see s0040cfe0) and the
// shapes: src/include/shape.h
#include "../include/shape.h"
// vtable 0x54ac58 (slot 0 0x4177b0, slot 1 0x4171f0), 0x1c bytes: a name and its collision shape.
struct body_part : named_object {
    shape_base* shape;                         // +0x18
    body_part();                               // 0x4171c0
    // inline body; deleting destructor 0x4177b0
    virtual ~body_part() { delete shape; }
    virtual int type();                        // 0x4171f0
};

// ---- key arrays ------------------------------------------------------------------------------------------
// 12-byte record: n keys and n 16-byte values (positions, or quaternions stored x y z w and serialised
// y z w x). s00416ef0 called it key_array, s00417850/s00418150 vec_array: one type.
struct key_array {
    rgl::vector4* data;                        // +0
    int* keys;                                 // +4
    int n;                                     // +8
    void operator=(const key_array& o);        // 0x417850
};

// Animation clip: name, tracks (key arrays) and one more key array.
struct key_clip : named_object {
    std::vector<key_array> tracks;             // +0x18
    key_array extra;                           // +0x28
};

#include "../include/vec_table.h"            // vec_table (shared with src/game/char_sheet/screen.h)

// vtable 0x54aeb4 (slot 0 0x41aee0, slot 1 0x49df30 returns 7), 0x34 bytes.
struct frame_set : named_object {
    int f18;                                   // +0x18
    int f1c;                                   // +0x1c
    vec_table table;                           // +0x20
    int f28, f2c, f30;                         // +0x28
    frame_set();                               // 0x417e10
    // deleting destructor 0x41aee0 (vtable slot 0)
    virtual ~frame_set();
    virtual int type();                        // 0x49df30
};

// ---- resource manager (global at 0x5d6fa0) --------------------------------------------------------------
// std::vector-shaped list of resource pointers (16 bytes: allocator, first, last, end).
template<class T> struct res_list {
    int alloc;
    T** first;
    T** last;
    T** end;
};
struct resource_manager {
    int pad[8];
    res_list<named_object> v20;                // +0x20
    res_list<texture> textures;                // +0x30
    res_list<material> shaders;                // +0x40
    res_list<skin> materials;                  // +0x50
    res_list<named_object> v60;                // +0x60
    res_list<named_object> v70;                // +0x70
    res_list<named_object> skel_anims;         // +0x80
    res_list<effect_set> effects;              // +0x90
    char pad_a0[0xbc - 0xa0];
    rgl::string root;                          // +0xbc
    texture* get_texture(const rgl::string& name);             // 0x418870
    skin* get_material(const rgl::string& name);               // 0x418a70
    material* get_shader(const rgl::string& name);             // 0x418b30
    effect_set* find_effect(const rgl::string& name);          // 0x418b90
    named_object* find_v20(const rgl::string& name);           // 0x418bd0
    named_object* get_object60(const rgl::string& name);       // 0x418c10
    named_object* get_skeleton_anim(const rgl::string& name);  // 0x418c80
    named_object* get_object70(const rgl::string& name);       // 0x418cf0
    int load_textures();                                       // 0x418d60
    int reload_textures();                                     // 0x418da0
    int unload_textures();                                     // 0x418e10
    void release_pixel_shaders();                              // 0x418e40
    material* resolve_shader(const rgl::string& name);         // 0x418e90
    void reset_effect_sets();                                  // 0x418fb0
    void resolve_effect_sets(ID3DXEffect* fx);                 // 0x418ff0
};
extern resource_manager g_resmgr;              // 0x5d6fa0

#endif
