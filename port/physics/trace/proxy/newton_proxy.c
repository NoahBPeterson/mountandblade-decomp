/* newton_proxy.c: a logging Newton.dll for the original 32-bit game (Newton 1.x, cdecl). Exports the 41 functions the
 * game imports, forwards each to the real DLL (LoadLibrary "Newton.dll" from the game directory, so this file is
 * installed under another name and the game's import is pointed at it: see ../RECORDING.md), and appends every call,
 * with its inputs and results, to a binary trace (format: ../newton_trace.h). After every NewtonUpdate it also probes the
 * real DLL for the state of every body of that world (matrix, velocity, omega, sleeping), and once per body for state the
 * game never reads (damping, freeze threshold, AABB, inverse mass), so the shim can be compared with Newton directly.
 *
 * Environment: NEWTON_TRACE = output path (default newton_trace.bin in the current directory), NEWTON_TRACE_MAX_MB =
 * size cap (default 3000; the trace stops growing there, the game is unaffected).
 * Build: ../build_proxy.sh (i686-w64-mingw32-gcc, static libgcc, exports from newton_proxy.def). */
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "../newton_trace.h"

typedef const void* H;
typedef void (*Cb)(H);

/* ---- the real DLL ---------------------------------------------------------------------------------------------------- */
static HMODULE g_real;
#define FN(ret, name, args) static ret (*r_##name) args;
FN(H, NewtonCreate, (H, H))
FN(void, NewtonDestroy, (H))
FN(void, NewtonUpdate, (H, float))
FN(void, NewtonSetSolverModel, (H, int))
FN(void, NewtonSetWorldSize, (H, const float*, const float*))
FN(int, NewtonMaterialGetDefaultGroupID, (H))
FN(void, NewtonMaterialSetDefaultSoftness, (H, int, int, float))
FN(void, NewtonMaterialSetDefaultElasticity, (H, int, int, float))
FN(void, NewtonMaterialSetDefaultFriction, (H, int, int, float, float))
FN(H, NewtonCreateCapsule, (H, float, float, const float*))
FN(H, NewtonCreateTreeCollision, (H, H))
FN(void, NewtonTreeCollisionBeginBuild, (H))
FN(void, NewtonTreeCollisionAddFace, (H, int, const float*, int, int))
FN(void, NewtonTreeCollisionEndBuild, (H, int))
FN(void, NewtonReleaseCollision, (H, H))
FN(void, NewtonConvexCollisionCalculateInertialMatrix, (H, float*, float*))
FN(H, NewtonCreateBody, (H, H))
FN(void, NewtonDestroyBody, (H, H))
FN(void, NewtonBodySetMatrix, (H, const float*))
FN(void, NewtonBodyGetMatrix, (H, float*))
FN(void, NewtonBodySetVelocity, (H, const float*))
FN(void, NewtonBodyGetVelocity, (H, float*))
FN(void, NewtonBodySetOmega, (H, const float*))
FN(void, NewtonBodyGetOmega, (H, float*))
FN(void, NewtonBodyAddForce, (H, const float*))
FN(void, NewtonBodyAddTorque, (H, const float*))
FN(void, NewtonBodySetMassMatrix, (H, float, float, float, float))
FN(void, NewtonBodyGetMassMatrix, (H, float*, float*, float*, float*))
FN(void, NewtonBodySetCentreOfMass, (H, const float*))
FN(void, NewtonBodySetMaterialGroupID, (H, int))
FN(void, NewtonBodySetAngularDamping, (H, const float*))
FN(void, NewtonBodySetFreezeTreshold, (H, float, float, int))
FN(int, NewtonBodyGetSleepingState, (H))
FN(H, NewtonCreateRagDoll, (H))
FN(void, NewtonRagDollBegin, (H))
FN(void, NewtonRagDollEnd, (H))
FN(void, NewtonRagDollSetForceAndTorqueCallback, (H, Cb))
FN(H, NewtonRagDollAddBone, (H, H, H, float, const float*, H, const float*))
FN(H, NewtonRagDollBoneGetBody, (H))
FN(void, NewtonRagDollBoneGetGlobalMatrix, (H, float*))
FN(void, NewtonRagDollBoneSetLimits, (H, const float*, float, float, float, const float*, float, float))
/* probes (read-only getters the game does not import; extra out-pointers are passed where 1.x's arity is unsure) */
FN(float, NewtonBodyGetLinearDamping, (H))
FN(void, NewtonBodyGetAngularDamping, (H, float*))
FN(void, NewtonBodyGetFreezeTreshold, (H, float*, float*, int*))
FN(int, NewtonBodyGetAutoFreeze, (H))
FN(void, NewtonBodyGetAABB, (H, float*, float*))
FN(void, NewtonBodyGetInvMass, (H, float*, float*, float*, float*))
FN(void, NewtonBodyGetCentreOfMass, (H, float*))
FN(int, NewtonBodyGetJointRecursiveCollision, (H))
FN(void, NewtonCollisionCalculateAABB, (H, const float*, float*, float*))
FN(float, NewtonConvexCollisionCalculateVolume, (H))
FN(int, NewtonWorldGetVersion, (H))
FN(void, NewtonRagDollBoneGetLocalMatrix, (H, float*))
#undef FN

