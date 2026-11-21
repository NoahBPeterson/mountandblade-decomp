//-----------------------------------------------------------------------------
// File: poly_mesh.cpp  (0x470a00..0x478830)
//
// Desc: the second object of terrain_b's range: the polygon-mesh geometry builder (PolyMesh::build_geometry, which calls
//       build_adjacency 0x470920 and compute_face_normals 0x4705e0 as external functions), the world light frame,
//       the terrain's height generator, sun lighting of the world-map particles and cell refresh, the Newton ragdoll
//       clamp and step, face-generator helpers, the mission's per-team statistics, spawn-rule choice and record-set
//       destructors, the VC7.1 <vector> / <algorithm> instantiations of the object's records, the id pools, and the
//       nav-map loader / builder (PolyMesh::build calls build_adjacency too). Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./terrain_b.h"
#include "./scene_file.h"              // SceneFile: GameData::scenes' record (visitors)
#include "../../rgl/options.h"   // g_options
#include "../config/config_block.h"   // g_cfg
#include "./frustum.h"
#include "./render_batch.h"
#include "./poly_mesh.h"
#include "../placed_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "./cell_grid.h"

#include "./terrain.h"
#include "./records.h"
#include "./game_data.h"
#include "./face.h"
#include "./troop_mgr.h"
#include "./shadow_map.h"
#include "../../include/pool.h"      // Pool<T>: the paged record pool
#include "../missile.h"   // Missile, the record of Pool<Missile> (Mission::poolJ)
#include "./agent_rec.h"
#include "../skeleton.h"          // node_groups (Agent::meters: set_slot 0x470d80 caps their node lists)
#include "./mission.h"
#include "./physics.h"
#include "../missile.h"        // Missile: ammo_scale 0x467f80, settle 0x472650
#include "./face_gen.h"
#include "../../include/vec_elems.h"      // FaceTexture (Skin::face_textures)
#include "../../rgl/core/resmgr.h"      // lod_group (Skin::copy_troop reads the five mesh lists of a lod mesh resource)
#include "../animator.h"       // Animator: unpack 0x473800, breed 0x4746e0 (g_anim); include/skin.h Skin::relax 0x4735e0
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../arr_set.h"
#include "../int_vec.h"
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/ptr_tag.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../game_globals.h"      // canonical g_module_settings.player_wounded_treshold / g_module_settings.hero_wounded_treshold (listed directly: the audit reads only the source's own includes)
#include "../../include/phys_set.h"
#include "../../include/height_field.h"
#include "../render_batch.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../shadow_map.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../terrain_desc.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/view_params.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../agent_ai.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../include/renderer.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../terrain_a/terrain.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mode_rec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
#include "../../include/item_kind.h"   // g_items 0x5e29d0
#include "../../include/render_flags.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

using rgl::vector4;
using rgl::matrix;

// @ 0x470a00
void PolyMesh::build_geometry()
{
    build_adjacency();
    for (unsigned i = 0; i < faces.size(); i++) {
        faces[i].centroid = vector4(0.0f, 0.0f, 0.0f);
        for (int j = 0; j < faces[i].n; j++) {
            faces[i].centroid += verts[faces[i].vid[j]].pos;
            faces[i].nbr[j] = -1;
            if (edges[faces[i].eid[j]].nfaces >= 2) {
                if (edges[faces[i].eid[j]].face[0] == (int)i)
                    faces[i].nbr[j] = edges[faces[i].eid[j]].face[1];
                else if (edges[faces[i].eid[j]].face[1] == (int)i)
                    faces[i].nbr[j] = edges[faces[i].eid[j]].face[0];
            }
        }
        float s = 1.0f / (float)faces[i].n;
        MPoly& h = faces[i];
        h.centroid.x *= s;
        h.centroid.y *= s;
        h.centroid.z *= s;
    }
    for (unsigned i = 0; i < edges.size(); i++) {
        edges.begin()[i].mid = (verts.begin()[edges.begin()[i].v0].pos + verts.begin()[edges.begin()[i].v1].pos) * 0.5f;
    }
    compute_face_normals();
}

// @ 0x470c00
void rgl::world::build_light_frame(matrix* m, vector4* amb)
{
    vector4 d;
    {
        vector4 p(0.0f, 0.0f, 0.0f);       // dies before the frame temporaries: cl shares its stack slot
        sample_lights(&p, &d, amb);
    }
    if (d.z > -0.1f) d.z = -0.1f;
    m->u = vector4(-d.x, -d.y, -d.z);
    m->f = vector4(0.0f, 0.0f, 1.0f);
    m->s = cross_i(m->f, m->u);
    m->s.normalize();
    m->f = cross_i(m->u, m->s);
    m->f.normalize();
}

// @ 0x470d80
void Agent::set_slot(int idx, int value)
{
    rgl::lod_mesh* m = meters[idx];
    if (m) {
        float v = (float)value;
        m->cap_all(&v);
    }
}

// @ 0x470db0
void Skin::apply_group(ArrSet* dst, int idx, rgl::mesh* obj)
{
    rgl::mesh* base = head->lists[0][0];
    int nbase = base->points.size();
    int n = obj->get70_rec().size();
    dst->cnt[idx] = n;
    dst->arr[idx] = new int[n];
    for (int i = 0; i < dst->cnt[idx]; i++) {
        float best = 100000.0f;
        int bi = 0;
        for (int j = 0; j < nbase; j++) {
            vector4 q = obj->get70_rec()[i].a;
            if (q.x < 0.0f) q.x = -q.x;
            const rgl::point20& bp = base->points[j];
            float dx = q.x - bp.a.x;
            float dy = q.y - bp.a.y;
            float dz = q.z - bp.a.z;
            float d = dy * dy + (dx * dx + dz * dz);
            if (d < best) { best = d; bi = j; }
        }
        dst->arr[idx][i] = bi;
    }
}

bool __cdecl in_box(const vector4* lo, const vector4* hi, const vector4* p);   // 0x4336d0
// rgl::cell::collect_point (0x439780, agent2/agent_stats.cpp) with its body visible, as the original's unit saw it: the
// hit array and the counts then do not escape, so the column pointer stays a register and the bound loads straight into ebx
inline __declspec(noinline) void rgl::cell::collect_point(const float* p, int* use3d, unsigned mask, shape_base** out, int* count)
{
    unsigned i = 0;
    *count = 0;
    while (i < a.size()) {
        shape_base** q = &a[i];
        shape_base* sh = *q;
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        vector4 tv[2];
        const vector4* smax = sh->get_max(&tv[0]);
        const vector4* smin = sh->get_min(&tv[1]);
        bool hit;
        if (*use3d == 0)
            hit = smin->x <= p[0] && p[0] <= smax->x && smin->y <= p[1] && p[1] <= smax->y;
        else
            hit = in_box(smin, smax, (const vector4*)p);
        if (!hit) {
            i++;
            continue;
        }
        out[*count] = *q;
        ++*count;
        i++;
    }
}

// @ 0x470f40
bool GameData::ground_height(rgl::world* lv, const float* p, float* out)   // a member that ignores this
{
    shape_base* hits[0x2000];
    int count;
    int use3d;
    int cellxy[2];
    vector4 q;
    q.x = p[0];
    q.y = p[1];
    q.z = 0.0f;
    use3d = 0;
    lv->to_cell(&q.x, cellxy);
    count = 0;
    rgl::cell* col = lv->cells[cellxy[0]];
    col[cellxy[1]].collect_point(&q.x, &use3d, 0x104, hits, &count);
    int n = count;
    for (int i = 0; i < n; i++) {
        if (hits[i]->m4 >= 0 && ((face*)hits[i])->contains_xy(p)) {
            face* f = (face*)hits[i];
            *out = f->w[0].z - ((p[0] - f->w[0].x) * f->normal.x + (p[1] - f->w[0].y) * f->normal.y) / f->normal.z;
            return true;
        }
    }
    return false;
}

// @ 0x471040
void GameData::blend(float* out, float x, float y)
{
    vector4 wn = wind_noise();
    float s1 = 0.0f, s0 = 0.0f, s2 = 0.0f, s3 = 0.0f, sum = 0.0f;
    const Vec2* xy = pos;
    const Block16* rec = val;
    int n = 4;
    do {
        n--;
        float dx = xy->x - x;
        float dy = xy->y - y;
        float w = 1.0f / (float)sqrt(dx * dx + dy * dy);
        s1 = w * rec->p.y + s1;
        s0 = w * rec->p.x + s0;
        s2 = w * rec->q.x + s2;
        s3 = s3 + w * rec->q.y;
        sum = w + sum;
        xy++;
        rec++;
    } while (n != 0);
    float t = 1.0f / sum;                    // inverse weight sum, then reused for the ground factor (address taken)
    float r0 = wn.x + t * s0;
    float r1 = wn.y + t * s1;
    float r2 = wn.z + s2 * t;
    float r3 = wn.w + t * s3;
    if (g_script.level == 0)
        t = 1.0f;
    else
        ground_height((rgl::world*)g_script.level, &x, &t);
    out[0] = r0 - t * 10.0f;
    out[1] = r1;
    out[2] = r2 * t + r2;
    out[3] = r3 * t + r3;
}

