// Slice s00432130: small game-logic helpers around 0x432130-0x4329d0: key/control-id group tables,
// agent team-relation tests, scalar min/max/clamp/round/rand helpers, 2D rotate, 3x3 frame algebra.
// All class names are provisional.
#include <math.h>
#include <stdlib.h>
#pragma intrinsic(sin, cos, fabs)
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- control-id groups (ids 0x97.. and 0xc7..0xed come in triples; provisional names). The ids are in the
// engine's key-code space (Module System header_triggers: key_home 0xc7, key_up 0xc8, key_page_up 0xc9, key_left 0xcb,
// key_right 0xcd, key_end 0xcf, key_down 0xd0, key_page_down 0xd1, key_insert 0xd2, key_delete 0xd3, mouse buttons 0xe0..0xe7) -------------------
// @ 0x432130
int __cdecl group_base(int id)
{
    int r = -1;
    if (id == 0xc7 || id == 0xc8 || id == 0xc9) {
        r = 0xc7;
    } else {
        if (id == 0x97 || id == 0x98 || id == 0x99) return 0x97;
        if (id == 0xca || id == 0xcb || id == 0xcc) return 0xca;
        if (id == 0xcd || id == 0xce || id == 0xcf) return 0xcd;
        if (id == 0xd0 || id == 0xd1 || id == 0xd2) return 0xd0;
        if (id == 0xd3 || id == 0xd4 || id == 0xd5) return 0xd3;
        if (id == 0xd6 || id == 0xd7 || id == 0xd8) return 0xd6;
        if (id == 0xd9 || id == 0xda || id == 0xdb) return 0xd9;
        if (id == 0xdc || id == 0xdd || id == 0xde) return 0xdc;
        if (id == 0xdf || id == 0xe0 || id == 0xe1) return 0xdf;
        if (id == 0xe2 || id == 0xe3 || id == 0xe4) return 0xe2;
        if (id == 0xe5 || id == 0xe6 || id == 0xe7) return 0xe5;
        if (id == 0xe8 || id == 0xe9 || id == 0xea) return 0xe8;
        if (id == 0xeb || id == 0xec || id == 0xed) return 0xeb;
    }
    return r;
}

// @ 0x432640
int __stdcall in_id_set(int id)
{
    int r = 0;
    if (id == 0x44 || id == 0xa2 || id == 0xc0 || id == 0xa8 || id == 0xab || id == 0x9c || id == 0xc6 ||
        id == 0xae || id == 0x9f || id == 0xc3 || id == 0xb1 || id == 0xa5 || id == 0xbd || id == 0xb4 ||
        id == 0xba || id == 0xb7)
        r = 1;
    return r;
}

// The same function, declared (not visible as a definition) so cl does not assume its register use.
int __cdecl group_base_ext(int id);   // 0x432130
// @ 0x4326d0
int __stdcall id_matches_dir(int id, int dir)
{
    int res = 0;
    int g = group_base_ext(id);
    if (g == 0xc7) {
        res = 1;
    } else if (g == 0xd9 || g == 0xcd || g == 0xe5) {
        if (dir == 3) res = 1;
    } else if (g == 0xdf || g == 0xd3 || g == 0xeb) {
        if (dir == 1) res = 1;
    } else if (g == 0xdc || g == 0xd0 || g == 0xe8) {
        if (dir == 2) res = 1;
    } else if (g == 0xd6 || g == 0xca || g == 0xe2) {
        if (dir == 0) res = 1;
    }
    return res;
}

// ---- team relations ---------------------------------------------------------------------------------
// The mission object at 0x5e292c keeps an 8x? relation matrix (stride 0x5b ints) at +0x53c8: -1 enemy, 1 friend.
struct Mission { char pad[0x53c8]; int relation[1]; };
extern Mission* g_mission;     // 0x5e292c

struct Pair { int a, b; };
struct Z_ { char pad[0xd8]; float fd8; };
struct Y_ { char pad[0x40]; Z_* f40; };
struct X_ { char pad[0x304]; Y_* f304; };

struct Agent {
    char pad0[0x10];
    int f10;
    char pad14[0x2c - 0x14];
    int f2c;
    char pad30[0x100 - 0x30];
    int f100;                  // +0x100 (0x48 = a particular action state)
    char pad104[0x1c4 - 0x104];
    int team;                  // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    Pair pairs[11];            // +0x1d0
    int pad228;
    int slots[5];              // +0x22c (slots[4] at +0x23c)
    char pad240[0x250 - 0x240];
    X_* f250;                  // +0x250
    bool is_enemy(const Agent* o) const;      // 0x432350
    __declspec(noinline) bool is_friend(const Agent* o) const;     // 0x4323a0
    int is_neutral(const Agent* o) const;    // 0x4323f0
    float speed_scale() const;                // 0x432450
    bool f100_is_0x48_and_charged() const;    // 0x432470
    Pair slot_pair(int idx) const;            // 0x432790 (returns {-1, <uninit>} when empty)
};

