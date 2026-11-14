// resource_manager.cpp (0x417bb0..0x41918f): the binary file streams' ctors/flush and mem_stream's dtor,
// key tables and animation-frame records, the vector deleting destructors of texture/effect_set, STL helper
// instantiations, the options.dat writer, render-target creation, the resource manager's lookups and
// per-frame maintenance, and the material loader.
// Migrated from slices s00417850 (0x417bb0..0x418110), s00418150 (0x418150..0x418870) and s00418a70
// (0x418a70..0x4190d0).
// Boundaries: content only (no call crosses either side). options::save (0x4181c0) inlines the bin_writer
// ctor defined here (0x417c10), which is glue between the two.
#include <windows.h>
#include <shlobj.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include <vector>
#include <list>
#include <memory>
#include <algorithm>
#include "./rgl_string.h"
#include "../game/tstring.h"      // tstring: the class that defines the rgl::string members called below (same 16-byte object)
#define TS(s) ((tstring&)(s))
#include "./rgl_math.h"
#include "./stream.h"
#include "./resource.h"
#include "./text_stream.h"
#include "./mem_stream.h"
#include "../include/renderer.h"      // g_render 0xa17910 (dev = g_render.dev 0xa17f28)
#include "../game/config/config_block.h"   // Config g_cfg (0x5a9f98; saved through rgl_config::save)
#include "./resource_manager.h"
#include "./mesh.h"                  // rgl::mesh: the element type of the manager's mesh list
#include "../include/vec_table.h"      // (listed directly: the resolution audit scans only the source's own includes)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
options g_options = {  // 0x5aa008
    /*i00*/ 3, /*i04*/ 3, /*f08*/ 0.4f, /*f0c*/ 1.0f, /*f10*/ 0.5f, /*i14*/ 0,
    /*i18*/ 1, /*i1c*/ 1, /*i20*/ 1, /*i24*/ 1, /*i28*/ 1, /*i2c*/ 1,
    /*i30*/ 1, /*i34*/ 50, /*i38*/ 1, /*i3c*/ 20, /*i40*/ 5, /*i44*/ 2,
    /*i48*/ 0, /*i4c*/ 0, /*i50*/ 0, /*i54*/ 1, /*i58*/ 1, /*i5c*/ 1,
    /*i60*/ 1, /*i64*/ 1, /*i68*/ 0, /*i6c*/ 2, /*i70*/ 0, /*f74*/ 0.3422f,
    /*i78*/ 1, /*i7c*/ 2, /*f80*/ 2.2f, /*f84*/ 0.5f, /*f88*/ 0.5f, /*i8c*/ 0,
    /*i90*/ 0, /*i94*/ 0, /*i98*/ 0
};


// ---- binary file streams ---------------------------------------------------------------------
// @ 0x417bb0
int bin_writer::flush()
{
    int r = 0;
    if (is_open) {
        int len = buf.len;
        if (flushed < len) {
            int bad = 0;
            const char* p = buf.c_str();
            size_t n = fwrite(flushed + p, 1, len - flushed, fp);
            if (n != buf.len - flushed)
                bad = 1;
            fflush(fp);
            flushed = buf.len;
            r = bad;
        }
    }
    return r;
}

