// mesh_scene.cpp: TU 63 "mesh", part 2 (0x47cae0..0x47e520): the scene-entity builder and light passes of the world
// object, the carried-item display meshes, the projectile update and its mesh helpers, and the animation-set blends of
// the character owner. See MIGRATION.md.
#include "./mesh_slot.h"
#include "../../include/fx.h"
#include "../../include/phys_set.h"
#include "../../rgl/mesh_x.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through troop_mgr.h) declares a global ::vat with the same signature
#define vat rgl::vat

namespace ms {
// resource-manager members this TU calls (the shared classes of src/rgl/resource.h / core/resmgr.h are not co-includable)
struct resmgr_m : resource_manager {
    std::vector<mesh*> find_meshes(const tstring* prefix);          // 0x41b960 (hidden return pointer, ret 4)
    lod_mesh* make_lod_mesh(const tstring& pat, bool copy);          // 0x41ca60
};
// the item's sound sub-object at ItemKind+0x228 (party_window.cpp's item_sound): `count` entries, play() 0x48add0
struct ItemSnd { int count; void play(int id); };
// the shooter agent of a missile (Agent::swing_trace 0x45a170, ecx = agent)
struct AgentSwing { int swing_trace(int hand, void* missile, void* blow, float* scale); };
// three-row placeholder record initialised by 0x4353a0
struct Rows {
    int m0, m4;
    vector4 v8, v18, v28;
    int m38, m3c, m40, m44, m48, m4c, m50, m54;
    Rows* init();                                // 0x4353a0 (returns this)
};
// the scene query for a swept capsule (0x472650, ecx = the missile)
struct SceneQuery { int query(capsule* c); };
struct SoundPlayer {
    int play(int sound, int priority, const vector4* pos, float pitch, float* out);   // 0x442650 (ret 0x14)
};
}
extern ms::SoundPlayer g_sounds;                 // 0x5aa128
extern int g_sound_ids[];                        // 0x5aa178
extern "C" int __stdcall FSOUND_StopSound(int channel);   // 0x5342a6
float frand();                                   // 0x40e400
// 0x433190: rgl::mesh member of the animation code (sets the morph time)
struct mesh_anim { void set_time(const float* t); };

// constants of the projectile code
extern float g_f54b3b4;        // 0x54b3b4 (0.8f: reach without an item)
extern float g_f54b708;        // 0x54b708 (per-tick gravity coefficient)
extern float g_f54a6f0;        // 0x54a6f0 (0.01f)
extern float g_f590fa4;        // 0x590fa4 (1.7f: first-sample distance)
extern float g_f590e68;        // 0x590e68 (0.23f)
extern float g_f549cdc;        // 0x549cdc (1.0f)
extern float g_f591334;        // 0x591334 (20.0f)
extern float g_f590c74;        // 0x590c74 (0.08f)
extern float g_f59190c;        // 0x59190c (0.92f)
extern float g_f54a6d0;        // 0x54a6d0 (1e-5f)
extern float g_f590c90;        // 0x590c90 (40.0f)
extern float g_f590d94;        // 0x590d94 (0.6f)
extern float g_f591908;        // 0x591908 (0.0027f)
extern float g_f54b660;        // 0x54b660 (0.0001f)
extern float g_f54b8b0;        // 0x54b8b0 (1.5f)

// ---- 0x47cae0 ----------------------------------------------------------------------------------------------------

