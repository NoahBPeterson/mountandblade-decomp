// tmesh.h: the pointer vector the interior builder fills (the lod mesh that rgl::lod_mesh::clone 0x47a370 copies is
// rgl/mesh.h's).
#ifndef TERRAIN_B_TMESH_H
#define TERRAIN_B_TMESH_H
#include <vector>
#include "../../rgl/mesh.h"
#include "../tstring.h"

// std::vector<mesh*> (VC7.1 layout); the out-of-line insert 0x410610 is shared by every pointer vector
struct MeshVec : std::vector<rgl::mesh*> {
    MeshVec& operator=(const std::vector<rgl::mesh*>& o);        // 0x43f0a0
    void insert_n(iterator where, size_t n, rgl::mesh* const& v);    // 0x410610 (mission.cpp forwards it to the shared vector<int>::_Insert_n)
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

#endif // TERRAIN_B_TMESH_H