// @ 0x417c10
bin_writer::bin_writer() {}
// the slots the original's linker folded with mem_stream's (0x417930..0x417b50) and text_stream's (0x415870): own bodies
stream_base& bin_writer::w_stream(stream_base* o)
{
    int n = o->buf.len - o->buf.pos;
    TS(buf).write(&n, 4);
    if (n > 0) {
        const char* p;
        if (o->buf.buf)
            p = o->buf.pos + o->buf.buf;
        else
            p = (const char*)&o->buf.buf;
        TS(buf).write(p, n);
    }
    return *this;
}
stream_base& bin_writer::w_cstr(const char* s)
{
    int n = strlen(s);
    TS(buf).write(&n, 4);
    TS(buf).write(s, n);
    return *this;
}
stream_base& bin_writer::w_int(int* v) { TS(buf).write(v, 4); return *this; }
stream_base& bin_writer::w_float(float* v) { TS(buf).write(v, 4); return *this; }
stream_base& bin_writer::w_uint(unsigned* v) { TS(buf).write(v, 4); return *this; }
stream_base& bin_writer::w_i64(__int64* v) { TS(buf).write(v, 8); return *this; }
stream_base& bin_writer::w_u64(unsigned __int64* v) { TS(buf).write(v, 8); return *this; }
stream_base& bin_writer::w_tstr(const rgl::string* s)
{
    int n = s->len;
    TS(buf).write(&n, 4);
    TS(buf).write(s->c_str(), n);
    return *this;
}
stream_base& bin_writer::w_byte(unsigned char* v) { TS(buf).write(v, 1); return *this; }
stream_base& bin_writer::w_bool(char* v) { TS(buf).write(v, 1); return *this; }
stream_base& bin_writer::r_stream(stream_base* o)
{
    int n;
    r_int(&n);
    o->reset();
    for (int i = 0; i < n; i++) {
        char c;
        TS(buf).read(&c, 1);
        TS(o->buf).push_back(c);
    }
    return *this;
}
stream_base& bin_writer::r_byte(char* v) { TS(buf).read(v, 1); return *this; }
stream_base& bin_writer::slot12(void* v) { return *this; }
stream_base& bin_writer::slot22(void* v) { return *this; }
stream_base& bin_writer::r_float(float* v) { return *this; }
stream_base& bin_writer::r_bool(char* v) { return *this; }
stream_base& bin_writer::r_uint(unsigned* v) { return *this; }
stream_base& bin_writer::r_u64(unsigned __int64* v) { return *this; }
stream_base& bin_writer::r_i64(__int64* v) { return *this; }
stream_base& bin_writer::r_int(int* v) { return *this; }
stream_base& bin_writer::r_tstr(rgl::string* v) { return *this; }

// @ 0x417c50
bin_reader::bin_reader() {}
// the write slots a reader does not have
stream_base& bin_reader::w_float(float* v) { return *this; }
stream_base& bin_reader::w_bool(char* v) { return *this; }
stream_base& bin_reader::w_u64(unsigned __int64* v) { return *this; }
stream_base& bin_reader::w_i64(__int64* v) { return *this; }
stream_base& bin_reader::w_uint(unsigned* v) { return *this; }
stream_base& bin_reader::w_int(int* v) { return *this; }
stream_base& bin_reader::w_tstr(const rgl::string* s) { return *this; }

// @ 0x417c90
mem_stream::~mem_stream() {}

// ---- key tables ------------------------------------------------------------------------------------
// @ 0x417cb0
vec_table::vec_table()
{
    count = 0x20;
    data = (rgl::vector4*)operator new[](0x200);
    rgl::vector4 tmp(0.0f, 0.0f, 0.0f);
    for (int j = 0; j < 4; j++)
        for (int k = 0; k < 8; k++) {
            data[j * 8 + k] = tmp;
            data[j * 8 + k].w = 0.0f;
        }
}

int frame_set::type() { return 7; }                          // 0x49df30 (folded with Camera::type)
// @ 0x417e10
frame_set::frame_set()
{
    f18 = 0;
    f28 = 0;
    f2c = 0;
    f30 = 0;
}

// @ 0x417e50
stream_base* read_continuous_quats(stream_base* s, key_array* a)
{
    r_key_array_yzwx(*s, a);
    rgl::vector4 prev;
    if (a->n > 0)
        prev = a->data[0];
    for (int i = 1; i < a->n; i++) {
        rgl::vector4* q = &a->data[i];
        if (prev.z * q->z + (prev.w * q->w + (prev.y * q->y + prev.x * q->x)) < 0.0f) {
            q->y = -q->y;
            q->z = -q->z;
            q->w = -q->w;
            q->x = -q->x;
        }
        prev = *q;
    }
    return s;
}

