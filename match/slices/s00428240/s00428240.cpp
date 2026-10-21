// Slice s00428240: the entity factory, entity loading from a stream, and the skeleton (FxDef) instance setup.
#include "world.h"

// @ 0x428240
entity* World::instantiate(int id)
{
    if (id == entity().type_id()) return new entity;
    if (id == mesh_entity().type_id()) return new mesh_entity;
    if (id == static_entity().type_id()) return new static_entity;
    if (id == dynamic_entity().type_id()) return new dynamic_entity;
    if (id == skeleton_entity().type_id()) return new skeleton_entity;
    if (id == strategic_entity().type_id()) return new strategic_entity;
    return 0;
}

// @ 0x428440
entity* World::create_entity(int id)
{
    entity* e = instantiate(id);
    if (!e) mb_error("Unable to create entity of type id %d.", id);
    e->id = next_id++;
    vpush(ents2, e);
    e->owner = this;
    return e;
}

// @ 0x4284e0
void entity::load(stream_base* s)
{
    tstring name;
    s->r_tstr(&name);
    if (!name.equals("none"))
        shape = g_resmgr.get_object60(name)->clone_shape();
    r_matrix(*s, &local);
    int n;
    s->r_int(&n);
    meshes.resize(n, 0);
    for (int i = 0; i < n; i++) {
        tstring nm;
        s->r_tstr(&nm);
        if (!nm.equals("none"))
        {
            mesh*& slot = *(meshes.begin() + i);
            slot = g_resmgr.get_object(&nm)->clone();
        }
    }
}

// @ 0x4285e0
shape_base* res_shape::clone_shape()
{
    shape_base* r;
    if (shape->shape_type() == 0) {
        return new sphere_shape(*(sphere_shape*)shape);
    } else if (shape->shape_type() == 1) {
        return new capsule_shape(*(capsule_shape*)shape);
    } else if (shape->shape_type() == 2) {
        return new face_shape(*(face_shape*)shape);
    } else if (shape->shape_type() == 3) {
        return new composite(*(composite*)shape);
    } else if (shape->shape_type() == 4) {
        return ((manifold*)shape)->build_composite();
    }
    return r;
}

// @ 0x4286d0
void strategic_entity::load(stream_base* s) { entity::load(s); }

// @ 0x4286e0
void skeleton_entity::load(stream_base* s)
{
    entity::load(s);
    tstring a;
    tstring b;
    s->r_tstr(&a);
    if (a.ne("none")) {
        fx = new FxDef(0);
        fx->init(g_resmgr.get_object70(a));
    }
    if (fx) {
        s->r_int(&nfx);
        for (int i = 0; i < nfx; i++) {
            int idx;
            mesh* m;
            s->r_int(&idx);
            s->r_tstr(&a);
            if (a.ne("none"))
                m = g_resmgr.get_object(&a)->clone();
            fx->elems[idx].psys.push_back((Psys*)m);
        }
    }
}

// @ 0x428820
FxDef::FxDef(int a) : psys(), groups(), f60(a)
{
    v0c = vector4(0.0f, 0.0f, 0.0f);
    nenv = 0;
    env = 0;
    count = 0;
    f4c = 0;
    elems = 0;
    f1c = 0;
    b50 = 0;
    b58 = 0;
    f64 = 0;
    b68 = 0;
    f5c = 1;
    f54 = 100.0f;
    v6c = vector4(0.0f, 0.0f, 0.0f);
}

// @ 0x4288c0
void FxDef::init(FxRes* r)
{
    res = r;
    int n = r->count;
    count = n;
    elems = new FxElem[n];
    for (int i = 0; i < count; i++) {
        elems[i].parent = r->recs[i].parent;
        if (elems[i].parent >= 0)
            elems[elems[i].parent].nchild++;
        elems[i].m44 = r->recs[i].m;
        elems[i].src = &r->recs[i];
        if (elems[i].parent < 0)
            elems[i].m4 = elems[i].m44;
        else
            elems[i].m4 = elems[elems[i].parent].m4 * elems[i].m44;
        elems[i].m84 = elems[i].m4;
    }
    for (int i = 0; i < count; i++) {
        if (elems[i].nchild != 0)
            elems[i].children = new int[elems[i].nchild];
        else
            elems[i].children = 0;
        elems[i].nchild = 0;
        quat_from_matrix(&elems[i].q, (const float*)&elems[i].m44);
    }
    for (int i = 0; i < count; i++) {
        int p = elems[i].parent;
        if (p >= 0) {
            elems[p].children[elems[p].nchild] = i;
            elems[p].nchild++;
        }
    }
    for (int i = 0; i < nenv; i++)
        env[i].alloc(count);
}
