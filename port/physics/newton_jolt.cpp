// port/physics/newton_jolt.cpp: Newton Game Dynamics 1.x (the 41 entry points src/ imports) over Jolt Physics (MIT).
// How src/ uses each call and what the real 1.53 does (traced from the original game and from controlled scenes on the
// real DLL: trace/RECORDING.md, NEWTON_USAGE.md "Traced behaviour"): the shim follows the traced behaviour; TUNING marks
// a choice the traces leave open. tests/trace_replay.cpp diffs this file against recorded Newton traces.
//  * one JPH::PhysicsSystem per NewtonWorld; gravity 0 (the game's force callback adds mass * -9.8 itself).
//  * matrices: Newton rows (s,f,u,o) <-> Jolt columns; the body origin (not the COM) is the Newton matrix origin.
//  * capsule: Newton's lies along local X, Jolt's along Y; wrapped in a RotatedTranslatedShape (offset matrix * Rz(-90)).
//    `height` = total length, cylinder half-length >= 0.05 (traced).
//  * collisions are refcounted (create = 1, each body holds one): the game uses a capsule after releasing it.
//  * inertia: CalculateInertialMatrix returns unit-mass moments in the body frame (traced); SetMassMatrix's values are
//    kept for GetMassMatrix and integrated with each moment >= mass / 100 (traced).
//  * bodies are Static (mass 0) until SetMassMatrix gives mass > 0; they are created with AllowDynamicOrKinematic.
//  * ragdoll: AddBone's matrix is parent-relative (traced); plain bodies + one SwingTwistConstraint per bone made in
//    SetLimits from the creation pose (bodies are teleported by the game after RagDollEnd). Swing half-angle =
//    (maxCone - minCone) / 2, twist +-maxTwist (traced); Newton's limits are soft, Jolt's hard (TUNING: 1 position
//    iteration). Parent/child/grandparent/sibling bones do not collide (GroupFilterTable); bones never sleep.
//  * force callback: called for each awake ragdoll bone before every step; AddForce/AddTorque accumulate.
//  * a step is min(dt, 1/60); damping is per step (v *= 1 - c/50), linear default 0.1045 (traced).
//  * GetSleepingState is 1 while awake (traced); Set* calls do not wake a sleeping body (traced).
//  * the world size box deactivates bodies that leave it.
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>

#include <algorithm>
#include <functional>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace JPH;

struct NewtonWorld;
struct NewtonBody;
struct NewtonCollision;
struct NewtonRagDoll;
struct NewtonRagDollBone;

namespace {

constexpr ObjectLayer kLayerStatic = 0, kLayerMoving = 1;
constexpr BroadPhaseLayer kBpStatic(0), kBpMoving(1);

class BPLayers final : public BroadPhaseLayerInterface {
public:
    uint GetNumBroadPhaseLayers() const override { return 2; }
    BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer l) const override { return l == kLayerStatic ? kBpStatic : kBpMoving; }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(BroadPhaseLayer l) const override { return l == kBpStatic ? "STATIC" : "MOVING"; }
#endif
};
class ObjVsBP final : public ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(ObjectLayer l, BroadPhaseLayer b) const override { return l == kLayerMoving || b == kBpMoving; }
};
class ObjPair final : public ObjectLayerPairFilter {
public:
    bool ShouldCollide(ObjectLayer a, ObjectLayer b) const override { return a == kLayerMoving || b == kLayerMoving; }
};

std::once_flag g_initOnce;
JobSystemThreadPool* g_jobs = nullptr;   // never freed: a static destructor joining threads at exit can hang

void initJolt()
{
    std::call_once(g_initOnce, [] {
        RegisterDefaultAllocator();
        Factory::sInstance = new Factory();
        RegisterTypes();
        unsigned hw = std::thread::hardware_concurrency();
        int threads = (int)std::max(1u, std::min(hw ? hw - 1 : 1u, 3u));
        g_jobs = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, threads);
    });
}

TempAllocator& tempAlloc()
{
    thread_local std::unique_ptr<TempAllocatorImpl> t(new TempAllocatorImpl(4 * 1024 * 1024));
    return *t;
}

// ---- Newton row-major matrix <-> Jolt ---------------------------------------------------------------------------
Vec3 v3(const float* p) { return Vec3(p[0], p[1], p[2]); }

Quat quatFromRows(const float* m)
{
    Vec3 s = v3(m), f = v3(m + 4);
    float ls = s.Length();
    s = ls > 1e-12f ? s / ls : Vec3::sAxisX();
    f = f - s * s.Dot(f);
    float lf = f.Length();
    f = lf > 1e-12f ? f / lf : s.GetNormalizedPerpendicular();
    Vec3 u = s.Cross(f);
    Mat44 r(Vec4(s, 0), Vec4(f, 0), Vec4(u, 0), Vec4(0, 0, 0, 1));
    return r.GetQuaternion().Normalized();
}

