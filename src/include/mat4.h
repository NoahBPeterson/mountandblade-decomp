// mat4.h: the engine's frame types. The classes are declared in src/rgl/rgl_math.h (rgl::matrix, rgl::mat3, rgl::rot3, rgl::smat):
//   rot3 / mat3 : 3x3 rotation, rows s(ide), f(orward), u(p) padded to 4 floats (48 bytes); mat3 has the by-value constructor
//   matrix      : rows s, f, u and origin o (64 bytes) with the operations the modules call (operator* 0x40e870,
//                 transform_point 0x408460, rot3 0x40a920, rot_t 0x429670, relative 0x4329d0, rotate_about_*)
//   smat        : a frame whose rows can be scaled in place (entity::local, 0x41fee0)
// The Module System calls the frame `pos`; the scene code called it mat4: both are rgl::matrix. rgl::identity3 (0x40cd60) and
// rgl::identity_pos (0x40cd00) are declared with the class.
// Needs rgl::vector4 / rgl::matrix (rgl_base.h picks the module's own).
#ifndef MB_MAT4_H
#define MB_MAT4_H
#include "./rgl_base.h"

typedef rgl::matrix pos;                                           // the Module System name

#endif // MB_MAT4_H
