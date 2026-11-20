// physics_trace_replay: feeds a Newton 1.x call trace recorded from the original game (port/physics/trace/RECORDING.md,
// format port/physics/trace/newton_trace.h) through the Newton API implemented by port/physics/newton_jolt.cpp, and diffs
// every value the real Newton 1.53 returned against what the shim returns.
//
//   physics_trace_replay TRACE [--world N] [--sync] [--steps] [--detail N] [--quiet]
//
// The game's inputs (Set*, AddBone, limits, tree faces, the mount capsule's per-frame pose) are replayed as recorded.
// The force callback: when the shim calls it for a body, the forces the game added to that body in the recorded step are
// applied (gravity and a pending hit); a body Newton had asleep gets the game's gravity. Default mode is open loop (the
// shim integrates on its own: drift accumulates, which is what the game would see); --sync resets every body to Newton's
// recorded pose and velocities after each step, so errors are one-step (model) errors.
//
// Compared: creation pose of every ragdoll bone (AddBone's matrix is parent-relative in 1.x), capsule AABB and unit-mass
// inertia, every Get* the game made, the proxy's per-step probe of every body (pose, velocity, omega, sleeping).
// Tolerances (a value beyond them is a divergence): position 0.02 m, rotation 3 deg, velocity 0.2 m/s, omega 0.5 rad/s,
// inertia 5 % relative, AABB 0.005 m. Per world: first divergence, max error per time bucket since the ragdoll started,
// settle time (all bones asleep) and the final pose error. Exit 1 when a world fails the qualitative check: final root
// position within 0.5 m and mean bone position within 0.35 m of Newton's, settles within 1.5 s of Newton (or both never
// settle), no NaN.
#include "../trace/newton_trace.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <zlib.h>

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
void NewtonCollisionCalculateAABB(const NewtonCollision*, const float*, float*, float*);
NewtonBody* NewtonCreateBody(const NewtonWorld*, const NewtonCollision*);
void NewtonDestroyBody(const NewtonWorld*, const NewtonBody*);
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

namespace {

const float kTolPos = 0.02f, kTolRot = 3.0f, kTolVel = 0.2f, kTolOmega = 0.5f, kTolInertia = 0.05f, kTolAabb = 0.005f;
const float kBuckets[] = {0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 1e9f};
const int kNB = 7;
const char* kBucketNames[kNB] = {"<0.1s", "<0.25s", "<0.5s", "<1s", "<2s", "<4s", ">=4s"};

struct Rec { int op, thread; const uint8_t* p; unsigned n; size_t index; };

struct Reader {
    const uint8_t* p; unsigned n, pos = 0;
    Reader(const Rec& r) : p(r.p), n(r.n) {}
    uint32_t h() { uint32_t v = 0; if (pos + 4 <= n) memcpy(&v, p + pos, 4); pos += 4; return v; }
    int i() { int v = 0; if (pos + 4 <= n) memcpy(&v, p + pos, 4); pos += 4; return v; }
    float f() { float v = 0; if (pos + 4 <= n) memcpy(&v, p + pos, 4); pos += 4; return v; }
    void v3(float* o) { for (int k = 0; k < 3; k++) o[k] = f(); }
    void M(float* o) { for (int k = 0; k < 16; k++) o[k] = f(); }
    bool m(float* o) { bool present = pos < n && p[pos]; pos++; if (present) M(o); return present; }
};

float len3(const float* a) { return std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]); }
float dist3(const float* a, const float* b) { float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; return len3(d); }
// angle (deg) between the rotations of two Newton matrices (rows = axes)
float rotErr(const float* a, const float* b)
{
    float tr = 0;
    for (int i = 0; i < 3; i++)
        for (int k = 0; k < 3; k++) tr += a[i * 4 + k] * b[i * 4 + k];
    float c = std::max(-1.0f, std::min(1.0f, (tr - 1.0f) * 0.5f));
    return std::acos(c) * 57.29578f;
}
bool finite16(const float* m) { for (int i = 0; i < 16; i++) if (!std::isfinite(m[i])) return false; return true; }

struct Err {   // max errors
    float pos = 0, rot = 0, vel = 0, omega = 0;
    void pose(float p, float r) { pos = std::max(pos, p); rot = std::max(rot, r); }
};

