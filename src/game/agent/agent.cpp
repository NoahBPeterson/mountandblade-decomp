//-----------------------------------------------------------------------------
// File: agent.cpp  (0x434b20..0x437830)
//
// Desc: item references and their modifier-adjusted stats, the troop record's accessors, the agent's camera,
//       steering and equipment queries, sound selection, key-frame sampling, the mission accessors and time of
//       day, the height-field line test, a 2-D grid walker, and the small STL instantiations the linker kept
//       from this object. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#include <math.h>
#include <string.h>
#include <new>
#include <algorithm>
#include "./agent.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/mat4.h"
#include "../../include/quaternion.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../../include/height_field.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)

// inline helpers: the original compiled them into the callers (no out-of-line copies in this object)
static inline void cross_inl(vector4& r, const vector4& a, const vector4& b)
{ r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x; }
inline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

// @ 0x434b20
tstring ItemRef::mesh_name()
{
    if (item >= 0 && item < g_item_kind_count)
        return g_items[item].mesh_name_for(b.mod);
    return tstring("Error: Invalid item");
}
// @ 0x434b70
int ItemRef::max_amount() const
{
    if (item < 0) return 0;
    int v = g_items[item].f210;
    if (v != 0 && (m4 & 0xff000000) == 0x2a000000) {
        float f = v * 0.13f;
        int d = round_float(&f);
        if (d < 1) d = 1;
        v += d;
    }
    return v;
}
// @ 0x434be0
void ItemRef::add_amount(int n)
{
    int v = (m4 & 0xff) + n;
    if (v > max_amount()) {
        v = max_amount();
        b.amount = 0;
        m4 |= v;
        return;
    }
    b.amount = 0;
    m4 |= v;
}
// @ 0x434c30
int ItemRef::room_left()
{
    int a = m4 & 0xff;
    return max_amount() - a;
}
// @ 0x434c50
int ItemRef::stat_1ec()
{
    if (item < 0) return 0;
    int v = g_items[item].f1ec;
    if (v != 0) {
        int m = b.mod;
        if ((char)g_items[item].type_flags == itp_type_horse) {
            if (m == imod_timid) return v - 1;
            if (m == imod_stubborn) return v + 1;
            if (m == imod_champion) return v + 2;
        } else {
            if (m == imod_heavy) return v + 1;
            if (m == imod_strong) return v + 2;
            if (m == imod_masterwork) v += 4;
        }
    }
    return v;
}
// @ 0x434cb0
unsigned ItemRef::stat_21x(int which)
{
    if (item < 0) return 0;
    unsigned v;
    if (which == 0) v = g_items[item].f218;
    else v = g_items[item].f214;
    v &= 0xff;
    if (v != 0) {
        switch (b.mod) {
        case imod_cracked: v -= 5; break;
        case imod_rusty:
        case imod_bent: v -= 3; break;
        case imod_chipped: v -= 1; break;
        case imod_crude: v -= 2; break;
        case imod_fine: v += 1; break;
        case imod_heavy: v += 2; break;
        case imod_balanced:
        case imod_strong: v += 3; break;
        case imod_tempered: v += 4; break;
        case imod_masterwork: v += 5; break;
        }
    }
    return v;
}
// @ 0x434d70
int ItemRef::stat_1f0a()
{
    if (item < 0) return 100;
    int v = g_items[item].f1f0;
    if (v != 0) {
        switch (b.mod) {
        case imod_balanced: v += 3; break;
        case imod_tempered: return v;
        case imod_masterwork: v += 1; break;
        case imod_heavy: v -= 2; break;
        case imod_bent:
        case imod_strong: v -= 3; break;
        }
    }
    return v;
}
// @ 0x434df0
int ItemRef::stat_1e8()
{
    if (item >= 0) {
        int v = g_items[item].f1e8;
        if ((char)g_items[item].type_flags == itp_type_horse) {
            if (v == 0) v = 100;
        } else if (v == 0)
            return v;
        switch (b.mod) {
        case imod_cracked: v -= 0x38; break;
        case imod_battered: v -= 0x1a; break;
        case 0x19: v += 0x2f; break;
        case 0x1b: v += 0x53; break;
        case 0x1d: v += 0x9b; break;
        case imod_heavy: v += 10; break;
        case 0x20: v += 5; break;
        }
        if (v >= 1) return v;
    }
    return 1;
}
// @ 0x434eb0
int ItemRef::stat_200()
{
    if (item < 0) return 0;
    int v = g_items[item].f200;
    if (v != 0) {
        switch (b.mod) {
        case 2:
        case 0x15: v -= 3; break;
        case 7: v -= 1; break;
        case 0x18: v += 1; break;
        case 0x19: v += 2; break;
        case 0x12:
        case 0x1a: v += 3; break;
        case 1: v -= 4; break;
        case 5:
        case 0x16: v -= 2; break;
        case 0x1b: v += 4; break;
        case 0x1d: v += 6; break;
        }
        if (v < 0) v = 0;
    }
    return v;
}
// @ 0x434f60
int ItemRef::stat_1f4()
{
    if (item < 0) return 0;
    int v = g_items[item].f1f4;
    if (v != 0) {
        switch (b.mod) {
        case 0x1e: v -= 5; break;
        case 0x1f: v -= 2; break;
        case 0x23: v += 1; break;
        case 0x24: v += 2; break;
        }
        if (v < 1) v = 1;
    }
    return v;
}
// @ 0x434fd0
int ItemRef::stat_1f0b()
{
    if (item < 0) return 0;
    int v = g_items[item].f1f0;
    if (v != 0) {
        switch (b.mod) {
        case 0x1e: v -= 5; break;
        case 0x1f: v -= 2; break;
        case 0x23: v += 1; break;
        case 0x24: v += 2; break;
        }
        if (v < 1) v = 1;
    }
    return v;
}
// @ 0x435040
int ItemRef::stat_214()
{
    if (item < 0) return 0;
    int v = g_items[item].f214 & 0xff;
    if (v != 0) {
        switch (b.mod) {
        case 0x12: v += 4; break;
        case 0x23: v += 1; break;
        case 0x24: v += 2; break;
        }
        if (v < 1) v = 1;
    }
    return v;
}
// @ 0x435090
int ItemRef::stat_204()
{
    if (item < 0) return 100;
    int v = g_items[item].f204;
    if (v == 0) v = 99;
    return v;
}
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
int slot_class(int item, int flag) { return slot_class_impl(item, flag); }

