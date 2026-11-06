//-----------------------------------------------------------------------------
// File: terrain_render.cpp  (0x4681b0..0x46e590)
//
// Desc: the second object of terrain_a's range: the _Uninit_copy / _Ucopy / uninit_fill_n instantiations that call
//       alloc24 / alloc52 / alloc100::construct (0x468130..0x4681b0, terrain_a.cpp) as external functions, the
//       texture / shader / skin binders, the troop auto-equip, the scene view setup, the shadow map extension and
//       the vertex / index buffer uploader and draw passes. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./common.h"
#include "../../rgl/options.h"   // g_options
#include "./terrain.h"
#include "./game_state.h"
#include "./misc.h"
#include "../render_batch.h"                // render_batch (draw 0x46d8c0), dev_buffer (upload 0x46d060)
#include "./wind.h"                          // tri_normal (0x4612d0)
#include "../../include/frustum.h"            // rgl::frustum (to_clip 0x4621f0)
#include "../../include/mat4.h"               // pos (identity_pos 0x40cd00)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../../include/item_kind.h"   // g_items 0x5e29d0

extern unsigned __int64 g_bit[];                            // 0x5a8e68 (game_globals.h): g_bit[2] = 4 (size mask), g_bit[4] = 0x10 (type mask)
unsigned g_river_mask_b = (unsigned)g_bit[2];               // 0x5e2584
unsigned g_river_mask_a = (unsigned)g_bit[4];               // 0x5e2580
#include "../mission.h"                                      // Mission, g_mission (0x5e292c)
#include "../../include/render_flags.h"

static inline vector4 p_add(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline int item_f20c(int i) { return *(int*)((char*)&g_items[i] + 0x20c); }

// @ 0x4681b0
elem24* __cdecl uninit_copy24(elem24* f, elem24* l, elem24* d, alloc24& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    elem24* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}

// @ 0x468220
particle100* __cdecl uninit_copy100(particle100* f, particle100* l, particle100* d, alloc100& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    particle100* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}

// @ 0x468290
elem52* __cdecl uninit_copy52(elem52* f, elem52* l, elem52* d, alloc52& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    elem52* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}

// ---- component-wise max / min of two 3-vectors -------------------------------------------------------------------------

static inline float vm_max(float x, float y) { return x > y ? x : y; }
static inline float vm_min(float x, float y) { return x < y ? x : y; }
// @ 0x468300
void __cdecl vmax3(float* a, const float* b)
{
    a[0] = vm_max(a[0], b[0]);
    a[1] = vm_max(a[1], b[1]);
    a[2] = vm_max(a[2], b[2]);
}

// @ 0x468360
void __cdecl vmin3(float* a, const float* b)
{
    a[0] = vm_min(a[0], b[0]);
    a[1] = vm_min(a[1], b[1]);
    a[2] = vm_min(a[2], b[2]);
}

// ---- material texture stages ----------------------------------------------------------------------------------------------

// @ 0x4683c0
void texture_x::bind(int stage, float t)
{
    if (loaded) {
        int idx = 0;
        if (flags & 0x0f000000)
            idx = (int)(owns_texture * t) % owns_texture;
        IDirect3DBaseTexture9* tex = ((IDirect3DBaseTexture9**)d3d_tex)[idx];
        if (g_render.tex_bound[stage] != tex) {
            g_render.dev->SetTexture(stage, tex);
            g_render.tex_bound[stage] = tex;
        }
        g_render.set_stage_sampler(stage, ((int)flags >> 20) & 3);
    }
}

// @ 0x468440
void material_x::bind(texture** textures, const float* t)
{
    int n = nstages;
    int last = -1;
    int k = 0;
    if (0 < n) {
        stage_t* s = stages;
        for (; k < n; ++k, ++s) {
            last = k;
            if (s->a != -1 && textures[s->a])
                ((texture_x*)textures[s->a])->bind(k, *t);
            g_render.set_stage_color(k, s->b);
            g_render.set_stage_alpha(k, s->c);
        }
    }
    g_render.set_stage_color(last + 1, 1);
    g_render.set_stage_alpha(last + 1, 1);
}

// @ 0x4684d0
void skin_x::apply_state(const float* t)
{
    unsigned char a = ~(flags >> 4) & 1;
    if (a != g_render.b54b) {
        g_render.b54b = a;
        if (a)
            g_render.dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        else
            g_render.dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    }
    unsigned char b = (flags >> 3) & 1;
    if (b != g_render.b549) {
        BOOL on = b == 0;
        g_render.b549 = b;
        g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, on);
    }
    g_render.set_alpha_mode((flags >> 12) & 3);
    unsigned char c = (flags >> 11) & 1;
    if (c != g_render.b548) {
        g_render.dev->SetRenderState(D3DRS_NORMALIZENORMALS, c);
        g_render.b548 = c;
    }
    unsigned char d = (shader->flags20 >> 5) & 1;
    if (d != g_render.b547) {
        g_render.dev->SetRenderState(D3DRS_SPECULARENABLE, d);
        g_render.b547 = d;
    }
    if (shader->flags20 & 0x20) {
        vector4 col;
        col.x = color.x * 0.5f;
        col.y = color.y * 0.5f;
        col.z = color.z * 0.5f;
        g_render.set_material(f44 + f44, &col.x);
    }
    ((material_x*)shader)->bind(tex, t);
}

// @ 0x468620
int rgl::mesh::count(int kind)
{
    if (kind == 1) {
        std::vector<rgl::face20>& f = local_a0 ? v90 : parent->get90();
        return f.size() * 3;
    }
    std::vector<rgl::vertex60>& v = local_a1 ? v50 : parent->get50();
    return v.size();
}

// ---- effect slot record ------------------------------------------------------------------------------------------------------

// @ 0x4686a0
void EnvRec::reset()
{
    f3c = -1;
    f8 = -1;
    fc = -1;
    f0 = 0;
    f30 = 0;
    fa4 = 10;
    f9c = 13;
    fa8 = 20;
    fb8 = 0;
    f34 = 1.3f;
    f38 = 4.3f;
    f4c = 0;
    k0 = 0;
    k1 = 0;
    f54 = 0;
    f58 = 0;
    f40 = 1.0f;
    fbc = 1.0f;
    fa0 = 1.0f;
    { int& one = fb4; one = 1; }
    { int& one = f50; one = 1; }
    t0 = g_ticks[clock];
    fac = 0;
    fb0 = 0;
    q.zero();
    f5c = 0;
    q.x = 1.0f;
    k2 = 0;
}

// @ 0x468770
int Rec468770::max_hit_points()
{
    int base = f58;
    int v = b7a & 0xf;
    if (v < 0)
        v = 0;
    else if (v > g_skill_tbl[36].max_rank)
        v = g_skill_tbl[36].max_rank;
    return base + v * 2 + 0x23;
}

// ---- troop auto-equip (built on Troop::add and Troop::equip_best, src/game/ui/troop_inv.cpp) ------------------------------------

// @ 0x4687a0
void Troop::unequip_all()
{
    ItemRef* p = equip;
    int n = 10;
    do {
        if (p->item >= 0) {
            if (add(p))
                p->item = -1;
        }
        ++p;
        --n;
    } while (n != 0);
}

// @ 0x4687e0
void Troop::auto_equip(int seed, int unused, char civilian, char no_helmet)
{
    if ((flags30 & tf_inactive) == 0) {
        bool b7 = false, b8 = false, b1 = false, b2 = false, b5 = false, b6 = false, b3 = false, b4 = false, b9 = false, b10 = false;
        unsigned mustnot_ = 0;
        unequip_all();
        unsigned must_ = 0;
        if ((flags30 & tf_hero) && civilian)
            must_ = itp_civilian;
        if ((flags30 & tf_hero) && no_helmet)
            mustnot_ = 0x80000000;
        equip_best(seed, ek_head, must_, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_helmet) != 0);
        equip_best(seed, ek_body, must_, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_armor) != 0);
        equip_best(seed, ek_foot, must_, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_boots) != 0);
        equip_best(seed, ek_gloves, must_, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_gloves) != 0);
        equip_best(seed, ek_horse, 0, mustnot_, -1, -1, -1, (flags30 & tf_guarantee_horse) != 0);
        for (int i = 0; i < 4; ++i) {
            char r = 0;
            if (i == 0) {
                r = equip_best(seed, 0, 0, mustnot_, itp_type_pistol, itp_type_musket, -1, (flags30 & tf_guarantee_ranged) != 0);
                if (!r)
                    r = equip_best(seed, 0, 0, mustnot_, itp_type_bow, itp_type_crossbow, itp_type_thrown, (flags30 & tf_guarantee_ranged) != 0);
                if (r) {
                    unsigned t = g_items[equip[0].item].type_flags & 0xff;
                    if (t == itp_type_bow) b1 = true;
                    else if (t == itp_type_crossbow) b3 = true;
                    else if (t == itp_type_musket || t == itp_type_pistol) b5 = true;
                }
                if (r) continue;
            }
            if (b1 && !b2) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_arrows, -1, -1, 1);
                if (r) b2 = true;
            }
            if (r) continue;
            if (b3 && !b4) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_bolts, -1, -1, 1);
                if (r) b4 = true;
            }
            if (r) continue;
            if (b5 && !b6) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_bullets, -1, -1, 1);
                if (r) b6 = true;
            }
            if (r) continue;
            if (b7 && !b8) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_shield, -1, -1, (flags30 & tf_guarantee_shield) != 0);
                if (r) b8 = true;
            }
            if (r) continue;
            if (!b9) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_one_handed_wpn, itp_type_two_handed_wpn, -1, 0);
                if (r) {
                    b9 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
            if (r) continue;
            if (!b10) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_polearm, -1, -1, 0);
                if (r) {
                    b10 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
            if (r) continue;
            if (!b9 && !b10) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_one_handed_wpn, itp_type_two_handed_wpn, -1, 1);
                if (r) {
                    b9 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                } else {
                    r = equip_best(seed, i, 0, mustnot_, itp_type_polearm, -1, -1, 1);
                    if (!r) continue;
                    b10 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
        }
    }
}

