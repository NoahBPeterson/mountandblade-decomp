// Slice s0045d820: Agent::move_step (0x45d820, 7.7 KB): the per-frame movement/collision step of an agent
// (human or horse). Integrates gravity/drag into the velocity, sweeps the agent's capsule through the scene (other
// agents: push-out + horse trample blow; static entities: push-out + collision callback), probes the floor under
// the new position, handles landing (impact blow, sound, order) and ground stepping, then commits the position.
// All names are provisional; layouts come from this function and from the neighbouring slices (s004375b0 orders,
// s0040d7d0 shapes, s00434ad0 Rows/Blow, s004465e0 mission/agent table, s00448eb0 scene queries).
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <new>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

extern __int64 g_ticks[];                           // 0x5e2948 (100 kHz clocks)

struct pos : matrix {
    vector4 transform_point(const vector4& p) const;    // 0x408460
    vector4 rot(const vector4& p) const;                // 0x40a9c0
};

// ---- collision shapes (layout of s0040d7d0) ---------------------------------------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                            // slot 0
    virtual int shape_type();                           // slot 1
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6(); virtual void v7();
    virtual void transform(const pos* m);               // slot 8
    virtual void v9();
    virtual vector4* get_min(vector4* out);             // slot 10
    virtual vector4* get_max(vector4* out);             // slot 11
    virtual void v12(); virtual void v13(); virtual void v14();
};
struct capsule : shape_base {                           // vtable 0x54a540, 0x64 bytes
    vector4 p0, p1, p2, p3;
    float radius;
    virtual void transform(const pos* m);               // 0x408770
    virtual vector4* get_min(vector4* out);             // 0x40ad80
    virtual vector4* get_max(vector4* out);             // 0x40ade0
};
struct SceneObj : shape_base {                          // scene object returned by the box query
    char pad20[0xa8 - 0x20];
    float fa8;                                          // +0xa8
};
struct face_shape : shape_base {
    char pad20[0xa0 - 0x20];
    vector4 normal;                                     // +0xa0
};
struct Contact {                                        // 0x24 bytes
    vector4 n;                                          // contact normal
    vector4 p;                                          // contact point
    float depth;                                        // +0x20
};
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, Contact* out);   // 0x40bfe0
vector4* __cdecl bounce(vector4* out, vector4* v, const vector4* n);                    // 0x432e30

// ---- scene / entities ---------------------------------------------------------------------------------------
struct StratEnt {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14();
    virtual bool is_movable();                          // slot 15 (+0x3c)
    char pad4[0x18 - 4];
    pos p18;                                            // +0x18
    pos p58;                                            // +0x58
};
struct Entity {                                         // scene entry
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(); virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15(); virtual void s16();
    virtual void s17(); virtual void s18(); virtual void s19(); virtual void s20(); virtual void s21();
    virtual void s22(); virtual void s23(); virtual void s24(); virtual void s25();
    virtual void collide(const vector4* point, const vector4* impulse);     // slot 26 (+0x68)
    char pad4[0xc - 4];
    unsigned id;                                        // +0xc
    int type;                                           // +0x10 (0xb = agent, 0 = static)
    char pad14[0x98 - 0x14];
    shape_base* shape;                                  // +0x98
};
struct EntVec {                                         // std::vector<Entity*> layout (VC7.1), freed by hand
    int alloc_pad;
    Entity** first;
    Entity** last;
    Entity** end;
};
struct FaceList { face_shape* first; };
struct Scene {
    char pad0[0x58];
    FaceList* faces;                                    // +0x58
    StratEnt* find_entity(int id);                      // 0x42ba50
    void collect_boxed(const vector4* bmin, const vector4* bmax, EntVec* out);   // 0x442180
    void query_box(const float* bmin, const float* bmax, unsigned mask, SceneObj** out, int* count);   // 0x42aa50
    void floor_at(const float* p, float* out_z, face_shape** out_face);          // 0x439be0
};
struct Agent;
struct AgentTable {
    int count;                                          // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (id >> 4 -> page of 16 agents)
    Agent* get(unsigned id);                            // 0x43b1f0
};
struct Mission {
    Scene* scene;                                       // +0
    AgentTable agents;                                  // +4
    char pad28[0x5c - 0x28];
    int local_id;                                       // +0x5c
    char pad60[0x653c - 0x60];
    float bounds[4];                                    // +0x653c (min x, min y, max x, max y)
    float water;                                        // +0x654c
};
extern Mission* g_mission;                              // 0x5e292c

