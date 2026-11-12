//-----------------------------------------------------------------------------
// File: mesh_hud.cpp  (0x4958f0..0x499050)
//
// Desc: rgl::mesh helpers (clone, frame transform, alpha, uv normalise), the slider widget factory, the map-legend mesh and
//       the mission HUD bar widgets (per-frame updates, frame mesh rebuild, scroll bar reload) of translation units 85
//       (rgl_mesh) and 86 (hud2) of the TU map. Names provisional. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../../include/image_addrs.h"
#include "./mesh_hud.h"
#include "../config/widget_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../ui/mission_ui.h"
#include "../tstring.h"
#include "../../rgl/mesh.h"
#include "../../rgl/rgl_math.h"
#include "../../include/mesh_parts.h"
#include "../../include/mesh_prims.h"

// x87 evaluation helpers for the fade blocks below (the original inlines these shapes).
struct mh_float2 { float x, y; mh_float2() {} mh_float2(float a, float b) { x = a; y = b; } };
static inline mh_float2 operator*(const mh_float2& a, float k) { return mh_float2(a.x * k, a.y * k); }
static inline mh_float2 mh_fade_mix(float s, const mh_float2& a, float t, const mh_float2& b)
{
    mh_float2 l(s * a.x, s * a.y);
    mh_float2 r(t * b.x, t * b.y);
    return mh_float2(l.x + r.x, l.y + r.y);
}

// Seconds since track i was (re)started: MissionUi::elapsed(i) with the original's parentheses. Both pairs are needed:
// each parenthesised float subexpression is an OPPAREN tuple, and with them the first fild's fmul is scheduled after the
// next timer's int64 stores, as in the original (0x4966e9..0x496717).
static inline float mh_elapsed(const MissionUi* w, int i)
{
    return (((float)(g_ticks[w->tm[i].clock] - w->tm[i].start)) * 1e-5f);
}

// ==== 0x4958f0: clone a mesh with the same owner, material, flags and name =====================================
// @ 0x4958f0
rgl::mesh* rgl::mesh::copy_of()
{
    rgl::mesh* m = new rgl::mesh;
    if (parent)
        m->parent = parent;
    else
        m->parent = this;
    m->res = res;
    if (m->res)
        ++*(int*)((char*)m->res + 0x64);              // material reference count
    m->flags = flags;
    m->color = color;
    m->f40 = f40;
    m->local_a0 = 0;
    m->local_a1 = 0;
    m->name = name;
    m->f228 = f228;
    return m;
}

