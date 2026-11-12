//-----------------------------------------------------------------------------
// File: mesh_poly_ray.cpp  (0x52dfd0..0x5305b0)
//
// Desc: one object of mesh_3's range, split where the original's caller / callee objects meet: Mesh::poly_ray and the rest of the map editor mesh object: toggle_flag, scale / transform / remove the selection, make_copy / vertex_index / edge_index / apply_copy, the editor panel update, the view controller apply and the mesh-name chooser.
//       pick_vert / pick_edge / pick_poly call the ray functions as external functions (the original compiled them in
//       separate objects). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "../std_vec_push_back.h"   // 0x52fbc0 calls std::vector<int>::push_back 0x440b30
#include "./mesh_3.h"
#include "../strat/gui_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../../rgl/mesh.h"
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)

#include "../key_names.h"

// Ray / convex-polygon intersection.  The polygon's vertices (at most four) are gathered once,
// their mean gives the plane point, the first three give the normal, and the hit point is then
// tested against every edge (a hit outside the polygon reports 1e6, i.e. "no hit").
// @ 0x52dfd0
void Mesh_s2da20::poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                    float* dist)
{
    const mpoly_s2da20& poly = polys.first[index];
    int n = poly.count;
    rgl::vector4 pts[4];
    rgl::vector4 mean(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < n; i++) {
        pts[i] = verts.first[poly.idx[i]].pos;
        mean.x += pts[i].x;
        mean.y += pts[i].y;
        mean.z += pts[i].z;
    }
    float inv = 1.0f / (float)n;
    mean.x *= inv;
    mean.y *= inv;
    mean.z *= inv;

    rgl::vector4 e1, e2, normal;
    rgl::vec3_sub(e1, pts[1], pts[0]);
    rgl::vec3_sub(e2, pts[2], pts[0]);
    cross(normal, e1, e2);
    normal.normalize();

    // from here on the original calls nothing: the differences, dot and cross products are inline
    float mx = mean.x - p.x, my = mean.y - p.y, mz = mean.z - p.z;
    // every sum below keeps the original's term order (x87 adds round per step)
    if (mx * d.x + mz * d.z + my * d.y < 0.0f) {   // face points away from the ray
        *dist = 1e6f;
        return;
    }
    float t = (normal.z * mz + normal.y * my + normal.x * mx) / (normal.x * d.x + normal.z * d.z + normal.y * d.y);
    rgl::vector4 hit(p.x + d.x * t, p.y + d.y * t, p.z + d.z * t);

    bool outside = false;
    for (int i = 0; i < n && !outside; i++) {
        const rgl::vector4& a = pts[i];
        const rgl::vector4& b = pts[(i + 1) % n];
        float ex = b.x - a.x, ey = b.y - a.y, ez = b.z - a.z;
        float len = (float)sqrt(ez * ez + ey * ey + ex * ex);
        if (len > 1e-10f) {
            float k = 1.0f / len;
            ex = ex * k;
            ey = ey * k;
            ez = ez * k;
        } else {
            ex = 0.0f;
            ey = 1.0f;
            ez = 0.0f;
        }
        float cx = ey * normal.z - ez * normal.y;
        float cy = ez * normal.x - ex * normal.z;
        float cz = ex * normal.y - ey * normal.x;
        if ((hit.z - a.z) * cz + (hit.y - a.y) * cy + (hit.x - a.x) * cx > 0.0f)
            outside = true;
    }
    if (outside) {
        *dist = 1e6f;
        return;
    }
    float qx = pts[0].x - p.x, qy = pts[0].y - p.y, qz = pts[0].z - p.z;
    *dist = (float)sqrt(qz * qz + qy * qy + qx * qx);
}

