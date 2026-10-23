// phys_set.h: the Newton physics set (PhysSet: a table of 0x38-byte world records: ragdoll, static tree
// collision, extra bodies) of phys_set.cpp (0x441360..0x441ff0), the Newton Game Dynamics 1.x imports it calls
// (cdecl, newton.dll; the IAT slot of each is cited) and the collision-shape layouts Agent::build_static_collision
// walks. Offsets are the ones read by a function of this module (cited). Names provisional.
// The members of PhysSet in other objects (create_world 0x44fad0, rebuild_static_body 0x43a900) are only declared.
#ifndef AGENT_PHYS_SET_H
#define AGENT_PHYS_SET_H
#include <stddef.h>
#include <vector>
#include "rgl.h"
#include "./agent.h"

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
__declspec(dllimport) void __cdecl NewtonDestroy(const NewtonWorld* w);                                          // 0x549268
__declspec(dllimport) NewtonBody* __cdecl NewtonCreateBody(const NewtonWorld* w, const NewtonCollision* c);     // 0x549270
__declspec(dllimport) void __cdecl NewtonReleaseCollision(const NewtonWorld* w, const NewtonCollision* c);       // 0x549274
__declspec(dllimport) void __cdecl NewtonBodySetMatrix(const NewtonBody* b, const float* m);                      // 0x549278
__declspec(dllimport) void __cdecl NewtonTreeCollisionAddFace(const NewtonCollision* c, int count, const float* verts, int stride, int attr);   // 0x54927c
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);                  // 0x549294
__declspec(dllimport) void __cdecl NewtonConvexCollisionCalculateInertialMatrix(const NewtonCollision* c, float* inertia, float* origin);   // 0x5492a4
}

struct PhysRec {                                  // 0x38 bytes
    int f0;
    const NewtonWorld* world;                     // +4 (0x441360, 0x4413e0)
    const NewtonRagDoll* ragdoll;                 // +8
    std::vector<const NewtonRagDollBone*> bones;  // +0xc (first at +0x10, last at +0x14)
    std::vector<const NewtonBody*> bodies;        // +0x1c (first at +0x20, last at +0x24)
    const NewtonCollision* collision;             // +0x2c (0x441610, 0x4417d0)
    const NewtonBody* body;                       // +0x30
    int material;                                 // +0x34
};
void push_back_body(std::vector<const NewtonBody*>& v, const NewtonBody* const& b);   // 0x440b30 (folded vector<int>::push_back)

struct PhysSet {
    std::vector<PhysRec> recs;                    // +0 (first at +4)
    char pad10[0x3c - 0x10];
    int nworlds;                                  // +0x3c (0x441360)
    void destroy_world(int idx);                  // 0x441360
    const NewtonRagDollBone* add_bone(int idx, const NewtonRagDollBone* parent, const float* mass, matrix m0,
                                      const float* radius, const float* height, matrix m1, const float* cone_dir,
                                      const float* lim_min, const float* lim_max, const float* lim_twist);   // 0x4413e0 (ret 0xa4)
    void begin_static_build(int idx);             // 0x441610
    NewtonBody* add_capsule_body(int idx, const float* radius, const float* height, const float* mass, matrix* m0, matrix* m1, bool list);   // 0x4416c0 (ret 0x1c)
    void rebuild_static_body(int idx);            // 0x43a900 (earlier object)
};
extern PhysSet g_phys;                            // 0xa181f8

// ---- collision shapes (vtables 0x54a540 capsule, 0x54a588 polygon face; layouts of the shape objects as in the
// shapes' own objects, 0x40c670 / 0x433310): only the fields build_static_collision reads
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base();
    virtual int shape_type();                           // slot 1: 1 = capsule, 2 = polygon face
};
struct capsule_shape : shape_base {                     // vtable 0x54a540, size 0x64
    vector4 p0, p1, p2, p3; float radius;
};
struct face_shape : shape_base {                        // vtable 0x54a588, size 0xb4
    vector4 v[8];                                       // +0x20
    vector4 normal;                                     // +0xa0
    int count;                                          // +0xb0
};
#include "../../include/mat4.h"
using rgl::mat3;
mat3* __cdecl identity3(mat3* out);                     // 0x40cd60

#endif // AGENT_PHYS_SET_H
