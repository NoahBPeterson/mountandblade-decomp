//-----------------------------------------------------------------------------
// File: game_state.cpp  (0x4636d0..0x464200)
//
// Desc: a five-array holder (copy ctor / dtor), a colour-ramp sampler, the game state's path helpers
//       ("SceneObj\\<name>.sco", "Modules\\<module>\\<sub>"), the wind-noise vector and the shadow-map
//       manager (render targets, light-camera fit, frustum setup, system-memory read-back). Functions in
//       ascending address order. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include <string.h>
#include <new>
#include "./game_state.h"
#include "../config/config_block.h"           // Config g_cfg (0x5a9f98): shadowmap_quality
#include "../../include/game_data.h"   // GameData (the game state object 0x5e2930; listed directly for the resolution tools)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../arr_set.h"
#include "../../include/frustum.h"
#include "../../include/mat4.h"
#include "../../include/vec.h"
#include "../shadow_map.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../../include/clock_table.h"
#include "../../include/render_flags.h"

extern "C" void _ReadWriteBarrier(void);   // compiler fence: cl 13.10 ships no <intrin.h> (docs/codegen.md 10)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
ShadowMap g_shadow_map;                             // 0xa70248 (.bss: target, target2, surf, w, h, ... zero until setup())




char g_path_buf[256];                               // 0x5e2480 (0x4639d0)
float g_ramp_gamma = 0.45454544f;                   // 0x5a9bfc (1 / 2.2: gamma exponent of the software colour path)


using rgl::vector4;
using rgl::matrix;

// @ 0x4636d0
ArrSet::ArrSet(const ArrSet& o)
{
    name = o.name;
    for (int i = 0; i < 5; i++) {
        cnt[i] = o.cnt[i];
        arr[i] = 0;
        if (o.cnt[i] > 0) {
            arr[i] = new int[o.cnt[i]];
            for (int j = 0; j < o.cnt[i]; j++)
                arr[i][j] = o.arr[i][j];
        }
    }
}

// @ 0x463760
ArrSet::~ArrSet()
{
    for (int i = 0; i < 5; i++) {
        if (arr[i])
            delete[] arr[i];
    }
}



// @ 0x463790
unsigned Ramp::eval(float* t)
{
    float x = *t < 0.99f ? *t : 0.99f;
    x = (float)(n - 1) * x;
    float fl = (float)floor(x);
    int i = (int)(fl + 0.1f);
    float frac = x - fl;
    float f = 1.0f - frac;
    // the stops as vectors through the inline operators (float * vector4, then +): hi and lo are vector4 temporaries
    vector4 c = f * *(const vector4*)stops[i].c + frac * *(const vector4*)stops[i + 1].c;
    if (g_use_shaders == 0) {
        c.x = (float)pow(c.x, g_ramp_gamma);
        c.y = (float)pow(c.y, g_ramp_gamma);
        c.z = (float)pow(c.z, g_ramp_gamma);
    }
    c.w = 1.0f;
    return rgl::pack_color(&c);
}

// @ 0x4638b0
Block16::Block16() : p(0.0f, 0.5f)
{
    q = Vec2(0.0f, 0.0f);
}

// @ 0x4638f0
bool GameData::agent_low(int idx)
{
    if (idx == player)
        return troops[idx].hp < g_module_settings.player_wounded_treshold;
    return troops[idx].hp < g_module_settings.hero_wounded_treshold;
}

// @ 0x463950
tstring GameData::scene_path(int idx)
{
    tstring s("SceneObj\\");
    s.append(levels[idx].name10);
    {
        tstring ext(".sco");
        s.append(ext);
    }
    return s;
}

// @ 0x4639d0
const char* GameData::module_path(const char* sub)
{
    tstring s("Modules\\");
    s.append(module);
    {
        tstring bs("\\");
        s.append(bs);
    }
    g_path_buf[0] = 0;
    strcpy(g_path_buf, s.c_str());
    strcat(g_path_buf, sub);
    return g_path_buf;
}

