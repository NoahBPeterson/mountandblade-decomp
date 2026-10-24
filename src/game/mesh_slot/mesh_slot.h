// mesh_slot.h: declarations shared by the modules of src/game/mesh_slot (TU 63 "mesh" 0x47aa40..0x4875e0 and TU 64
// "slot" 0x4875e0..0x488560 of work/migrate/tu_map.csv). Views (derived classes that only add member functions:
// a class cannot be extended from outside), globals and cdecl helpers; offsets cite the function that proves them.
#ifndef MESH_SLOT_H
#define MESH_SLOT_H
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <new>
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh_x.h"
#include "../../rgl/scene/world.h"
#include "../tstring.h"
#include "../arr_set.h"
#include "../terrain_a/terrain.h"
#include "../terrain_b/troop_mgr.h"
#include "../../include/rec1c8.h"
#include "../../include/timer.h"
#include "../../include/shape.h"
#include "../../include/item_kind.h"
#include "../item_ref.h"
#include "../mission.h"
#include "../shadow_map.h"

#define NOINL __declspec(noinline)     // keeps the definition's name visible to tools/difftest/slice.py's marker scan
using rgl::vector4;
using rgl::matrix;

extern __int64 g_ticks[];                       // 0x5e2948 clock table (64-bit, 1e-5 s ticks)
extern int g_use_shaders;                       // 0x5e28fc programmable pipeline in use
extern float g_ramp_gamma;                      // 0x5a9bfc gamma exponent of the software colour path (0x463790)
// rgl::unpack_color returns nothing in rgl_math.h; the callers below use its result pointer (eax = out)
typedef vector4* (__cdecl *unpack_color_fn)(vector4* out, const unsigned* argb);
#define UNPACK_COLOR ((unpack_color_fn)&rgl::unpack_color)
extern char* g_game_base;                       // 0x5e2930 g_game as a byte pointer (Game, src/game/party/game.h, and the GameData views)
unsigned pack_signed_color(const float* c);     // 0x461370 (cdecl)

// ---- members of rgl::mesh defined in this TU (0x47aa40..0x47b3b0) ------------------------------------------------
// mesh_x (the members above) is src/rgl/mesh_x.h
struct mesh_ms : rgl::mesh {
    void compute_normals();                                            // 0x451c50
};
struct lod_mesh_ms : rgl::lod_mesh {
    rgl::lod_mesh* clone();                                            // 0x47a370 (src/game/terrain_b/mission.cpp)
};
struct scene_ms2 : rgl::scene {
    void reset();                                                      // 0x428f30
};

// resource-manager members this TU calls (the lookups of src/rgl/resource.h plus the ones core/resmgr.h defines)
struct resmgr_ms : resource_manager {
    std::vector<rgl::mesh*> find_meshes(const tstring* prefix);          // 0x41b960 (hidden return pointer, ret 4)
    rgl::lod_mesh2* make_lod_mesh(const tstring& pat, bool copy);        // 0x41ca60 (ret 8)
    rgl::lod_mesh2* get_lod_mesh(const tstring& name);                   // 0x41cdc0
};
#define g_rms (*(resmgr_ms*)&g_resmgr)

// ---- the lod mesh and its lists: members defined here -------------------------------------------------------------
#include "../lod_mesh_x.h"             // mesh_list_x / lod_mesh_x (shared with terrain_a/misc.h)

// ---- the scene container's lighting pass --------------------------------------------------------------------------
struct Geom_t;
struct world_x : rgl::world {
    rgl::entity* create_entity_x(int id);                                                                    // 0x428440
    void set_geom(void* g);                                                                                  // 0x479980
    void relight(int use_shaders);                                                                           // 0x472ed0
    void main_camera(float* v, const float* sun_dir, const float* centre);                                   // 0x42ad60
    void light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale);   // 0x43ac40
    void light_mesh(rgl::mesh* m, const matrix* frame, unsigned flags);                                    // 0x47b7b0
};


// ---- the character / troop mesh owner (0x1f8 bytes): TroopMgr (src/game/terrain_b/troop_mgr.h) is the same object seen
// by terrain_b (0x470db0 apply_group, 0x473570 copy_troop); the fields past +0x3c are the ones this TU's functions read.
struct bone34 {                  // 0x34 bytes (EChar::blend 0x47c6b0: weights at +0x24/+0x28, per-point offsets at +0x30)
    char pad[0x24];
    float w0;                    // 0x24
    float w1;                    // 0x28
    char pad2[4];
    vector4* trans;              // 0x30
};
struct SetRec { char pad0[0x14]; rgl::lod_mesh* mesh; };       // 0x18-byte entry of EChar::sets; its lod mesh is at +0x14 (0x47f650)
struct Blend { char b[0x38]; };                                 // 0x38-byte entries of the blend tables at +0x4c / +0x5c (0x47f650)
struct EChar : TroopMgr {
    int bone_count;              // 0x3c (0x47c6b0)
    bone34* bones;               // 0x40 (0x47c6b0)
    SetRec* sets;                // 0x44 (0x47f650)
    char pad48[4];
    Blend* blend_a;              // 0x4c (0x47f650)
    char pad50[0xc];
    Blend* blend_b;              // 0x5c (0x47f650)
    char pad60[8];
    std::vector<ArrSet> arrsets; // 0x68 (0x47de60)
    char pad78[0x88 - 0x78];
    int set_count;               // 0x88 (0x47f650)
    rgl::lod_mesh* items[0x40];  // 0x8c
    int item_count;              // 0x18c
    int mat_count;               // 0x190
    int variant_count;           // 0x194
    int name_count;              // 0x198
    char pad19c[4];
    tstring* names_a;            // 0x1a0
    char pad1a4[0xc];
    tstring* names_b;            // 0x1b0
    char pad1b4[0xc];
    void* mats;                  // 0x1c0 (Ramp*, 0xb0 bytes each)
    char pad1c4[0x1f8 - 0x1c4];

