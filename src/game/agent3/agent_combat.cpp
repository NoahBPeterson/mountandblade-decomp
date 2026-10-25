//-----------------------------------------------------------------------------
// File: agent_combat.cpp  (0x43e190..0x441360)
//
// Desc: the game font's glyph metrics, the std::vector / std::deque instantiations the linker kept from this
//       object (tstring, int, E12, E48, E56, Rec1c8, E32 elements), the skeleton accessors, the agent's aim
//       point and capsule hit test, the attack-direction / shoot / sound-id pickers, the "can this hit" test,
//       the agent-table pool and the line-of-sight / fire-check scans. Functions in ascending address order
//       (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <stdexcept>
#include "./agent_combat.h"
#include "../tstring.h"
#include "./agent_types.h"      // (named here too: the resolution audit reads the annotations of directly included headers)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../font.h"
#include "../../include/mat4.h"
#include "../../include/timer.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../world.h"
#include "../skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/rec1c8.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// signed_angle reads the x and y words of its two arguments (agent2/agent_types.h declares them vec2*); the agent's vel / fx are
// float records (vector4 or float[]) whose first two words are x and y, so the pointer is the same
static __forceinline const vec2* xy(const void* v) { return (const vec2*)v; }

// inline helpers: the original compiled them into the callers
static __forceinline vector4 vmul(const vector4& v, const float& k) { return vector4(v.x * k, v.y * k, v.z * k); }
static __forceinline vector4 smul(const float& k, const vector4& v) { return vector4(k * v.x, k * v.y, k * v.z); }
static inline vector4 vscale(const vector4& v, float k) { return vector4(v.x * k, v.y * k, v.z * k); }
static inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline vector4 to_world(const matrix& m, const vector4& t)
{
    return vector4(t.x * m.s.x + (t.y * m.f.x + t.z * m.u.x) + m.o.x,
                   t.x * m.s.y + (t.y * m.f.y + t.z * m.u.y) + m.o.y,
                   t.x * m.s.z + (t.y * m.f.z + t.z * m.u.z) + m.o.z);
}
static inline vector4 rot_world(const matrix& m, const vector4& d)
{
    return vector4(m.s.x * d.x + (m.f.x * d.y + m.u.x * d.z),
                   m.s.y * d.x + (m.f.y * d.y + m.u.y * d.z),
                   m.s.z * d.x + (m.f.z * d.y + m.u.z * d.z));
}

// ---- font glyph metrics ---------------------------------------------------------------------------------------
// @ 0x43e190
float Font::advance(int ch)
{
    if (ch >= 0x200) {
        IntHashBase::iterator it = ((IntHashBase&)map).lower_bound(ch);
        GlyphNode* n = (GlyphNode*)it._Mynode();
        if (n != map.head) {
            Glyph* g = &n->val;
            return g->f18 + g->f10;
        }
        return dflt.f18 + dflt.f10;
    }
    Glyph* g = &ascii[ch];
    return g->f18 + g->f10;
}

// @ 0x43e200
float Font::measure(const int* text, int len, int* start, int* end)
{
    float sum = 0.0f;
    bool in_tag = false;
    int i = *start;
    if (i < *end) {
        do {
            if (i >= len)
                break;
            if (in_tag) {
                if (text[i] == 0x3e)
                    in_tag = false;
            } else {
                int c = text[i];
                if (c == 0x3c)
                    in_tag = true;
                else if (c == 9)
                    sum = (float)(int)(sum * 4.0f) * 0.25f + 0.4f;
                else
                    sum += advance(c);
            }
            i++;
        } while (i < *end);
    }
    return sum;
}

// @ 0x43e2a0
void vec_xlen() { throw std::length_error("vector<T> too long"); }