struct World {
    char pad0[0x13a00];
    char* troops;
    int skill_level(int troop, int skill);              // 0x437b20
};
extern World* g_world;                                  // 0x5e2930
extern float g_kick_scale;                              // 0xa1819c

// ---- agent records --------------------------------------------------------------------------------------------
struct Timer16 {                                        // 16 bytes
    int clock; int pad4; __int64 start;
    float elapsed() const;                              // 0x432c10
};
struct ItemRef {
    int item; unsigned m4;
    int stat_214();                                     // 0x435040
};
struct OrderSlot {                                      // 0x2c bytes
    int a, b, c;
    char padc[0x14 - 0xc];
    unsigned m14;                                       // +0x14
    char pad18[0x20 - 0x18];
    float rate;                                         // +0x20
    char pad24[0x2c - 0x24];
    bool set(int id, unsigned prio);                    // 0x4378a0
};
struct Part { char pad0[0xc]; float f0c; char pad10[0x2c - 0x10]; vector4 v2c; char pad3c[4]; };   // 0x40 bytes
struct AnimRec { char pad0[0x10]; Part* parts; int pad14; unsigned flags; char pad1c[0x30 - 0x1c]; };   // 0x30 bytes
struct AnimSet { AnimRec* recs; int count; };

struct Blow {                                           // 0x58 bytes (Rows of s00434ad0)
    int m0, m4;
    vector4 v8, v18, v28;
    int m38; float m3c; float m40; int m44, m48, m4c, m50, m54;
    Blow* init();                                       // 0x4353a0
};
struct vec2 {
    float x, y;
    float normalize();                                  // 0x4367c0
};
struct AgentBody {
    char pad0[0x58];
    pos m;                                              // +0x58
};
namespace rgl { float dot(const vector4& a, const vector4& b); }    // 0x407e90

struct Agent {
    char pad0[8];
    unsigned id;                                        // +8
    char padc[4];
    int type;                                           // +0x10 (0 human, 1 horse)
    int state;                                          // +0x14 (1 = alive)
    char pad18[0x2c - 0x18];
    int mount_id;                                       // +0x2c
    int rider_id;                                       // +0x30
    char pad34[0x3c - 0x34];
    unsigned flags;                                     // +0x3c (bit 0 = on the ground)
    Timer16 t40;                                        // +0x40
    vector4 pos;                                        // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                          // +0x88
    ItemRef item8c;                                     // +0x8c
    char pad94[0xa0 - 0x94];
    float fa0;                                          // +0xa0
    char pada4[0xa8 - 0xa4];
    float fa8;                                          // +0xa8 (side speed)
    float fac;                                          // +0xac (forward speed)
    char padb0[0xbc - 0xb0];
    face_shape* floor;                                  // +0xbc
    SceneObj* ground;                                   // +0xc0
    char padc4[0xd4 - 0xc4];
    OrderSlot slots[2];                                 // +0xd4
    AnimSet* defs;                                      // +0x12c
    char pad130[0x250 - 0x130];
    AgentBody* body;                                    // +0x250
    char pad254[0x5638 - 0x254];
    int has_blow;                                       // +0x5638
    Blow last_blow;                                     // +0x563c
    vector4 force;                                      // +0x5694
    vector4 vel;                                        // +0x56a4
    vector4 smooth;                                     // +0x56b4
    char pad56c4[0x5928 - 0x56c4];

    void move_step(float dt);                           // 0x45d820
    void fill_capsule(capsule* c);                      // 0x435c10
    void set_order(int slot, int id, unsigned prio, float rate);   // 0x4379c0
    void play_hit_sound(int* kind);                     // 0x4435c0
    void play_sound_var(int* kind, float* var);         // 0x443730
    void reduce_blow(Blow* b);                          // 0x454e30
    void note_blow(Blow* b);                            // 0x457af0
    bool is_enemy(const Agent* o) const;                // 0x432350
};

