//-----------------------------------------------------------------------------
// File: agent_static_collision.cpp  (0x441360..0x441ff0)
//
// Desc: Newton set: destroy world, add ragdoll bone, static tree-collision build, capsule body; Agent::build_static_collision
//       One part of what was game/agent4/agent_logic.cpp (the object's functions in address order; the cut points and the evidence for
//       them are in docs/source_tree.md section 3). The prelude and the helper definitions below are the
//       parts of the old file's prelude and earlier sections this part's code uses.
//-----------------------------------------------------------------------------

#define ANIM_SLOT_SET_OUT_OF_LINE       // 0x4378a0 is a real call here (the body is game/agent/agent_stats.cpp's), written out at two sites of on_wield_changed
#include <vector>
#include <deque>
#include "engine/math/rgl_math.h"
#include "engine/core/std_vec_out_of_line.h"
#include "game/agent/ai/agent_types.h"
#include "game/agent/combat/agent_types.h"
#include "game/agent/ai/agent_ai.h"
#include "game/agent/agent_logic.h"
#include "game/common/game_globals.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <algorithm>
#include <stdexcept>
#include "game/dialog/dialog_manager.h"      // g_script 0xa70440 (g_ev_a / g_ev_b live in it; listed directly: the resolution audit reads only the source's own includes)
#include "game/agent/agent_stats.h"
#include "game/agent/anim_slot.h"      // (named here too for the resolution audit)
#include "engine/fx/env_curve.h"      // Curve: the canonical class of the Skeleton record (Curve::recs +0x40 is Skeleton::chans)
#include "engine/math/mat4_include.h"
#include "engine/core/ptr_tag.h"
#include "engine/core/timer.h"
#include "engine/math/vec.h"
#include "engine/physics/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "engine/physics/phys_set.h"
#include "engine/physics/height_field.h"
#include "engine/scene/world.h"
#include "engine/physics/rigid_body_rgl_dynamic.h"      // rgl::dynamic_entity: a kind-7 prop's rigid body (spawn_prop 0x44c210)
#include "engine/render/light.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/mission/placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/resource/skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/data/item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/agent/agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/mission/mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/terrain/poly_mesh.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/skill_cap.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/terrain/scene_file.h"     // SceneFile: GameData::scenes' record (flags)
#include "game/data/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/party/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include <vector>
#include "game/data/str_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/config/options.h"   // g_options 0x5aa008
#include "game/data/item_kind_include.h"   // g_items 0x5e29d0
#include "game/input/slot.h"
#include "game/agent/combat/agent_combat.h"
#include "engine/core/tstring.h"
#include "engine/render/font.h"
#include "game/common/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/agent/physics/phys_set.h"
#include "engine/math/quaternion.h"
#include "game/data/env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "engine/resource/res_mgr.h"
#include "game/config/config_block.h"
#include "engine/fx/fx.h"
#include "game/data/emit.h"
#include "game/ui/gui_manager.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "engine/audio/sound_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/render/mesh.h"
#include "game/dialog/dialog_mgr.h"
#include <memory>
#include "game/agent/agent_queries.h"
#include "game/input/key_names.h"      // g_keyboard (0xa170c8): pick_sound_ids
#include "game/data/mode_rec.h"
#include "engine/render/mesh_parts.h"
#include "engine/render/mesh_prims.h"
#include "engine/core/hnode.h"
#include "game/agent/blow.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "game/mission/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)

