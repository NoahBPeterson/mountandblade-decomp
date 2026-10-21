// Slice s00413320: mesh morph update, mesh render, lod mesh list render, vector<vertex60> erase/resize.
#include "mesh.h"

namespace rgl {

// @ 0x413320
void mesh::update_morph()
{
    if (c8 != c4) {
        keyframes& k = get_kf();
        float t;
        int b, a;
        k.find(&a, &b, &t, &c8);
        if (a >= 0) {
            ensure_local();
            if (t < 0.001f) {
                for (unsigned i = 0; i < v70.size(); i++)
                    v70[i].b = k.frames[a].verts[i];
                for (unsigned i = 0; i < v50.size(); i++)
                    v50[i].normal = k.frames[a].norms[i];
            } else if (t > 0.999f) {
                for (unsigned i = 0; i < v70.size(); i++)
                    v70[i].b = k.frames[b].verts[i];
                for (unsigned i = 0; i < v50.size(); i++)
                    v50[i].normal = k.frames[b].norms[i];
            } else {
                for (unsigned i = 0; i < v70.size(); i++) {
                    float u = 1.0f - t;
                    v70[i].b = vadd(vscale(u, k.frames[a].verts[i]), vscale(t, k.frames[b].verts[i]));
                }
                for (unsigned i = 0; i < v50.size(); i++) {
                    float u = 1.0f - t;
                    v50[i].normal = vadd(vscale(u, k.frames[a].norms[i]), vscale(t, k.frames[b].norms[i]));
                    v50[i].normal.normalize();
                }
            }
            slots[0].f28 = 1;
        }
        c4 = c8;
    }
}

// @ 0x413750
int mesh::render(scene* sc, const mat4& frame)
{
    if (!(mask & sc->mask)) return 0;
    res_t* r = res;
    if (r) {
        slots[0].f0 = r->mat->get_flags();
        unsigned f20 = r->mat->f20;
        if (f20 & 0x80000000) {
            unsigned n = get60_().size();
            std::vector<vertex60>& w = get50_();
            if (n < w.size())
                FUN_00412700();
        }
        if (res->f5c >= 0 && res->p48->f4c == 0)
            res->fn68(res);
    } else {
        slots[0].f0 = 0;
        mstr name("default");
        res = g_resmgr.lookup(name);
    }
    update_morph();
    if (has_morph()) {
        std::vector<point20>& pts = get70();
        if (pts.size() > 0) {
            mesh* mm = sc->get_mesh(res);
            mm->append(this, &frame);
            goto done;
        }
    }
    {
        std::vector<face20>& fs = get90();
        if (fs.size() != 0) {
            unsigned f = flags;
            if (f & 0x10000000) slots[0].f4 = 0;
            else if (f & 0x20000000) slots[0].f4 = 1;
            slots[2].f4 = slots[0].f4;
            if (f & 0x40000000) slots[1].f4 = 0;
            else if (f & 0x80000000) slots[1].f4 = 1;
            m194 = frame * m_cc;
            g_renderer.submit(sc, this);
        }
    }
done:
    for (std::vector<mesh*>::iterator it = kids.begin(); it != kids.end(); ++it)
        (*it)->render_child(sc);
    if (p_c0) {
        vector4 t;
        t.x = m_cc.f.x * p_c0->in.y + m_cc.u.x * p_c0->in.z + m_cc.s.x * p_c0->in.x + m_cc.o.x;
        t.y = m_cc.f.y * p_c0->in.y + (m_cc.s.y * p_c0->in.x + m_cc.u.y * p_c0->in.z) + m_cc.o.y;
        t.z = m_cc.f.z * p_c0->in.y + (m_cc.s.z * p_c0->in.x + m_cc.u.z * p_c0->in.z) + m_cc.o.z;
        vector4 u;
        u.x = frame.f.x * t.y + frame.u.x * t.z + frame.s.x * t.x + frame.o.x;
        u.y = frame.s.y * t.x + frame.u.y * t.z + frame.f.y * t.y + frame.o.y;
        u.z = frame.s.z * t.x + frame.u.z * t.z + frame.f.z * t.y + frame.o.z;
        p_c0->out = u;
        vpush(sc->list, p_c0);
    }
    return 0;
}

// @ 0x413ac0
void mesh_list::render_all(scene* sc, const mat4& m)
{
    for (std::vector<mesh*>::iterator it = v.begin(); it < v.end(); ++it)
        if (*it) (*it)->render(sc, m);
}

// @ 0x413b00
void lod_mesh::render(scene* sc, const mat4& m)
{
    if ((mask & sc->mask) && cur < count)
        lists[cur].render_all(sc, m * m8c);
}

} // namespace rgl

// vector<vertex60>::erase (0x413b60), explicit instantiation
template std::vector<rgl::vertex60>::iterator std::vector<rgl::vertex60>::erase(std::vector<rgl::vertex60>::iterator, std::vector<rgl::vertex60>::iterator);

// vector<vertex60>::resize (0x413bb0), explicit instantiation
template void std::vector<rgl::vertex60>::resize(size_t, rgl::vertex60);
