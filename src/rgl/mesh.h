// mesh.h: the 0x260-byte mesh object (rgl::mesh, vtable 0x54a6f4, ctor 0x412a90, dtor 0x4127f0), its element types, the
// particle emitter that hangs off it (mesh+0xb0), the render scene it is drawn into (0x1d8 bytes, ctor 0x429740), the
// global scene registry (0x5fe898) and the lod mesh. One definition for src/rgl (mesh_obj.cpp 0x40e940..0x413b5f, psys,
// the scene objects), src/game/terrain_b / ui and the resource loaders. Merged from the former mesh.h (the slices'
// copies s00412a90, s00413320, s00413c50, s00413e50) and mesh_obj.h (mesh_obj.cpp's complete layout: the destructor
// 0x4127f0, copy-assignments and particle code need `skin_vert`, `bone_influence`, `frame_t` with its out-of-line
// ctor, `keyframes` with its destructor, the owned attachment pointer, the scene's registration fields), and
// src/rgl/scene/scene.h's full scene (constructor 0x429740, reset 0x428f30, Ctx::render_scene 0x429890, ~scene 0x41f4e0).
//
// All names provisional except where an address is cited; every offset below is read or written by a
// function of one of those modules (the citing function address is given).
#ifndef RGL_MESH_H
#define RGL_MESH_H
#include <vector>
#include "../include/layout32.h"
#include <stddef.h>
#include <string.h>
#include "./rgl_string.h"
#include "./rgl_math.h"
#include "../include/vec.h"
#include "../include/mesh_prims.h"
#include "../include/mesh_parts.h"       // vertex60, frame_t, obj_c0, slot, render_info, keyframes
#include "./resource.h"
#include "../include/frustum.h"
#include "./scene_rec.h"           // scene, ptr_list, idx_weight

namespace rgl {

struct matrix; typedef matrix mat4;

// std::vector helpers: the original inlines push_back's fast path and indexes through &*begin()
template<class T> __forceinline void vpush(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}
template<class T> inline T& vat(std::vector<T>& v, size_t i) { return (&*v.begin())[i]; }

// ---- element types of the mesh's std::vectors ---------------------------------------------------------------
// 0x30 bytes, vector at +0x60: per-vertex skinning record (build_skin 0x411d20 writes w[], bone[], bonef[])
struct skin_vert { float w[4]; int bone[4]; float bonef[4]; };
// 0x14 bytes, vector at +0x80: the vertices one bone influences (build_skin 0x411d20)
struct bone_influence { int bone; std::vector<idx_weight> verts; };
// face20 (0x20 bytes, vector at +0x90; a, b, c at +0x10: add_face 0x411920) is in src/include/mesh_prims.h

struct scene;
struct psys;                                   // the particle emitter: psys.h's / scene/emitter.h's views; mesh_obj.h's `emitter` is the simulation view

// ---- the mesh ------------------------------------------------------------------------------------------------
struct mesh;
struct scene;

struct mesh : named_object {
    mesh* parent;                // 0x18
    string name2;                // 0x1c
    skin* res;                   // 0x2c: the material it is drawn with (def->refcount at +0x64 is released by ~mesh)
    unsigned flags;              // 0x30
    int billboard;               // 0x34: 0 world frame, 1 billboard about the up axis, else a full camera-facing billboard (render_scene 0x46e590)
    bool force_alpha;            // 0x38: forces alpha blending (setup_material 0x40f3f0; scene::get_mesh sets it)
    unsigned color;              // 0x3c: packed ARGB multiplier
    unsigned color2;             // 0x40: second packed ARGB colour ("vMaterialColor2", render_scene 0x46e590; -1 by the ctor)
    float radius;                // 0x44: bounding radius (the point-light range test; 10000 for scene::get_mesh's meshes)
    float anim_phase;            // 0x48: texture animation phase (render_scene 0x46e590 multiplies the frame count by it: fmul [mesh+0x48])
    int mirrored;                // 0x4c: mirrored geometry: render_scene swaps the cull mode
    std::vector<vertex60> vertices;          // 0x50
    std::vector<skin_vert> skin_verts;       // 0x60 per-vertex skinning (build_skin 0x411d20)
    std::vector<point20> points;             // 0x70
    std::vector<bone_influence> influences;  // 0x80 the vertices each bone moves (build_skin 0x411d20)
    std::vector<face20> faces;               // 0x90
    char local_faces;            // 0xa0: the faces are this mesh's own (else the parent's)
    char local_points;           // 0xa1: the points / vertices / keyframes are this mesh's own
    keyframes kf;                // 0xa4
    std::vector<psys*> kids;     // 0xb0: particle systems (render calls psys::update 0x414960 on them); the class is rgl::psys (scene/emitter.h)
    obj_c0* p_c0;                // 0xc0 (deleted by ~mesh)
    float posed_time;            // 0xc4 the keyframe time the points were last posed at (update_morph 0x413320)
    float anim_time;             // 0xc8 the requested keyframe time (set_time 0x433190)
    matrix local_frame;          // 0xcc the frame relative to the owner (world_frame = frame * local_frame)
    unsigned mask;               // 0x10c
    slot slots[3];               // 0x110
    matrix world_frame;          // 0x194 the frame it was last rendered with (render 0x413750)
    matrix tex_matrix;           // 0x1d4 the texture transform (D3DTS_TEXTURE0..; its u row is the uv offset)
    int sort_order;              // 0x214 sort-key term (compute_sortkey 0x412980: << 20; 0x80 by the ctor)
    __int64 sortkey;             // 0x218
    matrix* bone_mats;           // 0x220: 32 bone matrices, allocated on first skinning (FxDef::skin 0x4291c0; delete[] in ~mesh)
    const vec_table* bone_origins;   // 0x224: the skeleton's bone-origin table ("matBoneOriginArray", FxDef::skin 0x4291c0)
    int nbones;                  // 0x228: max bone index + 1 ("matWorldArray" count)
    aabb bounds;                 // 0x22c