// ---- vector deleting destructors (compiler-generated): 0x417f20 texture, 0x417fb0 effect_set --------
// Generated because this object contains array news of the two classes (see the stand-in in mem_stream.cpp);
// the stand-in below is not in the binary.
void deleting_dtor_standin2(texture* a, effect_set* b)
{
    new texture[3];
    new effect_set[3];
    delete[] a;
    delete[] b;
}

// ---- STL instantiations ---------------------------------------------------------------------------
// Element types of these instantiations are unknown (the code that used them is not in this object's
// surviving functions): a pointer-sized list element and an 8-byte vector element.
struct res_obj;
#include "../include/pod_elems.h"   // E8 (shared)
#include "../include/render_flags.h"

namespace std {
// 0x4180d0 std::_Copy_opt
template key_array* _Copy_opt<key_array*, key_array*>(key_array*, key_array*, key_array*, _Nonscalar_ptr_iterator_tag);
// 0x418110 std::_Copy_backward_opt
template key_array* _Copy_backward_opt<key_array*, key_array*>(key_array*, key_array*, key_array*, _Nonscalar_ptr_iterator_tag);
// 0x418150 std::_Uninit_fill_n
template void _Uninit_fill_n<E8*, unsigned int, E8, allocator<E8> >(E8*, unsigned int, const E8&, allocator<E8>&, _Nonscalar_ptr_iterator_tag);
// 0x4184b0 std::_Uninit_copy
template int** _Uninit_copy<int**, int**, allocator<int*> >(int**, int**, int**, allocator<int*>&, _Nonscalar_ptr_iterator_tag);
// 0x418840 std::fill
template void fill<key_array*, key_array>(key_array*, key_array*, const key_array&);
}
// 0x418050 _Buynode(next, prev, val) and 0x4187b0 _Buynode() (the head node); their catch(...) funclets are
// 0x4180b9 and 0x41881f. (The funclets of 0x418150 and 0x4184b0 are 0x4181b4 and 0x418516.)
// 0x418050 list::_Buynode (next, prev, val)
// 0x4187b0 list::_Buynode (head node)
template class std::list<res_obj*>;

// ---- game options ---------------------------------------------------------------------------------
// The user-data path builder at 0x4bab70 takes its argument in ESI: a static helper of another object.
// Reproduced here with the same body so cl picks the same register convention for the call (a local copy:
// the original calls 0x4bab70).
static __declspec(noinline) void user_path(char* name)
{
    char folder[260];
    char dir[260];
    char tmp[260];
    SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, folder);
    sprintf(dir, "%s\\Mount&Blade", folder);
    CreateDirectoryA(dir, 0);
    sprintf(tmp, "%s", name);
    sprintf(name, "%s\\Mount&Blade\\%s", folder, tmp);
}

// @ 0x4181c0
void options::save()
{
    char path[0x104];
    bin_writer w;
    mb_snprintf(path, 0x104, "options.dat");
    user_path(path);
    w.name.assign(path);
    w.is_open = 0;
    w.fp = fopen(path, "wb");
    if (w.fp) {
        w.failed = 0;
        w.is_open = 1;
    } else
        w.failed = 1;
    stream_base& s = w;
    s.w_float(&music_volume);
    s.w_float(&master_volume);
    s.w_float(&exp_arg);
    s.w_int(&invert);
    s.w_int(&char_detail);
    s.w_int(&i20);
    s.w_int(&i24);
    s.w_int(&i28);
    s.w_int(&i30);
    s.w_int(&i00);
    s.w_int(&i34);
    s.w_int(&agent_count);
    s.w_int(&dmg_to_friends);
    s.w_int(&i98);
    s.w_int(&difficulty);
    s.w_int(&dmg_to_player);
    s.w_int(&fx_on);
    s.w_int(&i64);
    s.w_int(&i68);
    s.w_int(&damage_msgs);
    s.w_int(&shot_difficulty);
    s.w_float(&f74);
    s.w_int(&ctl_mode);
    s.w_int(&stick_mode);
    s.w_int(&i38);
    s.w_int(&i2c);
    s.w_int(&kill_msgs);
    s.w_int(&show_messages);
    s.w_int(&i7c);
    s.w_int(&i04);
    s.w_float(&scale_x);
    s.w_float(&scale_y);
    s.w_float(&scale_z);
    s.w_int(&blood);
    if (w.is_open) {
        int r = s.flush();
        if (r == 0)
            fclose(w.fp);
        w.failed = 1;
        w.is_open = 0;
    }
}

