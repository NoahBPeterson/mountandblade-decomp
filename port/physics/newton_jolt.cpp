// port/physics/newton_jolt.cpp: Newton Game Dynamics 1.x (the 41 entry points src/ imports) over Jolt Physics (MIT).
// How src/ uses each call and the semantics preserved: NEWTON_USAGE.md. Decisions made here (and the places where
// Jolt cannot be Newton, marked TUNING = a guess to settle by playtest, or UNVERIFIED = a 1.53 behaviour not traced):
//  * one JPH::PhysicsSystem per NewtonWorld; gravity 0 (the game's force callback adds mass * -9.8 itself).
//  * matrices: Newton rows (s,f,u,o) <-> Jolt columns; the body origin (not the COM) is the Newton matrix origin.
//  * capsule: Newton's lies along local X, Jolt's along Y; wrapped in a RotatedTranslatedShape (offset matrix * Rz(-90)).
//    `height` = distance between the end-sphere centres (UNVERIFIED against 1.53).
//  * collisions are refcounted (create = 1, each body holds one): the game uses a capsule after releasing it.
//  * inertia: NewtonBodySetMassMatrix's Ixx/Iyy/Izz are installed as-is as the diagonal inertia in the body frame
//    (Jolt: MassProperties::mInertia, about the COM). CalculateInertialMatrix returns the capsule's unit-mass principal
//    moments {axial, transverse, transverse} (UNVERIFIED scale); the game feeds them in without multiplying by mass.
//  * bodies are Static (mass 0) until SetMassMatrix gives mass > 0; they are created with AllowDynamicOrKinematic.
//  * ragdoll: plain bodies + one SwingTwistConstraint per bone (frames fixed at the creation pose: the constraint is
//    made in SetLimits from the bodies' creation matrices, bodies are teleported by the game after RagDollEnd).
//    Cone half-angle = max(maxCone, |minCone|); the 1.x minimum-cone dead zone and bilateral cone are not mapped.
//    Parent/child/grandparent/sibling bones do not collide (GroupFilterTable); bones collide with the tree and capsules.
//  * force callback: called for each awake ragdoll bone before every step; AddForce/AddTorque accumulate (Jolt clears
//    them after the step).
//  * freeze threshold -> the world's sleep velocity / time (TUNING); angular damping -> Jolt dw/dt = -c w (TUNING).
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
    Vec3 shapeCom = Vec3::sZero();             // COM the shape had before SetCentreOfMass
    NewtonRagDollBone* bone = nullptr;
};

struct NewtonRagDollBone {
    NewtonRagDoll* rd = nullptr;
    NewtonRagDollBone* parent = nullptr;
    NewtonBody* body = nullptr;
    int index = 0;
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
    s.mLinearDamping = 0.1f;                   // TUNING: Newton's default linear damping
    s.mAngularDamping = 0.1f;
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
    b->world->bi().SetPositionAndRotation(b->id, v3(m + 12), quatFromRows(m), EActivation::Activate);
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
    ps.mNumVelocitySteps = 30;                 // ragdolls with tiny inertia need a stiffer solver than Jolt's default
    ps.mNumPositionSteps = 10;
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
    for (auto& rd : w->ragdolls) {
        if (!rd->cb)
            continue;
        for (auto& bone : rd->bones)
            if (bone->body && bi.IsActive(bone->body->id))
                rd->cb(bone->body);
    }
    w->sys.Update(timestep, 2, &tempAlloc(), g_jobs);
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
    // Newton: iteration count of the solver (the game passes 6). Jolt's default (10 velocity / 2 position) is kept: fewer
    // iterations only make joints looser.
    (void)cw; (void)model;
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
    // Jolt has one Coulomb coefficient: take the kinetic one (the sliding behaviour); static is not modelled.
    (void)staticFriction;
    auto* w = const_cast<NewtonWorld*>(cw);
    w->friction = kineticFriction;
    for (NewtonBody* b : w->bodies) w->bi().SetFriction(b->id, kineticFriction);
}

// ---- collision ----------------------------------------------------------------------------------------------------
NewtonCollision* NewtonCreateCapsule(const NewtonWorld* cw, float radius, float height, const float* offset)
{
    auto* w = const_cast<NewtonWorld*>(cw);
    radius = std::max(radius, 1e-3f);
    float half = std::max(height * 0.5f, 1e-3f);
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
    MassProperties mp = inner->GetMassProperties();
    mp.ScaleToMass(1.0f);
    float trans = mp.mInertia(0, 0), axial = mp.mInertia(1, 1);
    c->inertia[0] = axial; c->inertia[1] = trans; c->inertia[2] = trans;
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
    BodyInterface& bi = b->world->bi();
    if (!bi.IsActive(b->id) && !b->tree && (v[0] != 0 || v[1] != 0 || v[2] != 0))
        bi.ActivateBody(b->id);
    bi.SetLinearVelocity(b->id, v3(v));
}
void NewtonBodyGetVelocity(const NewtonBody* cb, float* v)
{
    auto* b = const_cast<NewtonBody*>(cb);
    put3(v, b->world->bi().GetLinearVelocity(b->id));
}
void NewtonBodySetOmega(const NewtonBody* cb, const float* o)
{
    auto* b = const_cast<NewtonBody*>(cb);
    BodyInterface& bi = b->world->bi();
    if (!bi.IsActive(b->id) && !b->tree && (o[0] != 0 || o[1] != 0 || o[2] != 0))
        bi.ActivateBody(b->id);
    bi.SetAngularVelocity(b->id, v3(o));
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
    mp.mInertia = Mat44::sScale(Vec3(std::max(ixx, 1e-6f), std::max(iyy, 1e-6f), std::max(izz, 1e-6f)));
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
    // TUNING: Newton 1.x damping is a per-step coefficient in [0,1); Jolt's is dw/dt = -c w. A literal conversion of
    // 0.7 would stop all rotation, so take a gentle c proportional to the coefficient.
    auto* b = const_cast<NewtonBody*>(cb);
    float c = 3.0f * (d[0] + d[1] + d[2]) / 3.0f;
    withBody(b, [&](Body& body) { if (body.GetMotionPropertiesUnchecked()) body.GetMotionProperties()->SetAngularDamping(c); });
}

void NewtonBodySetFreezeTreshold(const NewtonBody* cb, float speed2, float omega2, int frames)
{
    // Jolt sleeps per world: velocity threshold and time. TUNING: a quarter of Newton's speed, frames * ~20 ms.
    (void)omega2;
    auto* b = const_cast<NewtonBody*>(cb);
    PhysicsSettings ps = b->world->sys.GetPhysicsSettings();
    ps.mPointVelocitySleepThreshold = std::sqrt(std::max(speed2, 0.0f)) * 0.25f;
    ps.mTimeBeforeSleep = std::max(0.05f, frames * 0.02f);
    b->world->sys.SetPhysicsSettings(ps);
}

int NewtonBodyGetSleepingState(const NewtonBody* cb)
{
    auto* b = const_cast<NewtonBody*>(cb);
    return b->world->bi().IsActive(b->id) ? 0 : 1;
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
    setMatrixImpl(body, matrix);
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
    float cone = std::min(std::max(std::max(maxConeAngle, std::fabs(minConeAngle)), 0.01f), JPH_PI - 0.01f);
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
