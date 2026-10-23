// resource_io.cpp (0x415900..0x417010): the text stream's read slots (the part of the engine's text serialiser
// that follows text_stream.cpp), matrix and skeleton serialisers, the noise hash, the rgl_config.txt writer,
// texture loading (D3DX), shader (`material`) and material (`skin`) records, and the bounding box.
// Migrated from slices s00415320 (0x415900..0x415c10), s00415c80 (0x415c80..0x416490), s00416540
// (0x416490..0x416e40: w_skeleton is s00415c80's) and s00416ef0 (aabb::extend/update).
// Boundaries: 0x415900 is hard (text_stream::r_tstr 0x415880 calls expect_token 0x415900 forward). The end
// 0x417010 is where mem_stream.cpp starts. No forward call inside the range proves a boundary:
// r_int (0x415930) -> mb_parse_int (0x4159a0), material::load -> stage_t::load (0x4169a0 -> 0x416ad0) and
// material::save -> stage_t::save are callees emitted after their first caller (header-inline functions).
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <new>
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../stream.h"
#include "../resource.h"
#include "../resource_manager.h"
#include "../particles/mesh_obj.h"
#include "../text_stream.h"
#include "../noise.h"
#include "./resource_io.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"

// 0x54a85c "Unexpected End of File while reading file: %s"
#define EXPECT_TOKEN() \
    do { if (!buf.has_token()) mb_error("Unexpected End of File while reading file: %s", name.c_str()); } while (0)

// ---- text stream: read slots -----------------------------------------------------------------------
// @ 0x415900
void text_stream::expect_token() { EXPECT_TOKEN(); }

// @ 0x415930
stream_base& text_stream::r_int(int* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    *v = mb_parse_int(tok);
    return *this;
}

// @ 0x4159a0
int mb_parse_int(const char* s)
{
    bool neg = false;
    int r = 0;
    int i = 0;
    if (s[0] > 0) {
        do {
            if (i >= 15)
                break;
            if (i == 0 && s[0] == '-')
                neg = true;
            else
                r = r * 10 + s[i] - '0';
            i++;
        } while (s[i] > 0);
        if (neg)
            r = -r;
    }
    return r;
}

// @ 0x4159f0
stream_base& text_stream::r_byte(char* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    *v = (char)atoi(tok);
    return *this;
}

// @ 0x415a60
stream_base& text_stream::r_i64(__int64* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    bool neg = false;
    int start = 0;
    if (tok[0] == '-') {
        neg = true;
        start = 1;
    }
    __int64 mult = 1;
    *v = 0;
    for (int i = toklen - 1; i >= start; i--) {
        *v += (__int64)(tok[i] - '0') * mult;
        mult *= 10;
    }
    if (neg)
        *v = -*v;
    return *this;
}

// @ 0x415b60
stream_base& text_stream::r_u64(unsigned __int64* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    *v = 0;
    __int64 mult = 1;
    for (int i = toklen - 1; i >= 0; i--) {
        *v += (__int64)(tok[i] - '0') * mult;
        mult *= 10;
    }
    return *this;
}

// @ 0x415c10
stream_base& text_stream::r_bool(char* v)
{
    char tok[8192];
    int toklen;
    EXPECT_TOKEN();
    buf.get_token(tok, &toklen);
    *v = atoi(tok) != 0;
    return *this;
}

// @ 0x415c80
stream_base& text_stream::r_float(float* v)
{
    char tok[0x2000];
    int len;
    EXPECT_TOKEN();
    buf.get_token(tok, &len);
    *v = (float)atof(tok);
    return *this;
}

// @ 0x415cf0
// Reads a length, then that many raw bytes of this stream's buffer into o's buffer.
stream_base& text_stream::r_stream(stream_base* o)
{
    int n;
    r_int(&n);
    o->reset();
    if (n) {
        for (int i = 0; i < n; i++) {
            char c;
            buf.read(&c, 1);
            o->buf.push_back(c);
        }
    }
    return *this;
}

// rgl::string::read_from (0x4151e0) is a header-inline function: this object inlines its body where
// text_stream.cpp holds the out-of-line copy. rgl_string.h declares it out of line, so the body is repeated.
static inline void read_from_inl(rgl::string& s, FILE* f, size_t n)
{
    s.reserve(n + 1);
    size_t got = fread(s.buf, 1, n, f);
    s.buf[got] = 0;
    s.len = got;
}