// @ 0x435130
bool Troop::has_flag10()
{
    if (flags30 & 0x10) return true;
    return false;
}
// @ 0x435150
int Troop::stat_int(int i) { return (int)f80[i]; }
// @ 0x435170
int Troop::scaled_b0()
{
    return (int)((b0 + 10) * (b0 + 10) * 0.1f);
}
// @ 0x4351a0
int Troop::nibble(int i)
{
    int v = (nib[i / 8] >> ((i % 8) * 4)) & 0xf;
    if (v < 0) return 0;
    int cap = g_skill_tbl[i].cap;
    if (v > cap) v = cap;
    return v;
}

// @ 0x4351f0
ItemRef Wielder::ranged_ref()
{
    ItemRef r;
    int it = a.item;
    if (it < 0) {
        r.item = -1;
        return r;
    }
    if ((char)g_items[it].type_flags == itp_type_thrown) {
        r.item = a.item;
        r.m4 = a.m4;
        return r;
    }
    r.item = b.item;
    r.m4 = b.m4;
    return r;
}
// @ 0x435260
int find_in_table20(const tstring* name)
{
    for (int i = 0; i < g_tbl20_count; i++)
        if (g_tbl20[i].name.equals(*name)) return i;
    return 0;
}
// @ 0x4352a0
int find_in_table78(const tstring* name)
{
    int r = -1;
    for (int i = 0; i < g_tbl78_count && r == -1; i++)
        if (g_tbl78[i].name.equals(*name)) r = i;
    return r;
}

// @ 0x4352f0
void PlacedObj::reset()
{
    m10 = 0;
    kind = 0;
    id = 0;
    value = 0;
    m1a4 = 0;
    m1b0 = 0;
    m1b4 = 0;
    m1b8 = 0;
    m168 = m14 = -1;
    md8 = rgl::identity();
    f16c = 1.0f;
    v158 = vector4(1.0f, 1.0f, 1.0f);
}
// @ 0x4353a0
RecZSubB* RecZSubB::init()
{
    vector4 zero(0.0f, 0.0f, 0.0f);
    vector4 unit(1.0f, 0.0f, 0.0f);
    v8 = zero;
    v18 = unit;
    v28 = unit;
    m0 = 0;
    m4 = -1;
    m38 = 0;
    m3c = 0;
    m48 = 0;
    m4c = -1;
    m54 = 0;
    m40 = 0;
    m44 = 0;
    return this;
}

// @ 0x435440
int ftoi_helper(int x)
{
    int r = -1;
    switch (x) {
    case 0: return 4;
    case 1: return 5;
    case 2: return 6;
    case 3: return 7;
    case 4: r = 3;
    }
    return r;
}

