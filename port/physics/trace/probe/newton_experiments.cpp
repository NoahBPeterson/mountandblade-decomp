// newton_experiments: small controlled scenes run against a Newton 1.x implementation, printing what the engine does,
// to settle behaviours the game traces cannot isolate (damping laws, integration, sleeping, joint limits, clamps).
// Built twice from the same source: for Windows against the game's real Newton.dll (build_experiments.sh, run on the
// laptop) and natively against newton_jolt.cpp (target physics_newton_experiments); diff the two outputs.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

struct NewtonWorld; struct NewtonBody; struct NewtonCollision; struct NewtonRagDoll; struct NewtonRagDollBone;
extern "C" {
NewtonWorld* NewtonCreate(void*, void*);
void NewtonDestroy(const NewtonWorld*);
void NewtonUpdate(const NewtonWorld*, float);
void NewtonSetSolverModel(const NewtonWorld*, int);
void NewtonSetWorldSize(const NewtonWorld*, const float*, const float*);
int NewtonMaterialGetDefaultGroupID(const NewtonWorld*);
void NewtonMaterialSetDefaultSoftness(const NewtonWorld*, int, int, float);
void NewtonMaterialSetDefaultElasticity(const NewtonWorld*, int, int, float);
void NewtonMaterialSetDefaultFriction(const NewtonWorld*, int, int, float, float);
NewtonCollision* NewtonCreateCapsule(const NewtonWorld*, float, float, const float*);
NewtonCollision* NewtonCreateTreeCollision(const NewtonWorld*, int);
void NewtonTreeCollisionBeginBuild(const NewtonCollision*);
void NewtonTreeCollisionAddFace(const NewtonCollision*, int, const float*, int, int);
void NewtonTreeCollisionEndBuild(const NewtonCollision*, int);
void NewtonReleaseCollision(const NewtonWorld*, const NewtonCollision*);
void NewtonConvexCollisionCalculateInertialMatrix(const NewtonCollision*, float*, float*);
NewtonBody* NewtonCreateBody(const NewtonWorld*, const NewtonCollision*);
void NewtonBodySetMatrix(const NewtonBody*, const float*);
void NewtonBodyGetMatrix(const NewtonBody*, float*);
void NewtonBodySetVelocity(const NewtonBody*, const float*);
void NewtonBodyGetVelocity(const NewtonBody*, float*);
void NewtonBodySetOmega(const NewtonBody*, const float*);
void NewtonBodyGetOmega(const NewtonBody*, float*);
void NewtonBodyAddForce(const NewtonBody*, const float*);
void NewtonBodyAddTorque(const NewtonBody*, const float*);
void NewtonBodyGetMassMatrix(const NewtonBody*, float*, float*, float*, float*);
void NewtonBodySetMassMatrix(const NewtonBody*, float, float, float, float);
void NewtonBodySetCentreOfMass(const NewtonBody*, const float*);
void NewtonBodySetAngularDamping(const NewtonBody*, const float*);
void NewtonBodySetFreezeTreshold(const NewtonBody*, float, float, int);
int NewtonBodyGetSleepingState(const NewtonBody*);
NewtonRagDoll* NewtonCreateRagDoll(const NewtonWorld*);
void NewtonRagDollBegin(const NewtonRagDoll*);
void NewtonRagDollEnd(const NewtonRagDoll*);
void NewtonRagDollSetForceAndTorqueCallback(const NewtonRagDoll*, void (*)(const NewtonBody*));
NewtonRagDollBone* NewtonRagDollAddBone(const NewtonRagDoll*, const NewtonRagDollBone*, void*, float, const float*,
                                        const NewtonCollision*, const float*);
NewtonBody* NewtonRagDollBoneGetBody(const NewtonRagDollBone*);
void NewtonRagDollBoneSetLimits(const NewtonRagDollBone*, const float*, float, float, float, const float*, float, float);
}