// ======================================================= slice s0052dfd0: functions
// @ 0x52dfd0
void Mesh_s2dfd0::poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                    float* dist)
{
    const mpoly_s2dfd0& poly = polys[index];
    int n = poly.count;
    rgl::vector4 pts[4];
    rgl::vector4 mean(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < n; i++) {
        pts[i] = verts[poly.idx[i]].pos;
        mean.x += pts[i].x;
        mean.y += pts[i].y;
        mean.z += pts[i].z;
    }
    float inv = 1.0f / (float)n;
    mean.x *= inv;
    mean.y *= inv;
    mean.z *= inv;

    rgl::vector4 e1, e2, normal;
    rgl::vec3_sub(e1, pts[1], pts[0]);
    rgl::vec3_sub(e2, pts[2], pts[0]);
    cross(normal, e1, e2);
    normal.normalize();

    // from here on the original calls nothing: the differences, dot and cross products are inline
    float mx = mean.x - p.x, my = mean.y - p.y, mz = mean.z - p.z;
    // every sum below keeps the original's term order (x87 adds round per step)
    if (mx * d.x + mz * d.z + my * d.y < 0.0f) {   // face points away from the ray
        *dist = 1e6f;
        return;
    }
    float t = (normal.z * mz + normal.y * my + normal.x * mx) / (normal.x * d.x + normal.z * d.z + normal.y * d.y);
    rgl::vector4 hit(p.x + d.x * t, p.y + d.y * t, p.z + d.z * t);

    bool outside = false;
    for (int i = 0; i < n && !outside; i++) {
        const rgl::vector4& a = pts[i];
        const rgl::vector4& b = pts[(i + 1) % n];
        float ex = b.x - a.x, ey = b.y - a.y, ez = b.z - a.z;
        float len = (float)sqrt(ez * ez + ey * ey + ex * ex);
        if (len > 1e-10f) {
            float k = 1.0f / len;
            ex = ex * k;
            ey = ey * k;
            ez = ez * k;
        } else {
            ex = 0.0f;
            ey = 1.0f;
            ez = 0.0f;
        }
        float cx = ey * normal.z - ez * normal.y;
        float cy = ez * normal.x - ex * normal.z;
        float cz = ex * normal.y - ey * normal.x;
        if ((hit.z - a.z) * cz + (hit.y - a.y) * cy + (hit.x - a.x) * cx > 0.0f)
            outside = true;
    }
    if (outside) {
        *dist = 1e6f;
        return;
    }
    float qx = pts[0].x - p.x, qy = pts[0].y - p.y, qz = pts[0].z - p.z;
    *dist = (float)sqrt(qz * qz + qy * qy + qx * qx);
}

// Select/deselect one vertex, edge or polygon.  Only the vertex case updates `selected`.
// @ 0x52e510
void Mesh_s2dfd0::toggle_flag(int type, int index)
{
    if (type == 0) {
        bool flag = verts[index].flag;
        verts[index].flag = !flag;
        if (verts[index].flag)
            selected = index;
        else
            selected = -1;
    } else if (type == 1) {
        bool flag = edges[index].flag;
        edges[index].flag = !flag;
    } else {
        bool flag = polys[index].flag;
        polys[index].flag = !flag;
    }
}

// Scale the selected vertices about their centroid.  The +0x14 selection stamp is recomputed
// first (0x4c54f0); the scale factors are per axis.
// @ 0x52e590
void Mesh_s2dfd0::scale_selected(int dummy, const rgl::vector4* scale)
{
    rgl::vector4 c(0.0f, 0.0f, 0.0f);          // the centroid: sums first, scaled in place (a 16-byte vector: frame 0x14)
    int n = 0;
    calc_vertex_mode(dummy);
    for (unsigned i = 0; i < verts.size(); i++) {
        if (verts[i].index) {
            c.x += verts[i].pos.x;
            c.y += verts[i].pos.y;
            c.z += verts[i].pos.z;
            n++;
        }
    }
    float inv = 1.0f / (float)n;
    c.x *= inv;
    c.y *= inv;
    c.z *= inv;
    for (unsigned i = 0; i < verts.size(); i++) {
        if (!verts[i].index)
            continue;
        verts[i].pos.x = (verts[i].pos.x - c.x) * scale->x + c.x;
        verts[i].pos.y = (verts[i].pos.y - c.y) * scale->y + c.y;
        verts[i].pos.z = (verts[i].pos.z - c.z) * scale->z + c.z;
    }
}

