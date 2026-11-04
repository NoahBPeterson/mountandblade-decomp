// growable_array.h: the object module_load.cpp builds (ctor 0x4d1360) and the mission constructor (world_app.cpp, 0x4d3880) places at
// Mission +0x6698. Split out of module_load.h so TUs that do not take that header's other types can use the real class.
#ifndef OPTIONS_PANEL_GROWABLE_ARRAY_H
#define OPTIONS_PANEL_GROWABLE_ARRAY_H
#include "../../rgl/rgl_math.h"

// Object with one range at +8 and six 0x10-byte ranges at +0x1c (ctor 0x4d1360; the fourth dword of every
// range is left alone by the constructor, so the class is declared with a spare word).
struct growable_array {
    int f0;                         // +0
    int f4;                         // +4
    struct range10 { int* first; int* last; int* end; int spare; };
    range10 head;                   // +8
    int f18;                        // +0x18
    range10 items[6];               // +0x1c
    int f7c;                        // +0x7c (-1)
    rgl::vector4 f80;               // +0x80 = (1,1,1) normalised
    int f90;                        // +0x90 = 1
    growable_array();
};

#endif