// @ 0x47cae0 -- build the 3x3 scene entities of the world object from the meshes named like the scene and light them
void TerrainX::build_entity()
{
    if (name.len < 4)
        return;
    smeshes = ((ms::resmgr_m*)&g_resmgr)->find_meshes(&name);

    vector4 center;
    center.x = wx - 10.0f;
    center.y = wy - 10.0f;
    center.z = (wx + wy) * 0.5f;
    vector4 tv(10.0f, 10.0f, 0.0f);

    entity** e = sent;
    int i = 3;
    do {
        int j = 3;
        do {
            *e = ((world_x*)world)->create_entity(9);
            (*e)->box.reset();
            e++;
        } while (--j);
    } while (--i);

    for (unsigned n = 0; n < smeshes.size(); n++) {
        mesh* m = ((mesh_x*)smeshes[n])->clone();
        ((mesh_x*)m)->set_color(center);
        ((mesh_x*)m)->translate(&tv);
        ((TerrainX*)this)->snap(m);
        ((mesh_x*)m)->finish_load();

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
        entity** ep = &sent[ix * 3 + iy];

        m->f214 = 10;
        entity* e0 = *ep;
        e0->meshes.push_back(m);
        e0->recalc();

        if (g_use_shaders == 0)
            ((world_x*)world)->light_mesh(m, &(*ep)->local, 1);
        else if ((((char*)m->res)[0x1c] & 0x40) != 0)
            ((world_x*)world)->light_mesh(m, &(*ep)->local, 1);

        (*ep)->box.extend(&m->bounds.min);
        (*ep)->box.extend(&m->bounds.max);
    }

    entity** q = sent;
    int k = 3;
    do {
        int j = 3;
        do {
            (*q)->box.update();
            (*q)->vc4 = (*q)->box.center;
            (*q)->set_fbc(&(*q)->box.radius);
            q++;
        } while (--j);
    } while (--k);
}

// ---- 0x47ce00 / 0x47ced0 -------------------------------------------------------------------------------------------

// @ 0x47ce00
// Light every mesh of `list` whose material wants the per-mesh pass, with an identity frame.
void TerrainX::light_entities(mesh** list, int count)
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
            ((world_x*)((Mission*)this)->scene_world)->light_mesh(e, &frame, 1);
        }
    }
}

// @ 0x47ced0
// Light every mesh of the entity of every placed object whose record is not in state 4.
struct PropRecV {                                // the fields of Rec1c8 (src/include/rec1c8.h) this pass reads, as a plain record
    char pad0[0x188];
    int destroyed;                               // 0x188
    char pad18c[0x1b4 - 0x18c];
    entity* ent;                                 // 0x1b4
    char pad1b8[0x1c8 - 0x1b8];
};
void TerrainX::light_props()
{
#define props (*(std::vector<PropRecV>*)&((Mission*)this)->props)
#define PENT(i) (props[i].ent)
    for (unsigned i = 0; i < props.size(); i++) {
        if (props[i].destroyed == 4 || props[i].ent == 0)
            continue;
        for (unsigned j = 0; j < PENT(i)->meshes.size(); j++) {
            skin* r = PENT(i)->meshes[j]->res;
            bool k = ((r->shader->flags20 >> 7) & 1) != 0;
            if (k && (r->flags & 0x700) != 0x300)
                ((world_x*)((Mission*)this)->scene_world)->light_mesh(PENT(i)->meshes[j], &PENT(i)->local, 1);
        }
    }
#undef PENT
#undef props
}

// ---- item mesh setup (0x47cfe0) ---------------------------------------------------------------------------------

static inline tstring item_mesh_name(ItemKind* kind, int item, unsigned m4)
{
    if (item < 0 || item >= g_item_kind_count)
        return tstring("Error: Invalid item");
    return kind->mesh_name_for((int)m4 >> 24);
}