// @ 0x4711e0
void ShadowMap::attach_camera(rgl::world* c)
{
    WL_MARKER(12)
    // marker "WL" 12 here in the original
    cam = c;
    cam->view.vmin = vector4(10000.0f, 10000.0f, 10000.0f);
    cam->view.vmax = vector4(-10000.0f, -10000.0f, -10000.0f);
    cam->view.size = vector4(1.0f, 1.0f, 1.0f);
    cam->view.center = vector4(0.0f, 0.0f, 0.0f);
    cam->view.wcenter = vector4(0.0f, 0.0f, 0.0f);
    cam->build_light_frame(&cam->view.world, &cam->shadow_amb);
    vector4 z(0.0f, 0.0f, 0.0f);
    cam->view.world.o = z;
    if (g_cfg.shadowmap_quality != g_cfg.shadow_quality_req) {
        release_all();
        g_cfg.shadowmap_quality = g_cfg.shadow_quality_req;
    }
    // marker "WL" 13 here in the original
    WL_MARKER(13)
}

// @ 0x4713d0
void Terrain::generate_heights()
{
    float r1 = (float)(rand() % 15817) * 6.322312e-05f;
    float scale = (r1 * 0.5f + 1.0f) * (wy + wx) * 0.5f;
    float r2 = (float)(rand() % 15817) * 6.322312e-05f;
    noise_seed = (r2 * 0.5f + 1.0f) * (wy + wx) * 0.2f;
    float fl = (float)(((unsigned)g_bit[7] - 1) & flags) * 0.01f;
    float off[3];
    off[0] = (float)(rand() % 15817) * 0.6322311f;
    off[1] = (float)(rand() % 15817) * 0.6322311f;
    off[2] = (float)(rand() % 15817) * 0.6322311f;
    // relief weight from low-frequency noise
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            float p[3], n[3];
            p[0] = off[0] + cell[x][y].px;
            p[1] = off[1] + cell[x][y].py;
            p[2] = off[2] + cell[x][y].pz;
            noise_fractal(noise_seed, 0.6f, 1, 0, p, n);
            float v = (n[0] + n[1] + n[2]) * fl * 9.0f;   // cl emits this as n[2] + n[1] + n[0] (0x47156f), as the original
            if (v < 0.0f) v = 0.0f;
            else if (v >= 1.0f) v = 1.0f;
            cell[x][y].weight = ((float)cos(v * 3.1415927f) + 1.0f) * 0.45f + 0.1f;
        }
    }
    // ridges along the four borders
    float amp = (float)(rand() % 15817) * 0.00018966934f + 7.0f;
    int* lim = &h;
    for (int pass = 0; pass < 2; pass++) {
        for (int b = 0; b < 13; b++) {
            if (*lim >= 0) {
                float k = (float)sin((1.0f - (float)b * 0.07692308f) * 3.2f) * amp;
                for (int a = 0; a <= *lim; a++) {
                    int x1 = b, y1 = a, x2 = w - b, y2 = a;
                    if (pass != 0) {
                        x1 = a; y1 = b; x2 = a; y2 = h - b;
                    }
                    terrain_cell* c1 = &cell[x1][y1];
                    float p1[3], n1[3];
                    p1[0] = off[0] + c1->px;
                    p1[1] = off[1] + c1->py;
                    p1[2] = off[2] + c1->pz;
                    noise_fractal(40.0f, 0.6f, 4, 0, p1, n1);
                    float v1 = (n1[2] + n1[2] + 0.4f) * k;
                    if (c1->pz < v1) { c1->pz = v1; c1->h2 = v1; }
                    terrain_cell* c2 = &cell[x2][y2];
                    float p2[3], n2[3];
                    p2[0] = off[0] + c2->px;
                    p2[1] = off[1] + c2->py;
                    p2[2] = off[2] + c2->pz;
                    noise_fractal(40.0f, 0.6f, 4, 0, p2, n2);
                    float v2 = (n2[2] + n2[2] + 0.4f) * k;
                    if (c2->pz < v2) { c2->pz = v2; c2->h2 = v2; }
                    if (b == 0) { c1->f90 = 0; c2->f90 = 0; }
                }
            }
        }
        lim = &w;
    }
    // random brush strokes, coarse to fine
    int lv = (int)(flags >> 14 & ((unsigned)g_bit[7] - 1));
    if (lv < 0) lv = 0;
    else if (lv > 99) lv = 99;
    float ratio = scale / ((float)pow(2.0, (double)((100 - lv) * 0.045f)) * 4.0f);
    if (ratio < 2.0f) ratio = 2.0f;
    int steps = (int)(log(ratio) * 1.442695f);
    float strength = 1.0f;
    for (int k = 0; k <= steps; k++) {
        float density;
        if (k == steps)
            density = (ratio - (float)g_bit[steps]) / (float)g_bit[steps];
        else
            density = 1.0f;
        seed_brushes(scale / (float)g_bit[k], density, strength);
        strength = strength - 0.05f;
    }
    // shift so the lowest cell sits at zero
    float lo = 10000.0f;
    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++)
            if (cell[x][y].pz < lo) lo = cell[x][y].pz;
    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++) {
            float v = -lo + cell[x][y].pz;
            cell[x][y].pz = v;
            cell[x][y].h2 = v;
        }
}

// @ 0x471a30
float Terrain::sun_visibility(const float* p)
{
    vector4 a(0.0f, 0.0f, 0.0f);
    vector4 d;
    {
        vector4 amb;                       // light colour: unused; its slot is shared with the scratch below
        world->sample_lights(&a, &d, &amb);
    }
    vector4 s;
    s.x = d.x * 1000.0f; s.y = d.y * 1000.0f; s.z = d.z * 1000.0f;
    a.x = p[0] - s.x; a.y = p[1] - s.y; a.z = p[2] - s.z;
    s.x = d.x * 0.05f; s.y = d.y * 0.05f; s.z = d.z * 0.05f;
    d.x = p[0] - s.x; d.y = p[1] - s.y; d.z = p[2] - s.z;
    bool vis = g_use_shaders != 0 || world->hf->line_clear(&d, &a);
    if (vis)
        return 1.0f;
    return 0.0f;
}