// ---- shadow-map bounds and depth test ------------------------------------------------------------------------------------------

// @ 0x468c20
void ShadowMap::extend(const vector4* p)
{
    vector4 local = cam->world.transform_point_to_local(*p);
    vmin3(&cam->vmin.x, &local.x);
    vmax3(&cam->vmax.x, &local.x);
}

// @ 0x468c80
bool ShadowMap::test(unsigned p, float* out)
{
    *out = 1.0f;
    bool result = true;
    if (cam) {
        vector4 pt = ((rgl::frustum*)&cam->fr)->to_clip(*(const vector4*)p);   // 0x4621f0 (hidden return pointer = &pt)
        if (!(0.0f <= pt.x && pt.x < 1.0f && 0.0f <= pt.y && pt.y < 1.0f)) return true;
        float scr[2];
        scr[0] = (float)scn.f8 * pt.x;
        scr[1] = (1.0f - pt.y) * (float)scn.fc;
        if (g_use_shaders && surf) {
            // cl overlays these two locals on the dead argument slots
            float depth;
            unsigned dummy;
            read_pixel((int)g_render.dev, (int)scr[0], (int)scr[1], (unsigned*)&depth, &dummy);
            if ((pt.z - 0.001f) > depth)
                *out = 0.0f;
        } else {
            result = false;
        }
    } else {
        result = false;
    }
    return result;
}

// ---- terrain: brush, path walk, flow spreading, ambient particles, normals -------------------------------------------------

// @ 0x468da0
void Terrain::apply_brush(const float* center, float radius, float strength)
{
    float fv = (float)((((unsigned*)g_bit)[14] - 1) & (flags >> 7));
    int lo[2], hi[2];
    const int* lim = &w;
    for (int k = 0; k < 2; ++k) {
        float a = (float)ceil((center[k] - radius) / scale);
        float fl = (float)floor(a);
        int i0 = (int)fl;
        int r = i0;
        if (!(a - fl <= 0.5f)) r = i0 + 1;
        lo[k] = r > 0 ? r : 0;
        float b = (float)floor((center[k] + radius) / scale);
        float fl2 = (float)floor(b);
        int i1 = (int)fl2;
        int r2 = i1;
        if (!(b - fl2 <= 0.5f)) r2 = i1 + 1;
        int h = lim[k];
        if (r2 < h) h = r2;
        hi[k] = h;
    }
    for (int j = lo[1]; j <= hi[1]; ++j) {
        for (int i = lo[0]; i <= hi[0]; ++i) {
            terrain_cell& c = cells[i * 251 + j];
            float dx = (float)i * scale - center[0];
            float dy = (float)j * scale - center[1];
            float d = (1.0f / radius) * (float)sqrt(dy * dy + dx * dx);
            if (d < 1.0f) {
                float v = ((((float)cos(d * 3.1415927f) + 1.0f) * strength) * 0.5f * c.weight * fv) * 0.01f + c.pz;
                c.pz = v;
                c.h2 = v;
            }
        }
    }
}

static __forceinline void vnorm(vec2& v)
{
    float l = (float)sqrt(v.y * v.y + v.x * v.x);
    if (l > 0.0f) {
        l = 1.0f / l;
        v.x = l * v.x;
        v.y = l * v.y;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
    }
}

// @ 0x468fc0
void Terrain::trace_path(const int* from, const int* to, unsigned char bit, int* steps, float* minz, float w1, float w2, int unused, char record)
{
    *minz = 10000.0f;
    ivec2 pos = *(const ivec2*)from;
    *steps = 0;
    vec2 g;
    float gx = (float)(to[0] - pos.dx);
    float gy = (float)(to[1] - pos.dy);
    float gl = (float)sqrt(gx * gx + gy * gy);
    if (gl > 0.0f) {
        gl = 1.0f / gl;
        g.x = gl * gx;
        g.y = gl * gy;
    } else {
        g.x = 0.0f;
        g.y = 1.0f;
    }
    vec2 nd[8];
    for (int k = 0; k < 8; k++) {
        nd[k].x = (float)g_nbr[k].dx;
        nd[k].y = (float)g_nbr[k].dy;
        vnorm(nd[k]);
    }
    unsigned bitmask = 1 << bit;
    terrain_cell* cur = &at(pos.dx, pos.dy);
    cur->flags |= bitmask;
    if (record) {
        cur->dir = g;
        vec2 t;
        t.x = 0.0f;
        t.y = (float)*steps;
        cur->disp = t;
    }
    bool blocked = false;
    for (;;) {
        unsigned fl = flags;
        float tx = (float)(to[0] - pos.dx);
        int w = this->w;
        int h = this->h;
        float ty = (float)(to[1] - pos.dy);
        float sc[8];
        for (int k = 0; k < 8; k++) {
            int nx = g_nbr[k].dx + pos.dx;
            int ny = g_nbr[k].dy + pos.dy;
            sc[k] = 0.0f;
            float tl = (float)sqrt(tx * tx + ty * ty);
            float ux, uy;
            if (tl > 0.0f) {
                tl = 1.0f / tl;
                ux = tl * tx;
                uy = tl * ty;
            } else {
                ux = 0.0f;
                uy = 1.0f;
            }
            int a = ny - h + 20;
            int b = nx - w + 20;
            int pen = (a < 0 ? 0 : a) + (20 - ny < 0 ? 0 : 20 - ny) + (b < 0 ? 0 : b) + (20 - nx < 0 ? 0 : 20 - nx);
            float edge = 0.0f;
            if (pen > 0 &&
                ((nx < 21 && nd[k].x <= 0.0f) || (ny < 21 && nd[k].y <= 0.0f) ||
                 (w - 20 <= nx && 0.0f <= nd[k].x) || (h - 20 <= ny && 0.0f <= nd[k].y))) {
                edge = (float)pen;
                if (edge < 0.0f) edge = 0.0f;
                else if (24.0f <= edge) edge = 24.0f;
                edge = edge * -0.04f;
            }
            if (nx < 0 || w < nx || ny < 0 || h < ny) {
                sc[k] = ((g.x * nd[k].x + g.y * nd[k].y) + 1.0f) * 0.5f;
            } else {
                terrain_cell* n = &at(nx, ny);
                if ((bitmask & n->flags) == 0) {
                    float dz = ((cur->pz - n->pz) * w2) * 3.0f;
                    if (dz < 0.0f)
                        dz = (0.25f - (float)((fl >> 7) & (((unsigned*)g_bit)[14] - 1)) * 0.0022f) * dz;
                    sc[k] = (((((uy * nd[k].y + ux * nd[k].x) + 0.7f) * 0.5f) * 0.65f + dz) +
                             (((g.y * nd[k].y + g.x * nd[k].x) + 0.7f) * 0.5f) * w1) + edge;
                } else {
                    if (n->order < *steps - 30)
                        blocked = true;
                    sc[k] = -1.0f;
                }
            }
        }
#define CUBE(i) if (0.0f < sc[i]) sc[i] = (sc[i] * sc[i]) * sc[i];
        CUBE(0) CUBE(1) CUBE(2) CUBE(3) CUBE(4) CUBE(5) CUBE(6) CUBE(7)
#undef CUBE
        float total = 0.0f;
#define ADD0(i) if (0.0f < sc[i]) total = sc[i];
#define ADD(i) if (0.0f < sc[i]) total = sc[i] + total;
        ADD0(0) ADD(1) ADD(2) ADD(3) ADD(4) ADD(5) ADD(6) ADD(7)
#undef ADD
#undef ADD0
        int r = rand();
        int pick = -1;
        float roll = ((float)(r % 0x3dc9) * total) * 6.322312e-05f;
        float acc = 0.0f;
        if (0.0f < sc[0]) {
            acc = sc[0];
            if (0.0f <= roll && roll < sc[0]) pick = 0;
        }
#define SEL(i) if (0.0f < sc[i]) { if (acc <= roll && roll < sc[i] + acc) pick = i; acc = acc + sc[i]; }
        SEL(1) SEL(2) SEL(3) SEL(4) SEL(5) SEL(6)
#undef SEL
        if (0.0f < sc[7] && acc <= roll && roll < sc[7] + acc)
            pick = 7;
        else if (pick == -1)
            return;
        pos.dx = g_nbr[pick].dx + pos.dx;
        if (pos.dx < 3) return;
        if (this->w - 3 < pos.dx) return;
        pos.dy = g_nbr[pick].dy + pos.dy;
        if (pos.dy < 3) return;
        if (this->h - 3 < pos.dy) return;
        if (sc[pick] < 0.0f) return;
        float mx = g.x * 0.9f + nd[pick].x * 0.1f;
        float my = g.y * 0.9f + nd[pick].y * 0.1f;
        g.y = sqrtf(mx * mx + my * my);
        if (g.y > 0.0f) {
            g.y = 1.0f / g.y;
            g.x = mx * g.y;
            g.y = my * g.y;
        } else {
            g.x = 0.0f;
            g.y = 1.0f;
        }
        (*steps)++;
        cur = &at(pos.dx, pos.dy);
        cur->flags |= bitmask;
        cur->order = *steps;
        if (record) {
            cur->dir = g;
            vec2 t;
            t.x = 0.0f;
            t.y = (float)*steps * 0.7f;
            cur->disp = t;
        }
        if (cur->pz < *minz)
            *minz = cur->pz;
        if (*steps == 100000)
            return;
        if (blocked)
            return;
    }
}

