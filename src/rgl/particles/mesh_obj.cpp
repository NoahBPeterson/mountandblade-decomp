// mesh_obj.cpp (0x40e940..0x413b5f): the mesh object (rgl::mesh) with its particle emitters, morph key frames,
// skinning table and scene queue, plus the render-state setters and the std::vector instantiations the linker
// placed in this object. Migrated from slices s0040e060 (0x40e940 only), s0040ea00, s0040f300, s0040f9a0,
// s00410080, s004108b0, s00411150, s00411a90, s00412150, s00412a90 and s00413320 (the last two without
// vector<vertex60>::erase/resize 0x413b60/0x413bb0, which src/rgl/psys.cpp keeps).
// Boundaries: start 0x40e940 (class change, low), end 0x413b60 (src/rgl/psys.cpp starts there; see MIGRATION.md).
#include <windows.h>
#include <d3d9.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <vector>
#define RGL_NAMED_OBJECT_DTOR_OUT_OF_LINE   // ~named_object is 0x4150f0 (tail-called by ~mesh)
#include "../rgl_string.h"
#include "../rgl_math.h"
#include "../stream.h"
#include "../resource.h"
#include "../noise.h"
#include "./render_cache.h"
#include "./mesh_obj.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

using namespace rgl;

// ---- render-state setters (global object 0xa17910) ----------------------------------------------------------------
// @ 0x40e940
void render_cache::set_blend_mode(int mode)
{
    if (mode != blend_mode) {
        blend_mode = mode;
        if (mode == 0) {
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
            return;
        }
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
        if (blend_mode == 1) {
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            return;
        }
        if (blend_mode == 2) {
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        } else {
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
        }
    }
}

// @ 0x40ea00
void render_cache::set_texcoord_gen(int stage, int mode)
{
    if (stage_texgen[stage] != mode) {
        if (mode <= 1) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, mode);
        } else if (mode == 0x10) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_SPHEREMAP);
            stage_texgen[stage] = mode;
            return;
        } else if (mode == 0x20) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);
        } else if (mode == 0x30) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
            stage_texgen[stage] = mode;
            return;
        } else if (mode == 0x40) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
        }
    }
    stage_texgen[stage] = mode;
}

// ---- small helpers --------------------------------------------------------------------------------------------------
// @ 0x40eab0 (shader class from the three top flag bits)
inline int material::get_flags()
{
    unsigned v = flags20;
    int r = 0;
    if (v & 0x20000000) {
        r = 1;
        if (v & 0x40000000)
            r = v & 0x80000000 ? 4 : 2;
        else if (v & 0x80000000)
            r = 3;
    }
    return r;
}

// @ 0x40eaf0 (three zeroed vector4 rows and a zero dword)
aabb::aabb() : radius(0)
{
    min = vector4(0.0f, 0.0f, 0.0f);
    max = vector4(0.0f, 0.0f, 0.0f);
    center = vector4(0.0f, 0.0f, 0.0f);
}

// ---- particle emitter: spawn one particle --------------------------------------------------------------------------
#define RND(k) ((float)(rand() % 15817) * ((k) / 15817.0f))


// @ 0x40ec90
void emitter::spawn(emitter_particle* out)
{
    if (disabled != 0)
        return;
    vector4 r;
    r.x = (RND(1.0f) - 0.5f) * 2.0f;
    r.y = (RND(1.0f) - 0.5f) * 2.0f;
    r.z = (RND(1.0f) - 0.5f) * 2.0f;
    {
        vector4 local;
        local.x = r.x * box[0];
        local.y = r.y * box[1];
        local.z = r.z * box[2];
        *(rot3*)out = *(const rot3*)&world;
        out->pos = static_cast<const frame_ops&>(world).transform_point(local);
    }
    if (flags & psf_emit_at_water_level)
        out->pos.z = water_z;
    r = vmul(r, vel_scale) + vel_base;
    vector4* vel = &out->vel;
    *vel = r;
    if (!(flags & psf_global_emit_dir))
        *vel = static_cast<const frame_ops&>(world).rot(*vel);
    float m = RND(1.0f) + RND(1.0f);
    vel->x = m * vel->x;
    vel->y = m * vel->y;
    vel->z = m * vel->z;
    out->age = 0.0f;
    out->angle = 0.0f;
    if (flags & psf_randomize_rotation) {
        if (!(flags & 0xf00)) {
            float a = RND(6.2831853f);
            ((matrix*)out)->rotate_about_s(&a);
            a = RND(6.2831853f);
            ((matrix*)out)->rotate_about_f(&a);
            a = RND(6.2831853f);
            ((matrix*)out)->rotate_about_u(&a);
        }
        out->angle = RND(6.2831853f);
    }
    out->angvel = (RND(2.0f) - 1.0f) * spin_scale;
    out->scale = 1.0f;
    if (flags & psf_randomize_size) {
        if (RND(1.0f) > 0.5f)
            out->scale = (1.0f - RND(0.4f)) * out->scale;
        else
            out->scale = (RND(0.6f) + 1.0f) * out->scale;
    }
}