// @ 0x471b30
void Terrain::light_particle(particle_list* l, int idx, bool trace)
{
    TerrainParticle* p = &l->items[idx];
    ptype* pt = &g_ptypes[p->type];
    if (g_use_shaders == 0 || g_options.i7c != 2) {
        for (int k = 0; k < p->nmesh; k++) {
            mesh*& m = p->meshes[k];
            if (m->res->flags & 0x40) {
                float light = 1.0f;
                if ((pt->flags & 0x80000i64) != 0 && ((int*)world->hf->cols[p->cx])[p->f0c + p->cy * 4 + 2] >= 0) {
                    vector4 n;
                    n.x = -sun_dir[0]; n.y = -sun_dir[1]; n.z = -sun_dir[2];
                    light = dot3(n, p->frame.u);
                    if (light <= 0.0f) light = 0.0f;
                }
                if (g_use_shaders == 0 || g_options.i7c == 0) {
                    vector4 sum, c;
                    AnimBox* bx = &pt->boxes[p->anim];
                    sum.x = bx->mx.x + bx->mn.x;
                    sum.y = bx->mn.y + bx->mx.y;
                    sum.z = bx->mn.z + bx->mx.z;
                    c.x = sum.x * 0.5f;
                    c.y = sum.y * 0.5f;
                    c.z = sum.z * 0.5f;
                    c.z = (c.z < 0.3f) ? 0.3f : c.z;
                    vector4 pos;
                    pos.x = c.x + p->frame.o.x;
                    pos.y = c.y + p->frame.o.y;
                    pos.z = c.z + p->frame.o.z;
                    if (c.z < 1.5f) {
                        vector4 d, d2, far_, near_;
                        vscale_to(d, *(vector4*)sun_dir, 1000.0f);
                        far_.x = pos.x - d.x; far_.y = pos.y - d.y; far_.z = pos.z - d.z;
                        vscale_to(d2, *(vector4*)sun_dir, 0.05f);
                        near_.x = pos.x - d2.x; near_.y = pos.y - d2.y; near_.z = pos.z - d2.z;
                        bool visible;
                        if (trace)
                            visible = world->trace_ray(&near_.x, &far_.x, 50.0f, 0, 0) != 0;
                        else
                            visible = world->hf->line_clear(&near_, &far_);
                        if (!visible)
                            light *= 0.1f;
                    }
                }
                vector4 wp;
                wp.w = 1.0f;
                for (int j = p->first[k]; j < p->last[k]; j++) {
                    float lf = light;
                    if (g_use_shaders != 0 && g_options.i7c == 1) {
                        vector4 q = m->points[m->vertices[j].index].a;
                        wp.x = q.x * m->local_frame.s.x + (q.z * m->local_frame.u.x + q.y * m->local_frame.f.x) + m->local_frame.o.x;
                        wp.y = q.x * m->local_frame.s.y + (q.z * m->local_frame.u.y + q.y * m->local_frame.f.y) + m->local_frame.o.y;
                        wp.z = q.x * m->local_frame.s.z + (q.z * m->local_frame.u.z + q.y * m->local_frame.f.z) + m->local_frame.o.z;
                        float occ;
                        g_shadow_map.test(&wp, &occ);
                        lf = occ * light;
                    }
                    vertex60& v = m->vertices[j];
                    vector4 col = rgl::unpack_color((const unsigned*)&v.f58);
                    vector4 t1, t2, t3, r;
                    vscale_to(t1, sun_diffuse, lf);
                    vscale_to(t2, sun_ambient, 1.5f);
                    t3.x = t1.x + t2.x; t3.y = t1.y + t2.y; t3.z = t2.z + t1.z;
                    r.x = t3.x * col.x; r.y = t3.y * col.y; r.z = t3.z * col.z;
                    v.color = rgl::pack_color(&r);
                }
                m->slots[0].f28 = 1;
            }
        }
    }
}

// @ 0x472020
void Terrain::light_all(particle_list* l, bool trace)
{
    for (int i = 0; i < l->count; i++)
        light_particle(l, i, trace);
}

// @ 0x472050
void Mission::update_prop_flags()
{
    if (g_cfg.enable_edit_mode != 0) {
        if (special)
            special->visible = (g_game->f1a20c == 0 && g_game->editor.active != 0) ? 1 : 0;
        for (unsigned i = 0; i < props.size(); i++) {
            // the flag statement is written once per arm: cl merges the two copies after allocation, but the two
            // zero stores are what makes the constant 0 worth a register (ebx), and the copies' scratch picks line up
            if (props[i].kind == 0) {
                if ((g_propkinds[props[i].id].flags & 0x4000) != 0) {
                    if (g_game->editor.active != 0 && g_game->f1a20c == 0)
                        props[i].ent->visible = 1;
                    else
                        props[i].ent->visible = 0;
                }
            } else if (props[i].kind == 1 || props[i].kind == 5) {
                if (g_game->editor.active != 0 && g_game->f1a20c == 0)
                    props[i].ent->visible = 1;
                else
                    props[i].ent->visible = 0;
            }
        }
    }
}

// the mark's clock (+0xd0) and stamp (+0xd8) are a Timer; MarkRec (mission.h) spells them as separate fields (stand-in view)
static inline void mark_restart(Timer& t) { t.start = g_ticks[t.clock]; }
// @ 0x472160
void Mission::replace_mark(int idx)
{
    int n = 0;
    recs[idx].delay = 0.1f;
    mark_restart(recs[idx].timer);
    recs[idx].cnt++;
    vector4* sv = &recs[idx].m.s;
    recs[idx].floor = 0;
    vector4* ov = &recs[idx].m.o;
    vector4 a;
    a.x = sv->x; a.y = sv->y; a.z = sv->z;
    if (recs[idx].cnt > 15) {
        recs[idx].cnt2++;
        recs[idx].cnt = 0;
        *(rgl::matrix*)sv = recs[idx].m2;
        vector4 t1, t2;
        vscale_to(t1, recs[idx].m.f, 0.7f);
        vscale_to(t2, *sv, -0.5f);
        a = t2 - t1;
    }
    vector4 b;
    do {
        b = vadd3(a, *ov);
        n++;
        float fz;
        face* fs;
        scene->floor_at(&b.x, &fz, &fs);
        b.z = fz;
        if (fs) {
            recs[idx].floor = fs;
            if (fabs(ov->z - fz) < 2.0f) {
                vector4 from = *ov;
                from.z = from.z + 0.7f;
                vector4 to = b;
                to.z = to.z + 0.7f;
                if (scene->trace_ray(&from.x, &to.x, 15.0f, 0, 0)) {
                    *ov = b;
                    goto done;
                }
            }
        }
        a.x = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 1.2f;
        a.y = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 1.2f;
    } while (n <= 100);
    if (recs[idx].floor == 0)
        recs[idx].floor = *(face**)scene->objs._Myfirst;
done:
    if (recs[idx].cnt == 0)
        recs[idx].m2 = *(rgl::matrix*)sv;
}

// @ 0x472460
template<class T> void rgl::pool<T>::free_values()
{
    for (typename std::list<T*>::iterator i = chunks.begin(); i != chunks.end(); ++i)
        delete[] *i;
    chunks.clear();
    freelist = 0;
}
template void rgl::pool<rgl::cellnode>::free_values();
template void rgl::pool<rgl::cellref>::free_values();


// @ 0x4724c0 (the _Ufill members of the vectors below; the fills are terrain_a's 0x46b500 .. 0x46b650)
template rec36* std::vector<rec36>::_Ufill(rec36*, size_t, const rec36&);
// @ 0x4724f0
template MVert* std::vector<MVert>::_Ufill(MVert*, size_t, const MVert&);
// @ 0x472520
template MPoly* std::vector<MPoly>::_Ufill(MPoly*, size_t, const MPoly&);
// @ 0x472550
template MEdge* std::vector<MEdge>::_Ufill(MEdge*, size_t, const MEdge&);

// @ 0x472580
template std::vector<MEdge>::iterator std::vector<MEdge>::erase(std::vector<MEdge>::iterator, std::vector<MEdge>::iterator);

// @ 0x4725d0
template std::vector<PlacedObj>::iterator std::vector<PlacedObj>::erase(std::vector<PlacedObj>::iterator, std::vector<PlacedObj>::iterator);

// @ 0x472630 (vector<rec136>::reserve's copy, onto terrain_a's _Uninit_copy 0x46b750)
template rec136* std::vector<rec136>::_Ucopy(std::vector<rec136>::iterator, std::vector<rec136>::iterator, rec136*);

// @ 0x467f80
float Missile::ammo_scale()
{
    if (ammo.item >= 0)
        return (float)g_items[ammo.item].weapon_length * 0.01f;
    return 0.8f;
}
// @ 0x472650
bool Missile::settle(const capsule* c)
{
    bool r = false;
    HeightField* hf = g_mission->scene->hf;
    if (hf) {
        const rgl::vector4* q = &c->p1;
        float h;
        int id;
        hf->sample(q->x, q->y, &h, &id);
        if (q->z < h) {
            r = true;
            if (id >= 0) {
                NavCell* gf = &g_mission->scene->ground->cells[id];
                vector4 pl = gf->trans;
                vector4 n;
                n.x = -pl.x; n.y = -pl.y; n.z = -pl.z;
                float t = (ammo_scale() + 0.02f) - ((h - q->z) * pl.z) / dot3(n, vel);
                vector4 d(t * vel.x, t * vel.y, t * vel.z);
                vector4* p = &pos;
                p->x += d.x; p->y += d.y; p->z += d.z;
                vector4* hp = &ent->local.o;
                *hp = *p;
            }
        }
    }
    return r;
}