static void load_real(void)
{
    if (g_real)
        return;
    g_real = LoadLibraryA("Newton.dll");
    if (!g_real) {
        MessageBoxA(0, "newton proxy: cannot load Newton.dll", "newton proxy", 0);
        ExitProcess(3);
    }
#define L(name) *(FARPROC*)&r_##name = GetProcAddress(g_real, #name);
    L(NewtonCreate) L(NewtonDestroy) L(NewtonUpdate) L(NewtonSetSolverModel) L(NewtonSetWorldSize)
    L(NewtonMaterialGetDefaultGroupID) L(NewtonMaterialSetDefaultSoftness) L(NewtonMaterialSetDefaultElasticity)
    L(NewtonMaterialSetDefaultFriction) L(NewtonCreateCapsule) L(NewtonCreateTreeCollision)
    L(NewtonTreeCollisionBeginBuild) L(NewtonTreeCollisionAddFace) L(NewtonTreeCollisionEndBuild)
    L(NewtonReleaseCollision) L(NewtonConvexCollisionCalculateInertialMatrix) L(NewtonCreateBody)
    L(NewtonDestroyBody) L(NewtonBodySetMatrix) L(NewtonBodyGetMatrix) L(NewtonBodySetVelocity)
    L(NewtonBodyGetVelocity) L(NewtonBodySetOmega) L(NewtonBodyGetOmega) L(NewtonBodyAddForce) L(NewtonBodyAddTorque)
    L(NewtonBodySetMassMatrix) L(NewtonBodyGetMassMatrix) L(NewtonBodySetCentreOfMass) L(NewtonBodySetMaterialGroupID)
    L(NewtonBodySetAngularDamping) L(NewtonBodySetFreezeTreshold) L(NewtonBodyGetSleepingState) L(NewtonCreateRagDoll)
    L(NewtonRagDollBegin) L(NewtonRagDollEnd) L(NewtonRagDollSetForceAndTorqueCallback) L(NewtonRagDollAddBone)
    L(NewtonRagDollBoneGetBody) L(NewtonRagDollBoneGetGlobalMatrix) L(NewtonRagDollBoneSetLimits)
    L(NewtonBodyGetLinearDamping) L(NewtonBodyGetAngularDamping) L(NewtonBodyGetFreezeTreshold)
    L(NewtonBodyGetAutoFreeze) L(NewtonBodyGetAABB) L(NewtonBodyGetInvMass) L(NewtonBodyGetCentreOfMass)
    L(NewtonBodyGetJointRecursiveCollision) L(NewtonCollisionCalculateAABB) L(NewtonConvexCollisionCalculateVolume)
    L(NewtonWorldGetVersion) L(NewtonRagDollBoneGetLocalMatrix)
#undef L
}