    mesh();                                         // 0x412a90
    virtual ~mesh();                                // 0x4127f0
    virtual int type();                             // 0x48b1f0 (returns 1)
    virtual void compute_sortkey();                 // 0x412980

    // parent-chain accessors: walk up until a mesh that owns its data. In-class (so inline, COMDAT) loops: the
    // out-of-line copies are 0x40eba0 .. 0x40ec60. They must stay inline-linkage functions (a non-inline
    // definition in the .cpp gives its callers interprocedural register information the original's callers
    // lack: should_batch 0x40f7f0 differs) and __declspec(noinline) (cl expands the loop into update_morph
    // otherwise). Several callers test the flag themselves first (`local_points ? points : parent->get70()`).
    __declspec(noinline) std::vector<point20>& get70() { mesh* n = this; while (!n->local_points) n = n->parent; return n->points; }     // 0x40eba0
    __declspec(noinline) std::vector<vertex60>& get50() { mesh* n = this; while (!n->local_points) n = n->parent; return n->vertices; }    // 0x40ebd0
    __declspec(noinline) keyframes& get_kf() { mesh* n = this; while (!n->local_points) n = n->parent; return n->kf; }               // 0x40ec00
    __declspec(noinline) std::vector<face20>& get90() { mesh* n = this; while (!n->local_faces) n = n->parent; return n->faces; }     // 0x40ec30
    __declspec(noinline) std::vector<skin_vert>& get60() { mesh* n = this; while (!n->local_points) n = n->parent; return n->skin_verts; }  // 0x40ec60
    // the call sites that expand one level of the parent-chain walk themselves (entity::render 0x420310)
    std::vector<point20>& get70_rec() { return local_points ? points : parent->get70(); }
    std::vector<vertex60>& get50_rec() { return local_points ? vertices : parent->get50(); }
    std::vector<face20>& get90_rec() { return local_faces ? faces : parent->get90(); }
    std::vector<skin_vert>& get60_rec() { return local_points ? skin_verts : parent->get60(); }
    // the out-of-line copies of the accessors (a call to 0x40eba0 .. 0x40ec60 instead of the expanded walk)
    std::vector<point20>& get70_();                 // 0x40eba0
    std::vector<vertex60>& get50_();                // 0x40ebd0
    std::vector<face20>& get90_();                  // 0x40ec30
    std::vector<skin_vert>& get60_();               // 0x40ec60
    void compute_bounds();                          // 0x419300 (bounds of the points)
    void linearise_vertex_colors();                 // 0x419360 (gamma 2.2 to linear, copy to +0x58)
    void compute_tangents();                        // 0x41ce80
    int count(int kind);                            // 0x468620 (1: faces * 3, else vertices)
    void fill_vertices(int* p, const unsigned* fmt);   // 0x46c4f0
    void fill_indices(unsigned short* out);         // 0x46c920
    void transform(mesh* src, const matrix* m, unsigned color);   // 0x47aa40 (ret 0xc)
    void multiply_color(const vector4* tint);       // 0x47b3b0

