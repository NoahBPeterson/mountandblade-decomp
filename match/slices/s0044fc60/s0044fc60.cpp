// Slice s0044fc60: Agent::start_ragdoll (0x44fc60, 6821 bytes): turn a dying agent's animated skeleton into a Newton
// ragdoll.  Allocates a physics record, builds one ragdoll bone (capsule) per skeleton bone that has capsule shapes,
// seeds each bone's linear/angular velocity from the animation (summing the ancestors' angular velocities), then
// finishes the ragdoll, places the bodies, rebuilds the static collision around the agent and adds a body for the
// mount (horse capsule).  Names are provisional.
#include <vector>
#include <math.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

struct NewtonWorld; struct NewtonBody; struct NewtonCollision; struct NewtonRagDoll; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonBodySetVelocity(const NewtonBody* b, const float* v);                  // 0x549214
__declspec(dllimport) void __cdecl NewtonBodySetOmega(const NewtonBody* b, const float* w);                     // 0x549218
__declspec(dllimport) void __cdecl NewtonBodySetMatrix(const NewtonBody* b, const float* m);                    // 0x549278
__declspec(dllimport) NewtonRagDoll* __cdecl NewtonCreateRagDoll(const NewtonWorld* w);                         // 0x549288
__declspec(dllimport) void __cdecl NewtonRagDollBegin(const NewtonRagDoll* rd);                                 // 0x54928c
__declspec(dllimport) void __cdecl NewtonRagDollEnd(const NewtonRagDoll* rd);                                   // 0x549284
__declspec(dllimport) void __cdecl NewtonRagDollSetForceAndTorqueCallback(const NewtonRagDoll* rd, void (__cdecl* cb)(const NewtonBody*));   // 0x549280
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);                 // 0x549294
}
void __cdecl apply_force_and_torque(const NewtonBody* body);                                                    // 0x432a50

// ---- physics set (layouts from s004413e0) -------------------------------------------------------------------
struct PhysRec {                                  // 0x38 bytes
    int f0;
    const NewtonWorld* world;                     // +4
    const NewtonRagDoll* ragdoll;                 // +8
    char pad0c[0x38 - 0xc];
};
struct PhysSet {
    std::vector<PhysRec> recs;                    // +0 (first at +4)
    int alloc();                                  // 0x44fad0 (thiscall): new record index
    const NewtonRagDollBone* add_bone(int idx, const NewtonRagDollBone* parent, const float* mass, matrix m0,
                                      const float* radius, const float* height, matrix m1, const float* cone_dir,
                                      const float* lim_min, const float* lim_max, const float* lim_twist);   // 0x4413e0 (ret 0xa4)
    NewtonBody* add_capsule_body(int idx, const float* radius, const float* height, const float* mass, matrix* m0, matrix* m1, bool list);   // 0x4416c0 (ret 0x1c)
};
extern PhysSet g_phys;                            // 0xa181f8

// ---- frames -------------------------------------------------------------------------------------------------
struct mat3 { vector4 s, f, u; };
mat3* __cdecl identity3(mat3* out);                                       // 0x40cd60

struct pos3 : matrix {                            // the frame type of s0040e060
    matrix mul(const matrix& m) const;                                      // 0x40e870 (thiscall, hidden return pointer)
    void rotate_fu(const float* angle);                                     // 0x40e570
    void rotate_su(const float* angle);                                     // 0x40e670
    void rotate_sf(const float* angle);                                     // 0x40e770
};

