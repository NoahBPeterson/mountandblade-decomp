//-----------------------------------------------------------------------------
// File: phys_set.cpp  (0x441360..0x441ff0)
//
// Desc: the Newton physics set: destroy a world record, add a ragdoll bone, start a static tree-collision
//       build, add a capsule rigid body; and Agent::build_static_collision, which fills the tree collision from
//       the static shapes around the agent and adds a body per capsule. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include "./phys_set.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/mat4.h"
#include "../../include/quaternion.h"
#include "../../include/timer.h"
#include "../../include/vec.h"

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
    anchor = body->m.o;
    g_phys.begin_static_build(phys_idx);
    shape_base* hits[0x2000];
    int cnt;
    {
        vector4 bmin, bmax;
        bmin.x = body->m.o.x - 5.0f;
        bmin.y = body->m.o.y - 5.0f;
        bmin.z = body->m.o.z - 5.0f;
        bmax.x = body->m.o.x + 5.0f;
        bmax.y = body->m.o.y + 5.0f;
        bmax.z = body->m.o.z + 5.0f;
        g_mission->scene->query_box(&bmin.x, &bmax.x, 0x8106, hits, &cnt);
    }
    int i;
    for (i = 0; i < cnt; i++) {
        if (hits[i]->shape_type() == 2) {
            face_shape* f = (face_shape*)hits[i];
            int tri[12];
            tri[0] = *(int*)&f->v[0].x; tri[1] = *(int*)&f->v[0].y; tri[2] = *(int*)&f->v[0].z;
            tri[3] = *(int*)&f->v[1].x; tri[4] = *(int*)&f->v[1].y; tri[5] = *(int*)&f->v[1].z;
            tri[6] = *(int*)&f->v[2].x; tri[7] = *(int*)&f->v[2].y; tri[8] = *(int*)&f->v[2].z;
            tri[9] = *(int*)&f->v[3].x; tri[10] = *(int*)&f->v[3].y; tri[11] = *(int*)&f->v[3].z;
            NewtonTreeCollisionAddFace(g_phys.recs[phys_idx].collision, ((face_shape*)hits[i])->count, (float*)tri, 12, 0);
        }
    }
    g_phys.rebuild_static_body(phys_idx);
    for (i = 0; i < cnt; i++) {
        if (hits[i]->shape_type() == 1) {
            capsule_shape* c = (capsule_shape*)hits[i];
            vector4 dir;
            int radius = *(int*)&c->radius;
            dir.x = c->p1.x - c->p0.x; dir.y = c->p1.y - c->p0.y; dir.z = c->p1.z - c->p0.z;
            float len = vnorm(dir);
            int zero = 0;
            frame R;
            vector4 z0(0.0f, 0.0f, 0.0f);
            mat3 t;
            mat3* ip = identity3(&t);
            *(mat3*)&R = *ip;
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
            *(mat3*)&M = *ip2;
            M.o = zero2;
            g_phys.add_capsule_body(phys_idx, (const float*)&radius, &len, (const float*)&zero, &R, &M, true);
        }
    }
}

