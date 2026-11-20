// vec_elems.h: the element types of the std::vector<T> instantiations shared by src/rgl/mesh2 (mesh_2.cpp, 0x4cc000..0x4cd800)
// and src/game/cls_map (0x4d9840..0x4dc1c0): FaceTexture (0xb0: a skin's face texture) and the morph constraint. The ragdoll bone
// record of the same vectors (0x50, the "E80" of the cls_map slices) is rgl/core/bone.h's RagdollBone, its capsules CapDef.
#ifndef MB_VEC_ELEMS_H
#define MB_VEC_ELEMS_H
#include <vector>
#include <stdlib.h>
#include "../game/tstring.h"

#include "./scalar4.h"
#include "./constraint.h"             // Term, Constraint (the 0x204-byte morph constraint of a skin)
#include "./face_texture.h"          // FaceTexture (0xb0: a skin's face texture)

#endif // MB_VEC_ELEMS_H