// the range algorithms of std::vector<ArrSet> (arr_set.h): the destruction loop and the uninitialised fill (its catch(...)
// funclet 0x472848 destroys the constructed prefix, then rethrows)
// @ 0x4727a0
template void std::_Destroy_range(ArrSet*, ArrSet*, std::allocator<ArrSet>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x4727e0
template void std::_Uninit_fill_n(ArrSet*, unsigned, const ArrSet&, std::allocator<ArrSet>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x472848
// (catch funclet of the _Uninit_fill_n above)

// ---- std::stable_sort of 8-byte records with a function-pointer predicate (VC7.1 <algorithm>) ----------------------------
// @ 0x472870 (_Chunked_merge: merge adjacent chunks into the temp buffer)
template void std::_Chunked_merge(It8, It8, std::_Temp_iterator<P8>, int, int, Pred8);
// @ 0x472980 (merge)
template It8 std::merge(P8*, P8*, P8*, P8*, It8, Pred8);
// @ 0x472a20 (_Merge_backward)
template It8 std::_Merge_backward(It8, It8, P8*, P8*, It8, Pred8);
// @ 0x472ad0 (_Buffered_rotate)
template It8 std::_Buffered_rotate(It8, It8, It8, int, int, std::_Temp_iterator<P8>&);

// @ 0x472c50
void PhysSet::clamp_rec(int i)
{
    if (recs[i].world) {
        for (unsigned j = 0; j < recs[i].bones.size(); j++) {
            NewtonBody* b = NewtonRagDollBoneGetBody(recs[i].bones[j]);
            if (b) {
                vector4 v;
                NewtonBodyGetVelocity(b, &v.x);
                if (v.length() > 30.0f) {
                    v.normalize();
                    v.x *= 30.0f;
                    v.y *= 30.0f;
                    v.z *= 30.0f;
                    NewtonBodySetVelocity(b, &v.x);
                }
                NewtonBodyGetOmega(b, &v.x);
                if (v.length() > 9.0f) {
                    v.normalize();
                    v.x *= 9.0f;
                    v.y *= 9.0f;
                    v.z *= 9.0f;
                    NewtonBodySetOmega(b, &v.x);
                }
            }
        }
    }
}

// @ 0x472dc0
void PhysSet::update(float dt)
{
    float step = 0.018f;
    if (nworlds > 1) {
        step = 0.021f;
        if (nworlds > 2) {
            step = 0.024f;
            if (nworlds > 3)
                step = 0.026f;
        }
    }
    if (dt > 0.075f)
        step *= 1.5f;
    while (dt > 0.0001f) {
        float s;
        if (dt < step)
            s = dt;
        else
            s = step;
        dt -= s;
        for (unsigned i = 0; i < recs.size(); i++) {
            if (recs[i].world) {
                clamp_rec(i);
                NewtonUpdate(recs[i].world, s);
            }
        }
    }
}

// @ 0x472ed0
void rgl::world::update_probes(unsigned flags)
{
    for (NavVert* pr = ground->verts._Myfirst; pr < ground->verts._Mylast; pr++) {
        if (pr->level < -0.1f) {
            vector4 dir, col;
            light_at(pr->p, pr->n, &dir, &col, flags, 1.0f);
            pr->level = (col.x + col.y + col.z) * 0.33f;
        }
    }
}

// @ 0x472f90
void Agent::reset()
{
    anim[0].next = -1;
    anim[0].cur = -1;
    anim[0].sub = -1;
    anim[0].prev_mask = 0;
    anim[0].f10 = -1;
    anim[0].events = 0;
    anim[0].scale = 0;
    anim[0].rate_scale = 1.0f;
    anim[0].rate = 1.0f;
    anim[0].phase = -1.0f;
    anim[0].f28 = 100;
    anim[1].next = -1;
    anim[1].cur = -1;
    anim[1].sub = -1;
    anim[1].prev_mask = 0;
    anim[1].f10 = -1;
    anim[1].events = 0;
    anim[1].scale = 0;
    anim[1].rate_scale = 1.0f;
    anim[1].rate = 1.0f;
    anim[1].phase = -1.0f;
    anim[1].f28 = 100;
    ai.init();
    state = 0;
    flags = 0;
    troop = -1;
    stand_animation = 1;
    pos = vector4(0.0f, 0.0f, 0.0f);
    facing = vector4(0.0f, 1.0f, 0.0f);
    vel = vector4(0.0f, 1.0f, 0.0f);
    walk_forward_animation = 9;
    in_flags = 0;
    rot[0] = 0;
    rot[1] = 0;
    rot[2] = 0.15f;
    *(Vec2*)&vel_xy = Vec2(0.0f, 0.0f);        // assigned whole from a Vec2 temporary (vec2 has no constructors)
    anchor = vector4(-10000.0f, -100000.0f, -100000.0f);
    v98 = vector4(0.0f, 0.0f, 0.0f);
    prev_facing = vector4(0.0f, 0.0f, 0.0f);
    v56a4 = vector4(0.0f, 0.0f, 0.0f);
    fb0 = 1.0f;
    body = 0;
    floor_face = 0;
    floor_blend = 0;
    disabled = 0;
    mount_sync = 1;
    mount_vel_sync = 1;
    floor_sound_done = 1;
    phys_idx = -1000;
    mount_id = -1;
    mount_body = 0;
    scale = 1.0f;
    party = -1;
    party_stack = -1;
    seed = -1;
    need_name = 1;
    f7c = 0;
    defs = 0;
    f138 = -1;
    f134 = -1;
    horse = -1;
    rider = -1;
    weapon_flags = 0;
    variant = 0;
    cur = 0;
    pitch = 0;
    active = 0;
    f5610 = 0;
    block_anim_a = -1;
    aim_error = 0;
    has_blow = 0;
    anim[0].prev_mask = -1;
    anim[1].prev_mask = 0;
    f56d0 = 0;
    flags3c = 1;
    v56b4 = vector4(0.0f, 0.0f, 0.0f);
    gait = 1;
    gait_ratio = 0;
    channel = -1;
    nav_pos = vector4(100000.0f, 10000.0f, 100000.0f);
    slot_mesh[0] = 0; slot_mesh[1] = 0; slot_mesh[2] = 0; slot_mesh[3] = 0; links[0] = 0; links[1] = 0; links[2] = 0; links[3] = 0;
    B[0] = 0; B[1] = 0; B[2] = 0; B[3] = 0; disp[0] = 0; disp[1] = 0; disp[2] = 0; disp[3] = 0;
    slot_mesh4 = 0; meters[0] = 0; meters[1] = 0; meters[2] = 0; meters[3] = 0; name_label = 0;
    lods.extra_count = 0;
    lods.body = 0;
    num_override_items = 0;
    override_flags = 0;
    entry_no = -1;
    wield[0] = -1;
    wield[1] = -1;
    wield_prev[0] = -1;
    wield_prev[1] = -1;
    f56d8 = 0;
    f56dc = 0;
    prev_scale0 = 1.0f;
    prev_scale1 = 1.0f;
    weapon_mode = 0;
    mount_attempts = 0;
    xp_pool = 0;
    fx1ac = 0;
    fx1b0 = 0;
    fx1b4 = 0;
    fx1b8 = 0;
    f5830 = 2;
    dismount_state = 0;
    spawn.z = 90000.0f;
    slot_vars.tidy();
    nav_face = -1;
    charge_ramp = 1.0f;
    obstacle = 0;
    mount_timer.active = 0;
    tm.restamp();
    timer.restamp();
    t5800.restamp();
    weave_timer.restamp();
    t40.restamp();
    snd_stamp.restamp();
    t5918.restamp();
    t5810.restamp();
    hit_vec = vector4(0.0f, 0.0f, 0.0f);
    targeted_by = 0;
    queued = 0;
    hit_bone = 0;
    turn_rate2 = 0;
    kill_count = 0;
    wounded_count = 0;
}

// @ 0x473570
void Skin::copy_troop(ArrSet* dst, const rgl::lod_mesh* r)
{
    dst->name = r->name;
    for (int i = 0; i < r->count; i++) {
        if (r->lists[i].size() != 0)
            ((SkinApplyExt*)this)->apply_group(dst, i, r->lists[i][0]);
    }
}

// @ 0x4735e0
void Skin::relax(int skip, float* v)
{
    for (unsigned i = 0; i < (unsigned)constraints.size(); i++) {
        bool done = false;
        for (int iter = 0; !done; iter++) {
            Constraint* c = &constraints[i];
            float sum = 0.0f;
            int free_n = 0;
            for (int j = 0; j < c->n; j++) {
                sum += v[c->t[j].idx] * c->t[j].coef;
                if (c->t[j].idx != skip)
                    free_n++;
            }
            if ((c->mode == -1 && sum < c->limit) || (c->mode == 1 && c->limit < sum)) {
                float d = c->limit - sum;
                float cnt = (float)free_n;
                if (!(cnt > 1.0f))
                    cnt = 1.0f;
                float step = d / cnt;
                for (int j = 0; j < c->n; j++) {
                    if (c->t[j].idx != skip) {
                        v[c->t[j].idx] += step / c->t[j].coef;
                        if (v[c->t[j].idx] < 0.0f)
                            v[c->t[j].idx] = 0.0f;
                        else if (v[c->t[j].idx] > 1.0f)
                            v[c->t[j].idx] = 1.0f;
                    }
                }
                if (iter > 30)
                    done = true;
            } else {
                done = true;
            }
        }
    }
}

// @ 0x473800
Skin* Animator::unpack(const unsigned __int64* key)
{
    item = ((unsigned)key[0] & ((unsigned)g_bit[6] - 1)) - 1;
    set = ((unsigned)(key[0] >> 6) & ((unsigned)g_bit[6] - 1)) - 1;
    material = (unsigned)(key[0] >> 12) & ((unsigned)g_bit[6] - 1);
    variant = (unsigned)(key[0] >> 18) & ((unsigned)g_bit[6] - 1);
    Skin* r = &skin_vec[cur];
    if (item >= r->n_hair)
        item = -1;
    if (set >= r->n_beard)
        set = -1;
    if (material >= r->n_face_tex)
        material = 0;
    if (variant >= r->n_hair_tex)
        variant = 0;
    time = (float)(int)((unsigned)(key[0] >> 24) & ((unsigned)g_bit[6] - 1)) * 0.015873017f;
    scale = (float)(int)((unsigned)(key[0] >> 30) & ((unsigned)g_bit[6] - 1)) * 0.015873017f;
    int i = 0;
    for (r = &skin_vec[cur]; i < r->n_face_keys; r = &skin_vec[cur]) {
        int w = 1;
        int sh = i * 3;
        if (sh >= 63) {
            w = 2;
            sh -= 63;
        }
        weights[i] = (float)(int)((unsigned)(key[w] >> sh) & ((unsigned)g_bit[3] - 1)) * 0.14285715f;
        i++;
    }
    return r;
}

// @ 0x473980
void Terrain::refresh_cells()
{
    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            terrain_cell* c = &(&cell[0][0])[x * 251 + y];
            c->f40 = ((TerrainExt*)this)->sun_visibility(&c->px);
        }
    }
}

