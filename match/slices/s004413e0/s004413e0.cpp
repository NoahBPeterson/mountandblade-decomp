// Slice s004413e0: Newton physics set (provisional name PhysSet; see s0043a280 / s00440a30): ragdoll bone creation,
// static tree-collision rebuild start and capsule body creation. Newton Game Dynamics 1.x imports (cdecl, via
// newton.dll). Names provisional.
#include <vector>
#include "rgl.h"
using rgl::matrix;

struct NewtonWorld; struct NewtonBody; struct NewtonCollision; struct NewtonRagDoll; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonDestroyBody(const NewtonWorld* w, const NewtonBody* b);                 // 0x549238
__declspec(dllimport) NewtonCollision* __cdecl NewtonCreateTreeCollision(const NewtonWorld* w, int id);          // 0x54923c
__declspec(dllimport) void __cdecl NewtonTreeCollisionBeginBuild(const NewtonCollision* c);                      // 0x549240
__declspec(dllimport) NewtonCollision* __cdecl NewtonCreateCapsule(const NewtonWorld* w, float radius, float height, const float* offset);   // 0x549244
__declspec(dllimport) NewtonRagDollBone* __cdecl NewtonRagDollAddBone(const NewtonRagDoll* rd, const NewtonRagDollBone* parent, void* user, float mass, const float* matrix, const NewtonCollision* shape, const float* size);   // 0x54924c
__declspec(dllimport) void __cdecl NewtonBodySetCentreOfMass(const NewtonBody* b, const float* com);             // 0x549250
__declspec(dllimport) void __cdecl NewtonBodySetMassMatrix(const NewtonBody* b, float mass, float ixx, float iyy, float izz);   // 0x549254
__declspec(dllimport) void __cdecl NewtonBodySetMaterialGroupID(const NewtonBody* b, int id);                    // 0x549258
__declspec(dllimport) void __cdecl NewtonBodySetAngularDamping(const NewtonBody* b, const float* damping);       // 0x54925c
__declspec(dllimport) void __cdecl NewtonBodySetFreezeTreshold(const NewtonBody* b, float speed2, float omega2, int frames);   // 0x549260
__declspec(dllimport) void __cdecl NewtonRagDollBoneSetLimits(const NewtonRagDollBone* bone, const float* cone_dir, float min_cone, float max_cone, float max_twist, const float* bilateral_dir, float neg_angle, float pos_angle);   // 0x549264
__declspec(dllimport) NewtonBody* __cdecl NewtonCreateBody(const NewtonWorld* w, const NewtonCollision* c);     // 0x549270
__declspec(dllimport) void __cdecl NewtonReleaseCollision(const NewtonWorld* w, const NewtonCollision* c);       // 0x549274
__declspec(dllimport) void __cdecl NewtonBodySetMatrix(const NewtonBody* b, const float* m);                      // 0x549278
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);                  // 0x549294
__declspec(dllimport) void __cdecl NewtonConvexCollisionCalculateInertialMatrix(const NewtonCollision* c, float* inertia, float* origin);   // 0x5492a4
}

struct PhysRec {                                  // 0x38 bytes
    int f0;
    const NewtonWorld* world;                     // +4
    const NewtonRagDoll* ragdoll;                 // +8
    std::vector<const NewtonRagDollBone*> bones;  // +0xc (first at +0x10, last at +0x14)
    std::vector<const NewtonBody*> bodies;        // +0x1c (first at +0x20, last at +0x24)
    const NewtonCollision* collision;             // +0x2c
    const NewtonBody* body;                       // +0x30
    int material;                                 // +0x34
};
void push_back_body(std::vector<const NewtonBody*>& v, const NewtonBody* const& b);   // 0x440b30 (folded vector<int>::push_back)
struct PhysSet {
    std::vector<PhysRec> recs;                    // +0 (first at +4)
    char pad10[0x3c - 0x10];
    int nworlds;                                  // +0x3c
    const NewtonRagDollBone* add_bone(int idx, const NewtonRagDollBone* parent, const float* mass, matrix m0,
                                      const float* radius, const float* height, matrix m1, const float* cone_dir,
                                      const float* lim_min, const float* lim_max, const float* lim_twist);   // 0x4413e0 (ret 0xa4)
    void begin_static_build(int idx);             // 0x441610
    NewtonBody* add_capsule_body(int idx, const float* radius, const float* height, const float* mass, matrix* m0, matrix* m1, bool list);   // 0x4416c0 (ret 0x1c)
};

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
