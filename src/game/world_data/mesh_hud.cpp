//-----------------------------------------------------------------------------
// File: mesh_hud.cpp  (0x4958f0..0x499050)
//
// Desc: rgl::mesh helpers (clone, frame transform, alpha, uv normalise), the slider widget factory, the map-legend mesh and
//       the mission HUD bar widgets (per-frame updates, frame mesh rebuild, scroll bar reload) of translation units 85
//       (rgl_mesh) and 86 (hud2) of the TU map. Names provisional. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../../rgl/scene/scene_entity.h"   // rgl::entity::add_mesh 0x442110
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
static inline V2 operator*(const vec2& a, float k) { return V2(a.x * k, a.y * k); }
static inline V2 mh_fade_mix(float s, const vec2& a, float t, const vec2& b)
{
    V2 l(s * a.x, s * a.y);
    V2 r(t * b.x, t * b.y);
    return V2(l.x + r.x, l.y + r.y);
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
        ++m->res->refcount;
    m->flags = flags;
    m->color = color;
    m->color2 = color2;
    m->local_faces = 0;
    m->local_points = 0;
    m->name = name;
    m->nbones = nbones;
    return m;
}

// ==== 0x495970: build the horizontal slider widget and append it to the manager ================================
// The slider widget is a UiB (ui/ui_widgets.h, vtable 0x591fb0; the original inlines its constructor 0x48b4b0 here).
// @ 0x495970
int widget_mgr::create_slider_overlay(int vmin, int vmax)
{
    UiB* w = new UiB;
    w->move_a(V2(0.0f, 0.0f));
    {
        tstring n("slider_hor");
        w->set_track(0, ((rgl::mesh*)g_resmgr.get_object(&n))->clone());
    }
    {
        tstring n("handle_hor");
        w->set_track(2, ((rgl::mesh*)g_resmgr.get_object(&n))->clone());
    }
    w->update_bounds();
    w->set_xy(V2(0.25f, w->rect_max.y));                         // 0x495a65 (+0xf4)
    float fmin = (float)vmin;
    w->f16c = fmin;
    w->f170 = (float)vmax;
    w->set168(fmin);                                            // 0x495a99: the float's bits are pushed
    scene->add_child(w);
    widgets.push_back(w);
    return (int)(widgets.size() - 1);
}

// ==== 0x495b20: build the map-legend mesh ==========================================================================
// @ 0x495b20
rgl::mesh* __stdcall build_legend_mesh(rgl::entity* parent)
{
    rgl::mesh* m = new rgl::mesh;
    m->billboard = 2;
    {
        rgl::string n("map_legend");
        m->res = g_resmgr.get_material(n);
    }
    m->mask = 1;
    m->slots[0].f4 = 1;
    m->slots[1].f4 = 1;
    parent->add_mesh(m);                       // 0x442110
    return m;
}

// ==== 0x495ba0: rotate the position list, the vertex normals and every key frame ====================================
// Takes a rotation frame (the s / f / u rows) by reference. Each vector goes through an inline rotation helper that
// returns a vector4 built from three floats, so the w lane of every rebuilt vector is whatever the temporary carries
// (the original never writes it; the out-of-line twin of the helper is rgl::matrix::rot 0x40a9c0). The vectors are
// indexed per use (real operator[], real size() in the loop tests), as the original's reloads of the vector's first
// pointer show (0x495bc0, 0x495c41, 0x495ce5, 0x495d80).
// Left: in the first expansion only (pos[i].a) cl sums x as (f, u, s) and z as (u, s, f) where the original has
// (f, s, u) and (s, u, f): an operand order decided by symbol keys.
static inline rgl::vector4 mh_rot(const rgl::matrix& m, const rgl::vector4& p)
{
    return rgl::vector4(m.s.x * p.x + m.f.x * p.y + m.u.x * p.z,
                        m.s.y * p.x + m.f.y * p.y + m.u.y * p.z,
                        m.s.z * p.x + m.f.z * p.y + m.u.z * p.z);
}