// Rotate/scale the selected vertices by a 3x3 matrix about their centroid.
// @ 0x52e6d0
void Mesh_s2dfd0::transform_selected(int dummy, const float* m)
{
    rgl::vector4 c(0.0f, 0.0f, 0.0f);
    int n = 0;
    calc_vertex_mode(dummy);
    for (unsigned i = 0; i < verts.size(); i++) {
        if (verts[i].index) {
            c.x += verts[i].pos.x;
            c.y += verts[i].pos.y;
            c.z += verts[i].pos.z;
            n++;
        }
    }
    float inv = 1.0f / (float)n;
    c.x *= inv;
    c.y *= inv;
    c.z *= inv;
    for (unsigned i = 0; i < verts.size(); i++) {
        if (!verts[i].index)
            continue;
        float dx = verts[i].pos.x - c.x;
        float dy = verts[i].pos.y - c.y;
        float dz = verts[i].pos.z - c.z;
        // the two parenthesised sums keep cl from reassociating rows y and z (dz and dx terms are added first)
        rgl::vector4 r(dz * m[8] + dy * m[4] + dx * m[0],
                       (dz * m[9] + dx * m[1]) + dy * m[5],
                       (dz * m[10] + dx * m[2]) + dy * m[6]);
        verts[i].pos = r + c;
    }
}

// Delete every flagged element of the current mode, then rebuild the derived data.  The
// index is held still after a deletion, since the array shrinks under it.
// @ 0x52e870
void Mesh_s2dfd0::remove_selected()
{
    if (mode == 0) {
        for (unsigned i = 0; i < verts.size(); i++) {
            if (verts[i].flag) {
                remove_node(i);
                i--;
            }
        }
    }
    if (mode == 1) {
        for (unsigned i = 0; i < edges.size(); i++) {
            if (edges[i].flag) {
                remove_edge(i);
                i--;
            }
        }
    } else {
        for (unsigned i = 0; i < polys.size(); i++) {
            if (polys[i].flag) {
                remove_face(i);
                i--;
            }
        }
    }
    build_adjacency();
}

// ======================================================= slice s0052e960: functions
// @ 0x52e960
void Mesh_s2e960::make_copy(const rgl::matrix& m)
{
    rgl::matrix basis = m;
    basis.u = rgl::vector4(0.0f, 0.0f, 1.0f);
    basis.s = rgl::vcross_s2e960(basis.f, basis.u);
    basis.s.normalize();
    basis.f = rgl::vcross_s2e960(basis.u, basis.s);

    if (mode == 2) {
        // std::vector::clear() is _Tidy() in this library, and the original calls it out of line (0x41f4b0, one body
        // for every vector of trivially destructible elements; defined in entity.cpp under vec<mface>::clear). Called
        // through that name: the real clear() would be expanded inline here.
        ((vec<mface>*)&edges2)->clear();
        ((vec<mface>*)&polys2)->clear();
        ((vec<mface>*)&verts2)->clear();
        for (unsigned i = 0; i < polys.size(); i++) {      // polys[i] through operator[] at every use
            if (!polys[i].flag)
                continue;
            mpoly_s2e960 t;
            t.idx[0] = -100000;
            t.idx[1] = -100000;
            t.idx[2] = -100000;
            t.idx[3] = -100000;
            t.idx2[0] = -100000;
            t.idx2[1] = -100000;
            t.idx2[2] = -100000;
            t.idx2[3] = -100000;
            t.unk00 = 0;
            t.unk5c = 0;
            t.flag = 0;
            t.count = polys[i].count;
            for (int j = 0; j < polys[i].count; j++) {
                t.idx[j] = vertex_index(polys[i].idx[j], basis);
                t.idx2[j] = edge_index(polys[i].idx2[j], basis);
            }
            polys2.push_back(t);
        }
    }
}

