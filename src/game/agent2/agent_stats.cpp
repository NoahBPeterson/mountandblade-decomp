//-----------------------------------------------------------------------------
// File: agent_stats.cpp  (0x437830..0x43b100)
//
// Desc: the 0x1c8-byte placed-object record (ctor, deleting dtor, copy ctor, assignment), the agent's combat-stat
//       and order helpers, the skill lookup, the std::vector / std::_Uninit_* instantiations the linker kept,
//       the mesh node bounds, the level queries over the collision grid and the navigation mesh, the animation
//       controller's link setup, the translation table lookup, the agent's hand position / quiver / channel
//       start, the physics set's two members and the light accumulator. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <vector>
#include <algorithm>
#include <stdexcept>
#define ANIM_SLOT_SET_OUT_OF_LINE   // AnimSlot::set 0x4378a0 stays a real function in this TU
#include "./agent_types.h"      // (named here too: the resolution audit reads the annotations of directly included headers)
#include "./agent_stats.h"
#include "../anim_slot.h"      // (named here too for the resolution audit)
#include "../channel.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/mat4.h"
#include "../../include/ptr_tag.h"
#include "../../include/ptr_vec.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/phys_set.h"
#include "../../include/height_field.h"
#include "../cell.h"
#include "../light.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../nav_mesh.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../skill_cap.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../int_vec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../str_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

// inline vector helpers of the original: rgl's operators are out-of-line in other objects
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 vneg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }
static __forceinline float vdot(const vector4& a, const vector4& b) { return a.x * b.x + (a.y * b.y + a.z * b.z); }
static __forceinline vector4 vmul(const vector4& a, float s) { return vector4(a.x * s, a.y * s, a.z * s); }


// ---- the 0x1c8-byte record -----------------------------------------------------------------------------------------
// @ 0x437830
PlacedObj::PlacedObj()
{
    reset();
}

// copy constructor and assignment (explicit memberwise copies; the compiler-generated ones are identical)
// @ 0x437bb0
PlacedObj::PlacedObj(const PlacedObj& o)
    : IfaceB(o), m10(o.m10), m14(o.m14), m18(o.m18), m58(o.m58), m98(o.m98), md8(o.md8), m118(o.m118),
      v158(o.v158), m168(o.m168), f16c(o.f16c), q170(o.q170),
      m180(o.m180), m184(o.m184), kind(o.kind), id(o.id), name(o.name), value(o.value), m1a4(o.m1a4),
      m1a8(o.m1a8), m1ac(o.m1ac), m1b0(o.m1b0), m1b4(o.m1b4), m1b8(o.m1b8), m1bc(o.m1bc), m1c0(o.m1c0),
      m1c4(o.m1c4)
{
}
// @ 0x437d60
PlacedObj& PlacedObj::operator=(const PlacedObj& o)
{
    IfaceB::operator=(o);
    m10 = o.m10; m14 = o.m14;
    m18 = o.m18; m58 = o.m58; m98 = o.m98; md8 = o.md8; m118 = o.m118;
    v158 = o.v158; m168 = o.m168; f16c = o.f16c;
    q170 = o.q170;
    m180 = o.m180; m184 = o.m184; kind = o.kind; id = o.id;
    name = o.name;
    value = o.value; m1a4 = o.m1a4; m1a8 = o.m1a8; m1ac = o.m1ac; m1b0 = o.m1b0; m1b4 = o.m1b4;
    m1b8 = o.m1b8; m1bc = o.m1bc; m1c0 = o.m1c0; m1c4 = o.m1c4;
    return *this;
}

// ---- animation / order slot and the agent's order helpers -------------------------------------------------------------
// accept a new animation when its priority is not lower than the current one
// @ 0x4378a0
inline bool AnimSlot::set(int id, unsigned prio)
{
    if ((prio & 0x1ffff) < (events & 0x1ffff) && (cur != -1 || next != -1))
        return false;
    next = id;
    events = prio;
    if (id == -1)
        events = 0;
    return true;
}

// @ 0x4378f0
void Agent::get_dir(vec2* out)
{
    vec2 v = *(vec2*)&vel;
    float l = (float)sqrt(v.x * v.x + v.y * v.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        v.x = inv * v.x;
        v.y = inv * v.y;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
    }
    if (f10 == 0 && f2c != -1 && (active != 0 || f5610 != 0)) {
        float t = cur;
        float c = (float)cos((double)t);
        float s = (float)sin((double)t);
        float nx = c * v.x - s * v.y;
        float tt = c * v.y;
        v.y = tt + s * v.x;
        v.x = nx;
    }
    *out = v;
}

// @ 0x4379c0
void Agent::set_order(int slot, int id, unsigned prio, float rate)
{
    if (id >= -1 && id < defs->count) {
        if (anim[slot].set(id, prio)) {
            anim[slot].rate = rate < 0.1f ? 0.1f : (rate >= 10.0f ? 10.0f : rate);
            if (prio & 0x4000000)
                body->skel->set_rec_phase(slot, 0.0f);
            if (slot == 0) {
                if (id >= 0 && (defs->defs[id].flags & 0x400) && anim[1].cur != -1 &&
                    (prio & 0x1ffff) > (anim[1].events & 0x1ffff))
                    anim[1].set(-1, prio & 0x1ffff);
            } else if (slot == 1) {
                int k = slots[0];
                if (k >= 0 && pairs[k].item >= 0) {
                    if (prio & 0x10000000)
                        links[k]->vis = 0;
                    else
                        links[k]->vis = 0xffff;
                }
            }
        }
    }
}

// skill level of a troop: the packed 4-bit level plus the event bonus, clamped to the skill's cap
// @ 0x437b20
int GameData::skill_level(int agent, int skill)
{
    unsigned w = chars[agent].nib[skill / 8];
    int lvl = (w >> (skill % 8 * 4)) & 0xf;
    g_ev_a = 0;
    g_ev_b = 0;
    emit_event(0x17, agent, skill);
    lvl += g_ev_a;
    if (lvl < 0)
        return 0;
    int cap = g_skill_tbl[skill].cap;
    if (lvl > cap)
        return cap;
    return lvl;
}

