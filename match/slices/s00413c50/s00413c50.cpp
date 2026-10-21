// Slice s00413c50: mesh vertex append and the mesh pool lookup.
#include "mesh.h"

namespace rgl {

// @ 0x413c50
int mesh::add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c)
{
    ensure_local();
    int n = (int)v50.size();
    v50.resize(n + 1);
    (&*v50.begin())[n].index = index;
    (&*v50.begin())[n].uv = *uv;
    (&*v50.begin())[n].color = *color;
    (&*v50.begin())[n].f5c = *f5c;
    vertex60* vv = v50._Myfirst + n;
    vv->normal = *pos;
    vv->pos = *pos;
    return n;
}

// @ 0x413d30
mesh* mesh_pool::get_mesh(res_t* r)
{
    mesh* found = 0;
    if (!r) {
        mstr name("default");
        r = g_resmgr.lookup(name);
    }
    for (unsigned i = 0; i < meshes.size(); ) {
        mesh* m = vat(meshes, i);
        if (m->res == r) found = m;
        i++;
        if (found) return found;
    }
    mesh* m = new mesh;
    m->res = r;
    m->slots[1].f4 = 1;
    m->slots[0].f4 = 1;
    m->ensure_local();
    vpush(meshes, m);
    return m;
}

} // namespace rgl
