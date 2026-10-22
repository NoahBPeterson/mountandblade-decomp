// Slice s0048a420: Mission methods for replay-frame interpolation (two recorded frames + easing), the
// per-team count table queries, the combatant slot flags (three 0x6c-byte sub-blocks of a 0x16c record),
// origin-table checks, a 10007-slot hash probe, and two generic 8-byte-element helpers (fill / move-backward).
// Class offsets come from the mission layout verified in s004759c0 (counts at 0x621c) and s00471b30.
#include <windows.h>
#include <math.h>
#include <string.h>

extern __int64 g_ticks[];       // 0x5e2948: 64-bit tick counters (100 kHz units)

// ---- rgl matrix (layout from rgl.h; only members used here) ----------------------------------------------
struct vector4 {
    float x, y, z, w;
    vector4() {}
};
struct matrix {
    vector4 s, f, u, o;
    void orthonormalize();                                      // 0x407ef0
};

// ---- combatant record: 0x16c bytes at Mission+0x527c -----------------------------------------------------
struct Blk {                                    // 0x6c bytes; three of them at +0/+0x6c/+0xd8
    char pad0[8];
    int k8;                                     // +0x08 kind (switch in active_kind)
    int kc;                                     // +0x0c kind (slot_mode)
    int k10;                                    // +0x10 kind (slot_kind_a)
    int k14;                                    // +0x14 kind (slot_kind_b)
    char pad18[0x4c - 0x18];
    int use;                                    // +0x4c slot in use
    int m4;                                     // +0x50
    char pad54[0x6c - 0x54];
};
struct Rec {
    Blk b[3];
    char pad144[0x16c - 0x144];
};

// ---- 8-byte map entry / vector element --------------------------------------------------------------------
struct MapEntry { unsigned key; int value; };
struct Map10007 { MapEntry tab[0x2717]; int lookup(unsigned id); };
struct E68 { char b[0x68]; };
struct Vec68 { char pad4[4]; E68* begin; E68* end; int count(); };

struct Mission {
    struct Pair { int a, b; } slots[1];          // +0x0 combatant-origin id pairs (indexed)
    char pad8[0x527c - 8];
    Rec recs[1];                                 // +0x527c combatant records (0x16c each)
    char pad53e8[0x621c - 0x53e8];
    int counts[8][6];                            // +0x621c per-team counters (verified in s004759c0)
    char pad62dc[0x6328 - 0x62dc];
    int clock_index;                             // +0x6328
    __int64 clock_start;                         // +0x6330
    char clock_set;                              // +0x6338
    char pad6339[0x653c - 0x6339];
    float rect_xmin;                             // +0x653c
    float rect_ymin;                             // +0x6540
    float rect_xmax;                             // +0x6544
    float rect_ymax;                             // +0x6548
    char pad654c[0x674c - 0x654c];
    matrix frame_a;                              // +0x674c
    matrix frame_b;                              // +0x678c
    float blend_from;                            // +0x67cc
    float blend_to;                              // +0x67d0
    float blend_time;                            // +0x67d4
    int origin;                                  // +0x67d8
    char pad67dc[0x67e0 - 0x67dc];
    __int64 start;                               // +0x67e0
    char pad67e8[0x67f0 - 0x67e8];
    int blend_n;                                 // +0x67f0
    float blend2_from;                           // +0x67f4
    float blend2_to;                             // +0x67f8
    char pad67fc[0x6800 - 0x67fc];
    int origin2;                                 // +0x6800
    __int64 start2;                              // +0x6808
    float blend2_time;                           // +0x6810

    Rec* rec(int i) { return (Rec*)((char*)this + 0x527c + i * 0x16c); }

