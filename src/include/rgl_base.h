// rgl_base.h: makes rgl::vector4 / rgl::matrix visible to the shared math headers of src/include, whichever
// of the two (identical-layout) declarations the including module uses: match/include/rgl.h (src/game) or
// src/rgl/rgl_math.h (src/rgl and the game modules built on it). The two cannot be included together; a module
// includes its own first and then the shared headers, which only add this fallback when neither is present.
#ifndef MB_RGL_BASE_H
#define MB_RGL_BASE_H
#if !defined(MB_RGL_H) && !defined(RGL_MATH_H)
#include "../rgl/rgl_math.h"
#endif
#endif // MB_RGL_BASE_H