// @ 0x495ba0
void rgl::mesh::set_part(const rgl::matrix& mtx)
{
    ensure_local();
    for (unsigned i = 0; i < points.size(); i++) {
        points[i].a = mh_rot(mtx, points[i].a);
        points[i].b = mh_rot(mtx, points[i].b);
    }
    for (unsigned i = 0; i < vertices.size(); i++)
        vertices[i].normal = mh_rot(mtx, vertices[i].normal);
    for (int i = 0; i < kf.count; i++) {
        for (unsigned j = 0; j < kf.frames[i].verts.size(); j++)
            kf.frames[i].verts[j] = mh_rot(mtx, kf.frames[i].verts[j]);
        for (unsigned j = 0; j < kf.frames[i].norms.size(); j++)
            kf.frames[i].norms[j] = mh_rot(mtx, kf.frames[i].norms[j]);
    }
    slots[0].f28 = 1;
    compute_bounds();            // 0x419300
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
    for (unsigned i = 0; i < vertices.size(); i++) {
        unsigned packed = vertices[i].color;
        rgl::vertex60* p = &vertices[i];
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
    std::vector<rgl::point20>::iterator p = src->points.begin();
    std::vector<rgl::point20>::iterator end = src->points.end();
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
    float su = (rect_max.x - rect_min.x) / (u0 - u1);
    float sv = (rect_max.y - rect_min.y) / (v0 - v1);
    float sm = (sv + su) * 0.5f;
    std::vector<rgl::point20>::iterator q = src->points.begin();
    if (q == end)
        return;
    for (;;) {
        float x = q->a.x, y = q->a.y, z = q->a.z;
        q->a.x = x * su;
        q->a.y = y * sv;
        q->a.z = z * sm;
        q->b = q->a;
        ++q;
        if (q == src->points.end())
            break;
    }
}

// ==== the HUD bar widget ==========================================================================================
// @ 0x496100
void MeshWidget::animate(int)
{
    float e0 = elapsed(0);
    float e1 = elapsed(1);
    float e2 = elapsed(4);
    float e3 = elapsed(5);

    if (e2 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - e2;
        V2 z = mh_fade_mix(e2, anim_size_to, rest, anim_size_from) * inv;
        rgl::vector4 v(z.x / cur_size.x, z.y / cur_size.y, 1.0f);
        msh(0)->set_color(v);
        update_bounds();
        cur_size.x = z.x;
        cur_size.y = z.y;
    } else if (c4) {
        c4 = 0;
        msh(0)->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        update_bounds();
        cur_size.x = anim_size_to.x;
        cur_size.y = anim_size_to.y;
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
        set_color(col);
    } else if (c0) {
        c0 = 0;
        unsigned col = (color14c & 0xff000000) | (ca().dword & 0xffffff);
        tracks[0]->color = col;
        set_color(col);
    }

    if (e1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(col);
    } else if (c1) {
        c1 = 0;
        unsigned col = aa().dword;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(col);
    }

    if (e3 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - e3;
        V2 z = mh_fade_mix(e3, anim_pos_to, rest, anim_pos_from) * inv;
        move_a(z);
    } else if (c5) {
        move_a(anim_pos_to);
        c5 = 0;
    }
}

// @ 0x4966b0
void XyzButton::animate(int)
{
    float e0 = elapsed(0);
    float e1 = elapsed(1);
    float e2 = elapsed(4);
    float e3 = elapsed(5);

    if (e2 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - e2;
        V2 z = mh_fade_mix(e2, anim_size_to, rest, anim_size_from) * inv;
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / cur_size.x, z.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(z.x / cur_size.x, z.y / cur_size.y, 1.0f));
        update_bounds();
        cur_size.x = z.x;
        cur_size.y = z.y;
    } else if (c4) {
        c4 = 0;
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        update_bounds();
        cur_size.x = anim_size_to.x;
        cur_size.y = anim_size_to.y;
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
        set_color(col);
    } else if (c0) {
        unsigned col = (color14c & 0xff000000) | (ca().dword & 0xffffff);
        c0 = 0;
        tracks[2]->color = col;
        tracks[1]->color = col;
        set_color(col);
    }

    if (e1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(col);
    } else if (c1) {
        c1 = 0;
        unsigned col = aa().dword;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(col);
    }

    if (e3 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - e3;
        V2 z = mh_fade_mix(e3, anim_pos_to, rest, anim_pos_from) * inv;
        move_a(z);
    } else if (c5) {
        move_a(anim_pos_to);
        c5 = 0;
    }
}