    void get_frame(matrix* out);                 // 0x48a420
    float blend_param();                         // 0x48a740
    float blend2_param();                        // 0x48a7d0
    void start_clock(int v);                     // 0x48a860
    int team_count();                            // 0x48a8b0
    int last_team();                             // 0x48a910
    bool point_in_rect(const float* p);          // 0x48a990
    void slot_finish(int i);                     // 0x48a9e0
    void slot_restore(int i);                    // 0x48a9e0 counterpart
    void slot_set(int i, int s);                 // 0x48aa60
    int active_kind(int i);                      // 0x48aac0
    int slot_mode(int i);                        // 0x48ab30
    int slot_kind_a(int i);                      // 0x48ab90
    int slot_kind_b(int i);                      // 0x48abf0
    bool has_origin(int i);                      // 0x48ac50
    bool has_free_origin(int i);                 // 0x48ac90
};

extern void* g_origin_table[];                  // 0xa170d4: object per origin id
extern int g_origin_state[];                    // 0xa174d4: 0 = origin idle/free

// @ 0x48a420
void Mission::get_frame(matrix* out)
{
    if (blend_time > 0.0f) {
        float t = (g_ticks[origin] - start) * 1e-5f;
        t = t / blend_time;
        if (blend_n > 0) {
            t = atan((t - 0.5f) * (float)blend_n * 3.1415927f) /
                    (atan((float)blend_n * 1.5707964f) * 2.0f) + 0.5f;
        }
        if (t >= 1.0f) {
            *out = frame_b;
            return;
        }
        float s = 1.0f - t;
        matrix m;
        vector4 v;
        // The original spills each product to a float temporary before adding; this source
        // cannot force the same intermediate rounding, so a few signed-zero results differ.
        v.x = frame_b.o.x * t + frame_a.o.x * s;
        v.y = frame_b.o.y * t + frame_a.o.y * s;
        v.z = frame_b.o.z * t + frame_a.o.z * s;
        m.o = v;
        v.x = frame_b.f.x * t + frame_a.f.x * s;
        v.y = frame_b.f.y * t + frame_a.f.y * s;
        v.z = frame_b.f.z * t + frame_a.f.z * s;
        m.f = v;
        v.x = frame_b.u.x * t + frame_a.u.x * s;
        v.y = frame_b.u.y * t + frame_a.u.y * s;
        v.z = frame_b.u.z * t + frame_a.u.z * s;
        m.u = v;
        v.x = frame_b.s.x * t + frame_a.s.x * s;
        v.y = frame_b.s.y * t + frame_a.s.y * s;
        v.z = frame_b.s.z * t + frame_a.s.z * s;
        m.s = v;
        m.orthonormalize();
        *out = m;
        return;
    }
    *out = frame_a;
}

// @ 0x48a740
float Mission::blend_param()
{
    if (blend_time > 0.0f) {
        float t = (g_ticks[origin] - start) * 1e-5f;
        t = t / blend_time;
        if (t >= 1.0f)
            return blend_to;
        return (1.0f - t) * blend_from + t * blend_to;
    }
    return blend_from;
}

// @ 0x48a7d0
float Mission::blend2_param()
{
    if (blend2_time > 0.0f) {
        float t = (g_ticks[origin2] - start2) * 1e-5f;
        t = t / blend2_time;
        if (t >= 1.0f)
            return blend2_to;
        return (1.0f - t) * blend2_from + t * blend2_to;
    }
    return blend2_from;
}

// @ 0x48a860
void Mission::start_clock(int v)
{
    clock_set = 1;
    int d = (int)((float)-v * 100000.0f);
    clock_start = g_ticks[clock_index] - d;
}

// @ 0x48a8b0
int Mission::team_count()
{
    int n = 0;
    if (0 < counts[0][1]) n = 1;
    if (0 < counts[1][1]) n++;
    if (0 < counts[2][1]) n++;
    if (0 < counts[3][1]) n++;
    if (0 < counts[4][1]) n++;
    if (0 < counts[5][1]) n++;
    if (0 < counts[6][1]) n++;
    if (0 < counts[7][1]) n++;
    return n;
}

