// Slice s0041bf40: resource manager lookups/loading, skeleton-animation and morph-set readers, the LOD mesh
// list constructor, std::vector<BoneInfluence> helpers and the mesh clone (provisional names).
#include <algorithm>
#include <new>
#include "s0041bf40.h"
#include "mesh.h"

extern int g_shader_flag;                      // 0x5e28fc

// @ 0x41bf40
small_res* resmgr::get_small(tstring name)
{
    small_res* r = find_small(&name);
    if (r == 0) {
        r = new small_res[1];
        r->name = name;
        all.push_back(r);
        v90.push_back(r);
    }
    return r;
}

// @ 0x41c000
void resmgr::load_shaders()
{
    if (g_shader_flag != 0) {
        for (std::vector<material*>::iterator it = materials.begin(); it != materials.end(); ++it) {
            if (((*it)->flags20 & 0x20000000) && (*it)->vs == 0)
                (*it)->vs = get_small((*it)->ps_file);
        }
    }
}

// @ 0x41c060
stream_base* read_skel_anim(stream_base* s, skel_anim* a)
{
    s->r_tstr(&a->name);
    int n;
    s->r_int(&n);
    a->keys.resize(n);
    for (unsigned i = 0; i < a->keys.size(); i++)
        read_continuous_quats(s, &a->keys[i]);
    read_pair(s, &a->tail);
    return s;
}

// @ 0x41c110
stream_base* read_bone_influence(stream_base* s, BoneInfluence* b)
{
    int n;
    s->r_int(&b->bone);
    s->r_int(&n);
    b->verts.resize(n);
    for (int i = 0; i < n; i++) {
        IdxWeight* e = &b->verts[i];
        s->r_int(&e->index);
        s->r_float(&e->weight);
    }
    return s;
}

// 0x20-byte pair of vector4 arrays (s00411150: ctor 0x411780)
struct VecPair {
    std::vector<rgl::vector4> a, b;
    VecPair();                                 // 0x411780
};
// raw 3-field owner (s00412150)
struct MorphSet {
    VecPair* sets; int* idx; int n;
};

// @ 0x41c180
stream_base* read_vec_pair(stream_base* s, VecPair* p)
{
    int n1, n2;
    s->r_int(&n1);
    p->a.resize(n1);
    for (int i = 0; i < n1; i++) {
        rgl::vector4* v = &p->a[i];
        s->r_float(&v->x).r_float(&v->y).r_float(&v->z);
    }
    s->r_int(&n2);
    p->b.resize(n2);
    for (int j = 0; j < n2; j++) {
        rgl::vector4* v = &p->b[j];
        s->r_float(&v->x).r_float(&v->y).r_float(&v->z);
    }
    return s;
}

// @ 0x41c2a0
// list of five mesh vectors with a name (the lod mesh of s00413e50 has this layout up to +0xcc)
struct mesh_res;
struct vec3x { char b[0x34]; vec3x(); };       // 0x40eaf0
mat4* identity_pos(mat4* out);                 // 0x40cd00 (cdecl, returns out)
struct lod_mesh2 {
    vec3x base;                                // +0
    std::vector<mesh_res*> lists[5];           // +0x34
    int count;                                 // +0x84
    int cur;                                   // +0x88
    mat4 m8c;                                  // +0x8c
    unsigned mask;                             // +0xcc
    tstring name;                              // +0xd0
    lod_mesh2();
};
lod_mesh2::lod_mesh2()
{
    count = 0;
    mat4 t;
    m8c = *identity_pos(&t);
    cur = 0;
    mask = 0xffff;
}

// @ 0x41c340
stream_base* read_morph_set(stream_base* s, MorphSet* m)
{
    s->r_int(&m->n);
    if (m->n != 0) {
        m->idx = new int[m->n];
        m->sets = new VecPair[m->n];
        for (int i = 0; i < m->n; i++) {
            s->r_int(&m->idx[i]);
            read_vec_pair(s, &m->sets[i]);
        }
    }
    return s;
}

// @ 0x41c400
render_res* resmgr::get_texture(int w, int h, int fmt) { return get_render_target(w, h, fmt, 2, 0); }

skin::skin()
{
    f5c = -1; f60 = -1; f64 = 0; id = 0;
    for (int i = 0; i < 5; i++) refs[i] = 0;
    color = rgl::vector4(0.0f, 0.0f, 0.0f);
    f30 = 0; f44 = 0.0f; f68 = 0;
}

// @ 0x41c420
skin* resmgr::get_shadow_skin(int w, int h, int fmt, int f34)
{
    skin* r = 0;
    for (std::vector<skin*>::iterator it = skins.begin(); r == 0 && it != skins.end(); ++it) {
        render_res* t = (*it)->refs[0];
        if (t != 0 && t->external == 1 && t->f34 == f34 && (*it)->f14 == 0 && t->width == w && t->height == h && t->fmt == fmt) {
            (*it)->f14 = 1;
            r = *it;
        }
    }
    if (r == 0) {
        r = new skin[1];
        r->name.assign("");
        r->file.assign("shadowmap_shader");
        r->refs[0] = g_resmgr.get_render_target(w, h, fmt, 1, f34);
        all.push_back(r);
        add(r);
        r->f14 = 1;
    }
    return r;
}

// @ 0x41c540
int resmgr::load_all(int check_exists, tstring* root)
{
    for (std::vector<render_res*>::iterator it = textures.begin(); it != textures.end(); ++it)
        (*it)->load(check_exists, root);
    for (std::vector<material*>::iterator it2 = materials.begin(); it2 != materials.end(); ++it2)
        (*it2)->create_shaders();
    load_shaders();
    for (std::vector<skin*>::iterator it3 = skins.begin(); it3 != skins.end(); ++it3) {
        skin* sk = *it3;
        sk->f30 = g_resmgr.lookup_file(&sk->file);
    }
    return 0;
}

// ---- std::vector<BoneInfluence> helpers (particle skin bones, see s00411150) -----------------
// @ 0x41c5c0
// (catch(...) funclet at 0x41c628: destroys the constructed prefix and rethrows)
template void std::_Uninit_fill_n(BoneInfluence*, size_t, const BoneInfluence&, std::allocator<BoneInfluence>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x41c650
template std::vector<BoneInfluence>::iterator std::vector<BoneInfluence>::erase(std::vector<BoneInfluence>::iterator, std::vector<BoneInfluence>::iterator);
// @ 0x41c6a0
template BoneInfluence* std::vector<BoneInfluence>::_Ufill(BoneInfluence*, size_t, const BoneInfluence&);

// @ 0x41c6d0
rgl::mesh* rgl::mesh::clone()
{
    mesh* self = this;
    rgl::mesh* m = new rgl::mesh;
    if (self->parent)
        m->parent = self->parent;
    else
        m->parent = self;
    m->res = self->res;
    if (m->res)
        ++*(int*)((char*)m->res + 0x64);
    m->flags = self->flags;
    m->color = self->color;
    m->f40 = self->f40;
    m->local_a0 = 0;
    m->local_a1 = 0;
    m->name = self->name;
    m->f228 = self->f228;
    m->v22c = self->v22c;
    return m;
}