static const int kStride = 251;

// @ 0x469a70
void Terrain::spread_flow(unsigned mask, int mode, unsigned mark)
{
    int order[9];
    unsigned n;
    if (mode == 1) {
        n = 4;
        order[0] = 2; order[1] = 4; order[2] = 5; order[3] = 6;
    } else {
        n = 8;
        for (int i = 0; i < 8; i++) order[i] = i;
    }
    static int seen_dummy;
    int seen[kStride * kStride];
    for (int y = 0; y <= this->h; y++)
        for (int x = 0; x <= this->w; x++)
            seen[x * kStride + y] = 0;
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            terrain_cell& c = at(x, y);
            int cnt = 0;
            vec2 dsum, rsum;
            dsum.x = 0.0f; dsum.y = 0.0f; rsum.x = 0.0f; rsum.y = 0.0f;
            for (unsigned k = 0; k < n; k++) {
                int nx = g_nbr[order[k]].dx + x;
                int ny = g_nbr[order[k]].dy + y;
                if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h) {
                    terrain_cell& d = at(nx, ny);
                    if ((mask & d.flags) != 0) {
                        cnt++;
                        float e[2], r[2], q[2];
                        e[0] = c.px - d.px; e[1] = c.py - d.py;
                        e[0] *= 0.3f; e[1] *= 0.3f;
                        r[0] = e[0] * d.dir.y - e[1] * d.dir.x;
                        r[1] = e[1] * d.dir.y + e[0] * d.dir.x;
                        q[0] = r[0] + d.disp.x;
                        q[1] = r[1] + d.disp.y;
                        dsum.x = q[0] + dsum.x;
                        dsum.y = q[1] + dsum.y;
                        rsum.x = rsum.x + d.dir.x;
                        rsum.y = rsum.y + d.dir.y;
                    }
                }
            }
            if (cnt > 0 || (mark != 0 && cnt != 0)) {
                seen[x * kStride + y] = 1;
                float inv = 1.0f / (float)cnt;
                vec2 nd1, nd2;
                nd1.x = dsum.x * inv; nd1.y = dsum.y * inv;
                c.disp = nd1;
                nd2.x = rsum.x * inv; nd2.y = rsum.y * inv;
                c.dir = nd2;
                float l = (float)sqrt(c.dir.y * c.dir.y + c.dir.x * c.dir.x);
                if (l > 0.0f) {
                    l = 1.0f / l;
                    c.dir.x = l * c.dir.x;
                    c.dir.y = l * c.dir.y;
                } else {
                    c.dir.x = 0.0f;
                    c.dir.y = 1.0f;
                }
            }
        }
    }
    if (mark != 0) {
        for (int y = 0; y <= this->h; y++)
            for (int x = 0; x <= this->w; x++)
                if (seen[x * kStride + y] != 0)
                    at(x, y).flags |= mark;
    }
}

#include "../../include/mat4.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../arr_set.h"
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../shadow_map.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../terrain_desc.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/view_params.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../view_state.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/renderer.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
using rgl::mat3;
static __forceinline rgl::vector4 t_add(const rgl::vector4& a, const rgl::vector4& b) { return rgl::vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline rgl::vector4 t_cross(const rgl::vector4& a, const rgl::vector4& b)
{
    return rgl::vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static __forceinline void t_normalize(rgl::vector4& v)
{
    float len = (float)sqrt(v.z * v.z + v.y * v.y + v.x * v.x);   // 0x46aa3e: z, y, x order
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv; v.y = v.y * inv; v.z = v.z * inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
}
#define RND() ((float)(rand() % 0x3dc9) * 6.322312e-05f)

// @ 0x46a270
void Terrain::spawn_particles(particle_list* list, int unused)
{
    list->count = 0;
    unsigned lv = (flags >> 0x15) & (((unsigned*)g_bit)[14] - 1);
    float lv_scale = (float)lv * 0.01f;
    unsigned sel = (flags14 >> 2) & (((unsigned*)g_bit)[4] - 1);
    int maxn = (int)(scale * scale * 0.8f);
    float k1 = (float)pow(scale, 1.7f) * 0.3f;
    float k2 = (float)g_options.i34 * 0.02f;
    for (int y = 0; y < this->h; y++) {
        for (int x = 0; x < this->w; x++) {
            if (!(at(x, y).pz > -0.3f && at(x + 1, y).pz > -0.3f && at(x, y + 1).pz > -0.3f && at(x + 1, y + 1).pz > -0.3f))
                continue;
            terrain_quad& q = quad(x, y);
            for (int t = 0; t < 2; t++) {
                q.slot[t] = -1;
                if (list->nsrc != 0 && (q.tri[t] == 2 || q.tri[t] == 1) && list->count < 0x40000) {
                    float fx = (float)x;
                    float fy = (float)y;
                    for (int j = 0; j < list->nsrc; j++) {
                        scatter_src& s = list->src[j];
                        int kind = s.kind;
                        float p[3];
                        p[0] = fx + s.v.x;
                        p[1] = fy + s.v.y;
                        p[2] = s.v.z;
                        float nz[3];
                        noise_fractal(s.freq, 0.6f, 3, 0, p, nz);
                        float v = (nz[2] + nz[1]) + nz[0];
                        if (!(v < 0.5f)) v = 0.5f;
                        ptype* pt = &g_ptypes[kind];
                        float rate = ((v - s.thresh) * s.rate) * k1;
                        if ((pt->flags & 0x80000) == 0) {
                            rate = rate * lv_scale;
                        } else {
                            rate = rate * k2;
                            if (sel != 0) rate = 0.0f;
                        }
                        int n = 0;
                        if (RND() < rate) {
                            do {
                                if (maxn <= n) break;
                                if (!(RND() < q.f34) && (pt->flags & 0x80000) != 0) break;
                                rate = rate - 1.0f;
                                TerrainParticle* e = &list->items[list->count];
                                e->type = kind;
                                e->cx = x;
                                e->cy = y;
                                e->nmesh = 0;
                                e->f0c = t;
                                n++;
                                int a = 0;
                                if (1 < pt->nframes)
                                    a = rand() % pt->nframes;
                                e->anim = a;
                                mat3 id;
                                rgl::matrix m;
                                {
                                    mat3* ip = identity3(&id);
                                    memcpy(&m, ip, 0x30);
                                }
                                int cr[6];
                                int* ca = &cr[0]; int* cb = &cr[2]; int* cc = &cr[4];
                                triangle_corners(x, y, t, ca, cb, cc);
                                rand(); rand(); rand();
                                const rgl::vector4& PA = (const rgl::vector4&)at(ca[0], ca[1]);
                                const rgl::vector4& PB = (const rgl::vector4&)at(cb[0], cb[1]);
                                const rgl::vector4& PC = (const rgl::vector4&)at(cc[0], cc[1]);
                                m.o = t_add(t_add((PA * 0.3333f), (PB * 0.3333f)), (PC * 0.3333f));
                                float r1 = RND();
                                rgl::vector4 d1 = ((PA - m.o) * r1);
                                float r2 = RND();
                                rgl::vector4 d2 = ((PB - m.o) * r2);
                                float r3 = RND();
                                rgl::vector4 d3 = ((PC - m.o) * r3);
                                m.o = t_add(t_add(t_add(d2, d1), d3), m.o);
                                if ((pt->flags & 0x40000) != 0) {
                                    m.u = q.nrm[t];
                                    m.f = t_cross(m.u, m.s);
                                    t_normalize(m.f);
                                    m.s = t_cross(m.f, m.u);
                                }
                                float ang = (float)(rand() % 0x3dc9) * 0.00039724255f;
                                m.rotate_about_u(&ang);
                                e->frame = m;
                                float sc = (float)(rand() % 0x3dc9) * 4.425618e-05f + 0.65f;
                                e->dir = rgl::vector4(((float)(rand() % 0x3dc9) * 1.2644623e-05f + 0.9f) * sc,
                                                      ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 0.9f) * sc,
                                                      ((float)(rand() % 0x3dc9) * 1.8966935e-05f + 0.85f) * sc);
                                list->count++;
                            } while (RND() < rate);
                        }
                    }
                }
            }
        }
    }
}


// @ 0x46ad50
void Terrain::compute_normals()
{
    int x, y, flip;
    vector4 tri[3];
    int ab[6];
    vector4 n;
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            for (flip = 0; flip < 2; flip++) {
                triangle_corners(x, y, flip, &ab[0], &ab[2], &ab[4]);
                tri[0] = at(ab[0], ab[1]).P();
                tri[1] = at(ab[2], ab[3]).P();
                tri[2] = at(ab[4], ab[5]).P();
                quad(x, y).nrm[flip] = *((vector4* (__cdecl*)(vector4*, const vector4*))tri_normal)(&n, tri);   // 0x4612d0: cdecl, hidden result pointer = &n, returned in eax
            }
        }
    }
    for (y = 0; y <= h; y++) {
        n.x = 0.0f; n.y = 0.0f; n.z = 0.0f;
        for (x = 0; x <= w; x++)
            at(x, y).N() = n;
    }
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            for (flip = 0; flip < 2; flip++) {
                triangle_corners(x, y, flip, &ab[0], &ab[2], &ab[4]);
                const vector4& q = quad(x, y).nrm[flip];
                vector4* v;
                v = &at(ab[0], ab[1]).N(); v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
                v = &at(ab[2], ab[3]).N(); v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
                v = &at(ab[4], ab[5]).N(); v->x = q.x + v->x; v->y = q.y + v->y; v->z = q.z + v->z;
            }
        }
    }
    for (y = 0; y <= h; y++) {
        for (x = 0; x <= w; x++) {
            vector4& v = at(x, y).N();
            float len = v.length();
            if (len > 1e-10f) {
                float inv = 1.0f / len;
                v.x *= inv; v.y *= inv; v.z *= inv;
            } else {
                v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
            }
        }
    }
}

