// mesh_scene.cpp: TU 63 "mesh", part 2 (0x47cae0..0x47e520): the scene-entity builder and light passes of the world
// object, the carried-item display meshes, the projectile update and its mesh helpers, and the animation-set blends of
// the character owner. See MIGRATION.md.
#include "./mesh_slot.h"
#include "./mesh_build.h"          // build_appearance (0x482240), setup_item_meshes (0x47cfe0)
#include "../../include/fx.h"
#include "../../include/phys_set.h"
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../../include/game_data.h"   // GameData, g_game (0x5e2930)
#include "../trigger_vecs.h"   // SimpleTriggerVec: the simple-trigger list
#include "../blow.h"                  // Blow: init 0x4353a0
#include "../../include/sound_mgr.h"   // SoundMgr, g_sound_mgr 0x5aa128 (play 0x442650)
#include "../../include/item_kind.h"   // g_item_kind_count 0x5e29d4
#include "../float_consts.h"   // shared float constants kept in memory (0x549cdc ...)
#include "../../include/render_flags.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through troop_mgr.h) declares a global ::vat with the same signature
#define vat rgl::vat

// The callees below are members of the classes that define them (the resource manager's `resmgr` is in mesh_slot.h). The header of game/agent/agent.h
// brings another layout than this TU's, so only the member is declared, under the defining class.
namespace ms {
}
struct mesh_node {
    void set_time(const float* t);   // 0x433190 (rgl/xmlcfg/game_util.cpp: sets the morph time)
};
extern "C" int __stdcall FSOUND_StopSound(int channel);   // 0x5342a6
float frand();                                   // 0x40e400

// constants of the projectile code
extern float g_f54a6f0;        // 0x54a6f0 (0.01f)
extern float g_f54b3b4;        // 0x54b3b4 (0.8f: reach without an item)
extern float g_f54b708;        // 0x54b708 (per-tick gravity coefficient)
extern float g_f590fa4;        // 0x590fa4 (1.7f: first-sample distance)
extern float g_f591334;        // 0x591334 (20.0f)
extern const float g_f590c74;        // 0x590c74 (0.08f)
extern const float g_f59190c;        // 0x59190c (0.92f)
extern float g_f591908;        // 0x591908 (0.0027f)
extern float g_f54b660;        // 0x54b660 (0.0001f)
extern float g_f54b8b0;        // 0x54b8b0 (1.5f)
extern float g_f54a6d0;        // 0x54a6d0 (1e-5f)

// ---- 0x47cae0 ----------------------------------------------------------------------------------------------------

// @ 0x47cae0 -- build the 3x3 scene entities of the world object from the meshes named like the scene and light them
void Terrain::build_entity()
{
    if (tail.len < 4)
        return;
    meshes = g_resmgr.find_meshes(&tail);

    {
    vector4 center;
    center.x = wx - 10.0f;
    center.y = wy - 10.0f;
    float z = wx;
    z += wy;
    center.z = z * 0.5f;
    vector4 tv(5.0f, 5.0f, 0.0f);

    entity** e = sent;
    int i = 3;
    do {
        int j = 3;
        do {
            *e = world->create_entity(9);
            (*e)->box.reset();
            e++;
        } while (--j);
    } while (--i);

    for (unsigned n = 0; n < meshes.size(); n++) {
        mesh* m = ((rgl::mesh*)meshes[n])->clone();
        ((rgl::mesh*)m)->set_color(center);
        ((mesh*)m)->translate(&tv);
        ((Terrain*)this)->snap(m);
        ((rgl::mesh*)m)->compute_bounds();

        int ix;
        if (m->bounds.center.x < 0.0f)
            ix = 0;
        else if (m->bounds.center.x > wx)
            ix = 2;
        else
            ix = 1;
        int iy;
        if (m->bounds.center.y < 0.0f)
            iy = 0;
        else if (m->bounds.center.y > wy)
            iy = 2;
        else
            iy = 1;
        m->sort_order = 10;
        entity** ep = &sent[ix * 3 + iy];
        entity* e0 = *ep;
        mesh* mm = m;
        e0->meshes.push_back(mm);
        e0->recalc();

        if (g_use_shaders == 0)
            world->light_mesh(m, &(*ep)->local, 1);
        else if ((m->res->flags & 0x40) != 0)
            world->light_mesh(m, &(*ep)->local, 1);

        (*ep)->box.extend(&m->bounds.min);
        (*ep)->box.extend(&m->bounds.max);
    }

    entity** q = sent;
    int k = 3;
    do {
        int j = 3;
        do {
            (*q)->box.update();
            (*q)->center = (*q)->box.center;
            (*q)->set_fbc(&(*q)->box.radius);
            q++;
        } while (--j);
    } while (--k);
    }
}