// @ 0x415d60
// Like open_read, but opens in text mode and takes the byte count fread returns.
bool text_stream::open_read_text(const char* path)
{
    name.assign(path);
    fp = fopen(path, "r");
    if (fp)
        failed = 0;
    else
        failed = 1;
    if (!failed) {
        size_t n = mb_file_size(fp);
        read_from_inl(buf, fp, n);
        fclose(fp);
    }
    return !failed;
}

// ---- matrix serialisers ----------------------------------------------------------------------------
// three floats of one row, through a pointer
static inline void w3(stream_base& s, float* p) { s.w_float(&p[0]).w_float(&p[1]).w_float(&p[2]); }
static inline void r3(stream_base& s, float* p) { s.r_float(&p[0]).r_float(&p[1]).r_float(&p[2]); }

// @ 0x415df0
// Writes the 3x3 rotation part (rows s, f, u as 3 floats each).
stream_base& w_rotation(stream_base& s, rgl::matrix* m)
{
    w3(s, &m->s.x);
    w3(s, &m->f.x);
    w3(s, &m->u.x);
    return s;
}

// @ 0x415e70
stream_base& r_rotation(stream_base& s, rgl::matrix* m)
{
    r3(s, &m->s.x);
    r3(s, &m->f.x);
    r3(s, &m->u.x);
    return s;
}

// @ 0x415ef0
stream_base& w_matrix(stream_base& s, rgl::matrix* m)
{
    w3(w_rotation(s, m), &m->o.x);
    return s;
}

// @ 0x415f30
stream_base& r_matrix(stream_base& s, rgl::matrix* m)
{
    r3(r_rotation(s, m), &m->o.x);
    return s;
}

// @ 0x415f70
// Hash of an int via the 0x5a8160 permutation table: one lookup per byte of (x + 0x4ae3).
unsigned noise_hash(int x)
{
    unsigned a = x + 0x4ae3;
    unsigned b = g_noise_perm[(a >> 8) & 0xff];
    return (((b << 8 | b) << 8) | b) << 8 | g_noise_perm[a & 0xff];
}

// ---- rgl_config.txt ---------------------------------------------------------------------------------
// Writes one "name = value" line (0x54a904 "%s = %d\n", 0x54a900 "%s\n").
#define WRITE_OPT(f, name, val) \
    do { _snprintf(line, 0x100, "%s = %d\n", name, val); fprintf(f, "%s\n", line); } while (0)

// @ 0x415fb0
// The option block is 0x1b ints at g_rgl_config (0x5a9f98); resource_manager.h's rgl_config has no fields.
void rgl_config::save(FILE* f)
{
    char line[0x100];
    int* c = (int*)this;
    WRITE_OPT(f, "first_time", c[0]);
    WRITE_OPT(f, "texture_detail", c[1]);
    WRITE_OPT(f, "render_buffer_size", c[18]);
    WRITE_OPT(f, "max_framerate", c[3]);
    WRITE_OPT(f, "start_windowed", (c[4] != 0));
    WRITE_OPT(f, "use_pixel_shaders", (c[14] != 0));
    WRITE_OPT(f, "use_vertex_shaders", (c[13] != 0));
    WRITE_OPT(f, "fake_reflections", (c[11] != 0));
    WRITE_OPT(f, "show_framerate", (c[2] != 0));
    WRITE_OPT(f, "disable_music", (c[15] != 0));
    WRITE_OPT(f, "disable_sound", (c[16] != 0));
    WRITE_OPT(f, "disable_frequency_variation", (c[12] != 0));
    WRITE_OPT(f, "cheat_mode", (c[9] != 0));
    WRITE_OPT(f, "enable_blood", (c[10] != 0));
    WRITE_OPT(f, "enable_edit_mode", (c[6] != 0));
    WRITE_OPT(f, "force_single_threading", (c[7] != 0));
    WRITE_OPT(f, "debug_mode", (c[8] != 0));
    WRITE_OPT(f, "display_width", c[19]);
    WRITE_OPT(f, "display_height", c[20]);
    WRITE_OPT(f, "display_bits", c[21]);
    WRITE_OPT(f, "antialiasing", c[22]);
    WRITE_OPT(f, "sample_quality", c[23]);
    WRITE_OPT(f, "force_vsync", c[24]);
    WRITE_OPT(f, "shadowmap_quality", c[26]);
}

// @ 0x416490
stream_base& w_skeleton(stream_base& s, skeleton* k)
{
    s.w_tstr(&k->name);
    s.w_int(&k->nbones);
    for (int i = 0; i < k->nbones; i++) {
        Bone* b = &k->bones[i];
        s.w_int(&b->parent);
        s.w_tstr(&b->name);
        s.w_uint(&b->flags);
        w_matrix(s, &b->local);
    }
    return s;
}