// ---- combat stats ----------------------------------------------------------------------------------------------------
// @ 0x432790
ItemRef Agent::slot_pair(int idx) const
{
    const ItemRef* p = 0;
    int i = slots[idx];
    if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
    ItemRef r;
    r.item = -1;
    if (p) r = *p;
    return r;
}

// @ 0x437f00
bool Agent::has_weapon_for(int mode)
{
    bool found = false;
    int t1 = team;
    if (t1 < 0 || t1 >= 8) t1 = 7;
    int tr = troop;
    int k1 = 0;
    if (tr >= 0) {
        if (f2c >= 0)
            k1 = 2;
        else
            k1 = (g_game->chars[tr].flags30 & 0x4000000) ? 1 : 0;
    }
    int ra = g_mission->teams[t1].cls[k1].a;
    int t2 = team;
    if (t2 < 0 || t2 >= 8) t2 = 7;
    int k2 = 0;
    if (tr >= 0) {
        if (f2c >= 0)
            k2 = 2;
        else
            k2 = (g_game->chars[tr].flags30 & 0x4000000) ? 1 : 0;
    }
    int rb = g_mission->teams[t2].cls[k2].b;
    for (int i = 0; i < 4; i++) {
        if (found) return found;
        ItemRef* p = &pairs[i];
        if (p->item >= 0) {
            ItemKind* ik = &g_items[p->item];
            unsigned fl = ik->type_flags;
            unsigned type = fl & 0xff;
            bool ok = ((ik->f218 & 0x300) == 0x200 || p->stat_21x(0) <= 1) &&
                      ((ik->f214 & 0x300) == 0x200 || p->stat_21x(1) <= 1);
            if ((ra == 0 || ok) &&
                (rb == 0 || (type != 8 && type != 9 && type != 10 && type != 0x10 && type != 0x11))) {
                if (mode == 1) {
                    if ((type == 2 || type == 3 || type == 4) && (fl & 0x400000))
                        found = true;
                } else if (mode == 2) {
                    if (type == 8 || type == 9 || type == 10 || type == 0x10 || type == 0x11) {
                        int missing, total, slot_out;
                        best_ammo(i, &missing, &total, &slot_out);
                        if (missing > 0) {
                            if (type != 9 || !(ik->type_flags & 0x100000) || f2c == -1)
                                found = true;
                        }
                    }
                } else if (mode == 3) {
                    if (type == 4)
                        found = true;
                }
            }
        }
    }
    return found;
}

// @ 0x438180
float Agent::stamina_scale()
{
    if (f10 != 1) {
        float r = 1.0f - f24c * 0.11f / (g_game->chars[troop].lvl58 + 9);
        if (0.0f > r) return 0.0f;
        return r;
    }
    return 1.0f;
}

// signed angle between two 2-D vectors (the first normalised; (0,1) for a zero vector)
// @ 0x4381e0
float __cdecl signed_angle(const vec2* a, const vec2* b)
{
    vec2 u = *a;
    float l = (float)sqrt(u.x * u.x + u.y * u.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        u.x = inv * u.x;
        u.y = inv * u.y;
    } else {
        u.x = 0.0f;
        u.y = 1.0f;
    }
    vec2 bb = *b;
    vec2 r;
    r.x = bb.x * u.y - bb.y * u.x;
    r.y = bb.y * u.y + bb.x * u.x;
    float l2 = (float)sqrt(r.y * r.y + r.x * r.x);
    if (l2 > 0.0f) {
        float inv = 1.0f / l2;
        r.x = inv * r.x;
        r.y = inv * r.y;
    } else {
        r.x = 0.0f;
        r.y = 1.0f;
    }
    return (float)atan2(-r.x, r.y);
}

// @ 0x4382e0
void Agent::set_default_order()
{
    set_order(1, f561c, 0x500000f, 1.0f);
}

// @ 0x438360
float Agent::attack_speed()
{
    float f = 1.0f;
    float base = 10.0f;
    ItemRef cur = cur_pair();
    if (cur.item >= 0) {
        ItemKind* ik = &g_items[cur.item];
        base = (float)ik->f1f4;
        unsigned type = ik->type_flags & 0xff;
        if (type == 8) {
            int lvl = g_game->skill_level(troop, 0x21);
            int v = cur.stat_1ec() + 4;
            if (v >= lvl) v = lvl;
            f = (float)v * 0.12f + 1.0f;
            if (f2c != -1)
                f = ((float)g_game->skill_level(troop, 0x17) * 0.019 + 0.8) * f;
            if (g_mission->fog_dense == 1)
                return (float)sqrt(f * 0.9f) * base * 1.2f;
        } else if (type == 10) {
            f = (float)g_game->skill_level(troop, 0x22) * 0.1f + 1.0f;
            if (f2c != -1)
                return (float)sqrt(((float)g_game->skill_level(troop, 0x17) * 0.019 + 0.8) * f) * base * 1.2f;
        } else if (type == 9) {
            if (g_mission->fog_dense == 1) f = 0.75f;
        }
    }
    return (float)sqrt(f) * base * 1.2f;
}

// ItemRef::reduce (0x437510) with n == 0, as cl inlines it here
static __forceinline void reduce_inl(ItemRef* r)
{
    if (r->item > 0) {
        unsigned t = g_items[r->item].type_flags & 0xff;
        if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts || t == itp_type_pistol ||
            t == itp_type_musket || t == itp_type_crossbow || t == itp_type_bullets) {
            ((unsigned char*)&r->m4)[0] = 0;
        } else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal) {
            unsigned v = r->stat_1e8();
            *(short*)&r->m4 = 0;
            r->m4 |= v;
        }
    }
}

// @ 0x438520
void Agent::clear_ammo(int unused)
{
    for (int i = 0; i < 4; i++) {
        ItemRef* p = &pairs[i];
        if (pairs[i].item >= 0) {
            unsigned t = g_items[pairs[i].item].type_flags & 0xff;
            if (t == itp_type_arrows || t == itp_type_bolts || t == itp_type_bullets || t == itp_type_thrown) {
                if (p->item > 0)
                    reduce_inl(p);
            }
        }
    }
}

