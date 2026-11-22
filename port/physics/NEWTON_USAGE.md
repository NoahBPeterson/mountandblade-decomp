# How src/ uses Newton 1.x, and the adapter seam

Engine-independent. Sources: src/game/agent4/agent_logic.cpp (PhysSet, Agent::start_ragdoll, per-frame ragdoll
read-back), src/game/common/game_util.cpp (force callback), src/game/terrain/poly_mesh.cpp (PhysSet::update,
clamp_rec), src/engine/physics/phys_set.h (PhysRec). Signatures: port/stubs/newton_stubs.cpp (Newton 1.53's, all cdecl).
Every statement below was read from src/. Items marked (traced) were settled against the real DLL (`Newton.dll` of the
install, `NewtonWorldGetVersion` = 153, built 2006-05-24): call traces of the original game recorded through a logging
proxy and controlled scenes run on the laptop (`trace/RECORDING.md`, `trace/probe/newton_experiments.cpp`, results in
"Traced behaviour of 1.53" at the end). Items still marked (1.x doc, unverified) were not exercised.

## What is used (and what is not)

41 imports. Only: one world per physics record, capsules, tree (triangle-soup) collision, plain bodies, ragdolls.
Not used: boxes, spheres, hulls, compounds, user joints, contact/material callbacks, raycasts, transform callbacks,
user data on bodies (AddBone's userData is 0), NewtonBodySetAutoFreeze, NewtonBodyGetUserData, joint API other than
the ragdoll bone limits.

## Worlds (PhysSet::create_world 0x44fad0, destroy_world 0x441360)

One NewtonWorld per physics record: one per ragdolling agent (`Agent::start_ragdoll`), recs[] reused when freed.
Each world holds at most: one ragdoll (<= 32 bones), one static tree body, 0-1 extra capsule body (the mount) plus
`bodies` (capsules from add_capsule_body(list=true), world-collision capsules). Per world, in order:
`NewtonCreate(0,0)`; `SetSolverModel(w, 6)`; `SetWorldSize(w, {-1000,-1000,-100}, {1000,1000,1000})`;
`material = MaterialGetDefaultGroupID(w)`; default softness 0.1, elasticity 0.5, friction static 1.1 / kinetic 0.9
for (material, material). Only the default group is ever used. destroy_world: `NewtonDestroy(w)` (frees ragdoll,
bones, bodies, collisions; the game drops its handles first and never DestroyBody's the ragdoll bones).

Concurrency: update runs from `PhysSet::update(dt)` (poly_mesh.cpp:700) which may run on the worker thread (ticket
012) while the main thread calls Set/Get on bodies. Newton 1.x gave no guarantee; an adapter should lock per world.

## Stepping (PhysSet::update)

Fixed substeps: step = 0.018 (1 world), 0.021 (2), 0.024 (3), 0.026 (4+), x1.5 if dt > 0.075. While dt > 1e-4:
s = min(dt, step); for each live world: `clamp_rec(i)` then `NewtonUpdate(world, s)`. clamp_rec reads every ragdoll
bone's velocity/omega and clamps |v| to 30 and |omega| to 9 (via Get/SetVelocity/Omega) before each substep. So
NewtonUpdate sees small steps (<= 0.039) and the adapter may use one engine step per call.

## Matrix convention

All 16-float matrices are Newton's: row-major, row vectors (v' = v*M). Rows 0..2 = side/front/up axes (x/y/z),
row 3 = position, column 3 = (0,0,0,1). The game's `rgl::matrix` {s,f,u,o} is byte-compatible (`&m.s.x`). The game
forces w components (0,0,0,1) before passing (add_bone, add_capsule_body). World is **Z-up**: gravity is -9.8 on z.
Rotations are orthonormal and right-handed (s x f = u). Engines with column-vector matrices (Jolt, Newton 3.x
dMatrix is also row-major row-vector, so 3.x is the same layout; Jolt/Bullet need a transpose) convert at the seam.
Units: metres, seconds, radians (limits are converted from degrees by the game: * 0.017453292). Float velocities,
omega are world-space.

## Gravity and forces

The engine has no gravity of its own. `apply_force_and_torque(body)` (game_util.cpp:218, 0x432a50) is installed with
`NewtonRagDollSetForceAndTorqueCallback(ragdoll, cb)` and is called by Newton for each ragdoll bone body (awake only;
in the traced battles every bone was awake in every step) once per NewtonUpdate step, before integration (traced). It does:
`NewtonBodyGetMassMatrix(body,&mass,..)`; default branch: `NewtonBodyAddForce(body, {0,0,-9.8*mass})`; if
g_phys.pending.body == body: force = (pending.force.x*mass, y*mass, (pending.force.z - 9.8)*mass), torque
(pending.torque*mass) when non-zero, then the pending record is cleared (a one-shot impulse-like acceleration from a
hit). Forces/torques are world-space and are cleared by the engine after the step. Only ragdoll bones have a callback:
capsule bodies from NewtonCreateBody (mount body, world capsules) get NO gravity and no force callback; they are
driven entirely by NewtonBodySetMatrix / SetVelocity from game code. The adapter must therefore: set engine gravity
to 0, and call the ragdoll callback per bone body each step (skipping sleeping bodies), with Add* accumulating into a
per-step force that is cleared after the step.

## Collision shapes

* `NewtonCreateCapsule(w, radius, height, offsetMatrix)`: offsetMatrix = capsule frame relative to the body (non-NULL
  always; `m1` in add_bone / add_capsule_body, row0 = capsule axis, i.e. the game builds frame.s = unit(p1-p0), origin =
  segment midpoint, orientation completed with cross products). Newton 1.x capsules lie along the local **X** axis.
  height = |p1-p0| * scale = distance between the end-sphere centres in the game's data model (skeleton_bodies.xml:
  radius + two points); some code paths pass radius 0 / height 0.01 (degenerate; clamp, do not fail).
  (traced) 1.53's `height` is the TOTAL length (cap to cap), and the cylinder part is never shorter than 0.1: cylinder
  half-length = max(height/2 - radius, 0.05). The game's short bones (pelvis, head, hands: height < 2r + 0.1) therefore
  get capsules longer than their xml segment. 3.x `NewtonCreateCapsule(radius0, radius1, height, shapeID, offset)` has
  a cylinder-only height: pass max(height - 2r, 0.1).
* `NewtonCreateTreeCollision(w, 0)` + `TreeCollisionBeginBuild` + N x `TreeCollisionAddFace(tree, count, verts, 12, 0)`
  + `TreeCollisionEndBuild(tree, 0)`: build_static_collision (agent_logic.cpp:2417) queries the scene within +-5 m of
  the agent and adds each face of type 2 (`count` = 3 or 4, vertices tightly packed float xyz, stride 12 bytes; a
  4-vertex face is a quad: fan-triangulate). Rebuilt whenever the agent moves > 3 m from `anchor` (begin_static_build
  destroys the old static body, and `bodies`, first). attribute 0, optimize 0. Tree faces are two-sided solid soup.
  1.x vs 3.x: 3.x `NewtonCreateTreeCollision(world, shapeID)` + `NewtonTreeCollisionBeginBuild/AddFace/EndBuild`
  exist with the same shape (3.x adds `NewtonCollisionSetUserData`; AddFace has extra faceAttribute param, same).
* `NewtonReleaseCollision(w, c)` right after CreateBody/AddBone: reference-counted. **The collision is used after the
  release** (`NewtonConvexCollisionCalculateInertialMatrix(cap, ...)` follows `NewtonReleaseCollision` in add_bone and
  add_capsule_body): the body holds the reference. The adapter must refcount (create = 1, each body +1).
  1.x vs 3.x: 3.x `NewtonDestroyCollision(c)` is the same refcounted release; `NewtonReleaseCollision` does not exist
  in 3.x (it is the 2.x/3.x rename). Provide it as a thin wrapper.
* `NewtonConvexCollisionCalculateInertialMatrix(cap, inertia[3], origin[3])`: returns principal moments and COM of the
  shape. The game passes the result **as-is** (not multiplied by mass) to `NewtonBodySetMassMatrix(body, mass, I[0],
  I[1], I[2])`. (traced) 1.53 returns unit-mass moments about the centre of mass (0.947..0.996 of the exact solid
  capsule's: Newton integrates a polygonal shape), as the DIAGONAL of the tensor in the body frame (the offset matrix's
  rotation applied, not principal moments); origin = the offset translation. The mass-times-too-small inertia does
  not reach the solver: 1.53 clamps each moment to at least mass / 100 when integrating (GetMassMatrix still returns
  what was set), so every game bone runs with I = mass / 100 on all axes. Also `NewtonBodySetCentreOfMass(body,
  origin)` with the same origin.
  1.x vs 3.x: 3.x has `NewtonConvexCollisionCalculateInertialMatrix(collision, float* inertia, float* origin)`,
  same signature, but returns inertia for *unit mass*? (3.x docs: "for a unit mass, density 1"); verify.

## Bodies

* `NewtonCreateBody(w, collision)`: starts at identity, mass 0 (static). Static tree body: matrix set to identity
  (rebuild_static_body), never given mass. Mount/world capsule bodies: `CreateBody`, `SetMatrix(m0)`, then
  CalculateInertia, `SetCentreOfMass(origin)`, `SetMassMatrix(body, mass, Ixx, Iyy, Izz)` (becomes dynamic when mass>0),
  `SetMaterialGroupID(material)`.
* `NewtonBodySetMatrix/GetMatrix` (origin = body frame origin, not COM), `Set/GetVelocity`, `Set/GetOmega` (world),
  `AddForce`, `AddTorque` (callback only), `GetMassMatrix` (mass, Ixx, Iyy, Izz as stored), `SetAngularDamping(body,
  {0.7,0.7,0.7})` (per-axis damping vector, 1.x), `SetFreezeTreshold(body, 0.25, 0.7, 5)` (speed^2, omega^2, frames),
  `GetSleepingState`: (traced) **1 while the body is awake, 0 once frozen**. So `PhysSet::none_sleeping()` (0x43a8a0,
  "false if any bone returns non-zero") really means "every bone asleep"; the end-of-ragdoll logic retires a ragdoll
  1 s after that. In the traced battles no ragdoll ever froze (every bone read 1 in every step); they were retired by
  the game's other timers. (traced) Writes from the game (SetMatrix/SetVelocity/SetOmega) do NOT wake a frozen body:
  SetMatrix still moves it. SetMatrix on the whole ragdoll after End teleports every bone body without moving joints'
  local frames (see ragdoll).
  1.x vs 3.x: 3.x `NewtonBodySetMassMatrix(body, mass, Ixx, Iyy, Izz)` exists; `NewtonBodySetCentreOfMass` and
  `NewtonBodySetFreezeTreshold` exist in 3.x as `NewtonBodySetCentreOfMass` / `NewtonBodySetFreezeState`+
  `SetSleepState`... name check needed; `NewtonBodySetAngularDamping` takes vec3 in 3.x too; 3.x `NewtonBodyGetSleepState`;
  3.x bodies need `NewtonBodySetForceAndTorqueCallback` per body for gravity (1.x ragdoll-level callback maps to it);
  3.x `NewtonBodySetMaterialGroupID` exists. 3.x NewtonUpdate is asynchronous-capable (`NewtonWaitForUpdateToFinish`).
* Tree body in Newton is static and collides with the dynamic bodies; no static-static.
* World size (+-1000, z -100..1000): bodies leaving it are frozen (1.x doc, unverified); unlikely to matter.
* (traced) NewtonUpdate integrates the timestep clamped to [1/1000, 1/60] s: one step, never subdivided, the rest of a
  longer timestep is dropped, a shorter one is stretched to 1 ms (the 1/60 is `NewtonSetMinimumFrameRate`'s default 60, never changed by the game). The game's 0.018..0.039 s
  substeps (PhysSet::update) therefore ran its ragdolls slower than real time. Linear damping defaults to 0.1045,
  angular damping is what the game sets (0.7); both act per step, not per second: v *= 1 - c/50 each update.
* Material: default group only. Softness/elasticity/friction -> engine restitution 0.5, friction ~0.9-1.1
  (static/kinetic). 3.x has `NewtonMaterialSetDefaultSoftness/Elasticity/Friction` with the same arguments, so this is
  a direct pass-through there.

## Ragdoll (the only joints)

`PhysSet` calls (Agent::start_ragdoll, 0x44fc60, then add_bone 0x4413e0 per bone):
1. `r = NewtonCreateRagDoll(w); NewtonRagDollBegin(r)`.
2. Per skeleton bone with capsules (parents before children; physics parent = nearest ancestor having a bone, else NULL
   for the root): `cap = NewtonCreateCapsule(w, radius, height, &m1)`;
   `bone = NewtonRagDollAddBone(r, parent, 0, mass, &m0, cap, size={0,0,0})` where m0 = T, the bone's rest frame
   **relative to its physics parent's** (start_ragdoll accumulates the local matrices of bodiless ancestors; the root's
   is the world frame). (traced) 1.53 composes it: the bone's global matrix right after AddBone is m0 * parent's global
   (row vectors), = A[i] of start_ragdoll. AddBone gives the body the mass and zero inertia; `NewtonReleaseCollision`;
   `body = NewtonRagDollBoneGetBody(bone)`; inertia/COM/mass/material set; `SetAngularDamping(0.7)`;
   `SetFreezeTreshold(0.25, 0.7, 5)`;
   `NewtonRagDollBoneSetLimits(bone, coneDir, minCone, maxCone, maxTwist, coneDir, 0.2, 0.2)`: coneDir = the xml
   socket direction (`socket_dir_*`, default x axis) rotated by the bone's rest frame A[i] (world space at creation,
   rot3v_cone), angles in radians; (traced) the game passes minCone = -0.01 deg for every bone, maxCone 1..70 deg,
   maxTwist 1..45 deg; the bilateral direction is the same vector with 0.2/0.2 rad. (traced, controlled scenes) the
   swing limit is **(maxCone - minCone) / 2** about coneDir (30 -> 15, 70 -> 35; min 20 -> 25; min -20 -> 45), the
   twist limit +-maxTwist; both are soft (1..2 deg past them under moderate torque, up to 26 deg while a ragdoll
   falls, and the in-game swing p90 sits 4..12 deg past the half-cone); the bilateral angles change nothing.