static const float I16[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
static float g_force[3], g_torque[3];   // per-step force/torque the ragdoll callback applies (world)
static bool g_gravity = false;

static void cb(const NewtonBody* b)
{
    float m, x, y, z;
    NewtonBodyGetMassMatrix(b, &m, &x, &y, &z);
    float f[3] = {g_force[0] * m, g_force[1] * m, (g_force[2] - (g_gravity ? 9.8f : 0.0f)) * m};
    NewtonBodyAddForce(b, f);
    if (g_torque[0] != 0 || g_torque[1] != 0 || g_torque[2] != 0) NewtonBodyAddTorque(b, g_torque);
}

static NewtonWorld* world()
{
    NewtonWorld* w = NewtonCreate(0, 0);
    NewtonSetSolverModel(w, 6);
    float mn[3] = {-1000, -1000, -100}, mx[3] = {1000, 1000, 1000};
    NewtonSetWorldSize(w, mn, mx);
    int m = NewtonMaterialGetDefaultGroupID(w);
    NewtonMaterialSetDefaultSoftness(w, m, m, 0.1f);
    NewtonMaterialSetDefaultElasticity(w, m, m, 0.5f);
    NewtonMaterialSetDefaultFriction(w, m, m, 1.1f, 0.9f);
    return w;
}

// a one-bone ragdoll (the only kind of body that gets a force callback), capsule r 0.1 length 0.5 along x
static NewtonBody* single(NewtonWorld* w, float mass, float inertiaScale, float damp, float z = 50.0f)
{
    NewtonRagDoll* rd = NewtonCreateRagDoll(w);
    NewtonRagDollBegin(rd);
    float off[16]; memcpy(off, I16, sizeof off); off[12] = 0.25f;
    NewtonCollision* c = NewtonCreateCapsule(w, 0.1f, 0.5f, off);
    float m[16]; memcpy(m, I16, sizeof m); m[14] = z;
    float size[3] = {0, 0, 0};
    NewtonRagDollBone* bone = NewtonRagDollAddBone(rd, 0, 0, mass, m, c, size);
    NewtonReleaseCollision(w, c);
    NewtonBody* b = NewtonRagDollBoneGetBody(bone);
    float in[4], org[4];
    NewtonConvexCollisionCalculateInertialMatrix(c, in, org);
    NewtonBodySetCentreOfMass(b, org);
    NewtonBodySetMassMatrix(b, mass, in[0] * inertiaScale, in[1] * inertiaScale, in[2] * inertiaScale);
    float d[3] = {damp, damp, damp};
    NewtonBodySetAngularDamping(b, d);
    NewtonBodySetFreezeTreshold(b, 0.25f, 0.7f, 5);
    NewtonRagDollSetForceAndTorqueCallback(rd, cb);
    NewtonRagDollEnd(rd);
    NewtonBodySetMatrix(b, m);
    return b;
}

static float len(const float* v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

static void e_angular_damping()
{
    printf("## E1 angular damping: free one-bone ragdoll, omega (0,0,10) about the capsule's transverse z, no forces\n");
    const float damps[] = {0.0f, 0.1f, 0.7f};
    const float dts[] = {0.009f, 0.018f, 0.036f};
    for (float d : damps)
        for (float dt : dts) {
            NewtonWorld* w = world();
            NewtonBody* b = single(w, 10.0f, 1.0f, d);
            float o[3] = {0, 0, 10};
            NewtonBodySetOmega(b, o);
            g_gravity = false; memset(g_force, 0, sizeof g_force); memset(g_torque, 0, sizeof g_torque);
            printf("damp %.1f dt %.3f |w| after steps 1,2,4,8,16:", d, dt);
            for (int s = 1; s <= 16; s++) {
                NewtonUpdate(w, dt);
                if ((s & (s - 1)) == 0) { NewtonBodyGetOmega(b, o); printf(" %.4f", len(o)); }
            }
            printf("\n");
            NewtonDestroy(w);
        }
}

static void e_linear()
{
    printf("## E2 linear: velocity (10,0,0) no force; then gravity from rest (callback force m*-9.8)\n");
    const float dts[] = {0.009f, 0.018f, 0.036f};
    for (float dt : dts) {
        NewtonWorld* w = world();
        NewtonBody* b = single(w, 10.0f, 1.0f, 0.7f);
        float v[3] = {10, 0, 0};
        NewtonBodySetVelocity(b, v);
        g_gravity = false;
        printf("dt %.3f |v| after steps 1,2,4,8,16:", dt);
        for (int s = 1; s <= 16; s++) {
            NewtonUpdate(w, dt);
            if ((s & (s - 1)) == 0) { NewtonBodyGetVelocity(b, v); printf(" %.4f", len(v)); }
        }
        printf("\n");
        NewtonDestroy(w);
        w = world();
        b = single(w, 10.0f, 1.0f, 0.7f);
        g_gravity = true;
        float m[16];
        printf("dt %.3f fall: vz, dz after steps 1,2,4,8,16:", dt);
        for (int s = 1; s <= 16; s++) {
            NewtonUpdate(w, dt);
            if ((s & (s - 1)) == 0) { NewtonBodyGetVelocity(b, v); NewtonBodyGetMatrix(b, m); printf(" %.4f/%.4f", v[2], m[14] - 50.0f); }
        }
        printf("\n");
        NewtonDestroy(w);
    }
}

static void e_torque()
{
    printf("## E3 torque: constant world torque (0,0,1) on a body with tiny (unit-mass) inertia, damping 0.7, dt 0.018\n");
    const float scales[] = {1.0f, 10.0f};
    for (float sc : scales) {
        NewtonWorld* w = world();
        NewtonBody* b = single(w, 10.0f, sc, 0.7f);
        g_gravity = false; memset(g_force, 0, sizeof g_force);
        g_torque[0] = 0; g_torque[1] = 0; g_torque[2] = 1.0f;
        float o[3], m, ix, iy, iz;
        NewtonBodyGetMassMatrix(b, &m, &ix, &iy, &iz);
        printf("inertia x%.0f (Izz %.5f) |w| after steps 1,2,4,8,16,32:", sc, iz);
        for (int s = 1; s <= 32; s++) {
            NewtonUpdate(w, 0.018f);
            if ((s & (s - 1)) == 0) { NewtonBodyGetOmega(b, o); printf(" %.3f", len(o)); }
        }
        printf("\n");
        memset(g_torque, 0, sizeof g_torque);
        NewtonDestroy(w);
    }
}

static void e_big_omega()
{
    printf("## E4 large omega / velocity: set then one step of 0.018 (clamps?)\n");
    const float ws[] = {20.0f, 60.0f, 200.0f};
    for (float x : ws) {
        NewtonWorld* w = world();
        NewtonBody* b = single(w, 10.0f, 1.0f, 0.0f);
        float o[3] = {0, 0, x}, v[3] = {x, 0, 0};
        NewtonBodySetOmega(b, o); NewtonBodySetVelocity(b, v);
        g_gravity = false;
        NewtonUpdate(w, 0.018f);
        NewtonBodyGetOmega(b, o); NewtonBodyGetVelocity(b, v);
        printf("set %.0f: |w| %.3f |v| %.3f\n", x, len(o), len(v));
        NewtonDestroy(w);
    }
}

static void ground(NewtonWorld* w)
{
    NewtonCollision* t = NewtonCreateTreeCollision(w, 0);
    NewtonTreeCollisionBeginBuild(t);
    float q[12] = {-20, -20, 0, 20, -20, 0, 20, 20, 0, -20, 20, 0};
    NewtonTreeCollisionAddFace(t, 4, q, 12, 0);
    NewtonTreeCollisionEndBuild(t, 0);
    NewtonBody* g = NewtonCreateBody(w, t);
    NewtonReleaseCollision(w, t);
    NewtonBodySetMatrix(g, I16);
}

static void e_sleep_and_contact()
{
    printf("## E5 drop on ground: one bone lying (axis x) from z=0.5, gravity, dt 0.018: z, |v|, |w|, awake\n");
    NewtonWorld* w = world();
    ground(w);
    NewtonBody* b = single(w, 10.0f, 1.0f, 0.7f, 0.5f);
    g_gravity = true; memset(g_force, 0, sizeof g_force); memset(g_torque, 0, sizeof g_torque);
    float m[16], v[3], o[3];
    int firstSleep = -1;
    for (int s = 1; s <= 400; s++) {
        NewtonUpdate(w, 0.018f);
        NewtonBodyGetMatrix(b, m); NewtonBodyGetVelocity(b, v); NewtonBodyGetOmega(b, o);
        int st = NewtonBodyGetSleepingState(b);
        if (st == 0 && firstSleep < 0) firstSleep = s;
        if (s <= 10 || s % 25 == 0)
            printf("step %3d z %.4f |v| %.4f |w| %.4f state %d\n", s, m[14], len(v), len(o), st);
    }
    printf("first step with state 0: %d\n", firstSleep);
    NewtonDestroy(w);
    printf("## E5b bounce: dropped from z=1.5 (restitution 0.5 by default material): max z after first contact\n");
    w = world();
    ground(w);
    b = single(w, 10.0f, 1.0f, 0.7f, 1.5f);
    float maxz = -1, minz = 9; bool hit = false;
    for (int s = 1; s <= 120; s++) {
        NewtonUpdate(w, 0.018f);
        NewtonBodyGetMatrix(b, m);
        if (m[14] < 0.11f) hit = true;
        if (!hit) minz = m[14];
        if (hit) maxz = std::fmax(maxz, m[14]);
    }
    printf("rest z %.4f, highest after first contact %.4f\n", m[14], maxz);
    NewtonDestroy(w);
    printf("## E5c slide: lying bone at rest on ground, velocity (3,0,0): distance until |v|<0.05 (friction 1.1/0.9)\n");
    w = world();
    ground(w);
    b = single(w, 10.0f, 1.0f, 0.7f, 0.1f);
    for (int s = 0; s < 30; s++) NewtonUpdate(w, 0.018f);
    NewtonBodyGetMatrix(b, m);
    float x0 = m[12];
    float vv[3] = {3, 0, 0};
    NewtonBodySetVelocity(b, vv);
    int s = 0;
    for (; s < 300; s++) {
        NewtonUpdate(w, 0.018f);
        NewtonBodyGetVelocity(b, v);
        if (len(v) < 0.05f) break;
    }
    NewtonBodyGetMatrix(b, m);
    printf("stopped after %d steps, slid %.4f m\n", s + 1, m[12] - x0);
    NewtonDestroy(w);
}

// two bones: a heavy parent (1e4 kg, inertia x1e4) along +x and a 5 kg child along +x jointed at x=0.5 (parent-relative
// AddBone matrix), limits as the game sets them (cone dir = x, min cone -0.01 deg, bilateral dir = x, angles a/b), no
// gravity; a constant world torque on the child for `steps` steps of 0.018. Returns max and final swing/twist (deg).
struct JointResult { float maxSwing, endSwing, maxTwist, endTwist, gap; int state; };
static JointResult joint_case(float cone, float twist, const float tq[3], float bilA = 0.2f, float bilB = 0.2f,
                              int steps = 56, float minCone = -0.01f)
{
    const float rad = 0.017453292f;
    NewtonWorld* w = world();
    NewtonRagDoll* rd = NewtonCreateRagDoll(w);
    NewtonRagDollBegin(rd);
    float off[16]; memcpy(off, I16, sizeof off); off[12] = 0.25f;
    NewtonCollision* c0 = NewtonCreateCapsule(w, 0.05f, 0.5f, off);
    NewtonCollision* c1 = NewtonCreateCapsule(w, 0.05f, 0.5f, off);
    float m0[16]; memcpy(m0, I16, sizeof m0); m0[14] = 10.0f;
    float m1[16]; memcpy(m1, I16, sizeof m1); m1[12] = 0.5f;
    float size[3] = {0, 0, 0};
    NewtonRagDollBone* b0 = NewtonRagDollAddBone(rd, 0, 0, 1e4f, m0, c0, size);
    NewtonRagDollBone* b1 = NewtonRagDollAddBone(rd, b0, 0, 5.0f, m1, c1, size);
    NewtonRagDollBone* bs[2] = {b0, b1};
    float masses[2] = {1e4f, 5.0f};
    for (int i = 0; i < 2; i++) {
        NewtonBody* b = NewtonRagDollBoneGetBody(bs[i]);
        float in[4], org[4];
        NewtonConvexCollisionCalculateInertialMatrix(i ? c1 : c0, in, org);
        NewtonBodySetCentreOfMass(b, org);
        float sc = i ? 1.0f : 1e4f;
        NewtonBodySetMassMatrix(b, masses[i], in[0] * sc, in[1] * sc, in[2] * sc);
        float d[3] = {0.7f, 0.7f, 0.7f};
        NewtonBodySetAngularDamping(b, d);
        NewtonBodySetFreezeTreshold(b, 0.25f, 0.7f, 5);
        float axis[3] = {1, 0, 0};
        NewtonRagDollBoneSetLimits(bs[i], axis, minCone * rad, cone * rad, twist * rad, axis, bilA, bilB);
    }
    NewtonReleaseCollision(w, c0); NewtonReleaseCollision(w, c1);
    NewtonRagDollSetForceAndTorqueCallback(rd, cb);
    NewtonRagDollEnd(rd);
    g_gravity = false; memset(g_force, 0, sizeof g_force);
    memcpy(g_torque, tq, sizeof g_torque);
    NewtonBody* child = NewtonRagDollBoneGetBody(b1);
    NewtonBody* par = NewtonRagDollBoneGetBody(b0);
    JointResult r = {0, 0, 0, 0, 0, 0};
    float m[16], p[16];
    for (int s = 0; s < steps; s++) {
        NewtonUpdate(w, 0.018f);
        NewtonBodyGetMatrix(child, m); NewtonBodyGetMatrix(par, p);
        float c = m[0] * p[0] + m[1] * p[1] + m[2] * p[2];
        float sw = std::acos(std::fmax(-1.0f, std::fmin(1.0f, c))) / rad;
        float ty = m[4] * p[4] + m[5] * p[5] + m[6] * p[6], tz = m[4] * p[8] + m[5] * p[9] + m[6] * p[10];
        float tw = std::fabs(std::atan2(tz, ty)) / rad;
        r.maxSwing = std::fmax(r.maxSwing, sw); r.maxTwist = std::fmax(r.maxTwist, tw);
        r.endSwing = sw; r.endTwist = tw;
    }
    float d0 = m[12] - (p[12] + 0.5f * p[0]), d1 = m[13] - (p[13] + 0.5f * p[1]), d2 = m[14] - (p[14] + 0.5f * p[2]);
    r.gap = std::sqrt(d0 * d0 + d1 * d1 + d2 * d2);
    r.state = NewtonBodyGetSleepingState(child);
    memset(g_torque, 0, sizeof g_torque);
    NewtonDestroy(w);
    return r;
}

static void e_joint()
{
    printf("## E6 joint limits: heavy parent along +x, 5 kg child jointed at its end; constant torque on the child 1 s;\n");
    printf("##    swing torque about z, twist torque about x: max / final swing and twist (deg)\n");
    const float cones[] = {30, 70};
    const float tqs[] = {0.25f, 0.5f, 1, 2, 5, 20};
    for (float cone : cones)
        for (float t : tqs) {
            float tq[3] = {0, 0, t};
            JointResult r = joint_case(cone, 20, tq);
            printf("cone %2.0f swing torque %5.2f: swing max %6.2f end %6.2f, gap %.4f\n", cone, t, r.maxSwing, r.endSwing, r.gap);
        }
    const float twists[] = {20, 45};
    for (float tw : twists)
        for (float t : tqs) {
            float tq[3] = {t, 0, 0};
            JointResult r = joint_case(30, tw, tq);
            printf("twist %2.0f twist torque %5.2f: twist max %6.2f end %6.2f, swing max %.2f\n", tw, t, r.maxTwist, r.endTwist, r.maxSwing);
        }
    printf("## E6b bilateral angles (a, b) with cone 70, swing torque 5 about z and about y\n");
    const float bils[][2] = {{0.2f, 0.2f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {0.2f, 1.0f}};
    for (auto& bl : bils) {
        float tz[3] = {0, 0, 5}, ty[3] = {0, 5, 0}, tzn[3] = {0, 0, -5};
        JointResult a = joint_case(70, 20, tz, bl[0], bl[1]), b = joint_case(70, 20, ty, bl[0], bl[1]),
                    c = joint_case(70, 20, tzn, bl[0], bl[1]);
        printf("bilateral %.1f/%.1f: swing about +z %6.2f, about +y %6.2f, about -z %6.2f\n", bl[0], bl[1], a.endSwing,
               b.endSwing, c.endSwing);
    }
    printf("## E6c min cone: cone 70, min cone 0 / 20 / -20 deg, swing torque 0.5 and 5\n");
    const float mins[] = {0.0f, 20.0f, -20.0f};
    for (float mc : mins) {
        float t1[3] = {0, 0, 0.5f}, t2[3] = {0, 0, 5};
        JointResult a = joint_case(70, 20, t1, 0.2f, 0.2f, 56, mc), b = joint_case(70, 20, t2, 0.2f, 0.2f, 56, mc);
        printf("min cone %5.1f: torque 0.5 -> %6.2f, torque 5 -> %6.2f\n", mc, a.endSwing, b.endSwing);
    }
}

static void e_capsule_wake()
{
    printf("## E7 non-ragdoll capsule body (no callback): mass 300, velocity (0,0,-1) set once, dt 0.018: z after 10 steps,\n");
    printf("##    does it fall by itself (gravity?) and keep its velocity (linear damping?)\n");
    NewtonWorld* w = world();
    NewtonCollision* c = NewtonCreateCapsule(w, 0.4f, 1.0f, I16);
    NewtonBody* b = NewtonCreateBody(w, c);
    NewtonReleaseCollision(w, c);
    float m[16]; memcpy(m, I16, sizeof m); m[14] = 20.0f;
    NewtonBodySetMatrix(b, m);
    float in[4], org[4];
    NewtonConvexCollisionCalculateInertialMatrix(c, in, org);
    NewtonBodySetCentreOfMass(b, org);
    NewtonBodySetMassMatrix(b, 300.0f, in[0], in[1], in[2]);
    float v[3] = {0, 0, -1};
    NewtonBodySetVelocity(b, v);
    for (int s = 0; s < 10; s++) NewtonUpdate(w, 0.018f);
    NewtonBodyGetMatrix(b, m); NewtonBodyGetVelocity(b, v);
    printf("z %.4f vz %.4f state %d\n", m[14], v[2], NewtonBodyGetSleepingState(b));
    NewtonDestroy(w);
}

static void e_inertia_floor()
{
    printf("## E8 torque vs mass and inertia (one step of 0.018 from rest, damping 0): |w| after 1 step\n");
    struct Case { float mass, scale, torque; };
    Case cs[] = {{10, 1, 0.1f}, {10, 1, 1}, {10, 1, 10}, {1, 1, 1}, {1, 1, 0.1f}, {100, 1, 10}, {100, 1, 100}, {10, 10, 1},
                 {10, 100, 1}, {10, 0.1f, 1}};
    for (const Case& k : cs) {
        NewtonWorld* w = world();
        NewtonBody* b = single(w, k.mass, k.scale, 0.0f);
        g_gravity = false; memset(g_force, 0, sizeof g_force);
        g_torque[0] = 0; g_torque[1] = 0; g_torque[2] = k.torque;
        float m, ix, iy, iz, o[3];
        NewtonBodyGetMassMatrix(b, &m, &ix, &iy, &iz);
        NewtonUpdate(w, 0.018f);
        NewtonBodyGetOmega(b, o);
        printf("mass %5.1f Izz %.5f torque %6.2f: |w| %.5f (T/I*h %.5f) -> effective Izz %.5f\n", k.mass, iz, k.torque, len(o),
               k.torque / iz / 60.0f, k.torque / 60.0f / std::fmax(len(o), 1e-9f));
        memset(g_torque, 0, sizeof g_torque);
        NewtonDestroy(w);
    }
    printf("## E9 a frozen body: does SetVelocity / SetMatrix wake it? (E5's bone after it froze)\n");
    NewtonWorld* w = world();
    ground(w);
    NewtonBody* b = single(w, 10.0f, 1.0f, 0.7f, 0.5f);
    g_gravity = true;
    int s = 0;
    for (; s < 600 && NewtonBodyGetSleepingState(b); s++) NewtonUpdate(w, 0.018f);
    float m[16], v[3] = {2, 0, 0};
    NewtonBodyGetMatrix(b, m);
    float x0 = m[12];
    printf("frozen after %d steps\n", s);
    NewtonBodySetVelocity(b, v);
    printf("after SetVelocity: state %d;", NewtonBodyGetSleepingState(b));
    NewtonUpdate(w, 0.018f);
    NewtonBodyGetMatrix(b, m);
    printf(" one step later: state %d, moved %.4f\n", NewtonBodySetVelocity ? NewtonBodyGetSleepingState(b) : 0, m[12] - x0);
    m[14] += 0.5f;
    NewtonBodySetMatrix(b, m);
    printf("after SetMatrix: state %d;", NewtonBodyGetSleepingState(b));
    NewtonUpdate(w, 0.018f);
    float m2[16]; NewtonBodyGetMatrix(b, m2);
    printf(" one step later: state %d, dz %.4f\n", NewtonBodyGetSleepingState(b), m2[14] - m[14]);
    NewtonDestroy(w);
}

static void e_small_steps()
{
    printf("## E10 small steps: free one-bone ragdoll under gravity, one update of dt: vz after it (= -9.8 * effective dt)\n");
    const float dts[] = {0.00005f, 0.0001f, 0.0005f, 0.001f, 0.002f, 0.005f, 0.01f, 0.0166f, 0.0167f, 0.02f};
    for (float dt : dts) {
        NewtonWorld* w = world();
        NewtonBody* b = single(w, 10.0f, 1.0f, 0.0f);
        g_gravity = true; memset(g_force, 0, sizeof g_force); memset(g_torque, 0, sizeof g_torque);
        NewtonUpdate(w, dt);
        float v[3]; NewtonBodyGetVelocity(b, v);
        printf("dt %.5f: vz %.6f -> effective dt %.6f\n", dt, v[2], -v[2] / 9.8f);
        NewtonDestroy(w);
    }
    printf("## E10b resting on the ground, alternating dt 0.018 and dt: |v| max over 200 updates\n");
    const float tiny[] = {0.0001f, 0.0005f, 0.001f};
    for (float dt : tiny) {
        NewtonWorld* w = world();
        ground(w);
        NewtonBody* b = single(w, 10.0f, 1.0f, 0.7f, 0.1f);
        g_gravity = true;
        float vmax = 0, v[3];
        for (int s = 0; s < 200; s++) {
            NewtonUpdate(w, (s & 1) ? dt : 0.018f);
            NewtonBodyGetVelocity(b, v);
            if (s > 20) vmax = std::fmax(vmax, len(v));
        }
        printf("dt %.4f: max |v| %.4f\n", dt, vmax);
        NewtonDestroy(w);
    }
}

int main(int argc, char** argv)
{
    const char* only = argc > 1 ? argv[1] : "";
    if (!*only || strchr(only, '1')) e_angular_damping();
    if (!*only || strchr(only, '2')) e_linear();
    if (!*only || strchr(only, '3')) e_torque();
    if (!*only || strchr(only, '4')) e_big_omega();
    if (!*only || strchr(only, '5')) e_sleep_and_contact();
    if (!*only || strchr(only, '6')) e_joint();
    if (!*only || strchr(only, '7')) e_capsule_wake();
    if (!*only || strchr(only, '8')) e_inertia_floor();
    if (!*only || strchr(only, '9')) e_small_steps();
    return 0;
}