// The slot-class lookup 0x4350c0 (src/game/agent) uses cl's register convention for same-TU statics (item in EAX,
// flag on the stack): reproduced by defining it here as a static (copy of the body).
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

// @ 0x4385e0
float Agent::damage_factor(int idx)
{
    float r = 1.0f;
    ItemRef sel = slot_pair(idx);
    int item = sel.item;
    if (item >= 0) {
        r = (float)sel.stat_1f0a() * 0.01f;
        unsigned flags = g_items[item].type_flags;
        unsigned type = flags & 0xff;
        ItemKind* ik = &g_items[item];

        if (type == 2 || type == 3 || type == 4 || type == 8 || type == 9 || type == 10 || type == 0x10 ||
            type == 0x11) {
            ItemRef o = slot_pair_inl(1);
            int cls = slot_class_impl(item, o.item >= 0);
            int v = (int)g_game->chars[troop].f80[cls + 0];
            float frac = 0.07f;
            if ((char)ik->type_flags == 8) frac = 0.11f;
            float vv = (float)v * 0.01f;
            r = ((1.0f - frac) + vv * frac) * r;
        }
        if (type == 3 || type == 4) {
            ItemRef o = slot_pair_inl(1);
            if (o.item >= 0 || f2c >= 0) {
                r = r - 0.15f;
                if (flags & 0x200000) r = r - 0.05f;
            }
        } else if (type == 7) {
            int lvl = g_game->skill_level(troop, 0x1a);
            r = ((float)lvl * 0.25f + 1.0f) * r;
            if (id == g_mission->f5c) {
                if (g_agent_count > 1)
                    r = (((float)g_agent_count * 0.7f + 9.3f) * r) * 0.07f;
            } else {
                r = ((float)g_agent_count * 0.07f + 0.93f) * r;
            }
        }
        unsigned t2 = ik->type_flags & 0xff;
        if ((t2 == 2 || t2 == 3 || t2 == 4) && (id != g_mission->f5c || g_agent_count > 1))
            r = ((float)g_agent_count * 0.07f + 0.93f) * r;
    }
    return r;
}

// ---- the eased turn value ------------------------------------------------------------------------------------------------
// @ 0x438860
void Agent::smooth_turn(float dt)
{
    float v = signed_angle((const vec2*)&vel, (const vec2*)&fx);
    int o = variant * 8;
    float c;
    if (*(float*)(g_range_tbl + o) <= v && v < *(float*)(g_range_tbl + o + 4))
        c = v;
    else
        c = clamp_ref(&v, (float*)(g_range_tbl + o), (float*)(g_range_tbl + o + 4));
    float d = c - cur;
    if (!(fabs(d) < 0.8f))
        c = d * dt * 9.0f + cur;
    cur = c;
}

// ---- VC7.1 <algorithm> / <memory> instantiations ----------------------------------------------------------------------------
// std::remove_copy over vector<T*>::iterator (0x438910, ICF-folded: one body for every pointer element type): not written here,
// cl instantiates it from the std::remove call of Skeleton::remove_shape (the symbol in the manifest is that instance)

// std::_Uninit_fill_n for 12-byte elements (catch funclet 0x4389c4: `throw;`)
// @ 0x438960
void __cdecl uninit_fill_n12(E12* f, unsigned n, const E12& v, allocE12& al, ptr_tag tag)
{
    E12* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) ;
        throw;
    }
}

// std::copy_backward for the 0x1c8-byte elements
// @ 0x4389d0
E1c8* __cdecl copy_backward1c8(E1c8* f, E1c8* l, E1c8* d)
{
    while (f != l) {
        --l; --d;
        *d = *l;
    }
    return d;
}

// std::_Uninit_copy for tstring (catch funclet 0x438a7a: destroys [s, d) then `throw;`)
// @ 0x438a10
tstring* __cdecl uninit_copy_str(tstring* f, tstring* l, tstring* d, allocstr& al, ptr_tag tag)
{
    tstring* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) s->~tstring();
        throw;
    }
    return d;
}

// std::_Uninit_copy for 4-byte elements (catch funclet 0x438b06: `throw;`)
// @ 0x438aa0
int* __cdecl uninit_copy_int(int* f, int* l, int* d, allocint& al, ptr_tag tag)
{
    int* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) ;
        throw;
    }
    return d;
}

// std::_Adjust_heap (min-heap on key) for 12-byte nodes
// @ 0x438b10
void __cdecl adjust_heap_node(hnode* first, int hole, int bottom, hnode val, node_gt pred)
{
    int top = hole;
    int child = 2 * hole + 2;
    for (; child < bottom; child = 2 * child + 2) {
        if (pred(first[child], first[child - 1]))
            --child;
        first[hole] = first[child];
        hole = child;
    }
    if (child == bottom) {
        first[hole] = first[bottom - 1];
        hole = bottom - 1;
    }
    push_heap_node(first, hole, top, val, pred);
}

// ---- small members -----------------------------------------------------------------------------------------------------------
// ItemRef::set_modifier: stores a 0..0x2a modifier in the top byte of the packed word, then re-clamps the amount
// @ 0x438bc0
void ItemRef::set_modifier(int k)
{
    if (k >= 0 && k < 0x2b) {
        ((char*)this)[7] = 0;
        m4 |= k << 24;
        reduce(0);
    }
}

// @ 0x438bf0
void ptr_vec::pop_back_if()
{
    if (!empty())
        --last;
}

// @ 0x438c10
E1c8* vec1c8::at(int i) { return first + i; }

// @ 0x438c30
int* IntVec::fill_ptr(int* f, unsigned n, const int& x)
{
    unsigned c = n;
    int* p = f;
    for (; 0 < c; --c, ++p) *p = x;
    return f + n;
}

// @ 0x438c60
void __cdecl fill1c8(E1c8* f, E1c8* l, const E1c8& v)
{
    for (; f != l; ++f) *f = v;
}