// ---- keyframe table lookup ------------------------------------------------------------------------------------------
// Finds the segment [*lo, *hi] of `times` that contains *t and the fraction of the way through it.
// @ 0x40f0d0
void keyframes::find(int* lo, int* hi, float* frac, const float* t)
{
    int n = count;
    if (n <= 0) {
        *lo = -1;
        *hi = -1;
        return;
    }
    int l = 0;
    int h = n - 1;
    if (h - 1 > 0) {
        do {
            int mid = (l + h) >> 1;
            if ((float)times[mid] > *t)
                h = mid;
            else
                l = mid;
        } while (l < h - 1);
    }
    *lo = l;
    *hi = h;
    int k0 = times[*lo];
    *frac = (*t - (float)k0) / (times[h] - k0);
}

// ---- morph key frames and the skinning table --------------------------------------------------------------------

// @ 0x411780
frame_t::frame_t() {}

// @ 0x411c10
void frame_t::copy_from(const frame_t& o)
{
    verts.resize(o.verts.size());
    for (unsigned i = 0; i < o.verts.size(); ++i)
        verts[i] = o.verts[i];
    norms.resize(o.norms.size());
    for (unsigned j = 0; j < o.norms.size(); ++j)
        norms[j] = o.norms[j];
}

// @ 0x4125e0
void keyframes::operator=(const keyframes& o)
{
    count = o.count;
    if (times) {
        delete[] times;
        times = 0;
    }
    if (frames) {
        delete[] frames;
        frames = 0;
    }
    if (o.count) {
        times = new int[count];
        frames = new frame_t[count];
        for (int i = 0; i < count; ++i)
            frames[i].copy_from(o.frames[i]);
        memcpy(times, o.times, count * 4);
    }
}

// Builds the per-vertex skinning table: up to 4 (bone, weight) influences for each point, then the 0x30-byte
// skin_vert records.
// @ 0x411d20
void mesh::build_skin()
{
    float tw[0x10000][4];
    int tb[0x10000][4];
    skin_vert tmp;
    unsigned i;

    for (i = 0; i < v70.size(); ++i) {
        tw[i][0] = 0; tw[i][3] = 0; tw[i][2] = 0; tw[i][1] = 0;
        tb[i][3] = 0; tb[i][2] = 0; tb[i][1] = 0; tb[i][0] = 0;
    }
    for (unsigned bi = 0; bi < v80.size(); ++bi) {
        bone_influence* b = &v80[bi];
        for (unsigned k = 0; k < b->verts.size(); ++k) {
            int vi = b->verts[k].index;
            unsigned s = 0;
            do {
                if (!(tw[vi][s] > 1e-5f)) {
                    if (s < 4) {
                        tb[vi][s] = b->bone;
                        tw[vi][s] = b->verts[k].weight;
                    }
                    break;
                }
                ++s;
            } while (s < 4);
        }
    }
    v60.resize(v50.size(), tmp);
    for (i = 0; i < v50.size(); ++i) {
        float sum = 0;
        for (int k = 0; k < 4; ++k) {
            v60[i].w[k] = tw[v50[i].index][k];
            v60[i].bone[k] = tb[v50[i].index][k];
            sum += v60[i].w[k];
        }
        if (sum < 1.0f)
            v60[i].w[0] += 1.0f - sum;
    }
    for (i = 0; i < v50.size(); ++i) {
        for (int k = 0; k < 4; ++k)
            v60[i].bonef[k] = (float)v60[i].bone[k];
    }
}