// ---- 0x47ce00 / 0x47ced0 -------------------------------------------------------------------------------------------

// @ 0x47ce00
// Light every mesh of `list` whose material wants the per-mesh pass, with an identity frame.
void Mission::light_entities(mesh** list, int count)
{
    for (int i = 0; i < count; i++) {
        mesh* e = list[i];
        skin* r = e->res;
        bool k = ((r->shader->flags20 >> 7) & 1) != 0;
        if (k && (r->flags & 0x700) != 0x300) {
            vector4 o(0.0f, 0.0f, 0.0f);
            mat3 ident;
            matrix frame;
            memcpy(&frame, identity3(&ident), 0x30);
            frame.o = o;
            ((rgl::world*)scene)->light_mesh(e, &frame, 1);
        }
    }
}

// @ 0x47ced0
// Light every mesh of the entity of every placed object whose record is not in state 4.
void Mission::light_props()
{
#define PENT(i) (props[i].ent)
    for (unsigned i = 0; i < props.size(); i++) {
        if (props[i].kind == 4 || props[i].ent == 0)
            continue;
        for (unsigned j = 0; j < PENT(i)->meshes.size(); j++) {
            skin* r = PENT(i)->meshes[j]->res;
            bool k = ((r->shader->flags20 >> 7) & 1) != 0;
            if (k && (r->flags & 0x700) != 0x300)
                ((rgl::world*)scene)->light_mesh(PENT(i)->meshes[j], &PENT(i)->local, 1);
        }
    }
#undef PENT
}

// ---- item mesh setup (0x47cfe0) ---------------------------------------------------------------------------------

// ItemRef::mesh_name (0x434b20) as the original inlines it in the ranged arm: in-range test first, the kind read
// through g_items[r.item] (the original also keeps the inline's item >= 0 test; ours folds it into the caller's)
static inline tstring ir_mesh_name(const ItemRef& r)
{
    if (r.item >= 0 && r.item < g_item_kind_count)
        return g_items[r.item].mesh_name_for((int)r.m4 >> 24);
    return tstring("Error: Invalid item");
}