// ---- definitions from the old file's prelude and earlier sections that this part's code uses (copies; each is a static
// or inline helper, a type or a macro: none of them emits code or data of its own)
// 0x40cd60 / 0x40cd00 (rgl::identity3 / rgl::identity_pos, engine/physics/collision_shapes.cpp owns the rows): the bodies visible, so a frame passed to
// identity_pos does not escape and its later stores are independent of pointer loads (EmitTable::setup 0x4424c0).
inline __declspec(noinline) rgl::mat3* __cdecl rgl::identity3(rgl::mat3* out)
{
    vector4 a(1.0f, 0.0f, 0.0f), b(0.0f, 1.0f, 0.0f), c(0.0f, 0.0f, 1.0f);
    out->s = a;
    out->f = b;
    out->u = c;
    return out;
}
// Bodies the original's object saw for the box query chain (0x42aa50 strat_entity, 0x426500 / 0x426690 / 0x426700
// world_collide): the shape array and the count passed to query_box do not escape
// Body the original's object saw (row owner engine/scene/frustum_grid.cpp 0x41ed00): with it visible lo/hi do not escape,
// so hi[1] is not reloaded after each cell walk and, with no callee-saved register left, stays in memory
inline __declspec(noinline) void rgl::world::to_cell(const float* p, int* out)
{
    for (int i = 0; i < 2; i++) {
        float f = (p[i] - lo[i]) / cell_size[i];
        if (f <= 0.0f)
            out[i] = 0;
        else if ((float)(n[i] - 1) <= f)
            out[i] = n[i] - 1;
        else
            out[i] = (int)f + 1;
    }
}
inline __declspec(noinline) int box_overlap3(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return 1;
    return 0;
}
inline __declspec(noinline) int box_overlap2(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y)
        return 1;
    return 0;
}
inline __declspec(noinline) void rgl::cell::collect(const vector4* bmin, const vector4* bmax, int* use3d, unsigned mask, shape_base** out, int* count)
{
    shape_base* buf[0x2000];
    int n = 0;
    unsigned i = 0;                 // lives in memory in the original (reloaded every iteration)
    int k = 0;
    while (i < a.size()) {
        shape_base* sh = a[i];
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        vector4 tv[2];                  // both query temporaries live in one block (min below max)
        const vector4* smax = sh->get_max(&tv[1]);
        const vector4* smin = sh->get_min(&tv[0]);
        char hit;
        if (*use3d == 0)
            hit = box_overlap2(bmin, bmax, smin, smax);
        else
            hit = box_overlap3(bmin, bmax, smin, smax);
        if (!hit) {
            i++;
            continue;
        }
        if (k < *count) {
            shape_base* o = out[k];
            shape_base* cur = a[i];
            if (cur >= o) {
                k++;
                if (cur == o) {
                    i++;
                    buf[n++] = cur;
                } else {
                    buf[n++] = o;
                }
                continue;
            }
        }
        shape_base* cur2 = a[i];
        i++;
        buf[n++] = cur2;
    }
    if (k < *count) {
        int m = *count - k;
        memcpy(&buf[n], &out[k], m * sizeof(shape_base*));
        n += m;
    }
    *count = n;
    for (int j = 0; j < *count; j++)
        out[j] = buf[j];
}
inline __declspec(noinline) void rgl::world::query_box(const vector4* bmin, const vector4* bmax, unsigned mask, shape_base** out, int* count)
{
    int lo[2], hi[2];
    to_cell(&bmin->x, lo);
    to_cell(&bmax->x, hi);
    *count = 0;
    int2 p;
    for (p.x = lo[0]; p.x <= hi[0]; p.x++) {
        if (lo[1] <= hi[1]) {
            int flat = 1;
            for (p.y = lo[1]; p.y <= hi[1]; p.y++)
                cells[p.x][p.y].collect(bmin, bmax, &flat, mask, out, count);
        }
    }
}

//=============================================================================
// 0x441360..0x441ff0, from phys_set.cpp
// Desc: the Newton physics set: destroy a world record, add a ragdoll bone, start a static tree-collision
//       build, add a capsule rigid body; and Agent::build_static_collision, which fills the tree collision from
//       the static shapes around the agent and adds a body per capsule. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//=============================================================================
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
PhysSet g_phys;                                             // 0xa181f8 the physics set (its constructor and destructor run as dynamic initialisers)