// @ 0x463a80
vector4 __stdcall wind_noise()
{
    float out[3];
    float t = (float)g_ticks[1] * 1e-5f;
    float in1[3] = { t, 0.0f, 0.0f };
    noise_fractal(96.0f, 0.65f, 4, 0, in1, out);
    float a = out[0];
    float in2[3] = { 713.0f, t, 973.0f };
    noise_fractal(72.0f, 0.63f, 4, 0, in2, out);
    float b = out[0];
    float in3[3] = { -125.0f, 6337.0f, t };
    noise_fractal(24.0f, 0.63f, 4, 0, in3, out);
    return vector4(a, b, out[0], out[1]);
}

// The compiler-generated initializer of g_shadow_map is 0x547de0 (the scene's constructor, then the four dwords and cam cleared) and its atexit
// function 0x548490 (the scene's destructor); the table entry is the compiler's own.
ShadowMap::ShadowMap()
{
    target = 0;
    f4 = 0;
    target2 = 0;
    surf = 0;
    cam = 0;
}

ShadowMap::~ShadowMap()
{
}

// @ 0x463b70
void ShadowMap::release_all()
{
    if (surf) {
        surf->Release();
        surf = 0;
    }
    if (target2) {
        ReleaseFn r2;
        r2.clear_f14 = clear_f14;
        (g_resmgr.*r2.as_member)(target2);
        target2 = 0;
    }
    if (target) {
        ReleaseFn r1;
        r1.clear_f14 = clear_f14;
        (g_resmgr.*r1.as_member)(target);
        target = 0;
    }
}

// the device's largest texture, by value
struct TexSize { int x, y; };
static inline TexSize max_texture_size()
{
    TexSize r;
    r.x = g_render.caps.MaxTextureWidth;
    r.y = g_render.caps.MaxTextureHeight;
    return r;
}

// @ 0x463bc0
void ShadowMap::pick_size()
{
    w = 0x400;
    h = 0x400;
    if (g_cfg.shadowmap_quality == 1) {
        w = 0x800;
        h = 0x800;
    } else if (g_cfg.shadowmap_quality == 2) {
        w = 0x1000;
        h = 0x1000;
    }
    // signed compares (jg/jle). Each max_texture_size() in the condition is its own by-value temporary: the field the
    // arm does not read keeps its store (height before the first branch, width in the second arm), which is the
    // original's 8-byte frame with two dead stores.
    if (w > max_texture_size().x || h > max_texture_size().y) {
        TexSize m = max_texture_size();
        w = m.x;
        h = m.y;
    }
}

// @ 0x463c20
vector4 ShadowMap::to_unit(const vector4& p)
{
    vector4 l = cam->world.transform_point_to_local(p);
    vector4 d = l - cam->vmin;
    l.x = d.x / cam->size.x;
    l.y = d.y / cam->size.y;
    l.z = d.z / cam->size.z;
    return l;
}

// @ 0x463cc0
// (the original brackets this body with two inline-asm data markers: jmp +0x10; "WL  ", 12 resp. 13, 0, "WL  ")
void ShadowMap::fit_bounds()
{
    WL_MARKER(12)
    cam->center = (cam->vmin + cam->vmax) * 0.5f;
    cam->wcenter = vector4((cam->world.u.x * cam->center.z + cam->world.f.x * cam->center.y) + cam->center.x * cam->world.s.x + cam->world.o.x,
                           (cam->world.s.y * cam->center.x + cam->world.u.y * cam->center.z) + cam->world.f.y * cam->center.y + cam->world.o.y,
                           (cam->world.s.z * cam->center.x + cam->world.u.z * cam->center.z) + cam->world.f.z * cam->center.y + cam->world.o.z);
    cam->size = cam->vmax - cam->vmin;
    WL_MARKER(13)
}