// @ 0x48a910
int Mission::last_team()
{
    int r = -1;
    if (0 < counts[0][1]) r = 0;
    if (0 < counts[1][1]) r = 1;
    if (0 < counts[2][1]) r = 2;
    if (0 < counts[3][1]) r = 3;
    if (0 < counts[4][1]) r = 4;
    if (0 < counts[5][1]) r = 5;
    if (0 < counts[6][1]) r = 6;
    if (0 < counts[7][1]) r = 7;
    return r;
}

// @ 0x48a990
bool Mission::point_in_rect(const float* p)
{
    return rect_xmin <= p[0] && p[0] < rect_xmax &&
           rect_ymin <= p[1] && p[1] < rect_ymax;
}

// @ 0x48a9e0
void Mission::slot_finish(int i)
{
    Rec* r = rec(i);
    r->b[0].m4 = r->b[0].use;
    r->b[1].m4 = r->b[1].use;
    r->b[2].m4 = r->b[2].use;
}

// @ 0x48aa20
void Mission::slot_restore(int i)
{
    Rec* r = rec(i);
    r->b[0].use = r->b[0].m4;
    r->b[1].use = r->b[1].m4;
    r->b[2].use = r->b[2].m4;
}

// @ 0x48aa60
void Mission::slot_set(int i, int s)
{
    Rec* r = rec(i);
    if (s >= 0 && s < 3) {
        r->b[0].use = 0;
        r->b[1].use = 0;
        r->b[2].use = 0;
        r->b[s].use = 1;
    } else {
        r->b[0].use = 1;
        r->b[1].use = 1;
        r->b[2].use = 1;
    }
}

// @ 0x48aac0
int Mission::active_kind(int i)
{
    Rec* r = rec(i);
    for (int k = 0; k < 3; k++) {
        if (r->b[k].use != 0) {
            switch (r->b[k].k8) {
            case 0: return 2;
            case 2: return 1;
            case 1: return 0;
            case 3: return 11;
            default: return -1;
            }
        }
    }
    return -1;
}

// @ 0x48ab30
int Mission::slot_mode(int i)
{
    Rec* r = rec(i);
    int v = 0;
    while (v < 3) {
        if (r->b[v].use != 0) {
            v = r->b[v].kc;
            goto sw;
        }
        v++;
    }
    v = -1;
sw:
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 2;
    }
    return -1;
}

// @ 0x48ab90
int Mission::slot_kind_a(int i)
{
    int v = -1;
    for (int k = 0; k < 3; k++) {
        if (recs[i].b[k].use) {
            v = recs[i].b[k].k10;
            break;
        }
    }
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    }
    return -1;
}

// @ 0x48abf0
int Mission::slot_kind_b(int i)
{
    int v = -1;
    for (int k = 0; k < 3; k++) {
        if (recs[i].b[k].use) {
            v = recs[i].b[k].k14;
            break;
        }
    }
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    }
    return -1;
}

// @ 0x48ac50
bool Mission::has_origin(int i)
{
    bool r = false;
    int a = slots[i].a;
    if (a >= 0 && g_origin_table[a] != 0)
        r = true;
    int b = slots[i].b;
    if (b >= 0 && g_origin_table[b] != 0)
        r = true;
    return r;
}

// @ 0x48ac90
bool Mission::has_free_origin(int i)
{
    bool r = false;
    int a = slots[i].a;
    if (a >= 0 && g_origin_table[a] != 0 && g_origin_state[a] == 0)
        r = true;
    int b = slots[i].b;
    if (b >= 0 && g_origin_table[b] != 0 && g_origin_state[b] == 0)
        r = true;
    return r;
}

// @ 0x48ace0
int Vec68::count()
{
    if (!begin)
        return 0;
    return end - begin;
}

// @ 0x48ad00
int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}

// @ 0x48ad70
void fill8(int* first, int* last, const int* val)
{
    for (; first != last; first += 2) {
        first[0] = val[0];
        first[1] = val[1];
    }
}

// @ 0x48ada0
void move_backward8(__int64* first, __int64* last, __int64* dest)
{
    int n = (int)(last - first);
    memmove(dest - n, first, n * 8);
}
