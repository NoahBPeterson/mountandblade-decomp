// Slice s004684d0: mesh-part render-state setup (0x4684d0), mesh element counts (0x468620), a 0xd0-byte state
// object's reset (0x4686a0), the troop inventory's auto-equip (0x4687a0 / 0x4687e0, built on Inv::add and
// Inv::equip_best from s00462e20), an AABB extension helper (0x468c20) and an occlusion test (0x468c80).
// ALL NAMES PROVISIONAL. Flags: default /O2 /MT /Gy /TP.
#include <windows.h>
#include <d3d9.h>
#include <math.h>
#include "rgl.h"
#include "mesh.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mesh;
using rgl::face20;
using rgl::vertex60;

// ---- globals ----------------------------------------------------------------------------------------
extern __int64 g_ticks[];                       // 0x5e2948: 64-bit clocks, 100 kHz units
extern IDirect3DDevice9* g_dev;                 // 0xa17f28
extern unsigned char g_rs_normalize;            // 0xa17e58 cached D3DRS_NORMALIZENORMALS
extern unsigned char g_rs_zwrite_off;           // 0xa17e59 cached "depth write disabled" bit
extern unsigned char g_rs_specular;             // 0xa17e57 cached D3DRS_SPECULARENABLE
extern unsigned char g_rs_zfunc_le;             // 0xa17e5b cached "depth test on" bit
extern int g_use_shaders;                       // 0x5e28fc
extern int g_lod_max;                           // 0x8ee3ac4
extern unsigned g_field_mask;                   // 0x5a8ea0

struct renderer {
    char pad[1];
    void set_stage_colorop(int stage, int v);    // 0x461bd0  SetTextureStageState(D3DTSS_COLOROP, v & 0x1f), cached at +0x55c
    void set_stage_alphaop(int stage, int v);    // 0x461c60  SetTextureStageState(D3DTSS_ALPHAOP, v & 0x1f), cached at +0x568
    void set_alpha_test_mode(int mode);          // 0x461cf0  ALPHATESTENABLE + ALPHAREF 8/0x88/0xfb, cached at +0x590
    void set_sampler_clamp(int stage, int v);    // 0x461d90  SetSamplerState ADDRESSU/V wrap or clamp (bits 0, 1), cached at +0x59c
    void set_material_specular(float power, const float* color);     // 0x461e10  SetMaterial with white diffuse/ambient and this specular
};
extern renderer g_render;                // 0xa17910

// ---- 0x4684d0: apply a mesh part's render states ------------------------------------------------------
struct Texture;
struct material {
    char pad0[0x20];
    unsigned flags;             // +0x20  bit 5: specular
    char pad24[0xb8 - 0x24];
    void bind(Texture** textures, const float* t);     // 0x468440 (s00467bf0)
};
struct mesh_part {
    char pad0[0x1c];
    unsigned flags;             // +0x1c  bit 3: no depth write, bit 4: depth test off, bits 11, 12..13
    char pad20[0x30 - 0x20];
    material* mat;              // +0x30
    float spec[3];              // +0x34
    char pad40[4];
    float spec_power;           // +0x44
    Texture* tex[10];           // +0x48
    void apply_state(const float* t);        // 0x4684d0
};
// @ 0x4684d0
void mesh_part::apply_state(const float* t)
{
    unsigned char a = ~(flags >> 4) & 1;
    if (a != g_rs_zfunc_le) {
        g_rs_zfunc_le = a;
        if (a)
            g_dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        else
            g_dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    }
    unsigned char b = (flags >> 3) & 1;
    if (b != g_rs_zwrite_off) {
        BOOL on = b == 0;
        g_rs_zwrite_off = b;
        g_dev->SetRenderState(D3DRS_ZWRITEENABLE, on);
    }
    g_render.set_alpha_test_mode((flags >> 12) & 3);
    unsigned char c = (flags >> 11) & 1;
    if (c != g_rs_normalize) {
        g_dev->SetRenderState(D3DRS_NORMALIZENORMALS, c);
        g_rs_normalize = c;
    }
    unsigned char d = (mat->flags >> 5) & 1;
    if (d != g_rs_specular) {
        g_dev->SetRenderState(D3DRS_SPECULARENABLE, d);
        g_rs_specular = d;
    }
    if (mat->flags & 0x20) {
        vector4 col;
        col.x = spec[0] * 0.5f;
        col.y = spec[1] * 0.5f;
        col.z = spec[2] * 0.5f;
        g_render.set_material_specular(spec_power + spec_power, &col.x);
    }
    mat->bind(tex, t);
}