struct World {
    int id = 0;
    uint32_t handle = 0;
    NewtonWorld* w = nullptr;
    double t = 0;             // simulated seconds (sum of NewtonUpdate dt)
    int steps = 0, bones = 0, ragdollBodies = 0;
    bool mounted = false, horse = false;
    Err bucket[kNB];
    long cmp = 0, divergent = 0, sleepCmp = 0, sleepMismatch = 0, cbExtra = 0, cbMissing = 0;
    std::string first;        // first divergence
    float createPos = 0, createRot = 0;   // creation pose error (AddBone)
    double newtonSettle = -1, shimSettle = -1;
    std::map<uint32_t, std::pair<std::vector<float>, std::vector<float>>> lastPose;   // body -> (newton, shim) matrix
    uint32_t root = 0;
    std::vector<uint32_t> ragBodies;   // trace handles of this world's ragdoll bone bodies
    bool nan = false;
    bool isRag(uint32_t h) const { return std::find(ragBodies.begin(), ragBodies.end(), h) != ragBodies.end(); }
    // the qualitative check (see the header comment)
    float rootErr = 0, meanErr = 0;
    bool settleOk() const
    {
        return (newtonSettle < 0 && shimSettle < 0) ||
               (newtonSettle >= 0 && shimSettle >= 0 && std::fabs(newtonSettle - shimSettle) <= 1.5);
    }
    bool ok()
    {
        rootErr = meanErr = 0; int n = 0;
        for (auto& kv : lastPose) {
            float e = std::sqrt(std::pow(kv.second.first[12] - kv.second.second[12], 2.0f) +
                                std::pow(kv.second.first[13] - kv.second.second[13], 2.0f) +
                                std::pow(kv.second.first[14] - kv.second.second[14], 2.0f));
            meanErr += e; n++;
            if (kv.first == root) rootErr = e;
        }
        meanErr = n ? meanErr / n : 0;
        return !nan && rootErr < 0.5f && meanErr < 0.35f && settleOk();
    }
    int bucketOf() const { int b = 0; while (b < kNB - 1 && t >= kBuckets[b]) b++; return b; }
};

struct Replay {
    std::vector<uint8_t> data;
    std::vector<Rec> recs;
    std::unordered_map<uint32_t, NewtonWorld*> worlds;
    std::unordered_map<uint32_t, NewtonCollision*> colls;
    std::unordered_map<uint32_t, NewtonBody*> bodies;
    std::unordered_map<uint32_t, NewtonRagDoll*> rds;
    std::unordered_map<uint32_t, NewtonRagDollBone*> bones;
    std::unordered_map<const NewtonBody*, uint32_t> bodyHandle;
    std::unordered_map<uint32_t, uint32_t> owner;       // any trace handle -> world handle
    std::unordered_map<uint32_t, int> curWorld;         // world handle -> index in all
    std::vector<World> all;
    std::unordered_map<uint32_t, std::vector<std::pair<int, std::vector<float>>>> pendingForces;   // body -> (op, v3)
    std::vector<uint32_t> calledBack;
    int onlyWorld = -1;
    bool sync = false, steps = false, quiet = false;
    int detail = 0;                                       // --detail N: every bone's errors for the first N steps
    Err global[kNB];
    long cmpAll = 0, divAll = 0;
    std::map<std::string, std::pair<long, double>> perOp;   // op -> (count beyond tol, max error)
    std::map<std::string, std::vector<float>> samples;      // op -> every error (for percentiles)

    World* worldOf(uint32_t h)
    {
        auto o = owner.find(h);
        if (o == owner.end()) return nullptr;
        auto c = curWorld.find(o->second);
        return c == curWorld.end() ? nullptr : &all[c->second];
    }
    bool skip(World* w) const { return onlyWorld >= 0 && (!w || w->id != onlyWorld); }

    void note(World* w, const char* what, size_t idx, float err, float tol, const char* unit)
    {
        cmpAll++;
        auto& po = perOp[what];
        po.second = std::max(po.second, (double)err);
        samples[what].push_back(err);
        if (!w) return;
        w->cmp++;
        if (!(err <= tol)) {
            po.first++;
            divAll++;
            w->divergent++;
            if (w->first.empty()) {
                char b[256];
                snprintf(b, sizeof b, "record %zu %s at t=%.3fs (step %d): %.4g %s (tol %.3g)", idx, what, w->t, w->steps, err, unit, tol);
                w->first = b;
            }
        }
    }
    void bucketPose(World* w, float p, float r)
    {
        if (!w) return;
        int b = w->bucketOf();
        w->bucket[b].pose(p, r); global[b].pose(p, r);
    }
    void bucketVel(World* w, float v, float o)
    {
        if (!w) return;
        int b = w->bucketOf();
        w->bucket[b].vel = std::max(w->bucket[b].vel, v); w->bucket[b].omega = std::max(w->bucket[b].omega, o);
        global[b].vel = std::max(global[b].vel, v); global[b].omega = std::max(global[b].omega, o);
    }