// @ 0x435490
int Agent::clamped_team() const
{
    int t = team;
    if (t < 0 || t >= 8) t = 7;
    return t;
}
// @ 0x4354b0
void Agent_f8::set_slot(int i, float x) { v[i][0] = x; }
// @ 0x4354d0
bool Agent::flag_0x400_or_0x114() const
{
    bool r = false;
    if ((int)(fe8 & 0x1ffff) >= 100 && fd4 >= 0 && ((*f12c)[fd4].flags & 0x400) != 0 || (int)(f114 & 0x1ffff) >= 100)
        r = true;
    return r;
}
// @ 0x435520
void Agent::classify(int key)
{
    if (slot_pair(key).item >= 0) {
        ItemKind* r = &g_items[slot_pair(key).item];
        unsigned t = r->type_flags & 0xff;
        if (t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11) {
            unsigned m = (unsigned)r->flags & 0xff000;
            int zero = 0;
            if ((m == 0x30000 && !zero) || (m == 0x20000 && !zero))
                f56c4 = 1;
            else
                f56c4 = 0;
        } else {
            f56c4 = (t == 7) * 2 + 1;
        }
    }
}
// @ 0x4355f0
float Agent::scale_f4() const
{
    int n = (f4 >> 4) & 0xf;
    if (n == 0) n = 5;
    return n * 0.0625f;
}
// @ 0x435620
float Agent::scale_f10() const
{
    int n = (f10 >> 8) & 0xf;
    if (n == 0) n = 10;
    return n * 0.1f;
}
// @ 0x435650
void Placement::set_matrix(const matrix* m)
{
    vector4 z(0.0f, 0.0f, 0.0f);
    v13fc4 = z;
    vector4 one(1.0f, 1.0f, 1.0f);
    v13fb4 = one;
    m14014 = *m;
}
// @ 0x4356e0
float time_of_day() { return fmod(g_time[1] * 1e-5f, 24.0f); }
// @ 0x435700
bool is_daytime()
{
    float t = time_of_day();
    return 4.0f <= t && t < 20.0f;
}
// @ 0x435740
bool is_night() { return !is_daytime(); }
// @ 0x435790
char SoundCtl::emit_event(int id, int a, int b)
{
    int count;
    __int64 v[16];
    v[0] = a;
    v[1] = b;
    unsigned tag;
    if (f1a068 && g_emitters[id].name.equals(name))
        tag = (id << 16) | f1a07c;
    else
        tag = (unsigned)-1;
    char r = 0;
    count = 2;
    if (g_emitter_idx[id] >= 0)
        r = g_emitters[g_emitter_idx[id]].sub.emit(0x200000, 0, &count, v, tag);
    return r;
}
// @ 0x435840
char Mission::mode_flag1() const { return ((g_modes[f5df0].flags >> 1) & 1) != 0; }
// @ 0x435860
void __stdcall init_placed(PlacedObj* o, const int* kind, const int* id, const int* value, const matrix* m)
{
    tstring name;
    o->reset();
    int k = *kind;
    if (k == 2 || k == 6 || k == 7) {
        name = g_items[*id].name;
        o->value = *value;
    } else if (k == 4) {
        name = g_tbl20[*id].name;
        o->value = *value;
    } else if (k == 0) {
        name = g_tbl78[*id].name;
    } else {
        int zero = 0;
        o->value = clamp_index(value, &zero, &g_value_max);
    }
    o->m18 = *m;
    o->m98 = *m;
    o->kind = *kind;
    o->id = *id;
    o->name = name;
}
// @ 0x435990
int Vec38::size() const { if (!first) return 0; return (last - first) / 0x38; }
// @ 0x4359c0
int Vec2c::size() const { if (!first) return 0; return (last - first) / 0x2c; }
// @ 0x4359e0
int Vec1c8::size() const { if (!first) return 0; return (last - first) / 0x1c8; }
// @ 0x435a10
template int* std::_Copy_opt<int*, int*>(int*, int*, int*, std::_Scalar_ptr_iterator_tag);
// @ 0x435a40
void __cdecl copy_ints_backward(int* first, int* last, int* dest_end)
{
    int n = last - first;
    memmove(dest_end - n, first, n * 4);
}
// @ 0x435a70
void __cdecl fill_tstrings(tstring* first, tstring* last, const tstring* val)
{
    for (; first != last; ++first) *first = *val;
}
// @ 0x435aa0
void __cdecl fill_triples(Triple* first, Triple* last, const Triple* val)
{
    for (; first != last; ++first) *first = *val;
}
// @ 0x435ae0
float __cdecl accel_factor(const float* a, const float* b, const float* c)
{
    float p = (float)pow((double)*a, (double)1.95f);
    float t = *b / p * 5.1f;
    float r;
    if (*c > 0.0f) {
        r = *c / (*a * *a) + t;
    } else {
        r = *c / (*a * *a) * 0.2f + t;
        if (0.0f > r) return 0.0f;
    }
    if (r > 1.0f) r = 1.0f;
    return r;
}
// @ 0x435b60
int __cdecl team_field5280(const Agent* a)
{
    int t = a->team;
    if (t < 0 || t >= 8) t = 7;
    return g_mission->field5280[t * 0x5b];
}
static char mission_mode_flag1() { return ((g_modes[g_mission->f5df0].flags >> 1) & 1) != 0; }
// @ 0x435b90
void Agent::reset_if_enemy_charged()
{
    if (mission_mode_flag1()) {
        Mission* ms = g_mission;
        int i = 0;
        const TeamSlot* s = ms->slots;
        const int* rel = &ms->relation[team * 0x5b];
        do {
            if (*rel == -1 && (s->b > 0 || s->a > 0)) {
                f2c8 = 12;
                f533c = 0;
                return;
            }
            i++; rel++; s++;
        } while (i < 8);
    }
}