// ---- an agent's equipment slots, line of fire ---------------------------------------------------------------------------------

// @ 0x46b190
__int64 __cdecl pick_slot_item(int troop, int slot, int mask, int n, int* cand, char remove, char* found)
{
    *found = 0;
    if (((__int64)mask & g_bit[slot]) == 0 &&
        ((mask & 0x200) == 0 || (g_bit[slot] & 0x10) == 0 ||
         (int)g_items[((World3*)g_game)->troops[troop].equip[slot].item].type_flags >= 0)) {
        *found = 1;
        return *(__int64*)&(*(World3**)&g_game)->troops[troop].equip[slot];
    }
    int i = 0;
    ItemRef res;
    res.item = -1;
    res.m4 = 0;
    do {
        if (n <= i) break;
        int v = cand[i];
        ItemRef r;
        r.m4 = 0;
        if (v >= 0) {
            r.item = v;
            if (((World3*)g_game)->troops[troop].fits_slot(&r, slot)) {
                res.item = v;
                res.m4 = 0;
                res.reduce(0);
                if (remove) cand[i] = -1;
            }
        }
        i++;
    } while (res.item == -1);
    return *(__int64*)&res;
}

// @ 0x46b2d0
void __stdcall AgentEq_pick_slots(AgentEq* a)
{
    if (a->f10 == 0) {
        int cand[8];
        for (int j = 0; j < a->ncand; j++)
            cand[j] = a->cand[j];
        for (int i = 0; i < 10; i++)
            *(__int64*)&a->sel[i] = pick_slot_item(a->troop, i, a->mask, a->ncand, cand, 1, &a->found[i]);
        return;
    }
    for (int i = 0; i < 10; i++)
        a->sel[i].item = -1;
}

// @ 0x46b3a0
void __cdecl fill56(elem56* f, elem56* l, const elem56& v) { for (; f != l; ++f) *f = v; }

// @ 0x46b410
void FireAgent::ray(FireRay* r)
{
    r->width = 0.002f;
    r->from = pos;
    float s;
    if (troop >= 0) s = (float)item_f20c(troop) * 0.01f;
    else s = 0.8f;
    vector4 d = (dir * s);
    r->to = p_add(d, org);
}

// @ 0x46b4c0
E1c8* __cdecl copy1c8(E1c8* f, E1c8* l, E1c8* d)
{
    for (; f != l; ++f, ++d) *d = *f;
    return d;
}

// ---- std::_Uninit_fill_n with an opaque allocator::construct (catch(...) { throw; } funclets follow) -------------------------------