// ---- skeleton -----------------------------------------------------------------------------------------------
struct CapDef { vector4 a; vector4 b; float r; int pad[2]; };               // 0x2c bytes
struct ChanRec {                                  // 0xdc bytes: one skeleton bone definition
    char pad0[4];
    int parent;                                   // +4
    char pad8[4];
    pos3 m;                                       // +0xc: local frame
    char pad4c[0x9c - 0x4c];
    float mass;                                   // +0x9c
    float lim_min, lim_max, lim_twist;            // +0xa0 / +0xa4 / +0xa8 (degrees)
    float rot_x, rot_y, rot_z;                    // +0xac / +0xb0 / +0xb4 (degrees)
    char padb8[4];
    vector4 cone;                                 // +0xbc (x, y, z of the limit cone direction)
    std::vector<CapDef> caps;                     // +0xcc (first +0xd0, last +0xd4)
};
struct SkelData {
    char pad0[0x18];
    int count;                                    // +0x18
    ChanRec* items;                               // +0x1c
};
struct SkelElem {                                 // 0x1dc bytes: runtime state of one bone (FxElem of s00428240)
    char pad0[4];
    matrix m4;                                    // +0x04
    matrix m44;                                   // +0x44: local frame as loaded
    matrix m84;                                   // +0x84: world frame
    char padc4[0xf4 - 0xc4];
    float wx, wy, wz;                             // +0xf4: angular velocity of the animation
    char pad100[0x1d8 - 0x100];
    const NewtonRagDollBone* bone;                // +0x1d8
};
struct Skeleton {
    int nbones;                                   // +0
    SkelElem* recs;                               // +4
    SkelData* data;                               // +8
    char pad0c[0x64 - 0xc];
    int has_poses;                                // +0x64
    char pad68[4];
    float rvx, rvy, rvz;                          // +0x6c: root velocity (body-local)
};
struct AgentBody {
    char pad0[0x58];
    pos3 m;                                       // +0x58
    char pad98[0x304 - 0x98];
    Skeleton* skel;                               // +0x304
};

struct V4 { float x, y, z, w; };               // constructor-free storage for the local arrays
struct M4 { V4 s, f, u, o; };

struct Agent;
struct Mission {
    char pad0[0x1c];
    Agent** pages;                                // +0x1c (id >> 4; 16 agents per page)
};
extern Mission* g_mission;                        // 0x5e292c

struct Agent {                                    // 0x5928 bytes
    char pad0[0x2c];
    unsigned link_id;                             // +0x2c (agent whose velocity this one inherits, -1 = none)
    char pad30[0x80 - 0x30];
    float scale;                                  // +0x80
    char pad84[0x250 - 0x84];
    AgentBody* body;                              // +0x250
    char pad254[0x56a4 - 0x254];
    vector4 vel;                                  // +0x56a4
    char pad56b4[0x5838 - 0x56b4];
    int mount_id;                                 // +0x5838
    NewtonBody* mount_body;                       // +0x583c
    char pad5840[0x5880 - 0x5840];
    int disabled;                                 // +0x5880
    char pad5884[0x58c4 - 0x5884];
    char f58c4, f58c5;                            // +0x58c4 / +0x58c5
    char pad58c6[2];
    vector4 link_vel;                             // +0x58c8
    char f58d8;                                   // +0x58d8
    char pad58d9[3];
    int phys_idx;                                 // +0x58dc
    char pad58e0[0x5928 - 0x58e0];
    void build_static_collision();                // 0x4417d0
    void start_ragdoll();                         // 0x44fc60
};

// inlined helpers (see s004417d0)
static __forceinline float vnorm(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}
static __forceinline vector4 cross3(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
    return r;
}
// frame whose side axis is `dir`, f/u completed with the cross products (the inline code of s004417d0)
static __forceinline void make_frame(matrix& R, const vector4& dir)
{
    R.s = dir;
    vector4 c1 = cross3(R.s, R.f);
    if (c1.length() < 0.001f) {
        R.f = cross3(R.u, R.s);
        vnorm(R.f);
        R.u = cross3(R.s, R.f);
        vnorm(R.u);
    } else {
        R.u = c1;
        vnorm(R.u);
        R.f = cross3(R.u, R.s);
        vnorm(R.f);
    }
}

// transform a point by a frame
static __forceinline void tpoint(vector4& r, const matrix& m, const vector4& p)
{
    r.x = p.y * m.f.x + p.z * m.u.x + p.x * m.s.x + m.o.x;
    r.y = m.f.y * p.y + m.u.y * p.z + m.s.y * p.x + m.o.y;
    r.z = m.f.z * p.y + m.u.z * p.z + m.s.z * p.x + m.o.z;
}
// rotate a vector by the 3x3 part of a frame
static __forceinline void rot3v(vector4& r, const matrix& m, float x, float y, float z)
{
    r.x = m.s.x * x + m.u.x * z + m.f.x * y;
    r.y = m.s.y * x + m.u.y * z + m.f.y * y;
    r.z = m.s.z * x + m.u.z * z + m.f.z * y;
}

