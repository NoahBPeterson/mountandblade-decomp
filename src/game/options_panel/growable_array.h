// growable_array.h: the object module_load.cpp builds (ctor 0x4d1360) and the mission constructor (world_app.cpp, 0x4d3880) places at
// Mission +0x6698. Split out of module_load.h so TUs that do not take that header's other types can use the real class.
#ifndef OPTIONS_PANEL_GROWABLE_ARRAY_H
#define OPTIONS_PANEL_GROWABLE_ARRAY_H
#include "../../rgl/rgl_math.h"

// The map editor's mesh graph (rgl/mesh2/graph_obj.h) as its constructor builds it: a stamp, seven VC7.1 vectors (allocator
// word, first, last, end: the constructor zeroes the three pointers and leaves the allocator word alone) with the open list's
// comparator word after the first, then the mode, the current element and two more members (ctor 0x4d1360).
struct growable_array {
    struct range10 { int alloc; int* first; int* last; int* end; };   // a std::vector's words
    int f0;                         // +0 (graph_obj::stamp)
    range10 head;                   // +4 (graph_obj::open; first at +8)
    int f14;                        // +0x14 (graph_obj::open_cmp)
    range10 items[6];               // +0x18 (graph_obj::nodes, faces, edges, vec48, vec58, vec68; first at +0x1c)
    int f78;                        // +0x78 (graph_obj::mode)
    int f7c;                        // +0x7c (-1; graph_obj::cur)
    rgl::vector4 f80;               // +0x80 = (1,1,1) normalised
    int f90;                        // +0x90 = 1
    growable_array();
};

#endif
