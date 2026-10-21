// Slice s00418a70: the resource manager (global at 0x5d6fa0): name lookups in its eight resource lists with the
// engine's "find by name" idiom, per-frame resource maintenance loops, a shader-chain resolver, the material
// (de)serialisers and a gamma conversion pass.
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include "mbbin.h"
#include "../../include/rgl.h"

void mb_error(const char* fmt, ...);                                  // 0x42cc80

// 0x5e28fc: non-zero when the programmable pipeline (shaders) is in use; zero selects the fixed-function path.
extern int g_use_shaders;                      // 0x5e28fc

// ---- resources and lists ------------------------------------------------------------------------
// Every resource derives from named_object (vtable slot 0 dtor, slot 1 type id; name at +4).
struct res_hdr : named_object {
    int f14;                                   // +0x14
};
struct resource : res_hdr {
    char loaded;                               // +0x18
    int f1c;
    int kind;                                  // +0x20
};
// std::vector-like list of resource pointers (16 bytes: allocator, first, last, end).
template<class T> struct plist {
    int alloc;
    T** first;
    T** last;
    T** end;
};
// ---- record types of the lists (partial; layouts from the neighbouring slices s00416540 / s00416ef0) ----
struct texture : resource {                    // list at +0x30 (vtable 0x54aaac)
    // loaded +0x18, id +0x1c, external +0x20 (kind), flags +0x24
    int load(int check_exists, tstring* root);     // 0x416540 (thiscall, ret 8)
    int unload();                                  // 0x4167c0
};
struct material : res_hdr {                    // list at +0x40 (vtable 0x54ab30, 0xc8 bytes)
    int f18, f1c;
    unsigned flags20;                          // +0x20 (bit 28: has a pixel shader)
    int nalt;                                  // +0x24
    tstring alts[3];                           // +0x28 alternative shader names
    char pad58[0xbc - 0x58];
    IUnknown* ps;                              // +0xbc pixel shader
    bool supported();                          // 0x416930
};
struct effect_set : res_hdr {                 // list at +0x90 (vtable 0x54ac08, 0x2c bytes)
    char resolved;                             // +0x18
    int h[4];                                  // +0x1c technique handles
    int resolve(void* fx);                     // 0x417050
};

// Text log stream (global object at 0x5a9f60, vtable of the text stream).
struct log_stream : stream_base {              // text stream (vtable 0x54a7d0), concrete
    BIN_STREAM_SLOTS
    FILE* fp;
    char failed, is_open;
};
extern log_stream g_log;                       // 0x5a9f60
struct resource_manager {
    int pad[8];
    plist<resource> v20;                       // +0x20
    plist<texture> textures;                   // +0x30
    plist<material> shaders;                   // +0x40
    plist<resource> materials;                 // +0x50
    plist<resource> v60;                       // +0x60
    plist<resource> v70;                       // +0x70
    plist<resource> skel_anims;                // +0x80
    plist<effect_set> v90;                     // +0x90
    char pad_a0[0xbc - 0xa0];
    tstring root;                              // +0xbc
    resource* get_material(const tstring& name);       // 0x418a70
    material* get_shader(const tstring& name);         // 0x418b30
    effect_set* find_v90(const tstring& name);         // 0x418b90
    resource* find_v20(const tstring& name);           // 0x418bd0
    resource* get_object60(const tstring& name);       // 0x418c10
    resource* get_skeleton_anim(const tstring& name);  // 0x418c80
    resource* get_object70(const tstring& name);       // 0x418cf0
    resource* get_texture(const tstring& name);        // 0x418870 (s00418150)
    int load_textures();                               // 0x418d60
    int reload_textures();                             // 0x418da0
    int unload_textures();                             // 0x418e10
    void release_pixel_shaders();                      // 0x418e40
    material* resolve_shader(const tstring& name);     // 0x418e90
    void reset_effect_sets();                          // 0x418fb0
    void resolve_effect_sets(void* fx);                // 0x418ff0
};

