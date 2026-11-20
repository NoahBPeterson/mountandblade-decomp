// elem_calls.h: the three vector elements of mesh_2.cpp (RagdollBone 0x50, E68 0x68, FaceTexture 0xb0; rgl/core/bone.h,
// src/include/e68.h and face_texture.h define them) as the option panel's loaders call them: their copy constructor and copy
// assignment are out of line in the original (mesh_2.cpp: 0x4cd080, 0x4cd100, 0x4cd170, operator= 0x4cd270 / 0x4cd2f0 / 0x4cd380).
#ifndef OPTIONS_PANEL_ELEM_CALLS_H
#define OPTIONS_PANEL_ELEM_CALLS_H
#include "../../rgl/core/bone.h"   // RagdollBone (copy constructor 0x4cd080 / assignment 0x4cd270 out of line, mesh_2.cpp)
#include "../../include/e68.h"   // E68 (copy constructor 0x4cd100 / assignment 0x4cd2f0 out of line, mesh_2.cpp)
#include "../../include/face_texture.h"   // FaceTexture (copy constructor 0x4cd170 / assignment 0x4cd380: the implicit ones, mesh_2.cpp)
#endif