    int add_vertex_rec(int index, const vertex60& v);   // add_vertex's form create_shadow_mesh 0x42b080 reaches: vertices grows by exactly ONE element per ground vertex (0x42b590 _Insert_n, 0x4106f0 _Ufill, 0x40ff60)
    void append_plain(mesh* other, const mat4* m);  // 0x4520b0 (ret 8, tail of append_skinned)
    void transform(mat4 m);                         // 0x44f3c0 (matrix by value, ret 0x40)
    void build_panel(float* base, float* size, float* uv0, float* uv1, int* col);   // 0x4de3e0 (loading_screen.cpp: the frame quads)
    void ensure_local();                            // 0x412ee0
    int add_point(const vector4* p);                // 0x412f50
    int add_face(int a, int b, int c);              // 0x411920
    void append(mesh* other, const matrix* m);      // 0x412fe0
    void update_morph();                            // 0x413320
    int render(scene* sc, const matrix& frame);     // 0x413750
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c);   // 0x413c50
    int should_batch();                             // 0x40f7f0
    void setup_material();                          // 0x40f3f0
    void ensure_skin();                             // 0x412700
    void build_skin();                              // 0x411d20
    void update_emitters(const float* dt, const matrix& frame);   // 0x4126a0
    // ---- src/game/party2 (the map party's model meshes) ----
    bool in_use() const;                            // 0x4b17d0 (a keyframe track is present: own, or the parent's)
    void set_time(const float* t);                  // 0x433190 (ret 4)
    void translate(const vector4* d);               // 0x44f240 (ret 4)
    mesh* clone();                                  // 0x41c6d0 (src/game/party2: s0049aca0 / s0049b630)
    void clear_geometry();                          // 0x428e80
    void compute_normals();                         // 0x451c50
    // ---- src/game/agent5 (agent_hud.cpp: the HUD's item and status meshes) ----
    void append_skinned(mesh* other, const matrix* m, int bone, unsigned color, char flip);   // 0x44eb00 (ret 0x14)
    void add_quad(const vector4* p0, const vector4* p1, const vec2* uv0, const vec2* uv1, const unsigned* color);   // 0x44e970 (ret 0x14: a two-point quad; world_data/obj_tbl.cpp)
    void scale(float s);                            // 0x44f0c0 (ret 4)
    void set_color(vector4 c);                      // 0x47af30 (ret 0x10: the colour multiplier of every vertex)
    void set_bone_index(int i);                     // 0x47b360 (ret 4)
    mesh* copy_of();                                // 0x4958f0 (same parent, material, flags and name; src/game/world_data/mesh_hud.cpp)
    void set_part(const matrix& frame);             // 0x495ba0 (ret 4)
    void set_alpha(float a);                        // 0x495f00 (ret 4: the alpha lane of every vertex colour)
    void calc_bounds(vector4* lo, vector4* hi);     // 0x438d80 (ret 8)
    void set_offset(const vector4& d);              // 0x4fb8e0 (agent_hud.cpp: shifts every point along its vertex normal)
    void darken_vertices();                         // 0x4fbef0 (agent_hud.cpp)
};

// The game's own growable array (VC7.1 vector layout: allocator slot, first, last, end) whose clear() frees the
// block, unlike std::vector::clear. Also used by ../../game/entity.h for the collision-shape arrays.
template<class T> struct vec {
    int alloc;
    T* first; T* last; T* end;
    void clear() { if (first != 0) operator delete(first); first = 0; last = 0; end = 0; }
    unsigned size() const { return first == 0 ? 0 : (unsigned)(last - first); }
    unsigned capacity() const { return first == 0 ? 0 : (unsigned)(end - first); }
    T& operator[](unsigned i) const { return first[i]; }
};


struct world;


