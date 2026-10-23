// render_batch.h: the shader-path scene renderer's object (render_batch, a ~4 MB object of which only the touched
// fields are modelled) and the render-side views it reads: the per-scene constants block (scene), the sky/fog
// block (sky_t), lights, the view frustum, the render-state cache (renderer, global object 0xa17910) and the
// material / mesh-part / texture records. Every offset is read or written by render_batch::render_scene (0x46e590,
// render_batch.cpp); the other users of these records (src/game/renderer.h, src/rgl/scene/*.h) declare their own
// partial views of the same objects (see MIGRATION.md, "Header conflicts"). Names provisional.
#ifndef TERRAIN_B_RENDER_BATCH_H
#define TERRAIN_B_RENDER_BATCH_H
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "./frustum.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mesh;
using rgl::face20;
using rgl::vertex60;

// ---- globals ------------------------------------------------------------------------------------------------
extern ID3DXEffect* g_effect;               // 0x5e293c
extern __int64 g_ticks[];                   // 0x5e2948 clock table (1e-5 s ticks)
extern int g_mesh_quality;                  // 0x5aa084 (2: shader quality / mode)
extern int g_x5aa034;                       // 0x5aa034

using rgl::unpack_color;                    // 0x40e4e0 (cdecl)

// parent-chain accessors of rgl::mesh that stay out of line here (the header inlines one level of get50/get90)
struct mesh_calls : mesh {
    std::vector<vertex60>& get50_();                // 0x40ebd0
    std::vector<face20>& get90_();                  // 0x40ec30
};

// ---- renderer (global object at 0xa17910) -----------------------------------------------------------------------
#include "../../include/renderer.h"       // renderer (shared)

// ---- render records (the shared classes of src/rgl/resource.h) -----------------------------------------------------
// `texture` (frame count at +0x28, which resource.h calls owns_texture; frame array at +0x40), `material` (the shader:
// flags +0x20, texture-stage texgen word +0x74, technique table +0xc0) and `skin` (the mesh's material: flags +0x1c,
// shader +0x30, textures +0x48.. in the order diffuse, diffuse2, normal, env, specular). The slice called them
// texres / material / mesh_part.
inline IDirect3DBaseTexture9* tex_current(const texture* t, const float& tm)    // 0x416xxx inline, see render_scene
{
    IDirect3DBaseTexture9* r;
    if (t->loaded) {
        int i = 0;
        if (t->flags & 0xf000000)
            i = (int)((float)t->owns_texture * tm) % t->owns_texture;
        r = t->d3d_tex[i];
    } else {
        r = 0;
    }
    return r;
}
struct tech_tbl { char pad[0x1c]; D3DXHANDLE tech[4]; };
inline tech_tbl* mat_techs(const material* m) { return (tech_tbl*)m->fc0; }     // 0xc0
inline unsigned mat_texgen(const material* m) { return (unsigned)m->stages[0].d; }   // 0x74
struct skin_calls : skin {
    void set_state();                   // 0x461f80 (mesh_part::set_state in renderer.h)
};

#include "../../include/vec.h"
static __forceinline vector4 make4(float a, float b, float c, float d)
{
    vector4 v;
    v.x = a; v.y = b; v.z = c; v.w = d;
    return v;
}
struct light {                          // 0x7c bytes (layout from s00461280)
    int type;                           // 0: point/dir with flicker, 2: sun
    vector4 diffuse;                    // 0x04
    vector4 v14;
    vec3 position;                      // 0x24
    char pad30[0x44 - 0x30];
    vec3 sun_dir;                       // 0x44
    char pad50[0x54 - 0x50];
    float radius;                       // 0x54
    char pad58[0x6c - 0x58];
    unsigned flags;                     // 0x6c
    char pad70[0x7c - 0x70];
    vector4 sample();                   // 0x461570
};

struct sky_t {
    char pad0[4];
    float fog_start_b, fog_end_b;       // 0x04, 0x08
    unsigned fog_color_b;               // 0x0c
    char fog_meshes;                    // 0x10
    char pad11[3];
    float fog_dens_b;                   // 0x14
    float fog_start_a, fog_end_a;       // 0x18, 0x1c
    unsigned fog_color_a;               // 0x20
    float height;                       // 0x24
    float fog_dens_a;                   // 0x28
    char sun_shadows;                   // 0x2c
    char pad2d[3];
    texture* shadow;                    // 0x30
    frustum shadow_cam;                 // 0x34
};

struct item_ref { mesh* m; int extra; };

// the render scene is the shared rgl::scene (src/rgl/mesh.h); this module reads its +0x194 / +0x1b0 / +0x1b4 / +0x16c fields
// through the types it knows: lights (vector<light*>), the sky block, the item list and the clip plane
using rgl::scene;
inline std::vector<light*>& scene_lights(scene* s) { return (std::vector<light*>&)s->list; }          // 0x194
inline sky_t* scene_sky(scene* s) { return (sky_t*)s->f1b0; }                                          // 0x1b0
inline std::vector<item_ref>& scene_items(scene* s) { return (std::vector<item_ref>&)s->queue; }       // 0x1b4
inline D3DXPLANE& scene_clip(scene* s) { return *(D3DXPLANE*)&s->f16c; }                               // 0x16c

// ---- the batch renderer object (a ~4 MB object; only the touched fields are modelled) -----------------------------
#include "../render_batch.h"                   // render_batch (shared with config.cpp)

#include "../../include/mat4.h"
using rgl::rot3;
static __forceinline vector4 cross_i(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}
struct vec_arr { D3DXVECTOR4* data; UINT count; };
struct mi_ops { void setup_material(); };   // 0x40f3f0 (mesh instance, thiscall)
struct frame4 { rot3 rot; vector4 o; };
rot3* __cdecl identity3(rot3* out);        // 0x40cd60
struct m4 {
    vector4 s, f, u, o;
    m4 operator*(const m4& b) const;        // 0x40e870 (this = left operand)
    void rotate_fu(const float* angle);     // 0x40e570
};

#define AT(T, base, off) (*(T*)((char*)(base) + (off)))

#endif // TERRAIN_B_RENDER_BATCH_H
