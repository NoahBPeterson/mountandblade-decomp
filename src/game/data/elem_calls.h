// elem_calls.h: the three vector elements of poly_mesh_edge_face_ops.cpp (RagdollBone 0x50, E68 0x68, FaceTexture 0xb0; engine/physics/bone.h,
// src/game/data/e68.h and face_texture.h define them) as the option panel's loaders call them: their copy constructor and copy
// assignment are out of line in the original (poly_mesh_edge_face_ops.cpp: 0x4cd080, 0x4cd100, 0x4cd170, operator= 0x4cd270 / 0x4cd2f0 / 0x4cd380).
#ifndef OPTIONS_PANEL_ELEM_CALLS_H
#define OPTIONS_PANEL_ELEM_CALLS_H
#include "engine/physics/bone.h"   // RagdollBone (copy constructor 0x4cd080 / assignment 0x4cd270 out of line, poly_mesh_edge_face_ops.cpp)
#include "game/data/e68.h"   // E68 (copy constructor 0x4cd100 / assignment 0x4cd2f0 out of line, poly_mesh_edge_face_ops.cpp)
#include "engine/render/face_texture.h"   // FaceTexture (copy constructor 0x4cd170 / assignment 0x4cd380: the implicit ones, poly_mesh_edge_face_ops.cpp)
#endif
