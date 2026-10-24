// tail2.h: Tail2, the 0x20-byte tail object whose destructor is 0x4c4d10 (rgl/mesh2/gui_edit.cpp). ONE definition for
// src/rgl/mesh2 (defines the destructor, drains the +0x10 reference window) and src/rgl/xmlcfg (records.h embeds it as a
// member of the world object's records, size 0x20, and calls the out-of-line destructor).
#ifndef MB_TAIL2_H
#define MB_TAIL2_H

struct Tail2 {
    void*        vec;      // +0
    void**       arr;      // +4  array of owned pointers (~Tail2 0x4c4d10)
    unsigned int count;    // +8
    int          refbase;  // +0xc
    int          refcap;   // +0x10
    char         pad14[0x20 - 0x14];   // +0x14 (size 0x20: records.h)
    void release() { if (refcap != 0) { refcap = refcap - 1; if (refcap == 0) refbase = 0; } }
    ~Tail2();              // 0x4c4d10
};

#endif // MB_TAIL2_H
