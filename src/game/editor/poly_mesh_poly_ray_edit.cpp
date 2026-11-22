//-----------------------------------------------------------------------------
// File: poly_mesh_poly_ray_edit.cpp  (0x52dfd0..0x5305b0)
//
// Desc: one object of mesh_3's range, split where the original's caller / callee objects meet: Mesh::poly_ray and the rest of the map editor mesh object: toggle_flag, scale / transform / remove the selection, make_copy / vertex_index / edge_index / apply_copy, the editor panel update, the view controller apply and the mesh-name chooser.
//       pick_vert / pick_edge / pick_poly call the ray functions as external functions (the original compiled them in
//       separate objects). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "engine/core/std_vec_push_back.h"   // 0x52fbc0 calls std::vector<int>::push_back 0x440b30
#include "game/editor/poly_mesh_pick.h"
#include "game/ui/gui_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/core/tstring.h"
#include "engine/math/rgl_math.h"
#include "engine/io/stream.h"
#include "engine/render/mesh.h"
#include "engine/math/float_consts.h"   // shared float constants kept in memory (0x549cdc ...)

#include "game/input/key_names.h"

// 0x432830 (game/common/game_util.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) void __cdecl vec3_sub(Vec3* out, const Vec3* a, const Vec3* b) { out->x = a->x - b->x; out->y = a->y - b->y; out->z = a->z - b->z; }      // 0x432830