// ---- 0x468620: element count of a mesh --------------------------------------------------------------------
// @ 0x468620
int rgl::mesh::count(int kind)
{
    if (kind == 1) {
        std::vector<face20>& f = local_a0 ? v90 : parent->get90_();
        return f.size() * 3;
    }
    std::vector<vertex60>& v = local_a1 ? v50 : parent->get50_();
    return v.size();
}

// ---- 0x4686a0: reset of a state object ------------------------------------------------------------------------
struct Quad4 {                  // four floats at Fx+0xc0 (zeroed member-wise, then x = 1)
    float x, y, z, w;
    void zero() { x = 0; z = 0; y = 0; w = 0; }
};
struct Fx {
    int f0;
    char pad4[4];
    int f8, fc;
    char pad10[0x10];
    int clock;                  // +0x20
    char pad24[4];
    __int64 t0;                 // +0x28
    int f30;
    float f34, f38;
    int f3c;
    float f40;
    char pad44[8];
    int f4c, f50, f54, f58, f5c, f60, f64, f68;
    char pad6c[0x9c - 0x6c];
    int f9c;
    float fa0;
    int fa4, fa8, fac;
    char fb0;
    char padb1[3];
    int fb4, fb8;
    float fbc;
    Quad4 q;                    // +0xc0
    void reset();               // 0x4686a0
};
// @ 0x4686a0
void Fx::reset()
{
    f3c = -1;
    f8 = -1;
    fb4 = 1;
    f0 = 0;
    f30 = 0;
    fa4 = 10;
    f9c = 13;
    fa8 = 20;
    fb8 = 0;
    f34 = 1.3f;
    f38 = 4.3f;
    f4c = 0;
    f60 = 0;
    f64 = 0;
    f54 = 0;
    f58 = 0;
    f40 = 1.0f;
    fbc = 1.0f;
    fa0 = 1.0f;
    fc = -1;
    f50 = 1;
    t0 = g_ticks[clock];
    fac = 0;
    fb0 = 0;
    q.zero();
    f5c = 0;
    q.x = 1.0f;
    f68 = 0;
}

