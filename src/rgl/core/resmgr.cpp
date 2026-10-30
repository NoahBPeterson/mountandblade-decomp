// resmgr.cpp (0x419190..0x41e46f): the resource manager's second half (registration, serialisation, loading,
// destruction), the mesh and skeleton serialisers, the mesh object's late members (bounds, vertex colours,
// tangent frames, clone), the LOD groups, and the std::vector / std::list instantiations over the resource types.
// Migrated from slices s00418a70 (0x419190..0x419420: its last 5 rows), s00419420, s00419a30, s0041a2c0,
// s0041ac10, s0041b5a0, s0041bf40, s0041c760, s0041ce80, s0041d6f0 and s0041db20.
// Boundaries: content only. The range follows 0x417bb0..0x419190 (resource_manager.cpp, whose class
// `resource_manager` is the other half of this object) and ends where core/frustum_grid.cpp (0x41e470) begins;
// the forward calls 0x41a2c0 -> 0x41ef30, 0x41ca60 -> 0x41f970 only put a boundary below world_items.cpp.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <new>
#include <vector>
#include <list>
#include <memory>
#include <algorithm>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../stream.h"
#include "../resource.h"
#include "../resource_manager.h"
#include "../particles/mesh_obj.h"
#include "../mem_stream.h"
#include "../text_stream.h"
#include "./resource_io.h"
#include "./resmgr.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh_x.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using namespace rgl;

extern int g_use_text_res;                     // 0x5a9fb0 (the text resources .trf are read when set)
extern int g_mesh_npts;                        // 0x5e2998 (point count of the mesh being read)
extern __int64 g_bit[];                        // 0x5a8e68 g_bit[i] == 1 << i

// ---- serialisers -----------------------------------------------------------------------------------------------
// @ 0x419190
stream_base* save_elem14(stream_base* s, bone_influence* e)
{
    s->w_int(&e->bone);
    int n = (int)e->verts.size();
    s->w_int(&n);
    for (int i = 0; i < n; i++) {
        idx_weight* p = &(&*e->verts.begin())[i];
        s->w_int(&p->index);
        s->w_float(&p->weight);
    }
    return s;
}

// @ 0x419200
stream_base* save_frame(stream_base* s, frame_t* f)
{
    int n = (int)f->verts.size();
    s->w_int(&n);
    for (int i = 0; i < n; i++) {
        vector4* v = &(&*f->verts.begin())[i];
        s->w_float(&v->x).w_float(&v->y).w_float(&v->z);
    }
    int n2 = (int)f->norms.size();
    s->w_int(&n2);
    for (int i = 0; i < n2; i++) {
        vector4* v = &(&*f->norms.begin())[i];
        s->w_float(&v->x).w_float(&v->y).w_float(&v->z);
    }
    return s;
}

// ---- the mesh object's late members ------------------------------------------------------------------------
// @ 0x419300
void mesh_x::compute_bounds()
{
    bounds.reset();
    std::vector<point20>* v;
    if (local_a1)
        v = &v70;
    else
        v = &parent->get70();
    for (unsigned i = 0; i < v->size(); i++)
        bounds.extend(&(&*v->begin())[i].a);
    bounds.update();
}

// Converts the vertex colours from gamma space to linear (pow 2.2) and copies the packed result to +0x58.
// @ 0x419360
void mesh_x::linearise_vertex_colors()
{
    if (local_a1) {
        int n = (int)v50.size();
        for (int i = 0; i < n; i++) {
            vertex60* v = &(&*v50.begin())[i];
            vector4 c;
            unpack_color(&c, &v->color);
            c.x = (float)pow(c.x, 2.2f);
            c.y = (float)pow(c.y, 2.2f);
            c.z = (float)pow(c.z, 2.2f);
            v->color = pack_color(&c);
            (&*v50.begin())[i].f58 = (&*v50.begin())[i].color;
        }
    }
}

// ---- owned mesh lists ----------------------------------------------------------------------------------------------
// @ 0x419420
mesh_owner::~mesh_owner()
{
    for (std::vector<mesh*>::iterator it = begin(); it < end(); ++it)
        delete *it;
}

