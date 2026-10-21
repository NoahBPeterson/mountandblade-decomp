// Slice s00419420: mesh-list owners, the binary mesh serialiser, resource-manager lookups and the
// std::vector/_Uninit_* instantiations over 8- and 12-byte records.
#include "s00419420.h"

// @ 0x419420
mesh_list::~mesh_list()
{
    for (std::vector<mesh*>::iterator it = begin(); it < end(); ++it)
        delete *it;
}

// @ 0x419470
void mesh_list::forget()
{
    for (unsigned i = 0; i < size(); i++)
        (*this)[i] = 0;
    tidy();
}

// @ 0x4194c0
stream_base* save_keyframes(stream_base* s, keyframes* k)
{
    s->w_int(&k->count);
    for (int i = 0; i < k->count; i++) {
        s->w_int(&k->times[i]);
        save_frame(s, &k->frames[i]);
    }
    return s;
}

// @ 0x419760
stream_base* save_mesh(stream_base* s, mesh* m)
{
    s->w_tstr(&m->name);
    s->w_int((int*)&m->flags);
    if (m->name2.len > 1) s->w_tstr(&m->name2); else s->w_tstr(&m->res->name);
    int i;
    int n = m->v70.size();
    s->w_int(&n);
    for (i = 0; i < n; i++) {
        point20* p = &m->v70[i];
        s->w_float(&p->a.x).w_float(&p->a.y).w_float(&p->a.z);
    }
    int n2 = m->v80.size();
    s->w_int(&n2);
    for (i = 0; i < n2; i++)
        save_elem14(s, &m->v80[i]);
    save_keyframes(s, &m->kf);
    int n3 = m->v50.size();
    s->w_int(&n3);
    for (i = 0; i < n3; i++) {
        s->w_int(&m->v50[i].index);
        s->w_uint(&m->v50[i].color);
        vertex60* v = &m->v50[i];
        s->w_float(&v->pos.x).w_float(&v->pos.y).w_float(&v->pos.z);
        for (int k = 0; k < 2; k++) {
            float* q = &m->v50[i].uv[k].u;
            s->w_float(q).w_float(q + 1);
        }
    }
    int n4 = m->v90.size();
    s->w_int(&n4);
    for (i = 0; i < n4; i++) {
        int* q = &m->v90[i].a;
        for (int k = 0; k < 3; k++)
            s->w_int(q++);
    }
    return s;
}

// ---- resource manager ----------------------------------------------------------------------
int g_resource_flag;                                     // 0x5e28fc

// @ 0x419540
mesh* resmgr::get_object(const tstring* name)
{
    mesh* r = find(name);
    if (!r)
        mb_error("get_object failed for: %s", name->c_str());
    return r;
}

// @ 0x419580
int resmgr::m419580(int arg)
{
    for (std::vector<res34*>::iterator it = v30.begin(); it != v30.end(); ++it)
        (*it)->m418520();
    if (g_resource_flag) {
        for (std::vector<res94*>::iterator it = v90.begin(); it != v90.end(); ++it)
            (*it)->m417050(arg);
    }
    return 0;
}

// @ 0x4195e0
int resmgr::m4195e0()
{
    for (std::vector<res34*>::iterator it = v30.begin(); it != v30.end(); ++it)
        (*it)->m4167c0();
    for (std::vector<res94*>::iterator it = v90.begin(); it != v90.end(); ++it) {
        res94* r = *it;
        r->f1c = 0;
        r->f20 = 0;
        r->f24 = 0;
        r->f28 = 0;
        r->b18 = 0;
    }
    m418e40();
    return 0;
}

// @ 0x419640
void resmgr::m419640()
{
    if (g_resource_flag && !c_ce) {
        for (std::vector<mesh*>::iterator it = meshes.begin(); it != meshes.end(); ++it)
            (*it)->m419360();
        c_ce = 1;
    }
}