// ---- container instantiations (VC7.1 <vector> / <deque>) -----------------------------------------------------------
// Built WITHOUT an EH switch: the STL try/catch alone makes cl emit the frames (with /EHa cl also keeps the ~E56 call
// out of line). The catch(...) funclets and cold tails (0x43e47a, 0x43e514, 0x43e910, 0x43ebc9, 0x43ec91, 0x43edd9,
// 0x43ef97) live inside the COMDAT of the function they belong to.
// @ 0x43e310 (+ 0x43e47a, 0x43e514)
template void std::vector<tstring>::_Insert_n(std::vector<tstring>::iterator, size_t, const tstring&);
// @ 0x43e590
template void std::deque<E32>::_Growmap(size_t);
// @ 0x43e6e0
template void std::_Destroy_range(E56*, E56*, std::allocator<E56>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x43e740
void Skeleton::ensure_updated()
{
    if (!done) {
        if (has_poses)
            ((FxAnim*)this)->update_poses();
        else
            ((FxAnim*)this)->update_poses2();
    }
}

// @ 0x43e760
template bool std::vector<int>::_Buy(size_t);
// @ 0x43e7b0 (+ 0x43e910)
template void std::vector<int>::_Insert_n(std::vector<int>::iterator, size_t, const int&);
// @ 0x43ea00 (+ 0x43ebc9, 0x43ec91)
template void std::vector<Rec1c8>::_Insert_n(std::vector<Rec1c8>::iterator, size_t, const Rec1c8&);
// @ 0x43ed20 (+ 0x43edd9)
template std::vector<int>::vector(const std::vector<int>&);
// @ 0x43edf0 (+ 0x43ef97)
template void std::vector<E12>::_Insert_n(std::vector<E12>::iterator, size_t, const E12&);
// @ 0x43f0a0
template std::vector<int>& std::vector<int>::operator=(const std::vector<int>&);
// @ 0x43f1f0
template void std::deque<E32>::push_back(const E32&);

// @ 0x43f260
char* Skeleton::rec(int i)
{
    if (!done) {
        if (has_poses)
            ((FxAnim*)this)->update_poses();
        else
            ((FxAnim*)this)->update_poses2();
    }
    return recs + i * 0x1dc + 0x84;
}

// @ 0x43f2a0
template void std::vector<tstring>::push_back(const tstring&);
// @ 0x43f310
template void std::vector<int>::resize(size_t, int);
// @ 0x43f3a0
template std::vector<E48>::iterator std::vector<E48>::insert(std::vector<E48>::iterator, const E48&);
// 0x4108b0 is vector<E48>::_Insert_n (an earlier object)
template void std::vector<E48>::_Insert_n(std::vector<E48>::iterator, size_t, const E48&);
// @ 0x43f410
template std::vector<Rec1c8>::iterator std::vector<Rec1c8>::insert(std::vector<Rec1c8>::iterator, const Rec1c8&);
// @ 0x43f480
template void std::vector<E56>::_Destroy(E56*, E56*);
// @ 0x43f4a0
template std::vector<E12>::iterator std::vector<E12>::insert(std::vector<E12>::iterator, const E12&);

// @ 0x43f510
E56::E56(const E56& o)
    : a(o.a), b(o.b), c(o.c), v1(o.v1), v2(o.v2), d(o.d), e(o.e), f(o.f)
{
}

// ---- aim point and hit tests ------------------------------------------------------------------------------------
// One 0x1dc-byte animator record seen at skeleton->recs + 0x1140 (the head/hand bone used as aim origin).
struct AimRec {
    char pad0[0x10];
    vector4 offs;               // +0x10
    vector4 dir;                // +0x20
    vector4 base;               // +0x30
};
// @ 0x43f560
void Agent::aim_point_and_dir(vector4* pos_out, vector4* dir_out)
{
    Skeleton* sk = body->skel;
    if (!sk->done) {
        if (sk->has_poses)
            ((FxAnim*)sk)->update_poses();
        else
            ((FxAnim*)sk)->update_poses2();
    }
    AimRec* r = (AimRec*)(sk->recs + 0x1140);
    vector4 t = vadd(vscale(r->offs, 0.2f), r->base);
    const matrix& m = body->m;
    *pos_out = to_world(m, t);
    *dir_out = rot_world(body->m, r->dir);
}

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
            c.l0 = smul(scale, d.a);
            c.l1 = vmul(d.b, scale);
            c.radius = d.r * scale;
            Skeleton* sk = body->skel;
            if (!sk->done) {
                if (sk->has_poses)
                    ((FxAnim*)sk)->update_poses();
                else
                    ((FxAnim*)sk)->update_poses2();
            }
            matrix tm = body->m.mul(*(matrix*)(sk->recs + i * 0x1dc + 0x84));
            c.transform((const ::pos*)&tm);
            contact ct;
            if (shape_contact_count(&c, other, 1, &ct)) {
                *out_point = ct.point;
                *out_a = ct.normal;
                *out_bone = i;
                return true;
            }
        }
    }
    return false;
}