    bool load(const char* path)
    {
        gzFile f = gzopen(path, "rb");   // plain or gzip-compressed (the committed reference trace is .bin.gz)
        if (!f) return false;
        uint8_t buf[1 << 16];
        int got;
        while ((got = gzread(f, buf, sizeof buf)) > 0) data.insert(data.end(), buf, buf + got);
        gzclose(f);
        size_t n = data.size();
        if (n < 8 || memcmp(data.data(), NT_MAGIC, 4)) return false;
        size_t pos = 8;
        while (pos + 4 <= data.size()) {
            uint16_t len; memcpy(&len, &data[pos + 2], 2);
            if (pos + 4 + len > data.size()) break;
            recs.push_back({data[pos], data[pos + 1], &data[pos + 4], len, recs.size()});
            pos += 4 + len;
        }
        return true;
    }

    void run();
    int report();
};

Replay* g_rp;

void forceCallback(const NewtonBody* body)
{
    Replay& rp = *g_rp;
    auto bh = rp.bodyHandle.find(body);
    uint32_t h = bh == rp.bodyHandle.end() ? 0 : bh->second;
    rp.calledBack.push_back(h);
    auto it = rp.pendingForces.find(h);
    if (it != rp.pendingForces.end()) {
        for (auto& fv : it->second) {
            if (fv.first == NT_BodyAddForce) NewtonBodyAddForce(body, fv.second.data());
            else NewtonBodyAddTorque(body, fv.second.data());
        }
        return;
    }
    // Newton did not call back for this body in this step (it was asleep there): the game's gravity
    float mass, ixx, iyy, izz;
    NewtonBodyGetMassMatrix(body, &mass, &ixx, &iyy, &izz);
    float g[3] = {0.0f, 0.0f, -9.8f * mass};
    NewtonBodyAddForce(body, g);
    if (World* w = rp.worldOf(h)) w->cbExtra++;
}