// @ 0x419470
void mesh_owner::forget()
{
    for (unsigned i = 0; i < size(); i++)
        (*this)[i] = 0;
    _Tidy();
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

// ---- resource manager: lookups, device maintenance ---------------------------------------------------------------------
// @ 0x419540
mesh* resmgr::get_object(const rgl::string* name)
{
    mesh* r = (mesh*)base().find_v20(*name);
    if (!r)
        mb_error("get_object failed for: %s", name->c_str());
    return r;
}

// @ 0x419580
int resmgr::restore(ID3DXEffect* fx)
{
    for (std::vector<texture*>::iterator it = textures.begin(); it != textures.end(); ++it)
        (*it)->create();
    if (g_use_shaders) {
        for (std::vector<effect_set*>::iterator it = effects.begin(); it != effects.end(); ++it)
            (*it)->resolve(fx);
    }
    return 0;
}

// @ 0x4195e0
int resmgr::invalidate()
{
    for (std::vector<texture*>::iterator it = textures.begin(); it != textures.end(); ++it)
        (*it)->unload();
    for (std::vector<effect_set*>::iterator it = effects.begin(); it != effects.end(); ++it) {
        effect_set* r = *it;
        r->h[0] = 0;
        r->h[1] = 0;
        r->h[2] = 0;
        r->h[3] = 0;
        r->resolved = 0;
    }
    base().release_pixel_shaders();
    return 0;
}

// @ 0x419640
void resmgr::linearise_meshes()
{
    if (g_use_shaders && !meshes_ready) {
        for (std::vector<mesh*>::iterator it = meshes.begin(); it != meshes.end(); ++it)
            ((mesh_x*)*it)->linearise_vertex_colors();
        meshes_ready = 1;
    }
}

// ---- STL: 8-byte records and pointer vectors ---------------------------------------------------------------------------------
// 0x419510 vector<idx_weight>::_Ufill, 0x4199f0 erase, 0x41a9b0 _Insert_n (+ funclet 0x41ab16), 0x41ae70 resize
// The fill helper under vector<idx_weight>::_Ufill (0x419510) is the copy in resource_manager.cpp (0x418150, the
// first object that instantiated it); declared, not instantiated, so that its address is the one the audit sees.
namespace std {
void _Uninit_fill_n(idx_weight*, unsigned int, const idx_weight&, allocator<idx_weight>&, _Nonscalar_ptr_iterator_tag);   // 0x418150
}
template class std::vector<idx_weight>;
// 0x419680 vector<T*>::_Ucopy (called by the copy constructor 0x41ac10), 0x41a2c0 _Insert_n (shared with vector<int>)
template class std::vector<int>;
template class std::vector<named_object*>;

// 12-byte key arrays: 0x4196a0 allocator::destroy, 0x4196d0 _Uninit_copy (+ funclet 0x41973a), 0x41a0f0 _Destroy_range,
// 0x41a130 _Uninit_fill_n (+ funclet 0x41a198), 0x41a1c0 _Ucopy, 0x41a270 _Destroy, 0x41a290 _Ufill, 0x41a660 erase,
// 0x41a6b0 _Insert_n (+ funclets 0x41a861, 0x41a916), 0x41adb0 resize
namespace std {
template vec_array* _Uninit_copy<vec_array*, vec_array*, allocator<vec_array> >(vec_array*, vec_array*, vec_array*, allocator<vec_array>&, _Nonscalar_ptr_iterator_tag);
template void _Uninit_fill_n<vec_array*, unsigned int, vec_array, allocator<vec_array> >(vec_array*, unsigned int, const vec_array&, allocator<vec_array>&, _Nonscalar_ptr_iterator_tag);
template void _Destroy_range<vec_array, allocator<vec_array> >(vec_array*, vec_array*, allocator<vec_array>&, _Nonscalar_ptr_iterator_tag);
}
template class std::allocator<vec_array>;
template class std::vector<vec_array>;

// 0x41a1e0 list<T*>::_Incsize (the "list<T> too long" check), 0x41ace0 push_back
template class std::list<named_object*>;

// 0x41a610 vector<0x2c-byte record>::_Buy (called from another object; the linker kept this copy)
struct rec44 { int v[11]; };
template class std::vector<rec44>;

// ---- resource manager: save --------------------------------------------------------------------------------------
// @ 0x419760
stream_base* save_mesh(stream_base* s, mesh* m)
{
    s->w_tstr(&m->name);
    s->w_int((int*)&m->flags);
    if (m->name2.len > 1)
        s->w_tstr(&m->name2);
    else
        s->w_tstr(&m->res->name);
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
            float* q = &m->v50[i].uv.x + k * 2;
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

// @ 0x4199f0 (vector<idx_weight>::erase is instantiated above)

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
                s->w_int((int*)&t->flags);
            }
        }
    }
    if (n_shader) {
        s->w_cstr("shader");
        s->w_int(&n_shader);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 4)
                ((named_object_x*)all[j])->save(s);
        }
    }
    if (n_material) {
        s->w_cstr("material");
        s->w_int(&n_material);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 3)
                ((named_object_x*)all[j])->save(s);
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
                w_body_part(*s, (body_part*)all[j]);
        }
    }
    if (n_skeleton) {
        s->w_cstr("skeleton");
        s->w_int(&n_skeleton);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 7)
                w_skeleton(*s, (skeleton*)all[j]);
        }
    }
    if (n_anim) {
        s->w_cstr("skeleton_anim");
        s->w_int(&n_anim);
        for (j = 0; j < all.size(); j++) {
            if (all[j]->type() == 8)
                save_key_clip(s, (key_clip*)all[j]);
        }
    }
    s->w_cstr("end");
}

