// face_texture.h: FaceTexture, the 0xb0-byte element of Skin::face_textures (src/include/skin.h). ONE definition: vec_elems.h
// (mesh_2.cpp, cls_map) and options_panel/elem_calls.h (the out-of-line copy loops of module_load.cpp / mission_items.cpp)
// include it. Its copy constructor 0x4cd170 / operator= 0x4cd380 are the implicit ones, emitted in mesh_2.cpp.
#ifndef MB_FACE_TEXTURE_H
#define MB_FACE_TEXTURE_H
#include <vector>
#include "../game/tstring.h"
#include "../rgl/rgl_math.h"            // rgl::vector4 (FaceTexture::hair_colors)
#include "./scalar4.h"

// One face texture of a skin (Skin::face_textures, module_skins' face_textures: name, colour, hair materials, hair colours);
// Skin::read 0x4e7b30 (cls_map/progress_bar.cpp) fills it, Skin::collect 0x47f650 (mesh_slot/mesh_char.cpp) applies it.
struct skin;
struct FaceTexture {                        // 0xb0
    tstring name;                           // 0x00
    skin* material;                         // 0x10 get_material(name)
    skin* material_lod;                     // 0x14 get_material(name + ".LOD")
    unsigned color;                         // 0x18
    std::vector<int> hair_materials;        // 0x1c indices into Skin::hair_textures
    rgl::vector4 hair_colors[8];            // 0x2c the hair colour ramp (the first two (1,1,1))
    int n_hair_colors;                      // 0xac (2 by default)
    FaceTexture();                          // 0x4ce680 (rgl/mesh2/mesh_2.cpp)
    unsigned hair_color(float* age);        // 0x463790 (terrain_a/game_state.cpp): the ramp sampled at *age
};

#endif // MB_FACE_TEXTURE_H
