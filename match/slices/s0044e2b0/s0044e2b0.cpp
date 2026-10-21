// Slice s0044e2b0: Agent::shoot (0x44e2b0, fires a missile from the agent's hand with aim error) and
// rgl::mesh::add_quad (0x44e970, axis-aligned textured quad). Names provisional.
#include <stdlib.h>
#include <math.h>
#include "mesh.h"
#include "rgl.h"

// ---- agent / mission layouts (offsets from this function; see s004465e0, s0044bc30, s00437f00) ----------------
struct Pair { int a; unsigned b; };

struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                        // +0x1c8
    char pad1cc[4];
    unsigned __int64 flags;                     // +0x1d0 (itcf_* bits; itcf_shoot_mask 0xff000)
    char pad1d8[0x238 - 0x1d8];
};
extern ItemKind* g_items;                       // 0x5e29d0

struct Agent;
struct Missile {                                // 0xb0 bytes, 16 per page (s0044bc30)
    char pad0[0x98];
    float f98;                                  // +0x98 (6.0 for itcf_throw_axe, 12.0 for itcf_throw_knife, else 0)
    char pad9c[0xb0 - 0x9c];
};
struct Mission {
    char pad0[0x1c];
    Agent** agent_pages;                        // +0x1c (first of the page vector at +0x18; id >> 4)
    char pad20[0x48 - 0x20];
    Missile** missile_pages;                    // +0x48 (first of the missile page vector at +0x44)
    char pad4c[0x5c - 0x4c];
    int player_id;                              // +0x5c
    char pad60[0x6610 - 0x60];
    float f6610;                                // +0x6610
    Missile& missile(unsigned id) { Missile** pp = &missile_pages[id >> 4]; return *(*pp + (id & 0xf)); }
    int spawn_missile(const rgl::vector4* pos, const rgl::vector4* dir, Pair* launcher, Pair* ammo,
                      unsigned shooter_id);     // 0x44be50
};
extern Mission* g_mission;                      // 0x5e292c

struct World {
    bool is_night();                            // 0x435740 (ecx = g_world, otherwise unused)
};
extern World* g_world;                          // 0x5e2930
extern int g_int_5aa04c;                        // 0x5aa04c

struct BoneRec {                                // 0x1dc bytes (Bone of s00422130): world frame at +0x84
    char pad0[0x84];
    rgl::matrix world;                          // +0x84 (origin at +0xb4)
    char pad_c4[0x1dc - 0xc4];
};
struct Animator {                               // *(*(agent+0x250)+0x304) (s00422130)
    char pad0[4];
    BoneRec* bones;                             // +4
    char pad8[0x50 - 8];
    char done;                                  // +0x50
    char pad51[0x64 - 0x51];
    int f64;                                    // +0x64
    void update();                              // 0x422130
    void update_poses();                        // 0x422080
};
struct AgentBody {
    char pad0[0x58];
    rgl::matrix m;                              // +0x58
    char pad98[0x304 - 0x98];
    Animator* skel;                             // +0x304
};

struct Agent {
    char pad0[8];
    int id;                                     // +8
    char padc[0x2c - 0xc];
    int f2c;                                    // +0x2c (mount agent id, -1 = on foot)
    char pad30[0x50 - 0x30];
    rgl::vector4 pos;                           // +0x50
    rgl::vector4 dir;                           // +0x60
    char pad70[0x1d0 - 0x70];
    Pair held[11];                              // +0x1d0
    int pad228;
    int slots[5];                               // +0x22c
    char pad240[0x250 - 0x240];
    AgentBody* body;                            // +0x250
    char pad254[0x31c - 0x254];
    int target;                                 // +0x31c
    char pad320[0x55d8 - 0x320];
    rgl::vector4 f55d8;                         // +0x55d8
    char pad55e8[0x5634 - 0x55e8];
    float spread;                               // +0x5634
    char pad5638[0x56b4 - 0x5638];
    rgl::vector4 vel;                           // +0x56b4
    char pad56c4[4];
    float f56c8;                                // +0x56c8
    char pad56cc[0x5928 - 0x56cc];