// @ 0x47cfe0
// Build the carried-item display meshes of an agent body: one lod mesh per equipment slot, plus the hand-carry mesh for
// the active item; publishes the ids/sounds for the item drop code. cdecl; the 5th argument is unused.
void setup_item_meshes(rgl::strategic_entity* body, int flag, int troop, rgl::lod_mesh** meshes, int* unused, ItemRef* items, int id)
{
    if (flag != 0)
        return;
    int i = 0;
    ItemRef* it = items;
    rgl::lod_mesh** p = meshes + 4;
    do {
        ItemRef ir = *it;
        if (ir.item >= 0) {
            ItemKind* kind = g_items + ir.item;
            unsigned t = (unsigned)kind->type_flags & 0xff;
            if (t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11 ||
                t == 2 || t == 3 || t == 4 || t == 7) {
                // the name is a temporary of the full expression: the original destroys it (0x47d180) before add_group
                *p = g_resmgr.make_lod_mesh(ir_mesh_name(ir), false);
                body->fx->elems[19].add_group(*p);
                (*p)->mask = 0;
                SimpleTriggerVec* snd = &kind->sound;      // the item's sound holder (ItemKind+0x228)
                if (snd->count > 0) {
                    g_game->event_ctx.agent = id;
                    g_game->event_ctx.troop = troop;
                    g_game->event_ctx.slot = i;
                    g_game->event_ctx.key_a = *p;
                    g_game->event_ctx.mesh = 0;
                    g_script.param1 = id;
                    g_script.param2 = troop;
                    snd->update(-0x32);                    // 0x48add0
                }
            } else if (t == 5 || t == 6) {
                *p = g_resmgr.make_lod_mesh(ir.mesh_name(), false);
                if ((char)kind->type_flags == 6) {
                    matrix mtx;
                    identity_pos(&mtx);
                    mtx.o.x = -0.01f;
                    mtx.o.z = -0.03f;
                    float angle = -1.5707964f;
                    mtx.rotate_about_f(&angle);
                    (*p)->transform(mtx);
                }
                body->fx->elems[19].add_group(*p);
                (*p)->mask = 0;
            }
            if (((((unsigned*)&kind->flags)[0] & 0xf0000000) | (((unsigned*)&kind->flags)[1] & 7)) != 0 &&
                ((unsigned)kind->type_flags & 0xff) != 7 && ((unsigned)kind->type_flags & 0xff) != 9) {
                tstring name = ir.mesh_name(0x3000000000000000ull);
                if (name.len > 1) {
                    lod_mesh* m = g_resmgr.make_lod_mesh(name, false);
                    matrix carry;
                    int bone, other;
                    kind->get_carry_pos((pos2*)&carry, &bone, &other);
                    m->frame = carry;
                    p[8] = m;
                    body->fx->elems[bone].add_group(m);
                }
            }
        }
        i++;
        it++;
        p++;
    } while (i < 4);
}

#include "../missile.h"   // Missile: settle 0x472650
// ---- the projectile (0x47d320, 0x47d3e0): Missile's update (../missile.h) ------------------------------------------

// @ 0x47d320 -- replace the first mesh of the entity by the stuck-weapon mesh, or drop all meshes when there is none
void Missile::update_ranged_mesh()
{
    delete ent->meshes.begin()[0];
    mesh* m = make_ranged_mesh();
    if (m != 0) {
        ent->meshes.begin()[0] = m;
        ent->center = vector4(0.0f, -ent->get_radius(), 0.0f);
        ent->visible = 1;
    } else {
        std::vector<mesh*>& v = ent->meshes;
        if (v._Myfirst != 0)
            operator delete(v._Myfirst);
        v._Myfirst = 0;
        v._Mylast = 0;
        v._Myend = 0;
    }
}