// Which of the four attack directions (1 left, 2 right, 3 up?, 0 thrust) the agent picks; derived from the
// mouse movement (mode > 0) or from where the target stands relative to the view (mode 0).
// @ 0x43f970
int Agent::pick_attack_dir(unsigned tid)
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
        float v = signed_angle(xy(&vel), xy(&fx));
        if (1.0f < agent_by_id_raw(f)->fac) {
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
        Agent* t = agent_by_id_raw(tid);
        AgentBody* tb = t->body;
        if (tb == 0)
            return 0;
        posm cam = g_mission->view;
        { const float quarter = -1.5707964f; cam.matrix::rotate_about_s(&quarter); }
        vector4 p = t->pos;
        p.z = p.z + 0.7f;
        if (t->f10 == 0)
            p = ((const ::pos&)t->body->m).transform_point(*(vector4*)(tb->skel->rec(9) + 0x30));
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
        float v = signed_angle(xy(&vel), xy(&fx));
        if (1.0f < agent_by_id_raw(f)->fac) {
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

// itcf_shoot_* (Module System header_items): the ranged-attack capability bits of an item.
enum {
    itcf_thrust_polearm = 0x100,
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
    ItemRef sp = slot_pair(slot);
    unsigned __int64& flags = *(unsigned __int64*)&sp;
    if (sp.item == -1)
        flags = 0;
    else
        flags = g_items[sp.item].flags;
    int dir, alt;
    if (f2c >= 0) {
        float v = signed_angle(xy(&vel), xy(&fx));
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
    int tgt = pick_front();
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

// @ 0x440070
// itcf_* capability bits are stored in the high dword of the item flags
#define ITCF(x) (((unsigned __int64)(x)) << 32)
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
                { const float quarter = -1.5707964f; cam.matrix::rotate_about_s(&quarter); }
                vector4 p = o->pos;
                p.z = p.z + 0.7f;
                if (o->f10 == 0)
                    p = ((const ::pos&)o->body->m).transform_point(*(vector4*)(tb->skel->rec(9) + 0x30));
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
    ItemRef p0 = slot_pair_inl(0);
    if (p0.item != -1)
        flags = g_items[p0.item].flags;
    ItemRef p1 = slot_pair_inl(1);
    if (p1.item > 0) {
        *a = 0xc7; *b = 0xc8; *c = 0xc9;
        return;
    }
    if (p0.item < 1) {
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

// @ 0x4406a0
int Agent::can_hit(unsigned aid, unsigned bid, int alt, int flag, const float* target)
{
    Agent* a = agent_by_id(aid);
    Agent* b = agent_by_id(bid);
    bool strike = flag != 0;
    int mode;
    if (a->f100_is_0x48_and_charged())
        mode = 0;
    else if (alt == 0)
        mode = a->mode1;
    else
        mode = a->mode2;
    int act = b->anim[1].cur;
    int grp = group_base(act);
    int not_base = not_group_base(act);
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    float len = (float)sqrt(dy * dy + dx * dx);
    struct { float x, y; } u, f;
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
            ItemRef sp = b->slot_pair_inl(1);
            if (sp.item >= 0) {
                lo = (float)g_items[sp.item].weight * 0.01f;
                if (lo == 0.0f)
                    lo = 0.5f;
                hi = lo;
                if (lo > 0.5f)
                    hi = 0.5f;
            }
            if (b->troop >= 0) {
                int k = g_game->skill_level(troop, 0x1a);
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
        if (not_base != 0 || !(b->anim[1].scale < 0.25f)) {
            if (grp == 0x97) {
                ItemRef sp = a->slot_pair_inl(0);
                if (sp.item < 0)
                    return 1;
            }
            return ::id_matches_dir(grp, mode);
        }
    }
    return 0;
}

// more container instantiations
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

// ---- id pool ---------------------------------------------------------------------------------------------------
// @ 0x440c90
int Pool::alloc()
{
    if (!(free_ids.size() > 0)) {
        RecJ* blk = new RecJ[16];
        pages.push_back(blk);
        for (int j = 0; j < 16; j++) {
            blk[j].f4 = 0;
            blk[j].f8 = next_id + j;
            free_ids.push_back(next_id + j);
        }
        next_id += 16;
    }
    unsigned id = free_ids.back();
    free_ids.pop_back();
    RecJ* r = &pages[id >> 4][id & 0xf];
    r->f4 = 1;
    r->fc = serial;
    serial++;
    return id;
}

// @ 0x440dc0
template void std::fill(E56*, E56*, const E56&);

// ---- agent line of sight / fire check ----------------------------------------------------------------------------
// @ 0x440e30
bool Agent::line_of_sight(unsigned tid, shape_base* ignore)
{
    Agent* o = agent_by_id(tid);
    bool r = false;
    vector4 d;
    d.x = o->pos.x - pos.x;
    d.y = o->pos.y - pos.y;
    d.z = o->pos.z - pos.z;
    if (d.normalize() <= 300.0f && body != 0 && o->body != 0) {
        vector4 p, dir;
        aim_point_and_dir(&p, &dir);
        vector4 head = ((const ::pos&)o->body->m).transform_point(*(const vector4*)(o->body->skel->recs + 0x1170));
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
            r = ((level*)g_mission->scene_world)->trace_ray(&p.x, &head.x, 400.0f, &g_hit_shape, 1) != 0;
    }
    return r;
}

// @ 0x441020
int Agent::fire_check(unsigned tid, int* out_blocker)
{
    g_hit_shape = 0;
    *out_blocker = -1;
    if (f2c != -1) {
        float v = signed_angle(xy(&vel), (const vec2*)blk294);
        if (!(g_range_tbl[variant].lo <= v) || !(v < g_range_tbl[variant].hi))
            return 3;
    }
    if (!line_of_sight(tid, 0))
        return 0;
    const vector4& tp = agent_by_id(tid)->pos;
    float score = 0.0f;
    vector4 a;
    a.x = tp.x - pos.x;
    a.y = tp.y - pos.y;
    float dist;
    {
        a.z = tp.z - pos.z;
        float dz = a.z;
        dist = a.normalize();
        float reach = attack_speed();
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
