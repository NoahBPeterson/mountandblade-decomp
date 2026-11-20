/* newton_trace.h: the binary format of a Newton 1.x call trace (written by proxy/newton_proxy.c, a logging Newton.dll
 * for the original 32-bit game; read by tests/trace_replay.cpp and trace/ntrace.py). C and C++.
 *
 * File: "NWTR" u32 version, then records. Little-endian, packed.
 * Record: u8 op, u8 thread (0 = the first thread that called Newton, 1 = any other), u16 payload bytes, payload.
 * The payload of op N is the fields of kNtSpec[N], in order:
 *   h  u32 handle (the 32-bit pointer value; 0 = NULL)     i  i32        f  f32
 *   3  3 x f32 (a NULL pointer is logged as 3 NaNs)          M  16 x f32 (a Newton row-major matrix)
 *   m  u8 present, then 16 x f32 when present                F  i32 n, then n x (3 x f32) (a face, stride applied)
 * Fields before '|' are the call's inputs, after it its outputs (the return value first, then out-parameters).
 * Probe records (op >= NT_PROBE_FIRST) are not game calls: the proxy asked the real DLL for state the game does not read
 * (bone poses after every step, AddBone's defaults, a capsule's AABB) so that replay can compare it. */
#ifndef NEWTON_TRACE_H
#define NEWTON_TRACE_H

#define NT_MAGIC "NWTR"
#define NT_VERSION 1

enum NtOp {
    NT_Create = 1,                 /* (malloc, free) -> world */
    NT_Destroy,
    NT_Update,                     /* logged before the real call: callbacks and their Add* follow, then NT_UpdateEnd */
    NT_SetSolverModel,
    NT_SetWorldSize,
    NT_MaterialGetDefaultGroupID,
    NT_MaterialSetDefaultSoftness,
    NT_MaterialSetDefaultElasticity,
    NT_MaterialSetDefaultFriction,
    NT_CreateCapsule,
    NT_CreateTreeCollision,
    NT_TreeCollisionBeginBuild,
    NT_TreeCollisionAddFace,
    NT_TreeCollisionEndBuild,
    NT_ReleaseCollision,
    NT_ConvexCollisionCalculateInertialMatrix,
    NT_CreateBody,
    NT_DestroyBody,
    NT_BodySetMatrix,
    NT_BodyGetMatrix,
    NT_BodySetVelocity,
    NT_BodyGetVelocity,
    NT_BodySetOmega,
    NT_BodyGetOmega,
    NT_BodyAddForce,
    NT_BodyAddTorque,
    NT_BodySetMassMatrix,
    NT_BodyGetMassMatrix,
    NT_BodySetCentreOfMass,
    NT_BodySetMaterialGroupID,
    NT_BodySetAngularDamping,
    NT_BodySetFreezeTreshold,
    NT_BodyGetSleepingState,
    NT_CreateRagDoll,
    NT_RagDollBegin,
    NT_RagDollEnd,
    NT_RagDollSetForceAndTorqueCallback,
    NT_RagDollAddBone,
    NT_RagDollBoneGetBody,
    NT_RagDollBoneGetGlobalMatrix,
    NT_RagDollBoneSetLimits,
    NT_CALL_COUNT,
    NT_PROBE_FIRST = 64,
    NT_CallbackBegin = 64,         /* the real DLL called the game's force callback for body */
    NT_CallbackEnd,
    NT_UpdateEnd,                  /* world, milliseconds since the trace started; NT_ProbeBody records follow */
    NT_ProbeBody,                  /* after every NewtonUpdate, every body of the world */
    NT_ProbeBoneInit,              /* after AddBone, before the game overrides anything */
    NT_ProbeCollision,             /* after CreateCapsule: AABB of the shape at identity, volume */
    NT_ProbeBodyInit,              /* after RagDollEnd (all bones) and CreateBody: damping, freeze, AABB */
    NT_ProbeVersion,               /* after NewtonCreate */
    NT_OP_MAX
};

/* indexed by op; "" = unused */
static const char* const kNtSpec[NT_OP_MAX] = {
    "",
    /* Create */ "hh|h", /* Destroy */ "h|", /* Update */ "hf|",
    /* SetSolverModel */ "hi|", /* SetWorldSize */ "h33|", /* MaterialGetDefaultGroupID */ "h|i",
    /* SetDefaultSoftness */ "hiif|", /* SetDefaultElasticity */ "hiif|", /* SetDefaultFriction */ "hiiff|",
    /* CreateCapsule */ "hffm|h", /* CreateTreeCollision */ "hh|h", /* TreeBeginBuild */ "h|",
    /* TreeAddFace */ "hiiF|", /* TreeEndBuild */ "hi|", /* ReleaseCollision */ "hh|",
    /* CalculateInertialMatrix */ "h|33",
    /* CreateBody */ "hh|h", /* DestroyBody */ "hh|", /* SetMatrix */ "hM|", /* GetMatrix */ "h|M",
    /* SetVelocity */ "h3|", /* GetVelocity */ "h|3", /* SetOmega */ "h3|", /* GetOmega */ "h|3",
    /* AddForce */ "h3|", /* AddTorque */ "h3|", /* SetMassMatrix */ "hffff|", /* GetMassMatrix */ "h|ffff",
    /* SetCentreOfMass */ "h3|", /* SetMaterialGroupID */ "hi|", /* SetAngularDamping */ "h3|",
    /* SetFreezeTreshold */ "hffi|", /* GetSleepingState */ "h|i",
    /* CreateRagDoll */ "h|h", /* RagDollBegin */ "h|", /* RagDollEnd */ "h|", /* SetForceAndTorqueCallback */ "hh|",
    /* AddBone */ "hhhfMh3|h", /* BoneGetBody */ "h|h", /* BoneGetGlobalMatrix */ "h|M",
    /* BoneSetLimits */ "h3fff3ff|",
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 42..63 */
    /* CallbackBegin */ "h|", /* CallbackEnd */ "h|", /* UpdateEnd */ "hi|",
    /* ProbeBody: world body | matrix velocity omega sleeping */ "hh|M33i",
    /* ProbeBoneInit: bone body | mass Ixx Iyy Izz, com, local matrix, global matrix */ "hh|ffff3MM",
    /* ProbeCollision: coll | aabb min, max (identity matrix), volume */ "h|33f",
    /* ProbeBodyInit: body | linear damping, angular damping, freeze speed2 omega2 frames(raw), autofreeze, aabb,
       invmass (invM invIxx invIyy invIzz), com, joint recursive collision */ "h|f3ffii33ffff3i",
    /* ProbeVersion */ "h|i",
};

#endif