    void blend(rgl::mesh* m, int unused, const float* weights);                 // 0x47c6b0
    void blend_indexed(rgl::mesh* m, void* set, int* index, const float* weights);   // 0x47c810
    void build_mirror(rgl::mesh* m, int* remap, int* count, char flip);         // 0x47c1d0 (ret 0x10, this in ecx)
    void blend_all(rgl::lod_mesh* m, void* set, const float* weights);          // 0x47ddf0
    void use_carry(rgl::lod_mesh* l, const float* weights);                     // 0x47de60
};

// the colour ramp / material record of the character owner (0xb0 bytes, ramp.h: Ramp; terrain_a/game_state.h includes it too)
#include "../ramp.h"

// the animation state (global object at 0x8ed4f60)
struct Animator {
    int set;                     // 0x00
    int item;                    // 0x04
    int cur;                     // 0x08
    int variant;                 // 0x0c
    char pad10[4];
    int material;                // 0x14
    char pad18[4];
    EChar* chars;                // 0x1c
    char pad20[0xc];
    float weights[63];           // 0x2c (one weight per bone, up to +0x128)
    float scale;                 // 0x128
    float time;                  // 0x12c
    void collect(char flag, rgl::lod_mesh** a, rgl::lod_mesh** b, rgl::lod_mesh** c);   // 0x47fb10
    void set_anim_a(void* a);                    // 0x473800
    void set_anim_b(void* a, void* b, int kind); // 0x4746e0
    void end_anim(void* a);                      // 0x4eb660 (ret 4; src/game/mission5: the closing call of reset)
    void reset(void* a, void* b);                // 0x520520 (ret 8; src/game/mission5: re-seeds the axes and smooths the 63 weights)
};
typedef char animator_weights_check[(offsetof(Animator, scale) == 0x128) ? 1 : -1];
extern Animator g_anim;          // 0x8ed4f60


// ---- members of Terrain (src/game/terrain_a/terrain.h) defined in this TU --------------------------------------------
// The object is the Terrain of terrain_a/terrain.h when the world map is up and the Mission (game/mission.h) in a battle: the
// same ~138 MB global (props at +0x6688 is Mission::props, wx/wy at +0x270 Terrain::wx). TerrainX adds the members past
// the end of Terrain (arr3 ends at 0x845e030) that the scene-entity builder uses.
extern ShadowMap g_probe;                       // 0xa70248 the light-camera / shadow object ("g_psys" in the slices)
extern int g_mesh_quality;                      // 0x5aa084 (2: high detail; shader quality)
struct ShadowMapP : ShadowMap {
    rgl::scene& sc() { return *(rgl::scene*)((char*)this + 0x18); }
    NOINL void build_hw();   // 0x47fce0
    NOINL void build_sw();   // 0x47fdd0
    NOINL void render();     // 0x47fef0
};
struct TerrainX : terrain_obj {
    // (TerrainLayer::f14 as the float scale of a layer's texture coordinates: layer[l].f14f())
    char pad_845e030[0x845e784 - 0x845e030];
    tstring name;                                    // +0x845e784 scene name (0x47cae0: shorter than 4 = none)
    rgl::entity* sent[9];                            // +0x845e794 3x3 entities by the sign of the mesh centre (0x47cae0)
    std::vector<rgl::mesh*> smeshes;                 // +0x845e7b8 the meshes find_meshes returned (allocator at +0x845e7b8)
    void snap(rgl::mesh* m);                         // 0x47c9b0
    void build_entity();                             // 0x47cae0
    void light_entities(rgl::mesh** list, int count);   // 0x47ce00
    void light_props();                              // 0x47ced0
    void gen_set_init(particle_list* s);             // 0x47df80
    void light_particles(particle_list* l, int mode);   // 0x472020 (Terrain::light_all with the flag as an int)
    void create_water(void** out);                   // 0x480db0
    void destroy_water(int a);                       // 0x4814a0
    void emit_particle(int type, int v, int x, int y, float z);   // 0x47ff60
    void tessellate();                               // 0x481570
    void generate(int seed, int* objs, char flag);   // 0x481d10
    void regenerate_water(void** out, int mode);     // 0x482170
};

#endif // MESH_SLOT_H
