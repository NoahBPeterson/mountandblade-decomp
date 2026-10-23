// tmesh.h: the interior lod-mesh record (0xe0 bytes: orientation block, five mesh lists, the number of used lists, a frame,
// a mask and the name) cloned by tmesh::clone (0x47a370), and the pointer vector the interior builder fills.
#ifndef TERRAIN_B_TMESH_H
#define TERRAIN_B_TMESH_H
#include <vector>
#include "../../rgl/mesh.h"
#include "../tstring.h"

// std::vector<mesh*> (VC7.1 layout); the out-of-line insert 0x410610 is shared by every pointer vector
struct MeshVec : std::vector<rgl::mesh*> {
    MeshVec& operator=(const std::vector<rgl::mesh*>& o);        // 0x43f0a0
    void insert_n(iterator where, size_t n, rgl::mesh* const& v);    // 0x410610
    void resize2(size_t n, rgl::mesh* v);                        // 0x41f970
    void push_back(rgl::mesh* const& x)
    {
        if (_Myfirst != 0 && size() < capacity()) {
            *_Mylast = x;
            _Mylast++;
        } else {
            insert_n(_Mylast, 1, x);
        }
    }
};

// the 0x34-byte orientation block at the head of an interior record (ctor 0x40eaf0)
struct rot_state {
    rgl::vector4 a, b, c;
    float f30;
    rot_state();                                // 0x40eaf0
};

struct tmesh {
    rot_state base;                             // +0x00
    MeshVec lists[5];                           // +0x34
    int count;                                  // +0x84
    int cur;                                    // +0x88
    rgl::matrix m8c;                            // +0x8c
    unsigned mask;                              // +0xcc
    tstring name;                               // +0xd0
    tmesh();                                    // 0x41c2a0
    tmesh* clone();                             // 0x47a370
};

#endif // TERRAIN_B_TMESH_H
