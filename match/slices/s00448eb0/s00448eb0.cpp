// Slice s00448eb0: Agent AI per-tick control update (0x448eb0, 11.6 KB): picks the movement target (path waypoint,
// followed agent, goal point), steers/jitters the heading, derives the move direction and the keyboard-style input
// bits (+0x53a4) and runs the combat state machine (+0x5398) for ranged and melee weapons.
// All names are provisional; offsets come from this function and from the neighbouring slices.
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <new>
#pragma intrinsic(sin, cos, fabs)
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- item records (0x238 bytes, array at 0x5e29d0; see s00437f00 / s004362e0) -------------------------------------
enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets
};
struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x20c - 0x1cc];
    int f20c;                                           // +0x20c
    char pad210[0x238 - 0x210];
    bool is_ranged();                                   // 0x433d90
    bool is_melee();                                    // 0x433dc0
};
extern ItemKind* g_items;                               // 0x5e29d0

// item index + packed amount word (s00434ad0 ItemRef)
struct Pair {
    int a;
    unsigned b;
    int max_amount();                                   // 0x434b70
    int stat_1f0a();                                    // 0x434d70
};

// ---- the world / mission objects -----------------------------------------------------------------------------------
struct Troop {                                          // 0x508 bytes
    char pad0[0x80];
    float prof[8];                                      // +0x80 (weapon proficiency by weapon class)
    char pada0[0xb0 - 0xa0];
    int skill;                                          // +0xb0
    char padb4[0x508 - 0xb4];
};
struct World {
    char pad0[0x13a00];
    Troop* troops;                                      // +0x13a00
};
extern World* g_world;                                  // 0x5e2930

struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base() {}                            // slot 0
    virtual int shape_type();                           // slot 1
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9();
    virtual void get_min(float* out);                   // slot 10  0x40ad80 (capsule)
    virtual void get_max(float* out);                   // slot 11  0x40ade0 (capsule)
};
struct capsule : shape_base {                           // vtable 0x54a540, 0x64 bytes
    vector4 p0, p1, p2, p3;
    float radius;
    capsule();                                          // 0x408600
};
struct Contact { vector4 n; int rest[5]; };             // 0x24 bytes (first dwords: contact normal)
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, Contact* out);   // 0x40bfe0
extern shape_base* g_hit_shape;                         // 0x5e246c: last shape hit by trace_ray
extern unsigned g_input_mask;                           // 0x5e2474
extern int g_difficulty;                                // 0x5aa04c
extern __int64 g_ticks[];                               // 0x5e2948 (100 kHz clocks)

struct Agent;
struct Entity {                                         // scene entry
    char pad0[0xc];
    unsigned id;                                        // +0xc
    int type;                                           // +0x10 (0xb = agent)
    char pad14[0x98 - 0x14];
    shape_base* shape;                                  // +0x98
};
struct EntVec {                                         // std::vector<Entity*> layout (VC7.1), freed by hand
    int alloc_pad;
    Entity** first;
    Entity** last;
    Entity** end;
};
struct Scene {
    void query_box(const float* bmin, const float* bmax, unsigned mask, shape_base** out, int* count);   // 0x42aa50
    void collect_boxed(const vector4* bmin, const vector4* bmax, EntVec* out);   // 0x442180
};
struct Mission {
    Scene* scene;                                       // +0
    char pad4[0x18 - 4];
    std::vector<Agent*> agents;                         // +0x18 (page table: id >> 4; 16 agents of 0x5928 bytes per page)
    char pad28[0x6528 - 0x28];
    unsigned char f6528;                                // +0x6528
    char pad6529[0x6554 - 0x6529];
    char f6554;                                         // +0x6554
};
extern Mission* g_mission;                              // 0x5e292c

struct AgentTable { Agent* get(unsigned id); };         // 0x43b1f0 (thiscall on mission+4)

struct gui_timer {                                      // 16 bytes
    int clock;
    int pad4;
    __int64 t;
    void set_delay(float seconds);                      // 0x42cd20
    float elapsed() const;                              // 0x432c10
};
struct vec2 {
    float x, y;
    float normalize();                                  // 0x4367c0
};
struct pos_matrix : matrix {
    void rotate_sf(const float* angle);                 // 0x40e770
};
struct AgentBody {
    char pad0[0x58];
    pos_matrix m;                                       // +0x58
};

float __cdecl frand();                                  // 0x40e400  (rand() % 15817) / 15817
float __cdecl accel_factor(const float* a, const float* b, const float* c);   // 0x435ae0
float __cdecl fmax_ref(const float* a, const float* b); // 0x4324d0
float __cdecl clamp_float(const float* x, const float* lo, const float* hi);  // 0x40e3d0
float __cdecl hit_chance(const Pair* who, Agent* a);    // 0x438cb0
int __stdcall in_id_set(int id);                        // 0x432640