// ---- the scene queue -------------------------------------------------------------------------------------------------
// @ 0x412040
void scene_registry::register_scene(scene* e)
{
    if (!e->registered && e->active) {
        if (e->flags & 0x1000) {
            scenes.push_back(e);
            e->registered = true;
            return;
        }
        scenes.resize(scenes.size() + 1, 0);
        int i = (int)scenes.size() - 1;
        while (i > 0) {
            scenes[i] = scenes[i - 1];
            --i;
        }
        scenes[0] = e;
        e->registered = true;
    }
}

// @ 0x412100
void scene_registry::add_mesh(scene* e, mesh* m)
{
    if (!e->registered && e->active)
        register_scene(e);
    idx_weight w;
    w.index = (int)m;
    w.weight = 0;
    e->queue.push_back(w);
}

// ---- emitter update ----------------------------------------------------------------------------------------------------
// Advances every particle by *pdt, culls dead ones (age >= 1), updates the bounding radius and spawns new
// particles from the remaining budget.
// `pv` is a second induction pointer into the current particle (+0x48 = &vel.z) that cl derives itself in the
// original; the in-source pointer reproduces its addressing:
//   pv[-6..-4] = pos.xyz, pv[-2..0] = vel.xyz, pv[2] = age, pv[4] = angle, pv[5] = angvel.
// The in-place helpers (add_to, scale_f) take references: they stop cl forwarding the stored value, as in the
// original, which re-reads vel/pos/angvel from memory after each update.
static inline void add_to(vector4& a, const vector4& b) { a.x = b.x + a.x; a.y = b.y + a.y; a.z = b.z + a.z; }
static inline void scale_f(float& a, const float& k) { a = k * a; }
// @ 0x412150
void emitter::update(const float* pdt, const matrix& frame)
{
    world = frame * local;
    float maxd2 = 0.0f;
    std::vector<emitter_particle>::iterator it;
    float* pv;
    for (it = particles.begin(), pv = (float*)((char*)&*it + 0x48); it != particles.end(); ++it, pv += 25) {
        vector4 g = vmul(accel, accel_scale);
        vector4 gd = vmul(g, *pdt);
        pv[-2] = gd.x + pv[-2];
        pv[-1] = gd.y + pv[-1];
        pv[0] = gd.z + pv[0];
        if (noise_amount > 0.01f) {
            vector4 in = *(vector4*)&pv[-6];
            float w = (float)pow((double)((float)g_noise_table[noise_index] * 1e-5f), 4000.0);
            in.z = w * noise_scale * 0.2f + in.z;
            vector4 out;
            noise_gradient(noise_scale, &in.x, &out.x);
            if (flags & 0x10000)
                out.z = 0.0f;
            float f = noise_amount * *pdt;
            vector4 nd = vmul(out, f);
            pv[-2] = nd.x + pv[-2];
            pv[-1] = nd.y + pv[-1];
            pv[0] = nd.z + pv[0];
        }
        float kd = drag * *pdt;
        float k = 1.0f - kd;
        pv[-2] = k * pv[-2];
        pv[-1] = k * pv[-1];
        pv[0] = k * pv[0];
        vector4 dp = vmul(*pdt, *(vector4*)&pv[-2]);
        add_to(*(vector4*)&pv[-6], dp);
        float sk = 1.0f - spin_drag * *pdt;
        scale_f(pv[5], sk);
        pv[4] = *pdt * pv[5] + pv[4];
        pv[2] = *pdt / lifetime + pv[2];
        float dx = pv[-6] - world.o.x;
        float dy = pv[-5] - world.o.y;
        float dzz = pv[-4] - world.o.z;
        float d2 = dx * dx + (dy * dy + dzz * dzz);
        if (d2 > maxd2)
            maxd2 = d2;
    }
    radius = (float)sqrt(maxd2);

    int i = 0;
    unsigned n = particles.size();
    if (0 < (int)n) {
      do {
        if (particles[i].age >= 1.0f) {
            particles[i] = particles[n - 1];
            --n;
        } else {
            ++i;
        }
      } while (i < (int)n);
    }
    if (n < particles.size())
        particles.erase(particles.begin() + n, particles.end());

    if (spawn_left > 0) {
        float r = ((float)spawn_left + 10.0f) / ((float)spawn_total + 10.0f);
        if (r < 0.0f)
            r = 0.0f;
        else if (r >= 1.0f)
            r = 1.0f;
        float rf = r * f198;
        spawn_accum += rf * *pdt;
        float t = (float)(rand() % 0x3dc9) * 6.3223117e-5f + (float)(rand() % 0x3dc9) * 6.3223117e-5f;
        if (t < spawn_accum) {
            do {
                if (spawn_left <= 0)
                    return;
                spawn_accum -= t;
                --spawn_left;
                emitter_particle np;
                spawn(&np);
                particles.push_back(np);
                t = (float)(rand() % 0x3dc9) * 6.3223117e-5f + (float)(rand() % 0x3dc9) * 6.3223117e-5f;
            } while (t < spawn_accum);
        }
    }
}