void rowsFromPose(Vec3Arg pos, QuatArg q, float* m)
{
    Mat44 r = Mat44::sRotation(q);
    Vec3 a[3] = {r.GetAxisX(), r.GetAxisY(), r.GetAxisZ()};
    for (int i = 0; i < 3; i++) {
        m[i * 4 + 0] = a[i].GetX(); m[i * 4 + 1] = a[i].GetY(); m[i * 4 + 2] = a[i].GetZ(); m[i * 4 + 3] = 0.0f;
    }
    m[12] = pos.GetX(); m[13] = pos.GetY(); m[14] = pos.GetZ(); m[15] = 1.0f;
}

void put3(float* o, Vec3Arg v) { if (o) { o[0] = v.GetX(); o[1] = v.GetY(); o[2] = v.GetZ(); } }

} // namespace

// ---- handles ---------------------------------------------------------------------------------------------------------
struct NewtonCollision {
    NewtonWorld* world = nullptr;
    int refs = 1;
    bool tree = false, built = false;
    Ref<Shape> shape;
    TriangleList tris;                // tree: accumulated until EndBuild
    float inertia[3] = {0, 0, 0};              // unit-mass principal moments (convex)
    float com[3] = {0, 0, 0};                  // centre of mass in the body frame (the offset matrix translation)
    float radius = 0, halfCyl = 0;             // capsule: radius, half the cylinder length
};

struct NewtonBody {
    NewtonWorld* world = nullptr;
    BodyID id;
    NewtonCollision* coll = nullptr;
    bool tree = false;
    float mass = 0, inertia[3] = {0, 0, 0};
    float com[3] = {0, 0, 0};
    float linDamp = 0.1045f, angDamp = 0.1f;   // Newton coefficients (1.53 defaults: GetLinearDamping reads 0.1045)
    Vec3 shapeCom = Vec3::sZero();             // COM the shape had before SetCentreOfMass
    NewtonRagDollBone* bone = nullptr;
};

struct NewtonRagDollBone {
    NewtonRagDoll* rd = nullptr;
    NewtonRagDollBone* parent = nullptr;
    NewtonBody* body = nullptr;
    int index = 0;
    float global0[16];                         // the creation pose: AddBone's (parent-relative) matrix * parent's
    Ref<Constraint> joint;
};

struct NewtonRagDoll {
    NewtonWorld* world = nullptr;
    void (*cb)(const NewtonBody*) = nullptr;
    Ref<GroupFilterTable> filter;
    std::vector<std::unique_ptr<NewtonRagDollBone>> bones;
    bool open = false;
};

struct NewtonWorld {
    BPLayers bpLayers;
    ObjVsBP objVsBp;
    ObjPair pairFilter;
    PhysicsSystem sys;
    std::vector<NewtonBody*> bodies;
    std::vector<NewtonCollision*> collisions;
    std::vector<std::unique_ptr<NewtonRagDoll>> ragdolls;
    float restitution = 0.0f, friction = 0.5f;
    bool hasBounds = false;
    Vec3 bmin = Vec3::sZero(), bmax = Vec3::sZero();
    uint32 groupCounter = 0;
    BodyInterface& bi() { return sys.GetBodyInterface(); }
};