struct Agent {                                          // 0x5928 bytes
    char pad0[8];
    unsigned id;                                        // +8
    char padc[4];
    int f10;                                            // +0x10 (1 = mount)
    int f14;                                            // +0x14
    char pad18[0x2c - 0x18];
    int mount_id;                                       // +0x2c (-1 on foot)
    int rider_id;                                       // +0x30
    char pad34[0x3c - 0x34];
    unsigned flags3c;                                   // +0x3c
    char pad40[0x50 - 0x40];
    vector4 pos;                                        // +0x50
    vector4 dir;                                        // +0x60
    char pad70[0x88 - 0x70];
    int troop;                                          // +0x88
    char pad8c[0xac - 0x8c];
    float fac;                                          // +0xac
    char padb0[0xbc - 0xb0];
    int active;                                         // +0xbc
    int fc0;                                            // +0xc0
    char padc4[0x100 - 0xc4];
    int action;                                         // +0x100
    char pad104[0x250 - 0x104];
    AgentBody* body;                                    // +0x250
    char pad254[0x25c - 0x254];
    vector4 goal;                                       // +0x25c
    vector4 goal_dir;                                   // +0x26c
    int goal_face;                                      // +0x27c
    char pad280[0x284 - 0x280];
    int goal_changed;                                   // +0x284
    float walk_scale;                                   // +0x288
    char pad28c[0x294 - 0x28c];
    vector4 move_dir;                                   // +0x294
    char pad2a4[0x2c0 - 0x2a4];
    int f2c0;                                           // +0x2c0
    char pad2c4[0x2c8 - 0x2c4];
    int f2c8;                                           // +0x2c8
    char pad2cc[0x2e0 - 0x2cc];
    int f2e0;                                           // +0x2e0
    char pad2e4[0x318 - 0x2e4];
    unsigned follow_id;                                 // +0x318
    unsigned target_id;                                 // +0x31c
    char pad320[0x1324 - 0x320];
    vector4 path_pos[0x400];                            // +0x1324
    int path_active;                                    // +0x5324
    int path_cur;                                       // +0x5328
    char pad532c[0x5338 - 0x532c];
    unsigned ai_flags;                                  // +0x5338
    unsigned ai_flags2;                                 // +0x533c
    char pad5340[0x5350 - 0x5340];
    float wp_x, wp_y, wp_z;                             // +0x5350
    char pad535c[0x5360 - 0x535c];
    float speed;                                        // +0x5360
    float veer;                                         // +0x5364
    float f5368, f536c;                                 // +0x5368
    gui_timer timer;                                    // +0x5370
    gui_timer timer2;                                   // +0x5380
    unsigned f5390;                                     // +0x5390
    int f5394;                                          // +0x5394
    int ai_state;                                       // +0x5398
    char pad539c[0x53a0 - 0x539c];
    int f53a0;                                          // +0x53a0
    unsigned in_bits;                                   // +0x53a4
    char pad53a8[0x53ac - 0x53a8];
    vector4 prev_move;                                  // +0x53ac
    unsigned in_flags;                                  // +0x53bc
    char pad53c0[0x5570 - 0x53c0];
    float f5570;                                        // +0x5570
    char pad5574[0x55bc - 0x5574];
    shape_base* last_hit;                               // +0x55bc
    char pad55c0[0x55c4 - 0x55c0];
    unsigned f55c4;                                     // +0x55c4
    float max_speed;                                    // +0x55c8
    char pad55cc[0x560c - 0x55cc];
    int f560c;                                          // +0x560c
    int f5610;                                          // +0x5610
    char pad5614[0x56a4 - 0x5614];
    vector4 vel;                                        // +0x56a4
    vector4 v56b4;                                      // +0x56b4
    char pad56c4[0x56e8 - 0x56c4];
    float f56e8;                                        // +0x56e8
    char pad56ec[0x5820 - 0x56ec];
    int clock;                                          // +0x5820
    char pad5824[0x5828 - 0x5824];
    __int64 deadline;                                   // +0x5828
    char pad5830[0x5928 - 0x5830];

    int update_nav_face();                              // 0x43cb30
    void advance_path();                                // 0x43df60
    vector4* local_velocity(vector4* out);              // 0x43b7b0
    bool charged_0x48() const;                          // 0x432470
    bool plan_path();                                   // 0x445910
    float attack_speed();                               // 0x438360
    bool is_enemy(const Agent* o) const;                // 0x432350
    Pair slot_pair(int idx) const;                      // 0x432790
    int fire_check(unsigned id, int* out_blocker);      // 0x441020
    unsigned pick_threat();                             // 0x443430
    bool line_of_sight(unsigned id, shape_base* ignore);// 0x440e30
    bool flag_0x400_or_0x114() const;                   // 0x4354d0
    void play_class_sound(const int* kind);             // 0x4459c0
    int can_hit(unsigned aid, unsigned bid, int alt, int flag, const float* target);   // 0x4406a0

    void ai_update_control();                           // 0x448eb0
};

static __forceinline Agent* agent_by_id(unsigned id)
{
    return &g_mission->agents[id >> 4][id & 0xf];
}

// The slot-class lookup 0x4350c0 uses cl's register convention for same-TU statics (item in EAX, flag on the stack).
// @ 0x4350c0
static __declspec(noinline) int slot_class_impl(int item, int flag)
{
    unsigned t = g_items[item].type_flags & 0xff;
    if (t == itp_type_bow) return 3;
    if (t == itp_type_crossbow) return 4;
    if (t == itp_type_thrown) return 5;
    if (t == itp_type_pistol || t == itp_type_musket) return 6;
    if (t == itp_type_polearm) return 2;
    if (t == itp_type_one_handed_wpn) return 0;
    return flag == 0;
}

