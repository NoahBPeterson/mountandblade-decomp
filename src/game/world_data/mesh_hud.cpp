//-----------------------------------------------------------------------------
// File: mesh_hud.cpp  (0x4958f0..0x499050)
//
// Desc: rgl::mesh helpers (clone, frame transform, alpha, uv normalise), the slider widget factory, the map-legend mesh and
//       the mission HUD bar widgets (per-frame updates, frame mesh rebuild, scroll bar reload) of translation units 85
//       (rgl_mesh) and 86 (hud2) of the TU map. Names provisional. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./mesh_hud.h"
#include "../config/widget_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../ui/mission_ui.h"
#include "../tstring.h"
#include "../../rgl/mesh.h"
#include "../../rgl/rgl_math.h"
#include "../../include/mesh_parts.h"
#include "../../include/mesh_prims.h"

// ==== 0x4958f0: clone a mesh with the same owner, material, flags and name =====================================
// @ 0x4958f0
MeshX* MeshX::copy_of()
{
    MeshX* m = (MeshX*)new rgl::mesh;
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
// The slider widget (UiB layout, vtable 0x591fb0) is called through the slots the call sites show.
struct SliderVt {
    virtual void s00(); virtual void s01(); virtual void s02();
    virtual void s03();                                         // +0x0c
    virtual void s04(); virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08();
    virtual void s09(); virtual void s10(); virtual void s11(); virtual void s12();
    virtual void s13(float v);                                  // +0x34 set the value
    virtual void s14(); virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual void s19(); virtual void s20(); virtual void s21();
    virtual void s22(void* range);                              // +0x58
    virtual void s23(); virtual void s24();
    virtual void s25(const void* v);                            // +0x64
    virtual void s26(); virtual void s27(); virtual void s28(); virtual void s29(); virtual void s30(); virtual void s31();
    virtual void s32(int slot, void* mesh);                     // +0x80
};
struct SliderW : MissionUi {                                    // 0x188 bytes
    int f178;                                                   // +0x178
    float f17c;                                                 // +0x17c
    int f180;                                                   // +0x180
    int f184;                                                   // +0x184
};
// @ 0x495970
int WidgetMgrW::create_slider_overlay(int vmin, int vmax)
{
    // built by hand: SliderW's constructor would emit a vtable of unresolved MissionUi slots (the real one is 0x591fb0, UiB's)
    SliderW* w = (SliderW*)operator new(sizeof(SliderW));
    if (w) {
        // the slider's own fields belong to its construction (0x495991..0x4959bf, inside the null check)
        new ((void*)w, raw_place()) MissionUi;                  // 0x48ae20
        *(void**)w = (void*)vtbl_591fb0;
        w->f178 = 0;
        w->f17c = 0.008f;                                       // 0x3c03126f
        w->v168 = 1.0f;
        w->f180 = 0;
        w->f184 = 0;
    }
    {
        float pos[2] = { 0.0f, 0.0f };                          // 0x4959d7: both halves stored before the call
        ((SliderVt*)w)->s22(pos);
    }
    {
        tstring n("slider_hor");
        gui_mesh* m = (gui_mesh*)((resmgr*)(void*)&g_resmgr_full)->get_object((rgl::string*)&n);
        void* object = m->clone();
        ((SliderVt*)w)->s32(0, object);
    }
    {
        tstring n("handle_hor");
        gui_mesh* m = (gui_mesh*)((resmgr*)(void*)&g_resmgr_full)->get_object((rgl::string*)&n);
        void* object = m->clone();
        ((SliderVt*)w)->s32(2, object);
    }
    ((SliderVt*)w)->s03();
    {
        float size[2];
        size[0] = 0.25f;
        size[1] = w->screen[1];                                 // 0x495a65 (+0xf4)
        ((SliderVt*)w)->s25(size);
    }
    float fmin = (float)vmin;
    w->f16c = fmin;
    w->f170 = (float)vmax;
    ((SliderVt*)w)->s13(fmin);                                  // 0x495a99: the float's bits are pushed
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
void MeshX::set_part(const rgl::matrix& mtx)
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
    ((mesh_x*)(void*)this)->compute_bounds();            // 0x419300
}

// ==== 0x495f00: set the alpha of every vertex record ===========================================================
// Each vertex record keeps its packed colour at +0x54; that colour is unpacked into floats, the alpha lane is replaced
// by `a`, and it is packed back.
// @ 0x495f00
void MeshX::set_alpha(float a)
{
    ensure_local();
    std::vector<rgl::vertex60>& verts = v50;
    for (unsigned i = 0; i < verts.size(); i++) {
        rgl::vertex60* base = &verts[0];
        unsigned packed = base[i].color;
        rgl::vertex60* p = &base[i];
        rgl::vector4 v;
        v = rgl::unpack_color(&packed);
        v.w = a;
        p->color = rgl::pack_color(&v);
    }
}

// ==== 0x495fa0: rescale `src`'s uv records into this mesh's uv rectangle ==========================================
// @ 0x495fa0
void MeshX::normalize_uv(MeshX* src)
{
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
    float su = (uv(0) - uv(2)) / (u0 - u1);
    float sv = (uv(1) - uv(3)) / (v0 - v1);
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
void HudBar::update_1(int)
{
    float e0 = elapsed(0);
    float e1 = elapsed(1);
    float e2 = elapsed(4);
    float e3 = elapsed(5);

    if (e2 < dur(4)) {
        float k = 1.0f / dur(4);
        float x = (e2 * f88 + (dur(4) - e2) * a8) * k;
        float y = ((dur(4) - e2) * ac + e2 * f8c) * k;
        rgl::vector4 v(x / fD8, y / fDC, 1.0f);
        msh(0)->set_color(v);
        s03();
        fD8 = x;
        fDC = y;
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
        col = round_float(&v);
        v = (cb().g * (dur(0) - e0) + ca().g * e0) * (1.0f / dur(0));
        col += round_float(&v) << 8;
        v = (cb().b * (dur(0) - e0) + ca().b * e0) * (1.0f / dur(0));
        col += round_float(&v) << 16;
        col |= color14c & 0xff000000;
        tracks[0]->color = col;
        set_color(&col);
    } else if (c0) {
        unsigned col = (color14c ^ ca().dword) & 0xffffff ^ color14c;
        c0 = 0;
        tracks[0]->color = col;
        set_color(&col);
    }

    if (e1 < dur(1)) {
        unsigned col = 0;
        float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
        col = round_float(&v) << 24;
        col |= color14c & 0xffffff;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | (col & 0xff000000);
        set_color(&col);
    } else if (c1) {
        unsigned col = aa().dword | (color14c & 0xffffff);
        c1 = 0;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | aa().dword;
        set_color(&col);
    }

    if (e3 < dur(5)) {
        float k = 1.0f / dur(5);
        rgl::vector4 v;
        v.x = (e3 * f80 + (dur(5) - e3) * a0) * k;
        v.y = ((dur(5) - e3) * a4 + e3 * f84) * k;
        s22(&v);
    } else if (c5) {
        s22(&f80);
        c5 = 0;
    }
}

// @ 0x4966b0
void HudBar::update_2(int)
{
    float e0 = elapsed(0);
    float e1 = elapsed(1);
    float e2 = elapsed(4);
    float e3 = elapsed(5);

    if (e2 < dur(4)) {
        float k = 1.0f / dur(4);
        float x = (e2 * f88 + (dur(4) - e2) * a8) * k;
        float y = ((dur(4) - e2) * ac + e2 * f8c) * k;
        msh(2)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        msh(1)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        s03();
        fD8 = x;
        fDC = y;
    } else if (c4) {
        c4 = 0;
        msh(2)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        msh(1)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        s03();
        fD8 = f88;
        fDC = f8c;
    }

    if (e0 < dur(0)) {
        unsigned col = 0;
        float v = ((cb().dword & 0xff) * (dur(0) - e0) + (ca().dword & 0xff) * e0) * (1.0f / dur(0));
        col = round_float(&v);
        v = (cb().g * (dur(0) - e0) + ca().g * e0) * (1.0f / dur(0));
        col += round_float(&v) << 8;
        v = (cb().b * (dur(0) - e0) + ca().b * e0) * (1.0f / dur(0));
        col += round_float(&v) << 16;
        col |= color14c & 0xff000000;
        tracks[2]->color = col;
        tracks[1]->color = col;
        set_color(&col);
    } else if (c0) {
        unsigned col = (color14c ^ ca().dword) & 0xffffff ^ color14c;
        c0 = 0;
        tracks[2]->color = col;
        tracks[1]->color = col;
        set_color(&col);
    }

    {
        unsigned col;
        if (e1 < dur(1)) {
            float v = (ab().a * (dur(1) - e1) + aa().a * e1) * (1.0f / dur(1));
            col = round_float(&v) << 24;
            tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        } else if (c1) {
            col = aa().dword;
            c1 = 0;
            tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        } else {
            goto alpha_done;
        }
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        set_color(&col);
    }
alpha_done:

    if (e3 < dur(5)) {
        float k = 1.0f / dur(5);
        rgl::vector4 v;
        v.x = (e3 * f80 + (dur(5) - e3) * a0) * k;
        v.y = ((dur(5) - e3) * a4 + e3 * f84) * k;
        s22(&v);
    } else if (c5) {
        s22(&f80);
        c5 = 0;
    }
}

// @ 0x496d30
void HudBar::update_3(int)
{
    float e0 = elapsed(0);
    float e1 = elapsed(1);
    float e2 = elapsed(2);
    float e3 = elapsed(3);
    float e4 = elapsed(4);
    float e5 = elapsed(5);
    unsigned char dirty = 0;

    if (e4 < dur(4)) {
        float k = 1.0f / dur(4);
        float x = (e4 * f88 + (dur(4) - e4) * a8) * k;
        float y = ((dur(4) - e4) * ac + e4 * f8c) * k;
        msh(2)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        msh(1)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        msh(3)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        float sy = y * 0.02f;
        rgl::vector4 v(sy, sy, 1.0f);
        s27(&v);
        v.x = sy;
        v.y = y * 0.013f;
        s28(&v);
        s03();
        fD8 = x;
        fDC = y;
        dirty = 1;
    } else if (c4) {
        msh(2)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        msh(1)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        msh(3)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        float sy = f8c * 0.02f;
        rgl::vector4 w(sy, sy, 1.0f);
        s27(&w);
        w.x = f8c * 0.02f;
        w.y = f8c * 0.013f;
        s28(&w);
        s03();
        c4 = 0;
        fD8 = f88;
        fDC = f8c;
        dirty = 1;
    }

    if (e0 < dur(0)) {
        unsigned col = 0;
        float v = ((cb().dword & 0xff) * (dur(0) - e0) + (ca().dword & 0xff) * e0) * (1.0f / dur(0));
        col = round_float(&v);
        v = (cb().g * (dur(0) - e0) + ca().g * e0) * (1.0f / dur(0));
        col += round_float(&v) << 8;
        v = (cb().b * (dur(0) - e0) + ca().b * e0) * (1.0f / dur(0));
        col += round_float(&v) << 16;
        col |= color14c & 0xff000000;
        set_color(&col);
        dirty = 1;
    } else if (c0) {
        unsigned col = (color14c ^ ca().dword) & 0xffffff ^ color14c;
        c0 = 0;
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
        unsigned a = aa().dword;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | a;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | a;
        tracks[3]->color = (tracks[3]->color & 0xffffff) | a;
        unsigned col = (color14c ^ a) & 0xffffff ^ a;
        set_color(&col);
        dirty = 1;
    }

    if (e2 < dur(2)) {
        unsigned col = 0;
        float v = ((cb2().dword & 0xff) * (dur(2) - e2) + (ca2().dword & 0xff) * e2) * (1.0f / dur(2));
        col = round_float(&v);
        v = (cb2().g * (dur(2) - e2) + ca2().g * e2) * (1.0f / dur(2));
        col += round_float(&v) << 8;
        v = (cb2().b * (dur(2) - e2) + ca2().b * e2) * (1.0f / dur(2));
        col += round_float(&v) << 16;
        col |= color150 & 0xff000000;
        s39(&col);
        dirty = 1;
    } else if (c2) {
        unsigned col = (color150 ^ ca2().dword) & 0xffffff ^ color150;
        c2 = 0;
        s39(&col);
        dirty = 1;
    }

    if (e3 < dur(3)) {
        unsigned col = 0;
        float v = (ab2().a * (dur(3) - e3) + aa2().a * e3) * (1.0f / dur(3));
        col = round_float(&v) << 24;
        col |= color150 & 0xffffff;
        s39(&col);
        dirty = 1;
    } else if (c3) {
        unsigned a = aa2().dword;
        unsigned col = (color150 ^ a) & 0xffffff ^ a;
        c3 = 0;
        s39(&col);
        dirty = 1;
    }

    if (e5 < dur(5)) {
        float k = 1.0f / dur(5);
        rgl::vector4 v;
        v.x = (e5 * f80 + (dur(5) - e5) * a0) * k;
        v.y = ((dur(5) - e5) * a4 + e5 * f84) * k;
        s22(&v);
    } else if (c5) {
        s22(&f80);
        c5 = 0;
    }

    if (dirty) {
        rgl::vector4 v;
        s37(&v);
    }
}

// @ 0x497880
void HudBar::update_4(int)
{
    float t1 = elapsed(1);
    float t4 = elapsed(4);
    float t5 = elapsed(5);

    if (t4 < dur(4)) {
        float k = 1.0f / dur(4);
        float x = (t4 * f88 + (dur(4) - t4) * a8) * k;
        float y = (t4 * f8c + (dur(4) - t4) * ac) * k;
        rgl::vector4 v(x / fD8, y / fDC, 1.0f);
        msh(0)->set_color(v);
        msh(2)->set_color(v);
        s03();
        v.x = x * 0.25f;
        v.y = screen[1];
        s25(&v);
        fD8 = x;
        fDC = y;
    } else if (c4) {
        rgl::vector4 v(f88 / fD8, f8c / fDC, 1.0f);
        c4 = 0;
        msh(0)->set_color(v);
        msh(2)->set_color(v);
        s03();
        v.x = f88 * 0.25f;
        v.y = screen[1];
        s25(&v);
        fD8 = f88;
        fDC = f8c;
    }

    if (t1 < dur(1)) {
        float f = (aa().a * t1 + ab().a * (dur(1) - t1)) * (1.0f / dur(1));
        unsigned col = round_float(&f) << 24;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        s38(&col);
    } else if (c1) {
        c1 = 0;
        unsigned col = aa().dword;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        s38(&col);
    }

    if (t5 < dur(5)) {
        float k = 1.0f / dur(5);
        rgl::vector4 v;
        v.x = (t5 * f80 + (dur(5) - t5) * a0) * k;
        v.y = (t5 * f84 + (dur(5) - t5) * a4) * k;
        s22(&v);
    } else if (c5) {
        s22(&f80);
        c5 = 0;
    }
}

// @ 0x497e30
void HudBar::update_5(int)
{
    float t1 = elapsed(1);
    float t4 = elapsed(4);
    float t5 = elapsed(5);

    if (t4 < dur(4)) {
        float k = 1.0f / dur(4);
        float x = (t4 * f88 + (dur(4) - t4) * a8) * k;
        float y = (t4 * f8c + (dur(4) - t4) * ac) * k;
        f194 = -23234124.0f;                     // 0xcbb14326: never a real frame, so refresh rebuilds
        rgl::vector4 c(x * 0.38940808f, y, 1.0f);
        v198 = c;
        msh(0)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        msh(2)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        s03();
        rgl::vector4 s(x * 0.25f, screen[1], 1.0f);
        s25(&s);
        fD8 = x;
        fDC = y;
    } else if (c4) {
        rgl::vector4 c(f88 * 0.38940808f, f8c, 1.0f);
        v198 = c;
        c4 = 0;
        f194 = -23234124.0f;
        msh(0)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        msh(2)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        s03();
        rgl::vector4 s(f88 * 0.25f, screen[1], 1.0f);
        s25(&s);
        fD8 = f88;
        fDC = f8c;
    }

    if (t1 < dur(1)) {
        float f = (aa().a * t1 + ab().a * (dur(1) - t1)) * (1.0f / dur(1));
        unsigned col = round_float(&f) << 24;
        mask = col;
        f194 = -23234124.0f;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        s38(&col);
    } else if (c1) {
        unsigned col = aa().dword;
        mask = col;
        f194 = -23234124.0f;
        c1 = 0;
        tracks[0]->color = (tracks[0]->color & 0xffffff) | col;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        s38(&col);
    }

    if (t5 < dur(5)) {
        float k = 1.0f / dur(5);
        rgl::vector4 v;
        v.x = (t5 * f80 + (dur(5) - t5) * a0) * k;
        v.y = (t5 * f84 + (dur(5) - t5) * a4) * k;
        s22(&v);
    } else if (c5) {
        s22(&f80);
        c5 = 0;
    }
}

// @ 0x4985f0
void HudBar::update_full(int)
{
    float t0 = elapsed(0);
    float t1 = elapsed(1);
    float t2 = elapsed(2);
    float t3 = elapsed(3);
    float t4 = elapsed(4);
    float t5 = elapsed(5);
    bool changed = false;

    // bars: lerp the (x, z) pair over dur(4), then the two companion vectors from its y component
    if (t4 < dur(4)) {
        float k = 1.0f / dur(4);
        float x = (t4 * f88 + (dur(4) - t4) * a8) * k;
        float y = (t4 * f8c + (dur(4) - t4) * ac) * k;
        msh(2)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        msh(1)->set_color(rgl::vector4(x / fD8, y / fDC, 1.0f));
        rgl::vector4 v(y * 0.02f, y * 0.02f, 1.0f);
        s27(&v);
        v.x = 0.0f;
        v.y = y * 0.009f;
        s28(&v);
        s03();
        fD8 = x;
        fDC = y;
        changed = true;
    } else if (c4) {
        c4 = 0;
        msh(2)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        msh(1)->set_color(rgl::vector4(f88 / fD8, f8c / fDC, 1.0f));
        rgl::vector4 v(f8c * 0.02f, f8c * 0.02f, 1.0f);
        s27(&v);
        v.x = 0.0f;
        v.y = f8c * 0.009f;
        s28(&v);
        s03();
        fD8 = f88;
        fDC = f8c;
        changed = true;
    }

    // colour group 1: RGB (from ca()/cb()) with the alpha the widget keeps in color14c
    unsigned col = 0;
    if (t0 < dur(0)) {
        float f = ((ca().dword & 0xff) * t0 + (cb().dword & 0xff) * (dur(0) - t0)) * (1.0f / dur(0));
        col = round_float(&f);
        f = (ca().g * t0 + cb().g * (dur(0) - t0)) * (1.0f / dur(0));
        col += round_float(&f) << 8;
        f = (ca().b * t0 + cb().b * (dur(0) - t0)) * (1.0f / dur(0));
        col += round_float(&f) << 16;
        col |= color14c & 0xff000000;
        s38(&col);
        changed = true;
    } else if (c0) {
        col = (ca().dword & 0xffffff) | (color14c & 0xff000000);
        c0 = 0;
        s38(&col);
        changed = true;
    }

    // colour group 1 alpha: it also tints both bar meshes
    if (t1 < dur(1)) {
        float f = (aa().a * t1 + ab().a * (dur(1) - t1)) * (1.0f / dur(1));
        col = round_float(&f) << 24;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        col |= color14c & 0xffffff;
        s38(&col);
        changed = true;
    } else if (c1) {
        col = aa().dword;
        tracks[2]->color = (tracks[2]->color & 0xffffff) | col;
        tracks[1]->color = (tracks[1]->color & 0xffffff) | col;
        c1 = 0;
        col = (col & 0xff000000) | (color14c & 0xffffff);
        s38(&col);
        changed = true;
    }

    // colour group 2: the same two passes with the second colour pair and the second kept word (color150)
    if (t2 < dur(2)) {
        float f = ((ca2().dword & 0xff) * t2 + (cb2().dword & 0xff) * (dur(2) - t2)) * (1.0f / dur(2));
        col = round_float(&f);
        f = (ca2().g * t2 + cb2().g * (dur(2) - t2)) * (1.0f / dur(2));
        col += round_float(&f) << 8;
        f = (ca2().b * t2 + cb2().b * (dur(2) - t2)) * (1.0f / dur(2));
        col += round_float(&f) << 16;
        col |= color150 & 0xff000000;
        s39(&col);
        changed = true;
    } else if (c2) {
        col = (ca2().dword & 0xffffff) | (color150 & 0xff000000);
        c2 = 0;
        s39(&col);
        changed = true;
    }

    if (t3 < dur(3)) {
        float f = (aa2().a * t3 + ab2().a * (dur(3) - t3)) * (1.0f / dur(3));
        col = (color150 & 0xffffff) | (round_float(&f) << 24);
        s39(&col);
        changed = true;
    } else if (c3) {
        c3 = 0;
        col = aa2().dword;
        col = (col & 0xff000000) | (color150 & 0xffffff);
        s39(&col);
        changed = true;
    }

    // position: lerp the (x, y) pair over dur(5)
    if (t5 < dur(5)) {
        float k = 1.0f / dur(5);
        rgl::vector4 v;
        v.x = (t5 * f80 + (dur(5) - t5) * a0) * k;
        v.y = (t5 * f84 + (dur(5) - t5) * a4) * k;
        s22(&v);
    } else if (c5) {
        s22(&f80);
        c5 = 0;
    }

    if (changed) {
        rgl::vector4 v;
        s37(&v);
    }
}

// ==== 0x497d10: rebuild the frame mesh from the current frame ===================================================
// @ 0x497d10
void HudBar::rebuild_frame()
{
    if (tracks[1]) {
        if (f194 == v168)
            return;
    }
    delete tracks[1];
    if (src) {
        gui_mesh* m = clone_gui(src);
        float f = (1.0f / screen[0]) * f17c;
        vector4 col = v198;
        ((float*)&col)[idx] = ((1.0f - 2.0f * f) * ((v168 - f16c) / (f170 - f16c)) + f) * ((float*)&col)[idx];
        tracks[1] = m;
        m->set_color(col);
        tracks[1]->color = (tracks[1]->color & 0xffffff) | mask;
        f17c = v198.x * 0.020544001f;            // 0x59272c (0.020544f rounds one ulp short)
    }
    f194 = v168;
}

// @ 0x498390
void HudBar::scale_lane(float arg)
{
    if (tracks[2]) {
        vector4 v(1.0f, 1.0f, 1.0f);
        ((float*)&v)[idx] = screen[idx] / f17c * arg;
        ((gui_mesh*)tracks[2])->set_color(v);
        f17c = arg * screen[idx];
    }
}

// ==== 0x498420: the scrollbar reload of the outer HUD ================================================================
// @ 0x498420
void HudOuter::scroll_refresh(vector4* arg)
{
    ScrollVt* t = *scroll->target;
    t->t25(arg);
    t->t27((char*)t + 0x140);

    f49c = arg->x;
    f4a0 = arg->y;

    vector4 v1;
    v1.x = 0.01f;
    v1.y = arg->y;
    bar->s25(&v1);
    bar->f17c = (float)(arg->y * 1.5576323f) * 0.31f;    // two fmuls (0x4984ac, 0x4984ba): the cast stops cl folding the constants

    delete bar->tracks[0];
    bar->tracks[0] = 0;
    delete bar->tracks[2];
    bar->tracks[2] = 0;

    vector4 scale(0.5f, arg->y * 1.5576323f, 1.0f);
    // the names are temporaries: each dies right after its clone (0x498534, 0x49858a)
    gui_mesh* a = ((gui_mesh*)((resmgr*)(void*)&g_resmgr_full)->get_object((rgl::string*)&tstring("scrollbar")))->clone();
    a->set_color(scale);
    gui_mesh* b = ((gui_mesh*)((resmgr*)(void*)&g_resmgr_full)->get_object((rgl::string*)&tstring("scrollbar_handle")))->clone();
    b->set_color(scale);
    bar->s32(0, a);
    bar->s32(2, b);

    ((FieldPanel*)(void*)this)->update();            // 0x48d470
}