// @ 0x496d30
void DropDown::animate(int)
{
    unsigned char dirty = 0;
    float e0 = elapsed(0);
    float e1 = elapsed(1);
    float e2 = elapsed(2);
    float e3 = elapsed(3);
    float e4 = elapsed(4);
    float e5 = elapsed(5);

    if (e4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - e4;
        V2 z = mh_fade_mix(e4, anim_size_to, rest, anim_size_from) * inv;
        // Two parenthesised y quotients (any two of the three calls match): each is one more tuple, and the scheduler's
        // 81-tuple region must end two tuples before the third set_color call (0x496fa8: its last store precedes the
        // mesh load). The size and offset arguments are 8-byte temporaries, one per call: they open the frame slot the
        // vector temporaries and the later mix temporaries then share (frame 0x38).
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / cur_size.x, (z.y / cur_size.y), 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(z.x / cur_size.x, (z.y / cur_size.y), 1.0f));
        ((rgl::mesh*)tracks[3])->set_color(rgl::vector4(z.x / cur_size.x, z.y / cur_size.y, 1.0f));
        float sy = z.y * 0.02f;
        set_size(V2(sy, sy));
        set_offset(V2(sy, z.y * 0.013f));
        update_bounds();
        cur_size.x = z.x;
        cur_size.y = z.y;
        dirty = 1;
    } else if (c4) {
        c4 = 0;
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[3])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        set_size(V2(anim_size_to.y * 0.02f, anim_size_to.y * 0.02f));
        set_offset(V2(anim_size_to.y * 0.02f, anim_size_to.y * 0.013f));
        update_bounds();
        cur_size.x = anim_size_to.x;
        cur_size.y = anim_size_to.y;
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
        set_color(col);
        dirty = 1;
    } else if (c0) {
        c0 = 0;
        unsigned col = (color14c & 0xff000000) | (ca().dword & 0xffffff);
        set_color(col);
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
        set_color(col);
        dirty = 1;
    } else if (c1) {
        c1 = 0;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | aa().dword;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | aa().dword;
        tracks[3]->color = (tracks[3]->color & 0xffffff) | aa().dword;
        unsigned col = (aa().dword & 0xff000000) | (color14c & 0xffffff);
        set_color(col);
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
        set_hilight_color(col);
        dirty = 1;
    } else if (c2) {
        c2 = 0;
        unsigned col = (color150 & 0xff000000) | (ca2().dword & 0xffffff);
        set_hilight_color(col);
        dirty = 1;
    }

    if (e3 < dur(3)) {
        unsigned col = 0;
        float v = (ab2().a * (dur(3) - e3) + aa2().a * e3) * (1.0f / dur(3));
        col = round_float(&v) << 24;
        col |= color150 & 0xffffff;
        set_hilight_color(col);
        dirty = 1;
    } else if (c3) {
        c3 = 0;
        unsigned col = (aa2().dword & 0xff000000) | (color150 & 0xffffff);
        set_hilight_color(col);
        dirty = 1;
    }

    if (e5 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - e5;
        V2 z = mh_fade_mix(e5, anim_pos_to, rest, anim_pos_from) * inv;
        move_a(z);
    } else if (c5) {
        move_a(anim_pos_to);
        c5 = 0;
    }

    if (dirty)
        get_xy();
}