// vector<E12>::_Ucopy -> _Uninit_copy(first, last, dest, allocator, _Ptr_cat(first, dest))
inline ptr_tag ptr_cat(const E12*, const E12*) { ptr_tag t; return t; }
// @ 0x438c90
E12* vec12::ucopy(E12* a, E12* b, E12* c) { return uninit_copy12(a, b, c, this, ptr_cat(a, c)); }

// ---- ranged hit chance ---------------------------------------------------------------------------------------------------------
// @ 0x438cb0
float __cdecl hit_chance(int* who, Agent* a)
{
    float v = a->attack_speed() * 2.0f;
    if (a->f2c >= 0)
        v *= 0.9f;
    int idx = *who;
    int s1;
    if (idx >= 0) {
        unsigned k = g_items[idx].type_flags & 0xff;
        if (k == 10)
            v *= 0.65f;                          // thrown
        else if (k == 8)
            v *= 0.9f;                           // bow
        else if (k == 0x11 || k == 0x10)
            v *= 0.55f;                          // musket / pistol
        s1 = g_items[idx].f204;
        if (s1 == 0) s1 = 99;
    } else
        s1 = 100;
    int s2 = idx < 0 ? 100 : (g_items[idx].f204 == 0 ? 99 : g_items[idx].f204);
    float p = (float)(s2 * s1) * 0.0001f;
    if (!(p < 1.0f)) p = 1.0f;
    return p * v;
}

// ---- mesh instance helpers --------------------------------------------------------------------------------------------------------
// min/max as written by the engine: first argument wins ties (the order decides the x87 code)
template<class T> inline T minv(T a, T b) { return (a < b) ? a : b; }
template<class T> inline T maxv(T a, T b) { return (a > b) ? a : b; }

// @ 0x438d80
void mnode::calc_bounds(vector4* mn, vector4* mx)
{
    *mn = vector4(1e7f, 1e7f, 1e7f);
    *mx = vector4(-1e7f, -1e7f, -1e7f);
    vec_e32* v = is_owner ? &verts : master->get_verts();
    for (unsigned i = 0; i < v->size(); ++i) {
        const V32& e = (*v)[i];
        mn->x = minv(mn->x, e.x);
        mn->y = minv(mn->y, e.y);
        mn->z = minv(mn->z, e.z);
        const V32& f = (*v)[i];
        mx->x = maxv(mx->x, f.x);
        mx->y = maxv(mx->y, f.y);
        mx->z = maxv(mx->z, f.z);
    }
}

// std::_Uninit_fill_n for tstring (catch funclet 0x438f68: destroys [s, f) then `throw;`)
// @ 0x438f00
void __cdecl uninit_fill_n_str(tstring* f, unsigned n, const tstring& v, allocstr& al, ptr_tag tag)
{
    tstring* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) s->~tstring();
        throw;
    }
}

// @ 0x438f90
void node_list::cap_all(const float* limit)
{
    for (mnode** it = first; it < last; ++it) {
        mnode* n = *it;
        struct L { int a0; int* data; int count; };
        L* l = n->is_owner ? (L*)&n->ints : (L*)n->master->get_ints();
        if (0 < l->count) {
            if (*limit > (float)l->data[l->count - 1])
                n->f_c8 = (float)l->data[l->count - 1];
            else
                n->f_c8 = *limit;
        }
    }
}

// @ 0x439000
void node_groups::cap_all(const float* limit)
{
    for (int i = 0; i < nlists; ++i)
        lists[i].cap_all(limit);
}

// @ 0x439040
void allocE1c8_def::construct(E1c8* p, const E1c8& v) { new ((void*)p) E1c8(v); }

// std::_Uninit_copy for the 0x1c8-byte elements (catch funclet 0x4390d0: destroys [s, d) then `throw;`)
// @ 0x439060
E1c8* __cdecl uninit_copy1c8(E1c8* f, E1c8* l, E1c8* d, allocE1c8& al, ptr_tag tag)
{
    E1c8* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) s->~E1c8();
        throw;
    }
    return d;
}

// ---- level and navigation-mesh queries -----------------------------------------------------------------------------------------------
// clone a body's shape (dispatch on shape_type)
// @ 0x439100
shape_base* body_res::copy_shape()
{
    shape_base* r;                                      // (uninitialised when the type is unknown)
    if (shape->shape_type() == 0) {
        r = new sphere(*(sphere*)shape);
    } else if (shape->shape_type() == 1) {
        r = new capsule(*(capsule*)shape);
    } else if (shape->shape_type() == 2) {
        r = new face(*(face*)shape);
    } else if (shape->shape_type() == 3) {
        r = new composite(*(composite*)shape);
    } else if (shape->shape_type() == 4) {
        r = (shape_base*)((manifold*)shape)->build_composite();
    }
    return r;
}

// @ 0x4391d0
bool NavMesh::inside(const int* face, const float* p)
{
    NavFace* f = &faces[*face];
    int n = f->nverts;
    bool in = true;
    int i = 0;
    const int* vi = f->vert_p();
    do {
        if (i >= n) return in;
        const NavPoint18* a = &verts[*vi];
        i++;
        const NavPoint18* b = &verts[f->vert_p()[i % n]];
        float e = b->x - a->x, f_ = b->y - a->y, g = p[0] - a->x, h = p[1] - a->y;
        if (h * e - g * f_ < 0.0f)
            in = false;
        vi++;
    } while (in);
    return in;
}

// @ 0x439280
float NavMesh::dist(const int* face, const float* p)
{
    float out = 0.0f;
    NavFace* f = &faces[*face];
    int n = f->nverts;
    int i = 0;
    bool in = true;
    if (0 < n) {
        const int* vi = f->vert_p();
        do {
            const NavPoint18* a = &verts[*vi];
            i++;
            const NavPoint18* b = &verts[f->vert_p()[i % n]];
            float dx = b->x - a->x;
            float dy = b->y - a->y;
            float len = (float)sqrt(dx * dx + dy * dy);
            float ux, uy;
            if (len > 0.0f) {
                float inv = 1.0f / len;
                ux = dx * inv;
                uy = inv * dy;
            } else {
                ux = 0.0f;
                uy = 1.0f;
            }
            float nx = uy, ny = -ux;
            float d = (p[0] - a->x) * nx + (p[1] - a->y) * ny;
            if (!(d < 0.0f)) {
                in = false;
                if (out < d) out = d;
            }
            vi++;
        } while (i < n);
        if (!in) goto have_out;
    }
    out = 0.0f;
have_out:
    const NavPoint18* a0 = &verts[f->vert_p()[0]];
    float dz = p[2] - (a0->z - ((p[0] - a0->x) * f->nx + (p[1] - a0->y) * f->ny) / f->nz);
    if (-1.0f < dz && dz < 0.1f)
        dz = 0.0f;
    return (float)fabs(dz) + out;
}

