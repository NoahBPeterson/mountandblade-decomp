// Slice s00418150: options.dat writer, render-target (re)creation with graceful fallback, a vec_array fill and
// the texture-by-name lookup of the resource manager. STL helpers that need an EH frame are in
// s00418150_eh.cpp. Stream layouts come from mbbin.h (copied from s00417850, verified there).
#include <windows.h>
#include <shlobj.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdarg.h>
#include <stdio.h>
#include <algorithm>
#include "mbbin.h"

void mb_snprintf(char* buf, size_t n, const char* fmt, ...);          // 0x415100
void mb_error(const char* fmt, ...);                                  // 0x42cc80 (logs; formats like printf)
FILE* mb_open_user_file(const char* name, const char* mode);          // 0x415160

// The user-data path builder at 0x4bab70 takes its argument in ESI: a static helper of another TU. Reproduced
// here with the same body so cl picks the same register convention for the call.
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

// ---- game options (global at 0x5aa008) --------------------------------------------------------
// Field names provisional. Serialised by options::save() in the order below; floats go through slot 3, ints
// through slot 8.
struct options {
    int   i00, i04;
    float f08, f0c, f10;
    int   i14, i18, i1c, i20, i24;
    int   i28;                 // 0x5aa030: cleared when a render-target texture cannot be created
    int   i2c;                 // 0x5aa034: cleared when the environment shadow map cannot be created
    int   i30, i34, i38, i3c, i40, i44, i48, i4c, i50, i54, i58, i5c, i60, i64, i68, i6c, i70;
    float f74;
    int   i78, i7c;
    float f80, f84, f88;
    int   i8c, i90, i94, i98;
    void save();               // 0x4181c0
};

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
    s.w_float(&f08);
    s.w_float(&f0c);
    s.w_float(&f10);
    s.w_int(&i14);
    s.w_int(&i1c);
    s.w_int(&i20);
    s.w_int(&i24);
    s.w_int(&i28);
    s.w_int(&i30);
    s.w_int(&i00);
    s.w_int(&i34);
    s.w_int(&i48);
    s.w_int(&i50);
    s.w_int(&i98);
    s.w_int(&i44);
    s.w_int(&i4c);
    s.w_int(&i54);
    s.w_int(&i64);
    s.w_int(&i68);
    s.w_int(&i58);
    s.w_int(&i5c);
    s.w_float(&f74);
    s.w_int(&i6c);
    s.w_int(&i70);
    s.w_int(&i38);
    s.w_int(&i2c);
    s.w_int(&i60);
    s.w_int(&i78);
    s.w_int(&i7c);
    s.w_int(&i04);
    s.w_float(&f80);
    s.w_float(&f84);
    s.w_float(&f88);
    s.w_int(&i18);
    if (w.is_open) {
        int r = s.flush();
        if (r == 0)
            fclose(w.fp);
        w.failed = 1;
        w.is_open = 0;
    }
}

// ---- render targets ------------------------------------------------------------------------------
extern IDirect3DDevice9* g_device;             // 0xa17f28
extern int g_shadow_quality;                   // 0x5aa000 (shadowmap quality step, counted down on failure)
extern options g_options;                      // 0x5aa008
struct rgl_config { void save(FILE* f); };     // 0x415fb0 (writes rgl_config.txt)
extern rgl_config g_rgl_config;                // 0x5a9f98

// vtable 0x54aaac (the texture record of s00416540 / s00417850): create() builds the depth-stencil surface (kind 2) or the render
// target texture (kind 1), retrying once with the sibling format and lowering the shadow quality on failure.
struct texture : named_object {
    int f14;
    char loaded;                               // +0x18
    int f1c;
    int kind;                                  // +0x20 0 = file texture, 1 = render-target texture, 2 = depth-stencil surface
    unsigned flags;                            // +0x24
    int owns_texture;                          // +0x28
    int width;                                 // +0x2c
    int height;                                // +0x30
    int mipmaps;                               // +0x34
    int format;                                // +0x38 D3DFORMAT
    int used_format;                           // +0x3c
    IDirect3DTexture9** d3d_tex;               // +0x40
    IDirect3DSurface9* surface;                // +0x44
    IDirect3DSurface9* level0;                 // +0x48
    int f4c;
    int create();                              // 0x418520
};