// the lod mesh as this TU sees it: the frame at +0x8c and the visibility word at +0xcc (lod_mesh::m8c, mask)
// @ 0x47cfe0
// Build the carried-item display meshes of an agent body: one lod mesh per equipment slot, plus the hand-carry mesh for
// the active item; publishes the ids/sounds for the item drop code. cdecl; the 5th argument is unused.
struct SkelHolder { char pad0[4]; FxElem* elems; };
struct SkelBodyV { char pad0[0x304]; SkelHolder* skel; };
extern __int64 g_spawn_a;                        // 0xa70448
extern __int64 g_spawn_b;                        // 0xa70450
void setup_item_meshes(SkelBodyV* body, int flag, int troop, char* outbase, int unused, ItemRef* items, int id)
{
    if (flag != 0)
        return;
    int i = 0;
    ItemRef* it = items;
    int* p = (int*)(outbase + 0x10);
    do {
        ItemRef ir = *it;
        if (ir.item >= 0) {
            ItemKind* kind = g_items + ir.item;
            unsigned t = (unsigned)kind->type_flags & 0xff;
            if (t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11 ||
                t == 2 || t == 3 || t == 4 || t == 7) {
                {
                    tstring name = item_mesh_name(kind, ir.item, ir.m4);
                    *p = (int)((ms::resmgr_m*)&g_resmgr)->make_lod_mesh(name, false);
                }
                ((FxElem*)((char*)body->skel->elems + 0x2354))->add_group(*p);
                ((lod_mesh*)*p)->mask = 0;
                ms::ItemSnd* snd = (ms::ItemSnd*)((char*)kind + 0x228);
                if (snd->count > 0) {
                    *(int*)(g_game_base + 0x1a1f4) = id;
                    *(int*)(g_game_base + 0x1a1f8) = troop;
                    *(int*)(g_game_base + 0x1a204) = i;
                    *(int*)(g_game_base + 0x1a1fc) = *p;
                    *(int*)(g_game_base + 0x1a200) = 0;
                    g_spawn_a = id;
                    g_spawn_b = troop;
                    snd->play(-0x32);
                }
            } else if (t == 5 || t == 6) {
                *p = (int)((ms::resmgr_m*)&g_resmgr)->make_lod_mesh(ir.mesh_name(), false);
                if ((char)kind->type_flags == 6) {
                    matrix mtx;
                    identity_pos(&mtx);
                    mtx.o.x = -0.01f;
                    mtx.o.z = -0.03f;
                    float angle = -1.5707964f;
                    mtx.rotate_about_f(&angle);
                    ((lod_mesh_x*)*p)->transform(mtx);
                }
                ((FxElem*)((char*)body->skel->elems + 0x2354))->add_group(*p);
                ((lod_mesh*)*p)->mask = 0;
            }
            if ((((*(unsigned*)((char*)kind + 0x1d0) & 0xf0000000) | (*(unsigned*)((char*)kind + 0x1d4) & 7)) != 0) &&
                ((unsigned)kind->type_flags & 0xff) != 7 && ((unsigned)kind->type_flags & 0xff) != 9) {
                tstring name = ir.mesh_name(0x3000000000000000ull);
                if (name.len > 1) {
                    lod_mesh* m = ((ms::resmgr_m*)&g_resmgr)->make_lod_mesh(name, false);
                    matrix carry;
                    int bone, other;
                    kind->get_carry_pos((pos2*)&carry, &bone, &other);
                    m->m8c = carry;
                    p[8] = (int)m;
                    ((FxElem*)((char*)body->skel->elems + bone * 0x1dc))->add_group((int)m);
                }
            }
        }
        i++;
        it++;
        p++;
    } while (i < 4);
}

// ---- the projectile (0x47d320, 0x47d3e0) --------------------------------------------------------------------------
// The flying projectile record as its own update sees it. src/game/terrain_a/misc.h declares the same pool record
// (Missile) with only the members terrain_a reads and cannot be included next to game/mission.h (it declares g_mission
// with another type), so the full layout lives here under its own name. Offsets: 0x47d3e0 / 0x47d320.
struct MissileU {
    char pad0[8];
    int id;                                      // 0x08
    char padc[0x10 - 0x0c];
    vector4 prev;                                // 0x10 previous position sample
    vector4 pos;                                 // 0x20
    vector4 start;                               // 0x30 position at the start of the frame
    vector4 vel;                                 // 0x40 unit direction
    entity* ent;                                 // 0x50 the flying mesh entity (0 once landed or removed)
    char pad54[0x58 - 0x54];
    unsigned shooter;                            // 0x58
    float speed;                                 // 0x5c
    char pad60[0x68 - 0x60];
    int state;                                   // 0x68 0 flying, 1 stuck, 2 removed, 3 hit ground
    char pad6c[0x70 - 0x6c];
    Timer clk;                                   // 0x70 (clock index at +0x70, start tick at +0x78)
    int item;                                    // 0x80
    char pad84[0x88 - 0x84];
    int troop;                                   // 0x88
    char pad8c[0x94 - 0x8c];
    float hit;                                   // 0x94
    float tumble;                                // 0x98
    char f9c;                                    // 0x9c
    char pad9d[0xa0 - 0x9d];
    int mode;                                    // 0xa0
    int sound;                                   // 0xa4
    float sound_t;                               // 0xa8

    mesh* make_ranged_mesh();                    // 0x44f6a0
    void update(float dt);                       // 0x47d3e0
    void update_ranged_mesh();                   // 0x47d320
};

