#include "./e96_vec.h"

// @ 0x4f47f0
template E96* std::_Uninit_copy<vec<E96>::iter, E96*, std::allocator<E96> >(vec<E96>::iter, vec<E96>::iter, E96*, std::allocator<E96>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x4f4960
template E32* std::_Uninit_copy<vec<E32>::iter, E32*, std::allocator<E32> >(vec<E32>::iter, vec<E32>::iter, E32*, std::allocator<E32>&, std::_Nonscalar_ptr_iterator_tag);
// @ 0x4f46d0
template void vec<E96>::reserve(unsigned);

template<class T> void vec<T>::reserve_nr(unsigned n)
{
    if ((unsigned)-1 / sizeof(T) < n)
        xlen_nr();
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
// @ 0x4f4860
template void vec<E32>::reserve_nr(unsigned);