// ---- render targets ----------------------------------------------------------------------------
// @ 0x418520
int texture::create()
{
    if (loaded)
        return 0;
    surface = 0;
    level0 = 0;
    used_format = format;
    if (kind == 2) {
        HRESULT hr = g_render.dev->CreateDepthStencilSurface(width, height, (D3DFORMAT)format, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, 0);
        if (hr >= 0)
            goto done;
        int alt = format;
        if (format == D3DFMT_D16)
            alt = D3DFMT_D24S8;
        else if (format == D3DFMT_D24S8)
            alt = D3DFMT_D16;
        else if (format == D3DFMT_R32F)
            alt = D3DFMT_R16F;
        else if (format == D3DFMT_R16F)
            alt = D3DFMT_R32F;
        if (alt != format) {
            hr = g_render.dev->CreateDepthStencilSurface(width, height, (D3DFORMAT)alt, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, 0);
            if (hr >= 0)
                used_format = alt;
        }
        if (hr < 0) {
            loaded = 0;
            if (width <= 0x400)
                return 0;
            if (g_cfg.shadowmap_quality > 0) {
                g_cfg.shadowmap_quality = g_cfg.shadowmap_quality - 1;
                mb_error("Unable to create depth stencil surface width=%d height=%d. Shadowmap quality will be reduced next time you start the game.", width, height);
                FILE* f = mb_open_user_file("rgl_config.txt", "w");
                ((rgl_config&)g_cfg).save(f);
                fclose(f);
                return 0;
            }
            g_options.i2c = 0;
            g_options.save();
            mb_error("Unable to create depth stencil surface width=%d height=%d. Environment shadows will be turned off next time you start the game.", width, height);
            return 0;
        }
    } else {
        if (kind != 1)
            return 0;
        if (d3d_tex == 0) {
            owns_texture = 1;
            d3d_tex = (IDirect3DTexture9**)operator new[](4);
        }
        HRESULT hr;
        if (format == D3DFMT_D24S8 || format == D3DFMT_D24X8 || format == D3DFMT_D16)
            hr = D3DXCreateTexture(g_render.dev, width, height, 1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)format, D3DPOOL_DEFAULT, d3d_tex);
        else
            hr = D3DXCreateTexture(g_render.dev, width, height, mipmaps == 0, (mipmaps != 0 ? D3DUSAGE_AUTOGENMIPMAP : 0) + D3DUSAGE_RENDERTARGET, (D3DFORMAT)format, D3DPOOL_DEFAULT, d3d_tex);
        if (hr < 0) {
            loaded = 0;
            if (width <= 0x400) {
                g_options.i28 = 0;
                g_options.save();
                return 0;
            }
            if (g_cfg.shadowmap_quality > 0) {
                g_cfg.shadowmap_quality = g_cfg.shadowmap_quality - 1;
                mb_error("Unable to create render target texture width=%d height=%d. Shadowmap quality will be reduced next time you start the game.", width, height);
                FILE* f = mb_open_user_file("rgl_config.txt", "w");
                ((rgl_config&)g_cfg).save(f);
                fclose(f);
                return 0;
            }
            g_options.i2c = 0;
            g_options.save();
            mb_error("Unable to create render target texture width=%d height=%d. Environment shadows will be turned off next time you start the game.", width, height);
            return 0;
        }
        (*d3d_tex)->GetSurfaceLevel(0, &level0);
        f4c = 0;
    }
