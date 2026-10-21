// Slice s0041b5a0: body-part loader, std::vector/std::fill instantiations, and the resource manager's
// render-target lookup, mesh-name search and registration (provisional names).
#include <algorithm>
#include <new>
#include "s0041b5a0.h"

// @ 0x41b5a0
stream_base* read_body(stream_base* s, body* b)
{
    tstring kind;
    s->r_tstr(&b->name);
    s->r_tstr(&kind);
    if (kind.equals("composite"))
        b->shape = new composite;
    else if (kind.equals("sphere"))
        b->shape = new sphere_shape;
    else if (kind.equals("capsule"))
        b->shape = new capsule;
    else if (kind.equals("face"))
        b->shape = new face;
    else if (kind.equals("manifold"))
        b->shape = new manifold;
    else
        mb_error("unrecognized body part %s \n", kind.c_str());
    b->shape->load(s);
    return s;
}

// @ 0x41b750
template void std::vector<res_obj*>::push_back(res_obj* const&);
// @ 0x41b7a0
template void std::fill(BoneInfluence*, BoneInfluence*, const BoneInfluence&);

// @ 0x41b7d0
render_res* resmgr::get_render_target(int w, int h, int fmt, int ext, int f34)
{
    render_res* r = 0;
    for (std::vector<render_res*>::iterator it = textures.begin(); r == 0 && it != textures.end(); ++it) {
        if ((*it)->external == ext && (*it)->f34 == f34 && (*it)->f14 == 0 && (*it)->width == w && (*it)->height == h) {
            (*it)->f14 = 1;
            r = *it;
        }
    }
    if (r == 0) {
        r = new render_res[1];
        r->name.assign("render_target");
        r->external = ext;
        r->f34 = f34;
        r->width = w;
        r->height = h;
        r->fmt = fmt;
        all.push_back(r);
        textures.push_back(r);
        r->init();
        r->f14 = 1;
    }
    return r;
}

// @ 0x41b960
std::vector<mesh_res*> resmgr::find_meshes(const tstring* prefix)
{
    std::vector<mesh_res*> out;
    tstring tmp;
    int to = prefix->len;
    int from;
    for (std::vector<mesh_res*>::iterator it = meshes.begin(); it != meshes.end(); ++it) {
        from = 0;
        if (!(*it)->name.range_equals(&from, &to, prefix))
            continue;
        if (to != (*it)->name.len) {
            const char* p = (*it)->name.buf + to;
            if (*p == '.') {
                if (to + 4 < (*it)->name.len) {
                    if (p[1] == 'l' || p[1] == 'L') {
                        if (p[2] == 'o' || p[2] == 'O') {
                            if (p[3] == 'd' || p[3] == 'D')
                                continue;
                        }
                    }
                }
            } else {
                from = (*it)->name.len;
                continue;
            }
        }
        out.push_back(*it);
    }
    return out;
}

// @ 0x41ba90
void resmgr::add(res_obj* r)
{
    int t = r->type();
    if (t == 1) {
        for (std::vector<mesh_res*>::iterator it = meshes.begin(); it != meshes.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister mesh %s", (*it)->name.c_str());
        meshes.push_back((mesh_res*)r);
    } else if (r->type() == 2) {
        for (std::vector<render_res*>::iterator it = textures.begin(); it != textures.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister texture %s", (*it)->name.c_str());
        ((render_res*)r)->id = c4;
        if (++c4 > 1000)
            c4 = 1;
        textures.push_back((render_res*)r);
    } else if (r->type() == 4) {
        for (std::vector<material*>::iterator it = materials.begin(); it != materials.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister shader %s", (*it)->name.c_str());
        ((int*)r)[6] = cc;
        if (++cc > 1000)
            cc = 1;
        materials.push_back((material*)r);
    } else if (r->type() == 3) {
        for (std::vector<skin*>::iterator it = skins.begin(); it != skins.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister material %s", (*it)->name.c_str());
        ((int*)r)[6] = c8;
        if (++c8 > 1000)
            c8 = 1;
        skins.push_back((skin*)r);
    } else if (r->type() == 6) {
        for (std::vector<body*>::iterator it = bodies.begin(); it != bodies.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister body %s", (*it)->name.c_str());
        bodies.push_back((body*)r);
    } else if (r->type() == 7) {
        for (std::vector<frame_set*>::iterator it = skels.begin(); it != skels.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister skeleton model %s", (*it)->name.c_str());
        skels.push_back((frame_set*)r);
    } else if (r->type() == 8) {
        for (std::vector<skel_anim*>::iterator it = anims.begin(); it != anims.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister skeleton anim %s", (*it)->name.c_str());
        anims.push_back((skel_anim*)r);
    } else {
        mb_error("Unrecognized resource type.");
    }
    loaded.push_back(r);
}

// the vector deleting destructor of skel_anim (0x41bed0); the dtor body is 0x41b520 in the previous slice
skel_anim::skel_anim() {}
int skel_anim::type() { return 8; }
skel_anim* make_skel_anim_array(int n) { return new skel_anim[n]; }
