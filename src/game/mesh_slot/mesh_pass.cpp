// mesh_pass.cpp: TU 63 "mesh", part 3 (0x47e520..0x47f650): the per-pass render driver of the scene registry object
// (0x5fe898) and the polygon-topology rebuild of the terrain geometry. See MIGRATION.md.
#include "./mesh_slot.h"
#include "../../include/layout32.h"
#include "../terrain_b/nav_data.h"   // NavData::compute_bounds 0x4704b0
#include "../../include/pod_elems.h"
#include "../strat/gui_manager.h"
#include "../../include/renderer.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../render_batch.h"  // render_batch (render 0x47e520), buffer_pair (restore_all 0x4623c0): the canonical classes of the batch and its device buffers
#include "../../include/render_flags.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
char g_scene_dirty;                                 // 0x5e2938 (set when the device must be rebuilt before the next pass)


using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

long __cdecl gfx_restore();                     // 0x4c1b00 (HRESULT: triggers/trigger_vec.cpp)
bool __cdecl rec_less(mesh* const* a, mesh* const* b);   // 0x4622a0 (renderer.cpp: sort predicate of the queued meshes, taken by pointer)

// The callees below are members of the classes that define them (terrain_a/misc.h, terrain_b, game/render_batch.h); those headers bring other
// layouts than this TU's, so only the members are declared, each under the defining class.
// ---- the batch renderer's render pass (render_batch::render, src/game/render_batch.h) --------------------------------------------------

// 0x46ca50: rgl::scene::apply_state, the per-frame Direct3D setup of a scene (src/game/terrain_a/terrain_render.cpp)

static __forceinline E8 mk_e8(int a, int b) { E8 e; e.a = a; e.b = b; return e; }