// @ 0x4739f0
void Mission::reset_spawned(int party_id)
{
    if (party_id != -1) {
        PartyMgr* pm = &g_game->parties;
        int i = pm->lookup(party_id);
        Party* p = &pm->pages[i >> 4][i & 15];
        for (int k = 0; k < p->nstacks; k++)
            p->stacks[k].spawned = 0;
    }
}

// helpers of pick_spawn (0x473a60): the party table lookup and the low-hp test, inlined in the original
inline bool agent_low_inl(int idx)
{
    if (idx == g_game->player)
        return g_game->troops[idx].hp < g_module_settings.player_wounded_treshold;
    return g_game->troops[idx].hp < g_module_settings.hero_wounded_treshold;
}
inline Party* get_party(int id)
{
    PartyMgr* pm = &g_game->parties;
    int i = pm->lookup(id);
    return &pm->pages[i >> 4][i & 15];
}
// @ 0x473a60
void Mission::pick_spawn(int idx, int* out_party, int* out_stack, int* out_troop, int* out_side, int* out_flag, int* out_seed)
{
    SpawnRec* r = &g_mission_templates[mode].recs[idx];
    *out_party = -1;
    *out_stack = -1;
    *out_side = -1;
    *out_flag = -1;
    *out_seed = -1;
    *out_troop = -1;
    *out_side = ((r->flags >> 12) & 7) - 1;
    int list_idx = -1;
    int local_side = -1;
    if (r->flags & 0x10) {
        *out_flag = 0;
        *out_troop = g_game->scenes[scene_idx].visitors[idx].troop;
        if (g_game->scenes[scene_idx].visitors[idx].dna == -0xd0cd)
            *out_seed = g_game->f4 * 0x93 + r->f1c * 0x13 + idx * 0x25 + scene_idx * 0x4f;
        else
            *out_seed = g_game->scenes[scene_idx].visitors[idx].dna;
    } else if (r->flags & 4) {
        if (!(r->id == f5df8)) { if (*out_troop == -1 && f6550 == 0) {
            for (int i = 1; i < g_game->ntroops; i++) {
                unsigned v = g_game->troops[i].site;
                if ((v & 0xffff) == (unsigned)scene_idx && ((v >> 16) & 0xff) == (unsigned)r->id && g_game->troops[i].used == 0)
                    *out_troop = i;
            }
        } } else {
            *out_troop = g_game->player;
        }
    } else if (r->flags & 0xc0) {
        int a = r->flags & 0x40;
        if (((a && f6214 == 0) || ((r->flags & 0x80) && f6214 == 1)) || (f6214 == 2 && ((f6218 == 1 && a) || (f6218 == 0 && (r->flags & 0x80))))) {
            local_side = 0;
            if (g_game->troops[g_game->player].used == 0) {
                *out_party = g_game->main_party;
            } else {
                int k = rand_below(&cnt[1 - f6218]);
                *out_party = list[1 - f6218][k];
                list_idx = 1 - f6218;
            }
        } else {
            local_side = 1;
            int k = rand_below(&cnt[f6218]);
            *out_party = list[f6218][k];
            list_idx = f6218;
        }
    } else if (r->flags & 1) {
        int* pc = &cnt[f6218];
        int k;
        if (*pc <= 1)
            k = 0;
        else
            k = rand() % *pc;
        *out_party = list[f6218][k];
        list_idx = f6218;
        *out_side = 1;
    } else {
        if (r->flags & 2) {
            int k = rand_below(&cnt[1 - f6218]);
            *out_party = list[1 - f6218][k];
            list_idx = 1 - f6218;
        } else {
            *out_party = g_game->main_party;
        }
        *out_side = 0;
    }

    if (*out_party >= 0) {
        Party* party = get_party(*out_party);
        int end = party->nstacks;
        int start = 0;
        int step = 1;
        if (r->flags & 0x1000000) {
            end = -1;
            step = -1;
            start = party->nstacks - 1;
        }
        int total = party->sum_stat(0x112);
        char x4 = 0, x3 = 0, x5 = 0;
        int ncand = (list_idx == -1) ? 1 : cnt[list_idx];
        if (idx >= 0)
            done[idx] = 1;
        for (int c = 0; c < ncand; c++) {
            int tmpl = (list_idx == -1) ? g_game->main_party : list[list_idx][c];
            Party* p2 = get_party(tmpl);
            int n2 = p2->nstacks;
            int nfirst;
            for (nfirst = 0; nfirst < n2; nfirst++)
                if (p2->stacks[nfirst].flags & 1)
                    break;
            if (nfirst > 0) {
                for (int j = 0; j < nfirst; j++) {
                    Stack* s = &p2->stacks[j];
                    Troop* t = &g_game->troops[s->troop];
                    if ((t->flags30s & 0x10) == 0 && s->count > s->wounded + s->spawned) {
                        done[idx] = 0;
                        if ((t->flags30s & 0x1000000) && (r->flags2 & 0x100) == 0)
                            x5 = 1;
                        else if (t->flags30s & 0x4000000)
                            x4 = 1;
                        else
                            x3 = 1;
                    } else if (s->spawned == 0) {
                        if (!agent_low_inl(s->troop)) {
                            done[idx] = 0;
                            if (t->equip[8].item >= 0)
                                x5 = 1;
                            else
                                x3 = 1;
                        }
                    }
                }
            }
        }
        if (*out_party == g_game->main_party && party->stacks[0].spawned == 0) {
            *out_stack = 0;
            *out_troop = g_game->player;
            party->stacks[0].spawned = 1;
        }
        if (end != start) {
            for (int k = start; k != end; k += step) {
                if (*out_troop != -1)
                    break;
                Stack* s = &party->stacks[k];
                Troop* t = &g_game->troops[s->troop];
                __int64 hero = t->flags30s & 0x10;
                if ((hero && s->spawned == 0 && t->used == 0 && (s->troop == g_game->player || !g_game->agent_low(s->troop)))
                    || (!hero && s->count > s->wounded + s->spawned)) {
                    if (rule_accepts(r, s, k, x3, x4, x5)) {
                        if (hero == 0) {
                            float chance = (float)(s->count - s->wounded) / (float)total + 0.25f;
                            float rnd = (float)(rand() % 15817) * (1.0f / 15817.0f);
                            if (rnd > chance)
                                continue;
                        }
                        *out_stack = k;
                        *out_troop = s->troop;
                        if ((g_game->troops[s->troop].flags30s & 0x10) == 0) {
                            *out_seed = rand();
                            if (s->spawned >= 0 && s->spawned < 32) {
                                if (*out_party == g_game->main_party)
                                    *out_seed = g_game->stack_seeds[k][s->spawned];
                            } else {
                                *out_seed = rand();
                            }
                        }
                        s->spawned++;
                    }
                }
            }
        }
    }
    if (*out_troop >= 0) {
        g_game->troops[*out_troop].used = 1;
        if (g_game->troops[*out_troop].flags30s & 0x10)
            *out_seed = -1;
    }
    if (*out_side == -1)
        *out_side = local_side;
}