/* ---- trace output ---------------------------------------------------------------------------------------------------- */
static CRITICAL_SECTION g_lock;
static HANDLE g_file = INVALID_HANDLE_VALUE;
static unsigned char* g_buf;
static unsigned g_used, g_cap = 4u << 20;
static unsigned long long g_written, g_max = 3000ull << 20;
static DWORD g_t0, g_lastFlush, g_thread0;
static int g_open;

static void flush_locked(void)
{
    DWORD n;
    if (g_used && g_file != INVALID_HANDLE_VALUE)
        WriteFile(g_file, g_buf, g_used, &n, 0);
    g_written += g_used;
    g_used = 0;
}

static void open_trace(void)
{
    char path[MAX_PATH], mb[32];
    if (g_open)
        return;
    g_open = 1;
    if (!GetEnvironmentVariableA("NEWTON_TRACE", path, sizeof path))
        strcpy(path, "newton_trace.bin");
    if (GetEnvironmentVariableA("NEWTON_TRACE_MAX_MB", mb, sizeof mb))
        g_max = (unsigned long long)atoi(mb) << 20;
    g_file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    g_buf = (unsigned char*)HeapAlloc(GetProcessHeap(), 0, g_cap);
    g_t0 = g_lastFlush = GetTickCount();
    memcpy(g_buf, NT_MAGIC, 4);
    { unsigned v = NT_VERSION; memcpy(g_buf + 4, &v, 4); }
    g_used = 8;
}

typedef struct { unsigned char b[1200]; unsigned n; } Rec;

static void r_begin(Rec* r, int op)
{
    DWORD t = GetCurrentThreadId();
    if (!g_thread0)
        InterlockedCompareExchange((LONG*)&g_thread0, (LONG)t, 0);
    r->b[0] = (unsigned char)op;
    r->b[1] = t == g_thread0 ? 0 : 1;
    r->n = 4;
}
static void r_raw(Rec* r, const void* p, unsigned n) { if (r->n + n <= sizeof r->b) { memcpy(r->b + r->n, p, n); r->n += n; } }
static void r_h(Rec* r, H p) { uint32_t v = (uint32_t)(uintptr_t)p; r_raw(r, &v, 4); }
static void r_i(Rec* r, int v) { r_raw(r, &v, 4); }
static void r_f(Rec* r, float v) { r_raw(r, &v, 4); }
static void r_3(Rec* r, const float* p)
{
    if (p) { r_raw(r, p, 12); return; }
    { float nan3[3]; uint32_t q = 0x7fc00000u; memcpy(&nan3[0], &q, 4); nan3[1] = nan3[2] = nan3[0]; r_raw(r, nan3, 12); }
}
static void r_M(Rec* r, const float* p) { if (p) r_raw(r, p, 64); else { float z[16] = {0}; r_raw(r, z, 64); } }
static void r_m(Rec* r, const float* p) { unsigned char c = p ? 1 : 0; r_raw(r, &c, 1); if (p) r_raw(r, p, 64); }
static void r_F(Rec* r, int n, const float* v, int stride)
{
    int i, k = n < 0 ? 0 : (n > 64 ? 64 : n);
    r_i(r, k);
    for (i = 0; i < k && v; i++)
        r_raw(r, (const char*)v + (size_t)i * stride, 12);
}
static void r_end(Rec* r)
{
    uint16_t len = (uint16_t)(r->n - 4);
    memcpy(r->b + 2, &len, 2);
    EnterCriticalSection(&g_lock);
    open_trace();
    if (g_written + g_used + r->n < g_max) {
        if (g_used + r->n > g_cap)
            flush_locked();
        memcpy(g_buf + g_used, r->b, r->n);
        g_used += r->n;
    }
    {   /* at most a second's worth is lost when the game is killed */
        DWORD now = GetTickCount();
        if (now - g_lastFlush > 1000) { flush_locked(); g_lastFlush = now; }
    }
    LeaveCriticalSection(&g_lock);
}