namespace {

void releaseCollision(NewtonCollision* c)
{
    if (!c || --c->refs > 0)
        return;
    auto& v = c->world->collisions;
    v.erase(std::remove(v.begin(), v.end(), c), v.end());
    delete c;
}

NewtonCollision* newCollision(NewtonWorld* w)
{
    auto* c = new NewtonCollision;
    c->world = w;
    w->collisions.push_back(c);
    return c;
}

void withBody(NewtonBody* b, const std::function<void(Body&)>& fn)
{
    BodyLockWrite lock(b->world->sys.GetBodyLockInterface(), b->id);
    if (lock.Succeeded())
        fn(lock.GetBody());
}

void destroyBodyImpl(NewtonWorld* w, NewtonBody* b)
{
    if (b->bone) {
        if (b->bone->joint) { w->sys.RemoveConstraint(b->bone->joint); b->bone->joint = nullptr; }
        b->bone->body = nullptr;
    }
    w->bi().RemoveBody(b->id);
    w->bi().DestroyBody(b->id);
    w->bodies.erase(std::remove(w->bodies.begin(), w->bodies.end(), b), w->bodies.end());
    releaseCollision(b->coll);
    delete b;
}

NewtonBody* createBodyImpl(NewtonWorld* w, NewtonCollision* c, const Mat44& unused, const CollisionGroup* group)
{
    (void)unused;
    if (!c || !c->shape)
        return nullptr;
    BodyCreationSettings s(c->shape, RVec3::sZero(), Quat::sIdentity(), EMotionType::Static,
                           c->tree ? kLayerStatic : kLayerMoving);
    s.mAllowDynamicOrKinematic = !c->tree;
    s.mFriction = w->friction;
    s.mRestitution = w->restitution;
    s.mLinearDamping = 0.0f;                   // set before every step from the Newton coefficients (NewtonUpdate)
    s.mAngularDamping = 0.0f;
    s.mGravityFactor = 0.0f;
    s.mMotionQuality = EMotionQuality::LinearCast;   // thin triangle soup under fast capsules
    if (group)
        s.mCollisionGroup = *group;
    Body* body = w->bi().CreateBody(s);
    if (!body)
        return nullptr;
    auto* b = new NewtonBody;
    b->world = w;
    b->id = body->GetID();
    b->coll = c;
    c->refs++;
    b->tree = c->tree;
    b->shapeCom = c->shape->GetCenterOfMass();
    b->com[0] = b->shapeCom.GetX(); b->com[1] = b->shapeCom.GetY(); b->com[2] = b->shapeCom.GetZ();
    w->bi().AddBody(b->id, c->tree ? EActivation::DontActivate : EActivation::Activate);
    w->bodies.push_back(b);
    return b;
}

void setMatrixImpl(NewtonBody* b, const float* m)
{
    // 1.53 moves a frozen body without waking it (traced: experiments E9)
    b->world->bi().SetPositionAndRotation(b->id, v3(m + 12), quatFromRows(m), EActivation::DontActivate);
}

} // namespace

