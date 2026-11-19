// physics_ragdoll_drop: builds a ragdoll with the call sequence of src/game/agent4/agent_logic.cpp (create_world,
// Agent::start_ragdoll, PhysSet::add_bone, build_static_collision, add_capsule_body) and apply_force_and_torque
// (src/rgl/xmlcfg/game_util.cpp), drops it on tree-collision ground next to a capsule body, steps it with
// PhysSet::update's substeps, and prints body positions over time. Exit 0 only if it fell, collided, settled and its
// joints stayed within their limits.
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

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
void NewtonBodyGetMassMatrix(const NewtonBody*, float*, float*, float*, float*);
void NewtonBodySetMassMatrix(const NewtonBody*, float, float, float, float);
void NewtonBodySetCentreOfMass(const NewtonBody*, const float*);
void NewtonBodySetMaterialGroupID(const NewtonBody*, int);
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
void NewtonRagDollBoneGetGlobalMatrix(const NewtonRagDollBone*, float*);
void NewtonRagDollBoneSetLimits(const NewtonRagDollBone*, const float*, float, float, float, const float*, float, float);
}

// game_util.cpp apply_force_and_torque, without the pending-hit branch
static void apply_force_and_torque(const NewtonBody* body)
{
    float mass, ixx, iyy, izz;
    NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
    float f[3] = {0.0f, 0.0f, mass * -9.8f};
    NewtonBodyAddForce(body, f);
}

struct Mat { float m[16]; };
static Mat frame(const float s[3], const float f[3], const float o[3])
{   // rows s, f, u = s x f, origin o
    Mat r; memset(r.m, 0, sizeof r.m);
    float u[3] = {s[1] * f[2] - s[2] * f[1], s[2] * f[0] - s[0] * f[2], s[0] * f[1] - s[1] * f[0]};
    for (int i = 0; i < 3; i++) { r.m[i] = s[i]; r.m[4 + i] = f[i]; r.m[8 + i] = u[i]; r.m[12 + i] = o[i]; }
    r.m[15] = 1.0f;
    return r;
}

struct BoneDef { const char* name; int parent; float origin[3]; float dir[3]; float len, radius, mass, cone, twist; };
static const BoneDef kBones[] = {
    {"pelvis",  -1, {0, 0, 1.00f},      {0, 0, 1},  0.15f, 0.12f, 10, 0, 0},
    {"spine",    0, {0, 0, 1.15f},      {0, 0, 1},  0.35f, 0.14f, 10, 30, 20},
    {"head",     1, {0, 0, 1.65f},      {0, 0, 1},  0.10f, 0.10f, 5,  40, 30},
    {"thigh_l",  0, {0.10f, 0, 0.98f},  {0, 0, -1}, 0.35f, 0.09f, 10, 70, 30},
    {"calf_l",   3, {0.10f, 0, 0.55f},  {0, 0, -1}, 0.35f, 0.06f, 6,  5,  45},
    {"thigh_r",  0, {-0.10f, 0, 0.98f}, {0, 0, -1}, 0.35f, 0.09f, 10, 70, 30},
    {"calf_r",   5, {-0.10f, 0, 0.55f}, {0, 0, -1}, 0.35f, 0.06f, 6,  5,  45},
    {"arm_l",    1, {0.22f, 0, 1.50f},  {1, 0, 0},  0.30f, 0.05f, 4,  80, 40},
    {"arm_r",    1, {-0.22f, 0, 1.50f}, {-1, 0, 0}, 0.30f, 0.05f, 4,  80, 40},
};
static const int kN = sizeof kBones / sizeof kBones[0];

