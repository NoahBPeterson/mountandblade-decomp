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
#include <list>
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
    void bind_pixel_shader();                  // 0x461f20 (src/game/renderer.cpp; terrain_a.cpp binds the material's pixel shader)
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

// 12-byte key array that owns both arrays (resource.h's key_array, plus constructor and destructor; the
// constructor stores n first, as skel_anim's inlined copy 0x41b4e0 does)
struct vec_array : key_array {
    vec_array() { n = 0; data = 0; keys = 0; }
    ~vec_array()                                                        // 0x4196a0 (the body of allocator::destroy)
    {
        if (data)
            delete[] data;
        if (keys)
            delete[] keys;
    }
};

// vtable 0x54b24c, 0x34 bytes, type() 8: skeleton animation (ctor 0x41b4e0, dtor 0x41b520, deleting 0x41bed0)
struct skel_anim : named_object {
    std::vector<vec_array> keys;               // +0x18 (first at +0x1c)
    vec_array tail;                            // +0x28
    skel_anim();                               // 0x41b4e0
    virtual int type();                        // 0x48b4a0 (8)
};

// ---- the resource manager (global at 0x5d6fa0; defined in core/resmgr.cpp, constructed by the compiler) -----------------------
// ONE class: the lookups and device maintenance (resource_manager.cpp, 0x418870..0x419190) and the registration, serialisation,
// loading and destruction (core/resmgr.cpp, 0x419540..0x41e46f) are its members. Layout from the constructor 0x41b000, add
// 0x41ba90, save 0x419a30 and the destructor 0x419e90. Every list is a std::vector of the resource pointers.
struct lod_group;                              // core/resmgr.h (0xe0 bytes, ctor 0x41c2a0)
namespace rgl { struct mesh; }
struct resmgr {
    char f0;                                   // +0x00
    unsigned c4, c8, cc;                       // +0x04 id counters for textures, shaders, materials (1..1000)
    std::vector<lod_group*> lods;              // +0x10 LOD groups (make_lod_mesh 0x41ca60)
    std::vector<rgl::mesh*> meshes;            // +0x20 type 1
    std::vector<texture*> textures;            // +0x30 type 2
    std::vector<material*> shaders;            // +0x40 type 4
    std::vector<skin*> materials;              // +0x50 type 3
    std::vector<body_part*> bodies;            // +0x60 type 6
    std::vector<frame_set*> skeletons;         // +0x70 type 7
    std::vector<skel_anim*> anims;             // +0x80 type 8
    std::vector<effect_set*> effects;          // +0x90 (technique sets of the shader files)
    std::list<named_object*> blocks;           // +0xa0 the array blocks to delete[] (one per loaded record group)
    std::vector<named_object*> all;            // +0xac every registered resource
    rgl::string root;                          // +0xbc resource root
    char editor_cc, editor_cd;                 // +0xcc, +0xcd (both set from enable_edit_mode by the editor start, game/char_sheet/editor_ui.cpp)
    char meshes_ready;                         // +0xce (set once by linearise_meshes)

    resmgr();                                  // 0x41b000
    ~resmgr();                                 // 0x419e90
    // resource_manager.cpp
    texture* get_texture(const rgl::string& name);             // 0x418870
    skin* get_material(const rgl::string& name);               // 0x418a70
    material* get_shader(const rgl::string& name);             // 0x418b30
    effect_set* find_effect(const rgl::string& name);          // 0x418b90
    rgl::mesh* find_mesh(const rgl::string& name);          // 0x418bd0
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
    // core/resmgr.cpp
    rgl::mesh* get_object(const rgl::string* name);                    // 0x419540
    int restore(ID3DXEffect* fx);                                      // 0x419580 (device reset: textures, effects)
    int invalidate();                                                  // 0x4195e0 (device lost)
    void linearise_meshes();                                           // 0x419640
    void save(stream_base* s);                                         // 0x419a30
    void add(named_object* r);                                         // 0x41ba90
    texture* get_render_target(int w, int h, int fmt, int ext, int f34);   // 0x41b7d0
    std::vector<rgl::mesh*> find_meshes(const rgl::string* prefix);    // 0x41b960
    effect_set* get_small(rgl::string name);                           // 0x41bf40
    void load_shaders();                                               // 0x41c000
    texture* get_texture(int w, int h, int fmt);                       // 0x41c400
    skin* get_shadow_skin(int w, int h, int fmt, int f34);             // 0x41c420
    int load_all(int check_exists, rgl::string* root);                 // 0x41c540
    lod_group* make_lod_mesh(const rgl::string& pat, bool copy);       // 0x41ca60
    lod_group* get_lod_mesh(const rgl::string& name);                  // 0x41cdc0
    stream_base* load_stream(stream_base* s);                          // 0x41db20
    void load_pack(const rgl::string& name, int user);                 // 0x41e150
};
extern resmgr g_resmgr;                        // 0x5d6fa0

// 0x4175f0 is rgl/mem_stream.cpp's stdcall clear_f14(named_object*), which ignores ecx (the original's linker folded the resource
// manager's release(res) into it); the callers that hold the manager in ecx call it through the member type of the original's call
void __stdcall clear_f14(named_object* r);               // 0x4175f0
union ReleaseFn { void (resmgr::*as_member)(void*); void (__stdcall *clear_f14)(named_object*); };

#endif
