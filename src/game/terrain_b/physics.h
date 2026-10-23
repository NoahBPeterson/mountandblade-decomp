// physics.h: the Newton physics set (a table of 0x38-byte world records) as far as the ragdoll velocity clamp
// (0x472c50) and the frame step (0x472dc0) go, and the Newton Game Dynamics 1.x imports they call (cdecl,
// newton.dll; IAT slot cited). src/game/agent/phys_set.h declares the rest of the same class and the other imports.
#ifndef TERRAIN_B_PHYSICS_H
#define TERRAIN_B_PHYSICS_H
#include <vector>

struct NewtonWorld; struct NewtonBody; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonUpdate(const NewtonWorld* w, float dt);                                 // 0x549204
__declspec(dllimport) void __cdecl NewtonBodyGetOmega(const NewtonBody* b, float* omega);                        // 0x549208
__declspec(dllimport) void __cdecl NewtonBodyGetVelocity(const NewtonBody* b, float* v);                         // 0x549210
__declspec(dllimport) void __cdecl NewtonBodySetVelocity(const NewtonBody* b, const float* v);                   // 0x549214
__declspec(dllimport) void __cdecl NewtonBodySetOmega(const NewtonBody* b, const float* omega);                  // 0x549218
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);                  // 0x549294
}

#include "../../include/phys_set.h"      // PhysRec, PhysSet (clamp_rec 0x472c50, update 0x472dc0)

#endif // TERRAIN_B_PHYSICS_H