// ==== 0x495970: build the horizontal slider widget and append it to the manager ================================
// The slider widget is a UiB (ui/ui_widgets.h, vtable 0x591fb0; the original inlines its constructor 0x48b4b0 here).
// @ 0x495970
int WidgetMgrW::create_slider_overlay(int vmin, int vmax)
{
    UiB* w = new UiB;
    {
        float pos[2] = { 0.0f, 0.0f };                          // 0x4959d7: both halves stored before the call
        w->move_a(pos);
    }
    {
        tstring n("slider_hor");
        w->set_track(0, ((rgl::mesh*)g_resmgr.get_object(&n))->clone());
    }
    {
        tstring n("handle_hor");
        w->set_track(2, ((rgl::mesh*)g_resmgr.get_object(&n))->clone());
    }
    w->update_bounds();
    {
        float size[2];
        size[0] = 0.25f;
        size[1] = w->screen[1];                                 // 0x495a65 (+0xf4)
        w->set_xy(size);
    }
    float fmin = (float)vmin;
    w->f16c = fmin;
    w->f170 = (float)vmax;
    w->set168(fmin);                                            // 0x495a99: the float's bits are pushed
    scene->d50(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// ==== 0x495b20: build the map-legend mesh ==========================================================================
// @ 0x495b20
rgl::mesh* __stdcall build_legend_mesh(ColourOwner* parent)
{
    rgl::mesh* m = new rgl::mesh;
    m->f34 = 2;
    {
        rgl::string n("map_legend");
        m->res = g_resmgr.get_material(n);
    }
    m->mask = 1;
    m->slots[0].f4 = 1;
    m->slots[1].f4 = 1;
    ((Poser*)parent)->add_extra((int)m);   // 0x442110
    return m;
}

// ==== 0x495ba0: transform the position list, the vertex list and every key frame ==================================
// Takes a 3x4 transform (the s / f / u rows) by reference; the w lane of every rebuilt vector is whatever the
// uninitialised local carries (the original never writes it; see the sibling rgl::mesh transform at 0x44f3c0).
// Every sum is written in the original's evaluation order, which differs per row (0x495bdd..0x495e92: the position
// pair's x rows add f,s then u; the other rows s,u then f, or u,f then s); cl keeps the written order here.
// @ 0x495ba0
void rgl::mesh::set_part(const rgl::matrix& mtx)
{
    ensure_local();
    {
        std::vector<rgl::point20>& pos = v70;
        for (unsigned i = 0; i < pos.size(); i++) {
            vector4 a;
            a.x = (mtx.f.x * pos[i].a.y + mtx.s.x * pos[i].a.x) + mtx.u.x * pos[i].a.z;
            a.y = (mtx.s.y * pos[i].a.x + mtx.u.y * pos[i].a.z) + mtx.f.y * pos[i].a.y;
            a.z = (mtx.s.z * pos[i].a.x + mtx.u.z * pos[i].a.z) + mtx.f.z * pos[i].a.y;
            pos[i].a = a;
            vector4 b;
            b.x = (mtx.f.x * pos[i].b.y + mtx.s.x * pos[i].b.x) + mtx.u.x * pos[i].b.z;
            b.y = (mtx.u.y * pos[i].b.z + mtx.f.y * pos[i].b.y) + mtx.s.y * pos[i].b.x;
            b.z = (mtx.s.z * pos[i].b.x + mtx.u.z * pos[i].b.z) + mtx.f.z * pos[i].b.y;
            pos[i].b = b;
        }
    }
    {
        std::vector<rgl::vertex60>& verts = v50;
        for (unsigned i = 0; i < verts.size(); i++) {
            vector4* p = &verts[i].normal;
            vector4 a;
            a.x = (mtx.u.x * p->z + mtx.f.x * p->y) + mtx.s.x * p->x;
            a.y = (mtx.s.y * p->x + mtx.u.y * p->z) + mtx.f.y * p->y;
            a.z = (mtx.s.z * p->x + mtx.u.z * p->z) + mtx.f.z * p->y;
            *p = a;
        }
    }
    for (int i = 0; i < kf.count; i++) {
        rgl::frame_t* l = kf.frames + i;
        {
            std::vector<rgl::vector4>& vl = l->verts;
            for (unsigned j = 0; j < vl.size(); j++) {
                vector4 a;
                a.x = (mtx.u.x * vl[j].z + mtx.f.x * vl[j].y) + mtx.s.x * vl[j].x;
                a.y = (mtx.s.y * vl[j].x + mtx.u.y * vl[j].z) + mtx.f.y * vl[j].y;
                a.z = (mtx.s.z * vl[j].x + mtx.u.z * vl[j].z) + mtx.f.z * vl[j].y;
                vl[j] = a;
            }
        }
        {
            std::vector<rgl::vector4>& vl = l->norms;
            for (unsigned j = 0; j < vl.size(); j++) {
                vector4 a;
                a.x = (mtx.u.x * vl[j].z + mtx.f.x * vl[j].y) + mtx.s.x * vl[j].x;
                a.y = (mtx.s.y * vl[j].x + mtx.u.y * vl[j].z) + mtx.f.y * vl[j].y;
                a.z = (mtx.s.z * vl[j].x + mtx.u.z * vl[j].z) + mtx.f.z * vl[j].y;
                vl[j] = a;
            }
        }
    }
    slots[0].f28 = 1;
    ((rgl::mesh*)(void*)this)->compute_bounds();            // 0x419300
}



// ==== 0x495f00: set the alpha of every vertex record ===========================================================
// The colour with its alpha lane replaced, packed again. An inline helper taking the colour by value: the copy of
// unpack_color's result into the parameter is then independent of the loads through the returned pointer (the
// original loads two lanes ahead of the stores; a plain local `v = unpack_color(..)` orders them load/store strictly).
static __forceinline unsigned pack_with_alpha(rgl::vector4 c, float a) { c.w = a; return rgl::pack_color(&c); }

// Each vertex record keeps its packed colour at +0x54; that colour is unpacked into floats, the alpha lane is replaced
// by `a`, and it is packed back.
// @ 0x495f00
void rgl::mesh::set_alpha(float a)
{
    ensure_local();
    for (unsigned i = 0; i < v50.size(); i++) {
        unsigned packed = v50[i].color;
        rgl::vertex60* p = &v50[i];
        p->color = pack_with_alpha(rgl::unpack_color(&packed), a);
    }
}

// ==== 0x495fa0: rescale `src`'s uv records into this mesh's uv rectangle ==========================================
// @ 0x495fa0
void MissionUi::normalize_uv(rgl::mesh* src_mesh)
{
    rgl::mesh* src = src_mesh;
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    src->ensure_local();
    rgl::point20* p = src->v70._Myfirst;
    rgl::point20* end = src->v70._Mylast;
    if (p != end) {
        do {
            if (p->a.x > u0)
                u0 = p->a.x;
            if (p->a.x < u1)
                u1 = p->a.x;
            if (p->a.y > v0)
                v0 = p->a.y;
            if (p->a.y < v1)
                v1 = p->a.y;
            ++p;
        } while (p != end);
    }
    if (u0 == 0.0f || v0 == 0.0f)
        return;
    float su = (screen[0] - screen[2]) / (u0 - u1);
    float sv = (screen[1] - screen[3]) / (v0 - v1);
    float sm = (sv + su) * 0.5f;
    rgl::point20* q = src->v70._Myfirst;
    if (q == end)
        return;
    for (;;) {
        float x = q->a.x, y = q->a.y, z = q->a.z;
        q->a.x = x * su;
        q->a.y = y * sv;
        q->a.z = z * sm;
        q->b = q->a;
        ++q;
        if (q == src->v70._Mylast)
            break;
    }
}

// ==== the HUD bar widget ==========================================================================================
// @ 0x496100
void MeshWidget::animate(int)
{
    float e0 = mh_elapsed(this, 0);
    float e1 = mh_elapsed(this, 1);
    float e2 = mh_elapsed(this, 4);
    float e3 = mh_elapsed(this, 5);

    if (e2 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - e2;
        mh_float2 z = mh_fade_mix(e2, *(mh_float2*)&f88, rest, *(mh_float2*)&a8) * inv;
        rgl::vector4 v(z.x / fD8, z.y / fDC, 1.0f);
        msh(0)->set_color(v);
        s03();
        fD8 = z.x;
        fDC = z.y;
    } else if (c4) {
        c4 = 0;
        msh(0)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        s03();
        fD8 = f88;
        fDC = f8c;
    }

    if (e0 < dur(0)) {
        unsigned col = 0;
        float v = ((cb().dword & 0xff) * (dur(0) - e0) + (ca().dword & 0xff) * e0) * (1.0f / dur(0));
        int r = round_float(&v);
        col = r;
        float v1 = (cb().g * (dur(0) - e0) + ca().g * e0) * (1.0f / dur(0));
        int g = round_float(&v1);
        col += g << 8;
        float v2 = (cb().b * (dur(0) - e0) + ca().b * e0) * (1.0f / dur(0));
        int b = round_float(&v2);
        col += b << 16;
        col |= color14c & 0xff000000;
        tracks[0]->color = col;
        set_color(&col);
    } else if (c0) {
        c0 = 0;
        unsigned col = (color14c & 0xff000000) | (ca().dword & 0xffffff);
        tracks[0]->color = col;
        set_color(&col);
    }

    if (e1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
    } else if (c1) {
        c1 = 0;
        unsigned col = aa().dword;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
    }

    if (e3 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - e3;
        mh_float2 z = mh_fade_mix(e3, *(mh_float2*)&f80, rest, *(mh_float2*)&a0) * inv;
        s22(&z);
    } else if (c5) {
        s22(&f80);
        c5 = 0;
    }
}

// @ 0x4966b0
void XyzButton::animate(int)
{
    float e0 = mh_elapsed(this, 0);
    float e1 = mh_elapsed(this, 1);
    float e2 = mh_elapsed(this, 4);
    float e3 = mh_elapsed(this, 5);

    if (e2 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - e2;
        mh_float2 z = mh_fade_mix(e2, *(mh_float2*)&f88, rest, *(mh_float2*)&a8) * inv;
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / fD8, z.y / fDC, 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(z.x / fD8, z.y / fDC, 1.0f));
        update_bounds();
        fD8 = z.x;
        fDC = z.y;
    } else if (c4) {
        c4 = 0;
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        update_bounds();
        fD8 = f88;
        fDC = f8c;
    }

    if (e0 < dur(0)) {
        unsigned col = 0;
        float v = ((cb().dword & 0xff) * (dur(0) - e0) + (ca().dword & 0xff) * e0) * (1.0f / dur(0));
        int r = round_float(&v);
        col = r;
        float v1 = (cb().g * (dur(0) - e0) + ca().g * e0) * (1.0f / dur(0));
        int g = round_float(&v1);
        col += g << 8;
        float v2 = (cb().b * (dur(0) - e0) + ca().b * e0) * (1.0f / dur(0));
        int b = round_float(&v2);
        col += b << 16;
        col |= color14c & 0xff000000;
        tracks[2]->color = col;
        tracks[1]->color = col;
        set_color(&col);
    } else if (c0) {
        unsigned col = (color14c & 0xff000000) | (ca().dword & 0xffffff);
        c0 = 0;
        tracks[2]->color = col;
        tracks[1]->color = col;
        set_color(&col);
    }

    if (e1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
    } else if (c1) {
        c1 = 0;
        unsigned col = aa().dword;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
    }

    if (e3 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - e3;
        mh_float2 z = mh_fade_mix(e3, *(mh_float2*)&f80, rest, *(mh_float2*)&a0) * inv;
        move_a((float*)&z);
    } else if (c5) {
        move_a((float*)&f80);
        c5 = 0;
    }
}

// @ 0x496d30
void DropDown::animate(int)
{
    unsigned char dirty = 0;
    float e0 = mh_elapsed(this, 0);
    float e1 = mh_elapsed(this, 1);
    float e2 = mh_elapsed(this, 2);
    float e3 = mh_elapsed(this, 3);
    float e4 = mh_elapsed(this, 4);
    float e5 = mh_elapsed(this, 5);

    if (e4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - e4;
        mh_float2 z = mh_fade_mix(e4, *(mh_float2*)&f88, rest, *(mh_float2*)&a8) * inv;
        // Two parenthesised y quotients (any two of the three calls match): each is one more tuple, and the scheduler's
        // 81-tuple region must end two tuples before the third set_color call (0x496fa8: its last store precedes the
        // mesh load). The size and offset arguments are 8-byte temporaries, one per call: they open the frame slot the
        // vector temporaries and the later mix temporaries then share (frame 0x38).
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / fD8, (z.y / fDC), 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(z.x / fD8, (z.y / fDC), 1.0f));
        ((rgl::mesh*)tracks[3])->set_color(rgl::vector4(z.x / fD8, z.y / fDC, 1.0f));
        float sy = z.y * 0.02f;
        set_size((float*)&mh_float2(sy, sy));
        set_offset((float*)&mh_float2(sy, z.y * 0.013f));
        update_bounds();
        fD8 = z.x;
        fDC = z.y;
        dirty = 1;
    } else if (c4) {
        c4 = 0;
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        ((rgl::mesh*)tracks[3])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        set_size((float*)&mh_float2(f8c * 0.02f, f8c * 0.02f));
        set_offset((float*)&mh_float2(f8c * 0.02f, f8c * 0.013f));
        update_bounds();
        fD8 = f88;
        fDC = f8c;
        dirty = 1;
    }

    if (e0 < dur(0)) {
        unsigned col = 0;
        float v = ((cb().dword & 0xff) * (dur(0) - e0) + (ca().dword & 0xff) * e0) * (1.0f / dur(0));
        int r = round_float(&v);
        col = r;
        float v1 = (cb().g * (dur(0) - e0) + ca().g * e0) * (1.0f / dur(0));
        int g = round_float(&v1);
        col += g << 8;
        float v2 = (cb().b * (dur(0) - e0) + ca().b * e0) * (1.0f / dur(0));
        int b = round_float(&v2);
        col += b << 16;
        col |= color14c & 0xff000000;
        set_color(&col);
        dirty = 1;
    } else if (c0) {
        c0 = 0;
        unsigned col = (color14c & 0xff000000) | (ca().dword & 0xffffff);
        set_color(&col);
        dirty = 1;
    }

    if (e1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        tracks[3]->color = (tracks[3]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
        dirty = 1;
    } else if (c1) {
        c1 = 0;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | aa().dword;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | aa().dword;
        tracks[3]->color = (tracks[3]->color & 0xffffff) | aa().dword;
        unsigned col = (aa().dword & 0xff000000) | (color14c & 0xffffff);
        set_color(&col);
        dirty = 1;
    }

    if (e2 < dur(2)) {
        unsigned col = 0;
        float v = ((cb2().dword & 0xff) * (dur(2) - e2) + (ca2().dword & 0xff) * e2) * (1.0f / dur(2));
        int r = round_float(&v);
        col = r;
        float v1 = (cb2().g * (dur(2) - e2) + ca2().g * e2) * (1.0f / dur(2));
        int g = round_float(&v1);
        col += g << 8;
        float v2 = (cb2().b * (dur(2) - e2) + ca2().b * e2) * (1.0f / dur(2));
        int b = round_float(&v2);
        col += b << 16;
        col |= color150 & 0xff000000;
        set_target(&col);
        dirty = 1;
    } else if (c2) {
        c2 = 0;
        unsigned col = (color150 & 0xff000000) | (ca2().dword & 0xffffff);
        set_target(&col);
        dirty = 1;
    }

    if (e3 < dur(3)) {
        unsigned col = 0;
        float v = (ab2().a * (dur(3) - e3) + aa2().a * e3) * (1.0f / dur(3));
        col = round_float(&v) << 24;
        col |= color150 & 0xffffff;
        set_target(&col);
        dirty = 1;
    } else if (c3) {
        c3 = 0;
        unsigned col = (aa2().dword & 0xff000000) | (color150 & 0xffffff);
        set_target(&col);
        dirty = 1;
    }

    if (e5 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - e5;
        mh_float2 z = mh_fade_mix(e5, *(mh_float2*)&f80, rest, *(mh_float2*)&a0) * inv;
        move_a((float*)&z);
    } else if (c5) {
        move_a((float*)&f80);
        c5 = 0;
    }

    if (dirty) {
        rgl::vector4 v;
        get_xy((float*)&v);
    }
}

// @ 0x497880
void UiB::animate(int)
{
    float t1 = mh_elapsed(this, 1);
    float t4 = mh_elapsed(this, 4);
    float t5 = mh_elapsed(this, 5);

    if (t4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - t4;
        mh_float2 z = mh_fade_mix(t4, *(mh_float2*)&f88, rest, *(mh_float2*)&a8) * inv;
        ((rgl::mesh*)tracks[0])->set_color(rgl::vector4(z.x / fD8, z.y / fDC, 1.0f));
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / fD8, z.y / fDC, 1.0f));
        update_bounds();
        set_xy((float*)&mh_float2(z.x * 0.25f, screen[1]));
        fD8 = z.x;
        fDC = z.y;
    } else if (c4) {
        c4 = 0;
        ((rgl::mesh*)tracks[0])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        update_bounds();
        set_xy((float*)&mh_float2(f88 * 0.25f, screen[1]));
        fD8 = f88;
        fDC = f8c;
    }

    if (t1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - t1) + aa().a * t1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
    } else if (c1) {
        c1 = 0;
        unsigned col = aa().dword;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
    }

    if (t5 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - t5;
        mh_float2 z = mh_fade_mix(t5, *(mh_float2*)&f80, rest, *(mh_float2*)&a0) * inv;
        move_a((float*)&z);
    } else if (c5) {
        move_a((float*)&f80);
        c5 = 0;
    }
}

