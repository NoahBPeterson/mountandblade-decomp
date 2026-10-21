// Slice s00440a30: VC7.1 <vector>/<algorithm> instantiations (std::copy_backward / std::fill over the 0x38-byte
// record, vector<E48/Rec1c8/E12/int>::push_back) and the free-list refill of a pool of 0xb0-byte records.
// Built with the plain flags (no EH switch), like s0043edf0 / s0043e514.
#include <vector>
#include <algorithm>
#include <memory>
#include <new>
#include <math.h>
#include "ai_agent.h"

// ---- element types (copied from s0043edf0) ---------------------------------------------------------------
struct E12 { int v[3]; };
struct E48 { int v[12]; };

struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring() {}
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
};

// 0x38-byte record: three ints, two int vectors, three ints.
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o);         // 0x43f510
};

// 0x1c8-byte polymorphic record (see s0043e514).
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Rec1c8 : IfaceB {
    char body1[0x190 - 0x10];
    tstring name;                               // +0x190
    char body2[0x1c8 - 0x1a0];
    Rec1c8(const Rec1c8& o);                    // 0x437bb0
    Rec1c8& operator=(const Rec1c8& o);         // 0x437d60
};

// @ 0x440a30
template E56* std::_Copy_backward_opt(E56*, E56*, E56*, std::_Nonscalar_ptr_iterator_tag);
// @ 0x440aa0
template void std::vector<E48>::push_back(const E48&);
// @ 0x440b30
template void std::vector<int>::push_back(const int&);
// @ 0x440b80
template void std::vector<Rec1c8>::push_back(const Rec1c8&);
// @ 0x440c10
template void std::vector<E12>::push_back(const E12&);
// @ 0x440dc0
template void std::fill(E56*, E56*, const E56&);

// ---- id pool: records handed out from pages of 16 -----------------------------------------------------------------
// 0xb0-byte record: +4 in-use flag, +8 id, +0xc serial number (provisional layout; ctor at 0x4377d0).
struct PoolRec {
    int f0;
    int used;                   // +4
    int id;                     // +8
    int serial;                 // +0xc
    char pad[0xb0 - 0x10];
    PoolRec();                  // 0x4377d0
    ~PoolRec();
};
struct Pool {
    int next_id;                // +0: ids handed out to pages so far
    std::vector<int> free_ids;  // +4
    std::vector<PoolRec*> pages; // +0x14 (first at +0x18)
    int serial;                 // +0x24
    int alloc();                // 0x440c90 (thiscall, no args; returns the id)
};
// @ 0x440c90
int Pool::alloc()
{
    if (!(free_ids.size() > 0)) {
        PoolRec* blk = new PoolRec[16];
        pages.push_back(blk);
        for (int j = 0; j < 16; j++) {
            blk[j].used = 0;
            blk[j].id = next_id + j;
            free_ids.push_back(next_id + j);
        }
        next_id += 16;
    }
    unsigned id = free_ids.back();
    free_ids.pop_back();
    PoolRec* r = &pages[id >> 4][id & 0xf];
    r->used = 1;
    r->serial = serial;
    serial++;
    return id;
}

// ---- agent line of sight / fire check ------------------------------------------------------------------------------
static inline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// @ 0x440e30
bool Agent::line_of_sight(unsigned tid, shape_base* ignore)
{
    Agent* o = &g_mission->agents.pages[tid >> 4][tid & 0xf];
    bool r = false;
    vector4 d;
    d.x = o->pos.x - pos.x;
    d.y = o->pos.y - pos.y;
    d.z = o->pos.z - pos.z;
    if (d.normalize() <= 300.0f && body != 0 && o->body != 0) {
        vector4 p, dir;
        aim_point_and_dir(&p, &dir);
        vector4 head = o->body->m.transform_point(*(const vector4*)(o->body->skel->recs + 0x1170));
        head.z += 0.2f;
        vector4 t;
        t.x = head.x - p.x;
        t.y = head.y - p.y;
        t.z = head.z - p.z;
        d = t;
        float len = d.normalize();
        float dp = dir.z * d.z + dir.y * d.y + dir.x * d.x;
        if (ignore) {
            capsule c(0.01f, &p, &head);
            if (shape_contact_count(&c, ignore, 0, 0))
                return false;
        }
        if (dp > -0.3f || len < 2.0f)
            r = g_mission->scene->trace_ray(&p.x, &head.x, 400.0f, &g_hit_shape, 1) != 0;
    }
    return r;
}