// @ 0x47d320 -- replace the first mesh of the entity by the stuck-weapon mesh, or drop all meshes when there is none
void MissileU::update_ranged_mesh()
{
    delete ent->meshes.begin()[0];
    mesh* m = make_ranged_mesh();
    if (m != 0) {
        ent->meshes.begin()[0] = m;
        ent->vc4 = vector4(0.0f, -ent->get_radius(), 0.0f);
        ent->f140 = 1;
    } else {
        std::vector<mesh*>& v = ent->meshes;
        if (v._Myfirst != 0)
            operator delete(v._Myfirst);
        v._Myfirst = 0;
        v._Mylast = 0;
        v._Myend = 0;
    }
}

static __forceinline vector4 vmul(const vector4& v, const float& k) { return vector4(v.x * k, v.y * k, v.z * k); }

// @ 0x47d3e0
// One integration step of a flying projectile: keep the previous / frame-start position samples, advance the position,
// test the swept capsule against the scene, run the owner's swing trace on a hit, then update the owning mesh's frame and
// apply the drag / tumble.
void MissileU::update(float dt)
{
    char qhit;
    float tmp;
    vector4 d;
    vector4 v;
    vector4 pa;
    vector4 pb;

    start = pos;
    if (state != 0)
        return;

    v.x = dt * 0.0f + speed * vel.x;
    v.y = dt * 0.0f + vel.y * speed;
    v.z = dt * g_f54b708 + vel.z * speed;
    pos.x = v.x * dt + pos.x;
    pos.y = v.y * dt + pos.y;
    pos.z = v.z * dt + pos.z;

    if (ent->f140 == 0) {
        float dx = pos.x - prev.x;
        float dy = pos.y - prev.y;
        float dz = pos.z - prev.z;
        if ((float)sqrt(dy * dy + (dz * dz + dx * dx)) > g_f590fa4)
            ent->f140 = 1;
    }

    if (troop < 0)
        d.z = g_f54b3b4;
    else
        d.z = (float)g_items[troop].weight * g_f54a6f0;
    pb.x = d.z * vel.x;
    d.y = d.z * vel.y;
    d.z = d.z * vel.z;
    pa.x = pb.x + pos.x;
    pa.y = d.y + pos.y;
    pa.z = d.z + pos.z;
    pb.x = pb.x + start.x;
    pb.y = d.y + start.y;
    pb.z = d.z + start.z;

    {
        capsule cap(0.01f, &pb, &pa);
        qhit = (char)((ms::SceneQuery*)this)->query(&cap);
    }
    if (qhit == 0) {
        ms::Rows rows;
        rows.init();
        // shooter agent: mission->pages[shooter>>4] + (shooter&0xf)*0x5928
        ms::AgentSwing* a = (ms::AgentSwing*)(*(char**)(*(char**)((char*)g_mission + 0x1c) + (shooter >> 4) * 4) + (shooter & 0xf) * 0x5928);
        int r = a->swing_trace(0, this, &rows, &this->hit);
        if (r >= 3) {
            pos = ent->local.o;
            qhit = 1;
        } else if (r != 0) {
            ((world_x*)g_mission->scene_world)->remove_entity(ent->id);
            ent = 0;
            state = 2;
            clk.start = g_ticks[clk.clock];
        } else {
            if (mode == 1) {
                pb.z = v.z * g_f590e68;
                pa.x = v.x * g_f590e68 + pos.x;
                pa.y = v.y * g_f590e68 + pos.y;
                pa.z = pb.z + pos.z;
                d.x = g_mission->view.o.x - pa.x;
                d.y = g_mission->view.o.y - pa.y;
                d.z = g_mission->view.o.z - pa.z;
                if (d.x * vel.x + (d.y * vel.y + d.z * vel.z) < 0.0f
                    && g_mission->view.o.z - g_f549cdc < pa.z) {
                    mode = 3;
                    if (d.out_length() < g_f591334) {
                        tmp = v.out_length();
                        int snd = 0x35;
                        int t = g_items[item].type_flags & 0xff;
                        if (t == 9) snd = 0x30;
                        else if (t == 8) snd = 0x2f;
                        else {
                            __int64 sub = g_items[item].flags & 0xff000;
                            if (sub == 0x10000) snd = 0x32;
                            else if (sub == 0x30000) snd = 0x33;
                            else if (sub == 0x40000) snd = 0x31;
                            else if (sub == 0x20000) snd = 0x34;
                        }
                        tmp = tmp * g_f54a6f0 + frand() * g_f590c74 + g_f59190c;
                        sound = g_sounds.play(g_sound_ids[snd], 0, &pa, tmp, 0);
                        sound_t = (float)g_ticks[0] * g_f54a6d0;
                    }
                }
            }
            // the r == 0 path rejoins after the item-sound block
            goto after_item_sound;
        }
    }

    {
        int t = item;
        if (t >= 0) {
            if ((char)g_items[t].type_flags != 0x0a)
                t = troop;
            if (t >= 0)
                ((ms::ItemSnd*)((char*)&g_items[t] + 0x228))->play(-0x34);
        }
    }

    if (qhit != 0) {
        clk.start = g_ticks[clk.clock];
        __int64 sub = g_items[item].flags & 0xff000;
        if (sub == 0x10000 || sub == 0x80000 || sub == 0x70000) {
            state = 3;
            ((world_x*)g_mission->scene_world)->remove_entity(ent->id);
            ent = 0;
            clk.start = g_ticks[clk.clock];
        } else {
            state = 1;
            update_ranged_mesh();
        }

        Mission* ms_ = g_mission;
        pb.x = ms_->view.o.x - pos.x;
        pb.y = ms_->view.o.y - pos.y;
        pb.z = ms_->view.o.z - pos.z;
        if (pb.normalize() < g_f590c90) {
            int m = mode;
            int snd = -1;
            if (m == 1 || m == 3 || (m == 0 && shooter == (unsigned)ms_->player_id)) {
                ItemKind* k = &g_items[item];
                int type = k->type_flags & 0xff;
                if (type == 9) snd = (-(unsigned)(m != 1) & 7) + 0x37;
                else if (type == 8) snd = (-(unsigned)(m != 1) & 7) + 0x36;
                else {
                    __int64 sub2 = k->flags & 0xff000;
                    if (sub2 == 0x10000) snd = (-(unsigned)(m != 1) & 7) + 0x39;
                    else if (sub2 == 0x30000) snd = (-(unsigned)(m != 1) & 7) + 0x3a;
                    else if (sub2 == 0x20000) snd = (-(unsigned)(m != 1) & 7) + 0x3b;
                    else if (sub2 == 0x40000) snd = (-(unsigned)(m != 1) & 7) + 0x38;
                    else snd = (-(unsigned)(m != 1) & 7) + 0x3c;
                }
            }
            if (sound >= 0 && (float)g_ticks[0] * g_f54a6d0 - g_f590d94 < sound_t)
                FSOUND_StopSound(sound);
            if (snd >= 0)
                g_sounds.play(g_sound_ids[snd], 0, &pos, 1.0f, 0);
        }
    }

after_item_sound:
    if (ent != 0) {
        speed = (float)sqrt(v.y * v.y + (v.z * v.z + v.x * v.x));
        v.normalize();
        speed = speed - (speed * speed * dt) * g_f591908;
        if (speed > g_f54b660) {
            vel = v;
            vel.normalize();
        }

        ent->local.f = vel;
        d.x = 0.0f;
        d.y = 0.0f;
        d.z = 1.0f;
        ent->local.u = d;
        ((matrix&)ent->local).orthonormalize();

        if (state == 0) {
            matrix idt;
            mesh* const& mr = ent->meshes[0];
            mr->m_cc = *identity_pos(&idt);
            tmp = 1.5707964f;
            ent->meshes[0]->m_cc.rotate_about_f(&tmp);
            if (tumble > 0.0f) {
                tmp = g_f54b8b0 - clk.elapsed() * tumble;
                ent->meshes[0]->m_cc.rotate_about_u(&tmp);
            }
        }
        ent->local.o = pos;
    }
    f9c = 0;
}