// @ 0x497e30
void UiC::animate(int)
{
    float t1 = mh_elapsed(this, 1);
    float t4 = mh_elapsed(this, 4);
    float t5 = mh_elapsed(this, 5);

    if (t4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - t4;
        mh_float2 z = mh_fade_mix(t4, *(mh_float2*)&f88, rest, *(mh_float2*)&a8) * inv;
        f194 = -23234124.0f;                     // 0xcbb14326: never a real frame, so refresh rebuilds
        rgl::vector4 c(z.x * 0.38940808f, z.y, 1.0f);
        v198 = c;
        ((rgl::mesh*)tracks[0])->set_color(rgl::vector4(z.x / fD8, z.y / fDC, 1.0f));
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / fD8, z.y / fDC, 1.0f));
        update_bounds();
        set_xy((float*)&mh_float2(z.x * 0.25f, screen[1]));
        fD8 = z.x;
        fDC = z.y;
    } else if (c4) {
        rgl::vector4 c(f88 * 0.38940808f, f8c, 1.0f);
        v198 = c;
        c4 = 0;
        f194 = -23234124.0f;
        ((rgl::mesh*)tracks[0])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        update_bounds();
        set_xy((float*)&mh_float2(f88 * 0.25f, screen[1]));
        fD8 = f88;
        fDC = f8c;
    }

    if (t1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - t1) + aa().a * t1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        mask = col;
        f194 = -23234124.0f;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | mask;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | mask;
        col |= color14c & 0xffffff;
        set_color(&col);
    } else if (c1) {
        mask = aa().dword;
        f194 = -23234124.0f;
        c1 = 0;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | mask;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | mask;
        unsigned col = (color14c & 0xffffff) | aa().dword;
        set_color(&col);
    }

    if (t5 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - t5;
        mh_float2 z = mh_fade_mix(t5, *(mh_float2*)&f80, rest, *(mh_float2*)&a0) * inv;
        move_a((float*)&z);
    } else if (c5) {
        move_a((float*)&f80);
        c5 = 0;
    }
}