// Return the copy index of source vertex `index`, creating it the first time it is asked for.
static __forceinline int mface_src_index(const mface_s2e960& v) { return v.index; }
// @ 0x52ebc0
int Mesh_s2e960::vertex_index(int index, const rgl::matrix& m)
{
    for (int i = 0; (unsigned)i < verts2.size(); i++) {   // real std::vector size() and operator[] throughout
        if (verts2[i].index == index) {
            if (i >= 0)
                return i;
            break;
        }
    }
    int n = verts2.size();
    mface_s2e960 f;
    f.flag = 0;
    f.pos = m.transform_point_to_local(verts[index].pos);
    f.index = index;
    verts2.push_back(f);
    return n;
}

// Same for edges: the copy keeps the two endpoint indices remapped and remembers where it came
// from in +0x20.
// @ 0x52eca0
int Mesh_s2e960::edge_index(int index, const rgl::matrix& m)
{
    // real std::vector size()/operator[]; the search leaves its result in r (-1: not found) and the fresh edge is
    // built in the `r < 0` arm, which returns its own index: that puts the found return after the arm. The fresh
    // edge's constants are set before its endpoints are looked up (a constructor in the original, presumably).
    int r = -1;
    for (int i = 0; (unsigned)i < edges2.size(); i++) {
        if (edges2[i].index == index) {
            r = i;
            break;
        }
    }
    if (r < 0) {
        int n = edges2.size();
        medge_s2e960 e;
        e.unk08 = -10000;
        e.unk0c = -10000;
        e.flag = 0;
        e.unk10 = 0;
        e.t0 = 1.0f;
        e.t1 = 1.0f;
        e.v0 = vertex_index(edges[index].v0, m);
        e.v1 = vertex_index(edges[index].v1, m);
        e.index = index;
        edges2.push_back(e);
        return n;
    }
    return r;
}

// Flatten the copy buffers into the real ones: every copied vertex goes back to world space via
// the same basis, its edges/polygons are appended with the index ranges shifted, and the derived
// data is rebuilt.  All selection flags are cleared first.
// @ 0x52edb0
void Mesh_s2e960::apply_copy(const rgl::matrix& m)
{
    // The basis lives in an inner block that ends after the vertex loop: the edge and polygon temporaries then share
    // its frame area (frame 0x9c), as in the original.
    int voff, eoff;
    {
    rgl::matrix basis = m;
    basis.u = rgl::vector4(0.0f, 0.0f, 1.0f);
    basis.s = rgl::vcross_s2e960(basis.f, basis.u);
    basis.s.normalize();
    basis.f = rgl::vcross_s2e960(basis.u, basis.s);

    selected = -1;
    for (unsigned i = 0; i < verts.size(); i++)
        verts[i].flag = 0;
    selected = -1;
    for (unsigned i = 0; i < edges.size(); i++)
        edges[i].flag = 0;
    selected = -1;
    for (unsigned i = 0; i < polys.size(); i++)
        polys[i].flag = 0;

    if (polys2.size() == 0)
        return;

    voff = verts.size();
    eoff = edges.size();

    for (unsigned i = 0; i < verts2.size(); i++) {
        mface_s2e960 v;
        v.pos = rgl::to_world_s2e960(basis, verts2[i].pos);
        v.flag = 1;
        verts.push_back(v);
    }
    }

    for (unsigned i = 0; i < edges2.size(); i++) {
        medge_s2e960 e;
        e.unk08 = -10000;
        e.unk0c = -10000;
        e.unk10 = 0;
        e.t0 = 1.0f;
        e.t1 = 1.0f;
        e.flag = 1;
        e.v0 = edges2[i].v0 + voff;
        e.v1 = edges2[i].v1 + voff;
        edges.push_back(e);
    }

    for (unsigned i = 0; i < polys2.size(); i++) {      // polys2[i] through operator[] at every use, as make_copy
        mpoly_s2e960 t;
        t.unk00 = 0;
        t.unk5c = 0;
        t.idx[0] = -100000;
        t.idx[1] = -100000;
        t.idx[2] = -100000;
        t.idx[3] = -100000;
        t.idx2[0] = -100000;
        t.idx2[1] = -100000;
        t.idx2[2] = -100000;
        t.idx2[3] = -100000;
        t.flag = 1;
        t.count = polys2[i].count;
        for (int j = 0; j < polys2[i].count; j++) {
            t.idx[j] = polys2[i].idx[j] + voff;
            t.idx2[j] = polys2[i].idx2[j] + eoff;
        }
        polys.push_back(t);
    }

    build_adjacency();
}