// ---- 0x47dc50 ----------------------------------------------------------------------------------------------------

// 0x38-byte physics record: the stack temporary of vector<MRow56>::resize(0, MRow56()) (0x47a980) zeroes f4, f8, f2c, f30
// and the six vector lanes in place. MRow56 (src/game/terrain_b/records.h) is the same record; PhysRec (include/phys_set.h)
// carries that constructor. This function is PhysSet::reset (the slices called the owner phys_owner).
// @ 0x47dc50
void PhysSet::reset()
{
    for (unsigned i = 0; i < recs.size(); i++)
        destroy_world(i);
    f10 = -1;
    f14 = 0;
    f18 = 0;
    recs.resize(0, PhysRec());
    nworlds = 0;
}

// ---- 0x47dcd0 / 0x47dd90 -------------------------------------------------------------------------------------------

static __forceinline void vcopy(vector4* d, const vector4* s) { d->x = s->x; d->y = s->y; d->z = s->z; d->w = s->w; }

struct mesh_anim_x : mesh_x {
    void apply_anim(const float* t);             // 0x47dcd0
};
struct lod_mesh_a : lod_mesh {
    void apply_anim_all(const float* t);         // 0x47dd90
};

// @ 0x47dcd0
// Snapshot the animation time into the mesh, rebuild the morph targets, then fold every morph (v70, 0x20-byte) and vertex
// (v50, 0x60-byte) record back onto its source, and re-upload.
void mesh_anim_x::apply_anim(const float* t)
{
    ((mesh_anim*)this)->set_time(t);              // 0x433190
    update_morph();                               // 0x413320
    ensure_local();                               // 0x412ee0

    for (unsigned i = 0; i < v70.size(); i++) {
        point20* p = &vat(v70, i);
        vcopy(&p->a, &p->b);
    }
    for (unsigned j = 0; j < v50.size(); j++) {
        vertex60* p = &vat(v50, j);
        vcopy(&p->pos, &p->normal);
    }
    finish_load();                                // 0x419300
}