// @ 0x4985f0
void UiE::animate(int)
{
    unsigned char dirty = 0;
    float e0 = mh_elapsed(this, 0);
    float e1 = mh_elapsed(this, 1);
    float e2 = mh_elapsed(this, 2);
    float e3 = mh_elapsed(this, 3);
    float e4 = mh_elapsed(this, 4);
    float e5 = mh_elapsed(this, 5);

    if (e4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - e4;
        mh_float2 z = mh_fade_mix(e4, *(mh_float2*)&f88, rest, *(mh_float2*)&a8) * inv;
        // DropDown::animate's shape with two meshes; both y quotients parenthesised (the only pair of the grid that matches).
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / fD8, (z.y / fDC), 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(z.x / fD8, (z.y / fDC), 1.0f));
        set_size((float*)&mh_float2(z.y * 0.02f, z.y * 0.02f));
        set_offset((float*)&mh_float2(0.0f, z.y * 0.009f));
        update_bounds();
        fD8 = z.x;
        fDC = z.y;
        dirty = 1;
    } else if (c4) {
        c4 = 0;
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        set_size((float*)&mh_float2(f8c * 0.02f, f8c * 0.02f));
        set_offset((float*)&mh_float2(0.0f, f8c * 0.009f));
        update_bounds();
        fD8 = f88;
        fDC = f8c;
        dirty = 1;
    }

    if (e0 < dur(0)) {
        unsigned col = 0;
        float v = ((cb().dword & 0xff) * (dur(0) - e0) + (ca().dword & 0xff) * e0) * (1.0f / dur(0));
        int r = round_float(&v);
        col = r;
        float v1 = (cb().g * (dur(0) - e0) + ca().g * e0) * (1.0f / dur(0));
        int g = round_float(&v1);
        col += g << 8;
        float v2 = (cb().b * (dur(0) - e0) + ca().b * e0) * (1.0f / dur(0));
        int b = round_float(&v2);
        col += b << 16;
        col |= color14c & 0xff000000;
        set_color(&col);
        dirty = 1;
    } else if (c0) {
        c0 = 0;
        unsigned col = (color14c & 0xff000000) | (ca().dword & 0xffffff);
        set_color(&col);
        dirty = 1;
    }

    if (e1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
        dirty = 1;
    } else if (c1) {
        c1 = 0;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | aa().dword;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | aa().dword;
        unsigned col = (aa().dword & 0xff000000) | (color14c & 0xffffff);
        set_color(&col);
        dirty = 1;
    }

    if (e2 < dur(2)) {
        unsigned col = 0;
        float v = ((cb2().dword & 0xff) * (dur(2) - e2) + (ca2().dword & 0xff) * e2) * (1.0f / dur(2));
        int r = round_float(&v);
        col = r;
        float v1 = (cb2().g * (dur(2) - e2) + ca2().g * e2) * (1.0f / dur(2));
        int g = round_float(&v1);
        col += g << 8;
        float v2 = (cb2().b * (dur(2) - e2) + ca2().b * e2) * (1.0f / dur(2));
        int b = round_float(&v2);
        col += b << 16;
        col |= color150 & 0xff000000;
        set_target(&col);
        dirty = 1;
    } else if (c2) {
        c2 = 0;
        unsigned col = (color150 & 0xff000000) | (ca2().dword & 0xffffff);
        set_target(&col);
        dirty = 1;
    }

    if (e3 < dur(3)) {
        unsigned col = 0;
        float v = (ab2().a * (dur(3) - e3) + aa2().a * e3) * (1.0f / dur(3));
        col = round_float(&v) << 24;
        col |= color150 & 0xffffff;
        set_target(&col);
        dirty = 1;
    } else if (c3) {
        c3 = 0;
        unsigned col = (aa2().dword & 0xff000000) | (color150 & 0xffffff);
        set_target(&col);
        dirty = 1;
    }

    if (e5 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - e5;
        mh_float2 z = mh_fade_mix(e5, *(mh_float2*)&f80, rest, *(mh_float2*)&a0) * inv;
        move_a((float*)&z);
    } else if (c5) {
        move_a((float*)&f80);
        c5 = 0;
    }

    if (dirty) {
        rgl::vector4 v;
        get_xy((float*)&v);
    }
}