    __forceinline Pair cur_pair() const
    {
        const Pair* p = 0;
        if (slots[0] >= 0) p = &held[slots[0]];
        Pair r;
        r.a = -1;
        if (p) { r.a = p->a; r.b = p->b; }
        return r;
    }
    __forceinline Pair slot_pair_inl(int idx) const
    {
        const Pair* p = 0;
        int i = slots[idx];
        if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &held[i];
        Pair r;
        r.a = -1;
        if (p) { r.a = p->a; r.b = p->b; }
        return r;
    }
    Pair best_ammo(int idx, int* missing, int* total, int* slot_out);   // 0x436880
    void shoot();                               // 0x44e2b0
};
rgl::vector4* __cdecl rotate_scale_xy(rgl::vector4* out, const rgl::vector4* in, const float* angle, const rgl::vector4* scale);   // 0x436210
void __cdecl rand_cone_dir(rgl::vector4* out, rgl::vector4 dir, float spread);   // 0x432c40

// The slot-class lookup 0x4350c0 uses cl's register convention for same-TU statics (item in EAX, flag on the
// stack): reproduced by defining it here as a static (copy of the body from s00437f00).
// @ 0x4350c0
static __declspec(noinline) int slot_class_impl(int item, int flag)
{
    unsigned t = g_items[item].type_flags & 0xff;
    if (t == 7) return 3;
    if (t == 8) return 4;
    if (t == 9) return 5;
    if (t == 0xf || t == 0x10) return 6;
    if (t == 4) return 2;
    if (t == 2) return 0;
    return flag == 0;
}

using rgl::vector4;
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

