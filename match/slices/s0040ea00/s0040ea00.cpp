// Slice s0040ea00: renderer texture-coordinate generation setter, mesh-instance helpers (flag decode, link chain
// accessors), a keyframe-table lookup, the particle emitter (spawn one particle) and std::vector<T> size()/
// capacity()/copy/fill/copy_backward instantiations for the particle-system record types.
// Type names are provisional. Flags: default /O2 /MT /Gy /TP (no EH switch needed).
#include <windows.h>
#include <d3d9.h>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

struct elem96  { int a[24]; };
struct elem48  { int a[12]; };
struct elem32  { int a[8]; };
struct elem20  { int a[5]; };
struct elem8   { int a, b; };
struct particle { int a[25]; };      // 100 bytes

// ---- renderer (global at 0xa17910) ----------------------------------------------------------------
struct renderer {
    char pad000[0x574];
    int stage_xform[3];              // 0x574
    int stage_texgen[3];             // 0x580  per-stage TEXCOORDINDEX mode cache
    int blend_mode;                  // 0x58c
    char pad590[0x618 - 0x590];
    IDirect3DDevice9* dev;           // 0x618
    void set_texcoord_gen(int stage, int mode);     // 0x40ea00
};

// @ 0x40ea00
void renderer::set_texcoord_gen(int stage, int mode)
{
    if (stage_texgen[stage] != mode) {
        if (mode <= 1) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, mode);
        } else if (mode == 0x10) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_SPHEREMAP);
            stage_texgen[stage] = mode;
            return;
        } else if (mode == 0x20) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);
        } else if (mode == 0x30) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
            stage_texgen[stage] = mode;
            return;
        } else if (mode == 0x40) {
            dev->SetTextureStageState(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
        }
    }
    stage_texgen[stage] = mode;
}

// ---- mesh instance helpers ------------------------------------------------------------------------
struct mesh_info {
    char pad00[0x20];
    unsigned flags;                  // 0x20  bits 29..31: shader/LOD class
    int shader_class();              // 0x40eab0
};
// @ 0x40eab0
int mesh_info::shader_class()
{
    unsigned v = flags;
    int r = 0;
    if (v & 0x20000000) {
        r = 1;
        if (v & 0x40000000)
            r = v & 0x80000000 ? 4 : 2;
        else if (v & 0x80000000)
            r = 3;
    }
    return r;
}

// 0x40eaf0: three zeroed vector4 rows and a zero dword (a 3x3 + one zero value)
struct rot_state {
    vector4 s, f, u;
    int extra;                       // 0x30
    rot_state();
};
// @ 0x40eaf0
rot_state::rot_state() : extra(0)
{
    s = vector4(0.0f, 0.0f, 0.0f);
    f = vector4(0.0f, 0.0f, 0.0f);
    u = vector4(0.0f, 0.0f, 0.0f);
}

// ---- link-chain accessors: follow `master` until a node whose flag byte is set --------------------
struct rnode {
    char pad00[0x18];
    rnode* master;                   // 0x18
    char pad1c[0x50 - 0x1c];
    char f50[0x10];                  // 0x50
    char f60[0x10];                  // 0x60
    char f70[0x20];                  // 0x70  std::vector<elem32>
    char f90[0x10];                  // 0x90
    char flag_a0;                    // 0xa0
    char flag_a1;                    // 0xa1
    char pada2[2];
    char fa4[4];                     // 0xa4
    void* get70();                   // 0x40eba0
    void* get50();                   // 0x40ebd0
    void* get_a4();                  // 0x40ec00
    void* get90();                   // 0x40ec30
    void* get60();                   // 0x40ec60
};
// @ 0x40eba0
void* rnode::get70() { rnode* n = this; while (!n->flag_a1) n = n->master; return n->f70; }
// @ 0x40ebd0
void* rnode::get50() { rnode* n = this; while (!n->flag_a1) n = n->master; return n->f50; }
// @ 0x40ec00
void* rnode::get_a4() { rnode* n = this; while (!n->flag_a1) n = n->master; return n->fa4; }
// @ 0x40ec30
void* rnode::get90() { rnode* n = this; while (!n->flag_a0) n = n->master; return n->f90; }
// @ 0x40ec60
void* rnode::get60() { rnode* n = this; while (!n->flag_a1) n = n->master; return n->f60; }

