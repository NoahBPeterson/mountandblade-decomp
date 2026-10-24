#ifndef E96_VEC_H
#define E96_VEC_H
// Slice s004f46d0: the out-of-line instantiations of std::vector<T>::reserve and std::_Uninit_copy for the
// engine's 0x60-byte and 0x20-byte elements (VC7.1 Dinkumware <vector>; layout: empty allocator slot, _Myfirst
// +4, _Mylast +8, _Myend +0xc).  reserve() is the STL member written out (so _Ucopy/_Uninitialized_copy can
// be forced inline, as in the original): max_size check -> _Xlen (0x43e2a0, shared ICF'd body), capacity() < n
// -> allocate, _Ucopy inside try/catch(...){ deallocate; throw; }, then free the old block and install
// first/last/end.  The 5-argument cdecl _Uninit_copy (first, last, dest, &allocator,
// _Nonscalar_ptr_iterator_tag) calls allocator::construct (0x40f5b0 for 0x60, 0x40f5f0 for 0x20) per element.
//   0x4f46d0  vec<E96>::reserve         0x4f47d8  its catch funclet (no symbol of its own)
//   0x4f47f0  _Uninit_copy<E96*>        0x4f4856  its rethrow funclet
//   0x4f4860  vec<E32>::reserve_nr        0x4f4947  its catch funclet
//   0x4f4960  _Uninit_copy<E32*>        0x4f49c6  its rethrow funclet
#include <vector>
#include <memory>

struct E96 { int v[24]; };      // 0x60
#include "../../include/pod_elems.h"   // E32 (shared with agent3/agent_combat.h)

template<class T> struct vec {                  // std::vector<T> layout
    std::allocator<T> al;
    T* first;
    T* last;
    T* end;
    struct iter {                               // std::vector<T>::iterator (_Ptrit)
        T* cur;
        iter(T* p) : cur(p) {}
        T& operator*() const { return *cur; }
        iter& operator++() { ++cur; return *this; }
        bool operator!=(const iter& r) const { return cur != r.cur; }
        bool operator==(const iter& r) const { return cur == r.cur; }
    };
    unsigned size() const { return first == 0 ? 0 : last - first; }
    unsigned capacity() const { return first == 0 ? 0 : end - first; }
    void xlen() const;                          // 0x43e2a0 (shared body, throws length_error)
    __declspec(noreturn) void xlen_nr() const;  // same body, declared noreturn in the E32 instance's TU
    void reserve(unsigned n);                   // 0x4f46d0 (E96): _Xlen is an ordinary call
    void reserve_nr(unsigned n);                // 0x4f4860 (E32): same code, but cl knew _Xlen is noreturn here (n stays in edx)
};

template<class I, class T> static __forceinline T* uninitialized_copy_(I f, I l, T* d, std::allocator<T>& a)
{
    return std::_Uninit_copy(f, l, d, a, std::_Ptr_cat(f, d));   // std::_Uninitialized_copy
}
template<class I, class T> static __forceinline T* ucopy(I f, I l, T* d, std::allocator<T>& a)
{
    return uninitialized_copy_(f, l, d, a);                      // vector::_Ucopy
}

template<class T> void vec<T>::reserve(unsigned n)
{
    if ((unsigned)-1 / sizeof(T) < n)
        xlen();
    if (capacity() < n) {
        T* p = al.allocate(n);
        try {
            ucopy(iter(first), iter(last), p, al);
        } catch (...) {
            al.deallocate(p, n);
            throw;
        }
        unsigned sz = size();
        if (first != 0)
            al.deallocate(first, end - first);
        end = p + n;
        last = p + sz;
        first = p;
    }
}

#endif
