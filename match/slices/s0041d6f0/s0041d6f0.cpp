// Slice s0041d6f0: the binary mesh reader (provisional names).
#include <vector>
#include <memory>
#include "s0041d6f0.h"

using namespace rgl;

// 0x41c110: reads one bone-influence record; 0x41c340: reads the morph key frames (cdecl)
stream_base* read_elem14(stream_base* s, elem14* e);              // 0x41c110
stream_base* read_keyframes(stream_base* s, keyframes* k);        // 0x41c340
extern int g_mesh_npts;                                           // 0x5e2998

// @ 0x41d6f0
stream_base* read_mesh(stream_base* s, mesh* m)
{
    tstring unused;
    int npts;
    int nbi;
    int nverts;
    int nfaces;

    s->r_tstr(&m->name);
    s->r_int((int*)&m->flags);
    s->r_tstr(&m->name2);
    m->res = g_resmgr.lookup(m->name2);
    if (m->res)
        m->res->refs++;

    s->r_int(&npts);
    point20 pv;
    m->v70.resize(npts, pv);
    for (int i = 0; i < npts; i++) {
        point20* p = &m->v70[i];
        s->r_float(&p->a.x).r_float(&p->a.y).r_float(&p->a.z);
        m->v70[i].b = m->v70[i].a;
    }

    s->r_int(&nbi);
    m->v80.resize(nbi);
    for (int i = 0; i < nbi; i++)
        read_elem14(s, &vat(m->v80, i));
    g_mesh_npts = npts;
    read_keyframes(s, &m->kf);

    s->r_int(&nverts);
    vertex60 vv;
    m->v50.resize(nverts, vv);
    for (int i = 0; i < nverts; i++) {
        s->r_int(&m->v50[i].index);
        s->r_uint(&m->v50[i].color);
        m->v50[i].f5c = 0;
        vertex60* v = &m->v50[i];
        s->r_float(&v->nrm.x).r_float(&v->nrm.y).r_float(&v->nrm.z);
        m->v50[i].nrm0 = m->v50[i].nrm;
        m->v50[i].f58 = m->v50[i].color;
        for (int j = 0; j < 2; j++) {
            float* f = &m->v50[i].uv[j * 2];
            s->r_float(f).r_float(f + 1);
        }
    }

    s->r_int(&nfaces);
    face20 fv;
    m->v90.resize(nfaces, fv);
    for (int i = 0; i < nfaces; i++) {
        int* q = &vat(m->v90, i).a;
        for (int j = 0; j < 3; j++)
            s->r_int(q++);
    }

    m->compute_tangents();

    m->f228 = 0;
    for (int i = 0; i < nbi; i++) {
        if (vat(m->v80, i).id + 1 > m->f228)
            m->f228 = vat(m->v80, i).id + 1;
    }
    if (m->f228 > 0x20)
        mb_error("Too many bones in mesh %s", m->name.c_str());
    if (nbi != 0 || (m->res && m->res->mat && (m->res->mat->f20 & 0x80000000) != 0))
        m->build_skin();
    m->finish_load();
    m->slots[1].f4 = 0;
    m->slots[0].f4 = 0;
    return s;
}