// @ 0x47e520
bool render_batch::render(rgl::scene* only)
{
    for (int i = 0; i < 5; i++)
        vf[i].restore_all();
    ib.restore_all();

    std::vector<rgl::scene*> tmp;
    if (only == 0)
        tmp = scenes;
    else
        tmp.push_back(only);

    cur_mat = 0;
    cur_part = 0;
    g_render.reset_state();

#define SC(p) (p)
#define Q(p) (*(std::vector<E8>*)&SC(p)->queue)
    for (std::vector<rgl::scene*>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        for (unsigned i = 0; i < SC(*it)->cache.size(); i++) {
            ((mesh*)SC(*it)->cache[i])->slots[0].f4 = 1;
            ((mesh*)SC(*it)->cache[i])->slots[1].f4 = 1;
            ((mesh*)SC(*it)->cache[i])->slots[2].f4 = 1;
            if (SC(*it)->registered == 0 && SC(*it)->active != 0)
                register_scene(SC(*it));
            std::vector<E8>& q = Q(*it);
            E8 e;
            e.m = SC(*it)->cache[i];
            e.b = 0;
            q.push_back(e);
        }
    }

again:
    if (g_scene_dirty) {
        g_scene_dirty = 0;
        gfx_restore();
    }

    for (std::vector<rgl::scene*>::iterator c = tmp.begin(); c != tmp.end(); ++c) {
        for (std::vector<E8>::iterator r = Q(*c).begin(); r != Q(*c).end(); ++r) {
            mesh* x = r->m;
            for (int k = 0; k < 3; k++)
                x->slots[k].f29 = 0;
            if (x->parent) {
                x->parent->slots[0].f29 = 0;
                x->parent->slots[1].f29 = 0;
                x->parent->slots[2].f29 = 0;
            }
        }
    }

    for (int i = 0; i < 5; i++) {
        vf[i].bufs[0].meshes.resize(0);
        vf[i].bufs[1].meshes.resize(0);
    }
    ib.bufs[0].meshes.resize(0);
    ib.bufs[1].meshes.resize(0);

    {
        for (std::vector<rgl::scene*>::iterator f = tmp.begin(); f != tmp.end(); ++f) {
            for (std::vector<E8>::iterator r = Q(*f).begin(); r != Q(*f).end(); ++r) {
                mesh* x = r->m;
                mesh* rec2 = x->local_faces ? x : x->parent;
                if (rec2->slots[1].f29 == 0) {
                    ib.bufs[rec2->slots[1].f4].meshes.push_back(rec2);
                    rec2->slots[1].f29 = 1;
                }
                if (r->m->slots[0].f29 == 0) {
                    vf[r->m->slots[0].f0].bufs[r->m->slots[0].f4].meshes.push_back(r->m);
                    r->m->slots[0].f29 = 1;
                }
            }
        }
    }

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 2; j++)
            if (vf[i].bufs[j].upload(scratch, frame) == (int)0xfff3ebc3)
                goto again;
    for (int j = 0; j < 2; j++)
        if (ib.bufs[j].upload(scratch, frame) == (int)0xfff3ebc3)
            goto again;

    for (std::vector<rgl::scene*>::iterator s = tmp.begin(); s != tmp.end(); ++s) {
        if (SC(*s)->flags & 1)
            continue;
        for (unsigned k = 0; k < Q(*s).size(); k++) {
            if (Q(*s)[k].m->res == 0) {
                tstring ts("default");
                Q(*s)[k].m->res = g_resmgr.get_material(ts);
            }
            Q(*s)[k].m->compute_sortkey();                // 0x412980 (slot 2 of the vtable)
        }
        std::stable_sort(Q(*s).begin(), Q(*s).end(), (bool (__cdecl*)(const E8&, const E8&))rec_less);   // the predicate is the definition's: the same function taking the record pointers
    }

    IDirect3DSurface9* rt = 0;
    IDirect3DSurface9* ds = 0;
    if (g_render.dev->GetRenderTarget(0, &rt) < 0)
        mb_error("GetRenderTarget failed");
    if (g_render.dev->GetDepthStencilSurface(&ds) < 0)
        mb_error("GetDepthStencilSurface failed");
    cur_vf_a = -1;
    cur_ib = -1;

    // ebx / ebp of the original: the previous pass's colour target (starts at &g_render) and depth target (starts at 0);
    // a scene only re-begins the scene when either differs from the previous one's (0x47eb32 / 0x47eb3a)
    const void* last_tex = &g_render;      // a start value no target equals (the renderer object itself)
    texture* last_depth = 0;                // the last pass's depth target (0x47ecad: ebp = o->depth_target)
    for (std::vector<rgl::scene*>::iterator d = tmp.begin(); d != tmp.end(); ++d) {
        if (SC(*d)->target != last_tex || SC(*d)->depth_target != last_depth) {
            if (g_render.pad504)
                g_render.dev->EndScene();
            g_render.dev->BeginScene();
            g_render.pad504 = 1;
        }
        if (SC(*d)->target != last_tex) {
            if (SC(*d)->target != 0) {
                texture* t = SC(*d)->target;
                IDirect3DSurface9* surf = t->kind == 1 ? t->level0 : t->surface;
                if (g_render.dev->SetRenderTarget(0, surf) < 0)
                    mb_error("Unable to set render target texture! Try turning off shadows");
                SC(*d)->target->last_frame = counter;
            } else {
                g_render.dev->SetRenderTarget(0, rt);
            }
        }
        last_tex = SC(*d)->target;
        float bias = 0.0f;
        g_render.dev->SetRenderState((D3DRENDERSTATETYPE)0xc3, *(unsigned*)&bias);
        g_render.dev->SetRenderState((D3DRENDERSTATETYPE)0xaf, *(unsigned*)&bias);
        if (SC(*d)->depth_target != 0 && SC(*d)->depth_target->loaded != 0) {
            texture* d2 = SC(*d)->depth_target;
            if (!(d2->kind == 2)) {
                IDirect3DSurface9* surf = d2->kind == 1 ? d2->level0 : d2->surface;
                g_render.dev->SetDepthStencilSurface(surf);
                bias = 0.0002f;
                g_render.dev->SetRenderState((D3DRENDERSTATETYPE)0xc3, *(unsigned*)&bias);
                bias = 2.0f;
                g_render.dev->SetRenderState((D3DRENDERSTATETYPE)0xaf, *(unsigned*)&bias);
            } else {
                g_render.dev->SetDepthStencilSurface(d2->surface);
            }
            last_depth = SC(*d)->depth_target;
        }
        SC(*d)->apply_state();
        if (!(g_use_shaders)) { draw(SC(*d)); } else { render_scene(SC(*d)); }
    }

    if (last_tex)
        g_render.dev->SetRenderTarget(0, rt);
    rt->Release();

    // restore the saved depth-stencil surface when a pass replaced it; ds stays the surface GetDepthStencilSurface
    // returned (ours overwrote it with the pass's texture record, which Wine's d3d9 asserts on: surface.c:454)
    if (last_depth)
        g_render.dev->SetDepthStencilSurface(ds);
    ds->Release();

    g_render.dev->SetStreamSource(0, 0, 0, 0);
    if (g_render.pad504)
        g_render.dev->EndScene();
    g_render.pad504 = 0;

    for (std::vector<rgl::scene*>::iterator e = tmp.begin(); e != tmp.end(); ++e) {
        scene* o = SC(*e);
        o->registered = 0;
        Q(*e).clear();
        for (unsigned i = 0; i < o->cache.size(); i++)
            ((mesh*)o->cache[i])->clear_geometry();
    }

    if (only == 0)
        scenes.resize(0);
    for (int i = 0; i < 3; i++)
        g_render.dev->SetTexture(i, 0);
    return true;
}
#undef SC
#undef Q

