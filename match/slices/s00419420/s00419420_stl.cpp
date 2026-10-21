// Slice s00419420 (STL part): VC7.1 <vector>/<memory> instantiations over the 8-byte record E8, a 4-byte
// record with an out-of-line copy constructor, and the 12-byte vec_array record. Built with the plain
// flags (no EH switch): an explicit try/catch (the STL's _TRY_BEGIN) still gets its frame and funclets,
// but objects with destructors are not registered, which is what the originals look like.
#include <vector>
#include <memory>

struct E8 { int v[2]; };       // 0x08, same type as s0040f9a0
struct E4 { int v; };          // 0x04 record (copy-constructed out of line at 0x436d80)

// 12-byte record: parallel arrays (see s00417850's vec_array). Implicit copy ctor, user destructor.
struct vec16 { float x, y, z, w; };
struct vec_array {
    vec16* v;
    int* idx;
    int n;
    ~vec_array() { if (v) delete[] v; if (idx) delete[] idx; }       // 0x4196a0 (body of allocator::destroy)
    void operator=(const vec_array& o);                              // 0x417850
};

namespace std {
// 0x4196d0 _Uninit_copy (+ catch funclet 0x41973a)
template vec_array* _Uninit_copy<vec_array*, vec_array*, allocator<vec_array> >(vec_array*, vec_array*, vec_array*, allocator<vec_array>&, _Nonscalar_ptr_iterator_tag);
// 0x41a130 _Uninit_fill_n (+ catch funclet 0x41a198)
template void _Uninit_fill_n<vec_array*, unsigned int, vec_array, allocator<vec_array> >(vec_array*, unsigned int, const vec_array&, allocator<vec_array>&, _Nonscalar_ptr_iterator_tag);
}

template class std::allocator<vec_array>;
template class std::vector<E8>;
template class std::vector<vec_array>;
template class std::vector<E4>;