// ======================================================= slice s0052f250: functions
#undef g_map_s2f250
#define g_map_s2f250 (*(Map_s2f250**)&(*(void**)((char*)&g_mission + 0x0)))
#undef g_game_s2f250
#define g_game_s2f250 (*(Frame__s2f250**)&g_game)
#undef g_objrecs_s2f250
#define g_objrecs_s2f250 (*(ObjRec20_s2f250**)&g_ptypes)
#undef slot_as<EditorView_s2f250>(14)
#undef g_editor_dialog_s2f250
#define g_editor_dialog_s2f250 (*(MapEditorDialog_s2f250*)&g_app_obj)
// @ 0x52f250
void MapEditor_s2f250::update()
{
    g_map_s2f250->object->f140 = 0;

    // repeat the two scroll keys every 35000 ticks
    if ((*(int*)((char*)&g_keyboard + 0x34c)) != 0) {
        if (this->f318 < g_ticks[0] - 35000)
            set_list_scroll(1);
    }
    if ((*(int*)((char*)&g_keyboard + 0x32c)) != 0) {
        if (this->f314 < g_ticks[0] - 35000)
            set_list_scroll(-1);
    }
    if ((*(int*)((char*)&g_keyboard + 0x34c)) != 0 && g_keyboard.prev[208] == 0)
        this->f318 = (int)g_ticks[0];
    if ((*(int*)((char*)&g_keyboard + 0x32c)) != 0 && g_keyboard.prev[200] == 0)
        this->f314 = (int)g_ticks[0];
    if ((*(int*)((char*)&g_keyboard + 0x34c)) == 0 && g_keyboard.prev[208] != 0)
        this->f318 = 0;
    if ((*(int*)((char*)&g_keyboard + 0x32c)) == 0 && g_keyboard.prev[200] != 0)
        this->f314 = 0;

    // rebuild the preview when the kind / record / variant changed
    if (g_game_s2f250->mode != g_game_s2f250->prev_mode ||
        g_game_s2f250->id != g_game_s2f250->prev_id ||
        (g_game_s2f250->mode == 4 && this->f31c != this->f320))
    {
        g_map_s2f250->object->clear();

        PlacedObj rec;
        MeshVec_s2f250 list;
        int value = 0;
        if (g_game_s2f250->mode == 4)
            value = this->f31c;

        matrix m;
        identity_pos(&m);
        g_map_s2f250->init_placed(&rec, &g_game_s2f250->mode, &g_game_s2f250->id, &value, &m);
        g_map_s2f250->build_mesh_name(&rec, &list);

        for (unsigned i = 0; i < list.size(); i++) {
            mesh_s2f250* c = list.first[i]->clone();
            g_map_s2f250->object->meshes.push_back(c);
            g_map_s2f250->object->recalc();
        }

        g_game_s2f250->prev_mode = g_game_s2f250->mode;
        g_game_s2f250->prev_id = g_game_s2f250->id;
        this->f320 = this->f31c;

        // preview colour / scale
        float v = 1.0f;
        if (g_game_s2f250->rand_scale != 0)
            v = (float)(rand() % 15817) * 4.741733573609963e-05f + 0.75f;
        g_game_s2f250->color = vector4(v, v, v);
        g_game_s2f250->f1a0ac = 0.0f;

        // frame: s = cross(-dir, up) normalised, f = cross(up, s), u = up
        mat3_s2f250 m3;
        m3.u = vector4(0.0f, 0.0f, 1.0f);
        vector4 d(-this->dir_x, -this->dir_y, -this->dir_z);
        vector4 s = cross_inline_s2f250(d, m3.u);
        m3.s = s;
        normalize_inline_s2f250(m3.s);
        vector4 f = cross_inline_s2f250(m3.u, m3.s);
        m3.f = f;
        *(mat3_s2f250*)&g_map_s2f250->object->frame = m3;

        if (g_game_s2f250->rand_rot != 0) {
            float angle = (float)(rand() % 15817) * 0.00039724254747852683f;
            g_map_s2f250->object->frame.rotate_sf(&angle);
        }
        if (g_game_s2f250->mode == 4) {
            int n = g_objrecs_s2f250[g_game_s2f250->id].variants;
            this->f31c = (n > 1) ? rand() % n : 0;
        }
    }

    // let the placement helper nudge the preview
    this->f2f8 = 0;
    vector4 off(0.0f, 0.0f, g_game_s2f250->f1a0ac);
    mat3_s2f250 m3 = *(mat3_s2f250*)&g_map_s2f250->object->frame;
    vector4 col = g_game_s2f250->color;
    bool fA, fB, fC;
    ((MapEditorDialog*)this)->apply(fA, off, fB, *(rgl::mat3*)&m3, fC, col, this->f2f8);

    if (fA)
        g_game_s2f250->f1a0ac = off.z;
    if (fB)
        *(mat3_s2f250*)&g_map_s2f250->object->frame = m3;
    if (fC) {
        vector4 old = g_game_s2f250->color;
        g_game_s2f250->color = col;
        for (unsigned i = 0; i < g_map_s2f250->object->meshes.size(); i++) {
            mesh_s2f250* c = g_map_s2f250->object->meshes.first[i];
            c->set_color(vector4(1.0f / old.x, 1.0f / old.y, 1.0f / old.z));
            c->set_color(g_game_s2f250->color);
        }
    }
    if (fC != 0 || fB != 0 || fA != 0)
        goto set_cursor;

    {
        char ok;
        vector4 pos;
        this->find_position(&ok, &pos);
        if (ok) {
            this->f304 = pos;
            g_map_s2f250->object->frame.o = pos;
            g_map_s2f250->object->frame.o.z += g_game_s2f250->f1a0ac;
            g_map_s2f250->object->f140 = 1;
            if ((*(int*)((char*)&g_keyboard + 0x390)) != 0 && (*(int*)((char*)&g_keyboard + 0x790)) == 0)
                goto place;
        }
        if (g_game_s2f250->commit == 0)
            goto refresh;
place:
        if (g_game_s2f250->mode >= 0 && g_game_s2f250->id >= 0) {
                matrix cm;
                if (g_game_s2f250->commit != 0) {
                    g_game_s2f250->commit = 0;
                    cm = g_game_s2f250->cam;
                } else {
                    cm = g_map_s2f250->object->frame;
                }

                int pv = 0;
                if (g_game_s2f250->mode == 4)
                    pv = this->f31c;
                int spawn = -1;
                int idx = g_map_s2f250->add_placed(&g_game_s2f250->mode, &g_game_s2f250->id, &pv, &cm, &spawn);

                g_game_s2f250->selected = idx;
                g_game_s2f250->sel.resize(0, 0);
                g_game_s2f250->sel.push_back_out(g_game_s2f250->selected);

                PlacedObj* p = &g_map_s2f250->records[g_game_s2f250->selected];
                p->v158 = g_game_s2f250->color;
                if (p->ent != 0)
                    ((Prop_s2f250*)p->ent)->apply(&p->m18, &p->v158);

                g_game_s2f250->dirty = 1;
                g_editor_dialog_s2f250.update_objects();
                slot_as<EditorView_s2f250>(14)->pick_entity();
                slot_as<EditorView_s2f250>(14)->f2f9 = 1;
                SetFocus(g_app_obj.m_hWnd);

                if (g_game_s2f250->rand_scale != 0) {
                    float v2 = (float)(rand() % 15817) * 4.741733573609963e-05f + 0.75f;
                    g_game_s2f250->color = vector4(v2, v2, v2);
                }
                if (g_game_s2f250->rand_rot != 0) {
                    mat3_s2f250 im;
                    identity3(&im);
                    *(mat3_s2f250*)&g_map_s2f250->object->frame = im;
                    float angle = (float)(rand() % 15817) * 0.00039724254747852683f;
                    g_map_s2f250->object->frame.rotate_sf(&angle);
                }
                if (g_game_s2f250->mode == 4) {
                    int n = g_objrecs_s2f250[g_game_s2f250->id].variants;
                    this->f31c = (n > 1) ? rand() % n : 0;
                }
        }
    }
    goto refresh;

set_cursor:
    SetCursorPos(g_gui.warp_x, g_gui.warp_y);
    g_gui.saved_x = g_gui.warp_x;
    g_gui.saved_y = g_gui.warp_y;
    g_map_s2f250->object->frame.o = this->f304;
    g_map_s2f250->object->frame.o.z += g_game_s2f250->f1a0ac;
    g_map_s2f250->object->f140 = 1;

refresh:
    g_editor_dialog_s2f250.refresh();
}

