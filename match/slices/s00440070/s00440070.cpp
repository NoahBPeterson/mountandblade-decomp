// Slice s00440070: two agent combat-input helpers (names provisional): the sound-id triple an agent plays for its
// current attack direction and weapon class (0x440070, the sibling of Agent::pick_attack_dir / pick_sounds in
// s0043f6d0 / s004362e0), and the "can this block/hit the other agent now" geometry test (0x4406a0).
#include <vector>
#include <math.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- shared with s0043f6d0 --------------------------------------------------------------------------------
struct posm : matrix {
    matrix mul(const matrix& m) const;                 // 0x40e870
    void rotate_fu(const float& angle);                // 0x40e570
    vector4 transform_point(const vector4& p) const;   // 0x408460
};
struct ChanRec { char pad[0xd8]; float charge; };      // animation channel (see s00432130 Z_)
struct Skel9 {
    char pad0[0x40];
    ChanRec* chans;                                    // +0x40
    char* rec(int i);                                  // 0x43f260 (s0043edf0): animator record i, brought up to date first
};
struct AgentBody {
    char pad0[0x58];
    posm m;                                            // +0x58
    char pad98[0x304 - 0x98];
    Skel9* skel;                                       // +0x304
};
struct vec2 { float x, y; };
struct Pair { int a; unsigned b; };                    // item index + amount word

struct Agent;
struct Mission {
    char pad0[0x18];
    std::vector<Agent*> agents;                        // +0x18 (page table: id >> 4, 16 agents of 0x5928 bytes per page)
    char pad28[0x5c - 0x28];
    int player_id;                                     // +0x5c
    char pad60[0x6624 - 0x60];
    posm view;                                         // +0x6624
};
struct posm;
extern Mission* g_mission;                             // 0x5e292c
struct ItemKind { char pad0[0x1d0]; unsigned __int64 flags; char pad1d8[0x238 - 0x1d8]; };   // 0x238 bytes
extern ItemKind* g_items;                              // 0x5e29d0
extern int g_stick_mode;                               // 0x5aa078 (0 = none, 1 = stick; see s0043f6d0 g_ctl_mode at 0x5aa074)
extern int g_invert;                                   // 0x5aa01c
extern float g_stick_x;                                // 0xa178e0
extern float g_stick_y;                                // 0xa178e4

// itcf_* capability bits (Module System header_items), stored in the high dword of the item flags
#define ITCF(x) (((unsigned __int64)(x)) << 32)
enum { itcf_thrust_polearm = 0x100, itcf_shoot_bow = 0x1000, itcf_throw_stone = 0x10000 };

struct Agent {
    char pad0[8];
    int id;                                            // +8
    char padc[4];
    int f10;                                           // +0x10
    char pad14[0x50 - 0x14];
    vector4 pos;                                       // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                         // +0x88 (troop index, -1 = none)
    char pad8c[0x100 - 0x8c];
    int action;                                        // +0x100 (current animation / action id)
    char pad104[0x118 - 0x104];
    float f118;                                        // +0x118
    char pad11c[0x1d0 - 0x11c];
    Pair pairs[11];                                    // +0x1d0
    int pad228;
    int slots[5];                                      // +0x22c (slots[4] at +0x23c)
    char pad240[0x250 - 0x240];
    AgentBody* body;                                   // +0x250
    char pad254[0x55f8 - 0x254];
    int mode1;                                         // +0x55f8 (attack direction chosen for hand 0)
    char pad55fc[0x5608 - 0x55fc];
    int mode2;                                         // +0x5608
    int active;                                        // +0x560c
    char pad5610[0x5928 - 0x5610];

    Pair slot_pair(int idx) const
    {
        const Pair* p = 0;
        int i = slots[idx];
        if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
        Pair r;
        r.a = -1;
        if (p) r = *p;
        return r;
    }
    bool charged_0x48() const { return action == 0x48 && body->skel->chans->charge > 0.1f; }   // 0x432470
    void get_dir(float* out);                          // 0x4378f0 (ret 4)
    int id_matches_dir(int id, int dir);               // 0x4326d0 (ret 8; this unused)
    int can_hit(unsigned aid, unsigned bid, int alt, int flag, const float* target);   // 0x4406a0 (ret 0x14)
    int pick_duel();                                   // 0x43bb80 (fastcall)
    int pick_front();                                  // 0x43b9e0 (fastcall)
    void pick_sound_ids(int* a, int* b, int* c);       // 0x440070 (ret 0xc)
};
struct GameData { int skill(int troop, int id); };     // 0x437b20 (thiscall, ret 8)
extern GameData* g_game;                               // 0x5e2930
struct ItemKind2 { char pad0[0x20c]; int weight; char pad210[0x238 - 0x210]; };   // 0x238 bytes; +0x20c item weight*100?
extern ItemKind2* g_items2;                            // 0x5e29d0 (same table as g_items)
int __cdecl group_base(int id);                        // 0x432130 (s00432130)
int __cdecl not_group_base(int id);                    // 0x4320b0 (s00431960)
static __forceinline Agent* agent_by_id(unsigned id)
{
    return &g_mission->agents[id >> 4][id & 0xf];
}