// ---- the mesh ------------------------------------------------------------------------------------------------------------
// @ 0x4126a0
void mesh::update_emitters(const float* dt, const matrix& frame)
{
    for (std::vector<psys*>::iterator it = kids.begin(); it != kids.end(); ++it)
        ((emitter*)*it)->update(dt, frame * m_cc);
}

// @ 0x412700
void mesh::ensure_skin()
{
    mesh* p = this;
    while (!p->local_a1)
        p = p->parent;
    if (p->v60.size() < p->v50.size())
        p->build_skin();
}

// @ 0x4127f0
mesh::~mesh()
{
    if (res)
        res->f64--;
    if (p220)
        delete[] p220;
    for (unsigned i = 0; i < kids.size(); ++i) {
        emitter* e = (emitter*)kids[i];
        if (e)
            delete e;
    }
    if (p_c0)
        delete p_c0;
}

// @ 0x412980
void mesh::compute_sortkey()
{
    if (res->flags & 0x10000) {
        sortkey = -1000000000LL;
        return;
    }
    int d = local_a0 ? slots[1].f4 : parent->slots[1].f4;
    __int64 t = (__int64)(slots[0].f4 * 100000000) + (__int64)slots[0].f0 * 400000000LL + (__int64)(d * 200000000);
    unsigned __int64 x = ((res->flags >> 24) - 8) & 0xf;
    if ((res->flags & 0x700) == 0x700 && (color & 0xff000000) != 0xff000000)
        x += 4;
    __int64 u = (__int64)(res->shader->f18 * 1000);
    u += (__int64)(f214 << 20);
    __int64 k = t | (x << 34);
    u += k;
    u += res->f18;
    sortkey = u;
}

// @ 0x412a90
mesh::mesh()
{
    matrix tmp;
    c4 = -1.0f;
    c8 = -1.0f;
    parent = 0;
    local_a0 = 1;
    local_a1 = 1;
    f4c = 0;
    res = 0;
    flags = 0;
    f34 = 0;
    f38 = 0;
    color = (unsigned)-1;
    f40 = -1;
    f44 = 0;
    m_cc = *identity(&tmp);
    m1d4 = *identity(&tmp);
    mask = 0xffff;
    m194 = *identity(&tmp);
    f48 = 0;
    p220 = 0; f224 = 0; f228 = 0;
    p_c0 = 0;
    slots[1].f4 = 0;
    slots[0].f4 = 0;
    f214 = 0x80;
}

// @ 0x412c30
mesh* scene::get_mesh(skin* r)
{
    unsigned found = (unsigned)-1;
    for (unsigned i = 0; i < cache.size(); ) {
        if (vat(cache, i)->res == r) found = i;
        i++;
        if (found != (unsigned)-1) return vat(cache, found);
    }
    unsigned n = cache.size();
    mesh* m = new mesh;
    m->res = r;
    m->flags |= 0xa0000000;
    m->slots[1].f4 = 1;
    m->slots[0].f4 = 1;
    m->f44 = 10000.0f;
    m->f38 = 1;
    vpush(cache, m);
    return vat(cache, n);
}