// @ 0x46b500 (+ funclet 0x46b564)
void __cdecl uninit_fill_n36(rec36* first, unsigned count, const rec36& val, alloc_rec36& al)
{
    rec36* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// @ 0x46b570 (+ funclet 0x46b5d4)
void __cdecl uninit_fill_n24(elem24* first, unsigned count, const elem24& val, alloc24& al)
{
    elem24* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// @ 0x46b5e0 (+ funclet 0x46b644)
void __cdecl uninit_fill_n100(particle100* first, unsigned count, const particle100& val, alloc100& al)
{
    particle100* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// @ 0x46b650 (+ funclet 0x46b6b4)
void __cdecl uninit_fill_n52(elem52* first, unsigned count, const elem52& val, alloc52& al)
{
    elem52* next = first;
    try {
        for (; 0 < count; --count, ++first) al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// ---- construct a 0x38-byte ArrSet (name + five count/array pairs) from another -----------------------------------------------------

// @ 0x46b6c0
void allocArr::construct(ArrSet* p, const ArrSet& v) { if (p) new (p) ArrSet(v); }   // thiscall ret 8 (ecx, the allocator, unused)

// ---- _Uninit_copy with an opaque allocator::construct ---------------------------------------------------------------------------------

// @ 0x46b6e0 (+ catch(...) funclet 0x46b746)
rec36* __cdecl uninit_copy36(rec36* first, rec36* last, rec36* dest, alloc_rec36& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    rec36* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) ;
        throw;
    }
    return dest;
}

// @ 0x46b750 (+ catch(...) funclet 0x46b7bc)
rec136* __cdecl uninit_copy_arrset(rec136* first, rec136* last, rec136* dest, alloc_rec136& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    rec136* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) ;
        throw;
    }
    return dest;
}

// ---- std::stable_sort over a vector of 8-byte records: the original has these instantiations in two groups (0x46b7d0 _Copy_opt,
// ---- 0x46b860 _Rotate; 0x46c260 copy, 0x46c2c0 merge, 0x46c3f0 copy, 0x46c420 copy_backward, 0x46c450 _Lower_bound, 0x46c4a0
// ---- _Upper_bound) with other functions between them; one instantiation point emits them together. -----------------------------------

void sort_e8(std::vector<E8>& v, Pr8 pred) { std::stable_sort(v.begin(), v.end(), pred); }

// @ 0x46b940
ViewParams::ViewParams()
{
    fog_blend = 0.1f; env_smooth = 0.1f;
    fog_color = 0x384878; env_color = 0x384878;
    f0 = 1000.0f; fog_rate = -100.0f; fog_ref = -1.0f;
    env_peak = -1.0f; env_time = -100.0f; env_c = -1.0e7f;
    fog_dirty = 0; b2c = 0;
    pos tmp;
    matrix* id = identity_pos(&tmp);
    world = *id;
    light = *id;
    f30 = 0;
}

// @ 0x46b9d0
void FxDef::set_env_count(int n)
{
    if (env != 0) {
        delete[] env;
        delete[] envidx;
    }
    nenv = n;
    env = new EnvRec[n];
    envidx = new int[nenv];
    for (int i = 0; i < nenv; i++) {
        EnvRec* b = env;
        EnvRec* e = b + i;
        int c = clock;
        env[i].fd0 = c;
        e->clock = c;
        e->t0 = g_ticks[c];
        env[i].alloc(count);
    }
}

// @ 0x46baf0
void Terrain::seed_brushes(float size, float density, float scale)
{
    float r = (float)(rand() % 15817) * 1.2644623e-5f + 0.4f;
    density = density * wx * wy / (size * size * 0.25f);
    float fl = (float)floor(density);
    int k = (int)fl;
    if (!(density - fl <= 0.5f)) k++;
    int n = (int)((float)k * r * 30.0f);
    for (; n > 0; n--) {
        density = size + size;      // (the original reuses the argument slot)
        float f = (rand_avg4() * 0.5f + 0.05f) * size;
        float pos[2];
        pos[1] = (density + wy) * (float)(rand() % 15817) * 6.322312e-05f - size;
        pos[0] = (density + wx) * (float)(rand() % 15817) * 6.322312e-05f - size;
        apply_brush(pos, f, ((float)(rand() % 15817) * 6.3223115e-06f + 0.2f) * f * scale);
    }
}

#define RAND01() ((float)(rand() % 15817) * 6.3223117e-05f)

// @ 0x46bc70
void Terrain::make_river()
{
    int a[2];
    int b[2];
    int steps;
    float minz;
    a[1] = 0;
    bool ok = false;
    if (flag843bd75) {
        int tries = 0;
        int bx = 0;
        do {
            if (3 < tries) break;
            int bestx = 0, besty = 0;
            float bestv = -10000.0f;
            int k = 20;
            do {
                int px, py;
                if (RAND01() < 0.5f) {
                    px = rand() % 4 + 3;
                    if (RAND01() < 0.5f) px = w - px;
                    int t = h - 3;
                    if (t < 2) py = 3;
                    else py = rand() % t + 3;
                } else {
                    int t = w - 3;
                    if (t < 2) px = 3;
                    else px = rand() % t + 3;
                    py = rand() % 4 + 3;
                    if (RAND01() < 0.5f) py = h - py;
                }
                float v = noise_at(bx, a[1]);
                if (bestv < v) {
                    bestv = v;
                    bestx = px;
                    besty = py;
                }
            } while (--k != 0);
            a[0] = bestx;
            a[1] = besty;
            int n = 0;
            bool is_far = false;
            do {
                if (is_far) goto trace;
                int ex, ey;
                if (RAND01() < 0.5f) {
                    ex = 0;
                    b[0] = 0;
                    if (RAND01() < 0.5f) {
                        ex = w + 1;
                        b[0] = ex;
                    }
                    int t = h + 1;
                    if (t < 2) {
                        ey = 0;
                        b[1] = 0;
                    } else {
                        ey = rand() % t;
                        b[1] = ey;
                    }
                } else {
                    ey = 0;
                    b[1] = 0;
                    if (RAND01() < 0.5f) {
                        ey = h + 1;
                        b[1] = ey;
                    }
                    int t = w + 1;
                    if (t < 2) {
                        ex = 0;
                        b[0] = 0;
                    } else {
                        ex = rand() % t;
                        b[0] = ex;
                    }
                }
                ex = ex - bestx;
                if (ex < 0) ex = -ex;
                ey = ey - besty;
                if (ey < 0) ey = -ey;
                if ((float)(w + h) * 0.7f < (float)(ey + ex)) is_far = true;
                n++;
            } while (n < 50);
            if (is_far) {
            trace:
                trace_path(a, b, 2, &steps, &minz, 1.5f, 0.9f, 0, 1);
                if (minz < 3.5f) ok = true;
                else clear_flags(g_river_mask_a | g_river_mask_b);
                tries++;
            }
            bx = a[0];
        } while (!ok);
    }
    int r = rand();
    n284 = r % 3;
    if (2.1f < scale && 1 < r % 3) n284 = 1;
    if (3.1f < scale) n284 = 0;
    if ((unsigned char)(((int*)g_bit)[2] - 1) & (unsigned char)((unsigned)f4 >> 31)) n284 = 0;
    if (ok) {
        unsigned mb = g_river_mask_b;
        unsigned both = g_river_mask_a | mb;
        spread_flow(mb, 1, both);
        for (int i = 0; i < n284; i++)
            spread_flow(mb, 2, both);
        unsigned ma = g_river_mask_a;
        spread_flow(mb, 2, ma);
        spread_flow(ma, 2, ma);
        float v = -1.3f;
        if ((unsigned char)(((int*)g_bit)[2] - 1) & (unsigned char)((unsigned)f4 >> 31)) v = -2.3f;
        set_cells(2, v);
        smooth_cells(2, 0.5f);
    }
}

// ---- std::list<int>::_Buynode() (12-byte node, head links to itself); the 0x46c12f funclet frees the node and rethrows -----------------

struct IntNodeList { std::list<int> l; };
IntNodeList* make_int_list() { return new IntNodeList; }

// ---- _Ucopy thunks: vector<T>::_Ucopy(first, last, dest) -> _Uninit_copy(first, last, dest, _Alval, tag) --------------------------------

// @ 0x46c150
rec36* vec36::_Ucopy(rec36* f, rec36* l, rec36* d) { return uninit_copy36(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c170
elem24* vec24::_Ucopy(elem24* f, elem24* l, elem24* d) { return uninit_copy24(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c190
particle100* vec100::_Ucopy(particle100* f, particle100* l, particle100* d) { return uninit_copy100(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c1b0
elem52* vec52::_Ucopy(elem52* f, elem52* l, elem52* d) { return uninit_copy52(f, l, d, _Alval, std::_Ptr_cat(f, d)); }

// ---- _Uninit_copy of the 0x38-byte ArrSet records (the 0x46c23a funclet destroys the constructed prefix, then rethrows) ----------------

// @ 0x46c1d0 (+ catch(...) funclet 0x46c23a: ~ArrSet over the constructed prefix, then rethrow)
ArrSet* __cdecl uninit_copy_arrset2(ArrSet* first, ArrSet* last, ArrSet* dest, allocArr& al)
{
    ArrSet* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) next->~ArrSet();
        throw;
    }
    return dest;
}

// ---- Direct3D vertex / index data of a mesh; transform of the child meshes of a lod_mesh ---------------------------------------------------

// @ 0x46c4f0
#define VS(f) (*(int*)&vs[i].f)
#define PT(f) (*(int*)&ps[vs[i].index].f)
void rgl::mesh::fill_vertices(int* p, const unsigned* fmt)
{
    bool b18 = (res->shader->stages[0].b & 0x1f) == 0x18;
    std::vector<rgl::vertex60>& vs = local_a1 ? v50 : parent->get50();
    std::vector<rgl::point20>& ps = local_a1 ? v70 : parent->get70();
    int *p7 = p + 7, *p6 = p + 6, *p5 = p + 5, *p4 = p + 4, *p3 = p + 3, *p2 = p + 2;
    for (unsigned i = 0; i < vs.size(); i++) {
        if (*fmt & 0x2000000) {
            p[0] = PT(b.x);
            p[1] = PT(b.y);
            *p2 = PT(b.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        } else if (*fmt & 0x1000000) {
            p[0] = PT(a.x);
            p[1] = PT(a.y);
            *p2 = PT(a.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        }
        if (*fmt & 1) {
            p[0] = VS(normal.x);
            p[1] = VS(normal.y);
            *p2 = VS(normal.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        }
        if (*fmt & 2) {
            p[0] = b18 ? VS(f58) : VS(color);
            p += 1; p2 += 1; p3 += 1; p4 += 1; p5 += 1; p6 += 1; p7 += 1;
        }
        if (*fmt & 0x40) {
            p[0] = VS(f5c);
            p += 1; p2 += 1; p3 += 1; p4 += 1; p5 += 1; p6 += 1; p7 += 1;
        }
        if (*fmt & 0x10) {
            p[0] = VS(uv.x);
            *(float*)&p[1] = 1.0f - vs[i].uv.y;
            p += 2; p2 += 2; p3 += 2; p4 += 2; p5 += 2; p6 += 2; p7 += 2;
        }
        if (*fmt & 0x20) {
            p[0] = VS(f48);
            *(float*)&p[1] = 1.0f - *(float*)&vs[i].f4c;
            p += 2; p2 += 2; p3 += 2; p4 += 2; p5 += 2; p6 += 2; p7 += 2;
        }
        if (*fmt & 0x100) {
            p[0] = VS(f10.x);
            p[1] = VS(f10.y);
            *p2 = VS(f10.z);
            *p3 = VS(f20.x);
            *p4 = VS(f20.y);
            *p5 = VS(f20.z);
            p += 6; p2 += 6; p3 += 6; p4 += 6; p5 += 6; p6 += 6; p7 += 6;
        }
        if (*fmt & 0x1000) {
            std::vector<rgl::skin_vert>& es = local_a1 ? v60 : parent->get60();
            p[0] = *(int*)&es[i].w[0];
            p[1] = *(int*)&es[i].w[1];
            *p2 = *(int*)&es[i].w[2];
            *p3 = *(int*)&es[i].w[3];
            *p4 = *(int*)&es[i].bonef[0];
            *p5 = *(int*)&es[i].bonef[1];
            *p6 = *(int*)&es[i].bonef[2];
            *p7 = *(int*)&es[i].bonef[3];
            p += 8; p2 += 8; p3 += 8; p4 += 8; p5 += 8; p6 += 8; p7 += 8;
        }
    }
}

// @ 0x46c920
void rgl::mesh::fill_indices(unsigned short* out)
{
    std::vector<rgl::face20>& fs = local_a0 ? v90 : parent->get90();
    for (unsigned i = 0; i < fs.size(); i++) {
        *out++ = (unsigned short)fs[i].a;
        *out++ = (unsigned short)fs[i].b;
        *out++ = (unsigned short)fs[i].c;
    }
}

// @ 0x46c990
void mesh_list_x::apply(matrix m)
{
    rgl::mesh** it = v._Myfirst;
    if (it < v._Mylast) {
        matrix t = m;
        do {
            rgl::mesh* x = *it;
            x->m_cc = t * x->m_cc;
            ++it;
        } while (it < v._Mylast);
    }
}

// @ 0x46c9f0
void lod_mesh_x::apply(const matrix* m)
{
    for (int i = 0; i < count; i++)
        ((mesh_list_x*)&lists[i])->apply(*m);
}

// ---- per-frame Direct3D 9 setup of a scene / view object -------------------------------------------------------------------------------------

// @ 0x46ca50
void RenderScene::apply_state()
{
    D3DVIEWPORT9 vp;
    DWORD clear = 0;
    if (use_alt_vp) {
        vp.X = vp_alt.X;
        vp.Y = vp_alt.Y;
        vp.Width = vp_alt.Width;
        vp.Height = vp_alt.Height;
        vp.MinZ = vp_alt.MinZ;
        vp.MaxZ = vp_alt.MaxZ;
    } else {
        vp.X = vp_local.X;
        vp.Y = vp_local.Y;
        vp.Width = vp_local.Width;
        vp.Height = vp_local.Height;
        vp.MinZ = vp_local.MinZ;
        vp.MaxZ = vp_local.MaxZ;
    }
    g_render.dev->SetViewport(&vp);
    if (flags & 0x100)
        clear = D3DCLEAR_TARGET;
    if (flags & 0x200)
        clear |= D3DCLEAR_ZBUFFER;
    if (clear)
        g_render.dev->Clear(0, 0, clear, clear_color, 1.0f, 0);
    if (use_alt_vp) {
        vp.X = vp_local.X;
        vp.Y = vp_local.Y;
        vp.Width = vp_local.Width;
        vp.Height = vp_local.Height;
        vp.MinZ = vp_local.MinZ;
        vp.MaxZ = vp_local.MaxZ;
        g_render.dev->SetViewport(&vp);
    }
    if (msaa)
        g_render.dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 1);
    else
        g_render.dev->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, 0);
    if (clip_on) {
        if (g_use_shaders == 0) {
            g_render.dev->SetClipPlane(0, clip_plane);
            g_render.dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 1);
        }
    } else {
        g_render.dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    }
    rgl::frustum* c = &cam;
    {
        D3DXMATRIX proj = c->projection();
        g_render.dev->SetTransform(D3DTS_PROJECTION, &proj);
    }
    g_render.set_view((const rgl::mat4*)&c->m);
    if (cull_flip) {
        if (g_render.use_hw_shadow)
            g_render.cull_setting = 1;
        else
            g_render.cull_setting = cam.left_handed ? 2 : 3;
    } else {
        g_render.cull_setting = (cam.left_handed != 0) + 2;
    }
    unsigned char fog_enable = (fog && fog->enabled) ? 1 : 0;
    if (g_render.b505 != fog_enable) {
        g_render.b505 = fog_enable;
        g_render.dev->SetRenderState(D3DRS_FOGENABLE, fog_enable);
    }
    if (fog && fog->enabled) {
        bool below = cam.m.o.z < fog->level;
        if (fog_invert)
            below = !below;
        if (below) {
            g_render.dev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            g_render.dev->SetRenderState(D3DRS_FOGCOLOR, fog->b_color);
            g_render.dev->SetRenderState(D3DRS_FOGSTART, fog->b_start);
            g_render.dev->SetRenderState(D3DRS_FOGEND, fog->b_end);
        } else {
            g_render.dev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            g_render.dev->SetRenderState(D3DRS_FOGCOLOR, fog->a_color);
            g_render.dev->SetRenderState(D3DRS_FOGSTART, fog->a_start);
            g_render.dev->SetRenderState(D3DRS_FOGEND, fog->a_end);
        }
    }
    if (g_use_shaders == 0) {
        if (lights.size() > 0 && lights_active && g_options.char_detail) {
            lights_active = 1;
            g_render.set_lighting(true);
            bool found_spot = false;
            D3DLIGHT9 L;
            unsigned i;
            for (i = 0; i < lights.size() && (int)i < 0x40; i++) {
                memset(&L, 0, sizeof(L));
                if (lights[i]->type == 0)
                    L.Type = D3DLIGHT_POINT;
                else if (lights[i]->type == 1)
                    L.Type = D3DLIGHT_SPOT;
                else if (lights[i]->type == 2)
                    L.Type = D3DLIGHT_DIRECTIONAL;
                L.Diffuse.r = lights[i]->diffuse[0];
                L.Diffuse.g = lights[i]->diffuse[1];
                L.Diffuse.b = lights[i]->diffuse[2];
                L.Ambient.r = 0;
                L.Ambient.g = 0;
                L.Ambient.b = 0;
                if (!found_spot && lights[i]->type == 2) {
                    found_spot = true;
                    L.Ambient.r = ambient[0];
                    L.Ambient.g = ambient[1];
                    L.Ambient.b = ambient[2];
                }
                L.Specular.r = lights[i]->specular[0];
                L.Specular.g = lights[i]->specular[1];
                L.Specular.b = lights[i]->specular[2];
                L.Position.x = lights[i]->pos[0];
                L.Position.y = lights[i]->pos[1];
                L.Position.z = lights[i]->pos[2];
                L.Direction.x = lights[i]->dir[0];
                L.Direction.y = lights[i]->dir[1];
                L.Direction.z = lights[i]->dir[2];
                L.Attenuation0 = lights[i]->atten0;
                L.Attenuation1 = lights[i]->atten1;
                L.Attenuation2 = lights[i]->atten2;
                L.Range = lights[i]->range;
                L.Theta = lights[i]->theta;
                L.Phi = lights[i]->phi;
                L.Falloff = 1.0f;
                g_render.dev->SetLight(i, &L);
            }
            if (!found_spot) {
                memset(&L, 0, sizeof(L));
                L.Diffuse.r = 0;
                L.Diffuse.g = 0;
                L.Diffuse.b = 0;
                L.Type = D3DLIGHT_DIRECTIONAL;
                L.Ambient.r = ambient[0];
                L.Ambient.g = ambient[1];
                L.Ambient.b = ambient[2];
                L.Specular.r = 0;
                L.Specular.g = 0;
                L.Specular.b = 0;
                L.Position.x = 1000000.0f;
                L.Position.y = 0;
                L.Position.z = 0;
                L.Direction.x = -1.0f;
                L.Direction.y = 0;
                L.Direction.z = 0;
                L.Range = 1000000.0f;
                g_render.dev->SetLight(i, &L);
            }
        } else {
            lights_active = 0;
            if (g_render.b506) {
                g_render.b506 = 0;
                g_render.dev->SetRenderState(D3DRS_LIGHTING, 0);
            }
        }
    }
}

// ---- vertex / index buffer uploader ---------------------------------------------------------------------------------------------------------

// Body of rgl::mesh::count (0x468620, above in this object): visible here so that cl sees it has no side effects, which lets it hoist loads
// across the calls below.
#define LOG_GROW(KIND_WORD, KIND_V_WORD)                                                           \
    do {                                                                                           \
        stream_base& lg = g_log;                                                                   \
        stream_base* ls;                                                                           \
        if (type == 1)                                                                             \
            ls = &lg.w_cstr("Out of " KIND_WORD " index buffer memory. Needed = ");                \
        else                                                                                       \
            ls = &lg.w_tstr(&name).w_cstr(KIND_V_WORD);                              \
        int needed = total;\
        ls = &ls->w_int(&needed);                                                                   \
        ls = &ls->w_cstr("Available:");                                                            \
        ls = &ls->w_int(pcap);                                                                \
        ls->w_cstr("\n");                                                                          \
        int grow = total / 4;                                                                      \
        if (grow <= 0x20)                                                                          \
            grow = 0x20;                                                                           \
        *pcap = grow + total;                                                                   \
        ls = &lg.w_cstr("Expanding Buffer Capacity to ");                                          \
        ls = &ls->w_int(pcap);                                                                \
        ls->w_cstr("\n");                                                                          \
        *pcap = *pcap / 32 * 32;                                                             \
        g_scene_dirty = 1;                                                                       \
        return 0xfff3ebc3;                                                                         \
    } while (0)

// @ 0x46d060
int dev_buffer::upload(void* scratch, void* tag)
{
    int n = 0;                          // elements staged in scratch
    int base = 0;                       // elements assigned so far in this upload
    int written = 0;                    // bytes already copied into the locked buffer
    char* locked;
    if (type == 1)
        elem_size = 2;
    std::vector<rgl::mesh*>::iterator it;
    for (it = meshes.begin(); it != meshes.end(); ++it) {
        rgl::slot& s = (*it)->slots[type];
        if (s.f24 != (int)tag) {
            s.f24 = (int)tag;
            (&s.pa)[s.f20] = (unsigned char)s.f28;
            s.f20 = (s.f20 + 1) % 2;
        }
    }
    it = meshes.begin();
    if (!is_dynamic) {
        int need = 0;
        int off = type * sizeof(rgl::slot);
        for (; it != meshes.end(); ++it) {
            rgl::slot& s = *(rgl::slot*)((char*)&(*it)->slots[0] + off);
            if (generation != s.fc || s.f28 != 0 || s.f14 != s.f4)
                need += ((rgl::mesh*)*it)->count(type);
        }
        if (need == 0)
            return 0;
        int* pcap = &count;
        if (cursor + need < *pcap) {
            int tries = 0;
            if (type == 1) {
                do {
                    if (ib->Lock(cursor * 2, need * 2, (void**)&locked, D3DLOCK_NOOVERWRITE) >= 0)
                        break;
                    tries++;
                    Sleep(10);
                    if (tries > 0x5a)
                        mb_error("unable to lock index buffer");
                } while (tries < 100);
            } else {
                do {
                    if (vb->Lock(cursor * elem_size, elem_size * need, (void**)&locked, D3DLOCK_NOOVERWRITE) >= 0)
                        break;
                    tries++;
                    Sleep(10);
                    if (tries > 0x5a)
                        mb_error("unable to lock vertex buffer");
                } while (tries < 100);
            }
            for (it = meshes.begin(); it != meshes.end(); ++it) {
                rgl::slot& s = (*it)->slots[type];
                if (s.fc != generation || s.f28 != 0 || s.f14 != s.f4) {
                    if ((((rgl::mesh*)*it)->count(type) + n) * elem_size >= 0x418000) {
                        memcpy(locked + written, scratch, elem_size * n);
                        written += elem_size * n;
                        n = 0;
                    }
                    char* dst = (char*)scratch + elem_size * n;
                    if (type == 1)
                        ((rgl::mesh*)*it)->fill_indices((unsigned short*)dst);
                    else
                        ((rgl::mesh*)*it)->fill_vertices((int*)dst, &fvf);
                    s.f8 = cursor + base;
                    s.fc = generation;
                    s.f14 = s.f4;
                    s.f28 = 0;
                    int k = type;
                    rgl::mesh* tm = (rgl::mesh*)*it;
                    n += tm->count(k);
                    base += tm->count(k);
                }
            }
            memcpy(locked + written, scratch, elem_size * n);
            if (type == 1) {
                ib->Unlock();
                cursor += base;
                return 0;
            }
            vb->Unlock();
            cursor += base;
            return 0;
        }
        generation++;
        base = 0;
        int total = 0;
        for (it = meshes.begin(); it != meshes.end(); ++it)
            total += ((rgl::mesh*)*it)->count(type);
        if (total > *pcap)
            LOG_GROW("Static", ":Out of Static vertex buffer memory. Needed = ");
        if (type == 1) {
            if (ib->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
                mb_error("unable to lock index buffer");
        } else {
            if (vb->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
                mb_error("unable to lock vertex buffer");
        }
        for (it = meshes.begin(); it != meshes.end(); ++it) {
            rgl::slot& s = (*it)->slots[type];
            if ((((rgl::mesh*)*it)->count(type) + n) * elem_size >= 0x418000) {
                memcpy(locked + written, scratch, elem_size * n);
                written += elem_size * n;
                n = 0;
            }
            char* dst = (char*)scratch + elem_size * n;
            if (type == 1)
                ((rgl::mesh*)*it)->fill_indices((unsigned short*)dst);
            else
                ((rgl::mesh*)*it)->fill_vertices((int*)dst, &fvf);
            s.f8 = base;
            s.fc = generation;
            s.f14 = s.f4;
            s.f28 = 0;
            int k = type;
            rgl::mesh* tm = (rgl::mesh*)*it;
            base += tm->count(k);
            n += tm->count(k);
        }
        memcpy(locked + written, scratch, elem_size * n);
        if (type == 1) {
            ib->Unlock();
            cursor = base;
            return 0;
        }
        vb->Unlock();
        cursor = base;
        return 0;
    }
    int total = 0;
    for (; it != meshes.end(); ++it)
        total += ((rgl::mesh*)*it)->count(type);
    int* pcap = &count;
    if (total > count)
        LOG_GROW("dynamic", ":Out of dynamic vertex buffer memory. Needed = ");
    if (type == 1) {
        if (ib->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) < 0)
            mb_error("unable to lock index buffer");
    } else if (count != 0) {
        int tries = 0;
        do {
            if (vb->Lock(0, 0, (void**)&locked, D3DLOCK_DISCARD) >= 0)
                break;
            tries++;
            Sleep(10);
            if (tries > 0x5a)
                mb_error("unable to lock vertex buffer");
        } while (tries < 100);
    }
    for (it = meshes.begin(); it != meshes.end(); ++it) {
        rgl::slot& s = (*it)->slots[type];
        if ((((rgl::mesh*)*it)->count(type) + n) * elem_size >= 0x418000) {
            memcpy(locked + written, scratch, elem_size * n);
            written += elem_size * n;
            n = 0;
        }
        char* dst = (char*)scratch + elem_size * n;
        if (type == 1)
            ((rgl::mesh*)*it)->fill_indices((unsigned short*)dst);
        else
            ((rgl::mesh*)*it)->fill_vertices((int*)dst, &fvf);
        s.f8 = base;
        s.f14 = s.f4;
        int k = type;
        rgl::mesh* tm = (rgl::mesh*)*it;
        base += tm->count(k);
        n += tm->count(k);
    }
    memcpy(locked + written, scratch, elem_size * n);
    if (type == 1) {
        ib->Unlock();
        return 0;
    }
    if (count != 0)
        vb->Unlock();
    return 0;
}

// ---- shadow-view draw pass ---------------------------------------------------------------------------------------------------------------------

static __forceinline vector4 cross_i(const vector4& a, const vector4& b)
{
    return vector4(a.z * b.y - b.z * a.y, b.z * a.x - a.z * b.x, a.y * b.x - b.y * a.x);
}

// @ 0x46d8c0
void render_batch::draw(view_t* v)
{
    if (v->b162 || v->f164) {
        if (g_render.b506) {
            g_render.b506 = 0;
            g_render.dev->SetRenderState(D3DRS_LIGHTING, 0);
        }
        if (g_render.alpha_mode)
            g_render.dev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
        g_render.alpha_mode = 0;
        g_render.set_blend_mode(2);   // 0x40e940: render_cache (rgl/particles/render_cache.h) is the same object as renderer
        if (v->f164) {
            if (g_render.b54b) {
                g_render.b54b = 0;
                g_render.dev->SetRenderState(D3DRS_ZFUNC, 8);
            }
            if (g_render.b549 != 1) {
                g_render.b549 = 1;
                g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
            }
            g_render.set_stage_alpha(0, 0x4a4);
        } else {
            if (g_render.b54b != 1) {
                g_render.b54b = 1;
                g_render.dev->SetRenderState(D3DRS_ZFUNC, 4);
            }
            if (g_render.b549) {
                g_render.b549 = 0;
                g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, 1);
            }
            g_render.set_stage_alpha(0, 0xa2);
        }
        if (g_render.b548) {
            g_render.dev->SetRenderState(D3DRS_NORMALIZENORMALS, 0);
            g_render.b548 = 0;
        }
        if (g_render.b547) {
            g_render.dev->SetRenderState(D3DRS_SPECULARENABLE, 0);
            g_render.b547 = 0;
        }
        g_render.set_stage_color(0, 0xa2);
        g_render.set_stage_color(1, 2);
        g_render.set_stage_alpha(1, 2);
        g_render.set_stage_color(2, 1);
        g_render.set_stage_alpha(2, 1);
    }

    matrix M;
    (rot3&)M = v->rot;
    M.s.x *= 0.5f; M.s.y *= 0.5f; M.s.z *= 0.5f;
    vector4 half(0.5f, 0.5f, 0.5f);
    M.o = half;
    M.f.x *= 0.5f; M.f.y *= 0.5f; M.f.z *= 0.5f;
    M.u.x *= 0.5f; M.u.y *= 0.5f; M.u.z *= 0.5f;
    matrix A;
    identity_pos((pos*)&A);                      // pos adds no data to rgl::matrix
    float ang = 1.5707964f;
    A.rotate_about_s(&ang);
    M = A * M;
    cur_part = 0;
    cur_mat = 0;
    M.s.w = 0.0f; M.f.w = 0.0f; M.u.w = 0.0f; M.o.w = 1.0f;

    mesh_entry* p = v->ents._Myfirst;
    if (p == v->ents._Mylast) return;
    do {
        skin_x* part = (skin_x*)p->m->res;
        int b = p->m->slots[0].f4;
        int a = p->m->slots[0].f0;
        if (cur_vf_a != b || cur_vf_b != a) {
            cur_vf_a = b;
            cur_vf_b = a;
            g_render.dev->SetVertexDeclaration(vf[a].bufs[b].decl);
            g_render.dev->SetStreamSource(0, vf[a].bufs[b].vb, 0, vf[a].bufs[b].elem_size);
        }
        rgl::mesh* node = p->m->local_a0 ? p->m : p->m->parent;
        int ibi = node->slots[1].f4;
        if (cur_ib != ibi) {
            cur_ib = ibi;
            g_render.dev->SetIndices(ib.bufs[ibi].ib);
        }
        if (!v->b162) {
            if (v->f164) goto L_state;
            if (part != cur_part) {
                cur_part = part;
                part->apply_state((const float*)&p->m->f48);
                if (cur_mat != part->shader) {
                    ((material*)part->shader)->bind_pixel_shader();   // 0x461f20, material's (src/game/renderer.cpp)
                    cur_mat = part->shader;
                }
            }
            if (v->p1b0 && v->p1b0->on) {
                if (part->flags & 1) {
                    if (g_render.b505) {
                        g_render.b505 = 0;
                        g_render.dev->SetRenderState(D3DRS_FOGENABLE, 0);
                    }
                } else {
                    if (g_render.b505 != 1) {
                        g_render.b505 = 1;
                        g_render.dev->SetRenderState(D3DRS_FOGENABLE, 1);
                    }
                }
            }
            if (v->b160 && !(part->shader->flags20 & 0x80)) {
                if (g_render.b506 != 1) {
                    g_render.b506 = 1;
                    g_render.dev->SetRenderState(D3DRS_LIGHTING, 1);
                }
                unsigned i = 0;
                light_t** b;
                while ((b = v->lights._Myfirst, b != 0) && i < (unsigned)(v->lights._Mylast - b) && (int)i < 0x40) {
                    light_t** pl = &b[i];
                    if ((*pl)->type == 2) {
                        if (g_render.light_on[i] != 1) {
                            g_render.light_on[i] = 1;
                            g_render.dev->LightEnable(i, 1);
                        }
                        i = i + 1;
                    } else {
                        light_t* l = *pl;
                        vector4 d = (l->pos - p->m->m194.o);
                        if (d.length() - p->m->f44 < (*pl)->range) {
                            if (g_render.light_on[i] != 1) {
                                g_render.light_on[i] = 1;
                                g_render.dev->LightEnable(i, 1);
                            }
                            i = i + 1;
                        } else {
                            if (g_render.light_on[i]) {
                                g_render.light_on[i] = 0;
                                g_render.dev->LightEnable(i, 0);
                            }
                            i = i + 1;
                        }
                    }
                }
            } else {
                if (g_render.b506) {
                    g_render.b506 = 0;
                    g_render.dev->SetRenderState(D3DRS_LIGHTING, 0);
                }
            }
            p->m->setup_material();
            unsigned k = part->shader->stages[0].d & 0xff;
            if (k == 0x30)
                g_render.dev->SetTransform((D3DTRANSFORMSTATETYPE)0x10, (const D3DMATRIX*)&M);
            else if (k == 0x20)
                g_render.dev->SetTransform((D3DTRANSFORMSTATETYPE)0x10, (const D3DMATRIX*)&M);
        } else {
            if (v->f164) {
L_state:
                if (g_render.stage_alpha[0] != 0xa2) {
                    g_render.stage_alpha[0] = 0xa2;
                    g_render.dev->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
                    g_render.dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, g_stage_arg[5]);
                    g_render.dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, g_stage_arg[0]);
                }
                if (g_render.stage_alpha[1] != 1) {
                    g_render.stage_alpha[1] = 1;
                    g_render.dev->SetTextureStageState(1, D3DTSS_ALPHAOP, 1);
                    g_render.dev->SetTextureStageState(1, D3DTSS_ALPHAARG1, g_stage_arg[0]);
                    g_render.dev->SetTextureStageState(1, D3DTSS_ALPHAARG2, g_stage_arg[0]);
                }
                if (g_render.stage_color[0] != 0xa2) {
                    g_render.stage_color[0] = 0xa2;
                    g_render.dev->SetTextureStageState(0, D3DTSS_COLOROP, 2);
                    g_render.dev->SetTextureStageState(0, D3DTSS_COLORARG1, g_stage_arg[5]);
                    g_render.dev->SetTextureStageState(0, D3DTSS_COLORARG2, g_stage_arg[0]);
                }
                if (g_render.stage_color[1] != 1) {
                    g_render.stage_color[1] = 1;
                    g_render.dev->SetTextureStageState(1, D3DTSS_COLOROP, 1);
                    g_render.dev->SetTextureStageState(1, D3DTSS_COLORARG1, g_stage_arg[0]);
                    g_render.dev->SetTextureStageState(1, D3DTSS_COLORARG2, g_stage_arg[0]);
                }
            } else {
                if (!(p->m->mask & 0x8000)) {
                    if (g_render.b549) {
                        g_render.b549 = 0;
                        g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, 1);
                    }
                    ((texture_x*)part->tex[0])->bind(0, 0.0f);
                    g_render.set_stage_alpha(0, 0xa2);
                    g_render.set_stage_alpha(1, 0xaa);
                } else {
                    if (part->tex[0]) {
                        if (g_render.b549 != 1) {
                            g_render.b549 = 1;
                            g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
                        }
                        ((texture_x*)part->tex[0])->bind(0, 0.0f);
                        g_render.set_stage_alpha(0, 0x4a4);
                    } else {
                        if (g_render.b549 != 1) {
                            g_render.b549 = 1;
                            g_render.dev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
                        }
                        g_render.set_stage_alpha(0, 0xa2);
                    }
                    g_render.set_stage_alpha(1, 2);
                }
            }
            if (g_render.cur_ps) {
                g_render.dev->SetPixelShader(0);
                g_render.cur_ps = 0;
            }
            int col = p->m->color;
            if (g_render.tex_factor != col) {
                g_render.tex_factor = col;
                g_render.dev->SetRenderState(D3DRS_TEXTUREFACTOR, col);
            }
        }

        int cull = g_render.cull_setting;
        if (p->m->f4c) {
            if (cull == 3) cull = 2;
            else if (cull == 2) cull = 3;
        }
        if (cull != g_render.cull_cache) {
            g_render.cull_cache = cull;
            g_render.dev->SetRenderState(D3DRS_CULLMODE, cull);
        }

        if (p->m->f34 == 0) {
            if (!(part->flags & 0x20000)) {
                A = p->m->m194;
            } else {
                (rot3&)A = *(rot3*)&p->m->m194;
                A.o = v->org;
                A.o.z = A.o.z * 0.75f;
            }
        } else if (p->m->f34 == 1) {
            matrix B;
            B.o = p->m->m194.o;
            B.u = p->m->m194.u;
            B.s = v->rot.s;
            B.f = cross_i(B.s, B.u);
            B.u.normalize();
            B.s = cross_i(B.u, B.f);
            A = B;
        } else {
            matrix B;
            B.o = p->m->m194.o;
            B.s = v->rot.s; B.f = v->rot.f; B.u = v->rot.u;
            A = B;
        }
        A.s.w = 0.0f; A.f.w = 0.0f; A.u.w = 0.0f; A.o.w = 1.0f;
        g_render.dev->SetTransform((D3DTRANSFORMSTATETYPE)0x100, (const D3DMATRIX*)&A);

        std::vector<rgl::face20>& fv = node->local_a0 ? node->v90 : node->parent->get90();
        int nf = fv.size();
        std::vector<rgl::vertex60>& vv = p->m->local_a1 ? p->m->v50 : p->m->parent->get50();
        int nv = vv.size();
        g_render.dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, p->m->slots[0].f8, 0, nv, node->slots[1].f8, (nf * 3) / 3);
        p++;
    } while (p != v->ents._Mylast);
}

