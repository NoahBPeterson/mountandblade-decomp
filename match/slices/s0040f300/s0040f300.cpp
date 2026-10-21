// Slice s0040f300: MSVC 7.1 STL instantiations for the particle-system vectors (fill / copy_backward /
// allocator::construct / _Uninit_copy / _Uninit_fill_n for 8-, 32-, 48-, 96- and 100-byte elements), the
// material/blend render-state setup of a mesh instance (0x40f3f0) and a draw-budget test (0x40f7f0).
// Element type names are provisional. Flags: /O2 /MT /Gy /TP /EHa (the STL helpers need /EHa: with /EHsc cl
// 13.10 enregisters the loop variables around try/catch(...), the original keeps them in the frame).
// The catch funclets after the _Uninit_* helpers (0x40f696, 0x40f706, ...) are not separate functions: they
// are the `throw;` tails of the helper above them.
#include <windows.h>
#include <d3d9.h>
#include <new>
#include <vector>
#include "rgl.h"

struct elem96  { int a[24]; };
struct elem48  { int a[12]; };
struct elem32  { int a[8]; };
struct elem8   { int a, b; };
struct particle { int a[25]; };      // 100 bytes (see s0040ea00: particle emitter)

// ---- fill / copy_backward (std::fill, std::copy_backward on pointers) --------------------------
// @ 0x40f300
void __cdecl fill32(elem32* f, elem32* l, const elem32& v) { for (; f != l; ++f) *f = v; }
// @ 0x40f330
elem32* __cdecl copy_backward32(elem32* f, elem32* l, elem32* d) { while (f != l) *--d = *--l; return d; }
// @ 0x40f360
elem8* __cdecl copy_backward8(elem8* f, elem8* l, elem8* d) { elem8* e = l; while (f != e) *--d = *--e; return d; }
// @ 0x40f390
void __cdecl fill100(particle* f, particle* l, const particle& v) { for (; f != l; ++f) *f = v; }
// @ 0x40f3c0
particle* __cdecl copy_backward100(particle* f, particle* l, particle* d) { while (f != l) *--d = *--l; return d; }

// ---- allocator<T>::construct (placement copy-construct) ----------------------------------------
// Callers in this TU must see an opaque call (the original does not inline it), so each allocator is a
// declaration-only stub; the out-of-line bodies are on the *_def twins.
struct alloc96 { void construct(elem96* p, const elem96& v); void destroy(elem96* p) { p->~elem96(); } };     // 0x40f5b0
struct alloc48 { void construct(elem48* p, const elem48& v); void destroy(elem48* p) { p->~elem48(); } };     // 0x40f5d0
struct alloc32 { void construct(elem32* p, const elem32& v); void destroy(elem32* p) { p->~elem32(); } };     // 0x40f5f0
struct alloc8  { void construct(elem8* p, const elem8& v); };       // 0x40f610
struct alloc100 { void construct(particle* p, const particle& v); void destroy(particle* p) { p->~particle(); } };    // 0x468170

struct alloc96_def { void construct(elem96* p, const elem96& v); };
struct alloc48_def { void construct(elem48* p, const elem48& v); };
struct alloc32_def { void construct(elem32* p, const elem32& v); };
struct alloc8_def  { void construct(elem8* p, const elem8& v); };
// @ 0x40f5b0
void alloc96_def::construct(elem96* p, const elem96& v) { new ((void*)p) elem96(v); }
// @ 0x40f5d0
void alloc48_def::construct(elem48* p, const elem48& v) { new ((void*)p) elem48(v); }
// @ 0x40f5f0
void alloc32_def::construct(elem32* p, const elem32& v) { new ((void*)p) elem32(v); }
// @ 0x40f610
void alloc8_def::construct(elem8* p, const elem8& v) { new ((void*)p) elem8(v); }

// ---- std::_Uninit_copy / _Uninit_fill_n with an opaque allocator::construct ---------------------
// @ 0x40f630
elem96* __cdecl uninit_copy96(elem96* f, elem96* l, elem96* d, alloc96& al, int tag)
{
    elem96* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}
// @ 0x40f6a0
elem48* __cdecl uninit_copy48(elem48* f, elem48* l, elem48* d, alloc48& al, int tag)
{
    elem48* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}
// @ 0x40f710
elem32* __cdecl uninit_copy32(elem32* f, elem32* l, elem32* d, alloc32& al, int tag)
{
    elem32* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}
// @ 0x40f780
particle* __cdecl uninit_copy100(particle* f, particle* l, particle* d, alloc100& al, int tag)
{
    particle* s = d;
    try {
        for (; f != l; ++d, ++f) al.construct(d, *f);
    } catch (...) {
        for (; s != d; ++s) al.destroy(s);
        throw;
    }
    return d;
}
// @ 0x40f8c0
void __cdecl uninit_fill_n96(elem96* f, unsigned n, const elem96& v, alloc96& al, int tag)
{
    elem96* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) al.destroy(s);
        throw;
    }
}
// @ 0x40f930
void __cdecl uninit_fill_n48(elem48* f, unsigned n, const elem48& v, alloc48& al, int tag)
{
    elem48* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) al.destroy(s);
        throw;
    }
}

// ---- fill_n for 4-byte elements; indexed element address -------------------------------------
// @ 0x40f8a0
int* __cdecl fill_n4(int* p, unsigned n, const int& v) { for (; 0 < n; --n, ++p) *p = v; return p; }