// @ 0x412ee0
void mesh::ensure_local()
{
    if (!local_a1) {
        v50 = parent->v50;
        v60 = parent->v60;
        v70 = parent->v70;
        v80 = parent->v80;
        kf = parent->kf;
        local_a1 = 1;
    }
}

// @ 0x412f50
int mesh::add_point(const vector4* p)
{
    ensure_local();
    int n = (int)v70.size();
    v70.resize(n + 1);
    v70[n].a = *p;
    v70[n].b = *p;
    return n;
}

// @ 0x411920
int mesh::add_face(int x, int y, int z)
{
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = true;
    }
    int n = (int)v90.size();
    v90.resize(n + 1);
    v90[n].a = x;
    v90[n].b = y;
    v90[n].c = z;
    return n;
}

// @ 0x412fe0
void mesh::append(mesh* other, const matrix* m)
{
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = 1;
    }
    ensure_local();
    int npts = (int)v70.size();
    int nverts = (int)v50.size();
    std::vector<point20>& pts = other->local_a1 ? other->v70 : other->parent->get70();
    std::vector<vertex60>& vs = other->local_a1 ? other->v50 : other->parent->get50();
    vector4 p;
    for (unsigned i = 0; i < pts.size(); i++) {
        const vector4& q = pts[i].b;
        p.x = m->f.x * q.y + m->u.x * q.z + m->s.x * q.x + m->o.x;
        p.y = m->s.y * q.x + m->u.y * q.z + m->f.y * q.y + m->o.y;
        p.z = m->s.z * q.x + m->u.z * q.z + m->f.z * q.y + m->o.z;
        add_point(&p);
    }
    unpack_color(&p, &other->color);
    for (unsigned i = 0; i < vs.size(); i++) {
        const vector4& n = vs[i].normal;
        vertex60 v = vs[i];
        vector4 t;
        t.x = (m->f.x * n.y + m->u.x * n.z) + m->s.x * n.x;
        t.y = (m->f.y * n.y + m->u.y * n.z) + m->s.y * n.x;
        t.z = (m->f.z * n.y + m->u.z * n.z) + m->s.z * n.x;
        v.normal = t;
        v.index += npts;
        vector4 col;
        unpack_color(&col, &v.color);
        col.x *= p.x; col.y *= p.y; col.z *= p.z; col.w *= p.w;
        v.color = pack_color(&col);
        v50.push_back(v);
    }
    std::vector<face20>& fs = other->local_a0 ? other->v90 : other->parent->get90();
    for (unsigned i = 0; i < fs.size(); i++)
        add_face(fs[i].a + nverts, fs[i].b + nverts, fs[i].c + nverts);
    slots[0].f28 = 1;
    slots[1].f28 = 1;
}

// @ 0x413320
void mesh::update_morph()
{
    if (c8 != c4) {
        keyframes& k = local_a1 ? kf : parent->get_kf();
        float t;
        int b, a;
        k.find(&a, &b, &t, &c8);
        if (a >= 0) {
            ensure_local();
            if (t < 0.001f) {
                for (unsigned i = 0; i < v70.size(); i++)
                    v70[i].b = k.frames[a].verts[i];
                for (unsigned i = 0; i < v50.size(); i++)
                    v50[i].normal = k.frames[a].norms[i];
            } else if (t > 0.999f) {
                for (unsigned i = 0; i < v70.size(); i++)
                    v70[i].b = k.frames[b].verts[i];
                for (unsigned i = 0; i < v50.size(); i++)
                    v50[i].normal = k.frames[b].norms[i];
            } else {
                for (unsigned i = 0; i < v70.size(); i++) {
                    float u = 1.0f - t;
                    v70[i].b = vadd(vscale(u, k.frames[a].verts[i]), vscale(t, k.frames[b].verts[i]));
                }
                for (unsigned i = 0; i < v50.size(); i++) {
                    float u = 1.0f - t;
                    v50[i].normal = vadd(vscale(u, k.frames[a].norms[i]), vscale(t, k.frames[b].norms[i]));
                    v50[i].normal.normalize();
                }
            }
            slots[0].f28 = 1;
        }
        c4 = c8;
    }
}