// @ 0x474300
template<> void Pool<Missile>::next_used(unsigned* i)
{
    ++*i;
    while (valid(*i)) {
        if (pages[*i >> 4][*i & 15].in_use != 0)
            return;
        ++*i;
    }
}

// @ 0x474350 (std::vector<ArrSet>'s, onto terrain_a's _Uninit_copy 0x46c1d0)
template ArrSet* std::vector<ArrSet>::_Ucopy(ArrSet*, ArrSet*, ArrSet*);

// @ 0x474370 (_Insertion_sort)
template void std::_Insertion_sort(It8, It8, Pred8);
// @ 0x474410 (_Buffered_merge: merge two sorted runs using the temp buffer)
template void std::_Buffered_merge(It8, It8, It8, int, int, std::_Temp_iterator<P8>&, Pred8);
// @ 0x474650 (_Chunked_merge: merge chunk pairs from the temp buffer back into the vector)
template void std::_Chunked_merge(P8*, P8*, It8, int, int, Pred8);

#define RAND() (g_rand_seed = (g_rand_seed * 0x1c13 - 0xd098556) * 0x1105)
#define RND01() ((float)(RAND() % 0x3dc9) * 6.322312e-05f)

static __forceinline int round_int(const float& x)
{
    float fl = floor(x);
    int r = (int)fl;
    return (x - fl <= 0.5f) ? r : r + 1;
}
static __forceinline int lerp_int(int a, const int& b)
{
    float t = RND01() * (b - a); return a + round_int(t);
}

#define MIX(k) { RAND(); float lo = save[i + k]; weights[i + k] = (weights[i + k] - lo) * ((float)(RAND() % 0x3dc9) * 0.00012644623f) * 0.5f + lo; }

// @ 0x4746e0
void Animator::breed(const unsigned __int64* a, const unsigned __int64* b, int seed)
{
    g_rand_seed = (seed + 0x8a767) * 0x1ddcf5f + 0x2f784719;
    unpack(a);
    int s3 = variant; float a0 = scale; float a1 = time;
    int s1 = item; int s0 = set; int s5 = material;
    float save[63];
    int i;
    for (i = 0; i < 63; i += 9) {
        save[i] = weights[i]; save[i+1] = weights[i+1]; save[i+2] = weights[i+2];
        save[i+3] = weights[i+3]; save[i+4] = weights[i+4]; save[i+5] = weights[i+5];
        save[i+6] = weights[i+6]; save[i+7] = weights[i+7]; save[i+8] = weights[i+8];
    }
    unpack(b);
    item = lerp_int(s1, item);
    set = lerp_int(s0, set);
    material = lerp_int(s5, material);
    variant = -1;
    FaceTexture* r = &skin_vec[cur].face_textures[material];
    std::vector<int>* v = &r->hair_materials;
    int n;
    if (v->size() == 0 || (n = (int)v->size()) <= 0) {
        variant = lerp_int(s3, variant);
    } else {
        RAND();
        variant = skin_vec[cur].face_textures[material].hair_materials[g_rand_seed % (unsigned)n];
    }
    scale = (float)(RAND() % 0x3dc9) * (scale - a0) * 6.322312e-05f + a0;
    time = (float)(RAND() % 0x3dc9) * (time - a1) * 6.322312e-05f + a1;
    for (i = 0; i < 63; i += 9) {
        MIX(0) MIX(1) MIX(2) MIX(3) MIX(4) MIX(5) MIX(6) MIX(7) MIX(8)
    }
    skin_vec[cur].relax(-1, weights);
}

// inline vector4 arithmetic as the game's header implements it: 3-component results, leaving the fourth lane
// uninitialised (whole-struct copies carry it along)
static __forceinline vector4 vsum(const vector4& a, const vector4& b)
{
    return vector4(a.x + b.x, a.y + b.y, a.z + b.z);
}
// statement-form sum: the original squares y, then x, then z; a single expression comes out x, y, z
static inline float len2(const vector4& v)
{
    float z = v.z; float y = v.y; float x = v.x; float s = y * y; s += x * x; s += z * z; return s;
}
static __forceinline float normalize_inline(vector4& v)
{
    float len = v.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        goto scale;   // stand-in: the original's store of inv and its first use are in separate blocks; source of that split unknown
scale:
        v.x = v.x * inv;
        v.y = v.y * inv;
        v.z = v.z * inv;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
    return len;
}
static __forceinline float normalize_dir(vector4& v)
{
    float len = (float)sqrt(len2(v));
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv;
        v.y = v.y * inv;
        v.z = v.z * inv;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
    return len;
}
// Helpers of update_team_stats (0x474e60). The shapes they give are load-bearing: a const-reference parameter keeps a
// materialised address (team_leader gives the team pointer its zero displacement, so the class pointer is a separate lea).
inline int AgentTable::first_used()
{
    unsigned v = 0;
    if (0 < count) {
        Agent** pg = &pages[0];
        do {
            if (pg[v >> 4][v & 0xf].in_use != 0) break;
            v++;
        } while ((int)v < count);
    }
    return v;
}
static inline Agent& agent_get(AgentTable& t, unsigned id) { return t.pages[id >> 4][id & 0xf]; }
static inline void vec_div(vector4& v, const float& d)
{
    float inv = 1.0f / d;
    v.x = inv * v.x;
    v.y = inv * v.y;
    v.z = inv * v.z;
}
static inline float dist(const vector4& a, const vector4& b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return (float)sqrt(dx * dx + (dy * dy + dz * dz));
}
static __forceinline void team_totals(TeamRec& tm_)
{
    for (int k = 0; k < 3; k++) {
        float n = tm_.cls[k].count;
        for (int j = 0; j < 3; j++) {
            if (j != k) {
                if (dist(tm_.cls[k].pos, tm_.cls[j].pos) < 1.0f)
                    n = n + tm_.cls[j].count;
            }
        }
        tm_.cls[k].half = n * 0.5f;
        tm_.cls[k].total = n;
    }
}
static inline int team_of(const Agent& a)
{
    int t = a.team;
    if (t < 0 || t >= 8) t = 7;
    return t;
}
static inline int team_leader(const TeamRec& tm) { return tm.leader; }
static inline int team_rel(const TeamRec& tm, int other) { return tm.relation[other]; }
static inline bool agent_live(AgentTable& t, int id)
{
    return id >= 0 && id < t.count && agent_get(t, id).in_use != 0;
}
static inline float timer_secs(const Timer& t) { return (float)(g_ticks[t.clock] - t.start) * 1e-5f; }
static inline void set_delay_inl(Timer& t, float seconds) { t.start = g_ticks[t.clock] - (int)(seconds * 100000.0f); }