// @ 0x435c10
void Agent::camera_params(CamParams* p)
{
    if (f10 == 0) {
        vector4 a(f80 * 0.0f, f80 * 0.0f, f2c == -1 ? f80 * 0.95f : f80 * 1.5f);
        float dx = a.x - v98.x;
        float dy = a.y - v98.y;
        float dz = a.z - v98.z;
        p->v40 = vector4(f80 * 0.0f + dx, f80 * 0.0f + dy, f80 * -0.2f + dz);
        p->v50 = vector4(f80 * 0.0f + dx, f80 * 0.0f + dy, f80 * 0.5f + dz);
        p->f60 = 0.37f;
        if (body && (fe8 & 0x800000)) {
            p->v50 = body->skel->bones->v1170;
            BoneRec* z = body->skel->bones;
            p->v40.x = (z->f46c + z->fa00) * 0.5f;
            p->v40.y = (z->fa04 + z->f470) * 0.5f;
            p->v40.z = (z->fa08 + z->f474) * 0.5f;
        }
    } else {
        p->v40 = vector4(0.0f, -0.3f, 1.05f);
        p->v50 = vector4(0.0f, 0.67f, 1.15f);
        p->f60 = 0.4f;
    }
    if (f57d8) {
        p->v40.z = 2.0f;
        p->v50.z = 2.1f;
        p->f60 = 0.01f;
    }
    if (f10 == 0 && f5830 != 2) {
        p->v40.z = 4.0f;
        p->v50.z = 4.1f;
        p->f60 = 0.001f;
        return;
    }
    if (f10 == 1 && f14 != 1) {
        p->v40.z = 5.0f;
        p->v50.z = 5.1f;
        p->f60 = 0.001f;
    }
}
// @ 0x435f10
void Agent::steer(float dt)
{
    float k = 3.0f;
    if (f8 == g_mission->f5c) k = 6.0f;
    float* pv = &vel.x;
    float d = vel.z * dirz + diry * pv[1] + vel.x * dirx;
    if (!(d > 0.0f)) d = 0.0f;
    float s = (d * 5.0f + k) * dt;
    vector4 add(s * dirx, s * diry, s * dirz);
    pv[0] += add.x;
    pv[1] += add.y;
    pv[2] += add.z;
    vel.normalize();
}
// @ 0x435fc0
ItemRef Agent::best_weapon_slot() const
{
    for (int i = 3; i >= 0; i--) {
        const ItemRef& p = pairs[i];
        if (p.item < 0 || (char)g_items[p.item].type_flags != 7) continue;
        if (slots[1] == i && slots[4] == 0) continue;
        if ((int)(p.m4 & 0xffff) <= 10) continue;
        return pairs[i];
    }
    ItemRef r;
    r.item = -1;
    return r;
}
// @ 0x436050
float Agent::equipment_value() const
{
    float sum = 0.0f;
    const ItemRef* p = pairs;
    for (int n = 8; n != 0; n--, p++) {
        if (p->item >= 0) {
            float v = g_items[p->item].f208;
            unsigned t = g_items[p->item].type_flags & 0xff;
            if (t == 5 || t == 6 || t == 0x12 || t == 10) {
                int cur = p->m4 & 0xff;
                v = (float)(p->max_amount() - cur) / (float)p->max_amount() * v;
            } else if (t == 7 && (int)(p->m4 & 0xffff) <= 0) {
                v = 0.0f;
            }
            sum += v;
        }
    }
    return sum;
}
// @ 0x436130
void Agent::update_pitch(float dt)
{
    if (f10 != 0 && fbc != 0) {
        const Obj_a0* o = fbc;
        vector4 c; cross_inl(c, o->d, vel);
        vector4 u; cross_inl(u, o->d, c);
        u.normalize();
        f56cc = (float)((asin(u.z) - f56cc) * dt * 8.0f + f56cc);
    }
}

// @ 0x436210
void __cdecl rotate_scale_xy(vector4* out, const vector4* in, const float* angle, const vector4* scale)
{
    vector4 t = *in;
    float a = *angle;
    float c = (float)cos(a);
    double s = sin(a);
    t.z = 0.0f;
    float tx = t.x * c - t.y * s;
    float ty = t.x * s + t.y * c;
    t.y = ty; t.x = tx;
    t.normalize();
    float len = (float)sqrt(scale->x * scale->x + scale->y * scale->y);
    t.x = len * t.x; t.y = len * t.y; t.z = scale->z;
    *out = t;
}