// vector<elem32>::iterator::operator+ (the iterator is a one-pointer class returned by value)
struct iter32 {
    elem32* _Ptr;
    iter32() {}
    iter32(elem32* p) : _Ptr(p) {}
    iter32 operator+(int off) const;
};
// @ 0x40f880
iter32 iter32::operator+(int off) const { return iter32(_Ptr + off); }

// ---- global render state (0xa17910) -------------------------------------------------------------
struct renderer {
    char pad000[0x574];
    int stage_xform[3];              // 0x574  per-stage D3DTSS_TEXTURETRANSFORMFLAGS cache
    int stage_texgen[3];             // 0x580  per-stage TEXCOORDINDEX mode cache
    int blend_mode;                  // 0x58c  0 off, 1 alpha, 2 additive, else multiply
    char pad590[4];
    unsigned tex_factor;             // 0x594  cached D3DRS_TEXTUREFACTOR
    char pad598[0x618 - 0x598];
    IDirect3DDevice9* dev;           // 0x618
    void set_texcoord_gen(int stage, int mode);     // 0x40ea00
    void set_blend_mode(int mode);                  // 0x40e940
};
extern renderer g_render;   // 0xa17910

rgl::vector4 unpack_color(const unsigned* c);       // 0x40e4e0

// ---- mesh instance ------------------------------------------------------------------------------
struct stage_rec { unsigned flags; char pad[0x10]; };      // 0x14 bytes per texture stage
struct material {
    char pad00[0x20];
    unsigned flags;                  // 0x20  bit 28: colour goes to a pixel-shader constant
    char pad24[0x74 - 0x24];
    stage_rec stages[3];             // 0x74
    char padb0[8];
    int num_stages;                  // 0xb8
};
struct mesh_part {
    char pad00[0x1c];
    unsigned flags;                  // 0x1c  bits 8..10 blend mode (7 = by vertex alpha), 0x40000/0x80000 draw flags
    char pad20[0x30 - 0x20];
    material* mat;                   // 0x30
};
struct mesh_inst {
    char pad00[0x18];
    mesh_inst* master;               // 0x18
    char pad1c[0x2c - 0x1c];
    mesh_part* part;                 // 0x2c
    char pad30[0x38 - 0x30];
    char force_blend;                // 0x38
    char pad39[3];
    unsigned color;                  // 0x3c  ARGB
    char pad40[0x70 - 0x40];
    std::vector<elem32> verts;       // 0x70
    char pad80[0xa1 - 0x80];
    char is_owner;                   // 0xa1  node owns its data (chain terminator)
    char padA2[0x1d4 - 0xa2];
    D3DMATRIX tex_matrix;            // 0x1d4
    void setup_material();           // 0x40f3f0
    int over_budget();               // 0x40f7f0
    std::vector<elem32>* get_verts();   // 0x40eba0 (out of line, opaque to callers)
    __forceinline std::vector<elem32>* get_verts_inl()      // one level of the recursive accessor, inlined
    {
        if (is_owner)
            return &verts;
        return master->get_verts();
    }
};

// @ 0x40f3f0
void mesh_inst::setup_material()
{
    unsigned flags = part->flags;
    unsigned bm = flags & 0x700;
    if (force_blend) {
        g_render.set_blend_mode(1);
    } else if (bm == 0x700 && (color & 0xff000000) == 0xff000000) {
        if (g_render.blend_mode) {
            g_render.blend_mode = 0;
            g_render.dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
        }
    } else if (bm == 0x700) {
        g_render.set_blend_mode(1);
    } else {
        g_render.set_blend_mode((flags >> 8) & 7);
    }
    material* m = part->mat;
    int i = 0;
    if (0 < m->num_stages) {
        int off = 0;
        do {
            g_render.set_texcoord_gen(i, *(unsigned*)((char*)m + 0x74 + off) & 0xff);
            if (*(unsigned*)((char*)part->mat + 0x74 + off) & 0x100) {
                if (g_render.stage_xform[i] != 1) {
                    g_render.dev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
                    g_render.stage_xform[i] = 1;
                }
                g_render.dev->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + i), &tex_matrix);
            } else if (g_render.stage_xform[i] != 0) {
                g_render.dev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
                g_render.stage_xform[i] = 0;
            }
            m = part->mat;
            ++i;
            off += 0x14;
        } while (i < m->num_stages);
    }
    if (part && (part->mat->flags & 0x10000000)) {
        rgl::vector4 c;
        c = unpack_color(&color);
        float* tmp = &c.x;
        g_render.dev->SetPixelShaderConstantF(0, tmp, 1);
        return;
    }
    unsigned col = color;
    if (g_render.tex_factor != col) {
        g_render.tex_factor = col;
        g_render.dev->SetRenderState(D3DRS_TEXTUREFACTOR, col);
    }
}

// ---- draw budget test ---------------------------------------------------------------------------
extern int g_vert_budget;    // 0x5a9c34 (base vertex budget; provisional name)

// @ 0x40f7f0
int mesh_inst::over_budget()
{
    mesh_part* p = part;
    unsigned fl = p->flags;
    if (!(fl & 0x40000)) {
        if (!(fl & 0x80000))
            return 0;
        std::vector<elem32>* v = get_verts_inl();
        if (!(0 < v->size()))
            return 0;
        int n = get_verts()->size();
        if (!(n < g_vert_budget + ((p->mat->flags & 0x80000000) ? 0x46 : 0)))
            return 0;
    }
    return 1;
}