// ---- material setup and draw-budget test --------------------------------------------------------------------------
// @ 0x40f3f0
void mesh::setup_material()
{
    unsigned fl = res->flags;
    unsigned bm = fl & 0x700;
    if (f38) {
        g_render_cache.set_blend_mode(1);
    } else if (bm == 0x700 && (color & 0xff000000) == 0xff000000) {
        if (g_render_cache.blend_mode) {
            g_render_cache.blend_mode = 0;
            g_render_cache.dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
        }
    } else if (bm == 0x700) {
        g_render_cache.set_blend_mode(1);
    } else {
        g_render_cache.set_blend_mode((fl >> 8) & 7);
    }
    material* m = res->shader;
    int i = 0;
    if (0 < m->nstages) {
        do {
            g_render_cache.set_texcoord_gen(i, m->stages[i].d & 0xff);
            if (res->shader->stages[i].d & 0x100) {
                if (g_render_cache.stage_xform[i] != 1) {
                    g_render_cache.dev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
                    g_render_cache.stage_xform[i] = 1;
                }
                g_render_cache.dev->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + i), (const D3DMATRIX*)&m1d4);
            } else if (g_render_cache.stage_xform[i] != 0) {
                g_render_cache.dev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                g_render_cache.stage_xform[i] = 0;
            }
            m = res->shader;
            ++i;
        } while (i < m->nstages);
    }
    if (res && (res->shader->flags20 & 0x10000000)) {
        vector4 c;
        c = unpack_color_ret(&color);
        float* tmp = &c.x;
        g_render_cache.dev->SetPixelShaderConstantF(0, tmp, 1);
        return;
    }
    unsigned col = color;
    if (g_render_cache.tex_factor != col) {
        g_render_cache.tex_factor = col;
        g_render_cache.dev->SetRenderState(D3DRS_TEXTUREFACTOR, col);
    }
}

// @ 0x40f7f0
int mesh::should_batch()
{
    skin* p = res;
    unsigned fl = p->flags;
    if (!(fl & 0x40000)) {
        if (!(fl & 0x80000))
            return 0;
        std::vector<point20>& v = (local_a1 ? v70 : parent->get70());
        if (!(0 < v.size()))
            return 0;
        int n = (int)get70().size();
        if (!(n < g_vert_budget + ((p->shader->flags20 & 0x80000000) ? 0x46 : 0)))
            return 0;
    }
    return 1;
}

// ---- rendering --------------------------------------------------------------------------------------------------------
// @ 0x413750
int mesh::render(scene* sc, const matrix& frame)
{
    if (!(mask & sc->mask)) return 0;
    skin* r = res;
    if (r) {
        slots[0].f0 = r->shader->get_flags();
        unsigned f20 = r->shader->flags20;
        if (f20 & 0x80000000) {
            unsigned n = get60().size();
            std::vector<vertex60>& w = get50();
            if (n < w.size())
                ensure_skin();
        }
        if (res->f5c >= 0 && res->tex[0]->f4c == 0)
            res->on_render(res);
    } else {
        slots[0].f0 = 0;
        string name("default");
        res = g_resmgr.get_material(name);
    }
    update_morph();
    if (should_batch()) {
        std::vector<point20>& pts = (local_a1 ? v70 : parent->get70());
        if (pts.size() > 0) {
            mesh* mm = sc->get_mesh(res);
            mm->append(this, &frame);
            goto done;
        }
    }
    {
        std::vector<face20>& fs = (local_a0 ? v90 : parent->get90());
        if (fs.size() != 0) {
            unsigned f = flags;
            if (f & 0x10000000) slots[0].f4 = 0;
            else if (f & 0x20000000) slots[0].f4 = 1;
            slots[2].f4 = slots[0].f4;
            if (f & 0x40000000) slots[1].f4 = 0;
            else if (f & 0x80000000) slots[1].f4 = 1;
            m194 = frame * m_cc;
            g_scene_registry.add_mesh(sc, this);
        }
    }
done:
    for (std::vector<psys*>::iterator it = kids.begin(); it != kids.end(); ++it)
        ((emitter*)*it)->render_update(sc);
    if (p_c0) {
        vector4 t;
        t.x = m_cc.u.x * p_c0->in.z + m_cc.f.x * p_c0->in.y + p_c0->in.x * m_cc.s.x + m_cc.o.x;
        t.y = m_cc.s.y * p_c0->in.x;
        t.y += m_cc.u.y * p_c0->in.z;
        t.y += m_cc.f.y * p_c0->in.y;
        t.y += m_cc.o.y;
        t.z = m_cc.s.z * p_c0->in.x;
        t.z += m_cc.u.z * p_c0->in.z;
        t.z += m_cc.f.z * p_c0->in.y;
        t.z += m_cc.o.z;
        vector4 u;
        u.x = t.z * frame.u.x + t.y * frame.f.x + t.x * frame.s.x + frame.o.x;
        u.y = t.z * frame.u.y + t.y * frame.f.y + t.x * frame.s.y + frame.o.y;
        u.z = t.z * frame.u.z + t.y * frame.f.z + t.x * frame.s.z + frame.o.z;
        p_c0->out = u;
        vpush((std::vector<void*>&)sc->list, (void*)p_c0);
    }
    return 0;
}