// Sound ids (provisional): triples (a, b, c) of consecutive sound ids per action.
// @ 0x4362e0
void Agent::pick_sounds(int key, int mode, int* snd_a, int* snd_b, int* snd_c, int* variant)
{
    *snd_a = -1;
    *snd_b = -1;
    *snd_c = -1;
    *variant = 0;
    int item;
    {
        ItemRef sp = slot_pair(key);
        item = sp.item;
    }
    if (item == -1) {
        if (mode == 2) {
            *snd_a = 0x8e; *snd_b = 0x8f; *snd_c = 0x90;
            return;
        }
        if (mode == 3) {
            *snd_a = 0x91; *snd_b = 0x92; *snd_c = 0x93;
            return;
        }
        if (mode == 0) {
            *snd_a = 0x94; *snd_b = 0x95; *snd_c = 0x96;
            return;
        }
        *snd_a = 0x8b; *snd_b = 0x8c; *snd_c = 0x8d;
        return;
    }
    bool no_off;
    {
        ItemRef o = slot_pair_inl(1);
        no_off = o.item == -1;
    }
    unsigned __int64 fl = g_items[item].flags;
    if (f2c == -1) {
        if (mode == 0) {
            if ((fl & 0x10) && no_off) {
                *snd_a = 0xa0; *snd_b = 0xa1; *snd_c = 0xa2;
                return;
            }
            unsigned __int64 x100 = fl & 0x100;
            if (x100 && no_off) {
                *snd_a = 0xbe; *snd_b = 0xbf; *snd_c = 0xc0;
                return;
            }
            if ((fl & 1) && (x100 == 0 || !no_off)) {
                *snd_a = 0xa6; *snd_b = 0xa7; *snd_c = 0xa8;
                return;
            }
            if ((fl & 0x4000000) && (x100 == 0 || !no_off)) {
                *snd_a = 0xa9; *snd_b = 0xaa; *snd_c = 0xab;
                return;
            }
        } else if (mode == 1) {
            if ((fl & 0x40) && no_off) {
                *snd_a = 0x9a; *snd_b = 0x9b; *snd_c = 0x9c;
                return;
            }
            if ((fl & 0x400) && no_off) {
                *snd_a = 0xc4; *snd_b = 0xc5; *snd_c = 0xc6;
                return;
            }
            if (fl & 4) {
                *snd_a = 0xac; *snd_b = 0xad; *snd_c = 0xae;
                return;
            }
        } else if (mode == 2) {
            if ((fl & 0x80) && no_off) {
                *snd_a = 0x9d; *snd_b = 0x9e; *snd_c = 0x9f;
                return;
            }
            if ((fl & 0x800) && no_off) {
                *snd_a = 0xc1; *snd_b = 0xc2; *snd_c = 0xc3;
                return;
            }
            if (fl & 8) {
                *snd_a = 0xaf; *snd_b = 0xb0; *snd_c = 0xb1;
                return;
            }
        } else if (mode == 3) {
            if ((fl & 0x20) && no_off) {
                *snd_a = 0xa3; *snd_b = 0xa4; *snd_c = 0xa5;
                return;
            }
            if ((fl & 0x200) && no_off) {
                *snd_a = 0xbb; *snd_b = 0xbc; *snd_c = 0xbd;
                return;
            }
            if (fl & 2) {
                *snd_a = 0xb2; *snd_b = 0xb3; *snd_c = 0xb4;
                return;
            }
        }
    } else if (mode == 0) {
        if (fl & 1) {
            *snd_a = 0xa6; *snd_b = 0xa7; *snd_c = 0xa8; *variant = 2;
            return;
        }
        if (fl & 0x4000000) {
            *snd_a = 0xa9; *snd_b = 0xaa; *snd_c = 0xab; *variant = 2;
            return;
        }
    } else if (mode == 2) {
        if (fl & 0x1000000) {
            *snd_a = 0xb8; *snd_b = 0xb9; *snd_c = 0xba;
            return;
        }
    } else if (mode == 1) {
        if (fl & 0x800000) {
            *snd_a = 0xb5; *snd_b = 0xb6; *snd_c = 0xb7;
            return;
        }
    } else if (mode == 3 && (fl & 2)) {
        *snd_a = 0xb2; *snd_b = 0xb3; *snd_c = 0xb4; *variant = 1;
    }
}
// @ 0x4367c0
float vec2::normalize()
{
    float len = (float)sqrt(y * y + x * x);
    if (len > 0.0f) {
        float inv = 1.0f / len;
        x = inv * x;
        y = inv * y;
    } else {
        x = 0.0f;
        y = 1.0f;
    }
    return len;
}
// @ 0x436810
void Agent::weapon_info(int idx, unsigned* skill, int* stat)
{
    if (idx == -1) idx = slots[1];
    *skill = (unsigned)-1;
    *stat = -1;
    if (idx >= 0) {
        ItemRef* p = &pairs[idx];
        int item = p->item;
        if (item >= 0 && (char)g_items[item].type_flags == 7) {
            *stat = p->stat_1e8();
            *skill = p->m4 & 0xffff;
        }
    }
}
// @ 0x436880
ItemRef Agent::best_ammo(int idx, int* missing, int* total, int* slot_out)
{
    ItemRef r;
    *slot_out = -1;
    r.item = -1;
    if (idx == -1) idx = slots[0];
    *missing = -1;
    *total = -1;
    if (idx >= 0 && pairs[idx].item >= 0) {
        unsigned wt = g_items[pairs[idx].item].type_flags & 0xff;
        if (wt == 8 || wt == 9 || wt == 10 || wt == 0x11 || wt == 0x10) {
            *missing = 0;
            *total = 0;
            for (int i = 0; i < 4; i++) {
                ItemRef* cur = &pairs[i];
                if (cur->item >= 0) {
                    int w = pairs[idx].item;
                    unsigned t = g_items[w].type_flags & 0xff;
                    if ((t == 8 && (char)g_items[cur->item].type_flags == 5) ||
                        (t == 9 && (char)g_items[cur->item].type_flags == 6) ||
                        (t == 0x10 && (char)g_items[cur->item].type_flags == 0x12) ||
                        (t == 0x11 && (char)g_items[cur->item].type_flags == 0x12) ||
                        (t == 10 && cur->item == w)) {
                        if (*slot_out == -1) {
                            r = *cur;
                            *slot_out = i;
                        }
                        if (*missing == 0) {
                            unsigned b = cur->m4;
                            if (cur->max_amount() != (int)(b & 0xff)) {
                                r = *cur;
                                *slot_out = i;
                            }
                        }
                        unsigned b2 = cur->m4;
                        *missing += cur->max_amount() - (b2 & 0xff);
                        *total += cur->max_amount();
                    }
                }
            }
        }
    }
    return r;
}
// @ 0x436a70
int Agent::total_missing_ammo()
{
    int sum = 0;
    const ItemRef* p = pairs;
    for (int n = 4; n != 0; n--, p++) {
        if (p->item >= 0) {
            unsigned t = g_items[p->item].type_flags & 0xff;
            if (t == 5 || t == 6 || t == 0x12 || t == 10) {
                unsigned cur = p->m4 & 0xff;
                sum += p->max_amount() - cur;
            }
        }
    }
    return sum;
}
// @ 0x436ae0
quaternion KeyTable::sample(int key)
{
    float k = (float)key;
    int cur = 0;
    float frac;
    find(&cur, &frac, &k);
    if (key == keys[cur]) return vals[cur];
    quaternion tmp;
    slerp(&tmp, &vals[cur], &vals[cur + 1], &frac);
    return tmp;
}
// @ 0x436b90
vector4 KeyHolder::sample(int key)
{
    float k = (float)key;
    float t;
    int a, b;
    kf.find(&a, &b, &t, &k);
    if (key == a) return kf.vals[a];
    float s = 1.0f - t;
    vector4 r = vadd(vscale(t, kf.vals[b]), vscale(s, kf.vals[a]));   // args evaluate right to left
    return r;
}