// @ 0x497880
void UiB::animate(int)
{
    float t1 = elapsed(1);
    float t4 = elapsed(4);
    float t5 = elapsed(5);

    if (t4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - t4;
        V2 z = mh_fade_mix(t4, anim_size_to, rest, anim_size_from) * inv;
        ((rgl::mesh*)tracks[0])->set_color(rgl::vector4(z.x / cur_size.x, z.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / cur_size.x, z.y / cur_size.y, 1.0f));
        update_bounds();
        set_xy(V2(z.x * 0.25f, rect_max.y));
        cur_size.x = z.x;
        cur_size.y = z.y;
    } else if (c4) {
        c4 = 0;
        ((rgl::mesh*)tracks[0])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        update_bounds();
        set_xy(V2(anim_size_to.x * 0.25f, rect_max.y));
        cur_size.x = anim_size_to.x;
        cur_size.y = anim_size_to.y;
    }

    if (t1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - t1) + aa().a * t1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(col);
    } else if (c1) {
        c1 = 0;
        unsigned col = aa().dword;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(col);
    }

    if (t5 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - t5;
        V2 z = mh_fade_mix(t5, anim_pos_to, rest, anim_pos_from) * inv;
        move_a(z);
    } else if (c5) {
        move_a(anim_pos_to);
        c5 = 0;
    }
}

// @ 0x497e30
void UiC::animate(int)
{
    float t1 = elapsed(1);
    float t4 = elapsed(4);
    float t5 = elapsed(5);

    if (t4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - t4;
        V2 z = mh_fade_mix(t4, anim_size_to, rest, anim_size_from) * inv;
        f194 = -23234124.0f;                     // 0xcbb14326: never a real frame, so refresh rebuilds
        rgl::vector4 c(z.x * 0.38940808f, z.y, 1.0f);
        v198 = c;
        ((rgl::mesh*)tracks[0])->set_color(rgl::vector4(z.x / cur_size.x, z.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / cur_size.x, z.y / cur_size.y, 1.0f));
        update_bounds();
        set_xy(V2(z.x * 0.25f, rect_max.y));
        cur_size.x = z.x;
        cur_size.y = z.y;
    } else if (c4) {
        rgl::vector4 c(anim_size_to.x * 0.38940808f, anim_size_to.y, 1.0f);
        v198 = c;
        c4 = 0;
        f194 = -23234124.0f;
        ((rgl::mesh*)tracks[0])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        update_bounds();
        set_xy(V2(anim_size_to.x * 0.25f, rect_max.y));
        cur_size.x = anim_size_to.x;
        cur_size.y = anim_size_to.y;
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
        set_color(col);
    } else if (c1) {
        mask = aa().dword;
        f194 = -23234124.0f;
        c1 = 0;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | mask;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | mask;
        unsigned col = (color14c & 0xffffff) | aa().dword;
        set_color(col);
    }

    if (t5 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - t5;
        V2 z = mh_fade_mix(t5, anim_pos_to, rest, anim_pos_from) * inv;
        move_a(z);
    } else if (c5) {
        move_a(anim_pos_to);
        c5 = 0;
    }
}