/* ---- body bookkeeping (for the probes) ------------------------------------------------------------------------------- */
typedef struct { H world, body, bone; int inited; } BodyRec;
typedef struct { H rd, world; } RdRec;
static BodyRec g_bodies[4096];
static int g_nbodies;
static RdRec g_rds[1024];
static int g_nrds;

static void track_body(H world, H body, H bone)
{
    int i;
    if (!body)
        return;
    EnterCriticalSection(&g_lock);
    for (i = 0; i < g_nbodies; i++)
        if (g_bodies[i].body == body) { g_bodies[i].world = world; if (bone) g_bodies[i].bone = bone; break; }
    if (i == g_nbodies && g_nbodies < 4096) {
        g_bodies[g_nbodies].world = world; g_bodies[g_nbodies].body = body; g_bodies[g_nbodies].bone = bone;
        g_bodies[g_nbodies].inited = 0;
        g_nbodies++;
    }
    LeaveCriticalSection(&g_lock);
}
static void untrack_where(H world, H body)
{
    int i, j = 0;
    EnterCriticalSection(&g_lock);
    for (i = 0; i < g_nbodies; i++)
        if (!((world && g_bodies[i].world == world) || (body && g_bodies[i].body == body)))
            g_bodies[j++] = g_bodies[i];
    g_nbodies = j;
    if (world) {
        for (i = 0, j = 0; i < g_nrds; i++)
            if (g_rds[i].world != world) g_rds[j++] = g_rds[i];
        g_nrds = j;
    }
    LeaveCriticalSection(&g_lock);
}
static H rd_world(H rd)
{
    int i;
    H w = 0;
    EnterCriticalSection(&g_lock);
    for (i = 0; i < g_nrds; i++) if (g_rds[i].rd == rd) w = g_rds[i].world;
    LeaveCriticalSection(&g_lock);
    return w;
}
/* the bodies of a world (copied out so that no lock is held across the real DLL) */
static int bodies_of(H world, BodyRec* out, int max)
{
    int i, n = 0;
    EnterCriticalSection(&g_lock);
    for (i = 0; i < g_nbodies && n < max; i++)
        if (g_bodies[i].world == world) out[n++] = g_bodies[i];
    LeaveCriticalSection(&g_lock);
    return n;
}
static void mark_inited(H body)
{
    int i;
    EnterCriticalSection(&g_lock);
    for (i = 0; i < g_nbodies; i++) if (g_bodies[i].body == body) g_bodies[i].inited = 1;
    LeaveCriticalSection(&g_lock);
}

static void probe_body_init(H body)
{
    Rec r;
    float ad[4] = {0}, s2 = -1, w2 = -1, p0[4] = {0}, p1[4] = {0}, inv[4] = {0}, com[4] = {0}, ld = -1;
    int frames = -1, af = -1, jr = -1;
    if (r_NewtonBodyGetLinearDamping) ld = r_NewtonBodyGetLinearDamping(body);
    if (r_NewtonBodyGetAngularDamping) r_NewtonBodyGetAngularDamping(body, ad);
    if (r_NewtonBodyGetFreezeTreshold) r_NewtonBodyGetFreezeTreshold(body, &s2, &w2, &frames);
    if (r_NewtonBodyGetAutoFreeze) af = r_NewtonBodyGetAutoFreeze(body);
    if (r_NewtonBodyGetAABB) r_NewtonBodyGetAABB(body, p0, p1);
    if (r_NewtonBodyGetInvMass) r_NewtonBodyGetInvMass(body, &inv[0], &inv[1], &inv[2], &inv[3]);
    if (r_NewtonBodyGetCentreOfMass) r_NewtonBodyGetCentreOfMass(body, com);
    if (r_NewtonBodyGetJointRecursiveCollision) jr = r_NewtonBodyGetJointRecursiveCollision(body);
    r_begin(&r, NT_ProbeBodyInit); r_h(&r, body);
    r_f(&r, ld); r_3(&r, ad); r_f(&r, s2); r_f(&r, w2); r_i(&r, frames); r_i(&r, af); r_3(&r, p0); r_3(&r, p1);
    r_f(&r, inv[0]); r_f(&r, inv[1]); r_f(&r, inv[2]); r_f(&r, inv[3]); r_3(&r, com); r_i(&r, jr);
    r_end(&r);
}

