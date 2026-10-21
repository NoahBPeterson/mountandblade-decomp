// Slice s0043f6d0: agent hit-testing against the animation skeleton's capsules and the ground-contact /
// facing helpers that decide which hit-reaction (animation index) an agent plays. Names are provisional;
// every offset comes from the three functions 0x43f6d0, 0x43f970 and 0x43fd10.
#include <vector>
#include <stdlib.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- collision shapes (see s00408110 / s0043a280) ----------------------------------------------------
struct pos;
struct stream;
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}
};
// vtable 0x54a540 (rgl::capsule, 0x64 bytes); the constructor is inlined into the callers.
struct capsule : shape_base {
    vector4 v20, v30, v40, v50;
    float radius;
    capsule() {}
    virtual ~capsule() {}
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void scale(const float* s);              // slot 7   0x408720
    virtual void transform(const matrix& m);         // slot 8   0x408770
};
struct contact { vector4 a; vector4 point; int extra; };          // 0x24 bytes
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, contact* out);    // 0x40bfe0

// 0x40e870: world = this * m (matrix product returned through a hidden pointer, thiscall)
struct posm : matrix {
    matrix mul(const matrix& m) const;               // 0x40e870
    void rotate_fu(const float& angle);              // 0x40e570
    vector4 transform_point(const vector4& p) const; // 0x408460
};

// ---- skeleton / animator (see s0043e514) -------------------------------------------------------------
struct CapDef { vector4 a; vector4 b; float r; int pad[2]; };     // 0x2c bytes
struct ChanRec {                                      // 0xdc bytes
    char pad0[0xcc];
    std::vector<CapDef> caps;                         // +0xcc (first +0xd0, last +0xd4)
};
struct SkelData {
    char pad0[0x18];
    int count;                                        // +0x18
    ChanRec* items;                                   // +0x1c
};
struct Skeleton {
    int nbones;                                       // +0
    char* recs;                                       // +4 (0x1dc-byte records, matrix at +0x84)
    SkelData* data;                                   // +8
    char pad0c[0x50 - 0xc];
    char done;                                        // +0x50
    char pad51[0x64 - 0x51];
    int has_poses;                                    // +0x64
    void update();                                    // 0x422130
    void update_poses();                              // 0x422080
};
struct AgentBody {
    char pad0[0x58];
    posm m;                                           // +0x58
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                   // +0x304
};

struct Pair { int a; unsigned b; };                   // item index + amount word (see s004362e0)

struct Mission {
    char pad0[0x1c];
    char** agents;                                    // +0x1c (page table: id >> 4, 0x5928 bytes per agent)
    char pad20[0x5c - 0x20];
    int player_id;                                    // +0x5c
    char pad60[0x6624 - 0x60];
    posm view;                                        // +0x6624
};
extern Mission* g_mission;                            // 0x5e292c
struct ItemKind { char pad0[0x1d0]; unsigned __int64 flags; char pad1d8[0x238 - 0x1d8]; };    // 0x238 bytes
extern ItemKind* g_items;                             // 0x5e29d0
extern int g_ctl_mode;                                // 0x5aa074 (0 = target, 1 = mouse, 2 = mirrored mouse)
extern int g_invert;                                  // 0x5aa01c
extern float g_stick_x;                               // 0xa178e0
extern float g_stick_y;                               // 0xa178e4
float __cdecl blend_target(void* a, void* b);         // 0x4381e0
struct Skel9 { char* rec(int i); };                   // 0x43f260 (s0043edf0)

struct Agent {
    char pad0[8];
    int id;                                           // +8
    char pad0c[0x10 - 0xc];
    int f10;                                          // +0x10
    char pad14[0x2c - 0x14];
    int f2c;                                          // +0x2c (target / weapon slot id, -1 = none)
    char pad30[0x50 - 0x30];
    vector4 pos;                                      // +0x50
    char pad60[0x80 - 0x60];
    float scale;                                      // +0x80
    char pad84[0xac - 0x84];
    float fac;                                        // +0xac
    char padb0[0x250 - 0xb0];
    AgentBody* body;                                  // +0x250
    char pad254[0x55d8 - 0x254];
    char blk55d8[0x55ec - 0x55d8];                    // +0x55d8 (aim block, see blend_target)
    int snd_a, snd_b, snd_c;                          // +0x55ec / +0x55f0 / +0x55f4 (shoot animation ids)
    int mode1;                                        // +0x55f8 (attack direction chosen for hand 0)
    int snd2_a, snd2_b, snd2_c;                       // +0x55fc / +0x5600 / +0x5604
    int mode2;                                        // +0x5608
    char pad560c[0x56c4 - 0x560c];
    int variant;                                      // +0x56c4
    bool capsule_hit(shape_base* other, vector4* out_point, vector4* out_a, int* out_bone);   // 0x43f6d0
    int pick_attack_dir(unsigned target_id);          // 0x43f970
    void start_shoot(int slot);                       // 0x43fd10
    Pair slot_pair(int idx) const;                    // 0x432790
    int find_target();                                // 0x43b9e0
    void pick_sounds(int key, int mode, int* snd_a, int* snd_b, int* snd_c, int* variant);   // 0x4362e0
};