// ======================================================= slice s0052fe40: functions
#undef g_items_s2fe40
#define g_items_s2fe40 (*(Item_s2fe40**)&g_items)
#undef g_map_pts_s2fe40
#define g_map_pts_s2fe40 (*(MapEntry_s2fe40**)&g_ptypes)
#undef g_map_points_s2fe40
#define g_map_points_s2fe40 (*(MapPoint_s2fe40**)&g_propkinds)
// @ 0x52fe40 ---------------------------------------------------------------------------------------------
void MapEditorDialog::apply(bool& moved, rgl::vector4& pos_v, bool& rotated, rgl::mat3& frame, bool& drag, rgl::vector4& dir, char& flag)
{
    bool* f1 = &moved;
    float* pos = &pos_v.x;
    bool* f2 = &rotated;
    float* out = (float*)&frame;
    bool* f3 = &drag;
    float* vec = &dir.x;
    char* f4 = &flag;
    float yaw = *(float*)((char*)this + 0x268);
    float k = 0.6f;
    if ((*(int*)((char*)&g_keyboard + 0x80)) != 0 || (*(int*)((char*)&g_keyboard + 0x280)) != 0)
        k = 0.6f * 0.1;
    // two statements: the original rounds (y+x)*k to 24 bit before multiplying by 0.005
    float angle = ((*(float*)((char*)&g_keyboard + 0x80c)) + (*(float*)((char*)&g_keyboard + 0x810))) * k;
    angle = angle * 0.005f;

    *f1 = 0;
    *f3 = 0;
    *f2 = 0;

    rot2 m;

    if (g_keyboard.down[34] != 0) {
        m.r[0].x = 1.0f;
        m.r[0].y = 0.0f;
        m.r[1].x = 0.0f;
        m.r[1].y = 1.0f;
        m.rotate_2x2(yaw);

        float dx = (*(float*)((char*)&g_keyboard + 0x80c)) * k * 0.01;
        pos[0] += m.r[0].x * dx;
        *f1 = 1;
        pos[1] += m.r[0].y * dx;
        float dy = (*(float*)((char*)&g_keyboard + 0x810)) * k * 0.01;
        pos[0] -= m.r[1].x * dy;
        pos[1] -= m.r[1].y * dy;
        *f4 = 1;
    }

    if ((*(int*)((char*)&g_keyboard + 0x5c)) != 0) {
        if (g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
            // (k*0.01) first, then *axis_y, to match the original's 24-bit rounding order
            double dz = k * 0.01;
            pos[2] = pos[2] - dz * (*(float*)((char*)&g_keyboard + 0x810));
        } else
            pos[2] = 0.0f;
        *f1 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[19] != 0 && g_keyboard.prev[19] == 0) {
        *f2 = 1;
        Matrix12_s2fe40 tmp;
        Matrix12_s2fe40* id = identity3(&tmp);
        *(Matrix12_s2fe40*)out = *id;
    }

    if (g_keyboard.down[45] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 && g_keyboard.down[48] == 0) {
        ((Matrix12_s2fe40*)out)->rotate_fu(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[21] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 && g_keyboard.down[48] == 0) {
        ((Matrix12_s2fe40*)out)->rotate_su(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[44] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 && g_keyboard.down[48] == 0) {
        ((Matrix12_s2fe40*)out)->rotate_sf(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[22] != 0) {
        float c = cos(angle);
        float s = sin(angle);
        rotate_pair_s2fe40(out + 0, c, s);
        rotate_pair_s2fe40(out + 4, c, s);
        rotate_pair_s2fe40(out + 8, c, s);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[48] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 &&
        g_keyboard.down[45] == 0 && g_keyboard.down[21] == 0 && g_keyboard.down[44] == 0) {
        float s = g_one - angle * g_hilite_dim;
        vec[0] = vec[0] * s;
        vec[1] = vec[1] * s;
        vec[2] = vec[2] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_keyboard.down[45] != 0 && g_keyboard.down[48] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
        float s = g_one - angle * g_hilite_dim;
        vec[0] = vec[0] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_keyboard.down[21] != 0 && g_keyboard.down[48] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
        float s = g_one - angle * g_hilite_dim;
        vec[1] = vec[1] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_keyboard.down[44] != 0 && g_keyboard.down[48] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
        float s = g_one - angle * g_hilite_dim;
        vec[2] = vec[2] * s;
        *f3 = 1;
        *f4 = 1;
    }
    if (g_keyboard.down[48] != 0 && g_keyboard.prev[48] == 0 && (g_keyboard.down[0x38] != 0 || g_keyboard.down[0xb8] != 0)) {
        *f3 = 1;
        vec[0] = 1.0f;
        vec[1] = 1.0f;
        vec[2] = 1.0f;
        vec[3] = m.r[1].y;
        *f4 = 1;
    }
}

// @ 0x5302f0 ---------------------------------------------------------------------------------------------
void __stdcall build_mesh_name(MeshOwner_s2fe40* o, std::vector<rgl::mesh*>* out)
{
    out->resize(0, (rgl::mesh*)0);

    tstring name;
    int kind = o->f188;

    if (kind == 2) {
        int idx = find_item(&o->name);
        name = ((ItemKind*)&g_items_s2fe40[idx])->mesh_name_for(0);
    } else if (kind == 6) {
        int idx = find_item(&o->name);
        int var = o->f1a0;
        Item_s2fe40* it = &g_items_s2fe40[idx];
        int t = it->type_flags & 0xff;
        ItemRef_s2fe40 ref;
        ref.item = idx;
        ref.m4 = var;
        rgl::mesh* m;
        if (t == 2 || t == 3 || t == 4 || t == 6 || t == 5 || t == 0x12) {
            if (idx >= 0 && idx < g_item_kind_count)
                m = it->mesh_for(0, 0x30000000, var >> 24);
            else
                m = 0;
            if (m != 0) {
                out->push_back(m);
                goto done;
            }
        }
        {
            name = ((ItemRef*)&ref)->mesh_name();
        }
    } else if (kind == 4) {
        int i;
        for (i = 0; i < g_ptype_count; i++) {
            if (g_map_pts_s2fe40[i].name.equals(o->name) != 0)
                break;
        }
        if (i == g_ptype_count)
            i = 0;
        o->f1a0 = o->f1a0 % g_map_pts_s2fe40[i].count;
        name = *(tstring*)((char*)g_map_pts_s2fe40[i].names + o->f1a0 * 0x48);
    } else if (kind == 7) {
        name = ((ItemKind*)&g_items_s2fe40[o->f18c])->mesh_name_for(o->f1a0);
    } else if (kind == 0) {
        int pi = find_in_table78(&o->name);
        if (pi < 0)
            pi = o->f18c;
        if (pi >= 0 && pi < g_propkind_count) {
            MapPoint_s2fe40* p = &g_map_points_s2fe40[pi];
            if (g_cfg.enable_edit_mode != 0 || (p->flags & 0x4000) == 0)
                *out = p->meshes;
        }
    } else if (kind == 1) {
        if (g_cfg.enable_edit_mode != 0)
            name.assign("entry_arrow");
        else
            name.assign("");
    } else if (kind == 5) {
        if (g_cfg.enable_edit_mode != 0)
            name.assign("_mesh_passage");
        else
            name.assign("");
    }

done:
    if (name.len > 0) {
        std::vector<rgl::mesh*> tmp = g_resmgr.find_meshes(&name);
        *out = tmp;
    }
}
