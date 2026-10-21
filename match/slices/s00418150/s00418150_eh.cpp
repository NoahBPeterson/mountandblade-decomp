// Slice s00418150, part 2: VC7.1 <memory>/<list> helper instantiations that need an EH frame (built with /EHa, like
// s0040f9a0: with /EHsc cl drops the try region because construct() inlines to nothing that can throw).
// The 9-byte blocks at 0x4181b4 and 0x418516 are the catch(...) { throw; } funclets of 0x418150 and 0x4184b0;
// the 21-byte block at 0x41881f is the catch(...) funclet of 0x4187b0 (operator delete + rethrow). cl emits them
// after the parent in the same COMDAT with no symbol of their own.
#include <memory>
#include <list>

struct E8 { int v[2]; };             // 8 bytes (same instantiation as s0040f9a0's 0x40fa10)

namespace std {
// @ 0x418150
template void _Uninit_fill_n<E8*, unsigned int, E8, allocator<E8> >(E8*, unsigned int, const E8&, allocator<E8>&, _Nonscalar_ptr_iterator_tag);
// @ 0x4184b0
template int** _Uninit_copy<int**, int**, allocator<int*> >(int**, int**, int**, allocator<int*>&, _Nonscalar_ptr_iterator_tag);
}

struct res_obj;                      // list element type unknown (a 4-byte pointer-sized value)
// @ 0x4187b0
// std::list<res_obj*>::_Buynode()  -- the head-node allocator, instantiated by the whole-class instantiation below
template class std::list<res_obj*>;
