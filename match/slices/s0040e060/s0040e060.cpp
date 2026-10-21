// Slice s0040e060: std::vector<32-byte> insert helpers, small float/colour helpers, matrix axis rotations,
// matrix * transform, the renderer's alpha-blend mode setter and the MSVC 7.1 <vector> instantiations for a
// 32-byte element. Build flags: /O2 /MT /Gy /TP /EHa (the STL instantiations need /EHa: with /EHsc cl 13.10
// enregisters loop variables around try/catch(...) and the frame no longer matches).
#include <windows.h>
#include <d3d9.h>
#include <math.h>
#include <stdlib.h>
#include "rgl.h"

using rgl::vector4;

// ---- float helpers ---------------------------------------------------------------------------
// @ 0x40e3d0
float clamp(const float* x, const float* lo, const float* hi)
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}

// @ 0x40e400
// uniform random in [0, 15817 * 6.3223e-5) = [0, 1)
float frand()
{
    return (float)(rand() % 15817) * (1.0f / 15817.0f);
}

// this += o (xyz only)
struct vec3 {
    float x, y, z;
    void add(const vec3& o);
};
// @ 0x40e420
void vec3::add(const vec3& o)
{
    x = o.x + x;
    y = o.y + y;
    z = o.z + z;
}

// @ 0x40e440
// pack (r, g, b, a) floats in 0..1 into 0xAARRGGBB
static inline unsigned min255(unsigned v) { return v < 255 ? v : 255; }
unsigned pack_color(const float* c)
{
    unsigned a = (unsigned)(c[3] * 256.0f);
    unsigned r = (unsigned)(c[0] * 256.0f);
    unsigned g = (unsigned)(c[1] * 256.0f);
    unsigned b = (unsigned)(c[2] * 256.0f);
    return (((min255(a) << 8 | min255(r)) << 8) | min255(g)) << 8 | min255(b);
}

// @ 0x40e4e0
// unpack 0xAARRGGBB into (r, g, b, a) floats in 0..1
vector4 unpack_color(const unsigned* c)
{
    vector4 v;
    const unsigned char* b = (const unsigned char*)c;
    v.w = b[3] * (1.0f / 255.0f);
    v.x = b[2] * (1.0f / 255.0f);
    v.y = b[1] * (1.0f / 255.0f);
    v.z = (*c & 0xff) * (1.0f / 255.0f);
    return v;
}

// ---- matrix helpers ----------------------------------------------------------------------------
// Inline vector4 helpers as the original's TU had them (scalar passed by const reference, so a double
// sin/cos result is stored as a float temporary per use).
static __forceinline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

struct pos2 : rgl::matrix {
    void rotate_fu(const float* angle);     // 0x40e570  rotate forward/up rows about the side axis
    void rotate_su(const float* angle);     // 0x40e670  rotate side/up rows about the forward axis
    void rotate_sf(const float* angle);     // 0x40e770  rotate side/forward rows about the up axis
};

// @ 0x40e570
void pos2::rotate_fu(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 nf, nu;
    nf = vadd(vmul(f, cos((double)*angle)), vmul(u, sn));
    nu = vsub(vmul(u, cos((double)*angle)), vmul(f, sn));
    u = nu;
    f = nf;
}
// @ 0x40e670
void pos2::rotate_su(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nu;
    ns = vsub(vmul(s, cos((double)*angle)), vmul(u, sn));
    nu = vadd(vmul(u, cos((double)*angle)), vmul(s, sn));
    s = ns;
    u = nu;
}
// @ 0x40e770
void pos2::rotate_sf(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nf;
    ns = vadd(vmul(s, cos((double)*angle)), vmul(f, sn));
    nf = vsub(vmul(f, cos((double)*angle)), vmul(s, sn));
    s = ns;
    f = nf;
}

// 0x40e870: matrix product returned by value (hidden return pointer = first stack arg): rotation rows via
// 0x40a920, origin = this->transform_point(m.o) with the transform inlined
struct mat3 { vector4 a, b, c; };
struct pos3 : rgl::matrix {
    mat3 rot3(const rgl::matrix& m) const;                      // 0x40a920 (s0040a640)
    __forceinline vector4 tp(const vector4& p) const
    {
        return vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                       s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                       s.z * p.x + f.z * p.y + u.z * p.z + o.z);
    }
    rgl::matrix mul(const rgl::matrix& m) const;                // 0x40e870
};
// @ 0x40e870
rgl::matrix pos3::mul(const rgl::matrix& m) const
{
    struct { mat3 r; vector4 o; } t;
    t.o = tp(m.o);
    t.r = rot3(m);
    return *(rgl::matrix*)&t;
}