// visit stamp: +3 per search, with a rollover reset of every face stamp past 1e9
// @ 0x439410
void NavMesh::next_stamp()
{
    if (stamp > 1000000000) {
        for (unsigned i = 0; i < faces.size(); ++i)
            faces[i].stamp = 0;
        stamp = 0;
    }
    stamp += 3;
}

// walk across neighbouring faces along the segment from `from` to `to`
// @ 0x439460
bool NavMesh::trace(int face, vector4 from, int end_face, const float* to, int* max_steps)
{
    float x = from.x, y = from.y, z = from.z;
    struct { int use_x; int found; int prev; int cur; int steps; } st;
    bool done;
    st.cur = -1;
    if (face >= 0 && end_face >= 0) {
        float dx = to[0] - x;
        float dy = to[1] - y;
        float dz = to[2] - z;
        if (dx * dx + (dy * dy + dz * dz) < 0.0001f)
            return true;
        st.use_x = 1;
        if (-1e-06f < dx && dx < 1e-06f)
            st.use_x = 0;
        st.steps = 0;
        st.prev = -1;
        done = false;
        st.cur = face;
        while (st.cur != end_face && st.cur >= 0 && !done && st.steps < *max_steps) {
            ++st.steps;
            NavFace* f = &faces[st.cur];
            st.found = -1;
            for (int i = 0; i < faces[st.cur].nverts; ++i) {
                if (f->edge_p()[i] != st.prev) {
                    const NavPoint18* a = &verts[f->vert_p()[i]];
                    const NavPoint18* b = &verts[f->vert_p()[(i + 1) % f->nverts]];
                    float ex = b->x - a->x;
                    float ey = b->y - a->y;
                    float t = ((x - a->x) * dy - (y - a->y) * dx) / (ex * dy - ey * dx);
                    float s;
                    if (st.use_x)
                        s = ((ex * t + a->x) - x) / dx;
                    else
                        s = ((ey * t + a->y) - y) / dy;
                    if (0.1f <= t && t <= 0.9f && 0.0f <= s && s <= 1.0f)
                        st.found = i;
                }
                if (st.found != -1) break;
            }
            if (st.found >= 0) {
                st.prev = f->edge_p()[st.found];
                int nb = f->nbr_p()[st.found];
                done = false;
                if (nb < 0) {
                    done = true;
                } else
                    st.cur = nb;
            } else
                done = true;
        }
    }
    return st.cur == end_face;
}

// merge this cell's shapes (those whose group mask does not intersect `mask`) into the sorted array out[0..*count)
// @ 0x4396b0
void cell::collect_masked(unsigned mask, shape_base** out, int* count)
{
    shape_base* buf[0x2000];
    int n = 0;
    int k = 0;
    unsigned i = 0;
    while (i < shapes.size()) {
        shape_base* sh = shapes[i];
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        if (k < *count) {
            shape_base* o = out[k];
            if (sh >= o) {
                if (sh == o) {
                    i++;
                    buf[n++] = sh;
                } else {
                    buf[n++] = o;
                }
                k++;
                continue;
            }
        }
        i++;
        buf[n++] = sh;
    }
    if (k < *count) {
        int m = *count - k;
        memcpy(&buf[n], &out[k], m * sizeof(shape_base*));
        n += m;
    }
    *count = n;
    for (int j = 0; j < *count; j++)
        out[j] = buf[j];
}

// shapes of this cell containing the point p (xy box, or the 3-D box via in_box when *use3d != 0)
// @ 0x439780
void cell::collect_point(const float* p, int* use3d, unsigned mask, shape_base** out, int* count)
{
    unsigned i = 0;
    *count = 0;
    while (i < shapes.size()) {
        shape_base* sh = shapes[i];
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        vector4 tv[2];
        const vector4* smax = sh->get_max(&tv[0]);
        const vector4* smin = sh->get_min(&tv[1]);
        bool hit;
        if (*use3d == 0)
            hit = smin->x <= p[0] && p[0] <= smax->x && smin->y <= p[1] && p[1] <= smax->y;
        else
            hit = in_box(smin, smax, (const vector4*)p);
        if (hit) {
            out[*count] = shapes[i];
            i++;
            (*count)++;
        } else
            i++;
    }
}

// @ 0x439870
void level::query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count)
{
    int pairs[0x2000][2];
    int n;
    walk(a, b, pairs, &n);
    *count = 0;
    for (int i = 0; i < n; i++)
        cells[pairs[i][0]][pairs[i][1]].collect_masked(mask, out, count);
}

// recursive walk over the neighbours of nav face *face, keeping the nearest face whose polygon contains p
// @ 0x439900
void level::nav_search(const float* p, const int* face, const int* max_depth, int depth, int* best, float* best_d)
{
    if (nav != 0) {
        for (int i = 0; i < nav->faces[*face].nverts; i++) {
            int nb = nav->faces[*face].nbr_p()[i];
            if (nb >= 0 && nav->faces[nb].stamp < nav->stamp) {
                nav->faces[nb].stamp = nav->stamp;
                if (nav->inside(&nb, p)) {
                    const NavPoint18* v = &nav->verts[nav->faces[nb].vert_p()[0]];
                    NavFace* f = &nav->faces[nb];
                    float dz = (float)fabs(p[2] - (v->z - ((p[0] - v->x) * f->nx + (p[1] - v->y) * f->ny) / f->nz));
                    if (dz < 2.0f && dz < *best_d) {
                        *best = nb;
                        *best_d = dz;
                    }
                }
                if (depth < *max_depth)
                    nav_search(p, &nb, max_depth, depth + 1, best, best_d);
            }
        }
    }
}