extern "C" {

// ---- world --------------------------------------------------------------------------------------------------------
NewtonWorld* NewtonCreate(void*, void*)
{
    initJolt();
    auto* w = new NewtonWorld;
    w->sys.Init(512, 0, 1024, 2048, w->bpLayers, w->objVsBp, w->pairFilter);
    w->sys.SetGravity(Vec3::sZero());
    PhysicsSettings ps = w->sys.GetPhysicsSettings();
    ps.mPointVelocitySleepThreshold = 0.1f;
    ps.mTimeBeforeSleep = 0.1f;
    ps.mNumVelocitySteps = 10;                 // Jolt's defaults until NewtonSetSolverModel
    ps.mNumPositionSteps = 2;
    w->sys.SetPhysicsSettings(ps);
    return w;
}

void NewtonDestroy(const NewtonWorld* cw)
{
    auto* w = const_cast<NewtonWorld*>(cw);
    if (!w)
        return;
    for (auto& rd : w->ragdolls)
        for (auto& b : rd->bones)
            if (b->joint) { w->sys.RemoveConstraint(b->joint); b->joint = nullptr; }
    for (NewtonBody* b : w->bodies) {
        w->bi().RemoveBody(b->id);
        w->bi().DestroyBody(b->id);
        delete b;
    }
    w->bodies.clear();
    for (NewtonCollision* c : w->collisions)
        delete c;
    w->collisions.clear();
    delete w;
}

void NewtonUpdate(const NewtonWorld* cw, float timestep)
{
    auto* w = const_cast<NewtonWorld*>(cw);
    if (!w || timestep <= 0.0f)
        return;
    BodyInterface& bi = w->bi();
    // 1.53 clamps the step to 1/60 s (NewtonSetMinimumFrameRate's default 60; traced: a 0.018 or 0.036 s update
    // integrates gravity for exactly 1/60 s, a 0.009 s one for 0.009 s), so the game's 0.018..0.039 s substeps run
    // slower than real time, as they did on Windows.
    const float h = std::min(timestep, 1.0f / 60.0f);
    // damping is per step, not per second (traced: |w| *= 1 - c/50 and |v| *= 1 - 0.1045/50 per update whatever the
    // step length): Jolt applies v *= 1 - c' h, so c' = c / (50 h)
    for (NewtonBody* b : w->bodies) {
        if (b->tree || b->mass <= 0.0f || !bi.IsActive(b->id))
            continue;
        withBody(b, [&](Body& body) {
            if (MotionProperties* mp = body.GetMotionPropertiesUnchecked()) {
                mp->SetLinearDamping(b->linDamp / (50.0f * h));
                mp->SetAngularDamping(b->angDamp / (50.0f * h));
            }
        });
    }
    for (auto& rd : w->ragdolls) {
        if (!rd->cb)
            continue;
        for (auto& bone : rd->bones)
            if (bone->body && bi.IsActive(bone->body->id))
                rd->cb(bone->body);
    }
    w->sys.Update(h, 1, &tempAlloc(), g_jobs);
    if (w->hasBounds) {
        for (NewtonBody* b : w->bodies) {
            if (b->tree || !bi.IsActive(b->id))
                continue;
            Vec3 p = bi.GetCenterOfMassPosition(b->id);
            if (p.GetX() < w->bmin.GetX() || p.GetY() < w->bmin.GetY() || p.GetZ() < w->bmin.GetZ() ||
                p.GetX() > w->bmax.GetX() || p.GetY() > w->bmax.GetY() || p.GetZ() > w->bmax.GetZ()) {
                bi.SetLinearAndAngularVelocity(b->id, Vec3::sZero(), Vec3::sZero());
                bi.DeactivateBody(b->id);
            }
        }
    }
}

void NewtonSetSolverModel(const NewtonWorld* cw, int model)
{
    // 1.x: 0 = exact solver, n > 0 = n iterations (the game passes 6). Jolt: n velocity iterations and 1 position
    // iteration; against the traced battles that gives the smallest one-step error (--sync: rotation median 0.49 deg,
    // position 1.1 mm per step; 10/2 gave 0.73 deg / 1.5 mm, 30/10 1.5 deg / 2.6 mm: Newton's limits are soft, and
    // more position iterations snap the animated start pose onto Jolt's hard limits harder).
    auto* w = const_cast<NewtonWorld*>(cw);
    PhysicsSettings ps = w->sys.GetPhysicsSettings();
    ps.mNumVelocitySteps = model > 0 ? (uint)model : 10u;
    ps.mNumPositionSteps = model > 0 ? 1u : 2u;
    w->sys.SetPhysicsSettings(ps);
}

void NewtonSetWorldSize(const NewtonWorld* cw, const float* mn, const float* mx)
{
    auto* w = const_cast<NewtonWorld*>(cw);
    w->bmin = v3(mn); w->bmax = v3(mx); w->hasBounds = true;
}

// ---- materials (only the default group exists) --------------------------------------------------------------------
int NewtonMaterialGetDefaultGroupID(const NewtonWorld*) { return 0; }
void NewtonMaterialSetDefaultSoftness(const NewtonWorld*, int, int, float) {}   // no Jolt equivalent
void NewtonMaterialSetDefaultElasticity(const NewtonWorld* cw, int, int, float e)
{
    auto* w = const_cast<NewtonWorld*>(cw);
    w->restitution = e;
    for (NewtonBody* b : w->bodies) w->bi().SetRestitution(b->id, e);
}
void NewtonMaterialSetDefaultFriction(const NewtonWorld* cw, int, int, float staticFriction, float kineticFriction)
{
    // Jolt has one Coulomb coefficient. The static one (1.1): the traced ragdolls spend most of their life resting or
    // creeping, where 1.53 holds them with static friction; with the kinetic 0.9 replayed bodies kept sliding down
    // slopes Newton had stopped on (3 fewer of 48 attract-mode ragdolls off by > 0.5 m).
    (void)kineticFriction;
    auto* w = const_cast<NewtonWorld*>(cw);
    w->friction = staticFriction;
    for (NewtonBody* b : w->bodies) w->bi().SetFriction(b->id, staticFriction);
}

// ---- collision ----------------------------------------------------------------------------------------------------
NewtonCollision* NewtonCreateCapsule(const NewtonWorld* cw, float radius, float height, const float* offset)
{
    // Newton 1.53 (traced: the shape's AABB for 13 radius/height pairs): `height` is the total length, end cap to end
    // cap, and the cylinder part is never shorter than 0.1 (half 0.05), so a "capsule" shorter than 2r + 0.1 grows.
    auto* w = const_cast<NewtonWorld*>(cw);
    radius = std::max(radius, 1e-3f);
    float half = std::max(height * 0.5f - radius, 0.05f);
    Ref<Shape> inner = new CapsuleShape(half, radius);
    Vec3 pos = Vec3::sZero();
    Quat rot = Quat::sRotation(Vec3::sAxisZ(), -0.5f * JPH_PI);   // Jolt capsule axis Y -> Newton's X
    if (offset) {
        pos = v3(offset + 12);
        rot = quatFromRows(offset) * rot;
    }
    ShapeSettings::ShapeResult r = RotatedTranslatedShapeSettings(pos, rot, inner).Create();
    if (!r.IsValid())
        return nullptr;
    NewtonCollision* c = newCollision(w);
    c->shape = r.Get();
    c->radius = radius; c->halfCyl = half;
    // unit-mass inertia about the COM, as the diagonal of the tensor in the BODY frame (the offset rotation applied, not
    // principal moments): what 1.53 returns (traced). 1.53 integrates a polygonal capsule: its moments are 0.947..0.996
    // (median 0.963) of the exact solid's over 2424 traced values, so the exact ones are scaled by 0.963.
    {
        double r = radius, hh = 2.0 * half;
        double vc = JPH_PI * r * r * hh, vs = 4.0 / 3.0 * JPH_PI * r * r * r, mc = vc / (vc + vs), ms = vs / (vc + vs);
        double ax = mc * r * r / 2 + ms * 0.4 * r * r;
        double tr = mc * (hh * hh / 12 + r * r / 4) + ms * (0.4 * r * r + hh * hh / 4 + 3 * hh * r / 8);
        double I[3] = {ax, tr, tr};
        for (int j = 0; j < 3; j++) {
            double d = 0;
            for (int i = 0; i < 3; i++) {
                double a = offset ? offset[i * 4 + j] : (i == j ? 1.0 : 0.0);
                d += I[i] * a * a;
            }
            c->inertia[j] = (float)(d * 0.963);
        }
    }
    Vec3 com = c->shape->GetCenterOfMass();
    c->com[0] = com.GetX(); c->com[1] = com.GetY(); c->com[2] = com.GetZ();
    return c;
}

NewtonCollision* NewtonCreateTreeCollision(const NewtonWorld* cw, int)
{
    auto* w = const_cast<NewtonWorld*>(cw);
    NewtonCollision* c = newCollision(w);
    c->tree = true;
    return c;
}

void NewtonTreeCollisionBeginBuild(const NewtonCollision* c)
{
    auto* t = const_cast<NewtonCollision*>(c);
    t->tris.clear(); t->built = false; t->shape = nullptr;
}

void NewtonTreeCollisionAddFace(const NewtonCollision* c, int vertexCount, const float* vertexPtr, int strideInBytes,
                                int)
{
    auto* t = const_cast<NewtonCollision*>(c);
    if (vertexCount < 3 || !vertexPtr)
        return;
    auto vtx = [&](int i) {
        const float* p = (const float*)((const char*)vertexPtr + (size_t)i * strideInBytes);
        return Float3(p[0], p[1], p[2]);
    };
    for (int i = 1; i + 1 < vertexCount; i++) {   // fan: quads become two triangles
        Float3 a = vtx(0), b = vtx(i), d = vtx(i + 1);
        Vec3 n = (Vec3(b) - Vec3(a)).Cross(Vec3(d) - Vec3(a));
        if (n.LengthSq() > 1e-14f)
            t->tris.push_back(Triangle(a, b, d));
    }
}

void NewtonTreeCollisionEndBuild(const NewtonCollision* c, int)
{
    auto* t = const_cast<NewtonCollision*>(c);
    t->built = true;
    if (t->tris.empty())
        return;
    ShapeSettings::ShapeResult r = MeshShapeSettings(t->tris).Create();
    if (r.IsValid())
        t->shape = r.Get();
    t->tris.clear();
    t->tris.shrink_to_fit();
}

void NewtonReleaseCollision(const NewtonWorld*, const NewtonCollision* c) { releaseCollision(const_cast<NewtonCollision*>(c)); }

void NewtonConvexCollisionCalculateInertialMatrix(const NewtonCollision* c, float* inertia, float* origin)
{
    for (int i = 0; i < 3; i++) {
        if (inertia) inertia[i] = c ? c->inertia[i] : 0.0f;
        if (origin) origin[i] = c ? c->com[i] : 0.0f;
    }
}

// Newton 1.53 reports a convex shape's box in the shape's own frame (the offset matrix is not applied: traced capsules
// with rotated, translated offsets give a box centred on 0 with the long side on x). Used by the replay harness.
void NewtonCollisionCalculateAABB(const NewtonCollision* c, const float* matrix, float* p0, float* p1)
{
    (void)matrix;
    Vec3 lo = Vec3::sZero(), hi = Vec3::sZero();
    if (c && !c->tree) {
        lo = Vec3(-(c->halfCyl + c->radius), -c->radius, -c->radius);
        hi = -lo;
    } else if (c && c->shape) {
        AABox b = c->shape->GetLocalBounds();
        lo = b.mMin; hi = b.mMax;
    }
    put3(p0, lo); put3(p1, hi);
}

// ---- bodies -------------------------------------------------------------------------------------------------------
NewtonBody* NewtonCreateBody(const NewtonWorld* cw, const NewtonCollision* c)
{
    auto* w = const_cast<NewtonWorld*>(cw);
    NewtonBody* b = createBodyImpl(w, const_cast<NewtonCollision*>(c), Mat44::sIdentity(), nullptr);
    if (b && b->tree)
        w->sys.OptimizeBroadPhase();
    return b;
}

void NewtonDestroyBody(const NewtonWorld* cw, const NewtonBody* b)
{
    if (b) destroyBodyImpl(const_cast<NewtonWorld*>(cw), const_cast<NewtonBody*>(b));
}

void NewtonBodySetMatrix(const NewtonBody* b, const float* m) { if (b && m) setMatrixImpl(const_cast<NewtonBody*>(b), m); }

void NewtonBodyGetMatrix(const NewtonBody* cb, float* m)
{
    auto* b = const_cast<NewtonBody*>(cb);
    rowsFromPose(b->world->bi().GetPosition(b->id), b->world->bi().GetRotation(b->id), m);
}

void NewtonBodySetVelocity(const NewtonBody* cb, const float* v)
{
    auto* b = const_cast<NewtonBody*>(cb);
    // 1.53 does not wake a frozen body here (traced: experiments E9); Jolt keeps the value for when it wakes
    withBody(b, [&](Body& body) { if (body.IsDynamic()) body.SetLinearVelocity(v3(v)); });
}
void NewtonBodyGetVelocity(const NewtonBody* cb, float* v)
{
    auto* b = const_cast<NewtonBody*>(cb);
    put3(v, b->world->bi().GetLinearVelocity(b->id));
}
void NewtonBodySetOmega(const NewtonBody* cb, const float* o)
{
    auto* b = const_cast<NewtonBody*>(cb);
    withBody(b, [&](Body& body) { if (body.IsDynamic()) body.SetAngularVelocity(v3(o)); });
}
void NewtonBodyGetOmega(const NewtonBody* cb, float* o)
{
    auto* b = const_cast<NewtonBody*>(cb);
    put3(o, b->world->bi().GetAngularVelocity(b->id));
}

void NewtonBodyAddForce(const NewtonBody* cb, const float* f)
{
    auto* b = const_cast<NewtonBody*>(cb);
    b->world->bi().AddForce(b->id, v3(f), EActivation::DontActivate);
}
void NewtonBodyAddTorque(const NewtonBody* cb, const float* t)
{
    auto* b = const_cast<NewtonBody*>(cb);
    b->world->bi().AddTorque(b->id, v3(t), EActivation::DontActivate);
}

void NewtonBodySetMassMatrix(const NewtonBody* cb, float mass, float ixx, float iyy, float izz)
{
    auto* b = const_cast<NewtonBody*>(cb);
    if (b->tree)
        return;
    b->mass = mass; b->inertia[0] = ixx; b->inertia[1] = iyy; b->inertia[2] = izz;
    BodyInterface& bi = b->world->bi();
    if (mass <= 0.0f) {
        bi.SetMotionType(b->id, EMotionType::Static, EActivation::DontActivate);
        return;
    }
    bi.SetMotionType(b->id, EMotionType::Dynamic, EActivation::Activate);
    MassProperties mp;
    mp.mMass = mass;
    // 1.53 integrates with each principal moment at least mass / 100 (traced, experiments E8: a torque on a 10 kg body
    // with Izz 0.018 turns it as if Izz were 0.1; 100 kg -> 1.0; 1 kg with 0.018 keeps 0.018). GetMassMatrix still
    // returns what was set. The game's capsules pass unit-mass moments (0.002..0.03), so in practice every ragdoll bone
    // runs with I = mass / 100 on all three axes.
    const float floorI = mass * 0.01f;
    mp.mInertia = Mat44::sScale(Vec3(std::max(ixx, floorI), std::max(iyy, floorI), std::max(izz, floorI)));
    withBody(b, [&](Body& body) {
        if (body.GetMotionPropertiesUnchecked())
            body.GetMotionProperties()->SetMassProperties(EAllowedDOFs::All, mp);
    });
}

void NewtonBodyGetMassMatrix(const NewtonBody* cb, float* mass, float* ixx, float* iyy, float* izz)
{
    if (mass) *mass = cb->mass;
    if (ixx) *ixx = cb->inertia[0];
    if (iyy) *iyy = cb->inertia[1];
    if (izz) *izz = cb->inertia[2];
}

void NewtonBodySetCentreOfMass(const NewtonBody* cb, const float* com)
{
    auto* b = const_cast<NewtonBody*>(cb);
    if (b->tree || !b->coll || !b->coll->shape)
        return;
    Vec3 want = v3(com);
    Vec3 delta = want - b->shapeCom;
    b->com[0] = com[0]; b->com[1] = com[1]; b->com[2] = com[2];
    if (delta.LengthSq() < 1e-10f)
        return;
    Ref<Shape> s = OffsetCenterOfMassShapeSettings(delta, b->coll->shape).Create().Get();
    b->world->bi().SetShape(b->id, s, false, EActivation::DontActivate);
}

void NewtonBodySetMaterialGroupID(const NewtonBody*, int) {}   // only the default group exists

void NewtonBodySetAngularDamping(const NewtonBody* cb, const float* d)
{
    // Newton 1.53: per-step factor 1 - d/50 on omega (traced, experiments E1); applied in NewtonUpdate. One coefficient
    // for the three axes (the game always passes 0.7, 0.7, 0.7).
    auto* b = const_cast<NewtonBody*>(cb);
    b->angDamp = (d[0] + d[1] + d[2]) / 3.0f;
}

void NewtonBodySetFreezeTreshold(const NewtonBody* cb, float speed2, float omega2, int frames)
{
    // Jolt sleeps per world (a point-velocity threshold and a time). 1.53 froze a lone resting bone once |v| fell to
    // ~0.05 m/s with |w| ~0.5 rad/s (experiments E5), i.e. sqrt(speed2) / 10; in game the ragdolls never froze before
    // being retired (every bone reads awake in every traced step), so the threshold is kept strict.
    (void)omega2;
    auto* b = const_cast<NewtonBody*>(cb);
    PhysicsSettings ps = b->world->sys.GetPhysicsSettings();
    ps.mPointVelocitySleepThreshold = std::sqrt(std::max(speed2, 0.0f)) * 0.1f;
    ps.mTimeBeforeSleep = std::max(1.0f / 60.0f, frames / 60.0f);
    b->world->sys.SetPhysicsSettings(ps);
}

int NewtonBodyGetSleepingState(const NewtonBody* cb)
{
    // 1.53 returns 1 while the body is AWAKE and 0 once it sleeps (traced: every bone reads 1 while falling, 0 at rest).
    // PhysSet::none_sleeping (0x43a8a0) therefore means "every bone asleep": the ragdoll is retired after 1 s of that.
    auto* b = const_cast<NewtonBody*>(cb);
    return b->world->bi().IsActive(b->id) ? 1 : 0;
}

// ---- rag dolls ----------------------------------------------------------------------------------------------------
NewtonRagDoll* NewtonCreateRagDoll(const NewtonWorld* cw)
{
    auto* w = const_cast<NewtonWorld*>(cw);
    auto rd = std::make_unique<NewtonRagDoll>();
    rd->world = w;
    rd->filter = new GroupFilterTable(64);
    w->ragdolls.push_back(std::move(rd));
    return w->ragdolls.back().get();
}

void NewtonRagDollBegin(const NewtonRagDoll* r) { const_cast<NewtonRagDoll*>(r)->open = true; }
void NewtonRagDollEnd(const NewtonRagDoll* r) { const_cast<NewtonRagDoll*>(r)->open = false; }
void NewtonRagDollSetForceAndTorqueCallback(const NewtonRagDoll* r, void (*cb)(const NewtonBody*))
{
    const_cast<NewtonRagDoll*>(r)->cb = cb;
}

NewtonRagDollBone* NewtonRagDollAddBone(const NewtonRagDoll* cr, const NewtonRagDollBone* parent, void*, float mass,
                                        const float* matrix, const NewtonCollision* boneCollision, const float*)
{
    auto* rd = const_cast<NewtonRagDoll*>(cr);
    NewtonWorld* w = rd->world;
    auto bone = std::make_unique<NewtonRagDollBone>();
    bone->rd = rd;
    bone->parent = const_cast<NewtonRagDollBone*>(parent);
    bone->index = (int)rd->bones.size();
    uint32 sub = std::min<uint32>((uint32)bone->index, 63);
    // jointed neighbours (parent, grandparent, siblings) do not collide
    if (bone->parent) {
        NewtonRagDollBone* p = bone->parent;
        rd->filter->DisableCollision(sub, std::min<uint32>(p->index, 63));
        if (p->parent) rd->filter->DisableCollision(sub, std::min<uint32>(p->parent->index, 63));
        for (auto& o : rd->bones)
            if (o->parent == p) rd->filter->DisableCollision(sub, std::min<uint32>(o->index, 63));
    }
    CollisionGroup group(rd->filter, (CollisionGroup::GroupID)1, sub);
    NewtonBody* body = createBodyImpl(w, const_cast<NewtonCollision*>(boneCollision), Mat44::sIdentity(), &group);
    if (!body)
        return nullptr;
    body->bone = bone.get();
    bone->body = body;
    // ragdoll bones never sleep: in every traced battle step all of 675 bones read awake (1.53 froze a bone only in a
    // still, isolated test scene), and Jolt would put a slowly accelerating bone to sleep where Newton keeps it moving
    // (experiments E3/E6). The game retires a ragdoll by its own timers then.
    withBody(body, [](Body& bd) { bd.SetAllowSleeping(false); });
    // 1.53 takes the matrix relative to the parent bone's (creation) frame: global = matrix * parent (row vectors).
    // Traced: the bone's global matrix right after AddBone is exactly that product; the root's is the matrix itself.
    if (bone->parent) {
        const float* P = bone->parent->global0;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                bone->global0[i * 4 + j] = matrix[i * 4 + 0] * P[0 * 4 + j] + matrix[i * 4 + 1] * P[1 * 4 + j] +
                                           matrix[i * 4 + 2] * P[2 * 4 + j] + matrix[i * 4 + 3] * P[3 * 4 + j];
    } else {
        memcpy(bone->global0, matrix, sizeof bone->global0);
    }
    setMatrixImpl(body, bone->global0);
    // Newton's AddBone gives the bone its mass; the game then overrides mass/inertia/COM.
    NewtonConvexCollisionCalculateInertialMatrix(boneCollision, body->inertia, nullptr);
    NewtonBodySetMassMatrix(body, mass, body->inertia[0] * mass, body->inertia[1] * mass, body->inertia[2] * mass);
    rd->bones.push_back(std::move(bone));
    return rd->bones.back().get();
}

NewtonBody* NewtonRagDollBoneGetBody(const NewtonRagDollBone* b) { return b ? b->body : nullptr; }

void NewtonRagDollBoneGetGlobalMatrix(const NewtonRagDollBone* b, float* m)
{
    NewtonBodyGetMatrix(b->body, m);
}

void NewtonRagDollBoneSetLimits(const NewtonRagDollBone* cb, const float* coneDir, float minConeAngle,
                                float maxConeAngle, float maxTwistAngle, const float*, float, float)
{
    auto* bone = const_cast<NewtonRagDollBone*>(cb);
    if (!bone->parent || !bone->parent->body || !bone->body)
        return;                                 // the root bone is free
    NewtonWorld* w = bone->rd->world;
    if (bone->joint) { w->sys.RemoveConstraint(bone->joint); bone->joint = nullptr; }
    BodyInterface& bi = w->bi();
    // the joint pivot is the bone's origin; the twist axis is coneDir (world space at the creation pose)
    Vec3 pivot = bi.GetPosition(bone->body->id);
    Vec3 axis = coneDir ? v3(coneDir) : Vec3::sZero();
    if (axis.LengthSq() < 1e-10f) {
        float m[16]; NewtonBodyGetMatrix(bone->body, m);
        axis = v3(m);
    }
    axis = axis.Normalized();
    Vec3 plane = axis.GetNormalizedPerpendicular();
    SwingTwistConstraintSettings s;
    s.mSpace = EConstraintSpace::WorldSpace;
    s.mPosition1 = s.mPosition2 = pivot;
    s.mTwistAxis1 = s.mTwistAxis2 = axis;
    s.mPlaneAxis1 = s.mPlaneAxis2 = plane;
    // 1.53's swing limit is (maxCone - minCone) / 2 about coneDir (traced, experiments E6/E6c: max 30 -> 15, 70 -> 35,
    // 70 with min 20 -> 25, with min -20 -> 45, each plus a 1..2 deg soft overshoot); the game passes min = -0.01 deg,
    // so a bone swings half its xml bone_cone_max. The twist limit is +-maxTwist (E6: 20 -> 20, 45 -> 45). The
    // bilateral cone changes nothing (E6b: 0, 0.2, 1 rad give the same swing).
    float cone = std::min(std::max(0.5f * (maxConeAngle - minConeAngle), 0.01f), JPH_PI - 0.01f);
    float twist = std::min(std::max(maxTwistAngle, 0.01f), JPH_PI);
    s.mNormalHalfConeAngle = s.mPlaneHalfConeAngle = cone;
    s.mTwistMinAngle = -twist;
    s.mTwistMaxAngle = twist;
    s.mMaxFrictionTorque = 0.0f;
    const BodyLockInterfaceNoLock& li = w->sys.GetBodyLockInterfaceNoLock();
    Body* p = const_cast<Body*>(li.TryGetBody(bone->parent->body->id));
    Body* c = const_cast<Body*>(li.TryGetBody(bone->body->id));
    if (!p || !c)
        return;
    bone->joint = s.Create(*p, *c);
    w->sys.AddConstraint(bone->joint);
}

} // extern "C"
