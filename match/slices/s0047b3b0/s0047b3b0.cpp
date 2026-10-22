// Slice s0047b3b0: rgl mesh colouring and mesh-list/LOD assembly.
// 0x47b3b0 tints a mesh's vertex colours; 0x47b480 appends (a copy of) a mesh list;
// 0x47b5b0/0x47b610/0x47b690 are the LOD-set operations that drive it.
// Layouts and names are provisional and come from mesh.h (copied from s0044f3c0).
#include "mesh.h"

namespace rgl {

// @ 0x47b3b0
void mesh::multiply_color(const vector4* tint)
{
    ensure_local();
    for (unsigned i = 0; i < v50.size(); i++) {
        vertex60* v = &vat(v50, i);
        vector4 c;
        unpack_color(&c, &v->color);
        c.x = c.x * tint->x;
        c.y = c.y * tint->y;
        c.z = c.z * tint->z;
        v->color &= 0xff000000;
        v->color |= pack_color(&c) & 0xffffff;
    }
    slots[0].f28 = 1;
}

// @ 0x47b480
void mesh_list::append(mesh_list* src, int bone, const mat4* m, const unsigned* color)
{
    for (std::vector<mesh*>::iterator it = src->v.begin(); it < src->v.end(); ++it) {
        if (bone == -1) {
            mesh* c = (*it)->clone();
            vpush(v, c);
        } else {
            mesh* c = new mesh;
            res_t* old = c->res;
            res_t* nw = (*it)->res;
            if (old)
                old->refs--;
            c->res = nw;
            if (nw)
                nw->refs++;
            vpush(v, c);
            c->append_skinned(*it, m, bone, *color, 0);
        }
    }
}

// @ 0x47b5b0
void lod_mesh::set_color(unsigned color)
{
    for (int i = 0; i < count; i++) {
        std::vector<mesh*>& l = lists[i].v;
        for (std::vector<mesh*>::iterator it = l.begin(); it < l.end(); ++it)
            (*it)->set_color(color);
    }
}

// @ 0x47b610
void lod_mesh::transform(const mat4& m)
{
    for (int i = 0; i < count; i++) {
        std::vector<mesh*>& l = lists[i].v;
        for (std::vector<mesh*>::iterator it = l.begin(); it < l.end(); ++it)
            (*it)->transform(m);
    }
}

// @ 0x47b690
void lod_mesh::append(lod_mesh* src, int bone, const mat4* m, const unsigned* color)
{
    if (count < src->count) {
        int old = count;
        count = src->count;
        if (old > 0) {
            unsigned white = 0xffffffff;
            mat4 ident;
            for (int i = old; i < count; i++)
                lists[i].append(&lists[old - 1], -1, identity(&ident), &white);
        }
    }
    if (src->count < count && src->count > 0) {
        for (int i = src->count; i < count; i++)
            lists[i].append(&src->lists[src->count - 1], bone, m, color);
    }
    for (int i = 0; i < src->count; i++)
        lists[i].append(&src->lists[i], bone, m, color);
}

} // namespace rgl
