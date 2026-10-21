// Slice s0040f9a0: VC7.1 <vector>/<memory> instantiations (std::vector<T>::_Insert_n and its helpers) for
// six POD element types, plus one vector deleting destructor.
// Flags: the STL helpers need /EHa (with /EHsc cl drops the EH frame because construct() inlines to nothing
// that can throw; the original keeps it). The 9-byte stubs after _Uninit_* are the catch(...) { throw; }
// funclets: cl emits them in the parent's COMDAT, with no symbol of their own.
// The vector deleting destructor (0x40fe10) comes from a TU built WITHOUT any EH switch: with /EHsc or /EHa
// cl calls the ??_M vector-destructor-iterator helper instead of looping inline.
#include <vector>
#include <memory>

// Element types (sizes from the instantiations; field meaning unknown).
struct E8   { int v[2]; };     // 0x08
struct E20  { int v[5]; };     // 0x14
struct E32  { int v[8]; };     // 0x20
struct E48  { int v[12]; };    // 0x30
struct E96  { int v[24]; };    // 0x60
struct E100 { int v[25]; };    // 0x64

// Record holding two vectors: sizeof 0x20, first/last/end at +4 and +0x14 (array deleted at 0x4125e0).
struct VecPair { std::vector<int> a; std::vector<int> b; };

namespace std {
// 0x40f9a0 _Uninit_fill_n
template void _Uninit_fill_n<E32*, unsigned int, E32, allocator<E32> >(E32*, unsigned int, const E32&, allocator<E32>&, _Nonscalar_ptr_iterator_tag);
// 0x40fa10 _Uninit_fill_n
template void _Uninit_fill_n<E8*, unsigned int, E8, allocator<E8> >(E8*, unsigned int, const E8&, allocator<E8>&, _Nonscalar_ptr_iterator_tag);
// 0x40fa80 _Uninit_fill_n
template void _Uninit_fill_n<E100*, unsigned int, E100, allocator<E100> >(E100*, unsigned int, const E100&, allocator<E100>&, _Nonscalar_ptr_iterator_tag);
// 0x40faf0 _Uninit_copy
template E8* _Uninit_copy<E8*, E8*, allocator<E8> >(E8*, E8*, E8*, allocator<E8>&, _Nonscalar_ptr_iterator_tag);
}

template class std::vector<E8>;
template class std::vector<E20>;
template class std::vector<E32>;
template class std::vector<E48>;
template class std::vector<E96>;
template class std::vector<E100>;

// @ 0x40fe10
void destroy_vecpair_array(VecPair* p) { delete[] p; }