static __forceinline float elapsed_inl(const Timer16& t) { return (float)(g_ticks[t.clock] - t.start) * 1e-5f; }
static __forceinline void vadd_to(vector4& a, const vector4& b) { a.x = a.x + b.x; a.y = a.y + b.y; a.z = a.z + b.z; }
struct V3 { float x, y, z; V3() {} V3(float a, float b, float c) : x(a), y(b), z(c) {} };
static __forceinline V3 vneg(const vector4& a) { return V3(-a.x, -a.y, -a.z); }
static __forceinline void vadd3(vector4& a, const V3& b) { a.x = a.x + b.x; a.y = a.y + b.y; a.z = a.z + b.z; }
static __forceinline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 unit_up(const vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.z * v.z + v.y * v.y);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        return vector4(v.x * inv, inv * v.y, inv * v.z);
    }
    return vector4(0.0f, 1.0f, 0.0f);
}
static __forceinline vector4 vscale(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }

// @ 0x45d820
void Agent::move_step(float dt)
{
    vector4 push(0.0f, 0.0f, 0.0f);
    vector4 v(0.0f, 0.0f, 0.0f);
    vector4 gv(0.0f, 0.0f, 0.0f);
    vector4 np;
    bool hit_ok;
    vector4 hits[16];
    SceneObj* shapes[8188];
    int nh;
    {
    ::pos m = body->m;
    if ((flags & 1) && elapsed_inl(t40) >= 0.0f) {
        vector4 a(m.s.x * fa8, m.s.y * fa8, m.s.z * fa8);
        vector4 b(m.f.x * fac, m.f.y * fac, m.f.z * fac);
        push = vector4(a.x + b.x, a.y + b.y, a.z + b.z);
        int fid = floor->mc;
        if (fid >= 0) {
            StratEnt* ent = g_mission->scene->find_entity(fid);
            if (ent && ent->is_movable()) {
                vector4 loc = ent->p18.transform_point_to_local(pos);
                vector4 wp = ent->p58.transform_point(loc);
                vector4 d = vsub(wp, pos);
                if (dt > 0.0f) {
                    float inv = 1.0f / dt;
                    vector4 t = vscale(inv, d);
                    vadd_to(push, t);
                }
            }
        }
    } else {
        v = vel;
        force.z = force.z - dt * 9.8f;
        vadd_to(v, force);
    }
    float k = 1.0f - ((flags & 1) ? dt * 0.4f : dt * 0.1f);
    v.x = v.x * k;
    v.y = v.y * k;
    v.z = k * v.z;
    if (body && slots[0].a != -1 && slots[0].c != -1) {
        AnimRec* r = &defs->recs[slots[0].a];
        if (r->flags & 0x4000) {
            Part* p = &r->parts[slots[0].c];
            float inv = 1.0f / p->f0c;
            gv = vscale(inv, body->m.rot(p->v2c));
        }
    }
    vector4 nv(v.x + push.x, v.y + push.y, v.z + push.z);
    vel = nv;
    push.x = 0.0f;
    push.y = 0.0f;
    push.z = 0.0f;
    gv = vector4(nv.x + gv.x, nv.y + gv.y, nv.z + gv.z);
    np.x = gv.x * dt;
    np.y = gv.y * dt;
    np.z = gv.z * dt;
    capsule cap;
    fill_capsule(&cap);
    m.o = vector4(np.x + pos.x, np.y + pos.y, np.z + pos.z);
    cap.transform(&m);
    vector4 t;
    vector4 bmin, bmax;
    {
        vector4* q = cap.get_min(&t);
        bmin.x = q->x - 0.1f;
        bmin.y = q->y - 0.1f;
        bmin.z = q->z - 0.1f;
    }
    {
        vector4* q = cap.get_max(&t);
        bmax.x = q->x + 0.1f;
        bmax.y = q->y + 0.1f;
        bmax.z = q->z + 0.1f;
    }
    hit_ok = true;
    Contact contact;
    if ((float)sqrt(np.y * np.y + np.x * np.x + np.z * np.z) > 0.0f) {
        EntVec ents;
        ents.first = 0;
        ents.last = 0;
        ents.end = 0;
        g_mission->scene->collect_boxed(&bmin, &bmax, &ents);
        for (Entity** it = ents.first; it < ents.last; ++it) {
            Entity* e = *it;
            if (e->shape != 0 && e->type == 0xb && e->id != id && e->id != (unsigned)mount_id && e->id != (unsigned)rider_id) {
                Agent* o = &g_mission->agents.pages[e->id >> 4][e->id & 0xf];
                if (o->state == 1 && shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                    vector4 old = np;
                    vector4 rv = *bounce(&t, &np, &contact.n);
                    if (type == 1 && fac > 4.0f && o->type == 0 && o->mount_id == -1) {
                        int st = item8c.stat_214();
                        float f = (float)st * 0.03f + 0.1f;
                        if (rider_id >= 0)
                            f = (float)g_world->skill_level(g_mission->agents.get(rider_id)->troop, 0x18) * 0.03f + f;
                        if (f < 0.0f)
                            f = 0.0f;
                        else if (f > 0.96f)
                            f = 0.96f;
                        float g = 1.0f - f;
                        rv.x = rv.x * g;
                        rv.y = rv.y * g;
                        rv.z = rv.z * g;
                        np = vector4((np.x - old.x) * g + old.x, g * (np.y - old.y) + old.y, g * (np.z - old.z) + old.z);
                    } else if (type == 1 && fac > 3.0f && o->type == 1) {
                        hit_ok = !(rgl::dot(body->m.f, o->body->m.f) > 0.0f) || !(o->fac > 0.2f);
                    }
                    if (type == 0 && o->type == 0 && o->mount_id == -1) {
                        vec2 d2;
                        d2.x = o->pos.x - (np.x + pos.x);
                        d2.y = o->pos.y - (np.y + pos.y);
                        float len = d2.normalize();
                        if (len < 0.7f) {
                            float pen = (0.7f - len) * -0.6f;
                            vec2 tt;
                            tt.x = d2.x * pen;
                            tt.y = d2.y * pen;
                            np.x = tt.x + np.x;
                            np.y = tt.y + np.y;
                        }
                    }
                    m.o = vector4(np.x + pos.x, np.y + pos.y, np.z + pos.z);
                    cap.transform(&m);
                    vadd3(push, vneg(rv));
                    if (type == 1 && fac > 3.5f && o->type == 0 && o->mount_id == -1 && (o->flags & 1) &&
                        o->t40.elapsed() >= 0.0f) {
                        if (rider_id == -1 || g_mission->agents.get(rider_id)->is_enemy(o)) {
                            Blow b;
                            b.init();
                            vector4 nn(-contact.n.x, -contact.n.y, -contact.n.z);
                            b.m38 = 7;
                            int s2 = item8c.stat_214() + 5;
                            float dmg = (((nn.z * vel.z + nn.y * vel.y + nn.x * vel.x) + 0.1f) * (float)s2 * g_kick_scale) * fac * 0.015f;
                            if (dmg < 0.0f)
                                dmg = 0.0f;
                            b.m3c = dmg;
                            b.v18 = nn;
                            b.v28 = nn;
                            b.m4 = id;
                            b.v8 = contact.p;
                            b.m48 = 2;
                            b.m0 = 0x10;
                            o->note_blow(&b);
                        }
                    }
                }
            } else if (e->shape != 0 && e->type == 0) {
                if (shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                    vector4 fv;
                    fv.x = contact.n.x * contact.depth;
                    fv.y = contact.depth * contact.n.y;
                    fv.z = contact.depth * contact.n.z;
                    np.x = fv.x + np.x;
                    np.y = np.y + fv.y;
                    np.z = np.z + fv.z;
                    V3 nn = vneg(contact.n);
                    vector4 mir;
                    mir.x = nn.x + nn.x;
                    mir.y = nn.y + nn.y;
                    mir.z = nn.z + nn.z;
                    e->collide(&contact.p, &mir);
                    cap.p0.x = cap.p0.x + fv.x;
                    cap.p0.y = cap.p0.y + fv.y;
                    cap.p0.z = cap.p0.z + fv.z;
                    cap.p1.x = cap.p1.x + fv.x;
                    cap.p1.y = cap.p1.y + fv.y;
                    cap.p1.z = cap.p1.z + fv.z;
                    push.x = fv.x + push.x;
                    push.y = fv.y + push.y;
                    push.z = fv.z + push.z;
                }
            }
        }
        if (ents.first != 0)
            operator delete(ents.first);
    }
    unsigned mask = 0x8002;
    if (id == (unsigned)g_mission->local_id || !(flags & 1))
        mask = 0x8102;
    int count;
    g_mission->scene->query_box(&bmin.x, &bmax.x, mask, shapes, &count);
    nh = 0;
    float best = 0.0f;
    ground = 0;
    vector4* hp = hits;
    for (int i = 0; i < count; i++) {
        SceneObj* sh = shapes[i];
        if (shape_contact_count(&cap, sh, 1, &contact) == 0)
            continue;
        bool ok = true;
        float d = np.y * contact.n.y + contact.n.x * np.x + contact.n.z * np.z;
        if (d > 0.0f) {
            ok = false;
        } else if (shapes[i]->shape_type() == 2 && sh->fa8 < 0.05f && d < best) {
            ground = sh;
            best = d;
        }
        if (!(flags & 1)) {
            if (contact.p.z < pos.z + 0.4f && np.z <= 0.0f && shapes[i]->shape_type() == 2 && sh->fa8 > 0.1f) {
                flags |= 1;
                if (type == 1)
                    set_order(0, 0x19e, 0x1000021, 1.0f);
                else
                    set_order(0, 3, 0x1000021, 1.0f);
            } else if (pos.z + 0.4f < contact.p.z && 0.95f < contact.n.z && np.z <= 0.0f) {
                continue;
            }
        }
        if (ok && nh < 16) {
            *hp = contact.n;
            nh++;
            hp++;
        }
    }
    }
    if (nh != 0) {
        int it = 0;
        bool ok2;
        do {
            ok2 = true;
            int jj = nh;
            vector4* hq = hits;
            if (jj > 0) do {
                vector4& n = *hq;
                vector4 nn(-n.x, -n.y, -n.z);
                float tq = np.y * nn.y + np.z * nn.z + nn.x * np.x;
                if (tq > 1e-6f) {
                    ok2 = false;
                    float tx = tq * n.x;
                    float ty = tq * n.y;
                    float tz = tq * n.z;
                    np.x = tx + np.x;
                    np.y = np.y + ty;
                    np.z = np.z + tz;
                    push.x = push.x + tx;
                    push.y = push.y + ty;
                    push.z = push.z + tz;
                }
                hq++;
            } while (--jj != 0);
            it++;
            if (it > 16) {
                if (!ok2) {
                    np.x = 0.0f;
                    np.y = 0.0f;
                    np.z = 0.0f;
                }
                break;
            }
        } while (!ok2);
    }
    face_shape* nf;
    int tries;
    float fz;
    Scene* scene = g_mission->scene;
    if (gv.y * gv.y + gv.z * gv.z + gv.x * gv.x == 0.0f) {
        np = pos;
        nf = 0;
        tries = 0;
    } else {
        np = vector4(np.x + pos.x, np.y + pos.y, np.z + pos.z);
        vector4 raw = np;
        if (np.x < g_mission->bounds[0])
            np.x = g_mission->bounds[0];
        if (np.y < g_mission->bounds[1])
            np.y = g_mission->bounds[1];
        if (np.x > g_mission->bounds[2])
            np.x = g_mission->bounds[2];
        if (np.y > g_mission->bounds[3])
            np.y = g_mission->bounds[3];
        push.x = (np.x - raw.x) + push.x;
        push.y = (np.y - raw.y) + push.y;
        nf = 0;
        tries = 0;
    }
    do {
        scene->floor_at(&np.x, &fz, &nf);
        face_shape* cur = floor;
        if (cur == 0) {
            np = vadd(vscale(0.95f, vsub(np, pos)), pos);
        }
        tries++;
        if (tries > 30) {
            if (nf != 0)
                break;
            nf = cur;
            if (nf == 0)
                nf = scene->faces->first;
            np = pos;
            fz = pos.z;
        }
    } while (nf == 0);
    if (flags & 1) {
        float dz = pos.z - fz;
        if (dt * fac + 0.4f < dz || nf->normal.z < 0.1f) {
            vector4 probe = vadd(vscale(0.4f, unit_up(vsub(np, pos))), pos);
            float fz2;
            face_shape* tf;
            scene->floor_at(&probe.x, &fz2, &tf);
            float dz2 = pos.z - fz2;
            bool up = false;
            if (nh > 0) {
                int jj = nh;
                vector4* hq = hits;
                do {
                    if (hq->z > 0.0f)
                        up = true;
                    hq++;
                } while (--jj != 0);
            }
            if (id == (unsigned)g_mission->local_id || (!(dz2 > 2.5f) && !(fz < g_mission->water - 1.5f))) {
                if (!up)
                    flags &= ~1u;
            } else {
                goto reset_pos;
            }
        } else if (!(dz < -1.5f)) {
            np.z = fz;
            if (dz < -0.06f || 0.06f < dz)
                fa0 = dz + fa0;
        } else {
reset_pos:
            nf = floor;
            np = pos;
        }
    } else if (np.z < fz) {
        np.z = fz;
        flags |= 1;
        int snd;
        if (type == 1) {
            if (defs->count > 0x19e && ((slots[0].m14 & 0x1ffff) < 0x22 || (slots[0].a == -1 && slots[0].b == -1))) {
                slots[0].b = 0x19e;
                slots[0].m14 = 0x1000021;
                slots[0].rate = 1.0f;
                if ((defs->recs[0x19e].flags & 0x400) && slots[1].a != -1 && (slots[1].m14 & 0x1ffff) < 0x21)
                    slots[1].set(-1, 0x21);
            }
            snd = 0x22;
            if (fz < g_mission->water)
                snd = 0x24;
        } else {
            if (defs->count > 3 && ((slots[0].m14 & 0x1ffff) < 0x22 || (slots[0].a == -1 && slots[0].b == -1))) {
                slots[0].b = 3;
                slots[0].m14 = 0x1000021;
                slots[0].rate = 1.0f;
                if ((defs->recs[3].flags & 0x400) && slots[1].a != -1 && (slots[1].m14 & 0x1ffff) < 0x21)
                    slots[1].set(-1, 0x21);
            }
            snd = 0x1e;
            if (fz < g_mission->water)
                snd = 0x20;
        }
        float var = 0.1f;
        play_sound_var(&snd, &var);
        vector4 vn = vel;
        float len2 = (float)sqrt(vn.x * vn.x + vn.z * vn.z + vn.y * vn.y);
        if (len2 > 1e-10f) {
            float inv = 1.0f / len2;
            vn.x = vn.x * inv;
            vn.y = vn.y * inv;
            vn.z = vn.z * inv;
        } else {
            vn.x = 0.0f;
            vn.y = 1.0f;
            vn.z = 0.0f;
        }
        if ((type == 1 && len2 > 11.0f) || len2 > 13.0f) {
            vector4 nrm = nf->normal;
            vector4 nn(-nrm.x, -nrm.y, -nrm.z);
            Blow b;
            b.init();
            vector4 hpos = vector4(np.x, np.y, fz + 0.01f);
            b.v8 = hpos;
            b.v18 = nf->normal;
            b.v28 = nf->normal;
            b.m3c = nn.z * vn.z + nn.y * vn.y + vn.x * nn.x;
            b.m3c = b.m3c * len2;
            b.m3c = b.m3c + b.m3c;
            b.m0 = 0;
            b.m4 = -1;
            b.m38 = (type != 1) ? 2 : 9;
            b.m48 = 2;
            int kind = 5;
            play_hit_sound(&kind);
            if ((type != 0 || mount_id != -1) && type == 1)
                b.m3c = b.m3c + b.m3c;
            reduce_blow(&b);
            if (has_blow == 0 || !(b.m40 < last_blow.m40)) {
                last_blow = b;
                has_blow = 1;
            }
        }
    }
    gv.x = np.x - pos.x;
    gv.y = np.y - pos.y;
    gv.z = np.z - pos.z;
    if (dt > 1e-5f) {
        float k5 = dt * 5.0f;
        float inv = 1.0f / dt;
        v = vector4(gv.x * inv, gv.y * inv, gv.z * inv);
        float om = 1.0f - k5;
        smooth = vector4(om * smooth.x + v.x * k5, om * smooth.y + v.y * k5, om * smooth.z + v.z * k5);
        vec2 pp;
        pp.x = push.x;
        pp.y = push.y;
        if (pp.y * pp.y + pp.x * pp.x > 0.0f) {
            float sp = (float)sqrt(gv.z * gv.z + gv.y * gv.y + gv.x * gv.x) * inv;
            if (sp < fac) {
                if (type == 1 && fac > 3.0f && sp < 0.5f && hit_ok) {
                    sp = 0.0f;
                    set_order(0, 0x19c, 0x1000049, 1.0f);
                    int kind = 4;
                    play_hit_sound(&kind);
                }
                fac = fac - (fac - sp) * dt * 10.0f;
            }
        }
    }
    if (np.z > -9000.0f && np.z < 9000.0f) {
        pos = np;
        floor = nf;
    }
    force = vector4(0.0f, 0.0f, 0.0f);
}
