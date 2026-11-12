// skin.h: the 0x1f8-byte skin record (one entry of skins.txt; the slices called it model_b, loading_config, SkinSet, MapSceneRec,
// CfgRec, Big (its destructor), FaceRace and TroopMgr). ONE class for every module that touches it:
//   constructor 0x4d3640 (options_panel/ragdoll_models.cpp)      destructor 0x4d3720 (options_panel/world_app.cpp)
//   copy constructor 0x4dc030, operator= 0x4da410 (cls_map)      read 0x4e7b30 (cls_map/progress_bar.cpp)
// The array of them is std::vector<Skin>: g_anim.skins (0x8ed4f78, char_sheet/facegen_misc.cpp), mesh_scene's list (inventory_panel).
// The member order follows skins.txt: id, flags, body / calf / hand / head meshes, face keys, hair meshes, beard meshes, hair /
// beard / face textures, voices, skeleton, scale, blood particles. The ArrSet vectors at +0x48 / +0x58 / +0x68 are the per-slot
// point maps of the beard meshes, the hair meshes and the head (terrain_b Skin::copy_troop fills them).
#ifndef MB_SKIN_H
#define MB_SKIN_H
#include <new>
#include <vector>
#include "../game/tstring.h"
#include "./constraint.h"

// element types the includer completes before it instantiates a member (ArrSet: game/arr_set.h, E176: include/vec_elems.h or a view)
struct ArrSet;
struct E176;
struct lod_group;                                   // rgl/core/resmgr.h (a lod mesh resource, get_lod_mesh 0x41cdc0)
struct named_object;                                // rgl/resource.h (get_object70 0x418cf0)

// one face key (0x34 bytes): a slider of the face generator. Its destructor is the inline one whose vector deleting
// destructor (??_E) the linker keeps from dxframework/d3dapp.cpp (0x401870).
struct FaceKey {
    tstring name;                                   // +0x00 display name (underscores shown as spaces)
    tstring key;                                    // +0x10 language key (the export of rgl/mesh2/gui_edit.cpp)
    int f20;                                        // +0x20
    float f24;                                      // +0x24
    float f28;                                      // +0x28
    unsigned f2c;                                   // +0x2c
    float* delta;                                   // +0x30 4 floats per point of the head mesh (place 0x4c6b30)
    ~FaceKey() { if (delta) delete[] delta; }
};

// one beard mesh (0x18 bytes)
struct BeardMesh {
    tstring name;                                   // +0x00
    int f10;                                        // +0x10
    lod_group* mesh;                                // +0x14 (get_lod_mesh of the name)
};

struct MsStream;                                    // the skin stream (cls_map/progress_bar.cpp: stream_base)
namespace rgl { struct mesh; struct lod_mesh; }

struct Skin {
    tstring id;                                     // +0x00
    unsigned flags;                                 // +0x10
    lod_group* body;                                // +0x14
    lod_group* hand_l;                              // +0x18
    lod_group* hand_r;                              // +0x1c
    lod_group* calf_l;                              // +0x20
    lod_group* calf_r;                              // +0x24
    tstring head_name;                              // +0x28
    lod_group* head;                                // +0x38
    int n_face_keys;                                // +0x3c
    FaceKey* face_keys;                             // +0x40
    BeardMesh* beard_meshes;                        // +0x44
    std::vector<ArrSet> beard_maps;                 // +0x48 (copy ctor 0x4cc750, operator= 0x4cca70)
    std::vector<ArrSet> hair_maps;                  // +0x58
    std::vector<ArrSet> head_maps;                  // +0x68
    std::vector<Constraint> constraints;            // +0x78 (copy ctor 0x4cc830, operator= 0x4ccc00)
    int n_beard;                                    // +0x88
    lod_group* hair_meshes[0x40];                   // +0x8c
    int n_hair;                                     // +0x18c
    int n_face_tex;                                 // +0x190
    int n_hair_tex;                                 // +0x194
    int n_beard_tex;                                // +0x198
    std::vector<tstring> hair_textures;             // +0x19c (copy ctor 0x4cc680, operator= 0x4cc910)
    std::vector<tstring> beard_textures;            // +0x1ac
    std::vector<E176> face_textures;                // +0x1bc (copy ctor 0x4da300, operator= 0x4d9840)
    struct Voices { int v[7]; } voices;             // +0x1cc (the copy constructor copies these seven as one block: rep movsd)
    named_object* skeleton;                         // +0x1e8
    float scale;                                    // +0x1ec
    int blood1;                                     // +0x1f0
    int blood2;                                     // +0x1f4

    Skin();                                         // 0x4d3640
    ~Skin();                                        // 0x4d3720
    // (the copy constructor 0x4dc030 is the implicit one, emitted out of line by cls_map/cls_map.cpp)
    Skin& operator=(const Skin& src);               // 0x4da410
    void read(MsStream* s);                         // 0x4e7b30
    void relax(int skip, float* v);                 // 0x4735e0 (terrain_b)
    void apply_group(ArrSet* dst, int idx, rgl::mesh* obj);         // 0x470db0 (terrain_b: nearest base point per point of obj)
    void copy_troop(ArrSet* dst, const lod_group* r);               // 0x473570 (terrain_b)
    void place(int e);                              // 0x4c6b30 (rgl/mesh2/gui_edit.cpp: morph-target pose of face key e)
    void build_mirror(rgl::mesh* m, int* remap, int* count, char flip);         // 0x47c1d0 (mesh_slot/mesh_ops.cpp)
    void blend(rgl::mesh* m, int unused, const float* weights);                 // 0x47c6b0
    void blend_indexed(rgl::mesh* m, void* set, int* index, const float* weights);   // 0x47c810
    void blend_all(rgl::lod_mesh* m, void* set, const float* weights);          // 0x47ddf0 (mesh_slot/mesh_scene.cpp)
    void use_carry(rgl::lod_mesh* l, const float* weights);                     // 0x47de60
    __declspec(noinline) void collect(rgl::lod_mesh** a, rgl::lod_mesh** b, rgl::lod_mesh** c, const float* weights,
                                      int item, int set, int material, int variant, float scale, float time);   // 0x47f650 (mesh_slot/mesh_char.cpp)
};

// the skin vector of the Animator (g_anim +0x18) and of mesh_scene: a VC7.1 std::vector<Skin> (allocator word, _Myfirst,
// _Mylast, _Myend). Its destructor is 0x4eb810 (emitted by game/char_sheet/facegen_misc.cpp, the unit that defines g_anim;
// the atexit function 0x548510 reaches it)
typedef std::vector<Skin> SkinVec;

#endif // MB_SKIN_H