static float dot(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
// world-space vector -> the body's local space (row-vector convention: local = world * M^T)
static void toLocal(const Mat& M, const float* w, float* l)
{ for (int i = 0; i < 3; i++) l[i] = dot(M.m + 4 * i, w); }
static void toWorld(const Mat& M, const float* l, float* w)
{ for (int j = 0; j < 3; j++) w[j] = l[0] * M.m[j] + l[1] * M.m[4 + j] + l[2] * M.m[8 + j]; }

int main()
{
    // PhysSet::create_world
    NewtonWorld* w = NewtonCreate(0, 0);
    NewtonSetSolverModel(w, 6);
    float mn[3] = {-1000, -1000, -100}, mx[3] = {1000, 1000, 1000};
    NewtonSetWorldSize(w, mn, mx);
    int material = NewtonMaterialGetDefaultGroupID(w);
    NewtonMaterialSetDefaultSoftness(w, material, material, 0.1f);
    NewtonMaterialSetDefaultElasticity(w, material, material, 0.5f);
    NewtonMaterialSetDefaultFriction(w, material, material, 1.1f, 0.9f);

    // Agent::start_ragdoll: bones, add_bone per bone
    NewtonRagDoll* rd = NewtonCreateRagDoll(w);
    NewtonRagDollBegin(rd);
    const NewtonRagDollBone* bone[kN];
    Mat T[kN];
    const float lift = 1.5f;
    for (int i = 0; i < kN; i++) {
        const BoneDef& d = kBones[i];
        float side[3] = {d.dir[0], d.dir[1], d.dir[2]};
        float fr[3] = {1, 0, 0};
        if (side[0] != 0) { fr[0] = 0; fr[1] = 1; }                 // keep f perpendicular to s
        T[i] = frame(side, fr, d.origin);                           // m0: bone frame, x axis along the bone
        float one[3] = {1, 0, 0}, two[3] = {0, 1, 0}, mid[3] = {d.len * 0.5f, 0, 0};
        Mat m1 = frame(one, two, mid);                              // m1: capsule frame inside the bone
        // PhysSet::add_bone
        NewtonCollision* cap = NewtonCreateCapsule(w, d.radius, d.len, m1.m);
        float v[4] = {0, 0, 0, 0};
        bone[i] = NewtonRagDollAddBone(rd, d.parent >= 0 ? bone[d.parent] : 0, 0, d.mass, T[i].m, cap, v);
        NewtonReleaseCollision(w, cap);
        NewtonBody* body = NewtonRagDollBoneGetBody(bone[i]);
        float inertia[4], origin[4];
        NewtonConvexCollisionCalculateInertialMatrix(cap, inertia, origin);   // after the release, as the game does
        NewtonBodySetCentreOfMass(body, origin);
        float is = getenv("INERTIA_SCALE") ? atof(getenv("INERTIA_SCALE")) * d.mass : 1.0f;
        NewtonBodySetMassMatrix(body, d.mass, inertia[0] * is, inertia[1] * is, inertia[2] * is);
        NewtonBodySetMaterialGroupID(body, material);
        v[0] = v[1] = v[2] = 0.7f;
        NewtonBodySetAngularDamping(body, v);
        NewtonBodySetFreezeTreshold(body, 0.25f, 0.7f, 5);
        const float rad = 0.017453292f;
        NewtonRagDollBoneSetLimits(bone[i], d.dir, 0.0f, d.cone * rad, d.twist * rad, d.dir, 0.2f, 0.2f);
        // the joint cone axis is d.dir in world space at the creation pose
    }
    // velocity/omega seeding, callback, End, then the teleport to the animated pose (here: lifted, tilted drift)
    for (int i = 0; i < kN; i++) {
        NewtonBody* nb = NewtonRagDollBoneGetBody(bone[i]);
        float vel[3] = {1.5f, 0.0f, 0.0f}, om[3] = {0.0f, 2.0f, 0.5f};
        NewtonBodySetVelocity(nb, vel);
        NewtonBodySetOmega(nb, om);
    }
    NewtonRagDollSetForceAndTorqueCallback(rd, apply_force_and_torque);
    NewtonRagDollEnd(rd);
    for (int i = 0; i < kN; i++) {
        Mat W = T[i]; W.m[14] += lift;
        NewtonBodySetMatrix(NewtonRagDollBoneGetBody(bone[i]), W.m);
    }

    // build_static_collision: a 30 x 30 quad at z = 0 (count 4, packed xyz, stride 12), then the body
    NewtonCollision* tree = NewtonCreateTreeCollision(w, 0);
    NewtonTreeCollisionBeginBuild(tree);
    float quad[12] = {-15, -15, 0, 15, -15, 0, 15, 15, 0, -15, 15, 0};
    NewtonTreeCollisionAddFace(tree, 4, quad, 12, 0);
    NewtonTreeCollisionEndBuild(tree, 0);
    NewtonBody* ground = NewtonCreateBody(w, tree);
    NewtonReleaseCollision(w, tree);
    float id16[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    NewtonBodySetMatrix(ground, id16);

    // add_capsule_body: a lying capsule (the "mount") in the ragdoll's path
    float one[3] = {1, 0, 0}, two[3] = {0, 1, 0}, zero[3] = {0, 0, 0};
    Mat m1 = frame(one, two, zero);
    float o2[3] = {1.0f, 0.0f, 0.4f};
    Mat m0 = frame(one, two, o2);
    NewtonCollision* cap = NewtonCreateCapsule(w, 0.4f, 1.0f, m1.m);
    NewtonBody* mount = NewtonCreateBody(w, cap);
    NewtonReleaseCollision(w, cap);
    NewtonBodySetMatrix(mount, m0.m);
    float inertia[4], origin[4];
    NewtonConvexCollisionCalculateInertialMatrix(cap, inertia, origin);
    NewtonBodySetCentreOfMass(mount, origin);
    NewtonBodySetMassMatrix(mount, 300.0f, inertia[0], inertia[1], inertia[2]);
    NewtonBodySetMaterialGroupID(mount, material);

    // joint-limit bookkeeping: axis in each body's local space at creation
    float ax_p[kN][3], ax_c[kN][3];
    for (int i = 1; i < kN; i++) {
        toLocal(T[kBones[i].parent], kBones[i].dir, ax_p[i]);
        toLocal(T[i], kBones[i].dir, ax_c[i]);
    }

    // PhysSet::update: 0.018 s substeps
    const float step = 0.018f;
    float t = 0.0f;
    float minZ = 1e9f, maxSwing[kN] = {0}, startZ = 0, pelvisMinZ = 1e9f;
    int sleeping = 0;
    printf("%6s  %-26s %-26s %5s %6s\n", "t", "pelvis (x y z)", "head (x y z)", "minZ", "sleep");
    for (int frameNo = 0; frameNo <= 600; frameNo++) {   // ~10.8 s
        float pm[16], hm[16], mm[16];
        NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(bone[0]), pm);
        NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(bone[2]), hm);
        NewtonBodyGetMatrix(mount, mm);
        float mo[3]; NewtonBodyGetOmega(mount, mo);
        float zmin = 1e9f;
        sleeping = 0;
        for (int i = 0; i < kN; i++) {
            Mat M; NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(bone[i]), M.m);
            // lowest point of the capsule axis end-points minus radius (axis = body x, from origin to origin + len)
            float e0[3] = {M.m[12], M.m[13], M.m[14]};
            float e1[3] = {e0[0] + M.m[0] * kBones[i].len, e0[1] + M.m[1] * kBones[i].len, e0[2] + M.m[2] * kBones[i].len};
            zmin = std::fmin(zmin, std::fmin(e0[2], e1[2]) - kBones[i].radius);
            sleeping += NewtonBodyGetSleepingState(NewtonRagDollBoneGetBody(bone[i]));
            if (i > 0) {
                Mat P; NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(bone[kBones[i].parent]), P.m);
                float a[3], b[3];
                toWorld(P, ax_p[i], a); toWorld(M, ax_c[i], b);
                float c = dot(a, b);
                c = c > 1 ? 1 : (c < -1 ? -1 : c);
                float ang = std::acos(c);
                if (ang > maxSwing[i]) maxSwing[i] = ang;
            }
        }
        if (frameNo == 0) startZ = pm[14];
        minZ = std::fmin(minZ, zmin);
        pelvisMinZ = std::fmin(pelvisMinZ, pm[14]);
        if (frameNo % 14 == 0)
            printf("%6.2f  (%6.2f %6.2f %6.2f)   (%6.2f %6.2f %6.2f)   %5.2f %3d/%d  mount (%5.2f %5.2f %5.2f) w(%5.2f %5.2f %5.2f)\n", t,
                   pm[12], pm[13], pm[14], hm[12], hm[13], hm[14], zmin, sleeping, kN, mm[12], mm[13], mm[14], mo[0], mo[1], mo[2]);
        NewtonUpdate(w, step);
        t += step;
    }

    // verdicts
    float pm[16], v[3], om[3];
    NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(bone[0]), pm);
    float maxV = 0, maxO = 0;
    for (int i = 0; i < kN; i++) {
        NewtonBodyGetVelocity(NewtonRagDollBoneGetBody(bone[i]), v);
        NewtonBodyGetOmega(NewtonRagDollBoneGetBody(bone[i]), om);
        maxV = std::fmax(maxV, std::sqrt(dot(v, v)));
        maxO = std::fmax(maxO, std::sqrt(dot(om, om)));
    }
    bool fell = pelvisMinZ < startZ - 0.5f;
    bool collided = minZ > -0.12f && pelvisMinZ > 0.02f;     // never went through the ground
    bool settled = maxV < 0.1f && maxO < 0.5f;
    bool jointsOk = true;
    printf("\njoint swing (deg): transient max / at rest / limit  (pass: transient <= limit + 10, at rest <= limit + 2)\n");
    for (int i = 1; i < kN; i++) {
        Mat P, M; float a[3], b[3];
        NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(bone[kBones[i].parent]), P.m);
        NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(bone[i]), M.m);
        toWorld(P, ax_p[i], a); toWorld(M, ax_c[i], b);
        float c = dot(a, b); c = c > 1 ? 1 : (c < -1 ? -1 : c);
        float rest = std::acos(c) * 57.29578f, deg = maxSwing[i] * 57.29578f;
        bool ok = deg <= kBones[i].cone + 10.0f && rest <= kBones[i].cone + 2.0f;
        jointsOk = jointsOk && ok;
        printf("  %-8s %6.1f / %6.1f / %4.0f %s\n", kBones[i].name, deg, rest, kBones[i].cone, ok ? "ok" : "VIOLATED");
    }
    printf("fell=%d (pelvis z %.2f -> min %.2f)  no-tunnel=%d (lowest point %.3f)  settled=%d (|v| %.3f |w| %.3f, asleep %d/%d)  joints=%d\n",
           fell, startZ, pelvisMinZ, collided, minZ, settled, maxV, maxO, sleeping, kN, jointsOk);
    NewtonDestroy(w);
    bool pass = fell && collided && settled && jointsOk;
    printf("%s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : 1;
}