// ---- Inv (troop inventory; see s00462e20) -------------------------------------------------------------------
extern struct ItemKind* g_items;                // 0x5e29d0
struct ItemKind {                               // 0x238 bytes, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned type_flags;                        // +0x1c8 (low byte = item type, itp_*)
    char pad1cc[0x238 - 0x1cc];
};
struct Slot { int item; int modifier; };
// Module System names (header_items.py / header_troops.py)
enum {
    itp_type_horse = 1, itp_type_one_handed_wpn, itp_type_two_handed_wpn, itp_type_polearm, itp_type_arrows,
    itp_type_bolts, itp_type_shield, itp_type_bow, itp_type_crossbow, itp_type_thrown, itp_type_goods,
    itp_type_head_armor, itp_type_body_armor, itp_type_foot_armor, itp_type_hand_armor, itp_type_pistol,
    itp_type_musket, itp_type_bullets
};
enum { itp_two_handed = 0x00200000, itp_civilian = 0x20000000 };
enum { ek_item_0 = 0, ek_head = 4, ek_body, ek_foot, ek_gloves, ek_horse };
enum {
    tf_hero = 0x00000010, tf_inactive = 0x00000020,
    tf_guarantee_boots = 0x00100000, tf_guarantee_armor = 0x00200000, tf_guarantee_helmet = 0x00400000,
    tf_guarantee_gloves = 0x00800000, tf_guarantee_horse = 0x01000000, tf_guarantee_shield = 0x02000000,
    tf_guarantee_ranged = 0x04000000
};
struct Inv {
    char pad0[0x30];
    __int64 flags;              // +0x30 (troop flags)
    char pad38[0x4a4 - 0x38];
    Slot equip[16];             // +0x4a4
    bool add(Slot* s);          // 0x463070
    bool equip_best(int seed, int equip_slot, unsigned must, unsigned mustnot, unsigned type, unsigned t2, unsigned t3, char cheapest);   // 0x463100
    void unequip_all();         // 0x4687a0
    void auto_equip(int seed, int unused, char civilian, char no_helmet);    // 0x4687e0
};
// @ 0x4687a0
void Inv::unequip_all()
{
    Slot* p = equip;
    int n = 10;
    do {
        if (p->item >= 0) {
            if (add(p))
                p->item = -1;
        }
        ++p;
        --n;
    } while (n != 0);
}
// @ 0x4687e0
void Inv::auto_equip(int seed, int unused, char civilian, char no_helmet)
{
    if ((flags & tf_inactive) == 0) {
        bool b7 = false, b8 = false, b1 = false, b2 = false, b5 = false, b6 = false, b3 = false, b4 = false, b9 = false, b10 = false;
        unsigned mustnot_ = 0;
        unequip_all();
        unsigned must_ = 0;
        if ((flags & tf_hero) && civilian)
            must_ = itp_civilian;
        if ((flags & tf_hero) && no_helmet)
            mustnot_ = 0x80000000;
        equip_best(seed, ek_head, must_, mustnot_, -1, -1, -1, (flags & tf_guarantee_helmet) != 0);
        equip_best(seed, ek_body, must_, mustnot_, -1, -1, -1, (flags & tf_guarantee_armor) != 0);
        equip_best(seed, ek_foot, must_, mustnot_, -1, -1, -1, (flags & tf_guarantee_boots) != 0);
        equip_best(seed, ek_gloves, must_, mustnot_, -1, -1, -1, (flags & tf_guarantee_gloves) != 0);
        equip_best(seed, ek_horse, 0, mustnot_, -1, -1, -1, (flags & tf_guarantee_horse) != 0);
        for (int i = 0; i < 4; ++i) {
            char r = 0;
            if (i == 0) {
                r = equip_best(seed, 0, 0, mustnot_, itp_type_pistol, itp_type_musket, -1, (flags & tf_guarantee_ranged) != 0);
                if (!r)
                    r = equip_best(seed, 0, 0, mustnot_, itp_type_bow, itp_type_crossbow, itp_type_thrown, (flags & tf_guarantee_ranged) != 0);
                if (r) {
                    unsigned t = g_items[equip[0].item].type_flags & 0xff;
                    if (t == itp_type_bow) b1 = true;
                    else if (t == itp_type_crossbow) b3 = true;
                    else if (t == itp_type_musket || t == itp_type_pistol) b5 = true;
                }
                if (r) continue;
            }
            if (b1 && !b2) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_arrows, -1, -1, 1);
                if (r) b2 = true;
            }
            if (r) continue;
            if (b3 && !b4) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_bolts, -1, -1, 1);
                if (r) b4 = true;
            }
            if (r) continue;
            if (b5 && !b6) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_bullets, -1, -1, 1);
                if (r) b6 = true;
            }
            if (r) continue;
            if (b7 && !b8) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_shield, -1, -1, (flags & tf_guarantee_shield) != 0);
                if (r) b8 = true;
            }
            if (r) continue;
            if (!b9) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_one_handed_wpn, itp_type_two_handed_wpn, -1, 0);
                if (r) {
                    b9 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
            if (r) continue;
            if (!b10) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_polearm, -1, -1, 0);
                if (r) {
                    b10 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
            if (r) continue;
            if (!b9 && !b10) {
                r = equip_best(seed, i, 0, mustnot_, itp_type_one_handed_wpn, itp_type_two_handed_wpn, -1, 1);
                if (r) {
                    b9 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                } else {
                    r = equip_best(seed, i, 0, mustnot_, itp_type_polearm, -1, -1, 1);
                    if (!r) continue;
                    b10 = true;
                    if (!(g_items[equip[i].item].type_flags & itp_two_handed))
                        b7 = true;
                }
            }
        }
    }
}