// ---- 0x47ee60: Geom::build ----------------------------------------------------------------------------------------
// Rebuilds the whole polygon topology of a terrain geometry: a per-vertex edge list, the face/edge adjacency of every
// corner, the per-face centroid, the per-edge midpoint, the per-face normal/area and the per-corner weight, then the bounding
// box of NavData's vertices, cells and edges (the cell's plane normal is NavCell::trans, its area scale, its centre pos4,
// the per-side weight dists).
// vector4::normalize(), inlined exactly as 0x407d60 does it (it returns the old length).
static float __forceinline normalize_i(vector4& v)
{
    float len = v.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = inv * v.x;
        v.y = inv * v.y;
        v.z = inv * v.z;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
    return len;
}

// vector4::operator*(const float&), inlined as 0x407cd0 does it (a value-returning vector4 temp).
static vector4 __forceinline scale_i(const vector4& v, const float& s)
{
    vector4 r;
    r.x = v.x * s;
    r.y = v.y * s;
    r.z = v.z * s;
    return r;
}

// @ 0x47ee60
void NavData::build()
{
    int* counts = new int[verts.size()];
    int** lists = new int*[verts.size()];
    for (unsigned i = 0; i < verts.size(); i++) {
        lists[i] = new int[0x20];
        counts[i] = 0;
    }

    int warned = 0;
    for (unsigned fi = 0; fi < cells.size(); fi++) {
        for (int k = 0; k < cells[fi].npts; k++) {
            int v0 = cells[fi].verts()[k];
            int v1 = cells[fi].verts()[(k + 1) % cells[fi].npts];
            bool found = false;
            for (int j = 0; j < counts[v0]; j++) {
                if (edges[lists[v0][j]].b == v1)
                    continue;
                if (edges[lists[v0][j]].a != v1)
                    continue;
                cells[fi].edge_ids()[k] = lists[v0][j];
                edges[lists[v0][j]].other = fi;
                edges[lists[v0][j]].count = edges[lists[v0][j]].count + 1;
                if (edges[lists[v0][j]].count > 2)
                    edges[lists[v0][j]].count = 2;
                found = true;
            }
            if (!found) {
                NavEdge e;
                e.a = v0;
                e.b = v1;
                edges.push_back(e);
                unsigned idx = edges.size() - 1;
                if (counts[v1] < 0x20) {
                    lists[v1][counts[v1]] = idx;
                    counts[v1]++;
                    edges[idx].cell = fi;
                    edges[idx].other = -1;
                    edges[idx].count = 1;
                    cells[fi].edge_ids()[k] = idx;
                } else if (!warned) {
                    g_gui.add(tstring("WARNING: neighbour faces exceeds the maximum in manifold"), 0xffff8888);
                    warned = 1;
                }
            }
        }
    }

    // edge id -> the face on the other side of it, stored per corner in adj[] (0x47f186..0x47f1af read +0x14+4k, write +0x28+4k)
    for (unsigned fi = 0; fi < cells.size(); fi++) {
        for (int k = 0; k < cells[fi].npts; k++) {
            int f4 = edges[cells[fi].edge_ids()[k]].cell;
            if (f4 == fi)
                cells[fi].targets()[k] = edges[cells[fi].edge_ids()[k]].other;
            else
                cells[fi].targets()[k] = f4;
        }
    }

    // face centroid
    for (unsigned fi = 0; fi < cells.size(); fi++) {
        cells[fi].pos4() = vector4(0.0f, 0.0f, 0.0f);
        for (int k = 0; k < cells[fi].npts; k++) {
            NavCell& g = cells[fi];
            NavVert& v = verts[g.verts()[k]];
            g.pos4().x = g.pos4().x + v.p.x;
            g.pos4().y = v.p.y + g.pos4().y;
            g.pos4().z = v.p.z + g.pos4().z;
        }
        NavCell& h = cells[fi];
        float s = 1.0f / (float)h.npts;
        h.pos4().x = s * h.pos4().x;
        h.pos4().y = s * h.pos4().y;
        h.pos4().z = s * h.pos4().z;
    }

    // edge midpoint
    for (unsigned i = 0; i < edges.size(); i++) {
        NavEdge& e = edges[i];
        NavVert& a = verts[e.a];
        NavVert& b = verts[e.b];
        e.mid = scale_i(vector4(a.p.x + b.p.x, a.p.y + b.p.y, a.p.z + b.p.z), 0.5f);
    }

    // face normal and area
    for (unsigned fi = 0; fi < cells.size(); fi++) {
        NavVert& p1 = verts[cells[fi].verts()[1]];
        NavVert& p0 = verts[cells[fi].verts()[0]];
        float ax = p1.p.x - p0.p.x;
        float ay = p1.p.y - p0.p.y;
        float az = p1.p.z - p0.p.z;
        NavVert& p2 = verts[cells[fi].verts()[2]];
        NavVert& q0 = verts[cells[fi].verts()[0]];
        vector4 b;
        b.x = p2.p.x - q0.p.x;
        b.y = p2.p.y - q0.p.y;
        b.z = p2.p.z - q0.p.z;
        vector4& n = cells[fi].trans;
        n.x = ay * b.z - az * b.y;
        n.y = b.x * az - b.z * ax;
        n.z = b.y * ax - b.x * ay;
        float len = normalize_i(cells[fi].trans);
        cells[fi].scale = fabsf(len) * 0.5f;
        if (cells[fi].npts > 3) {
            NavVert& r2 = verts[cells[fi].verts()[2]];
            NavVert& r0 = verts[cells[fi].verts()[0]];
            float cx = r2.p.x - r0.p.x;
            float cy = r2.p.y - r0.p.y;
            float cz = r2.p.z - r0.p.z;
            NavVert& r3 = verts[cells[fi].verts()[3]];
            NavVert& s0 = verts[cells[fi].verts()[0]];
            b.x = r3.p.x - s0.p.x;
            b.y = r3.p.y - s0.p.y;
            b.z = r3.p.z - s0.p.z;
            vector4 d;
            d.x = cy * b.z - cz * b.y;
            d.y = cz * b.x - cx * b.z;
            d.z = cx * b.y - cy * b.x;
            cells[fi].scale += fabsf(d.length()) * 0.5f;
        }
    }

    compute_bounds();

    for (unsigned fi = 0; fi < cells.size(); fi++) {
        for (int k = 0; k < cells[fi].npts; k++) {
            if (cells[fi].targets()[k] != -1)
                cells[fi].dists()[k] = 1.0f;
        }
    }

    delete[] counts;
    for (unsigned i = 0; i < verts.size(); i++)
        delete[] lists[i];
    delete[] lists;
}
