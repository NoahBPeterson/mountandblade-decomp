// mesh_pass.cpp: TU 63 "mesh", part 3 (0x47e520..0x47f650): the per-pass render driver of the scene registry object
// (0x5fe898) and the polygon-topology rebuild of the terrain geometry. See MIGRATION.md.
#include "./mesh_slot.h"
#include "../../include/pod_elems.h"
#include "../../include/msg_log.h"
#include "../../include/renderer.h"
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

extern IDirect3DDevice9* g_device;              // 0xa17f28
extern char g_scene_dirty;                      // 0x5e2938 (set when the device must be rebuilt before the next pass)
extern char g_scene_open;                       // 0xa17e14 (BeginScene issued)
void gfx_restore();                             // 0x4c1b00
bool __cdecl rec_less(const E8& a, const E8& b);   // 0x4622a0 (sort predicate of the queue records)

// ---- the registry object's render pass ---------------------------------------------------------------------------
// A 0x4c-byte device buffer record seen by its per-record item list (+0x34) and the uploader 0x46d060.
struct bucket_ms {
    char pad0[0x34];
    std::vector<int> v;                         // +0x34
    char pad44[0x4c - 0x44];
    int upload(void* geom, int count);          // 0x46d060
};
struct geompair_ms {                            // 0x98 (src/game/dev_buffer.h's buffer_pair)
    bucket_ms b[2];
    void restore_all();                         // 0x4623c0
};


// The object at 0x5fe898 as this pass sees it: scene_registry (src/rgl/mesh.h) is its small view (scenes at +0x3a4),
// render_batch (src/game/render_batch.h) the one the batch renderer's setup code sees (vertex records at +0x14).
// The queue of scenes of the pass is at +0x3a4 here.
struct RenderPass {
    char pad0[0x14];
    geompair_ms bp[5];                          // +0x14 five 0x98-byte pairs (one per vertex format)
    geompair_ms bp5;                            // +0x30c the index pair
    std::vector<int> v3a4;                      // +0x3a4 scenes queued for the next pass (scene_registry::scenes)
    int f3b4;                                   // +0x3b4
    int f3b8;                                   // +0x3b8
    char pad3bc[0x3c0 - 0x3bc];
    char geom3c0[0x4183c0 - 0x3c0];             // +0x3c0 the geometry-buffer arena
    int f4183c0;                                // +0x4183c0
    int f4183c4;                                // +0x4183c4
    int f4183c8;                                // +0x4183c8
    int f4183cc;                                // +0x4183cc
    int f4183d0;                                // +0x4183d0
    char pad4183d4[0x418828 - 0x4183d4];
    int f418828;                                // +0x418828 (render_batch::counter)

    bool render(int pass);                      // 0x47e520
    void register_obj(scene* o);                // 0x412040 (scene_registry::register_scene)
    void draw_a(scene* o);                      // 0x46e590 (render_batch::render_scene)
    void draw_b(scene* o);                      // 0x46d8c0 (the fixed-function drawer)
};

// 0x46ca50: the per-frame Direct3D setup of a scene (src/game/terrain_a: RenderScene::apply_state)
struct scene_ms : scene {
    void apply_state();                         // 0x46ca50
};

static __forceinline E8 mk_e8(int a, int b) { E8 e; e.a = a; e.b = b; return e; }