// nearest nav face to p: a face containing p (best |dz| < 2 below) wins over near faces (score 10 - dist - |dz|)
// @ 0x439a50
int level::nearest_face(const float* p)
{
    NavMesh* m = nav;
    int best = -1;
    float near_score = 0.0f;
    bool have = false;
    float best_dz = 10000.0f;
    if (m) {
        unsigned i = 0;
        while (i < m->faces.size()) {
            int idx = i;
            float d = m->dist(&idx, p);
            if (d <= 0.0f) {
                NavFace* f = &m->faces[i];
                const NavPoint18* v = &m->verts[f->vert_p()[0]];
                float dz = p[2] - (v->z - ((p[0] - v->x) * f->nx + (p[1] - v->y) * f->ny) / f->nz);
                if (dz > -2.0f) {
                    dz = (float)fabs(dz);
                    if (!have || dz < best_dz) {
                        best = i;
                        best_dz = dz;
                        have = true;
                    }
                }
            } else if (d < 10.0f) {
                NavFace* f = &m->faces[i];
                const NavPoint18* v = &m->verts[f->vert_p()[0]];
                float dz = p[2] - (v->z - ((p[0] - v->x) * f->nx + (p[1] - v->y) * f->ny) / f->nz);
                if (dz > -2.0f) {
                    float s = (10.0f - d) - (float)fabs(dz);
                    if (s > near_score) {
                        best = i;
                        near_score = s;
                    }
                }
            }
            i++;
        }
    }
    return best;
}

// highest floor polygon under (or at most 1 above) p; falls back to the lowest polygon above it
// @ 0x439be0
void level::floor_at(const float* p, float* out_z, face** out_face)
{
    int n = 0;
    *out_z = -1000000.0f;
    shape_base* shapes[0x2000];
    int count;
    {
        int use3d = 0;
        int ci[2];
        locate(p, ci);
        count = 0;
        cells[ci[0]][ci[1]].collect_point(p, &use3d, 0x304, shapes, &count);
    }
    {
        float pt[2];
        pt[0] = p[0];
        pt[1] = p[1];
        *out_face = 0;
        face* far_face = 0;
        float far_z = 100000.0f;
        for (; n < count; n++) {
            face* s = (face*)shapes[n];
            if (s->m4 >= 0 && s->contains_xy(pt)) {
                float z = s->w[0].z - ((pt[0] - s->w[0].x) * s->normal.x + (pt[1] - s->w[0].y) * s->normal.y) / s->normal.z;
                if (z < p[2] + 1.0f) {
                    if (*out_z < z) {
                        *out_z = z;
                        *out_face = s;
                    }
                } else if (*out_z < far_z) {
                    far_z = z;
                    far_face = s;
                }
            }
        }
        if (*out_z <= -900000.0f) {
            *out_z = far_z;
            *out_face = far_face;
        }
    }
}

// @ 0x439d40
float level::floor_z(const float* p)
{
    float z;
    face* f;
    floor_at(p, &z, &f);
    return z;
}

// sweep the segment a->b (clipped to maxlen) with a thin capsule; false if the height field blocks it or a shape
// is hit (that shape goes to *hit)
// @ 0x439d60
bool level::trace_ray(const float* a, const float* b, float maxlen, shape_base** hit, int unused)
{
    bool ok = true;
    if (hit) *hit = 0;
    if (hf == 0 || (ok = hf->line_clear((const vector4*)a, (const vector4*)b))) {
        vector4 d;
        d.x = b[0] - a[0];
        d.y = b[1] - a[1];
        d.z = b[2] - a[2];
        float len = d.normalize();
        if (maxlen < len) len = maxlen;
        vector4 t;
        t.x = d.x * len;
        t.y = d.y * len;
        t.z = d.z * len;
        d.x = t.x + a[0];
        d.y = t.y + a[1];
        d.z = t.z + a[2];
        capsule cap(0.015f, (const vector4*)a, &d);
        shape_base* shapes[0x2000];
        int n;
        query_segment(a, (const float*)&d, 0x114, shapes, &n);
        int i = 0;
        for (; i < n; i++) {
            if (!ok) break;
            ok = shape_contact_count(shapes[i], &cap, 0, 0) == 0;
        }
        if (!ok) {
            if (hit) *hit = shapes[i - 1];
        }
    }
    return ok;
}

// ---- animation controller ------------------------------------------------------------------------------------------------------------
// remove a shape from the controller's own list, else from the first region list that holds it
// @ 0x439ec0
void Skeleton::remove_shape(shape_base* s)
{
    int i;
    if (s) {
        bool done = false;
        std::vector<shape_base*>::iterator it = std::remove(shapes.begin(), shapes.end(), s);
        if (it == shapes.end()) {
            std::vector<shape_base*>::iterator r;
            for (i = 0; i < nbones; i++) {
                if (done) break;
                r = std::remove(regs[i].shapes.begin(), regs[i].shapes.end(), s);
                if (r != regs[i].shapes.end()) {
                    regs[i].shapes.erase(r, regs[i].shapes.end());
                    done = true;
                }
            }
        } else
            shapes.erase(it, shapes.end());
    }
}

// set up channel `rec` to run an animation between key a and key b
// @ 0x439fc0
void Skeleton::start_anim(int rec, Anim* poly, int a, int b)
{
    if (poly->elems.size() >= (unsigned)nbones) {
        chans[rec].k44 = 0x33;
        chans[rec].k48 = 0x33;
        chans[rec].k4c = a;
        chans[rec].k50 = b;
        chans[rec].va = poly->point(a);
        chans[rec].vb = poly->point(b);
        chans[rec].anim = poly;
        for (int i = 0; i < nbones; i++) {
            chans[rec].pa[i] = poly->elems[i].point(a);
            chans[rec].pb[i] = poly->elems[i].point(b);
        }
    }
}