// One level of a lod mesh (16 bytes): the meshes drawn at that distance. The list owns them: its destructor deletes
// every mesh (0x419420; inline, as the original: entity::clear 0x420960 inlines it, resmgr.cpp and build_appearance
// call the out-of-line copy).
struct mesh_list : std::vector<mesh*> {
    ~mesh_list() { for (iterator it = begin(); it < end(); ++it) delete *it; }      // 0x419420
    void forget();                                  // 0x419470 (rgl/core/resmgr.cpp: null the entries, free the block)
    void render_all(scene* sc, const matrix& m);    // 0x413ac0
    void set_color(const int& c);                   // 0x421990 (rgl/scene/mesh_list_color.cpp: tints every mesh)
    void cap_all(const float* limit);               // 0x438f90 (game/agent2/agent_stats.cpp)
    void apply(matrix m);                           // 0x46c990 (game/terrain_a/terrain_render.cpp: m * every mesh's local frame)
    void append(mesh_list* src, int bone, const matrix* m, const unsigned* color);   // 0x47b480 (game/mesh_slot/mesh_ops.cpp)
};
// The lod mesh (0xe0 bytes): the resource manager's lod meshes (resmgr::make_lod_mesh 0x41ca60 / get_lod_mesh
// 0x41cdc0) and their clones, held by the scene entities (entity::lods), the effect elements (FxDef::groups) and the
// agent's display slots. Five levels of meshes; render draws lists[cur].
struct lod_mesh {
    aabb bounds;                 // +0 (ctor 0x40eaf0)
    mesh_list lists[5];          // +0x34
    int count;                   // +0x84 levels in use
    int cur;                     // +0x88 the level select() picked
    matrix frame;                // +0x8c applied before the meshes' own frames
    unsigned mask;               // +0xcc visibility (0xffff shown, 0x2000 / 0 hidden: Agent::show / hide)
    string name;                 // +0xd0
    lod_mesh();                                     // 0x41c2a0 (rgl/core/resmgr.cpp)
    void render(scene* sc, const matrix& m);        // 0x413b00
    void select(const float* dist);                 // 0x4205b0 (rgl/scene/scene_objects.cpp)
    void cap_all(const float* limit);               // 0x439000 (game/agent2/agent_stats.cpp)
    void add_mesh(int level, mesh* m);              // 0x441ff0 (game/agent4/agent_logic.cpp)
    void apply(const matrix* m);                    // 0x46c9f0 (game/terrain_a/terrain_render.cpp)
    lod_mesh* clone();                              // 0x47a370 (game/terrain_b/mission.cpp)
    void set_color(unsigned color);                 // 0x47b5b0 (game/mesh_slot/mesh_ops.cpp: the bone index of every mesh)
    void transform(const matrix& m);                // 0x47b610
    void append(lod_mesh* src, int bone, const matrix* m, const unsigned* color);   // 0x47b690
    void apply_anim_all(const float* t);            // 0x47dd90 (game/mesh_slot/mesh_scene.cpp)
    void set_material(::skin* m);                   // 0x48cfc0 (game/mission_ui2/mission_3.cpp: every mesh draws with m; refcounts moved)
    __forceinline bool ready() { return cur < count; }
};

// ---- layout checks (the sizes and offsets that every module's view of the mesh / scene / lod mesh has always had) ----
#define RGL_MESH_AT(m, off) MB_LAYOUT32_ASSERT(mesh_##m##_at_##off, (offsetof(mesh, m) == (off)))
RGL_MESH_AT(parent, 0x18);   RGL_MESH_AT(name2, 0x1c);   RGL_MESH_AT(res, 0x2c);     RGL_MESH_AT(flags, 0x30);
RGL_MESH_AT(color, 0x3c);    RGL_MESH_AT(anim_phase, 0x48);     RGL_MESH_AT(vertices, 0x50);     RGL_MESH_AT(skin_verts, 0x60);
RGL_MESH_AT(points, 0x70);      RGL_MESH_AT(influences, 0x80);     RGL_MESH_AT(faces, 0x90);     RGL_MESH_AT(local_faces, 0xa0);
RGL_MESH_AT(kf, 0xa4);       RGL_MESH_AT(kids, 0xb0);    RGL_MESH_AT(p_c0, 0xc0);    RGL_MESH_AT(local_frame, 0xcc);
RGL_MESH_AT(mask, 0x10c);    RGL_MESH_AT(slots, 0x110);  RGL_MESH_AT(world_frame, 0x194);   RGL_MESH_AT(tex_matrix, 0x1d4);
RGL_MESH_AT(sort_order, 0x214);    RGL_MESH_AT(sortkey, 0x218); RGL_MESH_AT(bone_mats, 0x220);  RGL_MESH_AT(bounds, 0x22c);
RGL_MESH_AT(billboard, 0x34); RGL_MESH_AT(force_alpha, 0x38); RGL_MESH_AT(color2, 0x40); RGL_MESH_AT(radius, 0x44);
RGL_MESH_AT(mirrored, 0x4c); RGL_MESH_AT(bone_origins, 0x224); RGL_MESH_AT(nbones, 0x228);
MB_LAYOUT32_ASSERT(mesh_size_check, (sizeof(mesh) == 0x260));
#define RGL_SCENE_AT(m, off) MB_LAYOUT32_ASSERT(scene_##m##_at_##off, (offsetof(scene, m) == (off)))
RGL_SCENE_AT(fr, 0x30);      RGL_SCENE_AT(target, 0x150); RGL_SCENE_AT(list, 0x194);  RGL_SCENE_AT(mask, 0x1a8);
RGL_SCENE_AT(queue, 0x1b4);  RGL_SCENE_AT(cache, 0x1c8);
MB_LAYOUT32_ASSERT(scene_size_check, (sizeof(scene) == 0x1d8));
MB_LAYOUT32_ASSERT(lod_mesh_size_check, (sizeof(lod_mesh) == 0xe0));

extern int g_vert_budget;        // 0x5a9c34 base vertex budget of should_batch (provisional name)

} // namespace rgl
#endif
