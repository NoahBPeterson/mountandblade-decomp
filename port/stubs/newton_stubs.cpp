// port/stubs/newton_stubs.cpp: Newton Game Dynamics 1.x (Newton.dll) stand-in for the portable build, ticket 004
// milestone 2. The 41 entry points the game imports exist: creators return NULL, setters are no-ops, getters write a
// neutral result (zero vectors, identity matrices, mass 0, "asleep") so a caller never reads garbage. Ticket 011
// replaces this with the phys:: adapter over Jolt.
//
// Newton exports cdecl functions, so a stub's parameter list only has to be ABI-compatible with the callers'
// (src/game/agent4/agent_logic.h, src/game/agent/phys_set.h, src/game/agent2/agent_stats.h, src/game/terrain_b/physics.h,
// src/rgl/xmlcfg/game_util.cpp); the signatures below are Newton 1.53's. No src/ header is included.
#include <string.h>

struct NewtonWorld;
struct NewtonBody;
struct NewtonCollision;
struct NewtonRagDoll;
struct NewtonRagDollBone;

namespace {
void zero3(float* v) { if (v) v[0] = v[1] = v[2] = 0.0f; }
void identity(float* m)
{
    if (!m)
        return;
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}
} // namespace

extern "C" {

// ---- world --------------------------------------------------------------------------------------------------------
NewtonWorld* NewtonCreate(void* malloc_fn, void* free_fn) { return 0; }
void NewtonDestroy(const NewtonWorld* w) {}
void NewtonUpdate(const NewtonWorld* w, float timestep) {}
void NewtonSetSolverModel(const NewtonWorld* w, int model) {}
void NewtonSetWorldSize(const NewtonWorld* w, const float* minPoint, const float* maxPoint) {}

// ---- materials ----------------------------------------------------------------------------------------------------
int  NewtonMaterialGetDefaultGroupID(const NewtonWorld* w) { return 0; }
void NewtonMaterialSetDefaultSoftness(const NewtonWorld* w, int id0, int id1, float value) {}
void NewtonMaterialSetDefaultElasticity(const NewtonWorld* w, int id0, int id1, float elasticCoef) {}
void NewtonMaterialSetDefaultFriction(const NewtonWorld* w, int id0, int id1, float staticFriction, float kineticFriction) {}

// ---- collision ----------------------------------------------------------------------------------------------------
NewtonCollision* NewtonCreateCapsule(const NewtonWorld* w, float radius, float height, const float* offsetMatrix) { return 0; }
NewtonCollision* NewtonCreateTreeCollision(const NewtonWorld* w, int userCallback) { return 0; }
void NewtonTreeCollisionBeginBuild(const NewtonCollision* tree) {}
void NewtonTreeCollisionAddFace(const NewtonCollision* tree, int vertexCount, const float* vertexPtr, int strideInBytes,
                                int faceAttribute) {}
void NewtonTreeCollisionEndBuild(const NewtonCollision* tree, int optimize) {}
void NewtonReleaseCollision(const NewtonWorld* w, const NewtonCollision* collision) {}
void NewtonConvexCollisionCalculateInertialMatrix(const NewtonCollision* convex, float* inertia, float* origin)
{
    zero3(inertia);
    zero3(origin);
}

// ---- bodies -------------------------------------------------------------------------------------------------------
NewtonBody* NewtonCreateBody(const NewtonWorld* w, const NewtonCollision* collision) { return 0; }
void NewtonDestroyBody(const NewtonWorld* w, const NewtonBody* body) {}
void NewtonBodySetMatrix(const NewtonBody* body, const float* matrix) {}
void NewtonBodyGetMatrix(const NewtonBody* body, float* matrix) { identity(matrix); }
void NewtonBodySetVelocity(const NewtonBody* body, const float* velocity) {}
void NewtonBodyGetVelocity(const NewtonBody* body, float* velocity) { zero3(velocity); }
void NewtonBodySetOmega(const NewtonBody* body, const float* omega) {}
void NewtonBodyGetOmega(const NewtonBody* body, float* omega) { zero3(omega); }
void NewtonBodyAddForce(const NewtonBody* body, const float* force) {}
void NewtonBodyAddTorque(const NewtonBody* body, const float* torque) {}
void NewtonBodySetMassMatrix(const NewtonBody* body, float mass, float Ixx, float Iyy, float Izz) {}
void NewtonBodyGetMassMatrix(const NewtonBody* body, float* mass, float* Ixx, float* Iyy, float* Izz)
{
    if (mass) *mass = 0.0f;
    if (Ixx) *Ixx = 0.0f;
    if (Iyy) *Iyy = 0.0f;
    if (Izz) *Izz = 0.0f;
}
void NewtonBodySetCentreOfMass(const NewtonBody* body, const float* com) {}
void NewtonBodySetMaterialGroupID(const NewtonBody* body, int id) {}
void NewtonBodySetAngularDamping(const NewtonBody* body, const float* angularDamp) {}
void NewtonBodySetFreezeTreshold(const NewtonBody* body, float freezeSpeed2, float freezeOmega2, int framesCount) {}
int  NewtonBodyGetSleepingState(const NewtonBody* body) { return 1; }

// ---- rag dolls ----------------------------------------------------------------------------------------------------
NewtonRagDoll* NewtonCreateRagDoll(const NewtonWorld* w) { return 0; }
void NewtonRagDollBegin(const NewtonRagDoll* ragDoll) {}
void NewtonRagDollEnd(const NewtonRagDoll* ragDoll) {}
void NewtonRagDollSetForceAndTorqueCallback(const NewtonRagDoll* ragDoll, void (*callback)(const NewtonBody*)) {}
NewtonRagDollBone* NewtonRagDollAddBone(const NewtonRagDoll* ragDoll, const NewtonRagDollBone* parent, void* userData,
                                        float mass, const float* matrix, const NewtonCollision* boneCollision,
                                        const float* size) { return 0; }
NewtonBody* NewtonRagDollBoneGetBody(const NewtonRagDollBone* bone) { return 0; }
void NewtonRagDollBoneGetGlobalMatrix(const NewtonRagDollBone* bone, float* matrix) { identity(matrix); }
void NewtonRagDollBoneSetLimits(const NewtonRagDollBone* bone, const float* coneDir, float minConeAngle,
                                float maxConeAngle, float maxTwistAngle, const float* bilateralConeDir,
                                float negativeBilateralConeAngle, float positiveBilateralConeAngle) {}

} // extern "C"