void Replay::run()
{
    g_rp = this;
    std::vector<bool> consumed(recs.size(), false);
    for (size_t ri = 0; ri < recs.size(); ri++) {
        if (consumed[ri]) continue;
        const Rec& r = recs[ri];
        Reader rd(r);
        switch (r.op) {
        case NT_Create: {
            rd.h(); rd.h(); uint32_t wh = rd.h();
            World w;
            w.id = (int)all.size() + 1;
            w.handle = wh;
            all.push_back(w);
            curWorld[wh] = (int)all.size() - 1;
            owner[wh] = wh;
            if (skip(&all.back())) break;
            all.back().w = NewtonCreate(0, 0);
            worlds[wh] = all.back().w;
            break;
        }
        case NT_Destroy: {
            uint32_t wh = rd.h();
            World* w = worldOf(wh);
            if (skip(w)) break;
            if (worlds.count(wh)) NewtonDestroy(worlds[wh]);
            worlds.erase(wh);
            break;
        }
        case NT_SetSolverModel: { World* w = worldOf(rd.h()); if (skip(w)) break; NewtonSetSolverModel(w->w, rd.i()); break; }
        case NT_SetWorldSize: {
            World* w = worldOf(rd.h()); if (skip(w)) break;
            float a[3], b[3]; rd.v3(a); rd.v3(b); NewtonSetWorldSize(w->w, a, b); break;
        }
        case NT_MaterialGetDefaultGroupID: { World* w = worldOf(rd.h()); if (skip(w)) break; NewtonMaterialGetDefaultGroupID(w->w); break; }
        case NT_MaterialSetDefaultSoftness: {
            World* w = worldOf(rd.h()); if (skip(w)) break; int a = rd.i(), b = rd.i(); NewtonMaterialSetDefaultSoftness(w->w, a, b, rd.f()); break;
        }
        case NT_MaterialSetDefaultElasticity: {
            World* w = worldOf(rd.h()); if (skip(w)) break; int a = rd.i(), b = rd.i(); NewtonMaterialSetDefaultElasticity(w->w, a, b, rd.f()); break;
        }
        case NT_MaterialSetDefaultFriction: {
            World* w = worldOf(rd.h()); if (skip(w)) break;
            int a = rd.i(), b = rd.i(); float s = rd.f(), k = rd.f(); NewtonMaterialSetDefaultFriction(w->w, a, b, s, k); break;
        }
        case NT_CreateCapsule: {
            uint32_t wh = rd.h(); World* w = worldOf(wh);
            float rad = rd.f(), h = rd.f(), m[16]; bool has = rd.m(m); uint32_t ch = rd.h();
            owner[ch] = wh;
            if (skip(w)) break;
            colls[ch] = NewtonCreateCapsule(w->w, rad, h, has ? m : nullptr);
            break;
        }
        case NT_ProbeCollision: {
            uint32_t ch = rd.h(); World* w = worldOf(ch);
            if (skip(w) || !colls.count(ch)) break;
            float a0[3], a1[3], b0[4], b1[4], id[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            rd.v3(a0); rd.v3(a1);
            NewtonCollisionCalculateAABB(colls[ch], id, b0, b1);
            // Newton 1.53 reports the shape's own box (the offset is not applied here): compare extents
            float e = 0;
            for (int k = 0; k < 3; k++) e = std::max(e, std::fabs((a1[k] - a0[k]) - (b1[k] - b0[k])) * 0.5f);
            note(w, "capsule AABB half-extent", r.index, e, kTolAabb, "m");
            break;
        }
        case NT_CreateTreeCollision: {
            uint32_t wh = rd.h(); World* w = worldOf(wh); rd.h(); uint32_t ch = rd.h();
            owner[ch] = wh;
            if (skip(w)) break;
            colls[ch] = NewtonCreateTreeCollision(w->w, 0);
            break;
        }
        case NT_TreeCollisionBeginBuild: { uint32_t ch = rd.h(); if (skip(worldOf(ch)) || !colls.count(ch)) break; NewtonTreeCollisionBeginBuild(colls[ch]); break; }
        case NT_TreeCollisionAddFace: {
            uint32_t ch = rd.h(); if (skip(worldOf(ch)) || !colls.count(ch)) break;
            rd.i(); int attr = rd.i(); int n = rd.i();
            std::vector<float> v(3 * std::max(n, 0));
            for (int k = 0; k < 3 * n; k++) v[k] = rd.f();
            NewtonTreeCollisionAddFace(colls[ch], n, v.data(), 12, attr);
            break;
        }
        case NT_TreeCollisionEndBuild: { uint32_t ch = rd.h(); if (skip(worldOf(ch)) || !colls.count(ch)) break; NewtonTreeCollisionEndBuild(colls[ch], rd.i()); break; }
        case NT_ReleaseCollision: {
            uint32_t wh = rd.h(), ch = rd.h(); World* w = worldOf(wh);
            if (skip(w) || !colls.count(ch)) break;
            NewtonReleaseCollision(w->w, colls[ch]);
            break;
        }
        case NT_ConvexCollisionCalculateInertialMatrix: {
            uint32_t ch = rd.h(); World* w = worldOf(ch);
            float in[3], org[3]; rd.v3(in); rd.v3(org);
            if (skip(w) || !colls.count(ch)) break;
            float si[4], so[4];
            NewtonConvexCollisionCalculateInertialMatrix(colls[ch], si, so);
            float e = 0;
            for (int k = 0; k < 3; k++) e = std::max(e, std::fabs(si[k] - in[k]) / std::max(1e-9f, std::fabs(in[k])));
            note(w, "unit inertia (relative)", r.index, e, kTolInertia, "");
            note(w, "inertia origin", r.index, dist3(so, org), kTolPos, "m");
            break;
        }
        case NT_CreateBody: {
            uint32_t wh = rd.h(), ch = rd.h(), bh = rd.h(); World* w = worldOf(wh);
            owner[bh] = wh;
            if (skip(w) || !colls.count(ch)) break;
            NewtonBody* b = NewtonCreateBody(w->w, colls[ch]);
            bodies[bh] = b; bodyHandle[b] = bh;
            break;
        }
        case NT_DestroyBody: {
            uint32_t wh = rd.h(), bh = rd.h(); World* w = worldOf(wh);
            if (skip(w) || !bodies.count(bh)) break;
            bodyHandle.erase(bodies[bh]);
            NewtonDestroyBody(w->w, bodies[bh]);
            bodies.erase(bh);
            break;
        }
        case NT_BodySetMatrix: case NT_BodySetVelocity: case NT_BodySetOmega: case NT_BodySetCentreOfMass:
        case NT_BodySetAngularDamping: case NT_BodyAddForce: case NT_BodyAddTorque: {
            uint32_t bh = rd.h(); if (skip(worldOf(bh)) || !bodies.count(bh)) break;
            NewtonBody* b = bodies[bh];
            float m[16];
            if (r.op == NT_BodySetMatrix) { rd.M(m); NewtonBodySetMatrix(b, m); break; }
            rd.v3(m);
            if (r.op == NT_BodySetVelocity) NewtonBodySetVelocity(b, m);
            else if (r.op == NT_BodySetOmega) NewtonBodySetOmega(b, m);
            else if (r.op == NT_BodySetCentreOfMass) NewtonBodySetCentreOfMass(b, m);
            else if (r.op == NT_BodySetAngularDamping) NewtonBodySetAngularDamping(b, m);
            else if (r.op == NT_BodyAddForce) NewtonBodyAddForce(b, m);   // outside a callback: Newton ignores it too
            else NewtonBodyAddTorque(b, m);
            break;
        }
        case NT_BodyGetMatrix: case NT_BodyGetVelocity: case NT_BodyGetOmega: {
            uint32_t bh = rd.h(); World* w = worldOf(bh); if (skip(w) || !bodies.count(bh)) break;
            float want[16], got[16];
            if (r.op == NT_BodyGetMatrix) {
                rd.M(want); NewtonBodyGetMatrix(bodies[bh], got);
                float pe = dist3(want + 12, got + 12), re = rotErr(want, got);
                note(w, "GetMatrix position", r.index, pe, kTolPos, "m"); note(w, "GetMatrix rotation", r.index, re, kTolRot, "deg");
                bucketPose(w, pe, re);
            } else if (r.op == NT_BodyGetVelocity) {
                rd.v3(want); NewtonBodyGetVelocity(bodies[bh], got);
                float e = dist3(want, got); note(w, "GetVelocity", r.index, e, kTolVel, "m/s"); bucketVel(w, e, 0);
            } else {
                rd.v3(want); NewtonBodyGetOmega(bodies[bh], got);
                float e = dist3(want, got); note(w, "GetOmega", r.index, e, kTolOmega, "rad/s"); bucketVel(w, 0, e);
            }
            break;
        }
        case NT_BodySetMassMatrix: {
            uint32_t bh = rd.h(); if (skip(worldOf(bh)) || !bodies.count(bh)) break;
            float m = rd.f(), x = rd.f(), y = rd.f(), z = rd.f();
            NewtonBodySetMassMatrix(bodies[bh], m, x, y, z);
            break;
        }
        case NT_BodyGetMassMatrix: {
            uint32_t bh = rd.h(); World* w = worldOf(bh); if (skip(w) || !bodies.count(bh)) break;
            float want[4], got[4]; for (int k = 0; k < 4; k++) want[k] = rd.f();
            NewtonBodyGetMassMatrix(bodies[bh], &got[0], &got[1], &got[2], &got[3]);
            float e = 0; for (int k = 0; k < 4; k++) e = std::max(e, std::fabs(got[k] - want[k]) / std::max(1e-9f, std::fabs(want[k])));
            note(w, "GetMassMatrix (relative)", r.index, e, 1e-4f, "");
            break;
        }
        case NT_BodySetMaterialGroupID: { uint32_t bh = rd.h(); if (skip(worldOf(bh)) || !bodies.count(bh)) break; NewtonBodySetMaterialGroupID(bodies[bh], rd.i()); break; }
        case NT_BodySetFreezeTreshold: {
            uint32_t bh = rd.h(); if (skip(worldOf(bh)) || !bodies.count(bh)) break;
            float s = rd.f(), o = rd.f(); NewtonBodySetFreezeTreshold(bodies[bh], s, o, rd.i()); break;
        }
        case NT_BodyGetSleepingState: {
            uint32_t bh = rd.h(); World* w = worldOf(bh); if (skip(w) || !bodies.count(bh)) break;
            int want = rd.i(), got = NewtonBodyGetSleepingState(bodies[bh]);
            w->sleepCmp++;
            if ((want != 0) != (got != 0)) w->sleepMismatch++;
            break;
        }
        case NT_CreateRagDoll: {
            uint32_t wh = rd.h(), rh = rd.h(); World* w = worldOf(wh);
            owner[rh] = wh;
            if (skip(w)) break;
            rds[rh] = NewtonCreateRagDoll(w->w);
            break;
        }
        case NT_RagDollBegin: { uint32_t rh = rd.h(); if (skip(worldOf(rh)) || !rds.count(rh)) break; NewtonRagDollBegin(rds[rh]); break; }
        case NT_RagDollEnd: { uint32_t rh = rd.h(); if (skip(worldOf(rh)) || !rds.count(rh)) break; NewtonRagDollEnd(rds[rh]); break; }
        case NT_RagDollSetForceAndTorqueCallback: {
            uint32_t rh = rd.h(); uint32_t cb = rd.h(); if (skip(worldOf(rh)) || !rds.count(rh)) break;
            NewtonRagDollSetForceAndTorqueCallback(rds[rh], cb ? forceCallback : nullptr);
            break;
        }
        case NT_RagDollAddBone: {
            uint32_t rh = rd.h(), ph = rd.h(); rd.h(); float mass = rd.f(), m[16]; rd.M(m); uint32_t ch = rd.h();
            float size[3]; rd.v3(size); uint32_t boneH = rd.h();
            owner[boneH] = owner.count(rh) ? owner[rh] : 0;
            World* w = worldOf(rh);
            if (w) { w->bones++; if (!w->root) w->root = boneH; if (mass >= 50.0f) w->horse = true; }
            if (skip(w) || !rds.count(rh) || !colls.count(ch)) break;
            NewtonRagDollBone* b = NewtonRagDollAddBone(rds[rh], ph ? bones[ph] : nullptr, nullptr, mass, m, colls[ch], size);
            bones[boneH] = b;
            break;
        }
        case NT_ProbeBoneInit: {
            uint32_t boneH = rd.h(), bh = rd.h(); World* w = worldOf(boneH);
            owner[bh] = owner.count(boneH) ? owner[boneH] : 0;
            if (w && w->root == boneH) w->root = bh;
            if (skip(w) || !bones.count(boneH)) break;
            NewtonBody* b = NewtonRagDollBoneGetBody(bones[boneH]);
            bodies[bh] = b; bodyHandle[b] = bh;
            w->ragdollBodies++;
            w->ragBodies.push_back(bh);
            float mm[4], com[3], lm[16], gm[16], got[16];
            for (int k = 0; k < 4; k++) mm[k] = rd.f();
            rd.v3(com); rd.M(lm); rd.M(gm);
            NewtonRagDollBoneGetGlobalMatrix(bones[boneH], got);
            float pe = dist3(gm + 12, got + 12), re = rotErr(gm, got);
            w->createPos = std::max(w->createPos, pe); w->createRot = std::max(w->createRot, re);
            note(w, "bone creation pose position", r.index, pe, kTolPos, "m");
            note(w, "bone creation pose rotation", r.index, re, kTolRot, "deg");
            break;
        }
        case NT_RagDollBoneGetBody: {
            uint32_t boneH = rd.h(), bh = rd.h();
            owner[bh] = owner.count(boneH) ? owner[boneH] : 0;
            if (skip(worldOf(boneH)) || !bones.count(boneH)) break;
            NewtonBody* b = NewtonRagDollBoneGetBody(bones[boneH]);
            if (!bodies.count(bh)) { bodies[bh] = b; bodyHandle[b] = bh; }
            break;
        }
        case NT_RagDollBoneGetGlobalMatrix: {
            uint32_t boneH = rd.h(); World* w = worldOf(boneH); if (skip(w) || !bones.count(boneH)) break;
            float want[16], got[16]; rd.M(want);
            NewtonRagDollBoneGetGlobalMatrix(bones[boneH], got);
            if (!finite16(got)) w->nan = true;
            float pe = dist3(want + 12, got + 12), re = rotErr(want, got);
            note(w, "bone GetGlobalMatrix position", r.index, pe, kTolPos, "m");
            note(w, "bone GetGlobalMatrix rotation", r.index, re, kTolRot, "deg");
            bucketPose(w, pe, re);
            break;
        }
        case NT_RagDollBoneSetLimits: {
            uint32_t boneH = rd.h(); if (skip(worldOf(boneH)) || !bones.count(boneH)) break;
            float cd[3], bd[3]; rd.v3(cd); float mn = rd.f(), mx = rd.f(), tw = rd.f(); rd.v3(bd); float a = rd.f(), b = rd.f();
            NewtonRagDollBoneSetLimits(bones[boneH], cd, mn, mx, tw, bd, a, b);
            break;
        }
        case NT_Update: {
            uint32_t wh = rd.h(); float dt = rd.f(); World* w = worldOf(wh);
            if (skip(w) || !w->w) break;
            // the recorded callbacks of this step: forces per body; consumed here
            pendingForces.clear();
            calledBack.clear();
            std::vector<uint32_t> newtonCalled;
            size_t k = ri + 1;
            for (; k < recs.size(); k++) {
                const Rec& q = recs[k];
                Reader qr(q);
                if (q.op == NT_UpdateEnd && qr.h() == wh) break;
                if (q.op == NT_CallbackBegin) { newtonCalled.push_back(qr.h()); consumed[k] = true; }
                else if (q.op == NT_CallbackEnd || q.op == NT_BodyGetMassMatrix) consumed[k] = true;
                else if (q.op == NT_BodyAddForce || q.op == NT_BodyAddTorque) {
                    uint32_t bh = qr.h(); std::vector<float> v(3); qr.v3(v.data());
                    pendingForces[bh].push_back({q.op, v});
                    consumed[k] = true;
                }
            }
            NewtonUpdate(w->w, dt);
            w->t += dt; w->steps++;
            std::sort(calledBack.begin(), calledBack.end());
            for (uint32_t h : newtonCalled)
                if (!std::binary_search(calledBack.begin(), calledBack.end(), h)) w->cbMissing++;
            break;
        }
        case NT_UpdateEnd: break;
        case NT_ProbeBody: {
            uint32_t wh = rd.h(), bh = rd.h(); World* w = worldOf(wh);
            if (skip(w) || !bodies.count(bh)) break;
            // GetSleepingState in Newton 1.53 is 1 while the body is AWAKE, 0 once it sleeps (traced: every bone reads 1
            // while falling and 0 after settling); the shim follows that
            float m[16], v[3], o[3]; rd.M(m); rd.v3(v); rd.v3(o); int awake = rd.i();
            NewtonBody* b = bodies[bh];
            float gm[16], gv[3], go[3];
            NewtonBodyGetMatrix(b, gm); NewtonBodyGetVelocity(b, gv); NewtonBodyGetOmega(b, go);
            int gs = NewtonBodyGetSleepingState(b);
            if (!finite16(gm)) w->nan = true;
            bool rag = w->isRag(bh);
            if (rag) {
                float pe = dist3(m + 12, gm + 12), re = rotErr(m, gm), ve = dist3(v, gv), oe = dist3(o, go);
                note(w, "probe bone position", r.index, pe, kTolPos, "m");
                note(w, "probe bone rotation", r.index, re, kTolRot, "deg");
                // the bone's own axis (row 0 = along the skeleton bone) alone: twist about it is excluded
                float ca = std::max(-1.0f, std::min(1.0f, m[0] * gm[0] + m[1] * gm[1] + m[2] * gm[2]));
                samples["probe bone axis direction (deg, info)"].push_back(std::acos(ca) * 57.29578f);
                perOp["probe bone axis direction (deg, info)"].second =
                    std::max(perOp["probe bone axis direction (deg, info)"].second, (double)(std::acos(ca) * 57.29578f));
                note(w, "probe bone velocity", r.index, ve, kTolVel, "m/s");
                note(w, "probe bone omega", r.index, oe, kTolOmega, "rad/s");
                bucketPose(w, pe, re); bucketVel(w, ve, oe);
                if (w->steps <= detail)
                    printf("  w%d step %d body %08x: |dp| %.4f rot %.2f deg |dv| %.3f |dw| %.3f  newton |v| %.3f |w| %.3f  shim |v| %.3f |w| %.3f\n",
                           w->id, w->steps, bh, pe, re, ve, oe, len3(v), len3(o), len3(gv), len3(go));
                w->sleepCmp++;
                if ((awake != 0) != (gs != 0)) w->sleepMismatch++;
                w->lastPose[bh] = {std::vector<float>(m, m + 16), std::vector<float>(gm, gm + 16)};
                if (steps && bh == w->root)
                    printf("  w%-3d step %4d t=%6.3f root newton (%7.3f %7.3f %7.3f) shim (%7.3f %7.3f %7.3f) |dp| %.3f rot %.1f v %.2f/%.2f sleep %d/%d\n",
                           w->id, w->steps, w->t, m[12], m[13], m[14], gm[12], gm[13], gm[14], pe, re, len3(v), len3(gv), awake, gs);
            }
            if (sync) { NewtonBodySetMatrix(b, m); NewtonBodySetVelocity(b, v); NewtonBodySetOmega(b, o); }
            // settle: the time from which Newton (shim) has every ragdoll bone asleep (GetSleepingState == 0)
            if (rag) {
                // evaluated when the last probe of the step arrives
                bool lastOfStep = ri + 1 >= recs.size() || recs[ri + 1].op != NT_ProbeBody;
                static int awakeN = 0, awakeS = 0;
                if (awake) awakeN++;
                if (gs) awakeS++;
                if (lastOfStep) {
                    if (!awakeN && w->newtonSettle < 0) w->newtonSettle = w->t;
                    if (awakeN) w->newtonSettle = -1;
                    if (!awakeS && w->shimSettle < 0) w->shimSettle = w->t;
                    if (awakeS) w->shimSettle = -1;
                    awakeN = awakeS = 0;
                }
            }
            break;
        }
        default: break;   // probes not compared, callback markers outside an update
        }
    }
}

int Replay::report()
{
    int fails = 0, shown = 0;
    printf("tolerances: position %.3g m, rotation %.3g deg, velocity %.3g m/s, omega %.3g rad/s, inertia %.0f %%, AABB %.3g m\n",
           kTolPos, kTolRot, kTolVel, kTolOmega, kTolInertia * 100, kTolAabb);
    printf("\nper world (# = instance; R = ragdoll bones; H = horse; M = mount capsule present):\n");
    printf("%-4s %-5s %5s %7s %9s %9s %10s %10s %8s %8s  %s\n", "#", "kind", "steps", "create", "final root", "mean bone",
           "settle N", "settle S", "sleep!=", "cb +/-", "first divergence");
    for (World& w : all) {
        if (onlyWorld >= 0 && w.id != onlyWorld) continue;
        if (!w.steps) continue;
        bool ok = w.ok();
        fails += !ok;
        char sn[16], ss[16];
        snprintf(sn, sizeof sn, w.newtonSettle < 0 ? "never" : "%.2fs", w.newtonSettle);
        snprintf(ss, sizeof ss, w.shimSettle < 0 ? "never" : "%.2fs", w.shimSettle);
        char kind[8]; snprintf(kind, sizeof kind, "R%d%s", w.bones, w.horse ? "H" : "");
        if (!quiet || !ok || shown < 60) {
            printf("%-4d %-5s %5d %6.3fm %8.3fm %8.3fm %10s %10s %3ld/%-4ld %3ld/%-3ld %s%s\n", w.id, kind, w.steps,
                   w.createPos, w.rootErr, w.meanErr, sn, ss, w.sleepMismatch, w.sleepCmp, w.cbExtra, w.cbMissing,
                   ok ? "" : "FAIL ", w.first.empty() ? "-" : w.first.c_str());
            shown++;
        }
    }
    printf("\nmax error by time since the ragdoll started (all worlds):\n%-7s %9s %9s %9s %9s\n", "bucket", "pos m", "rot deg",
           "vel m/s", "omega");
    for (int b = 0; b < kNB; b++)
        printf("%-7s %9.3f %9.2f %9.3f %9.3f\n", kBucketNames[b], global[b].pos, global[b].rot, global[b].vel, global[b].omega);
    printf("\nper comparison: count, beyond tolerance, median, 90th percentile, 99th, max\n");
    for (auto& kv : perOp) {
        std::vector<float>& v = samples[kv.first];
        std::sort(v.begin(), v.end());
        auto pc = [&](double q) { return v.empty() ? 0.0f : v[std::min(v.size() - 1, (size_t)(q * v.size()))]; };
        printf("  %-30s %8zu %8ld  %9.4g %9.4g %9.4g %9.4g\n", kv.first.c_str(), v.size(), kv.second.first, pc(0.5), pc(0.9),
               pc(0.99), kv.second.second);
    }
    printf("\n%ld comparisons, %ld beyond tolerance; %d world(s) fail the qualitative check\n", cmpAll, divAll, fails);
    return fails;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s TRACE [--world N] [--sync] [--steps] [--quiet]\n", argv[0]);
        return 2;
    }
    Replay rp;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--world") && i + 1 < argc) rp.onlyWorld = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--sync")) rp.sync = true;
        else if (!strcmp(argv[i], "--steps")) rp.steps = true;
        else if (!strcmp(argv[i], "--quiet")) rp.quiet = true;
        else if (!strcmp(argv[i], "--detail") && i + 1 < argc) rp.detail = atoi(argv[++i]);
    }
    if (!rp.load(argv[1])) { fprintf(stderr, "cannot read trace %s\n", argv[1]); return 2; }
    printf("%s: %zu records%s\n", argv[1], rp.recs.size(), rp.sync ? " (sync mode: bodies reset to Newton's state after every step)" : "");
    rp.run();
    return rp.report() ? 1 : 0;
}