// @ 0x436c90
void alloc_tstring::construct(tstring* p, const tstring& v) { new ((void*)p) tstring(v); }
// @ 0x436cb0
void alloc_rec36::construct(rec36* p, const rec36& v) { new ((void*)p) rec36(v); }
// @ 0x436cd0
void alloc_rec136::construct(rec136* p, const rec136& v) { new ((void*)p) rec136(v); }
// @ 0x436cf0
void alloc_int3::construct(int3* p, const int3& v) { new ((void*)p) int3(v); }

// @ 0x436d10 (+ catch-all funclet 0x436d76: `throw;`)  std::_Uninit_copy (non-scalar tag)
int3* uninit_copy_int3(const int3* first, const int3* last, int3* dest, alloc_int3& al)
{
    int3* next = dest;
    try {
        for (; first != last; ++dest, ++first)
            al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next)
            ;   // _Al.destroy(next): trivial, but the empty loop is what keeps dest/first in memory
        throw;
    }
    return dest;
}
// @ 0x436d80
void alloc_int1::construct(int1* p, const int1& v) { new ((void*)p) int1(v); }

// @ 0x436da0
bool HeightField::line_clear(const vector4* a, const vector4* b)
{
    const float* org = &x0;
    vec2 lo = { org[0], org[1] };
    vec2 hi = lo;
    hi.x = (float)w * cell + hi.x;
    hi.y = (float)h * cell + hi.y;
    float t0 = 0.0f;
    vector4 d;
    d.x = b->x - a->x;
    d.y = b->y - a->y;
    d.z = b->z - a->z;
    float t1 = d.normalize();
    vector4 p0;
    p0.x = d.x * t0 + a->x;
    if (p0.x < lo.x) {
        if (d.x > 0.0f)
            t0 = (lo.x - p0.x) / d.x;
    } else if (p0.x > hi.x && d.x < 0.0f)
        t0 = (hi.x - p0.x) / d.x;
    p0.y = d.y * t0 + a->y;
    if (p0.y < lo.y) {
        if (d.y > 0.0f)
            t0 += (lo.y - p0.y) / d.y;
    } else if (p0.y > hi.y && d.y < 0.0f)
        t0 += (hi.y - p0.y) / d.y;
    p0.x = d.x * t1 + a->x;
    if (p0.x < lo.x) {
        if (d.x < 0.0f)
            t1 += (lo.x - p0.x) / d.x;
    } else if (p0.x > hi.x && d.x > 0.0f)
        t1 += (hi.x - p0.x) / d.x;
    p0.y = d.y * t1 + a->y;
    if (p0.y < lo.y) {
        if (d.y < 0.0f)
            t1 += (lo.y - p0.y) / d.y;
    } else if (p0.y > hi.y && d.y > 0.0f)
        t1 += (hi.y - p0.y) / d.y;
    if (t0 >= t1)
        return true;
    vector4 p1;
    p0.x = d.x * t0 + a->x;
    p0.y = d.y * t0 + a->y;
    p0.z = d.z * t0 + a->z;
    p1.x = d.x * t1 + a->x;
    p1.y = d.y * t1 + a->y;
    p1.z = d.z * t1 + a->z;
    vec2 seg;
    seg.x = p1.x - p0.x;
    seg.y = p1.y - p0.y;
    if (sqrt(seg.y * seg.y + seg.x * seg.x) > 1e10f)
        return false;
    vec2 dir = seg;
    float len = dir.normalize();
    float acc = 0.0f;
    float tmax[2], tdelta[2], fpos[2];
    int step[2], cellidx[2];
    const float* P0 = &p0.x;
    float* dd = &dir.x;
    for (int i = 0; i < 2; i++) {
        if (dd[i] > 0.0f)
            step[i] = 1;
        else
            step[i] = -1;
        float inv = 1.0f / dd[i];
        tdelta[i] = (float)fabs(inv * cell);
        fpos[i] = (P0[i] - org[i]) / cell;
        cellidx[i] = (int)floor(fpos[i]);
        float fr = (float)fmod(P0[i] - org[i], cell);
        if (dd[i] == 0.0f)
            tmax[i] = 1e9f;
        else {
            if (step[i] > 0)
                fr = cell - fr;
            tmax[i] = (float)fabs(fr * inv);
        }
    }
    bool hit = false;
    while (!hit && (tmax[0] + acc < len || tmax[1] + acc < len)) {
        int idx = !(tmax[0] < tmax[1]);
        acc += tmax[idx];
        tmax[1 - idx] -= tmax[idx];
        float zc = (p1.z - p0.z) * (acc / len) + p0.z;
        int cx = cellidx[0], cy = cellidx[1];
        if (cx >= 0 && cx < w && cy >= 0 && cy < h) {
            int lx = cx, ly = cy;
            if (step[idx] == 1) { if (idx == 0) lx++; else ly++; }
            int hx = lx, hy = ly;
            if (idx == 0) hy++; else hx++;
            float fr = (float)fabs((dd[1 - idx] / cell) * tmax[1 - idx]);
            if (step[1 - idx] == 1)
                fr = 1.0f - fr;
            if (zc < (1.0f - fr) * cols[lx][ly].h + fr * cols[hx][hy].h)
                hit = true;
        }
        cellidx[idx] += step[idx];
        tmax[idx] = tdelta[idx];
    }
    return !hit;
}