// @ 0x47d3e0
// One integration step of a flying projectile: keep the previous / frame-start position samples, advance the position,
// test the swept capsule against the scene, run the owner's swing trace on a hit, then update the owning mesh's frame and
// apply the drag / tumble.
void Missile::update(float dt)
{
    start = pos;
    if (state != 0)
        return;

    vector4 v = vel * speed + vector4(dt * 0.0f, dt * 0.0f, dt * (-9.8f));
    {
        vector4 dp = v * dt;
        pos.x += dp.x;
        pos.y += dp.y;
        pos.z += dp.z;
    }

    if (ent->visible == 0) {
        if ((pos - prev).length() > 1.7f)
            ent->visible = 1;
    }

    float s;
    if (ammo.item >= 0)
        s = (float)g_items[ammo.item].weapon_length * 0.01f;
    else
        s = 0.8f;

    bool qhit;
    {
        vector4 d = vel * s;
        qhit = settle(&capsule(0.01f, &(d + start), &(d + pos)));
    }
    if (!qhit) {
        Blow blow;
        blow.init();                                   // 0x4353a0
        int r = g_mission->agents[shooter].swing_trace(0, this, &blow, &this->hit);
        if (r >= 3) {
            pos = ent->local.o;
            qhit = true;
        } else if (r != 0) {
            g_mission->scene->remove_entity(ent->id);
            ent = 0;
            state = 2;
            clk.start = g_ticks[clk.clock];
        } else {
            if (mode == 1) {
                vector4 pa = v * g_k0_23 + pos;
                vector4 d = g_mission->view.o - pa;
                if (d.x * vel.x + (d.z * vel.z + d.y * vel.y) < 0.0f
                    && g_mission->view.o.z - g_one < pa.z) {
                    mode = 3;
                    if (d.out_length() < 20.0f) {
                        float sp = v.out_length();
                        int snd = 0x35;
                        int t = g_items[launcher.item].type_flags & 0xff;
                        if (t == 9) snd = 0x30;
                        else if (t == 8) snd = 0x2f;
                        else {
                            __int64 sub = g_items[launcher.item].flags & 0xff000;
                            if (sub == 0x10000) snd = 0x32;
                            else if (sub == 0x30000) snd = 0x33;
                            else if (sub == 0x40000) snd = 0x31;
                            else if (sub == 0x20000) snd = 0x34;
                        }
                        sound = g_sound_mgr.play(g_sound_mgr.slot[snd], 0, &pa.x, sp * 0.01f + frand() * g_f590c74 + g_f59190c, 0);
                        sound_t = (float)g_ticks[0] * 1e-5f;
                    }
                }
            }
            // the r == 0 path rejoins after the launcher.item-sound block
            goto after_item_sound;
        }
    }

    {
        int t = launcher.item;
        if (t >= 0) {
            if ((char)g_items[t].type_flags != 0x0a)
                t = ammo.item;
            if (t >= 0)
                g_items[t].sound.update(-0x34);      // 0x48add0
        }
    }

    if (qhit) {
        clk.start = g_ticks[clk.clock];
        __int64 sub = g_items[launcher.item].flags & 0xff000;
        if (sub == 0x10000 || sub == 0x80000 || sub == 0x70000) {
            state = 3;
            g_mission->scene->remove_entity(ent->id);
            ent = 0;
            clk.start = g_ticks[clk.clock];
        } else {
            state = 1;
            update_ranged_mesh();
        }

        if ((g_mission->view.o - pos).normalize() < 40.0f) {
            int m = mode;
            int snd = -1;
            if (m == 1 || m == 3 || (m == 0 && shooter == (unsigned)g_mission->player_id)) {
                ItemKind* k = &g_items[launcher.item];
                int type = k->type_flags & 0xff;
                if (type == 9) snd = (m == 1) ? 0x37 : 0x3e;
                else if (type == 8) snd = (m == 1) ? 0x36 : 0x3d;
                else {
                    __int64 sub2 = k->flags & 0xff000;
                    if (sub2 == 0x10000) snd = (m == 1) ? 0x39 : 0x40;
                    else if (sub2 == 0x30000) snd = (m == 1) ? 0x3a : 0x41;
                    else if (sub2 == 0x20000) snd = (m == 1) ? 0x3b : 0x42;
                    else if (sub2 == 0x40000) snd = (m == 1) ? 0x38 : 0x3f;
                    else snd = (m == 1) ? 0x3c : 0x43;
                }
            }
            if (sound >= 0 && (float)g_ticks[0] * 1e-5f - 0.6f < sound_t)
                FSOUND_StopSound(sound);
            if (snd >= 0)
                g_sound_mgr.play(g_sound_mgr.slot[snd], 0, &pos.x, 1.0f, 0);
        }
    }

after_item_sound:
    if (ent != 0) {
        speed = v.length();
        v.normalize();
        speed = speed - (speed * speed * dt) * 0.0027f;
        if (speed > 0.0001f) {
            vel = v;
            vel.normalize();
        }

        ent->local.f = vel;
        ent->local.u = vector4(0.0f, 0.0f, 1.0f);
        ((matrix&)ent->local).orthonormalize();

        if (state == 0) {
            ent->meshes[0]->local_frame = IDENTITY_VALUE();
            float a = 1.5707964f;
            ent->meshes[0]->local_frame.rotate_about_f(&a);
            if (tumble > 0.0f) {
                float b = 1.5f - clk.elapsed() * tumble;
                ent->meshes[0]->local_frame.rotate_about_u(&b);
            }
        }
        ent->local.o = pos;
    }
    f9c = 0;
}

// ---- 0x47dc50 ----------------------------------------------------------------------------------------------------