// ---- textures -----------------------------------------------------------------------------------------
// @ 0x416540
int texture::load(int check_exists, rgl::string* root)
{
    unsigned fl = flags;
    int hr = 0;
    if ((fl & 4) && !g_use_shaders)
        return 0;
    if (!loaded) {
        if (!kind) {
            int skip = 0;
            if (g_texture_detail >= 25) {
                int h = noise_hash(f1c);
                if ((float)(h & 0xf) * 0.0625f < (100.0f - (float)g_texture_detail) * 0.013333334f)
                    skip = 1;
            } else {
                skip = 1;
                int h = noise_hash(f1c);
                if ((float)(h & 0xf) * 0.0625f < (25.0f - (float)g_texture_detail) * 0.04f)
                    skip = 2;
            }
            if (fl & 2)
                skip = 0;
            owns_texture = 1;
            bool multi = (fl & 0xf000000) != 0;
            if (multi) {
                owns_texture = ((int)fl >> 24) & 0xf;
                owns_texture *= 4;
            }
            d3d_tex = new IDirect3DTexture9*[owns_texture];
            for (int i = 0; i < owns_texture; i++) {
                char path[1024];
                char full[1024];
                WIN32_FIND_DATAA fd;
                if (multi) {
                    sprintf(path, "Textures\\%s_%d.dds", name.c_str(), i);
                    sprintf(full, "%sTextures\\%s_%d.dds", root->c_str(), name.c_str(), i);
                } else {
                    sprintf(path, "Textures\\%s", name.c_str());
                    sprintf(full, "%sTextures\\%s", root->c_str(), name.c_str());
                }
                if (check_exists) {
                    HANDLE h = FindFirstFileA(full, &fd);
                    if (h != INVALID_HANDLE_VALUE) {
                        FindClose(h);
                        int k = 0;
                        char c;
                        do {
                            c = full[k];
                            path[k] = c;
                            k++;
                        } while (c != 0);
                    }
                }
                hr = D3DXCreateTextureFromFileExA(g_device, path, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_FROM_FILE,
                        0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER,
                        D3DX_SKIP_DDS_MIP_LEVELS(skip, D3DX_FILTER_BOX), 0, 0, 0, &d3d_tex[i]);
                if (hr < 0)
                    mb_error("Unable to open texture file %s", path);
            }
        }
        loaded = 1;
    }
    return hr;
}

// @ 0x4167c0
int texture::unload()
{
    if (loaded) {
        if (level0) {
            level0->Release();
            level0 = 0;
        }
        for (int i = 0; i < owns_texture; i++)
            d3d_tex[i]->Release();
        if (surface) {
            surface->Release();
            surface = 0;
        }
        loaded = 0;
        f4c = 0;
    }
    if (d3d_tex) {
        delete[] d3d_tex;
        d3d_tex = 0;
        owns_texture = 0;
    }
    return 0;
}

// ---- shaders (`material`) -----------------------------------------------------------------------------
// @ 0x416830
material::material()
{
    ps_done = 0;
    flags20 = 0;
    flags1c = 0;
    nstages = 0;
    nalt = 0;
    for (int i = 0; i < 4; i++) {
        stages[i].d = 0;
        stages[i].b = 1;
        stages[i].c = 1;
        stages[i].a = -1;
    }
    ps = 0;
    fc0 = 0;
    f18 = 0;
}

// @ 0x4168f0
// (the body is the inline ~material() of resource.h; this is the out-of-line copy)
// material::~material() is defined inline in resource.h: it is emitted here because the vtable's slot 0 is the
// deleting destructor in 0x417690 and this copy is the plain one.

// @ 0x416930
bool material::supported()
{
    bool ok = true;
    unsigned shdr = flags20 & 0x20000000;
    if (shdr != 0 && !g_use_shaders)
        ok = false;
    if (shdr == 0 && g_use_shaders)
        ok = false;
    if ((flags1c & 0x1000) && (g_caps.PixelShaderVersion < 0xffff0101 || !g_use_pixel_shaders))
        ok = false;
    if (shdr != 0 && (g_caps.VertexShaderVersion < 0xfffe0101 || !g_use_vertex_shaders))
        ok = false;
    if (flags20 & 0x10000)
        ok = false;
    return ok;
}