// ---- particle emitter: spawn one particle ---------------------------------------------------------
// Module System particle flags (header_particle_systems.py)
enum {
    psf_always_emit         = 0x0000000002,
    psf_global_emit_dir     = 0x0000000010,
    psf_emit_at_water_level = 0x0000000020,
    psf_billboard_2d        = 0x0000000100,
    psf_billboard_3d        = 0x0000000200,
    psf_turn_to_velocity    = 0x0000000400,
    psf_randomize_rotation  = 0x0000001000,
    psf_randomize_size      = 0x0000002000,
};
#define RND(k) ((float)(rand() % 15817) * ((k) / 15817.0f))

struct frame3 {                      // the three rotation rows of a matrix
    vector4 s, f, u;
    void rotate_fu(const float* angle);     // 0x40e570
    void rotate_su(const float* angle);     // 0x40e670
    void rotate_sf(const float* angle);     // 0x40e770
};
struct pos_ext : rgl::matrix {
    vector4 transform_point(const vector4& p) const;    // 0x408460 (s00408110)
    vector4 rot(const vector4& p) const;                // 0x40a9c0 (s0040a640)
};
struct particle_rec {                // 100 bytes
    frame3 frame;                    // 0x00
    vector4 pos;                     // 0x30
    vector4 vel;                     // 0x40
    float f50, f54, f58, f5c, f60;   // 0x50  (f58: spin angle, f5c: spin rate, f60: size scale)
};
static __forceinline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 operator+(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
struct emitter {
    char pad00[0xe8];
    unsigned flags;                  // 0xe8  psf_*
    float water_z;                   // 0xec
    int disabled;                    // 0xf0
    pos_ext frame;                   // 0xf4
    char pad134[0x174 - 0x134];
    float box[3];                    // 0x174 emission box half-extents
    char pad180[4];
    vector4 vel_base;                // 0x184 (w unused)
    float vel_scale;                 // 0x194
    char pad198[0x1b0 - 0x198];
    float spin_scale;                // 0x1b0
    void spawn(particle_rec* out);   // 0x40ec90
};
// @ 0x40ec90
void emitter::spawn(particle_rec* out)
{
    if (disabled != 0)
        return;
    vector4 r;
    r.x = (RND(1.0f) - 0.5f) * 2.0f;
    r.y = (RND(1.0f) - 0.5f) * 2.0f;
    r.z = (RND(1.0f) - 0.5f) * 2.0f;
    {
        vector4 local;
        local.x = r.x * box[0];
        local.y = r.y * box[1];
        local.z = r.z * box[2];
        out->frame = *(const frame3*)&frame;
        out->pos = frame.transform_point(local);
    }
    if (flags & psf_emit_at_water_level)
        out->pos.z = water_z;
    r = vmul(r, vel_scale) + vel_base;
    vector4* vel = &out->vel;
    *vel = r;
    if (!(flags & psf_global_emit_dir))
        *vel = frame.rot(*vel);
    float m = RND(1.0f) + RND(1.0f);
    vel->x = m * vel->x;
    vel->y = m * vel->y;
    vel->z = m * vel->z;
    out->f50 = 0.0f;
    out->f58 = 0.0f;
    if (flags & psf_randomize_rotation) {
        if (!(flags & 0xf00)) {
            float a = RND(6.2831853f);
            out->frame.rotate_fu(&a);
            a = RND(6.2831853f);
            out->frame.rotate_su(&a);
            a = RND(6.2831853f);
            out->frame.rotate_sf(&a);
        }
        out->f58 = RND(6.2831853f);
    }
    out->f5c = (RND(2.0f) - 1.0f) * spin_scale;
    out->f60 = 1.0f;
    if (flags & psf_randomize_size) {
        if (RND(1.0f) > 0.5f)
            out->f60 = (1.0f - RND(0.4f)) * out->f60;
        else
            out->f60 = (RND(0.6f) + 1.0f) * out->f60;
    }
}

// ---- keyframe table lookup --------------------------------------------------------------------------
struct key_table {
    int pad0;
    int* keys;                       // 0x04 ascending integer times
    int count;                       // 0x08
    // Find the segment [*lo, *hi] of `keys` that contains *t and the fraction of the way through it.
    void find(int* lo, int* hi, float* frac, float* t);     // 0x40f0d0
};
// @ 0x40f0d0
void key_table::find(int* lo, int* hi, float* frac, float* t)
{
    int n = count;
    if (n <= 0) {
        *lo = -1;
        *hi = -1;
        return;
    }
    int l = 0;
    int h = n - 1;
    if (h - 1 > 0) {
        do {
            int mid = (l + h) >> 1;
            if ((float)keys[mid] > *t)
                h = mid;
            else
                l = mid;
        } while (l < h - 1);
    }
    *lo = l;
    *hi = h;
    int k0 = keys[*lo];
    *frac = (*t - (float)k0) / (keys[h] - k0);
}

// ---- std::vector<T>::size() / capacity() (first = +4, last = +8, end = +0xc) -------------------------
#define VEC_CLASS(name, T) \
    struct name { int al; T *first, *last, *end; unsigned size() const; unsigned capacity() const; }
VEC_CLASS(vec_int, int);
VEC_CLASS(vec96, elem96);
VEC_CLASS(vec48, elem48);
VEC_CLASS(vec32, elem32);
VEC_CLASS(vec20, elem20);
VEC_CLASS(vec100, particle);
#define VEC_SIZE(name) unsigned name::size() const { return first == 0 ? 0 : last - first; }
#define VEC_CAP(name) unsigned name::capacity() const { return first == 0 ? 0 : end - first; }
// @ 0x40f010
VEC_SIZE(vec_int)
// @ 0x40f030
VEC_CAP(vec96)
// @ 0x40f050
VEC_SIZE(vec96)
// @ 0x40f070
VEC_SIZE(vec48)
// @ 0x40f090
VEC_CAP(vec32)
// @ 0x40f0b0
VEC_SIZE(vec20)
// @ 0x40f160
VEC_SIZE(vec100)

// ---- std::copy / std::fill / std::copy_backward on pointer ranges -----------------------------------
// @ 0x40f180
elem96* __cdecl copy96(elem96* f, elem96* l, elem96* d) { for (; f != l; ++d, ++f) *d = *f; return d; }
// @ 0x40f1b0
elem48* __cdecl copy48(elem48* f, elem48* l, elem48* d) { for (; f != l; ++d, ++f) *d = *f; return d; }
// @ 0x40f1e0
elem32* __cdecl copy32(elem32* f, elem32* l, elem32* d) { for (; f != l; ++d, ++f) *d = *f; return d; }
// @ 0x40f210
elem8* __cdecl copy8(elem8* f, elem8* l, elem8* d) { for (; f != l; ++d, ++f) *d = *f; return d; }
// @ 0x40f240
void __cdecl fill96(elem96* f, elem96* l, const elem96& v) { for (; f != l; ++f) *f = v; }
// @ 0x40f270
elem96* __cdecl copy_backward96(elem96* f, elem96* l, elem96* d) { while (f != l) *--d = *--l; return d; }
// @ 0x40f2a0
void __cdecl fill48(elem48* f, elem48* l, const elem48& v) { for (; f != l; ++f) *f = v; }
// @ 0x40f2d0
elem48* __cdecl copy_backward48(elem48* f, elem48* l, elem48* d) { while (f != l) *--d = *--l; return d; }
