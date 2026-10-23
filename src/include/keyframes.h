// keyframes.h: a mesh's morph key frames, 12 bytes at mesh+0xa4: frames[count] (rgl::frame_t, two vector4 arrays) and
// times[count] (INTEGER key times: keyframes::find 0x40f0d0 compares (float)times[mid] with the query and
// interpolates between times[lo] and times[hi]; operator= 0x4125e0 copies them with memcpy(count * 4)).
// Assignment 0x4125e0 returns nothing; the destructor is inline (~mesh 0x4127f0 expands it; the ??_E call is 0x40fe10).
// Users: src/rgl/particles/mesh_obj.cpp (find, operator=), src/rgl/core/resmgr.cpp (save_keyframes 0x4194c0,
// read_keyframes 0x41c340), the mesh headers of src/rgl and src/game/strat.
//
// NEEDS `frame_t` declared by the includer, in the enclosing scope of namespace rgl (rgl::frame_t in the rgl mesh
// headers, ::frame_t in src/game/strat/strat_ext.h): the three mesh headers each define their own frame_t.
// Namespace rgl.
#ifndef MB_KEYFRAMES_H
#define MB_KEYFRAMES_H

namespace rgl {

struct keyframes {
    frame_t* frames; int* times; int count;
    keyframes() { count = 0; frames = 0; times = 0; }
    ~keyframes()                                    // inline: ~mesh 0x4127f0 (the ??_E call 0x40fe10)
    {
        if (frames)
            delete[] frames;
        if (times)
            delete[] times;
    }
    void operator=(const keyframes& o);                                         // 0x4125e0 (returns nothing)
    void find(int* lo, int* hi, float* frac, const float* t);                   // 0x40f0d0
};

} // namespace rgl

#endif // MB_KEYFRAMES_H