// all shapes of the controller and of its regions, at most 32
// @ 0x43a130
void Skeleton::all_shapes(shape_base** out, int* n)
{
    int i = 0;
    *n = 0;
    for (unsigned k = 0; k < shapes.size(); k++) {
        if (*n < 0x20) {
            out[*n] = shapes[k];
            (*n)++;
        }
    }
    for (int j = 0; j < nbones; j++) {
        for (unsigned k = 0; k < regs[j].shapes.size(); k++) {
            if (*n < 0x20) {
                out[*n] = regs[j].shapes[k];
                (*n)++;
            }
        }
    }
}

// ---- string table ("STR NOT FOUND: ") ---------------------------------------------------------------------------------------------------
// @ 0x43a1e0
tstring* StrTable::lookup(const tstring& key)
{
    unsigned h = hash_string(&key);
    unsigned n = keys[h].v.size();
    for (unsigned i = 0; (int)i < (int)n; i++) {
        if (keys[h].v[i].equals(key))
            return &vals[h].v[i];
    }
    notfound.assign("STR NOT FOUND: ");
    notfound.append(key);
    return &notfound;
}

// stand-in caller for the register convention of ui_weapon_class_name (type in EAX, destination buffer in ESI): the
// real caller is far away (0x4598d5); this function only makes cl pass the buffer in ESI and is not in the binary
int ui_other2(char*, int);
// @ 0x43a280
static __declspec(noinline) void ui_weapon_class_name(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_one_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_two_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_polearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 3) {
        tstring k("ui_archery");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 4) {
        tstring k("ui_crossbows");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 5) {
        tstring k("ui_throwing");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 6) {
        tstring k("ui_firearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}
void ui_weapon_class_name_user(int t, int x, char* q)
{
    char* p = q + x;
    ui_weapon_class_name(t, p);
    ui_other2(p, 1);
    ui_other2(p, 2);
}

// the troop record's display name through the translation table (the record itself when absent)
// @ 0x43a480
tstring* TroopRec::display_name()
{
    tstring* t = g_strtable.find(&key);
    return t ? t : (tstring*)this;
}

// ---- <vector> helpers (VC7.1) --------------------------------------------------------------------------------------------------------------
// @ 0x43a4a0
E12* vec12::ufill(E12* f, unsigned n, const E12& v)
{
    ptr_tag t;
    uninit_fill_n12(f, n, v, *this, t);
    return f + n;
}
// @ 0x43a4d0
tstring* vecstr::ucopy(tstring* f, tstring* l, tstring* d) { ptr_tag t; return uninit_copy_str(f, l, d, *this, t); }
// @ 0x43a4f0
int* vecint::ucopy(int* f, int* l, int* d) { ptr_tag t; return uninit_copy_int(f, l, d, *this, t); }

// ---- the agent's hand position -----------------------------------------------------------------------------------------------------------------
// @ 0x43a510
void Agent::hand_pos(vector4* out)
{
    int c = team;
    if (c < 0 || c >= 8) c = 7;
    int h = 0;
    if (troop >= 0) {
        if (f2c >= 0) h = 2;
        else h = (g_game->chars[troop].flags30 & 0x4000000i64) != 0 ? 1 : 0;
    }
    ClassInfo* r = &g_mission->teams[c].cls[h];
    float dy = r->dir.y;
    float ndx = -r->dir.x;
    bool left = false;
    float k = 1.0f;
    if (r->mode == 1) {
        k = 0.7f;
        if (f1cc & 1) left = true;
    } else if (r->mode == 2) {
        k = 1.0f;
    } else if (r->mode == 3) {
        k = 1.65f;
    } else if (r->mode == 4) {
        k = 2.5f;
    }
    if (f2c != -1) k *= 1.6f;
    *out = r->pos;
    float nx = (r->dir.x * f5344) * k + out->x;
    out->x = nx;
    float ny = (r->dir.y * f5344) * k + out->y;
    out->y = ny;
    if (left) {
        out->x = nx - k * r->dir.x;
        out->y = ny - k * r->dir.y;
    }
    float t8 = (dy * f5340) * k;
    float x2 = t8 + out->x;
    out->x = x2;
    float y2 = (ndx * f5340) * k + out->y;
    out->y = y2;
    float t4 = (k * 0.1f) * f534c;
    out->x = x2 + (k * 0.1f) * f5348;
    out->y = y2 + t4;
    float height = x2;
    face* hit;
    g_mission->scene_level->floor_at(&out->x, &height, &hit);
    out->z = height;
}

// remove the entity of the agent's body from the scene
// @ 0x43a6f0
void Agent::release_body()
{
    g_mission->scene_level->remove_entity(body->id);
    body = 0;
}

// first shape in the box of s (mask 0x8116, or 0x811e when *flag) that touches it; its contact point goes to *out_point
// @ 0x43a720
int __stdcall probe_shape_contact(shape_base* s, vector4* out_point, shape_base** out_shape, const char* flag)
{
    vector4 mn, mx;
    shape_base* found[0x2000];
    int n;
    contact c;
    s->get_min(&mn);
    s->get_max(&mx);
    unsigned mask = 0x8116;
    if (*flag)
        mask = 0x811e;
    g_mission->scene_level->query_box(&mn.x, &mx.x, mask, found, &n);
    int count = n;
    for (int i = 0; i < count; i++) {
        if (shape_contact_count(s, found[i], 1, &c) != 0) {
            *out_point = c.point;
            *out_shape = found[i];
            return 1;
        }
    }
    return 0;
}

// quiver display: the arrows left in each ammo slot drive the display mesh
// @ 0x43a800
void Agent::update_quiver()
{
    for (int i = 0; i < 4; i++) {
        if (pairs[i].item >= 0) {
            ItemKind* ik = &g_items[pairs[i].item];
            unsigned t = ik->type_flags & 0xff;
            if ((t == 5 || t == 6 || t == 10) && disp[i] != 0) {
                int used = pairs[i].m4 & 0xff;
                int n = pairs[i].max_amount() - used;
                if (links[i] != 0 && links[i]->vis != 0) n--;
                float v = (float)((n * 5 + 5) * 2);
                disp[i]->cap_all(&v);
            }
        }
    }
}

// ---- ragdoll / static physics bodies --------------------------------------------------------------------------------------------------------------------
// @ 0x43a8a0
bool PhysSet::none_sleeping(int idx)
{
    for (unsigned i = 0; i < recs[idx].bones.size(); i++) {
        if (NewtonBodyGetSleepingState(NewtonRagDollBoneGetBody(recs[idx].bones[i])))
            return false;
    }
    return true;
}
// @ 0x43a900
void PhysSet::rebuild_static_body(int idx)
{
    NewtonTreeCollisionEndBuild(recs[idx].collision, 0);
    recs[idx].body = NewtonCreateBody(recs[idx].world, recs[idx].collision);
    NewtonReleaseCollision(recs[idx].world, recs[idx].collision);
    matrix m;
    float* mp = (float*)identity_pos(&m);
    PhysRec& r = recs[idx];
    NewtonBodySetMatrix(r.body, mp);
}

// start sub-animation `cmd` on channel idx (phase continuity from the previous sub-animation's flags and offset)
// @ 0x43a980
void Agent::start_channel(int idx, SubAnim* cmd, unsigned* in_flags, float* base)
{
    float cur = body->skel->rec_phase(idx);
    float scale = cmd->scale;
    float len = 1.5f;
    if (cmd->flags & 0xff)
        len = (float)(int)((cmd->flags & 0xff) - 1) * 0.1f;
    if (anim[idx].events & 0x400000) {
        float inv = 1.0f / anim[idx].rate;
        scale = inv * scale;
        len = inv * len;
    }
    body->skel->chans[idx].period = scale;
    body->skel->start_anim(idx, (Anim*)cmd->anim, cmd->a, cmd->b);
    body->skel->chans[idx].blend = len + 0.01f;
    body->skel->reset_rec(idx);
    if (cmd->flags & 0x10000000)
        body->skel->chans[idx].mode = 0x14;
    else
        body->skel->chans[idx].mode = 0x15;
    if (cmd->flags & 0xa0000000) {
        float v = 0.0f;
        unsigned fl = *in_flags;
        if (fl & 0x40000000)
            v = -*base;
        else if (fl & 0x20000000)
            v = cur - *base;
        else if (fl & 0x80000000)
            v = -cur - *base;
        float res;
        if (cmd->flags & 0x20000000)
            res = v + cmd->offset;
        else
            res = cmd->offset - v;
        body->skel->set_rec_phase(idx, res);
    } else if (cmd->flags & 0x40000000) {
        body->skel->set_rec_phase(idx, cmd->offset);
    }
}

// std::_Uninit_fill_n for the 0x1c8-byte elements (catch funclet 0x43abbb: destroys [s, f) then `throw;`)
// @ 0x43ab50
void __cdecl uninit_fill_n1c8(E1c8* f, unsigned n, const E1c8& v, allocE1c8& al, ptr_tag tag)
{
    E1c8* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) s->~E1c8();
        throw;
    }
}