done:
    loaded = 1;
    return 0;
}

// ---- resource manager ----------------------------------------------------------------------------
// The engine's "find by name" idiom: the found pointer is tested before the end of the list.
// @ 0x418870
texture* resmgr::get_texture(const rgl::string& name)
{
    texture* r = 0;
    std::vector<texture*>::iterator it;
    rgl::string t = name + ".dds";
    it = textures.begin();
    while (r == 0 && it != textures.end()) {
        if ((*it)->name.equals(t))
            r = *it;
        ++it;
    }
    if (r == 0) {
        t = name;
        it = textures.begin();
        while (r == 0 && it != textures.end()) {
            if ((*it)->name.equals(t))
                r = *it;
            ++it;
        }
        if (r == 0) {
            t = name + ".jpg";
            it = textures.begin();
            while (r == 0 && it != textures.end()) {
                if ((*it)->name.equals(t))
                    r = *it;
                ++it;
            }
            if (r == 0) {
                t = name + ".tga";
                it = textures.begin();
                while (r == 0 && it != textures.end()) {
                    if ((*it)->name.equals(t))
                        r = *it;
                    ++it;
                }
                if (r == 0)
                    mb_error("get_object failed for texture: %s", name.c_str());
            }
        }
    }
    return r;
}

// @ 0x418a70
skin* resmgr::get_material(const rgl::string& name)
{
    skin* r = 0;
    std::vector<skin*>::iterator it = materials.begin();
    while (r == 0 && it != materials.end()) {
        if ((*it)->name.equals(name))
            r = *it;
        ++it;
    }
    if (r == 0) {
        stream_base& log = g_log;
        log.w_cstr("WARNING: Unable to find material ").w_tstr(&name).w_cstr("\n");
        if (name.differs("default")) {
            rgl::string d("default");
            return get_material(d);
        }
        return *materials.begin();
    }
    return r;
}

// @ 0x418b30
material* resmgr::get_shader(const rgl::string& name)
{
    material* r = 0;
    std::vector<material*>::iterator it = shaders.begin();
    while (r == 0 && it != shaders.end()) {
        if ((*it)->name.equals(name))
            r = *it;
        ++it;
    }
    if (r == 0)
        mb_error("get_shader failed for: %s", name.c_str());
    return r;
}

// @ 0x418b90
effect_set* resmgr::find_effect(const rgl::string& name)
{
    effect_set* r = 0;
    std::vector<effect_set*>::iterator it = effects.begin();
    while (r == 0 && it != effects.end()) {
        if ((*it)->name.equals(name))
            r = *it;
        ++it;
    }
    return r;
}

// @ 0x418bd0
rgl::mesh* resmgr::find_mesh(const rgl::string& name)
{
    bool found = false;
    rgl::mesh* r = 0;
    std::vector<rgl::mesh*>::iterator it = meshes.begin();
    while (!found && it != meshes.end()) {
        if ((*it)->name.equals(name)) {
            r = *it;
            found = true;
        }
        ++it;
    }
    return r;
}