// @ 0x437290
void Grid2::walk(const float* a, const float* b, int (*cells)[2], int* count)
{
    vec2 d;
    d.x = b[0] - a[0];
    d.y = b[1] - a[1];
    vec2 dir = d;
    float l = (float)sqrt(dir.x * dir.x + dir.y * dir.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        dir.x = inv * dir.x;
        dir.y = inv * dir.y;
    } else {
        dir.x = 0.0f;
        dir.y = 1.0f;
    }
    int step[2], cur[2];
    float tdelta[2], tmax[2], fpos[2];
    float* dd = &dir.x;
    for (int i = 0; i < 2; i++) {
        if (dd[i] > 0.0f)
            step[i] = 1;
        else
            step[i] = -1;
        float inv = 1.0f / dd[i];
        tdelta[i] = (float)fabs(inv * cell[i]);
        fpos[i] = (a[i] - mn[i]) / cell[i];
        cur[i] = (int)floor(fpos[i]) + 1;
        if (cur[i] < 0)
            cur[i] = 0;
        else if (cur[i] > dim[i] - 1)
            cur[i] = dim[i] - 1;
        if (a[i] < mn[i]) {
            if (step[i] < 0)
                tmax[i] = 1e10f;
            else
                tmax[i] = (float)fabs((mn[i] - a[i]) * inv);
        } else if (a[i] > mx[i]) {
            if (step[i] > 0)
                tmax[i] = 1e10f;
            else
                tmax[i] = (float)fabs((a[i] - mx[i]) * inv);
        } else {
            float fr = (float)fmod(a[i] - mn[i], cell[i]);
            if (step[i] > 0)
                fr = cell[i] - fr;
            tmax[i] = (float)fabs((double)(fr * inv));
        }
    }
    int* pc = cur;
    *count = 0;
    cells[0][0] = cur[0];
    cells[0][1] = cur[1];
    int n = *count + 1;
    *count = n;
    float acc = 0.0f;
    float total = (float)sqrt(d.y * d.y + d.x * d.x);
    while (total > tmax[0] + acc || total > tmax[1] + acc) {
        int idx;
        if (tmax[0] < tmax[1])
            idx = 0;
        else
            idx = 1;
        cur[idx] += step[idx];
        cells[n][0] = cur[0];
        cells[n][1] = cur[1];
        n = *count + 1;
        *count = n;
        float* tp = &tmax[idx];
        float* to = &tmax[1 - idx];
        *to -= *tp;
        acc += *tp;
        if (pc[idx] == 0)
            *tp = 1e10f;
        else if (pc[idx] == dim[idx] - 1)
            *tp = 1e10f;
        else
            *tp = tdelta[idx];
    }
}

