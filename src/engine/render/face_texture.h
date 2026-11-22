// face_texture.h: FaceTexture, the 0xb0-byte element of Skin::face_textures (src/engine/render/skin.h). ONE definition: vec_elems.h
// (poly_mesh_edge_face_ops.cpp, cls_map) and game/data/elem_calls.h (the out-of-line copy loops of module_record_loaders.cpp / mission_item_records.cpp)
// include it. Its copy constructor 0x4cd170 / operator= 0x4cd380 are the implicit ones, emitted in poly_mesh_edge_face_ops.cpp.
#ifndef MB_FACE_TEXTURE_H
#define MB_FACE_TEXTURE_H
#include <vector>
#include "engine/core/tstring.h"
#include "engine/math/rgl_math.h"            // rgl::vector4 (FaceTexture::hair_colors)

// One face texture of a skin (Skin::face_textures, module_skins' face_textures: name, colour, hair materials, hair colours);
// Skin::read 0x4e7b30 (game/ui/screens/progress_bar_and_load_screens.cpp) fills it, Skin::collect 0x47f650 (game/mission/character_meshes.cpp) applies it.
struct skin;
struct FaceTexture {                        // 0xb0
    tstring name;                           // 0x00
    skin* material;                         // 0x10 get_material(name)
    skin* material_lod;                     // 0x14 get_material(name + ".LOD")
    unsigned color;                         // 0x18
    std::vector<int> hair_materials;        // 0x1c indices into Skin::hair_textures (copy constructor folded into 0x4cc280, _Insert_n 0x43e7b0)
    rgl::vector4 hair_colors[8];            // 0x2c the hair colour ramp (the first two (1,1,1))
    int n_hair_colors;                      // 0xac (2 by default)
    FaceTexture();                          // 0x4ce680 (game/editor/poly_mesh_edge_face_ops.cpp)
    unsigned hair_color(float* age);        // 0x463790 (game/terrain/scene_paths_and_wind_noise.cpp): the ramp sampled at *age
};

#endif // MB_FACE_TEXTURE_H