// @ 0x418c10
named_object* resmgr::get_object60(const rgl::string& name)
{
    bool found = false;
    named_object* r = 0;
    std::vector<body_part*>::iterator it = bodies.begin();
    while (!found && it != bodies.end()) {
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
named_object* resmgr::get_skeleton_anim(const rgl::string& name)
{
    bool found = false;
    named_object* r = 0;
    std::vector<skel_anim*>::iterator it = anims.begin();
    while (!found && it != anims.end()) {
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
named_object* resmgr::get_object70(const rgl::string& name)
{
    bool found = false;
    named_object* r = 0;
    std::vector<frame_set*>::iterator it = skeletons.begin();
    while (!found && it != skeletons.end()) {
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
int resmgr::load_textures()
{
    for (std::vector<texture*>::iterator it = textures.begin(); it != textures.end(); ++it) {
        if ((*it)->kind == 0)
            (*it)->load(1, &root);
    }
    return 0;
}

// @ 0x418da0
int resmgr::reload_textures()
{
    for (std::vector<texture*>::iterator it = textures.begin(); it != textures.end(); ++it) {
        if ((*it)->loaded && ((*it)->kind == 1 || (*it)->kind == 2))
            (*it)->unload();
    }
    for (std::vector<effect_set*>::iterator it = effects.begin(); it != effects.end(); ++it) {
        effect_set* e = *it;
        for (int i = 0; i < 4; i++)
            e->h[i] = 0;
        e->resolved = 0;
    }
    return 0;
}

// @ 0x418e10
int resmgr::unload_textures()
{
    for (std::vector<texture*>::iterator it = textures.begin(); it != textures.end(); ++it) {
        if ((*it)->kind == 0)
            (*it)->unload();
    }
    return 0;
}

// @ 0x418e40
void resmgr::release_pixel_shaders()
{
    for (std::vector<material*>::iterator it = shaders.begin(); it != shaders.end(); ++it) {
        material* m = *it;
        if ((m->flags20 & 0x10000000) && g_use_shaders == 0 && m->ps)
            m->ps->Release();
    }
}

// @ 0x418e90
// Picks the best usable shader: the named one if the hardware supports it, else the first supported alternative,
// else the recursively resolved alternatives; falls back to the default shader.
material* resmgr::resolve_shader(const rgl::string& name)
{
    material* best;
    material* shader = get_shader(name);
    if (g_use_shaders) {
        rgl::string s("def_shader");
        best = get_shader(s);
    } else {
        rgl::string s("def_shader_ffp");
        best = get_shader(s);
    }
    if (best == 0)
        mb_error("Unable to Find default shader!");
    if (shader->supported())
        return shader;
    bool found = false;
    {
        int i = 0;
        while (i < shader->nalt) {
            material* m = get_shader(shader->alts[i]);
            if (m->supported()) {
                found = true;
                best = m;
            }
            i++;
            if (found)
                break;
        }
    }
    if (!found) {
        int i = 0;
        while (i < shader->nalt) {
            material* m = resolve_shader(shader->alts[i]);
            if (m != best) {
                best = m;
                found = true;
            }
            i++;
            if (found)
                break;
        }
    }
    return best;
}

// @ 0x418fb0
void resmgr::reset_effect_sets()
{
    if (g_use_shaders) {
        for (std::vector<effect_set*>::iterator it = effects.begin(); it != effects.end(); ++it) {
            effect_set* e = *it;
            for (int i = 0; i < 4; i++)
                e->h[i] = 0;
            e->resolved = 0;
        }
    }
}

// @ 0x418ff0
void resmgr::resolve_effect_sets(ID3DXEffect* fx)
{
    if (g_use_shaders) {
        for (std::vector<effect_set*>::iterator it = effects.begin(); it != effects.end(); ++it)
            (*it)->resolve(fx);
    }
}

// ---- serialisers ----------------------------------------------------------------------------------
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

// reads x, y, z of a vector4 (the 4th lane is left alone)
static __forceinline stream_base& r_vec3(stream_base& s, rgl::vector4* v) { return s.r_float(&v->x).r_float(&v->y).r_float(&v->z); }

// @ 0x4190d0
void skin::load(stream_base* s)
{
    rgl::string tmp;
    s->r_tstr(&name);
    s->r_uint(&flags);
    s->r_tstr(&shader_name);
    for (int i = 0; i < 5; i++) {
        s->r_tstr(&tmp);
        if (tmp.equals("none"))
            tex[i] = 0;
        else
            tex[i] = g_resmgr.get_texture(tmp);
    }
    r_vec3(s->r_float(&f44), &color);
}
