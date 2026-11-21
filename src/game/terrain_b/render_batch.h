// render_batch.h: the render-side records the shader-path scene renderer (render_batch::render_scene, the class of
// src/game/render_batch.h) reads: the per-scene constants block (scene), the sky/fog
// block (ViewParams, include/view_params.h), lights, the view frustum, the render-state cache (renderer, global object 0xa17910) and the
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
#include "../shared_vars.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mesh;
using rgl::face20;
using rgl::vertex60;

// ---- globals ------------------------------------------------------------------------------------------------
#include "../../include/clock_table.h"   // g_ticks (the clock table)



using rgl::unpack_color;                    // 0x40e4e0 (cdecl)

// parent-chain accessors of rgl::mesh that stay out of line here (the header inlines one level of get50/get90)
struct mesh_calls : mesh {
    __forceinline std::vector<vertex60>& get50_() { return get50(); }       // 0x40ebd0
    __forceinline std::vector<face20>& get90_() { return get90(); }         // 0x40ec30
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
inline unsigned mat_texgen(const material* m) { return (unsigned)m->stages[0].d; }   // 0x74
struct mesh_part {
    void set_state();   // 0x461f80 (src/game/renderer.cpp: the material record's state setup)
};
struct skin_calls : skin {
    __forceinline void set_state() { ((mesh_part*)this)->set_state(); }     // 0x461f80
};

#include "../../include/vec.h"
static __forceinline vector4 make4(float a, float b, float c, float d)
{
    vector4 v;
    v.x = a; v.y = b; v.z = c; v.w = d;
    return v;
}
#include "../light.h"            // Light (sample 0x461570)


struct item_ref { mesh* m; int extra; };

// the render scene is the shared rgl::scene (src/rgl/mesh.h); this module reads its +0x194 / +0x1b0 / +0x1b4 / +0x16c fields
// through the types it knows: lights (vector<Light*>), the item list and the clip plane (scene::fog is the world's ViewParams)
using rgl::scene;
inline std::vector<Light*>& scene_lights(scene* s) { return s->list; }          // 0x194

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
struct mi_ops { __forceinline void setup_material() { ((mesh*)this)->setup_material(); } };   // 0x40f3f0 (rgl::mesh::setup_material, thiscall)
struct frame4 { rot3 rot; vector4 o; };
using rgl::mat3;

#define AT(T, base, off) (*(T*)((char*)(base) + (off)))

#endif // TERRAIN_B_RENDER_BATCH_H