// @ 0x418a70
resource* resource_manager::get_material(const tstring& name)
{
    resource* r = 0;
    resource** it = materials.first;
    while (r == 0 && it != materials.last) {
        if ((*it)->name.equals(name))
            r = *it;
        ++it;
    }
    if (r == 0) {
        stream_base& log = g_log;
        log.w_cstr("WARNING: Unable to find material ").w_tstr(&name).w_cstr("\n");
        if (name != "default") {
            tstring d("default");
            return get_material(d);
        }
        return materials.first[0];
    }
    return r;
}

// @ 0x418b30
material* resource_manager::get_shader(const tstring& name)
{
    material* r = 0;
    material** it = shaders.first;
    while (r == 0 && it != shaders.last) {
        if ((*it)->name.equals(name))
            r = *it;
        ++it;
    }
    if (r == 0)
        mb_error("get_shader failed for: %s", name.c_str());
    return r;
}

// @ 0x418b90
effect_set* resource_manager::find_v90(const tstring& name)
{
    effect_set* r = 0;
    effect_set** it = v90.first;
    while (r == 0 && it != v90.last) {
        if ((*it)->name.equals(name))
            r = *it;
        ++it;
    }
    return r;
}

// @ 0x418bd0
resource* resource_manager::find_v20(const tstring& name)
{
    bool found = false;
    resource* r = 0;
    resource** it = v20.first;
    while (!found && it != v20.last) {
        if ((*it)->name.equals(name)) {
            r = *it;
            found = true;
        }
        ++it;
    }
    return r;
}

// @ 0x418c10
resource* resource_manager::get_object60(const tstring& name)
{
    bool found = false;
    resource* r = 0;
    resource** it = v60.first;
    while (!found && it != v60.last) {
        if ((*it)->name.equals(name)) {
            r = *it;
            found = true;
        }
        ++it;
    }
    if (!found)
        mb_error("get_object failed for: %s", name.c_str());
    return r;
}

// @ 0x418c80
resource* resource_manager::get_skeleton_anim(const tstring& name)
{
    bool found = false;
    resource* r = 0;
    resource** it = skel_anims.first;
    while (!found && it != skel_anims.last) {
        if ((*it)->name.equals(name)) {
            r = *it;
            found = true;
        }
        ++it;
    }
    if (!found)
        mb_error("get_skeleton_anim failed for: %s", name.c_str());
    return r;
}

// @ 0x418cf0
resource* resource_manager::get_object70(const tstring& name)
{
    bool found = false;
    resource* r = 0;
    resource** it = v70.first;
    while (!found && it != v70.last) {
        if ((*it)->name.equals(name)) {
            r = *it;
            found = true;
        }
        ++it;
    }
    if (!found)
        mb_error("get_object failed for: %s", name.c_str());
    return r;
}

// @ 0x418d60
int resource_manager::load_textures()
{
    for (texture** it = textures.first; it != textures.last; ++it) {
        if ((*it)->kind == 0)
            (*it)->load(1, &root);
    }
    return 0;
}

// @ 0x418da0
int resource_manager::reload_textures()
{
    for (texture** it = textures.first; it != textures.last; ++it) {
        if ((*it)->loaded && ((*it)->kind == 1 || (*it)->kind == 2))
            (*it)->unload();
    }
    for (effect_set** it = v90.first; it != v90.last; ++it) {
        effect_set* e = *it;
        for (int i = 0; i < 4; i++)
            e->h[i] = 0;
        e->resolved = 0;
    }
    return 0;
}

// @ 0x418e10
int resource_manager::unload_textures()
{
    for (texture** it = textures.first; it != textures.last; ++it) {
        if ((*it)->kind == 0)
            (*it)->unload();
    }
    return 0;
}

// @ 0x418e40
void resource_manager::release_pixel_shaders()
{
    for (material** it = shaders.first; it != shaders.last; ++it) {
        if (((*it)->flags20 & 0x10000000) && g_use_shaders == 0 && (*it)->ps)
            (*it)->ps->Release();
    }
}