// @ 0x413ac0
void mesh_list::render_all(scene* sc, const matrix& m)
{
    for (std::vector<mesh*>::iterator it = v.begin(); it < v.end(); ++it)
        if (*it) (*it)->render(sc, m);
}

// @ 0x413b00
void lod_mesh::render(scene* sc, const matrix& m)
{
    if ((mask & sc->mask) && cur < count)
        lists[cur].render_all(sc, m * m8c);
}

// ---- std::vector instantiations the linker placed in this object ---------------------------------------------------
// Element types: 8 bytes idx_weight, 0x14 bone_influence, 0x20 point20 (and face20, identical code), 0x30
// skin_vert, 0x60 vertex60, 0x64 emitter_particle. Whole-class instantiations produce the members the callers
// outside this object reference (size/capacity/operator[] ... 0x40f010..0x40fb80); the helpers below are the
// algorithm templates they call. Each // 0x comment names the original address of the instantiation.
template class std::vector<idx_weight>;
template class std::vector<bone_influence>;
template class std::vector<point20>;
template class std::vector<skin_vert>;
template class std::vector<vertex60>;
template class std::vector<emitter_particle>;
template size_t std::vector<int>::size() const;                                         // 0x40f010
template void std::vector<int>::_Insert_n(std::vector<int>::iterator, size_t, const int&);   // 0x410610