// @ 0x419e90
resmgr::~resmgr()
{
    for (std::vector<lod_group*>::iterator it = lods.begin(); it < lods.end(); ++it) {
        lod_group* l = *it;
        for (int i = 0; i < l->count; i++)
            l->lists[i].forget();
        if (*it)
            delete *it;
    }
    for (std::vector<mesh*>::iterator it = meshes.begin(); it < meshes.end(); ++it)
        (*it)->res = 0;
    for (std::list<named_object*>::iterator it = blocks.begin(); it != blocks.end(); ++it) {
        if (*it)
            delete[] *it;
    }
}

// ---- the 0xdc-byte bone record ----------------------------------------------------------------------------------------
// @ 0x41a510 Bone::~Bone (implicit: the link vector, then the name); @ 0x41a550 ??_EBone is its vector deleting
// destructor, called by ??_Eframe_set; emitted because of the array delete below.
void destroy_bone_array(Bone* p) { delete[] p; }

// @ 0x41b000
resmgr::resmgr()
{
    c4 = 1;
    c8 = 1;
    cc = 1;
    f0 = 0;
    meshes_ready = 0;
}

// @ 0x41b0c0
Bone::Bone()
{
    f0 = 0;
    fa0 = 0; fa4 = 0; fa8 = 0;
    vb_c = vector4(0.0f, 1.0f, 0.0f);
    va_c = vector4(0.0f, 0.0f, 0.0f);
    f9c = 2.0f;
}

// ---- STL: more instantiations ----------------------------------------------------------------------------------------------
// 0x41ac10 vector<named_object*> copy constructor (+ funclet 0x41acc9) and 0x41ad20 resize are members of the
// class instantiated above; 0x41afc0 copy_backward and 0x41b7a0 fill over the bone influences:
template bone_influence* std::copy_backward(bone_influence*, bone_influence*, bone_influence*);
template void std::fill(bone_influence*, bone_influence*, const bone_influence&);
// 0x41c5c0 _Uninit_fill_n (+ funclet 0x41c628), 0x41c650 erase, 0x41c6a0 _Ufill; 0x41c760 _Insert_n
// (+ funclets 0x41c916, 0x41c9cd), 0x41cd10 resize
template void std::_Uninit_fill_n(bone_influence*, size_t, const bone_influence&, std::allocator<bone_influence>&, std::_Nonscalar_ptr_iterator_tag);
template class std::vector<bone_influence>;

// ---- skeleton model --------------------------------------------------------------------------------------------------------
#define BIT(i) ((unsigned)g_bit[i])
// @ 0x41b180
stream_base* read_frame_set(stream_base* s, frame_x* m)
{
    s->r_tstr(&m->name);
    s->r_int(&m->nbones());
    m->bones() = new Bone[m->nbones()];
    int i;
    for (i = 0; i < m->nbones(); i++) {
        Bone* b = &m->bones()[i];
        s->r_int(&b->parent);
        s->r_tstr(&b->name);
        s->r_uint(&b->flags);
        matrix* lm = &b->local;
        vector4* o = &lm->o;
        r_rotation(*s, lm).r_float(&o->x).r_float(&o->y).r_float(&o->z);
    }
    for (i = 0; i < m->nbones(); i++) {
        if (m->bones()[i].parent < 0)
            m->bones()[i].world = m->bones()[i].local;
        else
            m->bones()[i].world = m->bones()[m->bones()[i].parent].world * m->bones()[i].local;
        m->table.data[i] = m->bones()[i].world.o;
        m->table.data[i].w = 0.0f;
    }
    m->table.count = m->nbones();
    if (m->name == "skel_horse") {
        m->f28 = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15) | BIT(16) | BIT(17) | BIT(18) | BIT(19) | BIT(20) | BIT(21) | BIT(22) | BIT(23) | BIT(24) | BIT(25) | BIT(26) | BIT(27);
        m->f2c = m->f28 | BIT(4) | BIT(5) | BIT(6) | BIT(7);
        m->f30 = m->f2c;
    } else {
        m->f28 = BIT(4) | BIT(5) | BIT(6) | BIT(0) | BIT(1) | BIT(2) | BIT(3);
        m->f2c = m->f28 | BIT(7) | BIT(8) | BIT(9) | BIT(15) | BIT(16) | BIT(17) | BIT(18) | BIT(19);
        m->f30 = m->f2c | BIT(10) | BIT(11) | BIT(12) | BIT(13);
    }
    return s;
}

// The destructor of frame_set (shared with ??_Eframe_set 0x41aee0, the vector deleting destructor in vtable
// 0x54aeb4 slot 0).
frame_set::~frame_set() { if (f18 != 0 && f1c != 0) delete[] (Bone*)f1c; delete[] table.data; }

// @ 0x41b4e0
skel_anim::skel_anim() {}
int skel_anim::type() { return 8; }
// 0x41b520 is the implicit ~skel_anim; the vector deleting destructors ??_Eframe_set / ??_Eskel_anim are emitted
// because of the array news below
skel_anim* make_skel_anim_array(int n) { return new skel_anim[n]; }
frame_set* make_frame_set_array(int n) { return new frame_set[n]; }

