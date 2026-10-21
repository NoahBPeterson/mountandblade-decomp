// Slice s00463790: colour ramp sampler (0x463790), GameState path helpers (0x463950, 0x4639d0), the wind-noise
// vector (0x463a80) and the ShadowMap manager (render-target creation, light camera fit, frustum setup).
// Names provisional.
#include <string.h>
#include "s00463790.h"

// @ 0x463790
unsigned Ramp::eval(float* t)
{
    float x = *t < 0.99f ? *t : 0.99f;
    x = (float)(n - 1) * x;
    float fl = (float)floor(x);
    int i = (int)(fl + 0.1f);
    float hi[3], lo[3];
    hi[0] = (x - fl) * stops[i + 1].c[0]; hi[1] = (x - fl) * stops[i + 1].c[1]; hi[2] = (x - fl) * stops[i + 1].c[2];
    lo[0] = (1.0f - (x - fl)) * stops[i].c[0]; lo[1] = (1.0f - (x - fl)) * stops[i].c[1]; lo[2] = (1.0f - (x - fl)) * stops[i].c[2];
    vector4 c;
    c.x = lo[0] + hi[0];
    c.y = lo[1] + hi[1];
    c.z = lo[2] + hi[2];
    if (g_flag_5e28fc == 0) {
        c.x = (float)pow(c.x, g_gamma);
        c.y = (float)pow(c.y, g_gamma);
        c.z = (float)pow(c.z, g_gamma);
    }
    c.w = 1.0f;
    return pack_color(&c);
}

// @ 0x4638b0
Block16::Block16() : p(0.0f, 0.5f)
{
    q = Vec2(0.0f, 0.0f);
}

// @ 0x4638f0
bool GameState::agent_low(int idx)
{
    if (idx == cur_agent)
        return agents[idx].f_a0 < g_thr_self;
    return agents[idx].f_a0 < g_thr_other;
}

// @ 0x463950
tstring GameState::scene_path(int idx)
{
    tstring s("SceneObj\\");
    s.append(sceneobjs[idx].name);
    {
        tstring ext(".sco");
        s.append(ext);
    }
    return s;
}

// @ 0x4639d0
const char* GameState::module_path(const char* sub)
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
    vector4 r;
    float in[3], out[3];
    float t = (float)g_ticks_1 * 1e-5f;
    in[0] = t; in[1] = 0.0f; in[2] = 0.0f;
    noise_fractal(96.0f, 0.65f, 4, 0, in, out);
    float a = out[0];
    in[0] = 713.0f; in[1] = t; in[2] = 973.0f;
    noise_fractal(72.0f, 0.63f, 4, 0, in, out);
    float b = out[0];
    in[0] = -125.0f; in[1] = 6337.0f; in[2] = t;
    noise_fractal(24.0f, 0.63f, 4, 0, in, out);
    r.x = a;
    r.y = b;
    r.z = out[0];
    r.w = out[1];
    return r;
}

// @ 0x463b70
void ShadowMap::release_all()
{
    if (surf) {
        surf->Release();
        surf = 0;
    }
    if (target2) {
        g_resmgr.release(target2);
        target2 = 0;
    }
    if (target) {
        g_resmgr.release(target);
        target = 0;
    }
}

// @ 0x463bc0
void ShadowMap::pick_size()
{
    w = 0x400;
    h = 0x400;
    if (g_shadow_quality == 1) {
        w = 0x800;
        h = 0x800;
    } else if (g_shadow_quality == 2) {
        w = 0x1000;
        h = 0x1000;
    }
    if (w > g_max_tex_w || h > g_max_tex_h) {
        w = g_max_tex_w;
        h = g_max_tex_h;
    }
}

// @ 0x463c20
vector4 ShadowMap::to_unit(const vector4& p)
{
    vector4 l = cam->world.transform_point_to_local(p);
    const vector4* mn = &cam->vmin;
    l.x -= mn->x; l.y -= mn->y; l.z -= mn->z;
    LightCam* c = cam;
    l.x /= c->size.x; l.y /= c->size.y; l.z /= c->size.z;
    return l;
}

// @ 0x463cc0
// (the original brackets this body with two inline-asm data markers: jmp +0x10; "WL  ", 12 resp. 13, 0, "WL  ")
void ShadowMap::fit_bounds()
{
    LightCam* c = cam;
    c->center = vector4((c->vmax.x + c->vmin.x) * 0.5f, (c->vmin.y + c->vmax.y) * 0.5f, (c->vmin.z + c->vmax.z) * 0.5f);
    c = cam;
    c->wcenter = vector4((c->world.f.x * c->center.y + c->world.u.x * c->center.z) + c->center.x * c->world.s.x + c->world.o.x,
                         (c->world.s.y * c->center.x + c->world.u.y * c->center.z) + c->world.f.y * c->center.y + c->world.o.y,
                         (c->world.s.z * c->center.x + c->world.u.z * c->center.z) + c->world.f.z * c->center.y + c->world.o.z);
    c = cam;
    c->size = vector4(c->vmax.x - c->vmin.x, c->vmax.y - c->vmin.y, c->vmax.z - c->vmin.z);
}

// @ 0x463e10
void ShadowMap::create_target(Skin** slot)
{
    if (*slot == 0) {
        if (g_use_hw_shadow)
            *slot = g_resmgr.get_shadow_skin(w, h, 0x6f, 0);
        else
            *slot = g_resmgr.get_shadow_skin(w, h, 0x72, 0);
        if (*slot != 0) {
            (*slot)->file.assign("shadow_shader");
            Skin* sk = *slot;
            sk->shader = g_resmgr.resolve_shader(&sk->file);
            (*slot)->id = 0x108;
            (*slot)->refs[0]->f24 = 0x300000;
        }
    }
}

// @ 0x463e90
void ShadowMap::setup()
{
    pick_size();
    w2 = w;
    h2 = h;
    f18 = 0;
    f1c = 0;
    f28 = 0;
    f2c = 1.0f;
    LightCam* c = cam;
    c->light = c->world;
    c = cam;
    c->light.o = c->wcenter;
    c = cam;
    float lx = c->vmin.x - c->center.x;
    float ly = c->vmin.y - c->center.y;
    float lz = c->vmin.z - c->center.z;
    float rx = c->vmax.x - c->center.x;
    float ry = c->vmax.y - c->center.y;
    float rz = c->vmax.z - c->center.z;
    float dz = rz - lz;
    float t = dz * 0.5f + 20.0f;
    vector4 d(t * c->light.u.x, t * c->light.u.y, t * c->light.u.z);
    c->light.o.x += d.x;
    c->light.o.y += d.y;
    c->light.o.z += d.z;
    c = cam;
    frustum* f = &c->fr;
    f->m = c->light;
    f->build_planes();
    f = &cam->fr;
    f->perspective = 0;
    f->left = lx;
    f->right = rx;
    f->top = ry;
    f->bottom = ly;
    f->nearz = 20.0f;
    f->farz = dz + 20.0f;
    f->build_planes();
    fr = cam->fr;
    valid = 1;
}

// @ 0x464070
void ShadowMap::bind_target()
{
    if (g_flag_5e28fc != 0 && target != 0) {
        if (g_use_hw_shadow) {
            cam->target = target2;
            cam->dirty = 1;
            return;
        }
        cam->target = target->refs[0];
        cam->dirty = 1;
    }
}
