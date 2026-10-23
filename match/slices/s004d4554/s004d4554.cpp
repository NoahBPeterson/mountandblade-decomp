// Slice s004d4554: Microsoft C++ 7.1 standard-library helper code emitted for the game's vectors:
// std::_Uninit_fill_n (<memory>) for three element types (strides 0x50, 0x68 and 0xb0), the count-bounded
// "construct n copies of *val" loops the vector uses when it grows in place:
//
//     template<class _FI, class _Diff, class _Tval, class _Alloc> void _Uninit_fill_n(_FI _F, _Diff _N,
//         const _Tval& _X, _Alloc& _Al)
//     {   _FI _Next = _F;
//         try { for (; 0 < _N; --_N, ++_F) _Al.construct(_F, _X); }
//         catch (...) { for (; _Next != _F; ++_Next) _Al.destroy(_Next); throw; }
//     }
//
// The catch funclets are the original's 0x4d4a28 / 0x4d4ab8 / 0x4d4b4b (the destroy loops + rethrow),
// compiled with the function. The allocators' construct members (0x4d1140 / 0x4d1160 / 0x4d1180) live in
// slice s004d0c20 (byte-exact there); here they are only declared. The strides, so these records are
// distinct from that slice's 0x20/0x4c-byte views of the same copy-constructors: only sizeof matters.
#include <windows.h>
#include <new>

struct Elem50 { char pad0[0x50]; };
struct Elem68 {
    char pad0[0x68];
    void destroy();                                   // 0x4c8ba0 (the element's dtor, thiscall)
};
struct ElemB0 {
    char pad0[0xb0];
    void destroy();                                   // 0x4c8ab0 (the element's dtor, thiscall)
};

// allocator of the 0x50-byte vector: construct = 0x4d1140 (ctor 0x4cd080), destroy = 0x4cd6a0 (a real call)
struct Alloc50 {
    void construct(Elem50* p, const Elem50& v);       // 0x4d1140
    void destroy(Elem50* p);                          // 0x4cd6a0
};
// 0x68-byte vector: construct = 0x4d1160 (ctor 0x4cd100), destroy inlined to the dtor 0x4c8ba0
struct Alloc68 {
    void construct(Elem68* p, const Elem68& v);       // 0x4d1160
    void destroy(Elem68* p) { p->destroy(); }
};
// 0xb0-byte vector: construct = 0x4d1180 (ctor 0x4cd170), destroy inlined to the dtor 0x4c8ab0
struct AllocB0 {
    void construct(ElemB0* p, const ElemB0& v);       // 0x4d1180
    void destroy(ElemB0* p) { p->destroy(); }
};

// @ 0x4d49c0
void uninit_fill_n_50(Elem50* f, unsigned n, const Elem50& x, Alloc50& al)
{
    Elem50* next = f;
    try {
        for (; 0 < n; --n, ++f)
            al.construct(f, x);
    } catch (...) {
        for (; next != f; ++next)
            al.destroy(next);
        throw;
    }
}

// @ 0x4d4a50
void uninit_fill_n_68(Elem68* f, unsigned n, const Elem68& x, Alloc68& al)
{
    Elem68* next = f;
    try {
        for (; 0 < n; --n, ++f)
            al.construct(f, x);
    } catch (...) {
        for (; next != f; ++next)
            al.destroy(next);
        throw;
    }
}

// @ 0x4d4ae0
void uninit_fill_n_b0(ElemB0* f, unsigned n, const ElemB0& x, AllocB0& al)
{
    ElemB0* next = f;
    try {
        for (; 0 < n; --n, ++f)
            al.construct(f, x);
    } catch (...) {
        for (; next != f; ++next)
            al.destroy(next);
        throw;
    }
}