// @ 0x4985f0
void UiE::animate(int)
{
    unsigned char dirty = 0;
    float e0 = elapsed(0);
    float e1 = elapsed(1);
    float e2 = elapsed(2);
    float e3 = elapsed(3);
    float e4 = elapsed(4);
    float e5 = elapsed(5);

    if (e4 < dur(4)) {
        float inv = 1.0f / dur(4);
        float rest = dur(4) - e4;
        V2 z = mh_fade_mix(e4, anim_size_to, rest, anim_size_from) * inv;
        // DropDown::animate's shape with two meshes; both y quotients parenthesised (the only pair of the grid that matches).
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(z.x / cur_size.x, (z.y / cur_size.y), 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(z.x / cur_size.x, (z.y / cur_size.y), 1.0f));
        set_size(V2(z.y * 0.02f, z.y * 0.02f));
        set_offset(V2(0.0f, z.y * 0.009f));
        update_bounds();
        cur_size.x = z.x;
        cur_size.y = z.y;
        dirty = 1;
    } else if (c4) {
        c4 = 0;
        ((rgl::mesh*)tracks[2])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        ((rgl::mesh*)tracks[1])->set_color(rgl::vector4(anim_size_to.x / cur_size.x, anim_size_to.y / cur_size.y, 1.0f));
        set_size(V2(anim_size_to.y * 0.02f, anim_size_to.y * 0.02f));
        set_offset(V2(0.0f, anim_size_to.y * 0.009f));
        update_bounds();
        cur_size.x = anim_size_to.x;
        cur_size.y = anim_size_to.y;
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
        set_color(col);
        dirty = 1;
    } else if (c0) {
        c0 = 0;
        unsigned col = (color14c & 0xff000000) | (ca().dword & 0xffffff);
        set_color(col);
        dirty = 1;
    }

    if (e1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(col);
        dirty = 1;
    } else if (c1) {
        c1 = 0;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | aa().dword;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | aa().dword;
        unsigned col = (aa().dword & 0xff000000) | (color14c & 0xffffff);
        set_color(col);
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
        set_hilight_color(col);
        dirty = 1;
    } else if (c2) {
        c2 = 0;
        unsigned col = (color150 & 0xff000000) | (ca2().dword & 0xffffff);
        set_hilight_color(col);
        dirty = 1;
    }

    if (e3 < dur(3)) {
        unsigned col = 0;
        float v = (ab2().a * (dur(3) - e3) + aa2().a * e3) * (1.0f / dur(3));
        col = round_float(&v) << 24;
        col |= color150 & 0xffffff;
        set_hilight_color(col);
        dirty = 1;
    } else if (c3) {
        c3 = 0;
        unsigned col = (aa2().dword & 0xff000000) | (color150 & 0xffffff);
        set_hilight_color(col);
        dirty = 1;
    }

    if (e5 < dur(5)) {
        float inv = 1.0f / dur(5);
        float rest = dur(5) - e5;
        V2 z = mh_fade_mix(e5, anim_pos_to, rest, anim_pos_from) * inv;
        move_a(z);
    } else if (c5) {
        move_a(anim_pos_to);
        c5 = 0;
    }

    if (dirty)
        get_xy();
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
        float f = (1.0f / rect_max.x) * f17c;
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
        ((float*)&v)[f178] = rect_max[f178] / f17c * arg;
        ((rgl::mesh*)tracks[2])->set_color(v);
        f17c = arg * rect_max[f178];
    }
}

// ==== 0x498420: the scrollbar reload of the outer HUD ================================================================
// @ 0x498420
void Camera::scroll_refresh(const vec2& size)
{
    // the scroll object is a container (ui/container.h): its first child takes the new size, then its own text metrics
    // (+0x140). The container is re-read per statement and kids[0] is the real operator[] (0x498441..0x498453).
    target->kids[0]->set_xy(size);
    target->kids[0]->set_size(target->kids[0]->text_scale);

    tail.x = size.x;
    tail.y = size.y;

    vector4 scale;
    scale.x = 0.5f;
    scale.y = size.y * 1.5576323f;
    scale.z = 1.0f;
    follow[1]->set_xy(V2(0.01f, size.y));       // an 8-byte temporary, as in the animate family
    follow[1]->f17c = (float)(size.y * 1.5576323f) * 0.31f;    // two fmuls (0x4984ac, 0x4984ba): the cast stops cl folding the constants

    if (follow[1]->tracks[0]) {
        delete follow[1]->tracks[0];
        follow[1]->tracks[0] = 0;
    }
    if (follow[1]->tracks[2]) {
        delete follow[1]->tracks[2];
        follow[1]->tracks[2] = 0;
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
    follow[1]->set_track(0, a);
    follow[1]->set_track(2, b);

    update();
}