// Ray / convex-polygon intersection.  The polygon's vertices (at most four) are gathered once,
// their mean gives the plane point, the first three give the normal, and the hit point is then
// tested against every edge (a hit outside the polygon reports 1e6, i.e. "no hit").
// @ 0x52dfd0
void PolyMesh::poly_ray(const rgl::vector4& p, const rgl::vector4& d, unsigned& index,
                    float* dist)
{
    const MPoly& poly = faces[index];
    int n = poly.n;
    rgl::vector4 pts[4];
    rgl::vector4 mean(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < n; i++) {
        pts[i] = verts[poly.vid[i]].pos;
        mean += pts[i];
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
    rgl::vector4 hit = p + d * t;

    bool outside = false;
    for (int i = 0; i < n && !outside; i++) {
        rgl::vector4 a = pts[i];
        rgl::vector4 b = pts[(i + 1) % n];
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
void PolyMesh::toggle_flag(int type, int index)
{
    if (type == 0) {
        bool flag = verts[index].flag;
        verts[index].flag = !flag;
        if (verts[index].flag)
            cur = index;
        else
            cur = -1;
    } else if (type == 1) {
        bool flag = edges[index].flag;
        edges[index].flag = !flag;
    } else {
        bool flag = faces[index].flag;
        faces[index].flag = !flag;
    }
}

// Scale the selected vertices about their centroid.  The +0x14 selection stamp is recomputed
// first (0x4c54f0); the scale factors are per axis.
// @ 0x52e590
void PolyMesh::scale_selected(int dummy, const rgl::vector4* scale)
{
    rgl::vector4 c(0.0f, 0.0f, 0.0f);          // the centroid: sums first, scaled in place (a 16-byte vector: frame 0x14)
    int n = 0;
    calc_vertex_mode(dummy);
    for (unsigned i = 0; i < verts.size(); i++) {
        if (verts[i].mark) {
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
        if (!verts[i].mark)
            continue;
        verts[i].pos.x = (verts[i].pos.x - c.x) * scale->x + c.x;
        verts[i].pos.y = (verts[i].pos.y - c.y) * scale->y + c.y;
        verts[i].pos.z = (verts[i].pos.z - c.z) * scale->z + c.z;
    }
}

// Rotate/scale the selected vertices by a 3x3 matrix about their centroid.
// @ 0x52e6d0
void PolyMesh::transform_selected(int dummy, const rgl::mat3* m)
{
    rgl::vector4 c(0.0f, 0.0f, 0.0f);
    int n = 0;
    calc_vertex_mode(dummy);
    for (unsigned i = 0; i < verts.size(); i++) {
        if (verts[i].mark) {
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
        if (!verts[i].mark)
            continue;
        float dx = verts[i].pos.x - c.x;
        float dy = verts[i].pos.y - c.y;
        float dz = verts[i].pos.z - c.z;
        // the two parenthesised sums keep cl from reassociating rows y and z (dz and dx terms are added first)
        rgl::vector4 r(dz * m->u.x + dy * m->f.x + dx * m->s.x,
                       (dz * m->u.y + dx * m->s.y) + dy * m->f.y,
                       (dz * m->u.z + dx * m->s.z) + dy * m->f.z);
        verts[i].pos = r + c;
    }
}

// Delete every flagged element of the current mode, then rebuild the derived data.  The
// index is held still after a deletion, since the array shrinks under it.
// @ 0x52e870
void PolyMesh::remove_selected()
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
        for (unsigned i = 0; i < faces.size(); i++) {
            if (faces[i].flag) {
                remove_face(i);
                i--;
            }
        }
    }
    build_adjacency();
}

// ======================================================= slice s0052e960: functions
// @ 0x52e960
void PolyMesh::make_copy(const rgl::matrix& m)
{
    rgl::matrix basis = m;
    basis.u = rgl::vector4(0.0f, 0.0f, 1.0f);
    basis.s = rgl::vcross_s2e960(basis.f, basis.u);
    basis.s.normalize();
    basis.f = rgl::vcross_s2e960(basis.u, basis.s);

    if (mode == 2) {
        // the library's clear() out of line (0x41f4b0: poly_mesh_pick.h)
        copy_edges.clear();
        copy_faces.clear();
        copy_verts.clear();
        for (unsigned i = 0; i < faces.size(); i++) {      // faces[i] through operator[] at every use
            if (!faces[i].flag)
                continue;
            MPoly t((raw_t()));
            t.vid[0] = -100000;
            t.vid[1] = -100000;
            t.vid[2] = -100000;
            t.vid[3] = -100000;
            t.eid[0] = -100000;
            t.eid[1] = -100000;
            t.eid[2] = -100000;
            t.eid[3] = -100000;
            t.stamp = 0;
            t.f5c = 0;
            t.flag = 0;
            t.n = faces[i].n;
            for (int j = 0; j < faces[i].n; j++) {
                t.vid[j] = vertex_index(faces[i].vid[j], basis);
                t.eid[j] = edge_index(faces[i].eid[j], basis);
            }
            copy_faces.push_back(t);
        }
    }
}

// Return the copy index of source vertex `index`, creating it the first time it is asked for.
static __forceinline int mface_src_index(const MVert& v) { return v.mark; }
// @ 0x52ebc0
int PolyMesh::vertex_index(int index, const rgl::matrix& m)
{
    for (int i = 0; (unsigned)i < copy_verts.size(); i++) {   // real std::vector size() and operator[] throughout
        if (copy_verts[i].mark == index) {
            if (i >= 0)
                return i;
            break;
        }
    }
    int n = copy_verts.size();
    MVert f;
    f.flag = 0;
    f.pos = m.transform_point_to_local(verts[index].pos);
    f.mark = index;
    copy_verts.push_back(f);
    return n;
}

// Same for edges: the copy keeps the two endpoint indices remapped and remembers where it came
// from in +0x20.
// @ 0x52eca0
int PolyMesh::edge_index(int index, const rgl::matrix& m)
{
    // real std::vector size()/operator[]; the search leaves its result in r (-1: not found) and the fresh edge is
    // built in the `r < 0` arm, which returns its own index: that puts the found return after the arm. The fresh
    // edge's constants are set before its endpoints are looked up (a constructor in the original, presumably).
    int r = -1;
    for (int i = 0; (unsigned)i < copy_edges.size(); i++) {
        if (copy_edges[i].mark == index) {
            r = i;
            break;
        }
    }
    if (r < 0) {
        int n = copy_edges.size();
        MEdge e((raw_t()));
        e.face[0] = -10000;
        e.face[1] = -10000;
        e.flag = 0;
        e.nfaces = 0;
        e.crease[0] = 1.0f;
        e.crease[1] = 1.0f;
        e.v0 = vertex_index(edges[index].v0, m);
        e.v1 = vertex_index(edges[index].v1, m);
        e.mark = index;
        copy_edges.push_back(e);
        return n;
    }
    return r;
}

// Flatten the copy buffers into the real ones: every copied vertex goes back to world space via
// the same basis, its edges/polygons are appended with the index ranges shifted, and the derived
// data is rebuilt.  All selection flags are cleared first.
// @ 0x52edb0
void PolyMesh::apply_copy(const rgl::matrix& m)
{
    // The basis lives in an inner block that ends after the vertex loop: the edge and polygon temporaries then share
    // its frame area (frame 0x9c), as in the original.
    int voff, eoff;
    {
    rgl::matrix basis = m;
    unsigned i;
    basis.u = rgl::vector4(0.0f, 0.0f, 1.0f);
    basis.s = rgl::vcross_s2e960(basis.f, basis.u);
    basis.s.normalize();
    basis.f = rgl::vcross_s2e960(basis.u, basis.s);

    cur = -1;
    for (i = 0; i < verts.size(); i++)
        verts[i].flag = 0;
    cur = -1;
    for (i = 0; i < edges.size(); i++)
        edges[i].flag = 0;
    cur = -1;
    for (i = 0; i < faces.size(); i++)
        faces[i].flag = 0;

    if (copy_faces.size() == 0)
        return;

    voff = verts.size();
    eoff = edges.size();

    for (i = 0; i < copy_verts.size(); i++) {
        MVert v;
        v.pos = rgl::to_world_s2e960(basis, copy_verts[i].pos);
        v.flag = 1;
        verts.push_back(v);
    }
    }

    unsigned i;
    for (i = 0; i < copy_edges.size(); i++) {
        MEdge e((raw_t()));
        e.face[0] = -10000;
        e.face[1] = -10000;
        e.nfaces = 0;
        e.crease[0] = 1.0f;
        e.crease[1] = 1.0f;
        e.flag = 1;
        e.v0 = copy_edges[i].v0 + voff;
        e.v1 = copy_edges[i].v1 + voff;
        edges.push_back(e);
    }

    for (i = 0; i < copy_faces.size(); i++) {      // copy_faces[i] through operator[] at every use, as make_copy
        MPoly t((raw_t()));
        t.stamp = 0;
        t.f5c = 0;
        t.vid[0] = -100000;
        t.vid[1] = -100000;
        t.vid[2] = -100000;
        t.vid[3] = -100000;
        t.eid[0] = -100000;
        t.eid[1] = -100000;
        t.eid[2] = -100000;
        t.eid[3] = -100000;
        t.flag = 1;
        t.n = copy_faces[i].n;
        for (int j = 0; j < copy_faces[i].n; j++) {
            t.vid[j] = copy_faces[i].vid[j] + voff;
            t.eid[j] = copy_faces[i].eid[j] + eoff;
        }
        faces.push_back(t);
    }

    build_adjacency();
}

// ======================================================= slice s0052f250: functions
namespace rgl {
static __forceinline vector4 rot_vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 rot_vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 rot_vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
}
// 0x40e770 (engine/physics/collision_shapes.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) void rgl::mat3::rotate_about_u(const float* angle)      // 0x40e770
{
    double sn = sin((double)*angle);
    vector4 ns, nf;
    ns = rot_vadd(rot_vmul(s, cos((double)*angle)), rot_vmul(f, sn));
    nf = rot_vsub(rot_vmul(f, cos((double)*angle)), rot_vmul(s, sn));
    s = ns;
    f = nf;
}

// @ 0x52f250
void MissionBlocker::update_placement()
{
    g_mission->edit_obj->visible = 0;

    // repeat the two scroll keys every 35000 ticks
    if (g_keyboard.down[208] != 0) {
        if (this->up_repeat < g_ticks[0] - 35000)
            set_list_scroll(1);
    }
    if (g_keyboard.down[200] != 0) {
        if (this->down_repeat < g_ticks[0] - 35000)
            set_list_scroll(-1);
    }
    if (g_keyboard.down[208] != 0 && g_keyboard.prev[208] == 0)
        this->up_repeat = (int)g_ticks[0];
    if (g_keyboard.down[200] != 0 && g_keyboard.prev[200] == 0)
        this->down_repeat = (int)g_ticks[0];
    if (g_keyboard.down[208] == 0 && g_keyboard.prev[208] != 0)
        this->up_repeat = 0;
    if (g_keyboard.down[200] == 0 && g_keyboard.prev[200] != 0)
        this->down_repeat = 0;

    // rebuild the preview when the kind / record / variant changed
    if (g_game->editor.list_mode != g_game->editor.f1a0c0 ||
        g_game->editor.list_sel != g_game->editor.f1a0c4 ||
        (g_game->editor.list_mode == 4 && this->variant != this->prev_variant))
    {
        g_mission->edit_obj->clear();

        PlacedObj rec;
        std::vector<rgl::mesh*> list;
        int value = 0;
        if (g_game->editor.list_mode == 4)
            value = this->variant;

        matrix m;
        identity_pos(&m);
        init_placed(&rec, &g_game->editor.list_mode, &g_game->editor.list_sel, &value, &m);
        build_mesh_name(&rec, &list);

        for (unsigned i = 0; i < list.size(); i++) {
            rgl::mesh* c = list[i]->clone();
            g_mission->edit_obj->meshes.push_back(c);
            g_mission->edit_obj->recalc();
        }

        g_game->editor.f1a0c0 = g_game->editor.list_mode;
        g_game->editor.f1a0c4 = g_game->editor.list_sel;
        this->prev_variant = this->variant;

        // preview colour / scale
        float v = 1.0f;
        if (g_game->editor.opt_d0 != 0)
            v = (float)(rand() % 15817) * 4.741733573609963e-05f + 0.75f;
        g_game->editor.rt = vector4(v, v, v);
        g_game->editor.keep = 0.0f;

        // frame: s = cross(-dir, up) normalised, f = cross(up, s), u = up
        rgl::mat3 m3;
        m3.u = vector4(0.0f, 0.0f, 1.0f);
        vector4 d(-this->k.u.x, -this->k.u.y, -this->k.u.z);
        vector4 s = cross_inline_s2f250(d, m3.u);
        m3.s = s;
        normalize_inline_s2f250(m3.s);
        vector4 f = cross_inline_s2f250(m3.u, m3.s);
        m3.f = f;
        static_cast<rgl::mat3&>(g_mission->edit_obj->local) = m3;

        if (g_game->editor.opt_cc != 0) {
            float angle = (float)(rand() % 15817) * 0.00039724254747852683f;
            g_mission->edit_obj->local.rotate_about_u(&angle);
        }
        if (g_game->editor.list_mode == 4) {
            int n = g_ptypes[g_game->editor.list_sel].nframes;
            this->variant = (n > 1) ? rand() % n : 0;
        }
    }

    // let the placement helper nudge the preview
    this->placement_adjusted = 0;
    vector4 off(0.0f, 0.0f, g_game->editor.keep);
    rgl::mat3 m3 = g_mission->edit_obj->local;
    vector4 col = g_game->editor.rt;
    bool fA, fB, fC;
    apply(fA, off, fB, m3, fC, col, this->placement_adjusted);

    if (fA)
        g_game->editor.keep = off.z;
    if (fB)
        static_cast<rgl::mat3&>(g_mission->edit_obj->local) = m3;
    if (fC) {
        vector4 old = g_game->editor.rt;
        g_game->editor.rt = col;
        for (unsigned i = 0; i < g_mission->edit_obj->meshes.size(); i++) {
            rgl::mesh* c = g_mission->edit_obj->meshes[i];
            c->set_color(vector4(1.0f / old.x, 1.0f / old.y, 1.0f / old.z));
            c->set_color(g_game->editor.rt);
        }
    }
    if (fC != 0 || fB != 0 || fA != 0)
        goto set_cursor;

    {
        char ok;
        vector4 pos;
        this->find_position(&ok, &pos);
        if (ok) {
            this->picked_pos = pos;
            g_mission->edit_obj->local.o = pos;
            g_mission->edit_obj->local.o.z += g_game->editor.keep;
            g_mission->edit_obj->visible = 1;
            if (g_keyboard.down[225] != 0 && g_keyboard.prev[225] == 0)
                goto place;
        }
        if (g_game->editor.flag_c8 == 0)
            goto refresh;
place:
        if (g_game->editor.list_mode >= 0 && g_game->editor.list_sel >= 0) {
                matrix cm;
                if (g_game->editor.flag_c8 != 0) {
                    g_game->editor.flag_c8 = 0;
                    cm = g_game->editor.cam_frame;
                } else {
                    cm = g_mission->edit_obj->local;
                }

                int pv = 0;
                if (g_game->editor.list_mode == 4)
                    pv = this->variant;
                int spawn = -1;
                int idx = g_mission->add_placed(&g_game->editor.list_mode, &g_game->editor.list_sel, &pv, &cm, &spawn);

                g_game->editor.sel_index = idx;
                g_game->editor.sel.resize(0, 0);
                g_game->editor.sel.push_back(g_game->editor.sel_index);

                PlacedObj* p = &g_mission->props[g_game->editor.sel_index];
                p->scale = g_game->editor.rt;
                if (p->ent != 0)
                    p->ent->set_local_scaled_dirty(&p->frame, &p->scale);

                g_game->editor.dirty = 1;
                g_app_obj.refresh_editor_objects();
                slot_as<MissionBlocker>(14)->pick_entity();
                slot_as<MissionBlocker>(14)->f2f9 = 1;
                SetFocus(g_app_obj.m_hWnd);

                if (g_game->editor.opt_d0 != 0) {
                    float v2 = (float)(rand() % 15817) * 4.741733573609963e-05f + 0.75f;
                    g_game->editor.rt = vector4(v2, v2, v2);
                }
                if (g_game->editor.opt_cc != 0) {
                    rgl::mat3 im;
                    identity3(&im);
                    static_cast<rgl::mat3&>(g_mission->edit_obj->local) = im;
                    float angle = (float)(rand() % 15817) * 0.00039724254747852683f;
                    g_mission->edit_obj->local.rotate_about_u(&angle);
                }
                if (g_game->editor.list_mode == 4) {
                    int n = g_ptypes[g_game->editor.list_sel].nframes;
                    this->variant = (n > 1) ? rand() % n : 0;
                }
        }
    }
    goto refresh;

set_cursor:
    SetCursorPos(g_gui.warp_x, g_gui.warp_y);
    g_gui.saved_x = g_gui.warp_x;
    g_gui.saved_y = g_gui.warp_y;
    g_mission->edit_obj->local.o = this->picked_pos;
    g_mission->edit_obj->local.o.z += g_game->editor.keep;
    g_mission->edit_obj->visible = 1;

refresh:
    g_app_obj.refresh_editor_dialog();
}

// ======================================================= slice s0052fe40: functions
// @ 0x52fe40 ---------------------------------------------------------------------------------------------
void MissionBlocker::apply(bool& moved, rgl::vector4& pos_v, bool& rotated, rgl::mat3& frame, bool& drag, rgl::vector4& dir, char& flag)
{
    bool* f1 = &moved;
    float* pos = &pos_v.x;
    bool* f2 = &rotated;
    bool* f3 = &drag;
    float* vec = &dir.x;
    char* f4 = &flag;
    float yaw = this->yaw;
    float k = 0.6f;
    if (g_keyboard.down[29] != 0 || g_keyboard.down[157] != 0)
        k = 0.6f * 0.1;
    // two statements: the original rounds (y+x)*k to 24 bit before multiplying by 0.005
    float angle = (g_keyboard.axis[0] + g_keyboard.axis[1]) * k;
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

        float dx = g_keyboard.axis[0] * k * 0.01;
        pos[0] += m.r[0].x * dx;
        *f1 = 1;
        pos[1] += m.r[0].y * dx;
        float dy = g_keyboard.axis[1] * k * 0.01;
        pos[0] -= m.r[1].x * dy;
        pos[1] -= m.r[1].y * dy;
        *f4 = 1;
    }

    if (g_keyboard.down[20] != 0) {
        if (g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0) {
            // (k*0.01) first, then *axis_y, to match the original's 24-bit rounding order
            double dz = k * 0.01;
            pos[2] = pos[2] - dz * g_keyboard.axis[1];
        } else
            pos[2] = 0.0f;
        *f1 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[19] != 0 && g_keyboard.prev[19] == 0) {
        *f2 = 1;
        rgl::mat3 tmp;
        rgl::mat3* id = identity3(&tmp);
        frame = *id;
    }

    if (g_keyboard.down[45] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 && g_keyboard.down[48] == 0) {
        frame.rotate_about_s(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[21] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 && g_keyboard.down[48] == 0) {
        frame.rotate_about_f(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[44] != 0 && g_keyboard.down[0x38] == 0 && g_keyboard.down[0xb8] == 0 && g_keyboard.down[48] == 0) {
        frame.rotate_about_u(&angle);
        *f2 = 1;
        *f4 = 1;
    }

    if (g_keyboard.down[22] != 0) {
        float c = cos(angle);
        float s = sin(angle);
        rotate_pair_s2fe40(&frame.s.x, c, s);
        rotate_pair_s2fe40(&frame.f.x, c, s);
        rotate_pair_s2fe40(&frame.u.x, c, s);
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
void __stdcall build_mesh_name(PlacedObj* o, std::vector<rgl::mesh*>* out)
{
    out->resize(0, (rgl::mesh*)0);

    tstring name;
    int kind = o->kind;

    if (kind == 2) {
        int idx = find_item(&o->name);
        name = g_items[idx].mesh_name_for(0);
    } else if (kind == 6) {
        int idx = find_item(&o->name);
        int var = o->value;
        ItemKind* it = &g_items[idx];
        int t = it->type_flags & 0xff;
        ItemRef ref;
        ref.item = idx;
        ref.m4 = var;
        rgl::mesh* m;
        if (t == 2 || t == 3 || t == 4 || t == 6 || t == 5 || t == 0x12) {
            if (idx >= 0 && idx < g_item_kind_count)
                m = it->mesh_for((unsigned __int64)0x30000000 << 32, var >> 24);
            else
                m = 0;
            if (m) {
                out->push_back(m);
                goto done;
            }
        }
        {
            name = ref.mesh_name();
        }
    } else if (kind == 4) {
        int i;
        for (i = 0; i < g_ptype_count; i++) {
            if (g_ptypes[i].name.equals(o->name) != 0)
                break;
        }
        if (i >= g_ptype_count)
            i = 0;
        o->value = o->value % g_ptypes[i].nframes;
        name = g_ptypes[i].boxes[o->value].name;
    } else if (kind == 7) {
        name = g_items[o->id].mesh_name_for(o->value);
    } else if (kind == 0) {
        int pi = find_in_table78(&o->name);
        if (pi < 0)
            pi = o->id;
        if (pi >= 0 && pi < g_propkind_count) {
            MissionObj* p = &g_propkinds[pi];
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
