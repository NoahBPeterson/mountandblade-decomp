// frustum.h: the view frustum embedded in rgl::scene at +0x30 (0x120 bytes). Constructor 0x41ef60 (world_items.cpp);
// the plane builder 0x41e4f0 and the sphere test 0x41e470 are in the previous object.
// Layout: ctor 0x41ef60 (flags, plane count, frame at +0xc8, six clip floats), Ctx::render_scene 0x429890
// (planes[i].in_front, nplanes, build_planes, the frame at +0xc8 mirrored for the reflection pass).
#ifndef SCENE_FRUSTUM_H
#define SCENE_FRUSTUM_H
#include "../rgl_math.h"

namespace rgl {

struct plane {                  // 0x20 bytes
    vector4 pos, normal;
    int in_front(const vector4* p, float eps) const;            // 0x41e470 (sphere centre p, radius eps)
};

struct frustum {
    char perspective;           // +0
    char flag1;                 // +1
    char pad2[2];
    int nplanes;                // +4
    plane planes[6];            // +8
    matrix m;                   // +0xc8 camera frame
    float left, right, top, bottom, nearz, farz;    // +0x108
    frustum();                  // 0x41ef60
    void build_planes();        // 0x41e4f0
};

} // namespace rgl
#endif