// @ 0x44fc60
void Agent::start_ragdoll()
{
    int idx = g_phys.alloc();
    phys_idx = idx;
    g_phys.recs[idx].ragdoll = NewtonCreateRagDoll(g_phys.recs[idx].world);
    NewtonRagDollBegin(g_phys.recs[idx].ragdoll);

    AgentBody* b = body;
    SkelData* sd = b->skel->data;
    int n = sd->count;
    V4 wacc[32], vacc[32];
    M4 Wr[32], Ar[32];
    matrix* W = (matrix*)Wr;
    matrix* A = (matrix*)Ar;
    int i;

    // zero the accumulators
    for (i = 0; i < n; i++) {
        *(vector4*)&vacc[i] = vector4(0.0f, 0.0f, 0.0f);
        *(vector4*)&wacc[i] = vector4(0.0f, 0.0f, 0.0f);
    }
    vector4 v, w;                                           // velocity / angular velocity handed to Newton

    // sum the ancestors' angular velocities into every bone
    for (i = 0; i < n; i++) {
        float px, py, pz;
        {
            const vector4& o = b->skel->recs[i].m84.o;
            px = b->m.f.x * o.y + b->m.u.x * o.z + b->m.s.x * o.x + b->m.o.x;
            py = b->m.f.y * o.y + b->m.u.y * o.z + b->m.s.y * o.x + b->m.o.y;
            pz = b->m.f.z * o.y + b->m.u.z * o.z + b->m.s.z * o.x + b->m.o.z;
        }
        int j = sd->items[i].parent;
        if (j >= 0) {
            do {
                const vector4& o = b->skel->recs[j].m84.o;
                float qz = b->m.f.z * o.y + b->m.s.z * o.x + b->m.u.z * o.z + b->m.o.z;
                float rx = px - (b->m.f.x * o.y + b->m.u.x * o.z + b->m.s.x * o.x + b->m.o.x);
                float ry = py - (b->m.f.y * o.y + b->m.s.y * o.x + b->m.u.y * o.z + b->m.o.y);
                float rz = pz - qz;
                SkelElem* e = &b->skel->recs[j];
                float lx = e->m84.s.x * e->wx + e->m84.f.x * e->wy + e->m84.u.x * e->wz;
                float ly = e->m84.s.y * e->wx + e->m84.f.y * e->wy + e->m84.u.y * e->wz;
                float lz = e->m84.s.z * e->wx + e->m84.f.z * e->wy + e->m84.u.z * e->wz;
                float vx = ly * b->m.f.x + (lz * b->m.u.x + lx * b->m.s.x);
                float vy = ly * b->m.f.y + (lz * b->m.u.y + lx * b->m.s.y);
                float vz = ly * b->m.f.z + (lz * b->m.u.z + lx * b->m.s.z);
                float cx = vy * rz - vz * ry;
                float cz = vx * ry - rx * vy;
                vacc[i].x = cx + vacc[i].x;
                vacc[i].y = (rx * vz - vx * rz) + vacc[i].y;
                vacc[i].z = cz + vacc[i].z;
                wacc[i].x = vx + wacc[i].x;
                wacc[i].y = vy + wacc[i].y;
                wacc[i].z = vz + wacc[i].z;
                do {
                    j = sd->items[j].parent;
                } while (sd->items[j].caps.size() == 0);
            } while (j >= 0);
        }
    }

    // create the bones
    for (i = 0; i < n; i++) {
        ChanRec* rec = &sd->items[i];
        int p = rec->parent;
        matrix T;
        if (p < 0) {
            W[i] = b->m.mul(b->skel->recs[i].m84);
            T = b->m.mul(rec->m);
            A[i] = T;
        } else {
            W[i] = ((pos3&)W[p]).mul(b->skel->recs[i].m44);
            matrix cur = rec->m;
            int j = p;
            for (;;) {
                if (sd->items[j].caps.size() != 0)
                    break;
                cur = sd->items[j].m.mul(cur);
                j = sd->items[j].parent;
            }
            T = cur;
            float a0 = rec->rot_x * 0.017453292f;
            ((pos3&)T).rotate_fu(&a0);
            float a1 = rec->rot_y * 0.017453292f;
            ((pos3&)T).rotate_su(&a1);
            float a2 = rec->rot_z * 0.017453292f;
            ((pos3&)T).rotate_sf(&a2);
            A[i] = ((pos3&)A[j]).mul(T);
        }
        if (rec->caps.size() != 0) {
            float radius = 0.0f, height = 0.01f;
            matrix R;
            vector4 z0(0.0f, 0.0f, 0.0f);
            mat3 t;
            mat3* ip = identity3(&t);
            *(mat3*)&R = *ip;
            R.o = z0;
            for (unsigned k = 0; k < rec->caps.size(); k++) {
                const CapDef& d = rec->caps[k];
                radius = d.r * scale;
                vector4 dir;
                dir.x = d.b.x - d.a.x; dir.y = d.b.y - d.a.y; dir.z = d.b.z - d.a.z;
                float len = vnorm(dir);
                height = len * scale;
                make_frame(R, dir);
                vector4 mid(d.b.x + d.a.x, d.b.y + d.a.y, d.b.z + d.a.z);
                R.o = vector4(mid.x * 0.5f * scale, mid.y * 0.5f * scale, mid.z * 0.5f * scale);
            }
            // physics parent: nearest ancestor that already has a bone
            const NewtonRagDollBone* pbone = 0;
            int q = p;
            if (q >= 0) {
                if (b->skel->recs[q].bone == 0) {
                    do {
                        q = sd->items[q].parent;
                    } while (b->skel->recs[q].bone == 0);
                }
                pbone = b->skel->recs[q].bone;
            }
            vector4 cone;
            rot3v(cone, A[i], rec->cone.x, rec->cone.y, rec->cone.z);
            b->skel->recs[i].bone = g_phys.add_bone(phys_idx, pbone, &rec->mass, T, &radius, &height, R, &cone.x,
                                                     &rec->lim_min, &rec->lim_max, &rec->lim_twist);
            v = vel;
            link_vel = vector4(0.0f, 0.0f, 0.0f);
            if (link_id != (unsigned)-1) {
                const Agent* other = &g_mission->pages[link_id >> 4][link_id & 0xf];
                link_vel = other->vel;
                v = vector4(0.0f, 0.0f, 0.0f);
            }
            vector4 rv;
            rot3v(rv, b->m, b->skel->rvx, b->skel->rvy, b->skel->rvz);
            v.x = rv.x + v.x;
            v.y = rv.y + v.y;
            v.z = v.z + rv.z;
            v.x = v.x + vacc[i].x;
            v.y = v.y + vacc[i].y;
            v.z = v.z + vacc[i].z;
            NewtonBody* nb = NewtonRagDollBoneGetBody(b->skel->recs[i].bone);
            SkelElem* e = &b->skel->recs[i];
            vector4 wl;
            rot3v(wl, e->m84, e->wx, e->wy, e->wz);
            rot3v(w, b->m, wl.x, wl.y, wl.z);
            w.x = w.x + wacc[i].x;
            w.y = w.y + wacc[i].y;
            w.z = w.z + wacc[i].z;
            NewtonBodySetVelocity(nb, &v.x);
            NewtonBodySetOmega(nb, &w.x);
        }
    }

    NewtonRagDollSetForceAndTorqueCallback(g_phys.recs[phys_idx].ragdoll, apply_force_and_torque);
    NewtonRagDollEnd(g_phys.recs[phys_idx].ragdoll);
    for (i = 0; i < sd->count; i++) {
        if (body->skel->recs[i].bone != 0)
            NewtonBodySetMatrix(NewtonRagDollBoneGetBody(body->skel->recs[i].bone), &W[i].s.x);
    }
    build_static_collision();

    // the mount: a capsule body for the horse
    unsigned mid = mount_id;
    if ((int)mid >= 0 && g_mission->pages[mid >> 4][mid & 0xf].body != 0) {
        const Agent* h = &g_mission->pages[mid >> 4][mid & 0xf];
        vector4 z0(0.0f, 0.0f, 0.0f);
        mat3 t;
        matrix R;
        mat3* ip = identity3(&t);
        *(mat3*)&R = *ip;
        float radius = 0.2114f * h->scale;
        vector4 dir(0.0f, 1.2886f, 0.0f);
        float len = vnorm(dir);
        float height = len * h->scale;
        make_frame(R, dir);
        R.o = vector4(0.0f * h->scale, 0.5443f * h->scale, -0.0629f * h->scale);
        matrix m0 = h->body->m.mul(h->body->skel->recs->m84);
        float zero = 0.0f;
        mount_body = g_phys.add_capsule_body(phys_idx, &radius, &height, &zero, &m0, &R, false);
    }
    disabled = 1;
    f58d8 = 0;
    f58c4 = 1;
    f58c5 = 1;
    body->skel->has_poses = 1;
}