// @ 0x418e90
// Picks the best usable shader: the named one if the hardware supports it, else the first supported alternative,
// else the recursively resolved alternatives; falls back to the default shader.
material* resource_manager::resolve_shader(const tstring& name)
{
    material* shader = get_shader(name);
    material* best;
    if (g_use_shaders)
        best = get_shader(tstring("def_shader"));
    else
        best = get_shader(tstring("def_shader_ffp"));
    if (best == 0)
        mb_error("Unable to Find default shader!");
    if (shader->supported())
        return shader;
    bool found = false;
    int i;
    tstring* alt = &shader->alts[0];
    for (i = 0; i < shader->nalt; i++) {
        material* m = get_shader(*alt);
        if (m->supported()) {
            best = m;
            found = true;
        }
        alt++;
        if (found)
            break;
    }
    i = 0;
    if (!found) {
        alt = &shader->alts[0];
        for (; i < shader->nalt; i++) {
            material* m = resolve_shader(*alt);
            if (m != best) {
                best = m;
                found = true;
            }
            alt++;
            if (found)
                break;
        }
    }
    return best;
}

// @ 0x418fb0
void resource_manager::reset_effect_sets()
{
    if (g_use_shaders) {
        for (effect_set** it = v90.first; it != v90.last; ++it) {
            effect_set* e = *it;
            for (int i = 0; i < 4; i++)
                e->h[i] = 0;
            e->resolved = 0;
        }
    }
}

// @ 0x418ff0
void resource_manager::resolve_effect_sets(void* fx)
{
    if (g_use_shaders) {
        for (effect_set** it = v90.first; it != v90.last; ++it)
            (*it)->resolve(fx);
    }
}

// ---- serialisers --------------------------------------------------------------------------------
// keys[n] plus 16-byte vectors (s00416ef0 key_array); written/read as `int key; y z w x` or `x y z`.
struct key_array {
    rgl::vector4* data;            // +0
    int* keys;                     // +4
    int n;                         // +8
};
stream_base& w_key_array_yzwx(stream_base& s, key_array* a);       // 0x417330 (cdecl)
stream_base& w_key_array_xyz(stream_base& s, key_array* a);        // 0x4174a0 (cdecl)

// Animation clip: name, a vector of key arrays (12 bytes each, std::vector at +0x18) and one more key array.
struct key_clip : res_hdr {
    std::vector<key_array> tracks;             // +0x18 (allocator, first +0x1c, last +0x20, end +0x24)
    key_array extra;                           // +0x28
};

// @ 0x419030
stream_base* save_key_clip(stream_base* s, key_clip* c)
{
    s->w_tstr(&c->name);
    unsigned n = c->tracks.size();
    s->w_uint(&n);
    for (unsigned i = 0; i < c->tracks.size(); i++)
        w_key_array_yzwx(*s, &(&*c->tracks.begin())[i]);
    w_key_array_xyz(*s, &c->extra);
    return s;
}

// Morph key frame: two vectors of 16-byte vectors (positions, normals), each written as a count and x y z.
struct frame_t {
    std::vector<rgl::vector4> verts;           // +0
    std::vector<rgl::vector4> norms;           // +0x10
};

// @ 0x419200
stream_base* save_frame(stream_base* s, frame_t* f)
{
    int n = (int)f->verts.size();
    s->w_int(&n);
    for (int i = 0; i < n; i++) {
        rgl::vector4* v = &(&*f->verts.begin())[i];
        s->w_float(&v->x).w_float(&v->y).w_float(&v->z);
    }
    int n2 = (int)f->norms.size();
    s->w_int(&n2);
    for (int i = 0; i < n2; i++) {
        rgl::vector4* v = &(&*f->norms.begin())[i];
        s->w_float(&v->x).w_float(&v->y).w_float(&v->z);
    }
    return s;
}

// Mesh adjacency record: an id and a vector of (int, float) pairs.
struct pair_item { int a; float b; };
struct pair_list {
    int id;                                    // +0
    std::vector<pair_item> items;              // +4
};

// @ 0x419190
stream_base* save_pair_list(stream_base* s, pair_list* e)
{
    s->w_int(&e->id);
    int n = (int)e->items.size();
    s->w_int(&n);
    for (int i = 0; i < n; i++) {
        pair_item* p = &(&*e->items.begin())[i];
        s->w_int(&p->a);
        s->w_float(&p->b);
    }
    return s;
}

