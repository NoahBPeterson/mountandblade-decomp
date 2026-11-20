// scene_rec.h: the render scene rgl::scene (0x1d8 bytes), ONE definition for every universe (src/rgl/mesh.h, src/game/rgl_mesh.h, the
// game modules with local mesh views: ShadowMap embeds one). Needs only pointers to the mesh, texture and world.
#ifndef RGL_SCENE_REC_H
#define RGL_SCENE_REC_H
#include <vector>
#include "../include/layout32.h"
#include "./rgl_math.h"
#include "../include/frustum.h"
#include <d3d9.h>
struct texture;
struct skin;
struct ViewParams;              // include/view_params.h: the world's fog / view block (rgl::world::view)
namespace rgl {
struct mesh;
struct mesh_pool;
struct world;
// 8 bytes: (vertex index, weight). Also the scene's queue entries (m = the mesh, weight 0).
struct idx_weight { union { int index; mesh* m; }; float weight; };
// std::vector<void*> with its copy-assignment out of line (0x43f0a0)
struct ptr_list : std::vector<void*> {
    ptr_list& operator=(const ptr_list& o);                      // 0x43f0a0
};

// The render scene (0x1d8 bytes, ctor 0x429740, reset 0x428f30, dtor 0x41f4e0 in src/game/entity.cpp): viewport / camera block
// with the view frustum at +0x30. Offsets from the constructor 0x429740 (the field stores), reset 0x428f30, Ctx::render_scene
// 0x429890 (reflection pass), ~scene 0x41f4e0 and the mesh code (mesh::render 0x413750, scene_registry 0x412040/0x412100).
// Queue entries are (mesh pointer, 0.0f) idx_weights in scene+0x1b4; the scene itself is queued in the global
// registry 0x5fe898 (vector of scene* at +0x3a4) the first time something is added.
// The two leading 0x18-byte blocks are Direct3D viewports, each in a base subobject with its own inline constructor
// (0, 0, 0, 0, 0, 1): the scene constructor then takes the frustum's address after the first viewport's last store, as the
// original does (0x429740). As two plain members instead of bases, cl orders a few unrelated loads differently in byte-exact
// users (Recf0::frame_update 0x4d63b0); so does `fog` as a void* (it is the owner world's ViewParams). Names from the device setup RenderScene::apply_state 0x46ca50 (terrain_a/misc.h).
struct scene_vp_local {
    D3DVIEWPORT9 vp_local;      // 0x00 the pass's viewport
    scene_vp_local() { vp_local.X = 0; vp_local.Y = 0; vp_local.Width = 0; vp_local.Height = 0; vp_local.MinZ = 0.0f; vp_local.MaxZ = 1.0f; }
};
struct scene_vp_alt {
    D3DVIEWPORT9 vp_alt;        // 0x18 used instead when use_alt_vp is set (the whole render target, e.g. 0x80 x 0x80 around a 0x7e inset)
    scene_vp_alt() { vp_alt.X = 0; vp_alt.Y = 0; vp_alt.Width = 0; vp_alt.Height = 0; vp_alt.MinZ = 0.0f; vp_alt.MaxZ = 1.0f; }
};
struct scene : scene_vp_local, scene_vp_alt {
    frustum fr;                 // 0x30 (planes from 0x38, count at 0x34, camera frame at 0xf8)
    texture* target;            // 0x150 render target of this pass
    texture* rt;                // 0x154 render target of the reflection pass
    scene* refl;                // 0x158 lazily created reflection scene
    texture* depth_target;      // 0x15c depth-stencil texture of this pass (mesh_pass.cpp: SetDepthStencilSurface when loaded)
    char lights_active;         // 0x160
    char clip_on;               // 0x161 the user clip plane below is on
    char depth_pass;            // 0x162 the shadow-map camera's pass (terrain_a/game_state.cpp): no lighting, the depth shader
    char pad163;
    int mask_pass;              // 0x164 a silhouette pass (tableau alpha mask, the strategic entity's shadow scene): the fixed mask shaders
    char cull_flip;             // 0x168
    char fog_invert;            // 0x169
    char msaa;                  // 0x16a
    char pad16b;
    float clip_plane[4];        // 0x16c (a, b, c, d; d = the clip height)
    vector4 ambient;            // 0x17c
    unsigned flags;             // 0x18c (bit 8: clear colour, bit 9: clear depth, bit 12: queue at the end instead of the front, register 0x412040)
    DWORD clear_color;          // 0x190
    ptr_list list;              // 0x194 attached objects (obj_c0*) collected this frame (the world's light list is copied here)
    bool registered;            // 0x1a4
    char use_alt_vp;            // 0x1a5 draw with vp_alt instead of vp_local
    char pad1a6[2];
    unsigned mask;              // 0x1a8
    bool active;                // 0x1ac
    char pad1ad[3];
    ViewParams* fog;            // 0x1b0 the owner world's fog / view block (&world::view; terrain_a's fog_rec, terrain_b's sky_t)
    std::vector<idx_weight> queue;   // 0x1b4
    union {
        world* owner;           // 0x1c4 (scene modules)
        mesh_pool* view;        // 0x1c4: the particle mesh pool / camera (psys::update)
    };
    std::vector<mesh*> cache;   // 0x1c8: one mesh per resource
    scene();                    // 0x429740
    ~scene();                   // 0x41f4e0 (src/game/entity.cpp)
    void reset();               // 0x428f30
    mesh* get_mesh(skin* r);                        // 0x412c30
};

MB_LAYOUT32_ASSERT(scene_size_check, sizeof(scene) == 0x1d8);
} // namespace rgl
#endif // RGL_SCENE_REC_H