static __forceinline vector4 vscale2(const vector4& v, float s) { return vector4(v.x * s, v.y * s, v.z * s); }
static __forceinline vector4 vadd2(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 xf(const rgl::matrix* m, const vector4& k)
{
    vector4 T;
    T.x = k.z * m->u.x + k.y * m->f.x + k.x * m->s.x + m->o.x;
    T.y = k.z * m->u.y + k.y * m->f.y + k.x * m->s.y + m->o.y;
    T.z = k.z * m->u.z + k.y * m->f.z + k.x * m->s.z + m->o.z;
    return T;
}

// @ 0x44e2b0
void Agent::shoot()
{
    Pair cur = cur_pair();
    ItemKind* ik = &g_items[cur.a];

    Animator* an = body->skel;
    if (!an->done) {
        if (an->f64) an->update_poses(); else an->update();
    }
    BoneRec* bones = an->bones;
    const rgl::matrix* W = &bones[9].world;
    vector4 P(W->o.x * 0.65f, W->o.y * 0.65f, W->o.z * 0.65f);
    an = body->skel;
    if (!an->done) {
        if (an->f64) an->update_poses(); else an->update();
    }
    const vector4* o19 = &an->bones[19].world.o;
    const rgl::matrix* m = &body->m;
    float kx = o19->x * 0.35f + P.x;
    float ky = o19->y * 0.35f + P.y;
    float kz = o19->z * 0.35f + P.z;
    P.x = kz * m->u.x + ky * m->f.x + kx * m->s.x + m->o.x;
    P.y = kz * m->u.y + ky * m->f.y + kx * m->s.y + m->o.y;
    P.z = kz * m->u.z + ky * m->f.z + kx * m->s.z + m->o.z;
    P.x = W->f.x * 0.15f + P.x;
    P.y = W->f.y * 0.15f + P.y;
    P.z = W->f.z * 0.15f + P.z;

    vector4 d0 = dir;
    if (f2c >= 0) {
        vector4 tmp;
        d0 = *rotate_scale_xy(&tmp, &dir, &f56c8, &f55d8);
    }
    float spr = spread;
    Mission* ms = g_mission;
    int tid = target;
    if (id != ms->player_id && tid >= 0) {
        Pair c2 = slot_pair_inl(1);
        float cls = (float)slot_class_impl(cur.a, c2.a >= 0);
        Agent* tg = &ms->agent_pages[tid >> 4][tid & 0xf];
        vector4 rel = vsub(tg->vel, vel);
        vector4 diff = vsub(tg->pos, pos);
        vector4 dn = diff;
        float dist = dn.normalize();
        vector4 q = rgl::vadd(diff, rel);
        q.normalize();
        float cx = dn.y * q.z - dn.z * q.y;
        float cy = dn.z * q.x - q.z * dn.x;
        float cz = q.y * dn.x - dn.y * q.x;
        float err = cls + 20.0f;
        float g5 = (float)g_int_5aa04c * 0.5f;
        float cm = (float)sqrt(cx * cx + cz * cz + cy * cy);
        float rm = (float)sqrt(rel.x * rel.x + rel.z * rel.z + rel.y * rel.y);
        float A2 = err;
        float t2 = (g5 + 0.5f) * (((10.0f / (dist + 10.0f)) * (cm * rm)) * 0.3f / err);
        float t1 = (g5 + 1.0f) * (0.65f / A2);
        err = t1 + t2;
        if (err > 0.2f) err = 0.2f;
        if (ms->f6610 < 200.0f) err = dist / ms->f6610 * 0.05f + err;
        if (g_world->is_night()) err = dist * 0.001f + err;
        d0.x = (0.5f - (float)(rand() % 0x3dc9) * 6.322312e-05f) * err + d0.x;
        d0.y = (0.5f - (float)(rand() % 0x3dc9) * 6.322312e-05f) * err + d0.y;
        d0.z = (0.5f - (float)(rand() % 0x3dc9) * 6.322312e-05f) * err + d0.z;
        d0.normalize();
    }
    int missing, total, slot_out;
    Pair ammo = best_ammo(-1, &missing, &total, &slot_out);
    vector4 shot;
    rand_cone_dir(&shot, d0, spr);
    Pair launcher = cur_pair();
    unsigned mid = g_mission->spawn_missile(&P, &shot, &launcher, &ammo, id);
    unsigned __int64 shoot_bits = ik->flags & 0xff000ui64;
    if (shoot_bits == 0x30000ui64) {
        g_mission->missile(mid).f98 = 6.0f;
        return;
    }
    if (shoot_bits == 0x20000ui64) {
        g_mission->missile(mid).f98 = 12.0f;
        return;
    }
    g_mission->missile(mid).f98 = 0.0f;
}

namespace rgl {

// 0x412f50 add_point, 0x413c50 add_vertex, 0x411920 add_face: defined elsewhere.

// @ 0x44e970
void mesh::add_quad(const vector4* p0, const vector4* p1, const vec2* uv0, const vec2* uv1, const unsigned* color)
{
    int i0 = add_point(p0);
    vector4 q;
    q.x = p1->x; q.y = p0->y; q.z = p0->z;
    int i1 = add_point(&q);
    q.x = p1->x; q.y = p1->y; q.z = p0->z;
    int i2 = add_point(&q);
    q.x = p0->x; q.y = p1->y; q.z = p0->z;
    int i3 = add_point(&q);
    vector4 n(0.0f, 0.0f, 1.0f);
    int f5c = 0;
    int v0 = add_vertex(i0, uv0, color, &n, &f5c);
    vec2 u1(uv1->x, uv0->y);
    f5c = 0;
    int v1 = add_vertex(i1, &u1, color, &n, &f5c);
    f5c = 0;
    int v2 = add_vertex(i2, uv1, color, &n, &f5c);
    vec2 u3(uv0->x, uv1->y);
    f5c = 0;
    int v3 = add_vertex(i3, &u3, color, &n, &f5c);
    add_face(v0, v1, v2);
    add_face(v0, v2, v3);
    slots[0].f28 = 1;
    slots[1].f28 = 1;
}

} // namespace rgl