// @ 0x441360
void PhysSet::destroy_world(int idx)
{
    if (recs[idx].world != 0) {
        recs[idx].ragdoll = 0;
        recs[idx].collision = 0;
        recs[idx].body = 0;
        recs[idx].bones.resize(0, 0);
        recs[idx].bodies.resize(0, 0);
        NewtonDestroy(recs[idx].world);
        recs[idx].world = 0;
        nworlds--;
    }
}

// @ 0x4413e0
const NewtonRagDollBone* PhysSet::add_bone(int idx, const NewtonRagDollBone* parent, const float* mass, matrix m0,
                                           const float* radius, const float* height, matrix m1, const float* cone_dir,
                                           const float* lim_min, const float* lim_max, const float* lim_twist)
{
    PhysRec* r;
    m1.s.w = 0.0f; m1.f.w = 0.0f; m1.u.w = 0.0f; m1.o.w = 1.0f;
    m0.s.w = 0.0f; m0.f.w = 0.0f; m0.u.w = 0.0f; m0.o.w = 1.0f;
    r = &recs[idx];
    NewtonCollision* cap = NewtonCreateCapsule(r->world, *radius, *height, &m1.s.x);
    float v[4];
    v[0] = 0.0f; v[1] = 0.0f; v[2] = 0.0f;
    const NewtonRagDollBone* bone = NewtonRagDollAddBone(recs[idx].ragdoll, parent, 0, *mass, &m0.s.x, cap, v);
    NewtonReleaseCollision(recs[idx].world, cap);
    NewtonBody* body = NewtonRagDollBoneGetBody(bone);
    float inertia[4];
    float origin[4];
    NewtonConvexCollisionCalculateInertialMatrix(cap, inertia, origin);
    NewtonBodySetCentreOfMass(body, origin);
    NewtonBodySetMassMatrix(body, *mass, inertia[0], inertia[1], inertia[2]);
    NewtonBodySetMaterialGroupID(body, recs[idx].material);
    v[0] = 0.7f; v[1] = 0.7f; v[2] = 0.7f;
    NewtonBodySetAngularDamping(body, v);
    NewtonBodySetFreezeTreshold(body, 0.25f, 0.7f, 5);
    NewtonRagDollBoneSetLimits(bone, cone_dir, *lim_min * 0.017453292f, *lim_max * 0.017453292f, *lim_twist * 0.017453292f, cone_dir, 0.2f, 0.2f);
    recs[idx].bones.push_back(bone);
    return bone;
}

// @ 0x441610
void PhysSet::begin_static_build(int idx)
{
    if (recs[idx].collision != 0) {
        NewtonDestroyBody(recs[idx].world, recs[idx].body);
        recs[idx].collision = 0;
        recs[idx].body = 0;
        for (unsigned i = 0; i < recs[idx].bodies.size(); i++)
            NewtonDestroyBody(recs[idx].world, recs[idx].bodies[i]);
        recs[idx].bodies.resize(0, 0);
    }
    recs[idx].collision = NewtonCreateTreeCollision(recs[idx].world, 0);
    NewtonTreeCollisionBeginBuild(recs[idx].collision);
}

// @ 0x4416c0
NewtonBody* PhysSet::add_capsule_body(int idx, const float* radius, const float* height, const float* mass, matrix* m0, matrix* m1, bool list)
{
    m0->s.w = 0.0f; m0->f.w = 0.0f; m0->u.w = 0.0f; m0->o.w = 1.0f;
    m1->s.w = 0.0f; m1->f.w = 0.0f; m1->u.w = 0.0f; m1->o.w = 1.0f;
    NewtonCollision* cap = NewtonCreateCapsule(recs[idx].world, *radius, *height, &m1->s.x);
    NewtonBody* body = NewtonCreateBody(recs[idx].world, cap);
    NewtonReleaseCollision(recs[idx].world, cap);
    NewtonBodySetMatrix(body, &m0->s.x);
    float inertia[4];
    float origin[4];
    NewtonConvexCollisionCalculateInertialMatrix(cap, inertia, origin);
    NewtonBodySetCentreOfMass(body, origin);
    NewtonBodySetMassMatrix(body, *mass, inertia[0], inertia[1], inertia[2]);
    NewtonBodySetMaterialGroupID(body, recs[idx].material);
    if (list)
        recs[idx].bodies.push_back(body);
    return body;
}