// @ 0x448eb0
void Agent::ai_update_control()
{
    vector4* P = &pos;
    vector4 tgt = *P;                                   // movement target
    vector4 d;                                          // direction to the target
    float dlen;
    vector4 eye;
    float att, mz, mlen, strafe;
    unsigned fl2;
    int mid;
    float angle;
    vector4 op;

    if (active == 0)
        return;

    // ---- 1. keep the path / goal up to date ----------------------------------------------------------------------
    if (path_active != 0) {
        if (ai_flags & 2) {
            const vector4* tp = &agent_by_id(follow_id)->pos;
            vector4 dd(tp->x - goal.x, tp->y - goal.y, tp->z - goal.z);
            if ((float)sqrt(dd.y * dd.y + (dd.z * dd.z + dd.x * dd.x)) > 4.0f &&
                goal_face != agent_by_id(follow_id)->update_nav_face()) {
                unsigned fid = follow_id;
                goal = ((AgentTable*)((char*)g_mission + 4))->get(fid)->pos;
                goal_face = ((AgentTable*)((char*)g_mission + 4))->get(fid)->update_nav_face();
                goal_changed = 1;
                path_active = 0;
            }
        }
        if (path_active != 0)
            advance_path();
    }

    // ---- 2. pick the movement target -------------------------------------------------------------------------------
    if (path_active != 0)
        tgt = path_pos[path_cur];
    if (path_active == 0) {
        unsigned fl = ai_flags;
        tgt = *P;
        if (fl != 0) {
            if ((fl & 2) != 0) {
                unsigned fid = follow_id;
                vector4 lv;
                vector4* v = agent_by_id(fid)->local_velocity(&lv);
                float vz = v->z * 0.8f;
                vector4 np(v->x * 0.8f + agent_by_id(fid)->pos.x, v->y * 0.8f + agent_by_id(fid)->pos.y,
                           vz + agent_by_id(fid)->pos.z);
                tgt = np;
                if ((char)fl < 0) {
                    float m1 = -1.0f;
                    float k = 0.4f;
                    if (charged_0x48())
                        k = 0.25f;
                    float m = m1;
                    if (action == 0xb9 || action == 0xb8)
                        m = 1.0f;
                    float s = m * k;
                    float oy = -dir.x * s;
                    tgt.x = tgt.x + dir.y * s;
                    tgt.y = tgt.y + oy;
                }
                goal = agent_by_id(fid)->pos;
                goal_face = agent_by_id(fid)->update_nav_face();
            } else if ((fl & 1) != 0) {
                tgt = goal;
            }
        }
        bool ok = false;
        if (goal_changed != 0 && goal_face != update_nav_face()) {
            if (plan_path()) {
                path_active = 1;
                ok = true;
            } else {
                path_active = 0;
            }
            goal_changed = 0;
            if (ok) {
                advance_path();
                if (path_active != 0)
                    tgt = path_pos[path_cur];
            }
        }
    }

    // ---- 3. heading jitter and obstacle avoidance -----------------------------------------------------------------------
    {
        float base;
        if (f10 == 0 && mount_id < 0) {
            int r = rand() % 0x3dc9;
            base = (float)r * 6.3223115e-06f + 0.88f;
        } else {
            int r = rand() % 0x3dc9;
            base = (float)r * 6.3223115e-06f + 0.85f;
        }
        veer = base * veer;
    }
    d.x = tgt.x - P->x;
    d.y = tgt.y - P->y;
    d.z = tgt.z - P->z;
    dlen = d.normalize();

    if ((flags3c & 1) && dlen > 0.1f) {
        if (fc0 == 0) {
            capsule cap;
            float a = 0.3f;
            float b = 1.0f;
            if (f10 != 0 || mount_id != -1) {
                a = 0.5f;
                b = 2.0f;
            }
            if (b < dlen)
                b = dlen;
            if (b > 5.0f)
                b = 5.0f;
            cap.radius = 0.25f;
            vector4 t1(d.x * a + P->x, d.y * a + P->y, d.z * a + P->z);
            cap.p0 = t1;
            vector4 t2(d.x * b + P->x, d.y * b + P->y, d.z * b + P->z);
            cap.p1 = t2;
            cap.p0.z = cap.p0.z + 1.3f;
            cap.p1.z = cap.p1.z + 1.3f;
            vector4 bmin, bmax;
            cap.get_min(&bmin.x);
            cap.get_max(&bmax.x);
            shape_base* shapes[8188];
            int count;
            g_mission->scene->query_box(&bmin.x, &bmax.x, 2, shapes, &count);
            shape_base* hit;
            int found = 0;
            int i = 0;
            vector4 nrm;
            Contact contact;
            if (count > 0) {
                do {
                    if (found != 0)
                        goto process;
                    {
                        shape_base* sh = shapes[i];
                        found = shape_contact_count(&cap, sh, 1, &contact);
                        if (found != 0) {
                            nrm = contact.n;
                            hit = sh;
                        }
                    }
                    i++;
                } while (i < count);
                if (found != 0) {
process:
                    if (hit->shape_type() != 2) {
                        float cross = d.x * -nrm.y - d.y * -nrm.x;
                        if (cross > 0.0f) {
                            float t = (frand() + 1.0f) * 0.5f;
                            veer = (-0.7f - veer) * t + veer;
                        } else {
                            float t = (frand() + 1.0f) * 0.5f;
                            veer = (0.7f - veer) * t + veer;
                        }
                    }
                }
            }
            if (mount_id >= 0) {
                EntVec ents;
                ents.first = 0;
                ents.last = 0;
                ents.end = 0;
                g_mission->scene->collect_boxed(&bmin, &bmax, &ents);
                Entity** pe = ents.first;
                if (pe < ents.last) {
                    do {
                        Entity* e = *pe;
                        if (e->shape != 0 && e->type == 0xb && e->id != id && e->id != (unsigned)mount_id &&
                            e->id != (unsigned)rider_id) {
                            Agent* o = agent_by_id(e->id);
                            if (o->f10 == 1 && shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                                pos_matrix m = body->m;
                                int flag = 1;
                                float ang = 0.01f;
                                m.rotate_sf(&ang);
                                AgentBody* ob = o->body;
                                float lhs = (float)fabs(m.f.y * ob->m.f.y + m.f.z * ob->m.f.z + m.f.x * ob->m.f.x);
                                if (fabs(rgl::dot(body->m.f, ob->m.f)) > lhs)
                                    flag = -1;
                                veer = (float)flag * 0.1f + veer;
                            }
                        }
                        ++pe;
                    } while (pe < ents.last);
                }
                if (ents.first != 0)
                    operator delete(ents.first);
            }
        }
        // ---- speed-dependent weave ---------------------------------------------------------------------------------
        float nv;
        if (v56b4.length() < 0.3f) {
            if (mount_id >= 0) {
                if (!(speed > 0.9f))
                    goto weave_up;
                if (!(f5570 < -0.95f)) {
                    nv = f5570 - 0.03f;
                } else {
                    f2c8 = 3;
                    nv = f5570 - 0.03f;
                }
            } else {
                if (f10 != 0 || (f5390 & 1) == 0)
                    goto weave_up;
                nv = f5570 - 0.03f;
            }
        } else {
weave_up:
            nv = f5570 + 0.2f;
        }
        f5570 = nv;
        if (f5570 < -1.0f)
            f5570 = -1.0f;
        else if (f5570 > 1.0f)
            f5570 = 1.0f;
        if (f5570 < -0.1f) {
            if ((float)(g_ticks[clock] - deadline) * 1e-5f >= 0.0f) {
                f5570 = 1.0f;
                path_active = 0;
                goal_changed = 1;
                deadline = g_ticks[clock] + 6000000;
            } else {
                if (veer < 0.0f) {
                    int r = rand() % 0x3dc9;
                    veer = veer - ((float)r * 6.3223117e-05f + 0.5f) * (0.3f - f5570);
                } else {
                    int r = rand() % 0x3dc9;
                    veer = ((float)r * 6.3223117e-05f + 0.5f) * (0.3f - f5570) + veer;
                }
                int r = rand() % 0x3dc9;
                if ((float)r * 6.3223117e-05f < 0.01f)
                    veer = -veer;
            }
        }
    }
    if (veer < -0.03f || 0.03f < veer) {
        double ca = cos((double)veer);
        double sa = sin((double)veer);
        vector4 r;
        r.x = (float)(ca * d.x - d.y * sa);
        r.y = (float)((d.y * ca + sa * d.x) * dlen);
        r.z = d.z * dlen;
        vector4 nt(r.x * dlen + P->x, r.y + P->y, r.z + P->z);
        tgt = nt;
    }

    // ---- 4. eye position and move direction ----------------------------------------------------------------------------
    eye = pos;
    {
        float h;
        if (f10 == 0 && mount_id != -1)
            h = 2.4f;
        else
            h = 1.5f;
        eye.z = h + eye.z;
    }
    att = attack_speed();
    fl2 = ai_flags2;
    if ((fl2 & 1) != 0) {
        move_dir = vector4(wp_x - eye.x, wp_y - eye.y, wp_z - eye.z);
    } else if ((fl2 & 2) == 0 || target_id == 0xffffffff) {
        move_dir = vector4(tgt.x - P->x, tgt.y - P->y, tgt.z - P->z);
        if (move_dir.length() < 0.001f)
            move_dir = dir;
    } else {
        Agent* o = agent_by_id(target_id);
        op = o->pos;
        float h;
        if (o->f10 == 0 && o->mount_id != -1)
            h = 2.4f;
        else
            h = 1.5f;
        op.z = (h - 0.2f) + op.z;
        if (o->mount_id < 0 && o->f5610 == 0)
            goto lead;
        if ((fl2 & 0x20) == 0)
            goto lead_done;
        op.z = op.z - (float)(f55c4 & 2) * 0.8f;
lead:
        if ((fl2 & 0x20) != 0 && att > 0.1f) {
            float dx = op.x - eye.x;
            float dy = op.y - eye.y;
            float dz = op.z - eye.z;
            float dist = (float)sqrt(dx * dx + dz * dz + dy * dy);
            float s = 40.0f - (float)g_world->troops[troop].skill;
            if (0.0f > s)
                s = 0.0f;
            float j = (frand() - 0.5f) * s * 0.01f + 1.0f;
            float k = (float)g_world->troops[troop].skill * 0.02f + 0.6f;
            if (!(k < 1.0f))
                k = 1.0f;
            k = (dist / att) * k;
            float ox = (k * o->v56b4.x) * j;
            float oy = (k * o->v56b4.y) * j;
            float oz = (k * o->v56b4.z) * j;
            op.x = ox + op.x;
            op.y = oy + op.y;
            op.z = oz + op.z;
        }
lead_done:
        move_dir = vector4(op.x - eye.x, op.y - eye.y, op.z - eye.z);
    }
    mz = move_dir.z;
    mlen = move_dir.normalize();
    if ((ai_flags2 & 0x20) != 0) {
        float s = 0.2f - (float)g_world->troops[troop].skill * 0.005f;
        if (!(s > 0.0f))
            s = 0.0f;
        int r = rand() % 0x3dc9;
        float jit = ((float)r * 6.3223117e-05f - 0.5f) * s + 1.0f;
        float af = accel_factor(&att, &mlen, &mz);
        move_dir.z = af * jit + move_dir.z;
        move_dir.normalize();
    }

    // ---- 5. movement input bits ---------------------------------------------------------------------------------------
    mid = mount_id;
    d.x = tgt.x - P->x;
    d.y = tgt.y - P->y;
    d.z = tgt.z - P->z;
    strafe = 0.0f;
    f5368 = d.x;
    f536c = d.y;
    angle = 0.0f;
    if (mid == -1 && f10 == 0) {
        float mx = move_dir.x, my = move_dir.y;
        float ml2 = (float)sqrt(my * my + mx * mx);
        double nx, ny;
        if (ml2 > 0.0f) {
            float inv = 1.0f / ml2;
            nx = mx * inv;
            ny = my * inv;
        } else {
            nx = 0.0;
            ny = 1.0;
        }
        float lx = (float)(d.x * ny - d.y * nx);
        float ly = (float)(d.x * nx + d.y * ny);
        float fa, fb;
        if (path_active == 0) {
            fa = walk_scale * 1.4f;
            fb = walk_scale * 0.9f;
        } else {
            fa = 0.0f;
            fb = 0.0f;
        }
        float l2 = (float)sqrt(ly * ly + lx * lx);
        if (l2 > 0.0f) {
            float inv = 1.0f / l2;
            lx = lx * inv;
            ly = ly * inv;
        } else {
            lx = 0.0f;
            ly = 1.0f;
        }
        unsigned f = f5390;
        vec2 mv;
        mv.x = 0.0f;
        mv.y = 0.0f;
        if (((f & 1) != 0 && fb < l2) || ((f & 1) == 0 && fa < l2)) {
            if (((f & 2) != 0 && ly > 0.35f) || ((f & 2) == 0 && ly > 0.45f)) {
                f |= 2;
                in_bits |= 1;
                mv.y = 1.0f;
            } else if (((f & 4) != 0 && ly < -0.35f) || ((f & 4) == 0 && ly < -0.45f)) {
                f |= 4;
                in_bits |= 2;
                mv.y = -1.0f;
            } else {
                f &= 0xfffffff9;
            }
            f5390 = f;
            if (((f & 8) != 0 && lx > 0.35f) || ((f & 8) == 0 && lx > 0.45f)) {
                f |= 8;
                in_bits |= 4;
                mv.x = 1.0f;
            } else if (((f & 0x10) != 0 && lx < -0.35f) || ((f & 0x10) == 0 && lx < -0.45f)) {
                f |= 0x10;
                in_bits |= 8;
                mv.x = -1.0f;
            } else {
                f &= 0xffffffe7;
            }
            f5390 = f;
            f5390 = f | 1;
            mv.normalize();
            strafe = lx * mv.y - mv.x * ly;
        } else {
            f5390 = 0;
        }
        if ((ai_flags & 0x20) != 0 && max_speed > 2.6f)
            in_bits = in_bits | 0x200000;
    } else {
        // ---- follow / mount case ----
        vector4 gp = tgt;
        if (path_active != 0)
            gp = goal;
        d.x = gp.x - P->x;
        d.y = gp.y - P->y;
        d.z = gp.z - P->z;
        float gdist = (float)sqrt(d.x * d.x + d.z * d.z + d.y * d.y);
        float hi = 0.8f, lo = 0.5f;
        if (path_active == 0) {
            hi = walk_scale * 1.2f;
            lo = walk_scale * 0.8f;
        }
        if ((f5390 != 0 && lo < gdist) || (f5390 == 0 && hi < gdist)) {
            f5390 = 1;
            float t_x = tgt.x - P->x;
            float t_y = tgt.y - P->y;
            float h_y = dir.y;
            double hl = sqrt(h_y * h_y + dir.x * dir.x);
            double hx, hy;
            if (hl > 0.0) {
                float inv = (float)(1.0 / hl);
                hx = dir.x * inv;
                hy = h_y * inv;
            } else {
                hx = 0.0;
                hy = 1.0;
            }
            float tl = (float)sqrt(t_y * t_y + t_x * t_x);
            double tx, ty;
            if (tl > 0.0f) {
                float inv = 1.0f / tl;
                tx = t_x * inv;
                ty = t_y * inv;
            } else {
                tx = 0.0;
                ty = 1.0;
            }
            float ang = (float)atan2(-(hy * tx - ty * hx), hy * ty + hx * tx);
            float sp = gdist * 0.7f;
            if (sp < 1.0f)
                sp = 1.0f;
            else if (max_speed <= sp)
                sp = max_speed;
            speed = sp;
            float gv = goal_dir.length();
            if (sp < gv && d.x * goal_dir.x + (d.y * goal_dir.y + d.z * goal_dir.z) > 0.1f)
                speed = sp * 0.5f + gv;
            float other = f56e8;
            if (mid != -1)
                other = agent_by_id(mid)->f56e8;
            if (ang - other > 0.1f && ang > 0.1f)
                in_bits |= 0x80;
            else if (ang - other < -0.1f && ang < -0.1f)
                in_bits |= 0x40;
            if ((f5394 != 0 && ang > 0.1f) || (f5394 == 0 && ang > 0.2f)) {
                f5394 = 1;
                speed = (0.9f - ang * 0.45f) * speed;
            } else if ((f5394 != 0 && ang < -0.1f) || (f5394 == 0 && ang < -0.2f)) {
                f5394 = 1;
                speed = (ang * 0.45f + 0.9f) * speed;
            } else {
                f5394 = 0;
            }
            angle = ang;
            if (speed < 1.2f)
                speed = 1.2f;
            else if (max_speed < speed)
                speed = max_speed;
        } else {
            speed = 0.0f;
            f5390 = 0;
        }
        unsigned who = id;
        if (f10 == 0)
            who = mid;
        Agent* wa = agent_by_id(who);
        if (wa->fac < speed - 0.2f) {
            in_bits = in_bits | 0x10;
        } else if (speed + 0.2f < wa->fac && (ai_flags & 0x200) == 0) {
            in_bits = in_bits | 0x20;
        }
    }

    // ---- 6. combat state machine -----------------------------------------------------------------------------------------
    if (g_mission->f6554 == 0 && f10 == 0 && target_id != 0xffffffff && (ai_flags2 & 2) != 0) {
        Agent* o = agent_by_id(target_id);
        if (!is_enemy(o) || agent_by_id(target_id)->f14 != 1)
            goto no_target;
        o = agent_by_id(target_id);
        vec2 tv;
        tv.x = o->pos.x - P->x;
        tv.y = o->pos.y - P->y;
        float dist = tv.normalize();
        Pair p = slot_pair(0);
        float hc, fm, zt, one;
        unsigned itype;
        ItemKind* wk;
        float reach0, facing, ofacing, wprof, r2f, distL;
        int idx;
        Pair q, q2, op0, op1;
        bool b3e, b3f, b58, b59, b5a, b5b;
        if (p.a == -1 || !g_items[p.a].is_ranged())
            goto melee;

        // ---- ranged weapon ------------------------------------------------------------------------------------------------
        itype = g_items[p.a].type_flags & 0xff;
        hc = hit_chance(&p, this);
        one = 1.0f;
        zt = (o->pos.z - P->z) * 0.7f + dist;
        fm = fmax_ref(&zt, &one);
        if (ai_state == 0) {
            if ((itype == itp_type_crossbow || itype == itp_type_pistol || itype == itp_type_musket) &&
                p.max_amount() == (int)(p.b & 0xff)) {
                timer.t = g_ticks[timer.clock];
                ai_state = 4;
                goto bb72;
            }
            if (fm >= hc) {
set7:
                ai_state = 7;
                goto bb72;
            }
            if (fm < hc && frand() < ((float)g_world->troops[troop].skill + 1.0f) * 0.2f) {
                int blocker;
                int r = fire_check(target_id, &blocker);
                f53a0 = r;
                if (r == 1) {
                    ai_state = 1;
                    float x = (dist + 70.0f) * 0.01f;
                    float lo = 0.7f;
                    float hi = 1.2f;
                    float cl = clamp_float(&x, &lo, &hi);
                    float delay = (((frand() + 2.5f) * cl) * 35.0f) / (float)(g_world->troops[troop].skill + 0x23);
                    if (itype == itp_type_thrown)
                        delay = delay * 0.5f;
                    timer.set_delay(-delay);
                    goto bb72;
                }
                if (r != 0) {
                    if (r == 3) {
                        ai_state = 8;
                    } else {
                        f2e0 = 2;
                        f2c0 = 0;
                    }
                    goto bb72;
                }
                last_hit = g_hit_shape;
                int t = (int)pick_threat();
                if (t < 0)
                    goto set7;
                target_id = t;
            }
            goto bb72;
        }
        if (ai_state == 1) {
            // state 1
            in_flags = in_flags | 0x100;
            if (mount_id == -1) {
                if (dist < 2.5f && (o->mount_id == -1 || o->v56b4.out_length() < 1.5f)) {
                    f2c0 = 0;
                    ai_state = 3;
                }
                if ((itype == itp_type_crossbow || itype == itp_type_bow) && timer.elapsed() > -0.7f)
                    in_bits = in_bits & 0xfffffff0;
            }
            if (!(timer.elapsed() > 0.0f))
                goto bb72;
            {
                int blocker;
                if (fire_check(target_id, &blocker) == 1) {
                    timer.set_delay(-0.3f);
                    ai_state = 3;
                    goto bb72;
                }
            }
            in_flags = in_flags | 0x200;
            goto bb68;
        }
        if (ai_state == 3) {
            int m = mount_id;
            if (m == -1 && dist < 4.0f && (o->mount_id == -1 || o->v56b4.out_length() < 1.0f)) {
                f2c0 = 0;
                ai_state = 0;
            }
            if (m == -1 && (itype == itp_type_crossbow || itype == itp_type_bow))
                in_bits = in_bits & 0xfffffff0;
            if (timer.elapsed() >= 0.0f)
                ai_state = 0;
            goto bb72;
        }
        if (ai_state == 4) {
            int m = mount_id;
            if (m == -1 && dist < 2.5f && (o->mount_id == -1 || o->v56b4.out_length() < 1.5f)) {
                f2c0 = 0;
                ai_state = 0;
            }
            in_flags = in_flags | 0x100;
            if (m == -1)
                in_bits = in_bits & 0xfffffff0;
            if (p.max_amount() != (int)(p.b & 0xff))
                ai_state = 0;
            if (action != 0x7b && timer.elapsed() > 1.0f)
                ai_state = 0;
            goto bb72;
        }
        if (ai_state == 7) {
            if (!(fm < hc))
                goto bb72;
            if (frand() < 0.03f) {
                int t = (int)pick_threat();
                if (t < 0)
                    goto bb72;
                target_id = t;
            } else {
                if (!line_of_sight(target_id, last_hit)) {
                    last_hit = g_hit_shape;
                    goto bb72;
                }
            }
            goto bb68;
        }
        if (ai_state == 8) {
            Agent* t = agent_by_id(target_id);
            vec2 tv2;
            tv2.x = t->pos.x - P->x;
            tv2.y = t->pos.y - P->y;
            tv2.normalize();
            float fx = body->m.f.x;
            float fy = body->m.f.y;
            if (mount_id == -1 || agent_by_id(mount_id)->fac > 0.5f)
                goto bb68;
            unsigned bits;
            if (fy * tv2.y + tv2.x * fx > 0.0f)
                bits = in_bits | 0x40;
            else
                bits = in_bits | 0x80;
            in_bits = bits;
            if (!(tv2.y * fx - fy * tv2.x > 0.3f))
                goto bb72;
            goto bb68;
        }
        goto bb72;

melee:
        // ---- melee / no ranged weapon -----------------------------------------------------------------------------------
        reach0 = (frand() - 0.5f) * (float)(g_difficulty + 1) * 0.15f + 0.9f;
        facing = tv.y * dir.y + tv.x * dir.x;
        ofacing = -tv.x * o->dir.x + -tv.y * o->dir.y;
        {
            double r1 = frand() * 0.2f + 1.0f;
            float lx = (float)(r1 * vel.x + P->x);
            double ly = r1 * vel.y + P->y;
            float lz = (float)(r1 * vel.z) + P->z;
            float olx = (float)(r1 * o->vel.x + o->pos.x);
            double oly = r1 * o->vel.y + o->pos.y;
            float olz = (float)(r1 * o->vel.z) + o->pos.z;
            vector4 dd;
            dd.x = olx - lx;
            dd.y = (float)(oly - ly);
            dd.z = olz - lz;
            idx = p.a;
            float outlen = dd.out_length();
            if (idx < 0)
                idx = 0;
            wk = &g_items[idx];
            q = slot_pair(1);
            b3f = false;
            wprof = 50.0f;
            if (idx != 0) {
                q2 = slot_pair(1);
                int cls = slot_class_impl(p.a, q2.a >= 0);
                if (cls >= 0 && cls < 7)
                    wprof = g_world->troops[troop].prof[cls];
                reach0 = (100.0f / (float)p.stat_1f0a()) * reach0;
            }
            if (ai_state == 3 && f560c == 1)
                reach0 = reach0 * 0.5f;
            vector4 l2(reach0 * o->vel.x + o->pos.x, reach0 * o->vel.y + o->pos.y, reach0 * o->vel.z + o->pos.z);
            vector4 dv(l2.x - P->x, l2.y - P->y, l2.z - P->z);
            float dl = rgl::length(dv);
            float vl = rgl::length(vel);
            float reach2 = vl * reach0;
            int mc = mount_id;
            float distv;
            if (mc < 0) {
                distv = dl - reach2;
            } else {
                float lpx = (float)(reach0 * vel.x + P->x);
                double lpy = reach0 * vel.y + P->y;
                double lpz = reach0 * vel.z + P->z;
                vector4 d3;
                d3.x = l2.x - lpx;
                d3.y = (float)(l2.y - lpy);
                d3.z = (float)(l2.z - lpz);
                float l3 = rgl::length(d3);
                if (l3 < dl - reach2 * 0.5f) {
                    distv = dl - reach2;
                } else {
                    vector4 df = o->pos - pos;
                    distv = rgl::length(df) + 0.2f;
                }
            }
            distL = distv;
            if (q.a >= 0 && (g_items[q.a].type_flags & 0xff) == itp_type_shield)
                b3f = true;
            float reachf = 2.5f;
            if (p.a != -1)
                reachf = (float)g_items[p.a].f20c * 0.01f + 1.1f;
            if (mc >= 0)
                reachf = reachf - 0.3f;
            if (o->mount_id >= 0)
                reachf = reachf + 0.7f;
            op0 = o->slot_pair(0);
            float r2base = 2.5f;
            if (op0.a != -1)
                r2base = (float)g_items[op0.a].f20c * 0.01f + 1.9f;
            r2f = r2base * 1.3f;
            b5b = false;
            op1 = o->slot_pair(1);
            b3e = false;
            b5a = distL < reachf;
            b58 = (ofacing > 0.0f) && (distL < r2f);
            b59 = false;
            if ((char)wk->type_flags == 4 && facing > 0.4f && action == 0x47 && outlen < 19.0f)
                b59 = true;
            if (charged_0x48() && facing > 0.4f)
                b3e = true;
            if (op1.a >= 0 && (g_items[op1.a].type_flags & 0xff) == itp_type_shield)
                b5b = true;
            int st = ai_state;
            if (st < 2) {
                if (b3f && op0.a != -1 && !b3e && !b59) {
                    in_flags = in_flags | 0x200;
                } else if (outlen < 2.8f) {
                    if (g_world->troops[troop].skill >= 0x14 && (b3f || p.a == -1) && !b59 && !b3e)
                        in_flags = in_flags | 0x200;
                }
                if (flag_0x400_or_0x114()) {
                    if (f10 == 0 && mount_id == -1)
                        in_bits = (in_bits & 0xfffffffe) | 2;
                    if (b3f) {
                        timer2.set_delay(-((frand() + frand()) + 1.3f));
                        ai_state = 7;
                    }
                    goto bb72;
                }
                if (b5a && !b3e && (frand() < wprof * 0.004f + 0.5f || mount_id >= 0)) {
                    timer.set_delay(0.0f);
                    ai_state = 3;
                    goto bb72;
                }
                if (b3f) {
                    if (op0.a == -1)
                        goto b53e;
                    if (b3e || !g_items[op0.a].is_melee())
                        goto b535;
                    if (timer2.elapsed() > 0.0f &&
                        ((o->f560c == 1 && frand() < 0.3f - (float)g_world->troops[troop].skill * 0.01f) ||
                         (o->f560c == 2 && frand() < (float)g_world->troops[troop].skill * 0.05f + 0.5f)) &&
                        b58) {
                        goto b508;
                    }
                    goto b535;
                }
b535:
                if (op0.a == -1) {
b53e:
                    if (p.a == -1 && timer2.elapsed() > 0.0f && ofacing > 0.0f &&
                        ((o->f560c == 1 && frand() < (float)g_world->troops[troop].skill * 0.01f + 0.5f) ||
                         (o->f560c == 2 && frand() < (float)g_world->troops[troop].skill * 0.05f + 0.5f)) &&
                        outlen < r2f) {
b508:
                        timer2.set_delay(-((frand() + frand()) + 1.3f));
                        ai_state = 7;
                        goto bb72;
                    }
                }
                if (!b3f && !b3e && wk->is_melee() && (wk->type_flags & 0x4000) == 0 && timer2.elapsed() > 0.0f &&
                    op0.a != -1 && g_items[op0.a].is_melee() && o->f560c != 0 && b58 &&
                    frand() < wprof * 0.01f + 0.1f) {
                    timer2.set_delay(-(frand() * 0.5f + 0.6f));
                    ai_state = 2;
                    goto bb72;
                }
                if (dist < 15.0f && frand() < 0.005f && mount_id == -1 && (g_mission->f6528 & 1) == 0 &&
                    (!(frand() < 0.2f) || (g_mission->f6528 & 1) != 0)) {
                    int kind = 4;
                    play_class_sound(&kind);
                }
                goto bb72;
            }
            if (st == 2) {
                in_flags = in_flags | 0x200;
                if (timer2.elapsed() > 0.0f)
                    ai_state = 0;
                if (o->f560c != 0 || !(frand() < 0.9f))
                    goto bb72;
                if (frand() < 0.4f) {
                    timer.set_delay(-0.2f);
                    ai_state = 3;
                    goto bb72;
                }
                goto bb68;
            }
            if (st == 3) {
                if (frand() < 0.02f && mount_id == -1 && (g_mission->f6528 & 1) == 0) {
                    int kind = 4;
                    play_class_sound(&kind);
                }
                unsigned fl = in_flags;
                int fa = f560c;
                in_flags = fl | 0x100;
                if (fa <= 4 && timer.elapsed() > 0.0f && fa == 1) {
                    if (b5a) {
                        int sv = p.stat_1f0a();
                        int sk = g_world->troops[troop].skill + 0x28;
                        timer.set_delay(-((4.9f / (float)sk + (float)g_difficulty * 0.25f + 0.6f) * (100.0f / (float)sv)));
                        ai_state = 4;
                    } else if (timer.elapsed() > 8.0f || reach0 * 4.0f < distL) {
                        in_flags = fl | 0x300;
                        ai_state = 0;
                    }
                }
                if (flag_0x400_or_0x114())
                    goto bb68;
                goto bb72;
            }
            if (st == 4) {
                if (timer.elapsed() > 0.0f) {
                    ai_state = 0;
                } else if (in_id_set(action)) {
                    ai_state = 0;
                } else {
                    if (b5b && o->f5610 != 0 && ofacing > 0.0f) {
                        if (frand() < 0.03f)
                            ai_state = 0;
                    } else if (g_items[op0.a].is_melee() && !b5b && ofacing > 0.0f) {
                        float zero[3];
                        zero[0] = 0.0f;
                        zero[1] = 0.0f;
                        zero[2] = 0.0f;
                        if (can_hit(id, target_id, 0, 0, zero) == 0) {
                            if (wprof > 100.0f && frand() < 0.03f)
                                ai_state = 0;
                        } else if (frand() < ((4.0f - (float)g_difficulty) * wprof) * 0.0005f) {
                            ai_state = 0;
                        }
                    }
                }
                if (flag_0x400_or_0x114())
                    goto bb68;
                goto bb72;
            }
            if (st == 7) {
                in_flags = in_flags | 0x200;
                if ((op0.a == -1 || g_items[op0.a].is_melee()) && o->f560c == 0) {
                    if (frand() < 0.4f) {
                        timer2.set_delay(0.0f);
                        timer.set_delay(-0.2f);
                        ai_state = 3;
                    } else {
                        timer2.set_delay(0.0f);
                        ai_state = 0;
                    }
                }
                if (timer2.elapsed() > 0.0f)
                    goto bb68;
                goto bb72;
            }
            goto bb68;
        }
    } else {
no_target:
        if ((g_input_mask | 0x100) != 0 && (g_input_mask | 0x200) == 0)
            in_flags = in_flags | 0x300;
    }
bb68:
    ai_state = 0;
bb72:
    prev_move = move_dir;
    if ((!(strafe < -0.001f) || !(strafe < 0.001f)) && (in_bits & 0xf) != 0 &&
        ((in_flags | g_input_mask) & 0x100) == 0 && ai_state != 4) {
        double a = -asin((double)strafe);
        float c = (float)cos(a);
        double s = sin(a);
        float py = prev_move.y;
        prev_move.y = (float)(s * prev_move.x + c * prev_move.y);
        prev_move.x = (float)(c * prev_move.x - s * py);
    }
    prev_move.normalize();
}