// ---- body parts ----------------------------------------------------------------------------------------------
// The collision shapes read_body creates (src/include/shape.h): composite and sphere with the base constructor's
// stores expanded inline (shape_base(inline_init())), capsule (0x408600), face (0x408980) and
// manifold (0x40c670) through their out-of-line constructors.
// @ 0x41b5a0
stream_base* read_body(stream_base* s, body_part* b)
{
    rgl::string kind;
    s->r_tstr(&b->name);
    s->r_tstr(&kind);
    if (kind == "composite")
        b->shape = new composite(shape_base::inline_init());
    else if (kind == "sphere")
        b->shape = new sphere;
    else if (kind == "capsule")
        b->shape = new capsule;
    else if (kind == "face")
        b->shape = new face;
    else if (kind == "manifold")
        b->shape = new manifold;
    else
        mb_error("unrecognized body part %s \n", kind.c_str());
    b->shape->load(s);
    return s;
}

// ---- resource manager: registration ----------------------------------------------------------------------------------------
// resource.h declares the texture constructor out of line; the original expands it inline (in the array news of
// load_stream and in get_render_target).
inline texture::texture()
{
    loaded = 0;
    f1c = -1;
    width = 0x80;
    height = 0x80;
    kind = 0;
    d3d_tex = 0;
    surface = 0;
    level0 = 0;
    owns_texture = 0;
    used_format = 0x1c;
    format = 0x1c;
    f4c = 0;
    f50 = -1;
    mipmaps = 0;
}

// @ 0x41b7d0
texture* resmgr::get_render_target(int w, int h, int fmt, int ext, int f34)
{
    texture* r = 0;
    for (std::vector<texture*>::iterator it = textures.begin(); r == 0 && it != textures.end(); ++it) {
        if ((*it)->kind == ext && (*it)->mipmaps == f34 && (*it)->f14 == 0 && (*it)->width == w && (*it)->height == h) {
            (*it)->f14 = 1;
            r = *it;
        }
    }
    if (r == 0) {
        r = new texture[1];
        r->name.assign("render_target");
        r->kind = ext;
        r->mipmaps = f34;
        r->width = w;
        r->height = h;
        r->format = fmt;
        blocks.push_back(r);
        textures.push_back(r);
        r->create();
        r->f14 = 1;
    }
    return r;
}