// The stack temporary of vector<PhysRec>::resize(0, PhysRec()) (0x47a980) zeroes f4, f8, f2c, f30 and the six vector lanes
// in place: PhysRec (include/phys_set.h) carries that constructor. This function is PhysSet::reset (the slices called the
// owner phys_owner).
// @ 0x47dc50
void PhysSet::reset()
{
    for (unsigned i = 0; i < recs.size(); i++)
        destroy_world(i);
    f10 = -1;
    f14 = 0;
    pending.body = 0;
    recs.resize(0, PhysRec());
    nworlds = 0;
}

// ---- 0x47dcd0 / 0x47dd90 -------------------------------------------------------------------------------------------

static __forceinline void vcopy(vector4* d, const vector4* s) { d->x = s->x; d->y = s->y; d->z = s->z; d->w = s->w; }

struct mesh_anim_x : rgl::mesh {
    void apply_anim(const float* t);             // 0x47dcd0
};

// @ 0x47dcd0
// Snapshot the animation time into the mesh, rebuild the morph targets, then fold every morph (v70, 0x20-byte) and vertex
// (v50, 0x60-byte) record back onto its source, and re-upload.
void mesh_anim_x::apply_anim(const float* t)
{
    ((mesh_node*)this)->set_time(t);              // 0x433190 (mesh_node::set_time, rgl/xmlcfg/game_util.cpp)
    update_morph();                               // 0x413320
    ensure_local();                               // 0x412ee0

    for (unsigned i = 0; i < points.size(); i++) {
        vat(points, i).a = vat(points, i).b;
    }
    for (unsigned j = 0; j < vertices.size(); j++) {
        vat(vertices, j).pos = vat(vertices, j).normal;
    }
    compute_bounds();                                // 0x419300
}

// @ 0x47dd90
void rgl::lod_mesh::apply_anim_all(const float* t)
{
    for (int i = 0; i < count; i++) {
        std::vector<mesh*>& v = lists[i];
        mesh** p = v._Myfirst;
        if (p < v._Mylast) {
            do {
                ((mesh_anim_x*)*p)->apply_anim(t);
                p++;
            } while (p < v._Mylast);
        }
    }
}

// ---- 0x47ddf0 / 0x47de60 -------------------------------------------------------------------------------------------

// Body the original's object saw (row owner mesh_ops.cpp 0x47c810): with it visible, cl knows blend_indexed only reads through its
// index parameter and keeps blend_all's loop counter in a register across the call (docs/codegen.md section 16).
inline __declspec(noinline) void Skin::blend_indexed(mesh* m, void* set, int* index, const float* weights)
{
    m->ensure_local();
    ArrSet* s = (ArrSet*)set;
    if ((int)m->points.size() != s->cnt[*index])
        return;
    for (unsigned i = 0; i < m->points.size(); i++) {
        for (int j = 0; j < n_face_keys; j++) {
            int bi = s->arr[*index][i];
            FaceKey* b = &face_keys[j];
            vector4* t = (vector4*)b->delta + bi;
            if (t->x * t->x + t->y * t->y + t->z * t->z > 1e-10f) {
                float w = (b->f28 - b->f24) * weights[j] + b->f24;
                vector4 d;
                point20* p = &m->points[i];
                d.x = w * t->x;
                d.y = w * t->y;
                d.z = w * t->z;
                if (p->a.x < 0.0f)
                    d.x = -d.x;
                vector4 v;
                v.x = p->a.x + d.x;
                v.y = p->a.y + d.y;
                v.z = p->a.z + d.z;
                p->a = v;
                p->b = v;
            }
        }
    }
}

// @ 0x47ddf0
void Skin::blend_all(lod_mesh* m, void* set, const float* w)
{
    int i = 0;
    if (0 < m->count) {
        do {
            if (m->lists[i].size() != 0)
                blend_indexed(m->lists[i][0], set, &i, w);
            i++;
        } while (i < m->count);
    }
}

// @ 0x47de60
void Skin::use_carry(lod_mesh* l, const float* w)
{
    std::vector<ArrSet>& v = head_maps;
    unsigned idx = (unsigned)-1;
    unsigned i = 0;
    do {
        if (v._Myfirst == 0 || i >= (unsigned)v.size()) {
            idx = v.size();
            head_maps.resize(head_maps.size() + 1);
            copy_troop(&vat(head_maps, idx), (const rgl::lod_mesh*)l);
            break;
        }
        if (head_maps[i].name.equals(l->name))
            idx = i;
        i++;
    } while (idx == (unsigned)-1);
    blend_all(l, &vat(head_maps, idx), w);
}