/* ---- the exports ----------------------------------------------------------------------------------------------------- */
#define API __declspec(dllexport) __cdecl
#define BEGIN(op) Rec r; load_real(); r_begin(&r, op)

H API NewtonCreate(H a, H b)
{
    H w;
    BEGIN(NT_Create);
    w = r_NewtonCreate(a, b);
    r_h(&r, a); r_h(&r, b); r_h(&r, w); r_end(&r);
    if (r_NewtonWorldGetVersion) {
        Rec v; r_begin(&v, NT_ProbeVersion); r_h(&v, w); r_i(&v, r_NewtonWorldGetVersion(w)); r_end(&v);
    }
    return w;
}
void API NewtonDestroy(H w)
{
    BEGIN(NT_Destroy);
    r_h(&r, w); r_end(&r);
    untrack_where(w, 0);
    r_NewtonDestroy(w);
    EnterCriticalSection(&g_lock); flush_locked(); LeaveCriticalSection(&g_lock);
}
void API NewtonUpdate(H w, float dt)
{
    BodyRec bs[256];
    int i, n;
    DWORD now;
    BEGIN(NT_Update);
    n = bodies_of(w, bs, 256);
    for (i = 0; i < n; i++)
        if (!bs[i].inited) { probe_body_init(bs[i].body); mark_inited(bs[i].body); }
    r_h(&r, w); r_f(&r, dt); r_end(&r);
    r_NewtonUpdate(w, dt);
    now = GetTickCount();
    { Rec e; r_begin(&e, NT_UpdateEnd); r_h(&e, w); r_i(&e, (int)(now - g_t0)); r_end(&e); }
    for (i = 0; i < n; i++) {
        Rec p;
        float m[16], v[4], o[4];
        int s;
        r_NewtonBodyGetMatrix(bs[i].body, m);
        r_NewtonBodyGetVelocity(bs[i].body, v);
        r_NewtonBodyGetOmega(bs[i].body, o);
        s = r_NewtonBodyGetSleepingState(bs[i].body);
        r_begin(&p, NT_ProbeBody); r_h(&p, w); r_h(&p, bs[i].body); r_M(&p, m); r_3(&p, v); r_3(&p, o); r_i(&p, s);
        r_end(&p);
    }
}
void API NewtonSetSolverModel(H w, int m) { BEGIN(NT_SetSolverModel); r_h(&r, w); r_i(&r, m); r_end(&r); r_NewtonSetSolverModel(w, m); }
void API NewtonSetWorldSize(H w, const float* a, const float* b)
{
    BEGIN(NT_SetWorldSize); r_h(&r, w); r_3(&r, a); r_3(&r, b); r_end(&r); r_NewtonSetWorldSize(w, a, b);
}
int API NewtonMaterialGetDefaultGroupID(H w)
{
    int g;
    BEGIN(NT_MaterialGetDefaultGroupID);
    g = r_NewtonMaterialGetDefaultGroupID(w);
    r_h(&r, w); r_i(&r, g); r_end(&r);
    return g;
}
void API NewtonMaterialSetDefaultSoftness(H w, int a, int b, float v)
{
    BEGIN(NT_MaterialSetDefaultSoftness); r_h(&r, w); r_i(&r, a); r_i(&r, b); r_f(&r, v); r_end(&r);
    r_NewtonMaterialSetDefaultSoftness(w, a, b, v);
}
void API NewtonMaterialSetDefaultElasticity(H w, int a, int b, float v)
{
    BEGIN(NT_MaterialSetDefaultElasticity); r_h(&r, w); r_i(&r, a); r_i(&r, b); r_f(&r, v); r_end(&r);
    r_NewtonMaterialSetDefaultElasticity(w, a, b, v);
}
void API NewtonMaterialSetDefaultFriction(H w, int a, int b, float s, float k)
{
    BEGIN(NT_MaterialSetDefaultFriction); r_h(&r, w); r_i(&r, a); r_i(&r, b); r_f(&r, s); r_f(&r, k); r_end(&r);
    r_NewtonMaterialSetDefaultFriction(w, a, b, s, k);
}
H API NewtonCreateCapsule(H w, float rad, float h, const float* m)
{
    H c;
    BEGIN(NT_CreateCapsule);
    c = r_NewtonCreateCapsule(w, rad, h, m);
    r_h(&r, w); r_f(&r, rad); r_f(&r, h); r_m(&r, m); r_h(&r, c); r_end(&r);
    {
        Rec p;
        float id[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}, p0[4] = {0}, p1[4] = {0}, vol = -1;
        if (r_NewtonCollisionCalculateAABB) r_NewtonCollisionCalculateAABB(c, id, p0, p1);
        if (r_NewtonConvexCollisionCalculateVolume) vol = r_NewtonConvexCollisionCalculateVolume(c);
        r_begin(&p, NT_ProbeCollision); r_h(&p, c); r_3(&p, p0); r_3(&p, p1); r_f(&p, vol); r_end(&p);
    }
    return c;
}
H API NewtonCreateTreeCollision(H w, H cb)
{
    H c;
    BEGIN(NT_CreateTreeCollision);
    c = r_NewtonCreateTreeCollision(w, cb);
    r_h(&r, w); r_h(&r, cb); r_h(&r, c); r_end(&r);
    return c;
}
void API NewtonTreeCollisionBeginBuild(H c) { BEGIN(NT_TreeCollisionBeginBuild); r_h(&r, c); r_end(&r); r_NewtonTreeCollisionBeginBuild(c); }
void API NewtonTreeCollisionAddFace(H c, int n, const float* v, int stride, int attr)
{
    BEGIN(NT_TreeCollisionAddFace); r_h(&r, c); r_i(&r, stride); r_i(&r, attr); r_F(&r, n, v, stride); r_end(&r);
    r_NewtonTreeCollisionAddFace(c, n, v, stride, attr);
}
void API NewtonTreeCollisionEndBuild(H c, int opt) { BEGIN(NT_TreeCollisionEndBuild); r_h(&r, c); r_i(&r, opt); r_end(&r); r_NewtonTreeCollisionEndBuild(c, opt); }
void API NewtonReleaseCollision(H w, H c) { BEGIN(NT_ReleaseCollision); r_h(&r, w); r_h(&r, c); r_end(&r); r_NewtonReleaseCollision(w, c); }
void API NewtonConvexCollisionCalculateInertialMatrix(H c, float* in, float* org)
{
    BEGIN(NT_ConvexCollisionCalculateInertialMatrix);
    r_NewtonConvexCollisionCalculateInertialMatrix(c, in, org);
    r_h(&r, c); r_3(&r, in); r_3(&r, org); r_end(&r);
}
H API NewtonCreateBody(H w, H c)
{
    H b;
    BEGIN(NT_CreateBody);
    b = r_NewtonCreateBody(w, c);
    r_h(&r, w); r_h(&r, c); r_h(&r, b); r_end(&r);
    track_body(w, b, 0);
    return b;
}
void API NewtonDestroyBody(H w, H b)
{
    BEGIN(NT_DestroyBody); r_h(&r, w); r_h(&r, b); r_end(&r);
    untrack_where(0, b);
    r_NewtonDestroyBody(w, b);
}
void API NewtonBodySetMatrix(H b, const float* m) { BEGIN(NT_BodySetMatrix); r_h(&r, b); r_M(&r, m); r_end(&r); r_NewtonBodySetMatrix(b, m); }
void API NewtonBodyGetMatrix(H b, float* m) { BEGIN(NT_BodyGetMatrix); r_NewtonBodyGetMatrix(b, m); r_h(&r, b); r_M(&r, m); r_end(&r); }
void API NewtonBodySetVelocity(H b, const float* v) { BEGIN(NT_BodySetVelocity); r_h(&r, b); r_3(&r, v); r_end(&r); r_NewtonBodySetVelocity(b, v); }
void API NewtonBodyGetVelocity(H b, float* v) { BEGIN(NT_BodyGetVelocity); r_NewtonBodyGetVelocity(b, v); r_h(&r, b); r_3(&r, v); r_end(&r); }
void API NewtonBodySetOmega(H b, const float* v) { BEGIN(NT_BodySetOmega); r_h(&r, b); r_3(&r, v); r_end(&r); r_NewtonBodySetOmega(b, v); }
void API NewtonBodyGetOmega(H b, float* v) { BEGIN(NT_BodyGetOmega); r_NewtonBodyGetOmega(b, v); r_h(&r, b); r_3(&r, v); r_end(&r); }
void API NewtonBodyAddForce(H b, const float* v) { BEGIN(NT_BodyAddForce); r_h(&r, b); r_3(&r, v); r_end(&r); r_NewtonBodyAddForce(b, v); }
void API NewtonBodyAddTorque(H b, const float* v) { BEGIN(NT_BodyAddTorque); r_h(&r, b); r_3(&r, v); r_end(&r); r_NewtonBodyAddTorque(b, v); }
void API NewtonBodySetMassMatrix(H b, float m, float x, float y, float z)
{
    BEGIN(NT_BodySetMassMatrix); r_h(&r, b); r_f(&r, m); r_f(&r, x); r_f(&r, y); r_f(&r, z); r_end(&r);
    r_NewtonBodySetMassMatrix(b, m, x, y, z);
}
void API NewtonBodyGetMassMatrix(H b, float* m, float* x, float* y, float* z)
{
    BEGIN(NT_BodyGetMassMatrix);
    r_NewtonBodyGetMassMatrix(b, m, x, y, z);
    r_h(&r, b); r_f(&r, *m); r_f(&r, *x); r_f(&r, *y); r_f(&r, *z); r_end(&r);
}
void API NewtonBodySetCentreOfMass(H b, const float* v) { BEGIN(NT_BodySetCentreOfMass); r_h(&r, b); r_3(&r, v); r_end(&r); r_NewtonBodySetCentreOfMass(b, v); }
void API NewtonBodySetMaterialGroupID(H b, int g) { BEGIN(NT_BodySetMaterialGroupID); r_h(&r, b); r_i(&r, g); r_end(&r); r_NewtonBodySetMaterialGroupID(b, g); }
void API NewtonBodySetAngularDamping(H b, const float* v) { BEGIN(NT_BodySetAngularDamping); r_h(&r, b); r_3(&r, v); r_end(&r); r_NewtonBodySetAngularDamping(b, v); }
void API NewtonBodySetFreezeTreshold(H b, float s, float o, int f)
{
    BEGIN(NT_BodySetFreezeTreshold); r_h(&r, b); r_f(&r, s); r_f(&r, o); r_i(&r, f); r_end(&r);
    r_NewtonBodySetFreezeTreshold(b, s, o, f);
}
int API NewtonBodyGetSleepingState(H b)
{
    int s;
    BEGIN(NT_BodyGetSleepingState);
    s = r_NewtonBodyGetSleepingState(b);
    r_h(&r, b); r_i(&r, s); r_end(&r);
    return s;
}
H API NewtonCreateRagDoll(H w)
{
    H rd;
    BEGIN(NT_CreateRagDoll);
    rd = r_NewtonCreateRagDoll(w);
    r_h(&r, w); r_h(&r, rd); r_end(&r);
    EnterCriticalSection(&g_lock);
    if (g_nrds < 1024) { g_rds[g_nrds].rd = rd; g_rds[g_nrds].world = w; g_nrds++; }
    LeaveCriticalSection(&g_lock);
    return rd;
}
void API NewtonRagDollBegin(H rd) { BEGIN(NT_RagDollBegin); r_h(&r, rd); r_end(&r); r_NewtonRagDollBegin(rd); }
void API NewtonRagDollEnd(H rd) { BEGIN(NT_RagDollEnd); r_h(&r, rd); r_end(&r); r_NewtonRagDollEnd(rd); }