namespace std {
// 0x40f180 .. 0x40f210 copy, 0x40f270 .. 0x40f3c0 copy_backward
template vertex60* _Copy_opt<vertex60*, vertex60*>(vertex60*, vertex60*, vertex60*, _Nonscalar_ptr_iterator_tag);
template skin_vert* _Copy_opt<skin_vert*, skin_vert*>(skin_vert*, skin_vert*, skin_vert*, _Nonscalar_ptr_iterator_tag);
template point20* _Copy_opt<point20*, point20*>(point20*, point20*, point20*, _Nonscalar_ptr_iterator_tag);
template idx_weight* _Copy_opt<idx_weight*, idx_weight*>(idx_weight*, idx_weight*, idx_weight*, _Nonscalar_ptr_iterator_tag);
template bone_influence* _Copy_opt<bone_influence*, bone_influence*>(bone_influence*, bone_influence*, bone_influence*, _Nonscalar_ptr_iterator_tag);
template vertex60* _Copy_backward_opt<vertex60*, vertex60*>(vertex60*, vertex60*, vertex60*, _Nonscalar_ptr_iterator_tag);
template skin_vert* _Copy_backward_opt<skin_vert*, skin_vert*>(skin_vert*, skin_vert*, skin_vert*, _Nonscalar_ptr_iterator_tag);
template point20* _Copy_backward_opt<point20*, point20*>(point20*, point20*, point20*, _Nonscalar_ptr_iterator_tag);
template idx_weight* _Copy_backward_opt<idx_weight*, idx_weight*>(idx_weight*, idx_weight*, idx_weight*, _Nonscalar_ptr_iterator_tag);
template emitter_particle* _Copy_backward_opt<emitter_particle*, emitter_particle*>(emitter_particle*, emitter_particle*, emitter_particle*, _Nonscalar_ptr_iterator_tag);
// 0x40f240 .. 0x40f390 fill
template void fill<vertex60*, vertex60>(vertex60*, vertex60*, const vertex60&);
template void fill<skin_vert*, skin_vert>(skin_vert*, skin_vert*, const skin_vert&);
template void fill<point20*, point20>(point20*, point20*, const point20&);
template void fill<emitter_particle*, emitter_particle>(emitter_particle*, emitter_particle*, const emitter_particle&);
// 0x40f8a0
template void _Uninit_fill_n<int*, unsigned int, int, allocator<int> >(int*, unsigned int, const int&, allocator<int>&, _Scalar_ptr_iterator_tag);
// 0x40f630 .. 0x40f780, 0x40faf0 _Uninit_copy
template vertex60* _Uninit_copy<vertex60*, vertex60*, allocator<vertex60> >(vertex60*, vertex60*, vertex60*, allocator<vertex60>&, _Nonscalar_ptr_iterator_tag);
template skin_vert* _Uninit_copy<skin_vert*, skin_vert*, allocator<skin_vert> >(skin_vert*, skin_vert*, skin_vert*, allocator<skin_vert>&, _Nonscalar_ptr_iterator_tag);
template point20* _Uninit_copy<point20*, point20*, allocator<point20> >(point20*, point20*, point20*, allocator<point20>&, _Nonscalar_ptr_iterator_tag);
template emitter_particle* _Uninit_copy<emitter_particle*, emitter_particle*, allocator<emitter_particle> >(emitter_particle*, emitter_particle*, emitter_particle*, allocator<emitter_particle>&, _Nonscalar_ptr_iterator_tag);
template idx_weight* _Uninit_copy<idx_weight*, idx_weight*, allocator<idx_weight> >(idx_weight*, idx_weight*, idx_weight*, allocator<idx_weight>&, _Nonscalar_ptr_iterator_tag);
template bone_influence* _Uninit_copy<bone_influence*, bone_influence*, allocator<bone_influence> >(bone_influence*, bone_influence*, bone_influence*, allocator<bone_influence>&, _Nonscalar_ptr_iterator_tag);
// 0x40f8c0 .. 0x40fa80 _Uninit_fill_n
template void _Uninit_fill_n<vertex60*, unsigned int, vertex60, allocator<vertex60> >(vertex60*, unsigned int, const vertex60&, allocator<vertex60>&, _Nonscalar_ptr_iterator_tag);
template void _Uninit_fill_n<skin_vert*, unsigned int, skin_vert, allocator<skin_vert> >(skin_vert*, unsigned int, const skin_vert&, allocator<skin_vert>&, _Nonscalar_ptr_iterator_tag);
template void _Uninit_fill_n<point20*, unsigned int, point20, allocator<point20> >(point20*, unsigned int, const point20&, allocator<point20>&, _Nonscalar_ptr_iterator_tag);
template void _Uninit_fill_n<idx_weight*, unsigned int, idx_weight, allocator<idx_weight> >(idx_weight*, unsigned int, const idx_weight&, allocator<idx_weight>&, _Nonscalar_ptr_iterator_tag);
template void _Uninit_fill_n<emitter_particle*, unsigned int, emitter_particle, allocator<emitter_particle> >(emitter_particle*, unsigned int, const emitter_particle&, allocator<emitter_particle>&, _Nonscalar_ptr_iterator_tag);
// 0x411b20 _Destroy_range, 0x411b60 / 0x411b80 (try/catch destroys the constructed prefix; catch funclet 0x411bea)
template void _Destroy_range<bone_influence, allocator<bone_influence> >(bone_influence*, bone_influence*, allocator<bone_influence>&);
}
