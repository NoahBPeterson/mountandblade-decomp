// Slice s00419a30: the resource manager's save routine (writes every resource by type) and its destructor,
// plus std::vector<vec_array> / std::list instantiations (see s00419a30_stl.cpp).
#include "s00419a30.h"

// @ 0x419a30
void resmgr::save(stream_base* s)
{
    int n_texture = 0, n_shader = 0, n_material = 0, n_mesh = 0, n_body = 0, n_skeleton = 0, n_anim = 0;
    unsigned j = 0;
    unsigned i;
    for (i = 0; i < all.size(); ) {
        if (all[i]->type() == 2) {
            n_texture++;
            i++;
        } else if (all[i]->type() == 4) {
            n_shader++;
            i++;
        } else if (all[i]->type() == 3) {
            n_material++;
            i++;
        } else if (all[i]->type() == 1) {
            n_mesh++;
            i++;
        } else if (all[i]->type() == 6) {
            n_body++;
            i++;
        } else if (all[i]->type() == 7) {
            n_skeleton++;
            i++;
        } else {
            if (all[i]->type() == 8)
                n_anim++;
            i++;
        }
    }
    if (n_texture) {
        s->w_cstr("texture");
        s->w_int(&n_texture);
        for (; j < all.size(); j++) {
            if (all[j]->type() == 2) {
                texture* t = (texture*)all[j];
                s->w_tstr(&t->name);
                s->w_int(&t->f24);
            }
        }
    }
    if (n_shader) {
        s->w_cstr("shader");
        s->w_int(&n_shader);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 4)
                all[j]->save(s);
        }
    }
    if (n_material) {
        s->w_cstr("material");
        s->w_int(&n_material);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 3)
                all[j]->save(s);
        }
    }
    if (n_mesh) {
        s->w_cstr("mesh");
        s->w_int(&n_mesh);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 1)
                save_mesh(s, (mesh*)all[j]);
        }
    }
    if (n_body) {
        s->w_cstr("body");
        s->w_int(&n_body);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 6)
                save_body(s, (body_res*)all[j]);
        }
    }
    if (n_skeleton) {
        s->w_cstr("skeleton");
        s->w_int(&n_skeleton);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 7)
                save_skeleton(s, (skeleton_res*)all[j]);
        }
    }
    if (n_anim) {
        s->w_cstr("skeleton_anim");
        s->w_int(&n_anim);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 8)
                save_anim(s, (anim_res*)all[j]);
        }
    }
    s->w_cstr("end");
}

// @ 0x419e90
resmgr::~resmgr()
{
    for (std::vector<lod_mesh*>::iterator it = lods.begin(); it < lods.end(); ++it) {
        lod_mesh* l = *it;
        for (int i = 0; i < l->count; i++)
            l->lists[i].forget();
        if (*it)
            delete *it;
    }
    for (std::vector<mesh*>::iterator it = meshes.begin(); it < meshes.end(); ++it)
        (*it)->res = 0;
    for (std::list<res_base*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        if (*it)
            delete[] *it;
    }
}
