// Slice s00419a30 (STL part): std::vector<vec_array> helpers (_Ucopy/_Ufill/_Destroy, _Destroy_range, _Uninit_fill_n); no EH switch (see s00419420_stl.cpp).
#include <vector>
#include <memory>

struct res_base;
struct vec16 { float x, y, z, w; };
struct vec_array {              // 12 bytes, implicit copy ctor, user destructor
    vec16* v;
    int* idx;
    int n;
    ~vec_array() { if (v) delete[] v; if (idx) delete[] idx; }
    void operator=(const vec_array& o);                              // 0x417850
};

namespace std {
// 0x41a130 _Uninit_fill_n (+ catch funclet 0x41a198)
template void _Uninit_fill_n<vec_array*, unsigned int, vec_array, allocator<vec_array> >(vec_array*, unsigned int, const vec_array&, allocator<vec_array>&, _Nonscalar_ptr_iterator_tag);
// 0x41a0f0 _Destroy_range
template void _Destroy_range<vec_array, allocator<vec_array> >(vec_array*, vec_array*, allocator<vec_array>&, _Nonscalar_ptr_iterator_tag);
}

template class std::allocator<vec_array>;
template class std::vector<vec_array>;