// @ 0x418520
int texture::create()
{
    if (loaded)
        return 0;
    surface = 0;
    level0 = 0;
    used_format = format;
    if (kind == 2) {
        HRESULT hr = g_device->CreateDepthStencilSurface(width, height, (D3DFORMAT)format, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, 0);
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
            hr = g_device->CreateDepthStencilSurface(width, height, (D3DFORMAT)alt, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, 0);
            if (hr >= 0)
                used_format = alt;
        }
        if (hr < 0) {
            loaded = 0;
            if (width <= 0x400)
                return 0;
            if (g_shadow_quality > 0) {
                g_shadow_quality = g_shadow_quality - 1;
                mb_error("Unable to create depth stencil surface width=%d height=%d. Shadowmap quality will be reduced next time you start the game.", width, height);
                FILE* f = mb_open_user_file("rgl_config.txt", "w");
                g_rgl_config.save(f);
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
            d3d_tex = (IDirect3DTexture9**)operator new(4);
        }
        HRESULT hr;
        if (format == D3DFMT_D24S8 || format == D3DFMT_D24X8 || format == D3DFMT_D16)
            hr = D3DXCreateTexture(g_device, width, height, 1, D3DUSAGE_DEPTHSTENCIL, (D3DFORMAT)format, D3DPOOL_DEFAULT, d3d_tex);
        else
            hr = D3DXCreateTexture(g_device, width, height, mipmaps == 0, (mipmaps != 0 ? D3DUSAGE_AUTOGENMIPMAP : 0) + D3DUSAGE_RENDERTARGET, (D3DFORMAT)format, D3DPOOL_DEFAULT, d3d_tex);
        if (hr < 0) {
            loaded = 0;
            if (width <= 0x400) {
                g_options.i28 = 0;
                g_options.save();
                return 0;
            }
            if (g_shadow_quality > 0) {
                g_shadow_quality = g_shadow_quality - 1;
                mb_error("Unable to create render target texture width=%d height=%d. Shadowmap quality will be reduced next time you start the game.", width, height);
                FILE* f = mb_open_user_file("rgl_config.txt", "w");
                g_rgl_config.save(f);
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

// ---- vec_array fill (std::fill over the 12-byte record of s00417850) ------------------------------
struct vec16 { float x, y, z, w; };
struct vec_array {
    vec16* v;                          // +0
    int* idx;                          // +4
    int n;                             // +8
    void operator=(const vec_array& o);    // 0x417850
};
// @ 0x418840
void fill_vec_arrays(vec_array* first, vec_array* last, const vec_array& val) { std::fill(first, last, val); }

// ---- resource manager (global at 0x5d6fa0) ---------------------------------------------------------
// Eight std::vector-like lists of resource pointers (16 bytes each: allocator, first, last, end) at +0x20, +0x30,
// ... +0x90. Every resource derives from named_object, so its name is at +4.
struct resource : named_object {
    int f14;
    char loaded;                               // +0x18
    int f1c;
    int kind;                                  // +0x20
};
struct res_list {
    int alloc;
    resource** first;
    resource** last;
    resource** end;
};
struct resource_manager {
    int pad[8];
    res_list v20;                              // +0x20
    res_list textures;                         // +0x30
    res_list shaders;                          // +0x40
    res_list materials;                        // +0x50
    res_list v60;                              // +0x60
    res_list v70;                              // +0x70
    res_list v80;                              // +0x80
    res_list v90;                              // +0x90
    resource* get_texture(const tstring& name);    // 0x418870
};

// The engine's "find by name" idiom: the found pointer is tested before the end of the list.
// @ 0x418870
resource* resource_manager::get_texture(const tstring& name)
{
    resource* r = 0;
    resource** it;
    tstring t = name + ".dds";
    it = textures.first;
    while (r == 0 && it != textures.last) {
        if ((*it)->name.equals(t))
            r = *it;
        ++it;
    }
    if (r == 0) {
        t = name;
        it = textures.first;
        while (r == 0 && it != textures.last) {
            if ((*it)->name.equals(t))
                r = *it;
            ++it;
        }
        if (r == 0) {
            t = name + ".jpg";
            it = textures.first;
            while (r == 0 && it != textures.last) {
                if ((*it)->name.equals(t))
                    r = *it;
                ++it;
            }
            if (r == 0) {
                t = name + ".tga";
                it = textures.first;
                while (r == 0 && it != textures.last) {
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

// Equivalence-harness hook: tools/difftest compiles only <id>.cpp; build with /EHa /DEQUIV_EH to pull in the
// EH-frame functions of s00418150_eh.cpp (they need /EHa, so they cannot live in this file's default build).
#ifdef EQUIV_EH
#include "s00418150_eh.cpp"
#endif