// Game material record read from a stream (the records of the list at +0x50): name, flags, a shader-name string,
// five texture references (by name; "none" = no texture) and a few floats.
struct vec3f { float x, y, z; };
static __forceinline stream_base& r_vec3(stream_base& s, vec3f* v) { return s.r_float(&v->x).r_float(&v->y).r_float(&v->z); }
struct material_def : res_hdr {
    int f18;                                   // +0x18
    unsigned flags;                            // +0x1c
    tstring shader_name;                       // +0x20
    char pad30[4];
    vec3f v34;                                 // +0x34
    int f40;
    float f44;
    resource* tex[5];                          // +0x48
    void load(stream_base* s);                 // 0x4190d0
};
extern resource_manager g_resmgr;              // 0x5d6fa0

// @ 0x4190d0
void material_def::load(stream_base* s)
{
    tstring tmp;
    s->r_tstr(&name);
    s->r_uint(&flags);
    s->r_tstr(&shader_name);
    for (int i = 0; i < 5; i++) {
        s->r_tstr(&tmp);
        if (tmp == "none")
            tex[i] = 0;
        else
            tex[i] = g_resmgr.get_texture(tmp);
    }
    r_vec3(s->r_float(&f44), &v34);
}

// ---- mesh (partial layout from the s00412a90 mesh.h) ---------------------------------------------
void unpack_color(rgl::vector4* out, const unsigned* argb);            // 0x40e4e0 (cdecl)
unsigned pack_color(const rgl::vector4* c);                           // 0x40e440 (cdecl)

struct vertex60 {                              // 0x60 bytes
    rgl::vector4 pos, f10, f20, normal;
    float uv[2];
    int f48, f4c;
    int index;                                 // +0x50
    unsigned color;                            // +0x54 packed ARGB
    int f58, f5c;
};
struct point20 { rgl::vector4 a, b; };         // 0x20
struct aabb {                                  // 0x38 bytes (s00416540): min, max, centre, radius
    char b[0x38];
    void reset();                              // 0x416e40
    void extend(const rgl::vector4* p);        // 0x416ef0
    void update();                             // 0x416f70
};
struct mesh_obj {
    void* vptr;
    char pad4[0x18 - 4];
    mesh_obj* parent;                          // +0x18
    char pad1c[0x50 - 0x1c];
    std::vector<vertex60> v50;                 // +0x50
    char pad60[0x10];
    std::vector<point20> v70;                  // +0x70
    char pad80[0xa0 - 0x80];
    char local_a0, local_a1;                   // +0xa0 / +0xa1: faces / points+vertices are local
    char pada2[0x22c - 0xa2];
    aabb bounds;                               // +0x22c
    std::vector<point20>& get70_();            // 0x40eba0 (the parent-chain walk, out of line)
    void compute_bounds();                     // 0x419300
    void linearise_vertex_colors();            // 0x419360
};

// @ 0x419300
void mesh_obj::compute_bounds()
{
    bounds.reset();
    std::vector<point20>* v;
    if (local_a1)
        v = &v70;
    else
        v = &parent->get70_();
    for (unsigned i = 0; i < v->size(); i++)
        bounds.extend(&(&*v->begin())[i].a);
    bounds.update();
}

// Converts the vertex colours from gamma space to linear (pow 2.2) and copies the packed result to +0x58.
// @ 0x419360
void mesh_obj::linearise_vertex_colors()
{
    if (local_a1) {
        int n = (int)v50.size();
        for (int i = 0; i < n; i++) {
            vertex60* v = &(&*v50.begin())[i];
            rgl::vector4 c;
            unpack_color(&c, &v->color);
            c.x = (float)pow(c.x, 2.2f);
            c.y = (float)pow(c.y, 2.2f);
            c.z = (float)pow(c.z, 2.2f);
            v->color = pack_color(&c);
            (&*v50.begin())[i].f58 = (&*v50.begin())[i].color;
        }
    }
}