// @ 0x47e520
bool RenderPass::render(int pass)
{
    for (int i = 0; i < 5; i++)
        bp[i].restore_all();
    bp5.restore_all();

    std::vector<int> tmp;
    if (pass == 0)
        tmp = v3a4;
    else
        tmp.push_back(pass);

    f4183c8 = 0;
    f4183d0 = 0;
    g_render.reset_state();

#define SC(p) ((scene_ms*)(p))
#define Q(p) (*(std::vector<E8>*)&SC(p)->queue)
    for (std::vector<int>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
        for (unsigned i = 0; i < SC(*it)->cache.size(); i++) {
            ((mesh*)SC(*it)->cache[i])->slots[0].f4 = 1;
            ((mesh*)SC(*it)->cache[i])->slots[1].f4 = 1;
            ((mesh*)SC(*it)->cache[i])->slots[2].f4 = 1;
            if (SC(*it)->registered == 0 && SC(*it)->active != 0)
                register_obj(SC(*it));
            Q(*it).push_back(mk_e8((int)SC(*it)->cache[i], 0));
        }
    }

again:
    if (g_scene_dirty) {
        g_scene_dirty = 0;
        gfx_restore();
    }

    for (std::vector<int>::iterator c = tmp.begin(); c != tmp.end(); ++c) {
        for (std::vector<E8>::iterator r = Q(*c).begin(); r != Q(*c).end(); ++r) {
            mesh* x = (mesh*)r->a;
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
        bp[i].b[0].v.resize(0);
        bp[i].b[1].v.resize(0);
    }
    bp5.b[0].v.resize(0);
    bp5.b[1].v.resize(0);

    {
        bucket_ms* buckets10 = &bp5.b[0];
        bucket_ms* buckets0 = &bp[0].b[0];
        for (std::vector<int>::iterator f = tmp.begin(); f != tmp.end(); ++f) {
            for (std::vector<E8>::iterator r = Q(*f).begin(); r != Q(*f).end(); ++r) {
                mesh* x = (mesh*)r->a;
                mesh* rec2 = x->local_a0 ? x : x->parent;
                if (rec2->slots[1].f29 == 0) {
                    buckets10[rec2->slots[1].f4].v.push_back((int)rec2);
                    rec2->slots[1].f29 = 1;
                }
                if (((mesh*)r->a)->slots[0].f29 == 0) {
                    buckets0[((mesh*)r->a)->slots[0].f4 + ((mesh*)r->a)->slots[0].f0 * 2].v.push_back(r->a);
                    ((mesh*)r->a)->slots[0].f29 = 1;
                }
            }
        }
    }

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 2; j++)
            if (bp[i].b[j].upload(geom3c0, f4183c0) == (int)0xfff3ebc3)
                goto again;
    for (int j = 0; j < 2; j++)
        if (bp5.b[j].upload(geom3c0, f4183c0) == (int)0xfff3ebc3)
            goto again;

    for (std::vector<int>::iterator s = tmp.begin(); s != tmp.end(); ++s) {
        if (SC(*s)->flags & 1)
            continue;
        for (unsigned k = 0; k < Q(*s).size(); k++) {
            if (((mesh*)Q(*s)[k].a)->res == 0) {
                tstring ts("default");
                ((mesh*)Q(*s)[k].a)->res = g_resmgr.get_material(*(rgl::string*)&ts);
                ts.~tstring();
            }
            ((mesh*)Q(*s)[k].a)->compute_sortkey();                // 0x412980 (slot 2 of the vtable)
        }
        std::stable_sort(Q(*s).begin(), Q(*s).end(), rec_less);
    }

    IDirect3DSurface9* rt = 0;
    IDirect3DSurface9* ds = 0;
    if (g_device->GetRenderTarget(0, &rt) < 0)
        mb_error("GetRenderTarget failed");
    if (g_device->GetDepthStencilSurface(&ds) < 0)
        mb_error("GetDepthStencilSurface failed");
    f3b4 = -1;
    f3b8 = -1;

    texture* last_tex = 0;
    for (std::vector<int>::iterator d = tmp.begin(); d != tmp.end(); ++d) {
        scene_ms* o = SC(*d);
        if (o->target != (texture*)&g_render || o->f15c != 0) {
            if (g_scene_open)
                g_device->EndScene();
            g_device->BeginScene();
            g_scene_open = 1;
        }
        if (o->target != (texture*)&g_render) {
            if (o->target != 0) {
                texture* t = o->target;
                IDirect3DSurface9* surf = t->kind == 1 ? t->level0 : t->surface;
                if (g_device->SetRenderTarget(0, surf) < 0)
                    mb_error("Unable to set render target texture! Try turning off shadows");
                o->target->f50 = f418828;
            } else {
                g_device->SetRenderTarget(0, rt);
            }
        }
        last_tex = o->target;
        float bias = 0.0f;
        g_device->SetRenderState((D3DRENDERSTATETYPE)0xc3, *(unsigned*)&bias);
        g_device->SetRenderState((D3DRENDERSTATETYPE)0xaf, *(unsigned*)&bias);
        if (o->f15c != 0 && ((texture*)o->f15c)->loaded != 0) {
            texture* d2 = (texture*)o->f15c;
            if (d2->kind == 2) {
                g_device->SetDepthStencilSurface(d2->surface);
            } else {
                IDirect3DSurface9* surf = d2->kind == 1 ? d2->level0 : d2->surface;
                g_device->SetDepthStencilSurface(surf);
                bias = 0.0002f;
                g_device->SetRenderState((D3DRENDERSTATETYPE)0xc3, *(unsigned*)&bias);
                bias = 2.0f;
                g_device->SetRenderState((D3DRENDERSTATETYPE)0xaf, *(unsigned*)&bias);
            }
            ds = (IDirect3DSurface9*)o->f15c;
        }
        o->apply_state();
        if (g_use_shaders)
            draw_a(o);
        else
            draw_b(o);
    }

    if (last_tex)
        g_device->SetRenderTarget(0, rt);
    rt->Release();

    if (ds)
        g_device->SetDepthStencilSurface(ds);
    ds->Release();

    g_device->SetStreamSource(0, 0, 0, 0);
    if (g_scene_open)
        g_device->EndScene();
    g_scene_open = 0;

    for (std::vector<int>::iterator e = tmp.begin(); e != tmp.end(); ++e) {
        scene_ms* o = SC(*e);
        Q(*e).clear();
        o->registered = 0;
        for (unsigned i = 0; i < o->cache.size(); i++)
            ((mesh_ext*)o->cache[i])->clear_geometry();
    }

    if (pass == 0)
        v3a4.resize(0);
    for (int i = 0; i < 3; i++)
        g_device->SetTexture(i, 0);
    return true;
}
#undef SC
#undef Q

