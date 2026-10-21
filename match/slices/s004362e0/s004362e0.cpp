// Slice s004362e0: agent sound selection, equipment queries and key-frame sampling.
#include "s004362e0.h"

inline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

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
        Pair sp = slot_pair(key);
        item = sp.a;
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
        Pair o = slot_pair_inl(1);
        no_off = o.a == -1;
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
        Pair* p = &pairs[idx];
        int item = p->a;
        if (item >= 0 && (char)g_items[item].type == 7) {
            *stat = p->stat_1e8();
            *skill = p->b & 0xffff;
        }
    }
}
// @ 0x436880
Pair Agent::best_ammo(int idx, int* missing, int* total, int* slot_out)
{
    Pair r;
    *slot_out = -1;
    r.a = -1;
    if (idx == -1) idx = slots[0];
    *missing = -1;
    *total = -1;
    if (idx >= 0 && pairs[idx].a >= 0) {
        unsigned wt = g_items[pairs[idx].a].type & 0xff;
        if (wt == 8 || wt == 9 || wt == 10 || wt == 0x11 || wt == 0x10) {
            *missing = 0;
            *total = 0;
            for (int i = 0; i < 4; i++) {
                Pair* cur = &pairs[i];
                if (cur->a >= 0) {
                    int w = pairs[idx].a;
                    unsigned t = g_items[w].type & 0xff;
                    if ((t == 8 && (char)g_items[cur->a].type == 5) ||
                        (t == 9 && (char)g_items[cur->a].type == 6) ||
                        (t == 0x10 && (char)g_items[cur->a].type == 0x12) ||
                        (t == 0x11 && (char)g_items[cur->a].type == 0x12) ||
                        (t == 10 && cur->a == w)) {
                        if (*slot_out == -1) {
                            r = *cur;
                            *slot_out = i;
                        }
                        if (*missing == 0) {
                            unsigned b = cur->b;
                            if (cur->max_amount() != (int)(b & 0xff)) {
                                r = *cur;
                                *slot_out = i;
                            }
                        }
                        unsigned b2 = cur->b;
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
    const Pair* p = pairs;
    for (int n = 4; n != 0; n--, p++) {
        if (p->a >= 0) {
            unsigned t = g_items[p->a].type & 0xff;
            if (t == 5 || t == 6 || t == 0x12 || t == 10) {
                unsigned cur = p->b & 0xff;
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