// @ 0x440070
void Agent::pick_sound_ids(int* a, int* b, int* c)
{
    *a = -1;
    *b = -1;
    *c = -1;
    int mode = 0;
    if (id != g_mission->player_id || g_stick_mode == 0) {
        int t = pick_duel();
        if (t != -1) {
            Agent* o = agent_by_id(t);
            if (o->active == 2)
                mode = o->mode1;
            else if (o->active == 1)
                mode = o->mode1;
        }
    } else if (g_stick_mode == 1) {
        float dx = g_stick_x;
        float dy = g_stick_y;
        if (g_invert != 0)
            dy = -dy;
        if (dx > dy) {
            if (dx > -dy)
                mode = 2;
            else
                mode = 3;
        } else {
            if (dx < -dy)
                mode = 1;
            else
                mode = 0;
        }
    } else {
        mode = 0;
        int t = pick_front();
        if (t >= 0) {
            Agent* o = agent_by_id(t);
            AgentBody* tb = o->body;
            if (tb != 0) {
                posm cam = g_mission->view;
                cam.rotate_fu(-1.5707964f);
                vector4 p = o->pos;
                p.z = p.z + 0.7f;
                if (o->f10 == 0)
                    p = o->body->m.transform_point(*(vector4*)(tb->skel->rec(9) + 0x30));
                p = cam.transform_point_to_local(p);
                float inv = 1.0f / p.y;
                p.x = p.x * inv;
                p.z = inv * p.z;
                float k = 0.4f * p.z;
                if (p.x < k - 0.02f && p.x < -0.02f - k) mode = 2;
                else if (k + 0.02f < p.x && 0.02f - k < p.x) mode = 1;
                else if (p.z < -0.1f) mode = 3;
                else mode = 0;
            }
        }
    }
    unsigned __int64 flags = 0;
    Pair p0 = slot_pair(0);
    if (p0.a != -1)
        flags = g_items[p0.a].flags;
    Pair p1 = slot_pair(1);
    if (p1.a > 0) {
        *a = 0xc7; *b = 0xc8; *c = 0xc9;
        return;
    }
    if (p0.a < 1) {
        *a = 0x97; *b = 0x98; *c = 0x99;
        return;
    }
    unsigned __int64 pole = flags & ITCF(itcf_thrust_polearm);
    if (mode == 0) {
        if (pole) { *a = 0xd6; *b = 0xd7; *c = 0xd8; return; }
        if (flags & ITCF(itcf_shoot_bow)) { *a = 0xca; *b = 0xcb; *c = 0xcc; return; }
        if (flags & ITCF(itcf_throw_stone)) { *a = 0xe2; *b = 0xe3; *c = 0xe4; return; }
    } else if (mode == 2) {
        if (pole) { *a = 0xd0; *b = 0xd1; *c = 0xd2; return; }
        if (flags & ITCF(itcf_shoot_bow)) { *a = 0xd0; *b = 0xd1; *c = 0xd2; return; }
        if (flags & ITCF(itcf_throw_stone)) { *a = 0xe8; *b = 0xe9; *c = 0xea; return; }
    } else if (mode == 1) {
        if (pole) { *a = 0xd3; *b = 0xd4; *c = 0xd5; return; }
        if (flags & ITCF(itcf_shoot_bow)) { *a = 0xd3; *b = 0xd4; *c = 0xd5; return; }
        if (flags & ITCF(itcf_throw_stone)) { *a = 0xeb; *b = 0xec; *c = 0xed; return; }
    } else {
        if (pole) { *a = 0xd9; *b = 0xda; *c = 0xdb; return; }
        if (flags & ITCF(itcf_shoot_bow)) { *a = 0xcd; *b = 0xce; *c = 0xcf; return; }
        if (flags & ITCF(itcf_throw_stone)) { *a = 0xe5; *b = 0xe6; *c = 0xe7; }
    }
}

static __forceinline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
// @ 0x4406a0
int Agent::can_hit(unsigned aid, unsigned bid, int alt, int flag, const float* target)
{
    Agent* a = agent_by_id(aid);
    Agent* b = agent_by_id(bid);
    bool strike = flag != 0;
    int mode;
    if (a->charged_0x48())
        mode = 0;
    else if (alt == 0)
        mode = a->mode1;
    else
        mode = a->mode2;
    int act = b->action;
    int grp = group_base(act);
    int not_base = not_group_base(act);
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    float len = (float)sqrt(dy * dy + dx * dx);
    vec2 u, f;
    if (len > 0.0f) {
        float inv = 1.0f / len;
        u.x = inv * dx;
        u.y = inv * dy;
    } else {
        u.x = 0.0f;
        u.y = 1.0f;
    }
    b->get_dir(&f.x);
    if (strike) {
        if (grp == 199 && f.y * u.y + f.x * u.x < -0.4f) {
            matrix m = b->body->m.mul(*(const matrix*)b->body->skel->rec(12));
            vector4 hp = vadd(m.o, vmul(m.f, 0.2f));
            float pz = hp.z;
            float lo = 0.0f, hi = 0.0f;
            Pair sp = b->slot_pair(1);
            if (sp.a >= 0) {
                lo = (float)g_items2[sp.a].weight * 0.01f;
                if (lo == 0.0f)
                    lo = 0.5f;
                hi = lo;
                if (lo > 0.5f)
                    hi = 0.5f;
            }
            if (b->troop >= 0) {
                int k = g_game->skill(troop, 0x1a);
                float s = (float)k * 0.02f + 1.0f;
                float u2 = (float)k * 0.1f;
                hi = s * hi + u2;
                lo = s * lo + u2;
            }
            float z = target[2] - pz;
            if (-lo < z && z < hi)
                return 1;
        }
        return 0;
    } else {
        if (mode == 2 || mode == 1) {
            if (0.3f < f.y * u.y + f.x * u.x)
                return 0;
        } else if (-0.2f < f.y * u.y + f.x * u.x)
            return 0;
        if (not_base != 0 || !(b->f118 < 0.25f)) {
            if (grp == 0x97) {
                Pair sp = a->slot_pair(0);
                if (sp.a < 0)
                    return 1;
            }
            return id_matches_dir(grp, mode);
        }
    }
    return 0;
}