// ---- 0x47ee60: Geom::build ----------------------------------------------------------------------------------------
// Rebuilds the whole polygon topology of a terrain geometry: a per-vertex edge list, the face/edge adjacency of every
// corner, the per-face centroid, the per-edge midpoint, the per-face normal/area and the per-corner weight, then the bounding
// box. The object is the point set + polygon + edge containers of terrain_b/poly_mesh.h seen with the element records of this
// function (the 0x24-byte point of the vector at +0, the 0x88-byte face (rec136) at +0x10, the 0x24-byte edge at +0x20);
// terrain_b's PointSet / NavCore have the same container offsets with their own element types.
struct GeomPt {                         // 0x24 bytes: position at +0 (E36x of the vector instantiations)
    float px, py, pz;                   // +0x00
    int f0c;                            // +0x0c
    char pad10[0x10];                   // +0x10
    float f20;                          // +0x20
};
struct GeomFace {                       // 0x88 bytes (rec136 / E136)
    int n;                              // +0x00 corner count
    int v[4];                           // +0x04 point ids
    int e[4];                           // +0x14 edge id, then neighbour face id
    int f24;                            // +0x24
    int adj[4];                         // +0x28 per-corner id (-1: none)
    float w[4];                         // +0x38 per-corner weight
    int f48, f4c, f50;                  // +0x48
    vector4 normal;                     // +0x54
    float area;                         // +0x64
    unsigned f68;                       // +0x68
    int f6c, f70;                       // +0x6c
    vector4 center;                     // +0x74 centroid
    int f84;                            // +0x84
};
struct GeomEdge {                       // 0x24-byte directed edge (E36)
    int count;                          // +0x00 number of adjacent faces
    int f4;                             // +0x04 face that created the edge
    int f8;                             // +0x08 the other adjacent face (-1: none)
    int f0c;                            // +0x0c start point
    int f10;                            // +0x10 end point
    vector4 mid;                        // +0x14 midpoint
};
struct PolyGeom {
    std::vector<GeomPt> pts;            // +0x00
    std::vector<GeomFace> elems;        // +0x10
    std::vector<GeomEdge> edges;        // +0x20
    char pad30[0x14];                   // +0x30 a fourth vector triple (unused here)
    vector4 bmin;                       // +0x44
    vector4 bmax;                       // +0x54
    void build();                       // 0x47ee60
    void compute_bounds();              // 0x4704b0
};

#define OFFSET_CHECK(T, m, off) typedef char T##_##m##_at_##off[(offsetof(T, m) == (off)) ? 1 : -1]
OFFSET_CHECK(GeomPt, f0c, 0x0c);
OFFSET_CHECK(GeomPt, f20, 0x20);
OFFSET_CHECK(GeomFace, adj, 0x28);
OFFSET_CHECK(GeomFace, w, 0x38);
OFFSET_CHECK(GeomFace, normal, 0x54);
OFFSET_CHECK(GeomFace, area, 0x64);
OFFSET_CHECK(GeomFace, center, 0x74);
OFFSET_CHECK(GeomFace, f84, 0x84);
OFFSET_CHECK(GeomEdge, mid, 0x14);
OFFSET_CHECK(PolyGeom, elems, 0x10);
OFFSET_CHECK(PolyGeom, edges, 0x20);
OFFSET_CHECK(PolyGeom, bmin, 0x44);
OFFSET_CHECK(PolyGeom, bmax, 0x54);