// ---- 0x47df80: the scatter-set generator ---------------------------------------------------------------------------

// @ 0x47df80
// Build one mesh per (map record, grid cell) from the scatter set `s` and attach them to the terrain's chunk entities.
// The original walks the records with a cursor at TerrainParticle::anim (+0x64) and addresses every field relative to it.
void Terrain::gen_set_init(particle_list* s)
{
    std::vector<mesh*> grid[25];
    int ri = 0;
    if (0 < s->count) {
        TerrainParticle* pr = s->items;
        do {
            ptype* ge = &g_ptypes[pr->type];
            pr->anim = pr->anim + ge->nframes * 100;
            pr->anim = pr->anim % ge->nframes;
            std::vector<mesh*> list;
            AnimBox* gr = &ge->boxes[pr->anim];
            if (gr->name.len > 0)
                list = gr->meshes;
            mat3 sm;
            identity3(&sm);
            sm.s.x = pr->dir.x;              // the record's scale
            sm.f.y = pr->dir.y;
            sm.u.z = pr->dir.z;
            matrix mat;
            *(mat3*)&mat = pr->frame.rot3(*(const matrix*)&sm);
            mat.o = pr->frame.o;
            int& mi = pr->nmesh;
            mi = 0;
            for (unsigned k = 0; k < list.size(); k++) {
                mesh* src = list[k];
                int cx = pr->cx / block;
                if (cx < 0)
                    cx = 0;
                else if (cx >= nx)
                    cx = nx - 1;
                int cy = pr->cy / block;
                int cyy = 0;
                if (cy >= 0) {
                    cyy = ny - 1;
                    if (cy < ny)
                        cyy = cy;
                }
                int cell = cyy + cx * 5;
                unsigned found = (unsigned)-1;
                unsigned j = 0;
                do {
                    if (j >= grid[cell].size())
                        goto make;
                    if (src->res == grid[cell][j]->res)
                        found = j;
                    j++;
                } while (found == (unsigned)-1);
                if (grid[cell][found]->vertices.size() > 32000)
                    goto make;
                goto use;
            make:
                {
                    mesh* nm = new mesh();
                    nm->res = src->res;
                    nm->mask = 0x1ffff;
                    nm->flags |= 0x10000000;
                    found = grid[cell].size();
                    grid[cell].push_back(nm);
                }
            use:
                if (mi < 4) {
                    pr->meshes[mi] = grid[cell][found];
                    pr->first[mi] = grid[cell][found]->vertices.size();
                }
                if ((ge->flags & 0x20000) == 0)
                    grid[cell][found]->append_skinned(src, (const rgl::matrix*)&mat, -1, -1, 0);
                else
                    grid[cell][found]->transform(src, &mat, -1);
                if (mi < 4) {
                    pr->last[mi] = grid[cell][found]->vertices.size();
                    mi = mi + 1;
                }
            }
            ri++;
            pr++;
        } while (ri < s->count);
    }
    for (int y = 0; y < ny; y++) {
        int x = 0;
        if (0 < nx) {
            int cell = y;
            do {
                for (unsigned j = 0; j < grid[cell].size(); j++) {
                    if (g_use_shaders == 0 && (grid[cell][j]->res->flags & 0x40) == 0) {
                        mat3 im;
                        vector4 o(0.0f, 0.0f, 0.0f);
                        matrix id;
                        *(mat3*)&id = *identity3(&im);
                        id.o = o;
                        world->light_mesh(grid[cell][j], &id, 1);
                    }
                    grid[cell][j]->compute_bounds();
                    rgl::entity* e = ents[cell];
                    std::vector<mesh*>& em = e->meshes;
                    em.push_back(grid[cell][j]);
                    e->recalc();
                }
                x++;
                cell += 5;
            } while (x < nx);
        }
    }
}
