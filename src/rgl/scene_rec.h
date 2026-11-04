// scene_rec.h: the render scene rgl::scene (0x1d8 bytes), ONE definition for every universe (src/rgl/mesh.h, src/game/rgl_mesh.h, the
// game modules with local mesh views: ShadowMap embeds one). Needs only pointers to the mesh, texture and world.
#ifndef RGL_SCENE_REC_H
#define RGL_SCENE_REC_H
#include <vector>
#include "./rgl_math.h"
#include "../include/frustum.h"
struct texture;
struct skin;
namespace rgl {
struct mesh;
struct mesh_pool;
struct world;
// 8 bytes: (vertex index, weight). Also the scene's queue entries (key = the mesh pointer, weight 0).
struct idx_weight { int index; float weight; };
// std::vector<void*> with its copy-assignment out of line (0x43f0a0)
struct ptr_list : std::vector<void*> {
    ptr_list& operator=(const ptr_list& o);                      // 0x43f0a0
};

// The render scene (0x1d8 bytes, ctor 0x429740, reset 0x428f30, dtor 0x41f4e0 in src/game/entity.cpp): viewport / camera block
// with the view frustum at +0x30. Offsets from the constructor 0x429740 (the field stores), reset 0x428f30, Ctx::render_scene
// 0x429890 (reflection pass), ~scene 0x41f4e0 and the mesh code (mesh::render 0x413750, scene_registry 0x412040/0x412100).
// Queue entries are (mesh pointer, 0.0f) idx_weights in scene+0x1b4; the scene itself is queued in the global
// registry 0x5fe898 (vector of scene* at +0x3a4) the first time something is added.
struct scene {
    int f0, f4, f8, fc, f10;    // 0x00
    float f14;                  // 0x14
    int f18, f1c, f20, f24, f28;    // 0x18
    float f2c;                  // 0x2c
    frustum fr;                 // 0x30 (planes from 0x38, count at 0x34, camera frame at 0xf8)
    texture* target;            // 0x150 render target of this pass
    texture* rt;                // 0x154 render target of the reflection pass
    scene* refl;                // 0x158 lazily created reflection scene
    int f15c;                   // 0x15c
    char f160, f161, f162, pad163;
    int f164;                   // 0x164
    char f168, f169, f16a, pad16b;
    int f16c, f170, f174;       // 0x16c
    float f178;                 // 0x178 clip height
    vector4 ambient;            // 0x17c
    unsigned flags;             // 0x18c (bit 12: queue at the end instead of the front, register 0x412040)
    int f190;                   // 0x190
    ptr_list list;              // 0x194 attached objects (obj_c0*) collected this frame (the world's light list is copied here)
    bool registered;            // 0x1a4
    char f1a5, pad1a6[2];       // 0x1a5
    unsigned mask;              // 0x1a8
    bool active;                // 0x1ac
    char pad1ad[3];
    void* f1b0;                 // 0x1b0
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

typedef char scene_size_check[sizeof(scene) == 0x1d8 ? 1 : -1];
} // namespace rgl
#endif // RGL_SCENE_REC_H