struct MsgLogAdd : MsgLog {
    void add(tstring msg, int color);           // 0x452c00 (thiscall, ret 0x14)
};
extern MsgLogAdd g_msglog;                                          // 0x5d7080

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
void PolyGeom::build()
{
    int* counts = new int[pts.size()];
    int** lists = new int*[pts.size()];
    for (unsigned i = 0; i < pts.size(); i++) {
        lists[i] = new int[0x20];
        counts[i] = 0;
    }

    int warned = 0;
    for (unsigned fi = 0; fi < elems.size(); fi++) {
        for (int k = 0; k < elems[fi].n; k++) {
            int v0 = elems[fi].v[k];
            int v1 = elems[fi].v[(k + 1) % elems[fi].n];
            bool found = false;
            for (int j = 0; j < counts[v0]; j++) {
                if (edges[lists[v0][j]].f10 == v1)
                    continue;
                if (edges[lists[v0][j]].f0c != v1)
                    continue;
                elems[fi].e[k] = lists[v0][j];
                edges[lists[v0][j]].f8 = fi;
                edges[lists[v0][j]].count = edges[lists[v0][j]].count + 1;
                if (edges[lists[v0][j]].count > 2)
                    edges[lists[v0][j]].count = 2;
                found = true;
            }
            if (!found) {
                GeomEdge e;
                e.f0c = v0;
                e.f10 = v1;
                edges.push_back(e);
                unsigned idx = edges.size() - 1;
                if (counts[v1] < 0x20) {
                    lists[v1][counts[v1]] = idx;
                    counts[v1]++;
                    edges[idx].f4 = fi;
                    edges[idx].f8 = -1;
                    edges[idx].count = 1;
                    elems[fi].e[k] = idx;
                } else if (!warned) {
                    g_msglog.add(tstring("WARNING: neighbour faces exceeds the maximum in manifold"), 0xffff8888);
                    warned = 1;
                }
            }
        }
    }

    // edge id -> the face on the other side of it
    for (unsigned fi = 0; fi < elems.size(); fi++) {
        for (int k = 0; k < elems[fi].n; k++) {
            int f4 = edges[elems[fi].e[k]].f4;
            if (f4 == fi)
                elems[fi].e[k] = edges[elems[fi].e[k]].f8;
            else
                elems[fi].e[k] = f4;
        }
    }

    // face centroid
    for (unsigned fi = 0; fi < elems.size(); fi++) {
        elems[fi].center = vector4(0.0f, 0.0f, 0.0f);
        for (int k = 0; k < elems[fi].n; k++) {
            GeomFace& g = elems[fi];
            GeomPt& v = pts[g.v[k]];
            g.center.x = g.center.x + v.px;
            g.center.y = v.py + g.center.y;
            g.center.z = v.pz + g.center.z;
        }
        GeomFace& h = elems[fi];
        float s = 1.0f / (float)h.n;
        h.center.x = s * h.center.x;
        h.center.y = s * h.center.y;
        h.center.z = s * h.center.z;
    }

    // edge midpoint
    for (unsigned i = 0; i < edges.size(); i++) {
        GeomEdge& e = edges[i];
        GeomPt& a = pts[e.f0c];
        GeomPt& b = pts[e.f10];
        e.mid = scale_i(vector4(a.px + b.px, a.py + b.py, a.pz + b.pz), 0.5f);
    }

    // face normal and area
    for (unsigned fi = 0; fi < elems.size(); fi++) {
        GeomPt& p1 = pts[elems[fi].v[1]];
        GeomPt& p0 = pts[elems[fi].v[0]];
        float ax = p1.px - p0.px;
        float ay = p1.py - p0.py;
        float az = p1.pz - p0.pz;
        GeomPt& p2 = pts[elems[fi].v[2]];
        GeomPt& q0 = pts[elems[fi].v[0]];
        vector4 b;
        b.x = p2.px - q0.px;
        b.y = p2.py - q0.py;
        b.z = p2.pz - q0.pz;
        vector4& n = elems[fi].normal;
        n.x = ay * b.z - az * b.y;
        n.y = b.x * az - b.z * ax;
        n.z = b.y * ax - b.x * ay;
        float len = normalize_i(elems[fi].normal);
        elems[fi].area = fabsf(len) * 0.5f;
        if (elems[fi].n > 3) {
            GeomPt& r2 = pts[elems[fi].v[2]];
            GeomPt& r0 = pts[elems[fi].v[0]];
            float cx = r2.px - r0.px;
            float cy = r2.py - r0.py;
            float cz = r2.pz - r0.pz;
            GeomPt& r3 = pts[elems[fi].v[3]];
            GeomPt& s0 = pts[elems[fi].v[0]];
            b.x = r3.px - s0.px;
            b.y = r3.py - s0.py;
            b.z = r3.pz - s0.pz;
            vector4 d;
            d.x = cy * b.z - cz * b.y;
            d.y = cz * b.x - cx * b.z;
            d.z = cx * b.y - cy * b.x;
            elems[fi].area += fabsf(d.length()) * 0.5f;
        }
    }

    compute_bounds();

    for (unsigned fi = 0; fi < elems.size(); fi++) {
        for (int k = 0; k < elems[fi].n; k++) {
            if (elems[fi].adj[k] != -1)
                elems[fi].w[k] = 1.0f;
        }
    }

    delete[] counts;
    for (unsigned i = 0; i < pts.size(); i++)
        delete[] lists[i];
    delete[] lists;
}