// @ 0x47dd90
void lod_mesh_a::apply_anim_all(const float* t)
{
    for (int i = 0; i < count; i++) {
        std::vector<mesh*>& v = lists[i].v;
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

// @ 0x47ddf0
void EChar::blend_all(lod_mesh* m, void* set, const float* w)
{
    int i = 0;
    if (0 < m->count) {
        do {
            int j = i;
            if (m->lists[i].v._Myfirst != 0 && m->lists[i].v._Mylast - m->lists[i].v._Myfirst != 0)
                blend_indexed(*m->lists[i].v._Myfirst, set, &i, w);
            i = j + 1;
        } while (i < m->count);
    }
}

// std::vector push_back with the fast path guarded exactly as the original does.
template<class T> __forceinline void vpush_g(std::vector<T>& v, const T& x)
{
    if (v._Myfirst != 0 && v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}

// @ 0x47de60
void EChar::use_carry(lod_mesh* l, const float* w)
{
    std::vector<ArrSet>& v = arrsets;
    unsigned idx = (unsigned)-1;
    unsigned i = 0;
    unsigned off = 0;
    do {
        if (v._Myfirst == 0 || i >= (unsigned)v.size()) {
            idx = v.size();
            arrsets.resize(arrsets.size() + 1);
            ArrSet* e = &vat(arrsets, idx);
            copy_troop(e, (const MeshGroupRec*)l);
            break;
        }
        if (((ArrSet*)((char*)arrsets._Myfirst + off))->name.equals(*(tstring*)((char*)l + 0xd0)))
            idx = i;
        i++;
        off += 0x38;
    } while (idx == (unsigned)-1);
    ArrSet* e2 = &vat(arrsets, idx);
    blend_all(l, e2, w);
}

// ---- 0x47df80: the scatter-set generator ---------------------------------------------------------------------------
struct GmRec {                        // 0x48 bytes: a per-map record list (the AnimBox of terrain_a/terrain.h, seen by its mesh list)
    char pad0[0xc];
    int n;                            // 0x0c
    std::vector<mesh*> items;         // 0x10
    char pad1[0x48 - 0x20];
};

// @ 0x47df80
// Build one mesh per (map record, grid cell) from the scatter set `s` and attach them to the terrain's chunk entities.
// The record cursor `pr` points at TerrainParticle::anim (+0x64) of the current record; the original addresses every
// field relative to it.
#define RV(T, off) (*(T*)(pr + (off)))
void TerrainX::gen_set_init(particle_list* s)
{
    std::vector<mesh*> grid[25];
    int ri = 0;
    if (0 < s->count) {
        char* pr = (char*)s + 0x68;
        do {
            ptype* ge = &g_ptypes[RV(int, -0x64)];
            RV(int, 0) = RV(int, 0) + ge->nframes * 100;
            std::vector<mesh*> list;
            RV(int, 0) = RV(int, 0) % ge->nframes;
            GmRec* gr = &((GmRec*)ge->boxes)[RV(int, 0)];
            if (gr->n > 0)
                list = gr->items;
            mat3 sm;
            identity3(&sm);
            sm.s.x = RV(float, -0x10);
            sm.f.y = RV(float, -0xc);
            sm.u.z = RV(float, -8);
            matrix mat;
            *(mat3*)&mat = ((pos*)(pr - 0x50))->rot3(*(const matrix*)&sm);
            mat.o = *(vector4*)(pr - 0x20);
            RV(int, 0x38) = 0;
            for (unsigned k = 0; k < list.size(); k++) {
                mesh* src = list[k];
                int cx = RV(int, -0x60) / block;
                if (cx < 0)
                    cx = 0;
                else if (cx >= nx)
                    cx = nx - 1;
                int cy = RV(int, -0x5c) / block;
                int cyy = 0;
                if (cy >= 0) {
                    cyy = ny - 1;
                    if (cy < ny)
                        cyy = cy;
                }
                int cell = cyy + cx * 5;
                unsigned found = (unsigned)-1;
                unsigned j = 0;
                bool make = false;
                do {
                    if (grid[cell]._Myfirst == 0 || j >= (unsigned)grid[cell].size()) {
                        make = true;
                        break;
                    }
                    if (src->res == grid[cell][j]->res)
                        found = j;
                    j++;
                } while (found == (unsigned)-1);
                if (!make) {
                    mesh* f = grid[cell][found];
                    if (f->v50._Myfirst != 0 && 32000 < (unsigned)f->v50.size())
                        make = true;
                }
                if (make) {
                    mesh* nm = new mesh();
                    nm->res = src->res;
                    nm->mask = 0x1ffff;
                    nm->flags |= 0x10000000;
                    found = grid[cell].size();
                    vpush_g(grid[cell], nm);
                }
                if (RV(int, 0x38) < 4) {
                    mesh* cur = grid[cell][found];
                    ((mesh**)(pr + 8))[RV(int, 0x38)] = cur;
                    ((int*)(pr + 0x18))[RV(int, 0x38)] = cur->v50._Myfirst ? (int)cur->v50.size() : 0;
                }
                mesh* cur2 = grid[cell][found];
                if ((ge->flags & 0x20000) == 0)
                    ((mesh_x*)cur2)->append_skinned(src, &mat, -1, -1, 0);
                else
                    ((mesh_x*)cur2)->transform(src, &mat, -1);
                if (RV(int, 0x38) < 4) {
                    ((int*)(pr + 0x28))[RV(int, 0x38)] = cur2->v50._Myfirst ? (int)cur2->v50.size() : 0;
                    RV(int, 0x38) = RV(int, 0x38) + 1;
                }
            }
            ri++;
            pr += 0xa0;
        } while (ri < s->count);
    }
    for (int y = 0; y < ny; y++) {
        int x = 0;
        if (0 < nx) {
            int cell = y;
            do {
                for (unsigned j = 0; j < grid[cell].size(); j++) {
                    if (g_use_shaders == 0 && (*(unsigned char*)((char*)grid[cell][j]->res + 0x1c) & 0x40) == 0) {
                        vector4 o(0.0f, 0.0f, 0.0f);
                        mat3 im;
                        matrix id;
                        *(mat3*)&id = *identity3(&im);
                        id.o = o;
                        ((world_x*)world)->light_mesh(grid[cell][j], &id, 1);
                    }
                    ((mesh_x*)grid[cell][j])->finish_load();
                    TEntity* e = ents[cell];
                    mesh* mm = grid[cell][j];
                    std::vector<mesh*>& em = *(std::vector<mesh*>*)((char*)e + 0x118);
                    vpush_g(em, mm);
                    ((entity*)e)->recalc();
                }
                x++;
                cell += 5;
            } while (x < nx);
        }
    }
}
