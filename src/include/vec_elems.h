// vec_elems.h: the element types of the std::vector<T> instantiations shared by src/rgl/mesh2 (mesh_2.cpp, 0x4cc000..0x4cd800)
// and src/game/cls_map (0x4d9840..0x4dc1c0): R44 (0x2c trivial), b16 (0x10 POD block), E50 (0x50 record with a vector<R44> at +0x40,
// the "E80" of the cls_map slices) and E176 (0xb0 record with a vector<int> at +0x1c). ONE definition each (moved from mesh_2.h).
#ifndef MB_VEC_ELEMS_H
#define MB_VEC_ELEMS_H
#include <vector>
#include <stdlib.h>
#include "../game/tstring.h"

struct E516 { int v[0x81]; };        // 0x204-byte record
struct R44 { int v[11]; };           // 0x2c trivial element
struct b16 { int v[4]; };            // 0x10 POD block

// 0x38-byte record: a tstring followed by five raw pointers that a user destructor frees.
struct NamePtrs56 {
    tstring s;
    int mid[5];
    void* q[5];
    ~NamePtrs56() { for (int i = 0; i < 5; ++i) free(q[i]); }
};

struct E50 {                                // 0x50
    tstring s;                              // 0x00
    int x0, x1, x2, x3;                     // 0x10
    b16 y;                                  // 0x20
    b16 z;                                  // 0x30
    std::vector<R44> v;                     // 0x40
    // memberwise, spelled out: the implicit one would call tstring's copy constructor, because the shared
    // tstring::operator= returns void (src/game/tstring.h)
    E50& operator=(const E50& o)
    {
        s = o.s;
        x0 = o.x0; x1 = o.x1; x2 = o.x2; x3 = o.x3;
        y = o.y;
        z = o.z;
        v = o.v;
        return *this;
    }
    void load(struct TiXmlElement* e);      // 0x4d3330 (the ragdoll bone loader, src/game/options_panel/ragdoll_models.cpp)
};

struct E176 {                               // 0xb0
    tstring s;                              // 0x00
    int a;                                  // 0x10
    int b;                                  // 0x14
    int c;                                  // 0x18
    std::vector<int> v;                     // 0x1c
    b16 big[8];                             // 0x2c
    int t;                                  // 0xac
};

#endif // MB_VEC_ELEMS_H