// @ 0x437510
void ItemRef::reduce(int n)
{
    if (item > 0) {
        unsigned t = g_items[item].type_flags & 0xff;
        if (t == itp_type_thrown || t == itp_type_arrows || t == itp_type_bolts || t == itp_type_pistol ||
            t == itp_type_musket || t == itp_type_crossbow || t == itp_type_bullets) {
            if (n <= 0) {
                b.amount = 0;
                return;
            }
            unsigned v = (m4 & 0xff) - n;
            b.amount = 0;
            m4 = m4 | (((int)v < 0) - 1 & v);
        } else if (t == itp_type_shield || t == itp_type_horse || t == itp_type_animal) {
            unsigned v = stat_1e8();
            *(short*)&m4 = 0;
            m4 |= v;
        }
    }
}

// @ 0x4375b0
float Troop::falloff(float dist, float k)
{
    float f = 70.0f / (dist + 70.0f);
    int n = (nib[3] >> 12) & 0xf;
    if (n < 0)
        n = 0;
    else if (n > g_clamp_level)
        n = g_clamp_level;
    int tier = n;
    if (tier >= 11)
        tier = 10;
    float th = (float)g_tier_dist[tier];
    if (dist > th)
        f = f / (dist - th + 20.0f) * 20.0f;
    if (dist > 50.0f) f *= 0.65f;
    if (dist > 100.0f) f *= 0.65f;
    if (dist > 150.0f) f *= 0.65f;
    if (dist > 200.0f) f *= 0.65f;
    if (dist > 250.0f) f *= 0.65f;
    if (dist > 300.0f) f *= 0.65f;
    if (dist > 350.0f) f *= 0.65f;
    if (dist > 400.0f) f *= 0.65f;
    if (dist > 450.0f) f *= 0.65f;
    if (dist > 500.0f) f *= 0.65f;
    if (dist > 550.0f) f *= 0.65f;
    if (dist > 600.0f) f *= 0.65f;
    if (dist > 650.0f) f *= 0.65f;
    if (dist > 700.0f) f *= 0.65f;
    return f * k + f * k + dist;
}

// The same function seen from outside this TU (cl must not assume the register use of the body above).
struct TroopExt { float falloff(float dist, float k); };   // 0x4375b0
// @ 0x437770
void Troop::adjust(int i, float k)
{
    float r = ((TroopExt*)this)->falloff(f80[i], k);
    f80[i] = r;
    if (r < 0.0f) {
        f80[i] = 0.0f;
        return;
    }
    if (r > 1000.0f)
        f80[i] = 1000.0f;
}

// @ 0x4377d0
RecJ::RecJ()
{
    slot1();
}
// @ 0x437810
void RecJ::slot1()
{
    m50 = 0;
    m98 = 0;
    m88 = -1;
    m80 = -1;
}