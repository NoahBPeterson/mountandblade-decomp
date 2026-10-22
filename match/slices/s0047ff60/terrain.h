// Layout of the world-map terrain object (provisional name `terrain_obj`), s0047ff60 view.
// Fields match slices s00466aa0 / s00464a60 (verified there). The object is huge; here only what this slice
// needs is named, and everything else is reached with the raw offsets written in the comments.
#ifndef TERRAIN_H
#define TERRAIN_H
#include "rgl.h"

struct terrain_obj;

void triangle_corners(terrain_obj* t, int x, int y, int flip, int* a, int* b, int* c);   // 0x466e20
void sun_light(terrain_obj* t, rgl::vector4* out, const rgl::vector4* dir,
               const rgl::vector4* scale);                                              // 0x466ed0
#endif
