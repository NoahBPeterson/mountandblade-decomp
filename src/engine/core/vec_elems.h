// vec_elems.h: the element types of the std::vector<T> instantiations shared by src/game/editor (poly_mesh_edge_face_ops.cpp, 0x4cc000..0x4cd800)
// and src/game/ui/screens (0x4d9840..0x4dc1c0): FaceTexture (0xb0: a skin's face texture) and the morph constraint. The ragdoll bone
// record of the same vectors (0x50, the "E80" of the cls_map slices) is engine/physics/bone.h's RagdollBone, its capsules CapDef.
#ifndef MB_VEC_ELEMS_H
#define MB_VEC_ELEMS_H
#include <vector>
#include <stdlib.h>
#include "engine/core/tstring.h"

#include "engine/core/glyph_hash.h"      // GlyphIt
#include "engine/physics/constraint.h"             // Term, Constraint (the 0x204-byte morph constraint of a skin)
#include "engine/render/face_texture.h"          // FaceTexture (0xb0: a skin's face texture)

#endif // MB_VEC_ELEMS_H