// ==== 0x497d10: rebuild the frame mesh from the current frame ===================================================
// @ 0x497d10
void UiC::update_children()
{
    if (tracks[1]) {
        if (f194 == v168)
            return;
    }
    delete tracks[1];
    if (src) {
        rgl::mesh* m = ((rgl::mesh*)(src))->clone();
        float f = (1.0f / screen[0]) * f17c;
        vector4 col = v198;
        ((float*)&col)[f178] = ((1.0f - 2.0f * f) * ((v168 - f16c) / (f170 - f16c)) + f) * ((float*)&col)[f178];
        tracks[1] = m;
        m->set_color(col);
        tracks[1]->color = (tracks[1]->color & 0xffffff) | mask;
        f17c = v198.x * 0.020544001f;            // 0x59272c (0.020544f rounds one ulp short)
    }
    f194 = v168;
}

// @ 0x498390
void UiD::update_children(float arg)
{
    if (tracks[2]) {
        vector4 v(1.0f, 1.0f, 1.0f);
        ((float*)&v)[f178] = screen[f178] / f17c * arg;
        ((rgl::mesh*)tracks[2])->set_color(v);
        f17c = arg * screen[f178];
    }
}

// ==== 0x498420: the scrollbar reload of the outer HUD ================================================================
// @ 0x498420
void HudOuter::scroll_refresh(vector4* arg)
{
    // the scroll object is a container (ui/container.h): its first child takes the new size, then its own text metrics
    // (+0x140). The container is re-read per statement and kids[0] is the real operator[] (0x498441..0x498453).
    ((ContainerBase*)scroll)->kids[0]->set_xy((float*)arg);
    ((ContainerBase*)scroll)->kids[0]->set_size(&((ContainerBase*)scroll)->kids[0]->f140);

    f49c = arg->x;
    f4a0 = arg->y;

    vector4 scale;
    scale.x = 0.5f;
    scale.y = arg->y * 1.5576323f;
    scale.z = 1.0f;
    bar->set_xy((float*)&mh_float2(0.01f, arg->y));       // an 8-byte temporary, as in the animate family
    bar->f17c = (float)(arg->y * 1.5576323f) * 0.31f;    // two fmuls (0x4984ac, 0x4984ba): the cast stops cl folding the constants

    if (bar->tracks[0]) {
        delete bar->tracks[0];
        bar->tracks[0] = 0;
    }
    if (bar->tracks[2]) {
        delete bar->tracks[2];
        bar->tracks[2] = 0;
    }

    // the names are block locals: their address is recomputed for get_object (0x498518) and each dies after its clone
    rgl::mesh* a;
    {
        tstring n("scrollbar");
        a = ((rgl::mesh*)g_resmgr.get_object(&n))->clone();
    }
    a->set_color(scale);
    rgl::mesh* b;
    {
        tstring n("scrollbar_handle");
        b = ((rgl::mesh*)g_resmgr.get_object(&n))->clone();
    }
    b->set_color(scale);
    bar->set_track(0, a);
    bar->set_track(2, b);

    ((FieldPanel*)(void*)this)->update();
}