// ---- render state object (global at 0xa17910) --------------------------------------------------
struct renderer {
    char pad000[0x58c];
    int blend_mode;                  // 0x58c  0 off, 1 alpha, 2 additive, else multiply
    char pad590[0x618 - 0x590];
    IDirect3DDevice9* dev;           // 0x618
    void set_blend_mode(int mode);   // 0x40e940
};
// @ 0x40e940
void renderer::set_blend_mode(int mode)
{
    if (mode != blend_mode) {
        blend_mode = mode;
        if (mode == 0) {
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
            return;
        }
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
        if (blend_mode == 1) {
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            return;
        }
        if (blend_mode == 2) {
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        } else {
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
        }
    }
}

// ---- std::vector<32-byte element>::_Insert_n and _Uninit_fill_n ---------------------------------
// The original is MSVC 7.1's <vector> instantiated for a 32-byte POD (probably a vector4 pair). It keeps
// loop variables in memory around the try/catch(...) regions; cl 13.10.3052 enregisters them, so these
// are complete and equivalent but not byte-exact.
#include <vector>
#include <new>
struct elem32 { int a[8]; };
struct vec32 {
    std::allocator<elem32> _Alval;
    elem32 *_Myfirst, *_Mylast, *_Myend;
    typedef unsigned size_type;
    typedef elem32* iterator;
    size_type capacity() const { return _Myfirst == 0 ? 0 : _Myend - _Myfirst; }
    size_type size() const { return _Myfirst == 0 ? 0 : _Mylast - _Myfirst; }
    size_type max_size() const { return _Alval.max_size(); }
    __forceinline elem32* _Ucopy(elem32* f, elem32* l, elem32* p) { return std::_Uninitialized_copy(f, l, p, _Alval); }
    __forceinline elem32* _Ufill(elem32* p, size_type n, const elem32& v) { std::_Uninitialized_fill_n(p, n, v, _Alval); return p + n; }
    void _Destroy(elem32* f, elem32* l) { for (; f != l; ++f) std::_Destroy(f); }
    void _Xlen() const;                                         // 0x43e2a0 (length_error "vector<T> too long")
    void _Insert_n(iterator _Where, size_type _Count, const elem32& _Val);      // 0x40e060
};
void vec32::_Xlen() const { throw std::length_error("vector<T> too long"); }
// @ 0x40e060
void vec32::_Insert_n(iterator _Where, size_type _Count, const elem32& _Val)
{
    elem32 _Tmp = _Val;
    size_type _Capacity = capacity();
    if (_Count == 0)
        ;
    else if (max_size() - size() < _Count)
        _Xlen();
    else if (_Capacity < size() + _Count) {
        _Capacity = max_size() - _Capacity / 2 < _Capacity ? 0 : _Capacity + _Capacity / 2;
        if (_Capacity < size() + _Count)
            _Capacity = size() + _Count;
        elem32* _Newvec = _Alval.allocate(_Capacity);
        elem32* _Ptr = _Newvec;
        try {
            _Ptr = _Ucopy(_Myfirst, _Where, _Newvec);
            _Ptr = _Ufill(_Ptr, _Count, _Tmp);
            _Ucopy(_Where, _Mylast, _Ptr);
        } catch (...) {
            _Destroy(_Newvec, _Ptr);
            _Alval.deallocate(_Newvec, _Capacity);
            throw;
        }
        _Count += size();
        if (_Myfirst != 0) {
            _Destroy(_Myfirst, _Mylast);
            _Alval.deallocate(_Myfirst, _Myend - _Myfirst);
        }
        _Myend = _Newvec + _Capacity;
        _Mylast = _Newvec + _Count;
        _Myfirst = _Newvec;
    } else if ((size_type)(_Mylast - _Where) < _Count) {
        _Ucopy(_Where, _Mylast, _Where + _Count);
        try {
            _Ufill(_Mylast, _Count - (_Mylast - _Where), _Tmp);
        } catch (...) {
            _Destroy(_Where + _Count, _Mylast + _Count);
            throw;
        }
        _Mylast += _Count;
        {
            elem32* _Fe = _Mylast - _Count;
            for (elem32* _Q = _Where; _Q != _Fe; ++_Q)
                *_Q = _Tmp;
        }
    } else {
        elem32* _Oldend = _Mylast;
        _Mylast = _Ucopy(_Oldend - _Count, _Oldend, _Mylast);
        {
            elem32* _L = _Oldend - _Count;
            elem32* _D = _Oldend;
            while (_Where != _L)
                *--_D = *--_L;
        }
        {
            elem32* _Q = _Where;
            elem32* _Fe = _Where + _Count;
            for (; _Q != _Fe; ++_Q)
                *_Q = _Tmp;
        }
    }
}

// 0x40e350: std::_Uninit_fill_n for elem32, allocator::construct inlined (placement copy-construct)
// @ 0x40e350
void __cdecl uninit_fill_n32(elem32* f, unsigned n, const elem32& v, std::allocator<elem32>& al, int tag)
{
    elem32* s = f;
    try {
        for (; 0 < n; --n, ++f) new ((void*)f) elem32(v);
    } catch (...) {
        for (; s != f; ++s) al.destroy(s);
        throw;
    }
}