// ---- 0x468c20: grow the bounding box of a body by a world point ----------------------------------------------
void __cdecl vmax3(float* a, const float* b);   // 0x468300 (s00467bf0)
void __cdecl vmin3(float* a, const float* b);   // 0x468360 (s00467bf0)
struct Body {
    char pad0[0x22c];
    vector4 bmin;               // +0x22c
    vector4 bmax;               // +0x23c
    char pad24c[0x27c - 0x24c];
    matrix inv;                 // +0x27c  world -> local
};
struct BodyHolder {
    char pad0[0x1f0];
    Body* body;                 // +0x1f0
    void extend(const vector4* p);      // 0x468c20
};
// @ 0x468c20
void BodyHolder::extend(const vector4* p)
{
    vector4 local = body->inv.transform_point_to_local(*p);
    vmin3(&body->bmin.x, &local.x);
    vmax3(&body->bmax.x, &local.x);
}

// ---- 0x468c80: occlusion test of a world point against the depth buffer ---------------------------------------------
struct Proj {
    void project(float* out, unsigned p);       // 0x4621f0
};
struct Probe {
    char pad0[0xc];
    int f0c;                    // +0xc
    char pad10[0x20 - 0x10];
    int width;                  // +0x20
    int height;                 // +0x24
    char pad28[0x1f0 - 0x28];
    char* scene;                // +0x1f0 (object holding a Proj at +0x10c)
    bool test(unsigned p, float* out);          // 0x468c80
    void read_depth(IDirect3DDevice9* dev, int x, int y, float* a, float* b);   // 0x4641a0
};
// @ 0x468c80
bool Probe::test(unsigned p, float* out)
{
    float* o = out;
    *o = 1.0f;
    bool result = true;
    if (scene) {
        vector4 pt;
        ((Proj*)(scene + 0x10c))->project(&pt.x, p);
        if (!(0.0f <= pt.x)) return true;
        if (!(pt.x < 1.0f)) return true;
        if (!(0.0f <= pt.y)) return true;
        if (!(pt.y < 1.0f)) return true;
        float scr[2];
        scr[0] = (float)width * pt.x;
        scr[1] = (1.0f - pt.y) * (float)height;
        if (g_use_shaders && f0c) {
            // the depth reader overwrites the two argument slots (&out receives the stored depth)
            read_depth(g_dev, (int)scr[0], (int)scr[1], (float*)&out, (float*)&p);
            if ((pt.z - 0.001f) > *(float*)&out)
                *o = 0.0f;
        } else {
            result = false;
        }
    } else {
        result = false;
    }
    return result;
}

// ---- 0x468770: movement-speed-like clamp of a 4-bit field ----------------------------------------------------------
struct Rec468770 {
    char pad0[0x58];
    int f58;                    // +0x58
    char pad5c[0x7a - 0x5c];
    unsigned char b7a;          // +0x7a (low nibble = level)
    int level_index();          // 0x468770
};
// @ 0x468770
int Rec468770::level_index()
{
    int base = f58;
    int v = b7a & 0xf;
    if (v < 0)
        v = 0;
    else if (v > g_lod_max)
        v = g_lod_max;
    return base + v * 2 + 0x23;
}