// @ 0x474e60
void Mission::update_team_stats()
{
    for (int t = 0; t < 8; t++) {
        if (teams[t].leader >= 0) {
            for (int k = 0; k < 3; k++) {
                if (teams[t].cls[k].kind == 2) {
                    teams[t].cls[k].pos = agent_get(agents, teams[t].leader).pos;
                    teams[t].cls[k].nav = agent_get(agents, teams[t].leader).update_nav_face();
                    teams[t].cls[k].target = agent_get(agents, teams[t].leader).v56b4;
                }
            }
        }
    }
    if (timer_secs(tm) >= 0.0f) {
        set_delay_inl(tm, -1.1f);
        {
        vector4 zero(0.0f, 0.0f, 0.0f);
        for (int t = 0; t < 8; t++)
            for (int k = 0; k < 3; k++) {
                teams[t].cls[k].count = 0.0f;
                teams[t].cls[k].sum = zero;
            }
        }

        {
        unsigned i = agents.first_used();
        while (i < (unsigned)agents.count) {
            Agent& a = agent_get(agents, i);
            if (a.state == 1 && a.kind == 0) {
                int tid = team_of(a);
                int cl = a.agent_class();
                a.class_rank = (int)(teams[tid].cls[cl].count > 0.0f ? teams[tid].cls[cl].count : teams[tid].cls[cl].count - 0.99999988f);
                if (tid >= 0 && tid < 8 && cl >= 0 && cl < 3) {
                    teams[tid].cls[cl].count = teams[tid].cls[cl].count + 1.0f;
                    teams[tid].cls[cl].sum.x = a.pos.x + teams[tid].cls[cl].sum.x;
                    teams[tid].cls[cl].sum.y = a.pos.y + teams[tid].cls[cl].sum.y;
                    teams[tid].cls[cl].sum.z = a.pos.z + teams[tid].cls[cl].sum.z;
                }
            }
            agents.next_used(&i);
        }
        }

        {
        for (int t = 0; t < 8; t++) {
            team_totals(teams[t]);
            if (team_leader(teams[t]) >= 0 && agent_get(agents, team_leader(teams[t])).state != 1) {
                bool found = false;
                unsigned i2 = agents.first_used();
                while (i2 < (unsigned)agents.count) {
                    if (!found) {
                        Agent& a = agent_get(agents, i2);
                        if (a.state == 1 && a.kind != 1) {
                            int tid = team_of(a);
                            if (tid == t) {
                                found = true;
                                teams[t].leader = i2;
                            }
                        }
                    }
                    agents.next_used(&i2);
                }
            }
        }

        }

        {
        for (int t = 0; t < 8; t++) {
            for (int k = 0; k < 3; k++) {
                vec_div(teams[t].cls[k].sum, teams[t].cls[k].count);
            }
        }
        }

        {
            unsigned i3 = agents.first_used();
            while (i3 < (unsigned)agents.count) {
                Agent& a = agent_get(agents, i3);
                if (a.state == 1 && a.kind != 1) {
                    int tid = team_of(a);
                    int cl = a.agent_class();
                    if (tid >= 0) {
                        a.ai.hand_off.x = (float)a.class_rank - teams[tid].cls[cl].count * 0.5f;
                        if (tid == 2) a.ai.hand_off.y = 1.2f;
                        else if (tid == 1) a.ai.hand_off.y = 0.0f;
                        else if (tid == 0) a.ai.hand_off.y = -0.7f;
                    }
                }
                agents.next_used(&i3);
            }
        }

        {
        for (int t = 0; t < 8; t++) {
            if (teams[t].leader >= 0) {
                for (int k = 0; k < 3; k++) {
                    vector4 v;
                    v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
                    unsigned i4 = agents.first_used();
                    while (i4 < (unsigned)agents.count) {
                        Agent& a = agent_get(agents, i4);
                        if (a.state == 1 && a.kind != 1) {
                            int tid = team_of(a);
                            a.agent_class();
                            if (team_rel(teams[t], tid) == -1) {
                                vector4 d = a.pos - teams[t].cls[k].pos;
                                float len = normalize_inline(d);
                                if (len < 2.0f) len = 2.0f;
                                else if (len > 400.0f) len = 400.0f;
                                float s = 1.0f / len;
                                vector4 sd = d * s;            // the scaled direction is a vector4 temporary (float slots, rounded)
                                v.x = v.x + sd.x;
                                v.y = v.y + sd.y;
                                v.z = v.z + sd.z;
                            }
                        }
                        agents.next_used(&i4);
                    }
                    v.z = 0.0f;
                    if (v.length() > 1e-11f) {
                        normalize_inline(v);
                        if (len2(teams[t].cls[k].dir) < 0.5f) {
                            teams[t].cls[k].dir = v;
                        } else {
                            teams[t].cls[k].dir = vsum(teams[t].cls[k].dir * 0.8f, v * 0.2f);
                            normalize_dir(teams[t].cls[k].dir);
                        }
                    }
                }
            }
        }
        }

        {
            unsigned i5 = agents.first_used();
            while (i5 < (unsigned)agents.count) {
                agent_get(agents, i5).targeted_by = 0;
                agents.next_used(&i5);
            }
        }
        {
            unsigned i6 = agents.first_used();
            while (i6 < (unsigned)agents.count) {
                Agent& a = agent_get(agents, i6);
                if (agent_live(agents, a.ai.target))
                    agent_get(agents, a.ai.target).targeted_by++;
                agents.next_used(&i6);
            }
        }
    }
}

// @ 0x4759c0
void Mission::update_team_counts()
{
    int old[8][6];
    int* dst = &old[0][0];
    int* src = &counts[0][0];
    for (int t = 8; t != 0; t--) {
        for (int j = 6; j != 0; j--) {
            *dst = *src;
            *src = 0;
            dst++;
            src++;
        }
    }
    unsigned i = agents.first_used();
    while (i < (unsigned)agents.count) {
        Agent& a = agents.pages[i >> 4][i & 0xf];
        if (a.kind == 0 && a.team >= 0 && a.team < 8)
            counts[a.team][a.state]++;
        agents.next_used(&i);
    }
    for (int t = 0; t < 8; t++) {
        for (int j = 0; j < 6; j++)
            if (old[t][j] != counts[t][j]) changed_at = (float)g_ticks[2] * 1e-5f;
    }
}

// @ 0x475b60
template void std::vector<ArrSet>::_Destroy(ArrSet*, ArrSet*);
// @ 0x475b80
template ArrSet* std::vector<ArrSet>::_Ufill(ArrSet*, size_t, const ArrSet&);

// @ 0x475bb0 (_Buffered_merge_sort)
template void std::_Buffered_merge_sort(It8, It8, int, std::_Temp_iterator<P8>&, Pred8);

rgl::cell::~cell() {}                // 0x431ab0: defined here so that the array delete below is emitted as in the original

// @ 0x475c60
__declspec(noinline) rgl::cell_grid::~cell_grid()
{
    for (int i = 0; i < n[0]; i++)
        if (cells[i])
            delete[] cells[i];
    delete[] cells;
    nodes.free_values();
    refs.free_values();
}

// @ 0x475d50
rgl::world::~world()
{
    reset();
}

// @ 0x475e50
AgentAi::AgentAi()
{
    Vec2 z; z.x = 0.0f; z.y = 0.0f;
    hand_off = z;
}

// @ 0x475f20
template std::vector<ArrSet>::iterator std::vector<ArrSet>::erase(std::vector<ArrSet>::iterator, std::vector<ArrSet>::iterator);
// @ 0x475f70 (_Stable_sort, recursive)
template void std::_Stable_sort(It8, It8, int, std::_Temp_iterator<P8>&, Pred8);

// ---- vector<> instantiations of the object's records (VC7.1) ---------------------------------------------------------
// @ 0x476020 vector<E36x>::reserve (its catch(...) funclet follows at 0x476111), 0x476130 vector<rec136>::reserve
// (funclet 0x476225), 0x476240 vector<E36x>::insert, 0x4762b0 vector<rec136>::insert, 0x476320 vector<PlacedObj>::_Buy.
template void std::vector<E36x>::reserve(size_t);
template void std::vector<rec136>::reserve(size_t);
template std::vector<E36x>::iterator std::vector<E36x>::insert(std::vector<E36x>::iterator, const E36x&);
template std::vector<rec136>::iterator std::vector<rec136>::insert(std::vector<rec136>::iterator, const rec136&);
template bool std::vector<PlacedObj>::_Buy(size_t);

// @ 0x476370 (+ 0x476515: catch funclet and cold branches)
template void std::vector<rec36>::_Insert_n(std::vector<rec36>::iterator, size_t, const rec36&);
// @ 0x476620 (+ 0x4767de)
template void std::vector<MVert>::_Insert_n(std::vector<MVert>::iterator, size_t, const MVert&);
// @ 0x4768f0 (+ 0x476a9a)
template void std::vector<MPoly>::_Insert_n(std::vector<MPoly>::iterator, size_t, const MPoly&);
// @ 0x476bb0 (+ 0x476d51: catch funclet and cold branches)
template void std::vector<MEdge>::_Insert_n(std::vector<MEdge>::iterator, size_t, const MEdge&);
// @ 0x476e60 (+ 0x47701d, 0x4770dc)
template void std::vector<ArrSet>::_Insert_n(std::vector<ArrSet>::iterator, size_t, const ArrSet&);

// 0x477160 vector<PlacedObj> assignment (no marker: a scope-less operator= annotation would capture every operator= callee)
template std::vector<PlacedObj>& std::vector<PlacedObj>::operator=(const std::vector<PlacedObj>&);
// @ 0x4772f0
template void std::vector<E36x>::push_back(const E36x&);
// @ 0x477380
template void std::vector<rec136>::push_back(const rec136&);
// @ 0x477410
template std::vector<rec36>::iterator std::vector<rec36>::insert(std::vector<rec36>::iterator, const rec36&);
// @ 0x477480
template void std::vector<MVert>::resize(size_t, MVert);
// @ 0x477530
template std::vector<MVert>::iterator std::vector<MVert>::insert(std::vector<MVert>::iterator, const MVert&);
// @ 0x4775a0
template void std::vector<MPoly>::resize(size_t, MPoly);
// @ 0x477640
template std::vector<MPoly>::iterator std::vector<MPoly>::insert(std::vector<MPoly>::iterator, const MPoly&);
// @ 0x4776b0
template void std::vector<MEdge>::resize(size_t, MEdge);
// @ 0x477750
template std::vector<MEdge>::iterator std::vector<MEdge>::insert(std::vector<MEdge>::iterator, const MEdge&);
// @ 0x4777c0
template void std::vector<PlacedObj>::resize(size_t, PlacedObj);
// @ 0x477880
template void std::vector<ArrSet>::resize(size_t, ArrSet);