// rgl::vector4 operator*(v, s) 0x407cd0 and operator*(s, v) 0x407d00, as inlined into callers.
static __forceinline vector4 vmul(const vector4& v, const float& k) { return vector4(v.x * k, v.y * k, v.z * k); }
static __forceinline vector4 smul(const float& k, const vector4& v) { return vector4(k * v.x, k * v.y, k * v.z); }

// @ 0x43f6d0
bool Agent::capsule_hit(shape_base* other, vector4* out_point, vector4* out_a, int* out_bone)
{
    capsule c;
    SkelData* sd = body->skel->data;
    for (int i = 0; i < sd->count; i++) {
        for (unsigned j = 0; ; j++) {
            if (j >= sd->items[i].caps.size())
                break;
            const CapDef& d = sd->items[i].caps[j];
            c.v40 = smul(scale, d.a);
            c.v50 = vmul(d.b, scale);
            c.radius = d.r * scale;
            Skeleton* sk = body->skel;
            if (!sk->done) {
                if (sk->has_poses)
                    sk->update_poses();
                else
                    sk->update();
            }
            c.transform(body->m.mul(*(matrix*)(sk->recs + i * 0x1dc + 0x84)));
            contact ct;
            if (shape_contact_count(&c, other, 1, &ct)) {
                *out_point = ct.point;
                *out_a = ct.a;
                *out_bone = i;
                return true;
            }
        }
    }
    return false;
}

// Agent-id -> agent pointer (page table lookup, as inlined by callers).
static __forceinline Agent* agent_by_id(unsigned id)
{
    return (Agent*)(g_mission->agents[id >> 4] + (id & 0xf) * 0x5928);
}

// Which of the four attack directions (1 left, 2 right, 3 up?, 0 thrust) the agent picks; derived from the
// mouse movement (mode > 0) or from where the target stands relative to the view (mode 0).
// @ 0x43f970
int Agent::pick_attack_dir(unsigned id)
{
    int alt = 0;
    int dir;
    if (g_ctl_mode > 0) {
        float dx = g_stick_x;
        float dy = g_stick_y;
        if (g_invert != 0)
            dy = -dy;
        if (g_ctl_mode == 2) {
            dx = -dx;
            dy = -dy;
        }
        if (dx > dy) {
            if (dx > -dy) {
                dir = 2;
            } else {
                dir = 0;
                alt = 3;
            }
        } else {
            dir = 1;
            if (!(dx < -dy))
                dir = 3;
        }
        int f = f2c;
        if (f < 0)
            goto done;
        float v = blend_target((char*)this + 0x60, blk55d8);
        if (1.0f < agent_by_id(f)->fac) {
            if (v < -0.2f && dir == 2) {
                dir = 1;
                alt = 0;
                goto done;
            }
            if (!(0.2f < v) || dir != 1)
                goto done;
            alt = 0;
        } else {
            if (v < -0.2f && dir == 2) {
                dir = 1;
                goto done;
            }
            if (!(0.2f < v) || dir != 1)
                goto done;
        }
        dir = 2;
    } else {
        Agent* t = agent_by_id(id);
        AgentBody* tb = t->body;
        if (tb == 0)
            return 0;
        posm cam = g_mission->view;
        cam.rotate_fu(-1.5707964f);
        vector4 p = t->pos;
        p.z = p.z + 0.7f;
        if (t->f10 == 0)
            p = t->body->m.transform_point(*(vector4*)(((Skel9*)tb->skel)->rec(9) + 0x30));
        vector4 loc = cam.transform_point_to_local(p);
        float inv = 1.0f / loc.y;
        float x = loc.x * inv;
        float z = inv * loc.z;
        float a = 0.4f * z;
        if (x < a - 0.02f && x < -0.02f - a) {
            dir = 1;
            alt = 0;
        } else if (a + 0.02f < x && 0.02f - a < x) {
            dir = 2;
            alt = 0;
        } else if (z < -0.1f) {
            dir = 3;
            alt = 0;
        } else {
            dir = 0;
            alt = 2;
            if (!(0.0f < x))
                alt = 1;
        }
        int f = f2c;
        if (f < 0)
            goto done;
        float v = blend_target((char*)this + 0x60, blk55d8);
        if (1.0f < agent_by_id(f)->fac) {
            alt = 0;
            dir = (v < 0.0f) ? 1 : 2;
        } else {
            if (v < 0.0f && dir == 2) {
                dir = 1;
                goto done;
            }
            if (!(0.0f < v) || dir != 1)
                goto done;
            dir = 2;
        }
    }
done:
    int snd_a_, snd_b_, snd_c_, var_;
    pick_sounds(0, dir, &snd_a_, &snd_b_, &snd_c_, &var_);
    if (snd_a_ == -1)
        dir = alt;
    return dir;
}