// @ 0x463e10
void ShadowMap::create_target(ShSkin** slot)
{
    if (*slot == 0) {
        if (g_render.use_hw_shadow)
            *slot = (ShSkin*)g_resmgr.get_shadow_skin(w, h, 0x6f, 0);
        else
            *slot = (ShSkin*)g_resmgr.get_shadow_skin(w, h, 0x72, 0);
        if (*slot != 0) {
            (*slot)->file.assign("shadow_shader");
            ShSkin* sk = *slot;
            sk->shader = (int)g_resmgr.resolve_shader(sk->file);   // 0x418e90
            (*slot)->id = 0x108;
            (*slot)->refs[0]->f24 = 0x300000;
        }
    }
}

// @ 0x463e90
void ShadowMap::setup()
{
    pick_size();
    scn.f8 = w;      // the scene's viewport: x, y, width, height, min z, max z at +0..+0x14 (ShadowMap+0x18..+0x2c)
    scn.fc = h;
    scn.f0 = 0;
    scn.f4 = 0;
    scn.f10 = 0;
    scn.f14 = 1.0f;
    LightCam* c = cam;
    c->light = c->world;
    c = cam;
    const vector4& wc = c->wcenter;
    c->light.o = wc;
    c = cam;
    vector4 lo(c->vmin.x - c->center.x, c->vmin.y - c->center.y, c->vmin.z - c->center.z);
    vector4 hi(c->vmax.x - c->center.x, c->vmax.y - c->center.y, c->vmax.z - c->center.z);
    float dz = hi.z - lo.z;
    float t = dz * 0.5f + 20.0f;
    vector4 d(t * c->light.u.x, t * c->light.u.y, t * c->light.u.z);
    c->light.o.x += d.x;
    c->light.o.y += d.y;
    c->light.o.z += d.z;
    c = cam;
    rgl::frustum* f = &c->fr;
    f->m = c->light;
    f->build_planes();
    f = &cam->fr;
    f->perspective = 0;
    _ReadWriteBarrier();     // pins the farz x87 chain (fld/fadd/fstp) ahead of the l/r/b/t word stores, as in the original
    _ReadWriteBarrier();
    f->nearz = 20.0f;
    f->farz = dz + 20.0f;
    f->left = lo.x;
    f->right = hi.x;
    f->bottom = lo.y;
    f->top = hi.y;
    f->build_planes();
    scn.fr = cam->fr;
    scn.f162 = 1;
}

// @ 0x464070
void ShadowMap::bind_target()
{
    if (g_use_shaders != 0 && target != 0) {
        if (g_render.use_hw_shadow)
            cam->target = target2;
        else
            cam->target = target->refs[0];
        cam->dirty = 1;
    }
}


// @ 0x4640d0
void ShadowMap::sync_readback()
{
    if (surf == 0) {
        D3DFORMAT fmt = D3DFMT_R32F;
        if (g_render.use_hw_shadow)
            fmt = D3DFMT_R16F;
        IDirect3DTexture9* tex = 0;
        HRESULT hr = g_render.dev->CreateTexture(w, h, 1, D3DUSAGE_DYNAMIC, fmt, D3DPOOL_SYSTEMMEM, &tex, 0);
        if (hr < 0) {
            g_options.i7c = 0;
            surf = 0;
        } else {
            tex->GetSurfaceLevel(0, &surf);
        }
        if (tex)
            tex->Release();
        if (surf == 0)
            return;
    }
    if (target != 0) {
        IDirect3DSurface9* src;
        (*target->refs[0]->tex)->GetSurfaceLevel(0, &src);
        g_render.dev->GetRenderTargetData(src, surf);
        src->Release();
    }
}

// @ 0x4641a0
void ShadowMap::read_pixel(int unused, int x, int y, unsigned* a, unsigned* b)
{
    D3DLOCKED_RECT lr;
    RECT r;
    r.left = x;
    r.right = x + 1;
    r.top = y;
    r.bottom = y + 1;
    surf->LockRect(&lr, &r, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK);
    *a = *(unsigned*)lr.pBits;
    *b = *(unsigned*)lr.pBits;
    surf->UnlockRect();
}