3. After all bones: `NewtonRagDollSetForceAndTorqueCallback(r, apply_force_and_torque)`; per bone
   `SetVelocity/SetOmega` were already issued before this (velocity includes skeleton motion + ancestors' omega x r);
   `NewtonRagDollEnd(r)`; then per bone `NewtonBodySetMatrix(body, &W[i])` (current animated pose, not the rest
   matrix T used to create the joints); then build_static_collision.
4. Each frame the game reads back `NewtonRagDollBoneGetGlobalMatrix(bone, m)` (bone 0 drives agent body->m; the rest
   feed skinning; position sanity-checked |o.x| < 1e6) and, when mounted, Get/SetMatrix and Get/SetVelocity on bone
   bodies (+mount velocity shifts, mount capsule re-posed each frame).
   `NewtonRagDollBoneGetBody` is used pervasively (also in clamp_rec, none_sleeping).
Semantics the adapter must reproduce: bodies are created with matrix m0 and joint frames are fixed in the bodies'
local spaces at creation (so the later teleport to W keeps the joints valid); joint = ball-and-socket at the bone
origin with cone (swing) limit about coneDir and a twist limit; jointed (and, 1.x doc unverified, nearby) bones do not
collide with each other; bones collide with the static tree and the mount capsule.
1.x vs 3.x: 3.x removed the ragdoll API (`NewtonCreateRagDoll`, `RagDollBegin/AddBone/End/BoneSetLimits` are 1.x/2.x
only); it has `dCustomBallAndSocket`/`dCustomRagdoll`-style joints in dgJoints (dCustomJoints library, C++, not the C
API; `CustomBallAndSocket::SetConeAngle/SetTwistAngle` exist in later 3.x; older: `CustomLimitBallAndSocket`
(cone + twist, closest match to RagDollBoneSetLimits)). The adapter builds the ragdoll from NewtonCreateBody +
`CustomLimitBallAndSocket(pivotMatrix, childBody, parentBody)` with SetConeAngle(max) and SetTwistAngle(-twist, +twist);
the minConeAngle and the bilateral cone have no direct 3.x equivalent (the 1.x min cone is a dead zone, bilateral
an extra elliptical limit): approximate by the max cone and note it. 3.x also needs the bodies' own
`NewtonBodySetForceAndTorqueCallback` (gravity) and per-body transform callback is optional (the game polls matrices).

## Adapter seam

Everything above reduces to a small internal interface; the game-facing layer is the 41 extern "C" Newton1 symbols
(identical to port/stubs/newton_stubs.cpp signatures), implemented in terms of:

```
struct PhysBackend {                        // one instance per NewtonWorld
  virtual ~PhysBackend();
  virtual void setSolverIterations(int), setWorldBounds(const float mn[3], const float mx[3]);
  virtual void setMaterial(float restitution, float staticFriction, float kineticFriction);
  virtual Shape* capsule(float radius, float cylinderHeight, const float offset[16]);   // axis +X in offset frame
  virtual Shape* treeBegin(); treeAddFace(Shape*, int n, const float* xyz, int strideBytes); treeEnd(Shape*);
  virtual void shapeInertia(const Shape*, float inertia[3], float comOut[3]);          // unit-mass principal
  virtual Body* createBody(Shape*);                                                    // static until mass > 0
  virtual void destroyBody(Body*);
  virtual void setMatrix/getMatrix(Body*, float m[16]);   virtual void setVelocity/getVelocity/setOmega/getOmega(...);
  virtual void addForce/addTorque(Body*, const float v[3]);   // world space, cleared after step
  virtual void setMass(Body*, float mass, float ixx, float iyy, float izz);  getMass(...)
  virtual void setCentreOfMass(Body*, const float com[3]);
  virtual void setAngularDamping(Body*, const float d[3]); setFreeze(Body*, float v2, float w2, int frames);
  virtual bool isSleeping(Body*);
  virtual Joint* coneTwist(Body* parent, Body* child, const float pivotWorld[3], const float axisWorld[3],
                           float coneHalfAngle, float twistMin, float twistMax);
  virtual void disableCollision(Body*, Body*);
  virtual void step(float dt);   // gravity 0; forces already added by the layer
};
```

The Newton1 layer (backend independent) owns: handles (NewtonWorld/Body/Collision/RagDoll/RagDollBone), collision
refcounting, tree triangle accumulation and quad fan-triangulation, mass/inertia bookkeeping (GetMassMatrix returns
what was set), ragdoll Begin/AddBone/End bookkeeping with joints created at SetLimits from the bodies' creation
poses, the force-and-torque callback loop (awake ragdoll bones only, before each step), sleep policy from the
freeze threshold, and the world-bounds freeze. A Jolt backend, a Newton 3.x backend (zlib, `dNewton` C API plus
dCustomJoints), or a test fake can then plug in without touching the layer. The Newton-3.x backend has the smallest
shape/body mismatch (same matrix layout, row-vector convention, Z/Y-up agnostic, same capsule-along-X); its cost is
building the ragdoll joints in C++ (dCustomJoints) and the 3.x library's different threading/update model.

## Traced behaviour of 1.53 (settled; newton_jolt.cpp follows it)

Sources: a 155 s attract-mode battle of the original game (48 ragdolls incl. a horse and mounted riders, up to 6 at
once; replay with `physics_trace_replay`), and `trace/probe/newton_experiments.cpp` run on the real DLL.

| question | answer |
|---|---|
| capsule `height` | total length; cylinder half = max(h/2 - r, 0.05) |
| CalculateInertialMatrix | unit mass, about the COM, diagonal in the body frame, ~0.963 of the exact solid |
| effective inertia | each moment clamped to >= mass/100 when integrating; GetMassMatrix returns the set values |
| AddBone matrix | parent-relative; global = m0 * parent global |
| GetSleepingState | 1 = awake, 0 = frozen; in battle no bone ever froze |
| frozen body + SetVelocity / SetMatrix | stays frozen (SetMatrix still moves it) |
| step length | dt clamped to [1/1000, 1/60]; never subdivided |
| damping | per step: v *= 1 - 0.1045/50 (default linear), w *= 1 - d/50 (d = SetAngularDamping) |
| swing / twist limit | (maxCone - minCone)/2 / +-maxTwist, soft; bilateral cone has no effect |
| GetGlobalMatrix vs GetMatrix of the bone body | identical |
| restitution 0.5 / friction | a dropped capsule bounces to ~0.23 of its fall height (e ~ 0.48); rests 5 mm in the ground |
| freeze | an isolated resting bone froze at |v| ~0.05, |w| ~0.5 after 238 steps; GetFreezeTreshold reads 16x the set values |
| force callback | only for awake bodies, once per update, before integration |

Still open: bone-bone collision rules in a ragdoll (the replay keeps parent/grandparent/siblings non-colliding);
`NewtonBodyGetSleepingState` of non-ragdoll capsules in battle; the exact soft-limit law (the shim's limits are hard).