// @ 0x477940 std::copy over PhysRec (operator= is memberwise: vector<int>::operator= 0x43f0a0 for the two vectors)
PhysRec* __cdecl copy_e56(PhysRec* first, PhysRec* last, PhysRec* dest)
{
    for (; first != last; ++first, ++dest)
        *dest = *first;
    return dest;
}

// @ 0x4779b0
void mesh_flip::flip_faces()
{
    if (!local_faces) {
        faces = parent->faces;
        local_faces = 1;
    }
    for (unsigned i = 0; i < faces.size(); i++) {
        int t = faces[i].a;
        faces[i].a = faces[i].c;
        faces[i].c = t;
    }
    rgl::slot& s1 = slots[1];
    rgl::slot& s0 = slots[0];
    s1.f28 = 1;
    s0.f28 = 1;
}

// @ 0x477a30
Agent::Agent()
{
}

// @ 0x477b70
void EmptySlot::slot() {}

// @ 0x477b80
template void std::vector<rec36>::push_back(const rec36&);
// @ 0x477c10
template void std::vector<MVert>::push_back(const MVert&);
// @ 0x477ca0 resize(n) pads with MPoly(): the library wrapper, out of line (the 2-arg overload is 0x4775a0)
template<>
__declspec(noinline) void std::vector<MPoly>::resize(size_t _Newsize)
{
    size_t n = _Newsize;
    MPoly _Val;
    int* lim = _Val.lim;
    lim[0] = -100000;
    _Val.stamp = 0;
    _Val.f5c = 0;
    _Val.flag = 0;
    _Val.n = 0;
    for (int i = 1; i < 8; i++)
        lim[i] = -100000;
    std::vector<MPoly>::resize(n, _Val);
}
// @ 0x477d20
template void std::vector<MPoly>::push_back(const MPoly&);
// @ 0x477db0 resize(n) pads with MEdge(): the library wrapper, out of line (the 2-arg overload is 0x4776b0)
template<>
__declspec(noinline) void std::vector<MEdge>::resize(size_t _Newsize)
{
    MEdge _Val;
    _Val.flag = 0;
    _Val.crease[0] = 1.0f;
    _Val.crease[1] = 1.0f;
    int* lim = &_Val.v0;
    lim[0] = -10000;
    lim[1] = -10000;
    lim[2] = -10000;
    lim[3] = -10000;
    _Val.nfaces = 0;
    std::vector<MEdge>::resize(_Newsize, _Val);
}
// @ 0x477e10
template void std::vector<MEdge>::push_back(const MEdge&);
// @ 0x477ea0 resize(n) pads with PlacedObj() (the inlined constructor 0x437830)
template void std::vector<PlacedObj>::resize(size_t);

// ---- id pools ---------------------------------------------------------------------------------------------------
// @ 0x477f00
template<> void Pool<Agent>::reset()
{
    free_ids.resize(0, 0);
    for (int i = 0; i < count; i++) {
        pages[(unsigned)i >> 4][i & 0xf].in_use = 0;
        free_ids.push_back(i);
    }
}
// @ 0x477f90
template<> void Pool<Missile>::reset()
{
    free_ids.resize(0, 0);
    for (int i = 0; i < count; i++) {
        pages[(unsigned)i >> 4][i & 0xf].in_use = 0;
        free_ids.push_back(i);
    }
}
// @ 0x478020
template<> int Pool<Agent>::alloc()
{
    if (!(free_ids.size() > 0)) {
        Agent* blk = new Agent[16];
        pages.push_back(blk);
        for (int j = 0; j < 16; j++) {
            blk[j].in_use = 0;
            blk[j].pool_id = count + j;
            free_ids.push_back(count + j);
        }
        count += 16;
    }
    unsigned id = free_ids.back();
    free_ids.pop_back();
    Agent* r = &pages[id >> 4][id & 0xf];
    r->in_use = 1;
    r->serial = serial;
    serial++;
    return id;
}

// ---- the nav map (PolyMesh) ---------------------------------------------------------------------------------------------
// @ 0x478150
NavData::NavData()
{
    e44 = vector4(0.0f, 0.0f, 0.0f);
    f54 = vector4(0.0f, 0.0f, 0.0f);
    m64 = 0;
}

// @ 0x4781d0
void PolyMesh::load(stream_base* s)
{
    int n1, n2, n3;
    s->r_int(&n1);
    MVert v;
    v.flag = 0;
    verts.resize(n1, v);
    for (int i = 0; i < n1; i++) {
        MVert* p = &verts[i];
        s->r_float(&p->pos.x).r_float(&p->pos.y).r_float(&p->pos.z);
    }
    s->r_int(&n2);
    MEdge e;
    edges.resize(n2, e);
    for (int i = 0; i < n2; i++) {
        s->r_int(&edges[i].nfaces);
        s->r_int(&edges[i].v0);
        s->r_int(&edges[i].v1);
        s->r_int(&edges[i].face[0]);
        s->r_int(&edges[i].face[1]);
    }
    s->r_int(&n3);
    faces.resize(n3);
    for (int i = 0; i < n3; i++) {
        s->r_int(&faces[i].n);
        for (int j = 0; j < faces[i].n; j++)
            s->r_int(&faces[i].vid[j]);
        for (int j = 0; j < faces[i].n; j++)
            s->r_int(&faces[i].eid[j]);
        s->r_int(&faces[i].f5c);
    }
}

// @ 0x478450
int PolyMesh::add_edge(int a, int b)
{
    raw_t raw;
    MEdge e(raw);
    e.face[0] = -10000;
    e.face[1] = -10000;
    e.v0 = a;
    e.flag = 0;
    e.nfaces = 0;
    e.crease[0] = 1.0f;
    e.crease[1] = 1.0f;
    e.v1 = b;
    edges.push_back(e);
    return (int)edges.size() - 1;
}

// @ 0x4784d0
int PolyMesh::add_face(int n, int v0, int v1, int v2, int v3, int e0, int e1, int e2, int e3)
{
    raw_t raw;
    MPoly f(raw);
    f.n = n;
    f.vid[0] = v0; f.vid[1] = v1; f.vid[2] = v2; f.vid[3] = v3;
    f.stamp = 0;
    f.eid[0] = e0; f.eid[1] = e1;
    f.f5c = 0;
    f.flag = 0;
    f.eid[2] = e2; f.eid[3] = e3;
    faces.push_back(f);
    return (int)faces.size() - 1;
}

// @ 0x478580
void PolyMesh::clear()
{
    MVert v;
    v.flag = 0;
    verts.resize(0, v);
    edges.resize(0);
    faces.resize(0);
}

// @ 0x4785e0
void PolyMesh::build(HeightField* hf, int step_)
{
    step = step_;
    clear();
    int nx = (hf->w - 0x18) / step;
    int ny = (hf->h - 0x18) / step;
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            MVert v;
            v.flag = 0;
            v.pos = vector4((float)(step * ix + 0xc) * hf->cell, (float)(step * iy + 0xc) * hf->cell,
                            hf->cols[step * ix + 0xc][step * iy + 0xc].h + 0.4f);
            verts.push_back(v);
        }
    }
    for (int r = 0; r < ny; r++)
        for (int k = 0; k < nx - 1; k++)
            add_edge(r * nx + k, r * nx + k + 1);
    int nh = (nx - 1) * ny;
    for (int r = 0; r < ny - 1; r++)
        for (int k = 0; k < nx; k++)
            add_edge(r * nx + k, r * nx + k + nx);
    for (int i = 0; i < nx - 1; i++)
        for (int j = 0; j < ny - 1; j++)
            add_face(4, i + j * nx, i + 1 + j * nx, i + 1 + (j + 1) * nx, i + (j + 1) * nx,
                     j * (nx - 1) + i, nh + j * nx + i + 1, (j + 1) * (nx - 1) + i, nh + j * nx + i);
    build_adjacency();
}