static Cb g_gameCb;
static void __cdecl force_tramp(H body)
{
    Rec r;
    r_begin(&r, NT_CallbackBegin); r_h(&r, body); r_end(&r);
    if (g_gameCb) g_gameCb(body);
    r_begin(&r, NT_CallbackEnd); r_h(&r, body); r_end(&r);
}
void API NewtonRagDollSetForceAndTorqueCallback(H rd, Cb cb)
{
    BEGIN(NT_RagDollSetForceAndTorqueCallback); r_h(&r, rd); r_h(&r, (H)cb); r_end(&r);
    if (cb) g_gameCb = cb;      /* the game installs one function (apply_force_and_torque) for every ragdoll */
    r_NewtonRagDollSetForceAndTorqueCallback(rd, cb ? force_tramp : 0);
}
H API NewtonRagDollAddBone(H rd, H parent, H ud, float mass, const float* m, H coll, const float* size)
{
    H bone, body;
    BEGIN(NT_RagDollAddBone);
    bone = r_NewtonRagDollAddBone(rd, parent, ud, mass, m, coll, size);
    r_h(&r, rd); r_h(&r, parent); r_h(&r, ud); r_f(&r, mass); r_M(&r, m); r_h(&r, coll); r_3(&r, size); r_h(&r, bone);
    r_end(&r);
    body = bone ? r_NewtonRagDollBoneGetBody(bone) : 0;
    if (body) {
        Rec p;
        float mm = 0, ix = 0, iy = 0, iz = 0, com[4] = {0}, lm[16] = {0}, gm[16] = {0};
        r_NewtonBodyGetMassMatrix(body, &mm, &ix, &iy, &iz);
        if (r_NewtonBodyGetCentreOfMass) r_NewtonBodyGetCentreOfMass(body, com);
        if (r_NewtonRagDollBoneGetLocalMatrix) r_NewtonRagDollBoneGetLocalMatrix(bone, lm);
        r_NewtonRagDollBoneGetGlobalMatrix(bone, gm);
        r_begin(&p, NT_ProbeBoneInit); r_h(&p, bone); r_h(&p, body);
        r_f(&p, mm); r_f(&p, ix); r_f(&p, iy); r_f(&p, iz); r_3(&p, com); r_M(&p, lm); r_M(&p, gm);
        r_end(&p);
        track_body(rd_world(rd), body, bone);
    }
    return bone;
}
H API NewtonRagDollBoneGetBody(H bone)
{
    H b;
    BEGIN(NT_RagDollBoneGetBody);
    b = r_NewtonRagDollBoneGetBody(bone);
    r_h(&r, bone); r_h(&r, b); r_end(&r);
    return b;
}
void API NewtonRagDollBoneGetGlobalMatrix(H bone, float* m)
{
    BEGIN(NT_RagDollBoneGetGlobalMatrix);
    r_NewtonRagDollBoneGetGlobalMatrix(bone, m);
    r_h(&r, bone); r_M(&r, m); r_end(&r);
}
void API NewtonRagDollBoneSetLimits(H bone, const float* cd, float mn, float mx, float tw, const float* bd, float a, float b)
{
    BEGIN(NT_RagDollBoneSetLimits);
    r_h(&r, bone); r_3(&r, cd); r_f(&r, mn); r_f(&r, mx); r_f(&r, tw); r_3(&r, bd); r_f(&r, a); r_f(&r, b); r_end(&r);
    r_NewtonRagDollBoneSetLimits(bone, cd, mn, mx, tw, bd, a, b);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID res)
{
    (void)h; (void)res;
    if (reason == DLL_PROCESS_ATTACH)
        InitializeCriticalSection(&g_lock);
    else if (reason == DLL_PROCESS_DETACH && g_open) {
        flush_locked();
        if (g_file != INVALID_HANDLE_VALUE) CloseHandle(g_file);
    }
    return TRUE;
}