// nearest nav face to pos by a depth-limited neighbour search from *hint
// @ 0x43abf0
int level::locate_near(const vector4* pos, int* hint, int* mode)
{
    int face = -1;
    float dist = 100000.0f;
    nav->next_stamp();
    nav_search(&pos->x, hint, mode, 0, &face, &dist);
    return face;
}

// ---- scene light accumulator ---------------------------------------------------------------------------------------------------------------------------
// @ 0x43ac40
void level::light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale)
{
    *out_dir = vector4(0.0f, 0.0f, 0.0f);
    *out_col = vector4(0.0f, 0.0f, 0.0f);
    Light** it = lights_first;
    if (it < lights_last) {
        unsigned skip_dir = flags & 2;
        do {
            if (skip_dir && (*it)->type == 2) goto next;
            if ((flags & 4) && (*it)->type != 2) goto next;
            {
                Light* l = *it;
                int type = l->type;
                double dist;
                vector4 d;
                float k;
                if (type == 2) {
                    dist = 1000.0f;
                    d = l->dir;
                    k = scale;
                } else {
                    d = vsub(p, l->pos);
                    dist = d.normalize();
                    k = 1.0f;
                }
                float dp = vdot(n, vneg(d));
                if (0.0f < dp && (type == 2 || dist < l->range)) {
                    vector4 hp = l->pos;
                    if (type == 2) {
                        vector4 t = vmul(d, (float)dist);
                        hp = vsub(p, t);
                    } else {
                        k = (float)(1.0 / (dist * dist) * k);
                    }
                    bool nothing = (flags == 0);
                    if (nothing) {
                        vector4 p0 = vsub(p, vmul(d, 0.05f));
                        if (!trace_ray(&p0.x, &hp.x, 150.0f, 0, 0)) goto next;
                    }
                    l = *it;
                    if (l->flags & 1) k = k * 0.5f;
                    float t = dp * 0.9f + 0.1f;
                    float s = k * t;
                    float cr = s * l->color.x;
                    float cg = s * l->color.y;
                    float cb = s * l->color.z;
                    out_col->x = cr + out_col->x;
                    out_col->y = cg + out_col->y;
                    out_col->z = cb + out_col->z;
                    l = *it;
                    float br = (l->color.z + l->color.y) + l->color.x;
                    float ax = (d.x * k) * t;
                    float ay = (d.y * k) * t;
                    float az = (d.z * k) * t;
                    float bx = ax * 0.33f;
                    float by = ay * 0.33f;
                    float bz = az * 0.33f;
                    out_dir->x = bx * br + out_dir->x;
                    out_dir->y = by * br + out_dir->y;
                    out_dir->z = br * bz + out_dir->z;
                }
            }
        next:
            it++;
        } while (it < lights_last);
    }
    float len = (float)sqrt(out_dir->x * out_dir->x + (out_dir->y * out_dir->y + out_dir->z * out_dir->z));
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        out_dir->x = inv * out_dir->x;
        out_dir->y = inv * out_dir->y;
        out_dir->z = inv * out_dir->z;
    } else {
        out_dir->x = 0.0f; out_dir->y = 1.0f; out_dir->z = 0.0f;
    }
}

