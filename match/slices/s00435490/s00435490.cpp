// Slice s00435490: small agent/mission accessors, time-of-day tests, array helpers.
#include "s00435490.h"
#include <string.h>
#include <algorithm>

extern __int64 g_time[];          // 0x5e2948 clock table (1e-5 s ticks), g_time[1] at 0x5e2950

// callees (other slices)
// 0x4352f0 PlacedObj::reset()
// 0x432510 clamp_index(const int*, const int*, const int*) (s00432130)
int __cdecl clamp_index(const int* v, const int* lo, const int* hi);
// 0x4b09e0 event dispatch
// 0x42bdd0 tstring::equals

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
    if (slot_pair(key).a >= 0) {
        ItemRec* r = &g_items[slot_pair(key).a];
        unsigned t = r->type & 0xff;
        if (t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11) {
            unsigned m = r->flags & 0xff000;
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
        name = g_recs20[*id].name;
        o->value = *value;
    } else if (k == 0) {
        name = g_recs78[*id].name;
    } else {
        int zero = 0;
        o->value = clamp_index(value, &zero, (const int*)0x590a9c);
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
