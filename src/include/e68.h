// e68.h: E68, the 0x68-byte element of the vector at src/rgl/mesh2/mesh_2.cpp (vector<E68>, destructor calls the out-of-line
// free_materials 0x4c8330) and of the table walked by src/game/ui/mission_ui.cpp (Vec68::count 0x48ace0: (end - begin) / 0x68).
// ONE definition for src/rgl/mesh2 and src/game/ui. M28 is the 0x1c member at +0x38 (copy constructor 0x4bd7a0), scalar4 the
// 4-byte element of the vector at +0x54.
#ifndef MB_E68_H
#define MB_E68_H
#include <vector>
#include "../game/tstring.h"

struct scalar4 { int v; };           // 4-byte scalar element

// empty base of E68 with the out-of-line 0x4c8330 (Block::free_materials in gui_edit.h is the same address)
struct block_base { void free_materials(); };    // 0x4c8330

struct M28 {                                // 0x1c member of E68
    char* p;                                // 0x00
    int n;                                  // 0x04
    char c;                                 // 0x08
    tstring s;                              // 0x0c
    M28() {}                                // needed by vector<E68>(size_type)
    M28(const M28&);                        // 0x4bd7a0
    ~M28() { if (p) delete[] p; }
    M28& operator=(const M28& o)
        { p = o.p; n = o.n; c = o.c; s = o.s; return *this; }
};

struct E68 : block_base {                   // 0x68
    tstring s;                              // 0x00
    int b0, b1, b2, b3, b4;                 // 0x10
    int b5, b6, b7, b8, b9;                 // 0x24
    M28 m;                                  // 0x38
    std::vector<scalar4> v;                 // 0x54
    int t;                                  // 0x64
    ~E68() { free_materials(); }
};

#endif // MB_E68_H
