# 011 Physics: replace Newton 1.x behind a `phys::` interface

Area: physics | Effort: L-XL | Impact: very high, highest gameplay risk | Depends on: 004, 006 (soft)

## Summary

Newton 1.x is a closed-source 32-bit Windows DLL (`Newton.dll`, 446 KB). A portable 64-bit threaded engine needs an open replacement. The API surface actually used is
small (41 functions) and the game uses physics for a limited set of things, which makes a replacement realistic; the cost is gameplay feel.

## Evidence

- Imports (41): world (`Create, Destroy, Update, SetSolverModel, SetWorldSize`), materials (`MaterialGetDefaultGroupID, SetDefaultSoftness/Elasticity/Friction` - only the *default* group),
  bodies (`CreateBody, DestroyBody, Set/GetMatrix, Set/GetVelocity, Set/GetOmega, AddForce, AddTorque, SetMassMatrix, GetMassMatrix, SetCentreOfMass, SetMaterialGroupID, SetAngularDamping,
  SetFreezeTreshold, GetSleepingState`), shapes (`CreateCapsule, CreateTreeCollision + Begin/AddFace/End, ReleaseCollision, ConvexCollisionCalculateInertialMatrix`), **ragdolls**
  (`CreateRagDoll, RagDollBegin/AddBone/End, BoneSetLimits, BoneGetBody, BoneGetGlobalMatrix, SetForceAndTorqueCallback`).
- Usage: ~254 `Newton*` tokens in 13 files; `NewtonBody` 53, `NewtonRagDollBone` 27+13, `NewtonWorld` 24, `NewtonCollision` 18 (counted in src/). Only capsules and tree (triangle-soup) collisions are created: no boxes,
  spheres, convex hulls, compound shapes, joints other than ragdolls, callbacks other than the ragdoll force callback. Ragdoll bone layout is data-driven by `Data/skeleton_bodies.xml` (read at src/game/mission5/game_2.cpp:969).
- Stepping: `PhysSet::update(dt)` 0x472dc0 (terrain_b.cpp:1310): step 0.018 s for one world, 0.021 / 0.024 / 0.026 for 2/3/4+ worlds, x1.5 when `dt > 0.075`; loops `NewtonUpdate(world, s)` over `recs[i].world`.
  It is the work the existing worker thread runs (`loader_thread_body` 0x4c8900, ticket 012).
- Where bodies are driven: agent4/agent_logic.cpp:3796 (ragdoll bone matrices), 4820-4835 (mounts), agent2/agent_stats.cpp:1307, agent/phys_set.cpp:94.

## Options

| option | pros | cons |
|---|---|---|
| Jolt Physics (MIT) | multithreaded job system, capsules, mesh shapes, ragdoll (skeleton + constraints), 64-bit, stable, active | feel differs from Newton 1.x (solver, damping, freeze threshold); API is C++; learn-by-tuning |
| Newton Dynamics 3.x/4.x (zlib) | closest lineage | ragdoll API gone (`dCustomRagdoll`), different threading, less maintained |
| Bullet/PhysX | common | heavier, no advantage |
| Keep Newton 1.x binary | zero behaviour change | no macOS arm64/64-bit build exists to my knowledge (unverified: check whether a Linux build of 1.53 existed); not an option for the stated goals |

Recommendation: **Jolt**, wrapped by a `phys::` header mirroring the 41 functions one-to-one first (so game code is unchanged), then simplified.

## Detailed plan

1. `port/phys.h` with `NewtonWorld*`-shaped opaque handles; `port/phys_jolt.cpp` implements them 1:1: `NewtonCreateCapsule` -> `CapsuleShape`; tree collision -> `MeshShape` built from `AddFace`s;
   `NewtonBodySet*` -> `BodyInterface`; ragdoll -> `Ragdoll` with `SwingTwistConstraint`s from `BoneSetLimits`.
2. Materials: one default material (softness/elasticity/friction); map to Jolt friction/restitution.
3. Stepping: keep `PhysSet::update`'s fixed substeps (0.018-0.026 s) initially; later one global fixed step.
4. Tuning harness: record body states in a scripted battle on the CrossOver build (`NewtonBodyGetMatrix` dumps through a patch) and compare trajectories qualitatively (fall time, ragdoll settle, mount stop distance).
5. Threading: Jolt's `JobSystemThreadPool` provides the multithreading for free (012).
6. Determinism: not required (no multiplayer in 1.011).

## Risks

- Combat feel: agent capsules are pushed/stopped by physics; mounts and charges depend on it. Needs playtesting; keep tuning values in config.
- Ragdoll stability with Native skeletons (limits are authored for Newton's joint model).
- Tree collision of large scenes (terrain + props): build time at scene load (013).

## Acceptance

Agents stand, walk, ride, collide; dead agents ragdoll plausibly; missiles collide with scene; frame step cost not above Newton's on the same hardware; no `Newton` symbol in the link.

## Effort

L-XL (8-16 wk including tuning). Estimate only; the narrow API is the reason it is not larger.