// @ 0x432350
bool Agent::is_enemy(const Agent* o) const
{
    if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
        return g_mission->relation[team * 0x5b + o->team] == -1;
    return false;
}
// @ 0x4323a0
bool Agent::is_friend(const Agent* o) const
{
    if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
        return g_mission->relation[team * 0x5b + o->team] == 1;
    return false;
}
// @ 0x4323f0
int Agent::is_neutral(const Agent* o) const
{
    if (!is_enemy(o) && !is_friend(o)) return 1;
    return 0;
}
// @ 0x432450
float Agent::speed_scale() const
{
    if (f10 == 0 && f2c != -1) return 2.4f;
    return 1.5f;
}
// @ 0x432470
bool Agent::f100_is_0x48_and_charged() const
{
    if (f100 != 0x48) return false;
    return f250->f304->f40->fd8 > 0.1f;
}

// ---- scalar helpers -------------------------------------------------------------------------------
// @ 0x4324b0
float __cdecl fmin_ref(const float* a, const float* b) { return *a < *b ? *a : *b; }
// @ 0x4324d0
float __cdecl fmax_ref(const float* a, const float* b) { return *a > *b ? *a : *b; }
// @ 0x4324f0
int __cdecl imax_ref(const int* a, const int* b) { return *a > *b ? *a : *b; }
// @ 0x432510
int __cdecl clamp_index(const int* v, const int* lo, const int* hi)
{
    if (*v < *lo) return *lo;
    if (*v >= *hi) return *hi - 1;
    return *v;
}
// @ 0x432540
int __cdecl round_float(const float* p)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}
// @ 0x432580
int __cdecl rand_below(const int* n)
{
    if (*n <= 1) return 0;
    return rand() % *n;
}
// @ 0x4325a0
float __cdecl closer_angle(const float* ref, const float* a, const float* b)
{
    float da = (float)fabs(*a - *ref);
    if (da > 3.1415927f) da = 6.2831855f - da;
    float db = (float)fabs(*b - *ref);
    if (db > 3.1415927f) db = 6.2831855f - db;
    if (db > da) return *a;
    return *b;
}

// ---- 2D / 3D vectors --------------------------------------------------------------------------------
struct vec2 { float x, y; void rotate_a(float ang); void rotate_b(float ang); };
// @ 0x432600
void vec2::rotate_a(float ang)
{
    float c = (float)cos((double)ang);
    float s = (float)sin((double)ang);
    float nx = c * x - s * y;
    float ny = s * x + c * y;
    y = ny;
    x = nx;
}
// @ 0x4327f0
void vec2::rotate_b(float ang)
{
    float c = (float)cos((double)ang);
    float s = (float)sin((double)ang);
    float nx = c * x - s * y;
    float t = c * y;
    y = t + s * x;
    x = nx;
}

struct vec3 { float x, y, z; void sub_assign(const vec3& o); };
// @ 0x432770
void vec3::sub_assign(const vec3& o) { x = x - o.x; y = y - o.y; z = z - o.z; }
// @ 0x432830
void __cdecl vec3_sub(vec3* out, const vec3* a, const vec3* b) { out->x = a->x - b->x; out->y = a->y - b->y; out->z = a->z - b->z; }

// @ 0x432790
Pair Agent::slot_pair(int idx) const
{
    const Pair* p = 0;
    int i = slots[idx];
    if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
    Pair r;
    r.a = -1;
    if (p) r = *p;
    return r;
}

// ---- frame algebra ----------------------------------------------------------------------------------
// 3 rows of vector4 (the 4th lane is never written: whole-struct copies carry stack garbage along).
struct mat3 {
    vector4 s, f, u;
    __declspec(noinline) mat3 mul_t(const mat3& b) const;           // 0x432860: result row i, col j = dot(b.row i, this->row j)
};

// @ 0x432860
mat3 mat3::mul_t(const mat3& b) const
{
    mat3 r;
    vector4 a, c, d;
    a.x = (b.s.x*this->s.x + b.s.y*this->s.y) + b.s.z*this->s.z;
    a.y = (b.s.y*this->f.y + b.s.z*this->f.z) + b.s.x*this->f.x;
    a.z = (b.s.x*this->u.x + b.s.z*this->u.z) + b.s.y*this->u.y;
    r.s = a;
    c.x = (b.f.x*this->s.x + b.f.y*this->s.y) + b.f.z*this->s.z;
    c.y = (b.f.x*this->f.x + b.f.z*this->f.z) + b.f.y*this->f.y;
    c.z = (b.f.x*this->u.x + b.f.z*this->u.z) + this->u.y*b.f.y;
    r.f = c;
    d.x = (b.u.y*this->s.y + b.u.z*this->s.z) + b.u.x*this->s.x;
    d.y = (b.u.x*this->f.x + b.u.z*this->f.z) + b.u.y*this->f.y;
    d.z = (b.u.x*this->u.x + b.u.z*this->u.z) + b.u.y*this->u.y;
    r.u = d;
    return r;
}

// @ 0x4329d0
struct pos : matrix {
    matrix relative(const matrix& b) const;    // b expressed in this frame
};
matrix pos::relative(const matrix& b) const
{
    matrix m;
    m.o = transform_point_to_local(b.o);
    *(mat3*)&m = ((const mat3*)this)->mul_t(*(const mat3*)&b);
    return m;
}