typedef matrix frame;   // a 4-row frame as the original builds it inline: three rows from identity3, origin zero

// the original inlines vector4::normalize (returns the old length) and the out-parameter cross product
static __forceinline float vnorm(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}
static __forceinline void vcross(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}

static __forceinline vector4 cross3(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
    return r;
}

// @ 0x4417d0
void Agent::build_static_collision()
{
    anchor = body->local.o;
    g_phys.begin_static_build(phys_idx);
    shape_base* hits[0x2000];
    int cnt;
    {
        vector4 bmin, bmax;
        bmin.x = body->local.o.x - 5.0f;
        bmin.y = body->local.o.y - 5.0f;
        bmin.z = body->local.o.z - 5.0f;
        bmax.x = body->local.o.x + 5.0f;
        bmax.y = body->local.o.y + 5.0f;
        bmax.z = body->local.o.z + 5.0f;
        g_mission->scene->query_box(&bmin, &bmax, 0x8106, hits, &cnt);   // scene is the scene object Ctx (engine/scene/strategic_entity.h)
    }
    int i;
    for (i = 0; i < cnt; i++) {
        if (hits[i]->shape_type() == 2) {
            face* f = (face*)hits[i];
            float tri[12];
            tri[0] = f->w[0].x; tri[1] = f->w[0].y; tri[2] = f->w[0].z;
            tri[3] = f->w[1].x; tri[4] = f->w[1].y; tri[5] = f->w[1].z;
            tri[6] = f->w[2].x; tri[7] = f->w[2].y; tri[8] = f->w[2].z;
            tri[9] = f->w[3].x; tri[10] = f->w[3].y; tri[11] = f->w[3].z;
            NewtonTreeCollisionAddFace(g_phys.recs[phys_idx].collision, ((face*)hits[i])->count, tri, 12, 0);
        }
    }
    g_phys.rebuild_static_body(phys_idx);
    for (i = 0; i < cnt; i++) {
        if (hits[i]->shape_type() == 1) {
            capsule* c = (capsule*)hits[i];
            vector4 dir;
            float radius = c->radius;
            dir.x = c->p1.x - c->p0.x; dir.y = c->p1.y - c->p0.y; dir.z = c->p1.z - c->p0.z;
            float len = vnorm(dir);
            float zero = 0.0f;
            frame R;
            vector4 z0(0.0f, 0.0f, 0.0f);
            mat3 t;
            mat3* ip = identity3(&t);
            static_cast<mat3&>(R) = *ip;
            R.s = dir;
            R.o = z0;
            vector4 c1 = cross3(R.s, R.f);
            if (c1.length() < 0.001f) {
                R.f = cross3(R.u, R.s);
                vnorm(R.f);
                R.u = cross3(R.s, R.f);
                vnorm(R.u);
            } else {
                R.u = c1;
                vnorm(R.u);
                R.f = cross3(R.u, R.s);
                vnorm(R.f);
            }
            vector4 zero2(0.0f, 0.0f, 0.0f);
            vector4 mid(c->p0.x + c->p1.x, c->p0.y + c->p1.y, c->p0.z + c->p1.z);
            mid.x *= 0.5f; mid.y *= 0.5f; mid.z *= 0.5f;
            R.o = mid;
            frame M;
            mat3 t2;
            mat3* ip2 = identity3(&t2);
            static_cast<mat3&>(M) = *ip2;
            M.o = zero2;
            g_phys.add_capsule_body(phys_idx, &radius, &len, &zero, &R, &M, true);
        }
    }
}
