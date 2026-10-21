// Slice s004251c0, part 2: VC7.1 <memory> helper that needs an EH frame (built with /EHa, like s0040f9a0: with /EHsc cl
// drops the try region because construct() inlines to nothing that can throw).
// 0x42522d (9 bytes) is the catch(...) { throw; } funclet of 0x4251c0: cl emits it after the parent with no symbol.
// std::_Uninit_fill_n itself does not match for a 0x88-byte element: cl 13.10 leaves allocator::construct out of
// line once the copied element exceeds 128 bytes, while the original inlined it (E100 in s0040f9a0 does inline).
// The function below writes the same loop with the placement new spelled out (as s0040cfe0's uninit_copy does).
#include <new>
#include <memory>

struct E136 { int v[34]; };     // 0x88-byte record (the group record of s00423ed0)

// std::_Uninit_fill_n with allocator::construct expanded
template<class T> __declspec(noinline) void uninit_fill_n(T* first, unsigned count, const T& val, std::allocator<T>& al, std::_Nonscalar_ptr_iterator_tag tag)
{
    T* next = first;
    try {
        for (; 0 < count; --count, ++first)
            ::new ((void*)first) T(val);
    } catch (...) {
        for (; next != first; ++next) ;
        throw;
    }
}

// @ 0x4251c0
template void uninit_fill_n<E136>(E136*, unsigned, const E136&, std::allocator<E136>&, std::_Nonscalar_ptr_iterator_tag);