// itcf_shoot_* (module system header_items): the ranged-attack capability bits of an item.
enum {
    itcf_shoot_bow      = 0x1000,
    itcf_shoot_crossbow = 0x4000,
    itcf_throw_stone    = 0x10000,
    itcf_throw_knife    = 0x20000,
    itcf_throw_axe      = 0x30000,
    itcf_throw_javelin  = 0x40000,
    itcf_shoot_pistol   = 0x70000,
    itcf_shoot_musket   = 0x80000,
    itcf_shoot_mask     = 0xff000,
};

// Picks the animation and sound ids for starting a ranged attack with the item in the given slot: fixed ids
// for each itcf_shoot_* class, otherwise the pair chosen by pick_attack_dir / pick_sounds.
// @ 0x43fd10
void Agent::start_shoot(int slot)
{
    snd_a = -1;
    snd_b = -1;
    snd_c = -1;
    Pair sp = slot_pair(slot);
    unsigned __int64& flags = *(unsigned __int64*)&sp;
    if (sp.a == -1)
        flags = 0;
    else
        flags = g_items[sp.a].flags;
    int dir, alt;
    if (f2c >= 0) {
        float v = blend_target((char*)this + 0x60, blk55d8);
        if (v < 0.0f)
            dir = 1;
        else
            dir = 2;
        alt = 0;
    } else {
        dir = rand() % 4;
        alt = rand() % 4;
        if (dir == alt)
            alt = 3 - dir;
    }
    int tgt = find_target();
    if (id == g_mission->player_id && (g_ctl_mode != 0 || tgt != -1))
        dir = pick_attack_dir(tgt);
    unsigned __int64 kind = flags & itcf_shoot_mask;
    if (kind == 0) {
        if (slot == 0) {
            mode1 = dir;
            pick_sounds(0, dir, &snd_a, &snd_b, &snd_c, &variant);
            if (snd_a == -1) {
                mode1 = alt;
                pick_sounds(0, alt, &snd_a, &snd_b, &snd_c, &variant);
            }
        } else {
            mode2 = dir;
            pick_sounds(slot, mode1, &snd2_a, &snd2_b, &snd2_c, &variant);
            if (snd2_a == -1) {
                mode2 = alt;
                pick_sounds(slot, mode1, &snd2_a, &snd2_b, &snd2_c, &variant);
            }
        }
        return;
    }
    if (kind == itcf_shoot_bow) {
        snd_b = 0x76;
        mode1 = 0;
        snd_a = 0x75;
        return;
    }
    if (kind == itcf_shoot_crossbow) {
        snd_b = 0x7a;
        mode1 = 0;
        snd_a = 0x79;
        return;
    }
    if (kind == itcf_shoot_pistol) {
        snd_b = 0x86;
        mode1 = 0;
        snd_a = 0x85;
        return;
    }
    if (kind == itcf_shoot_musket) {
        snd_b = 0x89;
        mode1 = 0;
        snd_a = 0x88;
        return;
    }
    if (kind == itcf_throw_stone) {
        snd_b = 0x84;
        mode1 = 0;
        snd_a = 0x83;
        return;
    }
    if (kind == itcf_throw_knife) {
        snd_b = 0x80;
        mode1 = 0;
        snd_a = 0x7f;
        return;
    }
    if (kind == itcf_throw_axe) {
        snd_b = 0x82;
        mode1 = 0;
        snd_a = 0x81;
        return;
    }
    if (kind == itcf_throw_javelin) {
        snd_a = 0x7d;
        snd_b = 0x7e;
    }
    mode1 = 0;
}