// @ 0x41b960
std::vector<mesh*> resmgr::find_meshes(const rgl::string* prefix)
{
    std::vector<mesh*> out;
    rgl::string tmp;
    int to = prefix->len;
    int from;
    for (std::vector<mesh*>::iterator it = meshes.begin(); it != meshes.end(); ++it) {
        from = 0;
        if (!((tstring*)&(*it)->name)->range_equals(from, to, *(const tstring*)prefix))
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
void resmgr::add(named_object* r)
{
    int t = r->type();
    if (t == 1) {
        for (std::vector<mesh*>::iterator it = meshes.begin(); it != meshes.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister mesh %s", (*it)->name.c_str());
        meshes.push_back((mesh*)r);
    } else if (r->type() == 2) {
        for (std::vector<texture*>::iterator it = textures.begin(); it != textures.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister texture %s", (*it)->name.c_str());
        ((texture*)r)->f1c = c4;
        if (++c4 > 1000)
            c4 = 1;
        textures.push_back((texture*)r);
    } else if (r->type() == 4) {
        for (std::vector<material*>::iterator it = shaders.begin(); it != shaders.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister shader %s", (*it)->name.c_str());
        ((int*)r)[6] = cc;
        if (++cc > 1000)
            cc = 1;
        shaders.push_back((material*)r);
    } else if (r->type() == 3) {
        for (std::vector<skin*>::iterator it = materials.begin(); it != materials.end(); ++it)
            if (r->name.len && (*it)->name.len && (*it)->name.equals(r->name))
                mb_error("attempt to reregister material %s", (*it)->name.c_str());
        ((int*)r)[6] = c8;
        if (++c8 > 1000)
            c8 = 1;
        materials.push_back((skin*)r);
    } else if (r->type() == 6) {
        for (std::vector<body_part*>::iterator it = bodies.begin(); it != bodies.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister body %s", (*it)->name.c_str());
        bodies.push_back((body_part*)r);
    } else if (r->type() == 7) {
        for (std::vector<frame_set*>::iterator it = skeletons.begin(); it != skeletons.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister skeleton model %s", (*it)->name.c_str());
        skeletons.push_back((frame_set*)r);
    } else if (r->type() == 8) {
        for (std::vector<skel_anim*>::iterator it = anims.begin(); it != anims.end(); ++it)
            if ((*it)->name.equals(r->name))
                mb_error("attempt to reregister skeleton anim %s", (*it)->name.c_str());
        anims.push_back((skel_anim*)r);
    } else {
        mb_error("Unrecognized resource type.");
    }
    all.push_back(r);
}

// @ 0x41bf40
effect_set* resmgr::get_small(str_x name)
{
    effect_set* r = base().find_effect(name);
    if (r == 0) {
        r = new effect_set[1];
        r->name = name;
        blocks.push_back(r);
        effects.push_back(r);
    }
    return r;
}

// @ 0x41c000
void resmgr::load_shaders()
{
    if (g_use_shaders != 0) {
        for (std::vector<material*>::iterator it = shaders.begin(); it != shaders.end(); ++it) {
            if (((*it)->flags20 & 0x20000000) && (*it)->fc0 == 0)
                (*it)->fc0 = (int)get_small((*it)->ps_file);
        }
    }
}

// ---- readers of the animation, bone influence and morph records ------------------------------------------------------
// @ 0x41c060
stream_base* read_skel_anim(stream_base* s, skel_anim* a)
{
    s->r_tstr(&a->name);
    int n;
    s->r_int(&n);
    a->keys.resize(n);
    for (unsigned i = 0; i < a->keys.size(); i++)
        read_continuous_quats(s, &a->keys[i]);
    r_key_array_xyz(*s, &a->tail);
    return s;
}

// @ 0x41c110
stream_base* read_bone_influence(stream_base* s, bone_influence* b)
{
    int n;
    s->r_int(&b->bone);
    s->r_int(&n);
    b->verts.resize(n);
    for (int i = 0; i < n; i++) {
        idx_weight* e = &b->verts[i];
        s->r_int(&e->index);
        s->r_float(&e->weight);
    }
    return s;
}

// @ 0x41c180
stream_base* read_vec_pair(stream_base* s, frame_t* p)
{
    int n1, n2;
    s->r_int(&n1);
    p->verts.resize(n1);
    for (int i = 0; i < n1; i++) {
        vector4* v = &p->verts[i];
        s->r_float(&v->x).r_float(&v->y).r_float(&v->z);
    }
    s->r_int(&n2);
    p->norms.resize(n2);
    for (int j = 0; j < n2; j++) {
        vector4* v = &p->norms[j];
        s->r_float(&v->x).r_float(&v->y).r_float(&v->z);
    }
    return s;
}

// @ 0x41c2a0
lod_group::lod_group()
{
    count = 0;
    matrix t;
    m8c = *identity(&t);
    cur = 0;
    mask = 0xffff;
}

// @ 0x41c340
stream_base* read_keyframes(stream_base* s, keyframes* m)
{
    s->r_int(&m->count);
    if (m->count != 0) {
        m->times = new int[m->count];
        m->frames = new frame_t[m->count];
        for (int i = 0; i < m->count; i++) {
            s->r_int(&m->times[i]);
            read_vec_pair(s, &m->frames[i]);
        }
    }
    return s;
}

// @ 0x41c400
texture* resmgr::get_texture(int w, int h, int fmt) { return get_render_target(w, h, fmt, 2, 0); }

// @ 0x41c420
skin* resmgr::get_shadow_skin(int w, int h, int fmt, int f34)
{
    skin* r = 0;
    for (std::vector<skin*>::iterator it = materials.begin(); r == 0 && it != materials.end(); ++it) {
        texture* t = (*it)->tex[0];
        if (t != 0 && t->kind == 1 && t->mipmaps == f34 && (*it)->f14 == 0 && t->width == w && t->height == h && t->format == fmt) {
            (*it)->f14 = 1;
            r = *it;
        }
    }
    if (r == 0) {
        r = new skin[1];
        r->name.assign("");
        r->shader_name.assign("shadowmap_shader");
        r->tex[0] = g_resmgr_full.get_render_target(w, h, fmt, 1, f34);
        blocks.push_back(r);
        add(r);
        r->f14 = 1;
    }
    return r;
}

// @ 0x41c540
int resmgr::load_all(int check_exists, rgl::string* root)
{
    for (std::vector<texture*>::iterator it = textures.begin(); it != textures.end(); ++it)
        (*it)->load(check_exists, root);
    for (std::vector<material*>::iterator it2 = shaders.begin(); it2 != shaders.end(); ++it2)
        ((material_io*)*it2)->create_shaders();
    load_shaders();
    for (std::vector<skin*>::iterator it3 = materials.begin(); it3 != materials.end(); ++it3) {
        skin* sk = *it3;
        sk->shader = g_resmgr_full.base().resolve_shader(sk->shader_name);
    }
    return 0;
}

// ---- mesh clone, LOD groups ---------------------------------------------------------------------------------------------------
// @ 0x41c6d0
mesh_x* mesh_x::clone()
{
    mesh_x* self = this;
    mesh_x* m = (mesh_x*)new mesh;
    if (self->parent)
        m->parent = self->parent;
    else
        m->parent = self;
    m->res = self->res;
    if (m->res)
        ++m->res->f64;
    m->flags = self->flags;
    m->color = self->color;
    m->f40 = self->f40;
    m->local_a0 = 0;
    m->local_a1 = 0;
    m->name = self->name;
    m->f228 = self->f228;
    m->bounds = self->bounds;
    return m;
}

static inline int lod_digit(char c)
{
    if (c == '4') return 4;
    if (c == '3') return 3;
    if (c == '2') return 2;
    return c == '1';
}

// @ 0x41ca60
// Builds a LOD group from every mesh whose name matches `pat` (name, or name.lodN); copy != 0 shares the
// meshes, copy == 0 clones them. Missing lower-detail levels are then filled from the previous level.
lod_group* resmgr::make_lod_mesh(const rgl::string& pat, bool copy)
{
    lod_group* g = new lod_group;
    rgl::string unused;
    int pos = pat.len;
    int a;
    g->name = pat;
    for (std::vector<mesh*>::iterator it = meshes.begin(); it != meshes.end(); ++it) {
        int level;
        a = level = 0;
        if (!((tstring*)&(*it)->name)->range_equals(a, pos, *(const tstring*)&pat))
            continue;
        if (pos != (*it)->name.len) {
            const char* p = (*it)->name.buf + pos;
            if (*p == '.') {
                if ((*it)->name.len <= pos + 4)
                    continue;
                if (p[1] != 'l' && p[1] != 'L')
                    continue;
                if (p[2] != 'o' && p[2] != 'O')
                    continue;
                if (p[3] != 'd' && p[3] != 'D')
                    continue;
                level = lod_digit(p[4]);
            } else {
                a = (*it)->name.len;
                continue;
            }
        }
        if (copy) {
            vpush(g->lists[level], *it);
        } else {
            mesh* mm = ((mesh_x*)*it)->clone();
            vpush(g->lists[level], mm);
        }
        if (level + 1 > g->count)
            g->count = level + 1;
    }
    for (int k = 1; k < g->count - 1; k++) {
        std::vector<mesh*>& prev = g->lists[k - 1];
        std::vector<mesh*>& cur = g->lists[k];
        if (cur.size() == 0 && prev.size() > 0) {
            cur.resize(prev.size());
            for (unsigned j = 0; j < prev.size(); j++) {
                if (copy) {
                    vat(cur, j) = vat(prev, j);
                } else {
                    mesh** src = &vat(prev, j);
                    mesh** dst = &vat(cur, j);
                    *dst = ((mesh_x*)*src)->clone();
                }
            }
        }
    }
    return g;
}

// @ 0x41cdc0
lod_group* resmgr::get_lod_mesh(const rgl::string& name)
{
    bool found = false;
    lod_group* result = 0;
    std::vector<lod_group*>::iterator p = lods.begin();
    while (p != lods.end()) {
        if ((*p)->name.equals(name)) {
            result = *p;
            found = true;
        }
        ++p;
        if (!found)
            continue;
        return result;
    }
    if (found)
        return result;
    lod_group* g = make_lod_mesh(name, true);
    result = g;
    g->name = name;
    vpush(lods, result);
    return g;
}

// ---- tangent frames ---------------------------------------------------------------------------------------------------------
// vector4::normalize, inlined here (rgl_math.h declares it out of line)
static inline void vnorm(vector4& v)
{
    float l = (float)sqrt((v.x * v.x + v.y * v.y) + v.z * v.z);
    if (l > 1e-10f) {
        float r = 1.0f / l;
        v.x = r * v.x;
        v.y = r * v.y;
        v.z = r * v.z;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
}

// the tangent-frame code sums the squares as x*x + z*z + y*y
static inline float len2_xzy(const vector4& v) { return (v.x * v.x + v.z * v.z) + v.y * v.y; }
static inline void vnorm_xzy(vector4& v)
{
    float l = (float)sqrt(len2_xzy(v));
    if (l > 1e-10f) {
        float r = 1.0f / l;
        v.x = r * v.x;
        v.y = r * v.y;
        v.z = r * v.z;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
}

// @ 0x41ce80
// Per face the tangent and bitangent are accumulated onto its three vertices from the point positions and the
// texture coordinates; each vertex frame is then orthonormalised against the normal (Gram-Schmidt, with fallback
// axes for degenerate cases) and the bitangent is flipped to keep the frame's handedness.
// The vertex's vector at +0 is the normal as read, +0x10 the tangent and +0x20 the bitangent (vertex60 names
// them pos, f10, f20); +0x30 keeps the normal as read.
void mesh_x::compute_tangents()
{
    ensure_local();
    if (!local_a0) {
        v90 = parent->v90;                 // 0x411630 (vector<face20>::operator=)
        local_a0 = 1;
    }

    for (vertex60* v = v50._Myfirst; v < v50._Mylast; v++) {
        v->f10 = vector4(0.0f, 0.0f, 0.0f);
        v->f20 = vector4(0.0f, 0.0f, 0.0f);
        vnorm(v->pos);
    }

    for (face20* f = v90._Myfirst; f < v90._Mylast; f++) {
        vertex60* v1 = &vat(v50, f->b);
        vertex60* v0 = &vat(v50, f->a);
        point20* p0 = &vat(v70, v0->index);
        point20* p1 = &vat(v70, v1->index);
        float e1x = p1->a.x - p0->a.x;
        vertex60* v2 = &vat(v50, f->c);
        point20* p2 = &vat(v70, v2->index);
        float e2x = p2->a.x - p0->a.x;
        float e1y = p1->a.y - p0->a.y;
        float e2y = p2->a.y - p0->a.y;
        float e1z = p1->a.z - p0->a.z;
        float e2z = p2->a.z - p0->a.z;
        float du1 = v1->uv.x - v0->uv.x;
        float du2 = v2->uv.x - v0->uv.x;
        float dv1 = v1->uv.y - v0->uv.y;
        float dv2 = v2->uv.y - v0->uv.y;
        float r = 1.0f / (du1 * dv2 - dv1 * du2);
        float tx = (dv2 * e1x - dv1 * e2x) * r;
        float ty = (e1y * dv2 - dv1 * e2y) * r;
        float tz = (dv2 * e1z - dv1 * e2z) * r;
        float bx = (du1 * e2x - du2 * e1x) * r;
        float by = (du1 * e2y - du2 * e1y) * r;
        float bz = (du1 * e2z - du2 * e1z) * r;
        v0->f10.x = tx + v0->f10.x;
        v0->f10.y = ty + v0->f10.y;
        v0->f10.z = tz + v0->f10.z;
        vertex60* q = &vat(v50, f->b);
        q->f10.x = tx + q->f10.x;
        q->f10.y = ty + q->f10.y;
        q->f10.z = tz + q->f10.z;
        q = &vat(v50, f->c);
        q->f10.x = tx + q->f10.x;
        q->f10.y = ty + q->f10.y;
        q->f10.z = tz + q->f10.z;
        q = &vat(v50, f->a);
        q->f20.x = bx + q->f20.x;
        q->f20.y = by + q->f20.y;
        q->f20.z = bz + q->f20.z;
        q = &vat(v50, f->b);
        q->f20.x = bx + q->f20.x;
        q->f20.y = by + q->f20.y;
        q->f20.z = bz + q->f20.z;
        q = &vat(v50, f->c);
        q->f20.x = bx + q->f20.x;
        q->f20.y = by + q->f20.y;
        q->f20.z = bz + q->f20.z;
    }

    for (vertex60* v = v50._Myfirst; v < v50._Mylast; v++) {
        vector4& n = v->pos;
        vector4& t = v->f10;
        vector4& b = v->f20;
        // handedness of the accumulated frame
        bool flip = true;
        float h = (t.z * n.y - n.z * t.y) * b.x + (n.z * t.x - t.z * n.x) * b.y + (n.x * t.y - n.y * t.x) * b.z;
        if (!(h < 0.0f))
            flip = false;
        float d = (t.x * n.x + n.y * t.y) + t.z * n.z;
        t = vector4(t.x - d * n.x, t.y - d * n.y, t.z - d * n.z);
        if (len2_xzy(t) < 1e-5f) {
            t = vector4(n.y * 0.7f - n.z * 0.2f, n.z * 0.1f - n.x * 0.7f, n.x * 0.2f - n.y * 0.1f);
            if (len2_xzy(t) < 1e-5f)
                t = vector4(n.y * 0.35f - n.z * 0.17f, n.z * 0.73f - n.x * 0.35f, n.x * 0.17f - n.y * 0.73f);
        }
        vnorm_xzy(t);
        b = vector4(t.z * n.y - n.z * t.y, n.z * t.x - t.z * n.x, n.x * t.y - n.y * t.x);
        if (len2_xzy(b) < 1e-5f) {
            t = vector4(n.y * 0.4f - n.z * 0.8f, n.z * 0.2f - n.x * 0.4f, n.x * 0.8f - n.y * 0.2f);
            if (len2_xzy(t) < 1e-5f)
                t = vector4(n.y * 0.39f - n.z * 0.67f, n.z * 0.76f - n.x * 0.39f, n.x * 0.67f - n.y * 0.76f);
            vnorm_xzy(t);
            b = vector4(t.z * n.y - n.z * t.y, n.z * t.x - t.z * n.x, n.x * t.y - n.y * t.x);
        }
        if (flip)
            b = vector4(-b.x, -b.y, -b.z);
    }
}

// ---- mesh reader --------------------------------------------------------------------------------------------------------------
// @ 0x41d6f0
stream_base* read_mesh(stream_base* s, mesh* m)
{
    rgl::string unused;
    int npts;
    int nbi;
    int nverts;
    int nfaces;

    s->r_tstr(&m->name);
    s->r_int((int*)&m->flags);
    s->r_tstr(&m->name2);
    m->res = g_resmgr.get_material(m->name2);
    if (m->res)
        m->res->f64++;

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
        read_bone_influence(s, &vat(m->v80, i));
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
        s->r_float(&v->pos.x).r_float(&v->pos.y).r_float(&v->pos.z);
        m->v50[i].normal = m->v50[i].pos;
        m->v50[i].f58 = m->v50[i].color;
        for (int j = 0; j < 2; j++) {
            float* f = &m->v50[i].uv.x + j * 2;
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

    ((mesh_x*)m)->compute_tangents();

    m->f228 = 0;
    for (int i = 0; i < nbi; i++) {
        if (vat(m->v80, i).bone + 1 > m->f228)
            m->f228 = vat(m->v80, i).bone + 1;
    }
    if (m->f228 > 0x20)
        mb_error("Too many bones in mesh %s", m->name.c_str());
    if (nbi != 0 || (m->res && m->res->shader && (m->res->shader->flags20 & 0x80000000) != 0))
        m->build_skin();
    ((mesh_x*)m)->compute_bounds();
    m->slots[1].f4 = 0;
    m->slots[0].f4 = 0;
    return s;
}

// ---- resource files ------------------------------------------------------------------------------------------------------------
// @ 0x41db20
stream_base* resmgr::load_stream(stream_base* s)
{
    rgl::string tok;
    rgl::string tok2;
    int n;
    s->r_tstr(&tok);
    while (tok != "end") {
        if (tok == "texture") {
            s->r_int(&n);
            if (n != 0) {
                texture* t = new texture[n];
                blocks.push_back(t);
                for (int i = 0; i < n; i++) {
                    s->r_tstr(&t[i].name);
                    s->r_int((int*)&t[i].flags);
                    add(&t[i]);
                }
            }
        } else if (tok == "shader") {
            s->r_int(&n);
            if (n != 0) {
                material* m = new material[n];
                blocks.push_back(m);
                for (int i = 0; i < n; i++) {
                    m[i].load(s);
                    add(&m[i]);
                }
            }
        } else if (tok == "material") {
            s->r_int(&n);
            if (n != 0) {
                skin* k = new skin[n];
                blocks.push_back(k);
                for (int i = 0; i < n; i++) {
                    k[i].load(s);
                    add(&k[i]);
                }
            }
        } else if (tok == "mesh") {
            s->r_int(&n);
            if (n != 0) {
                mesh* m = new mesh[n];
                blocks.push_back(m);
                for (int i = 0; i < n; i++) {
                    read_mesh(s, &m[i]);
                    add(&m[i]);
                }
            }
        } else if (tok == "body") {
            s->r_int(&n);
            if (n != 0) {
                body_part* b = new body_part[n];
                blocks.push_back(b);
                for (int i = 0; i < n; i++) {
                    read_body(s, &b[i]);
                    add(&b[i]);
                }
            }
        } else if (tok == "skeleton") {
            s->r_int(&n);
            if (n != 0) {
                frame_set* f = new frame_set[n];
                blocks.push_back(f);
                for (int i = 0; i < n; i++) {
                    read_frame_set(s, (frame_x*)&f[i]);
                    add(&f[i]);
                }
            }
        } else if (tok == "skeleton_anim") {
            s->r_int(&n);
            if (n != 0) {
                skel_anim* a = new skel_anim[n];
                blocks.push_back(a);
                for (int i = 0; i < n; i++) {
                    read_skel_anim(s, &a[i]);
                    add(&a[i]);
                }
            }
        } else {
            mb_error("Unrecognized resource type %s", tok.c_str());
        }
        s->r_tstr(&tok);
    }
    return s;
}

static __forceinline rgl::string mkpath(const rgl::string& dir, const rgl::string& name, const char* ext)
{
    tstring ext_str(ext);
    const rgl::string& e = (const rgl::string&)ext_str;
    return dir + name + e;
}

// @ 0x41e150
void resmgr::load_pack(const rgl::string& name, int user)
{
    rgl::string dir;
    if (user) {
        dir = cur;
        tstring r_str("Resource\\");
        dir.append((const rgl::string&)r_str);
    } else {
        tstring r_str("CommonRes\\");
        dir.append((const rgl::string&)r_str);
    }
    rgl::string trf = mkpath(dir, name, ".trf");
    rgl::string brf = mkpath(dir, name, ".brf");
    all.resize(0, 0);
    bool done = false;
    if (g_use_text_res) {
        struct _stat st_t, st_b;
        if (_stat(trf.c_str(), &st_t) != -1) {
            if (_stat(brf.c_str(), &st_b) == -1 || difftime(st_t.st_mtime, st_b.st_mtime) > 0.0) {
                text_reader ts;
                if (((text_stream*)&ts)->open_read_text(trf.c_str())) {
                    load_stream(&ts);
                    ts.failed = 1;
                    done = true;
                    bin_writer w;
                    ((text_stream*)&w)->open_write(brf.c_str());   // file_writer_base members (0x415320, 0x415370)
                    save(&w);
                    ((text_stream*)&w)->close();
                }
            }
        }
    }
    if (!done) {
        file_reader br;
        const char* p = brf.c_str();
        if (!((text_stream*)&br)->open_read(p))        // 0x415290
            mb_error("Unable to open file: %s\n", p);
        load_stream(&br);
        br.failed = 1;
    }
}

// ---- the reader classes' slots (the original's linker folded them into text_stream's / mem_stream's) ----
STREAM_FWD_TEXT_BOTH(text_reader_b)
STREAM_FWD_TEXT_READER(text_reader)
STREAM_FWD_MEM(mem_stream_inl)
STREAM_FWD_BIN_READER(file_reader)