// @ 0x4169a0
void material::load(stream_base* s)
{
    rgl::string tmp;
    s->r_tstr(&name);
    s->r_uint(&flags20);
    s->r_uint(&flags1c);
    s->r_tstr(&ps_file);
    s->r_int(&nalt);
    if (nstages > 3)
        mb_error("Number of elternatives exceeds %d for material %s.", 3, name);
    for (int i = 0; i < nalt; i++)
        s->r_tstr(&alts[i]);
    s->r_int(&nstages);
    if (nstages > 4)
        mb_error("Number of stages exceeds %d for material %s.", 4, name);
    for (int i = 0; i < nstages; i++)
        ((stage_io*)&stages[i])->load(s);
}

// @ 0x416ad0
void stage_io::load(stream_base* s)
{
    s->r_int(&a);
    s->r_uint((unsigned*)&b);
    s->r_uint((unsigned*)&c);
    s->r_uint((unsigned*)&d);
}

// @ 0x416b10
void material::save(stream_base* s)
{
    s->w_tstr(&name);
    s->w_uint(&flags20);
    s->w_uint(&flags1c);
    s->w_tstr(&ps_file);
    s->w_int(&nalt);
    for (int i = 0; i < nalt; i++)
        s->w_tstr(&alts[i]);
    s->w_int(&nstages);
    for (int i = 0; i < nstages; i++)
        ((stage_io*)&stages[i])->save(s);
}

// @ 0x416bc0
void stage_io::save(stream_base* s)
{
    s->w_int(&a);
    s->w_uint((unsigned*)&b);
    s->w_uint((unsigned*)&c);
    s->w_uint((unsigned*)&d);
}

// @ 0x416c00
void material_io::create_shaders()
{
    if (!ps_done) {
        if ((flags20 & 0x10000000) && g_use_pixel_shaders && supported()) {
            ID3DXBuffer* code;
            ID3DXBuffer* errors;
            if (D3DXAssembleShaderFromFileA(ps_file.c_str(), 0, 0, 0, &code, &errors) < 0)
                mb_error("Failed to assemble shader %s from file", name.c_str());
            if (g_device->CreatePixelShader((const DWORD*)code->GetBufferPointer(), &ps) < 0)
                mb_error("Failed to create pixel shader %s", name.c_str());
        }
        ps_done = 1;
    }
}

// ---- materials (`skin`) ---------------------------------------------------------------------------------
// @ 0x416cd0
skin::skin()
{
    f5c = -1;
    f60 = -1;
    f64 = 0;
    flags = 0;
    for (int i = 0; i < 5; i++)
        tex[i] = 0;
    color = rgl::vector4(0.0f, 0.0f, 0.0f);
    shader = 0;
    f44 = 0.0f;
    on_render = 0;
}

// @ 0x416d50
void skin::save(stream_base* s)
{
    s->w_tstr(&name);
    s->w_uint(&flags);
    s->w_tstr(&shader_name);
    for (int i = 0; i < 5; i++) {
        if (tex[i] == 0) {
            s->w_cstr("none");
        } else {
            rgl::string t("");
            texture* r = tex[i];
            for (int j = 0; j < r->name.len; j++) {
                int c = r->name.buf[j];
                if ((char)c == '.')
                    break;
                t.push_back(c);
                r = tex[i];
            }
            s->w_tstr(&t);
        }
    }
    w3(s->w_float(&f44), &color.x);
}

// ---- bounding box ------------------------------------------------------------------------------------------
// @ 0x416e40
void rgl::aabb::reset()
{
    min = rgl::vector4(1e8f, 1e8f, 1e8f);
    max = rgl::vector4(-1e8f, -1e8f, -1e8f);
    radius = 0.0f;
    center = rgl::vector4(0.0f, 0.0f, 0.0f);
}

// @ 0x416ef0
void rgl::aabb::extend(const rgl::vector4* p)
{
    if (p->x < min.x)
        min.x = p->x;
    if (p->y < min.y)
        min.y = p->y;
    if (p->z < min.z)
        min.z = p->z;
    if (p->x > max.x)
        max.x = p->x;
    if (p->y > max.y)
        max.y = p->y;
    if (p->z > max.z)
        max.z = p->z;
}

// inline vector helpers the original used (add and scale by a constant)
static inline rgl::vector4 add(const rgl::vector4& a, const rgl::vector4& b)
{
    return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z);
}
static inline rgl::vector4 half(const rgl::vector4& a)
{
    return rgl::vector4(a.x * 0.5f, a.y * 0.5f, a.z * 0.5f);
}

// @ 0x416f70
void rgl::aabb::update()
{
    if (min.x <= max.x) {
        rgl::vector4* c = &center;
        *c = half(add(max, min));
        float dx = max.x - c->x;
        float dy = max.y - c->y;
        float dz = max.z - c->z;
        radius = (float)sqrt(dx * dx + dy * dy + dz * dz);
    }
}