// @ 0x441020
int Agent::fire_check(unsigned tid, int* out_blocker)
{
    g_hit_shape = 0;
    *out_blocker = -1;
    if (f2c != -1) {
        float v = blend_target(blk60, blk294);
        if (!(g_range_tbl[f56c4].lo <= v) || !(v < g_range_tbl[f56c4].hi))
            return 3;
    }
    if (!line_of_sight(tid, 0))
        return 0;
    const vector4& tp = g_mission->agents.pages[tid >> 4][tid & 0xf].pos;
    float score = 0.0f;
    vector4 a;
    a.x = tp.x - pos.x;
    a.y = tp.y - pos.y;
    float dist;
    {
        a.z = tp.z - pos.z;
        float dz = a.z;
        dist = a.normalize();
        float reach = duration();
        a.z = (accel_factor(&reach, &dist, &dz) + a.z) * 0.1f;
    }
    a.normalize();
    float best = 0.0f;
    Mission* m = g_mission;
    unsigned i = m->agents.first_used();
    if (i < (unsigned)m->agents.count) {
        do {
            Agent* o = &m->agents.pages[i >> 4][i & 0xf];
            if (o->f10 == 0 && m->friends(team, o->team) && o->f14 == 1 && o->id != id) {
                vector4 b;
                b.x = o->pos.x - pos.x;
                b.y = o->pos.y - pos.y;
                b.z = (o->pos.z - pos.z) * 0.1f;
                float bl = b.normalize();
                float dp = b.x * a.x + b.y * a.y + b.z * a.z;
                if (dp > 0.2f) {
                    float s = (float)sqrt(1.0f - dp * dp);
                    float t = s - ((o->v56b4.out_length() + 1.0f) * 0.4f) / (bl + 0.2f);
                    if (t < 0.0f)
                        t = 0.0f;
                    if (bl > dist)
                        t = ((bl - dist) + 25.0f) * t * 0.052f;
                    t = (0.3f - t) * 3.3333333f;
                    if (t < 0.01f)
                        t = 0.0f;
                    t = (t * t) * (t * t);
                    if (t > 0.7f) {
                        score += t;
                        if (t > best) {
                            best = t;
                            *out_blocker = i;
                            m = g_mission;
                        }
                    }
                }
            }
            m->agents.next_used(&i);
        } while (i < (unsigned)m->agents.count);
        if (score > 0.9f)
            return 2;
    }
    return 1;
}

// ---- Newton physics sets (ragdolls / static bodies; see s0043a280) ---------------------------------------------------
struct NewtonWorld; struct NewtonBody; struct NewtonCollision; struct NewtonRagDoll; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonDestroy(const NewtonWorld* w);       // 0x549268
}
struct PhysRec {                                  // 0x38 bytes
    int f0;
    const NewtonWorld* world;                     // +4
    const NewtonRagDoll* ragdoll;                 // +8
    std::vector<const NewtonRagDollBone*> bones;  // +0xc (first at +0x10, last at +0x14)
    std::vector<const NewtonBody*> bodies;        // +0x1c (first at +0x20, last at +0x24)
    const NewtonCollision* collision;             // +0x2c
    NewtonBody* body;                             // +0x30
    int material;                                 // +0x34
};
struct PhysSet {
    std::vector<PhysRec> recs;                    // +0 (first at +4)
    char pad10[0x3c - 0x10];
    int nworlds;                                  // +0x3c
    void destroy_world(int idx);                  // 0x441360
};
// @ 0x441360
void PhysSet::destroy_world(int idx)
{
    if (recs[idx].world != 0) {
        recs[idx].ragdoll = 0;
        recs[idx].collision = 0;
        recs[idx].body = 0;
        recs[idx].bones.resize(0, 0);
        recs[idx].bodies.resize(0, 0);
        NewtonDestroy(recs[idx].world);
        recs[idx].world = 0;
        nworlds--;
    }
}
