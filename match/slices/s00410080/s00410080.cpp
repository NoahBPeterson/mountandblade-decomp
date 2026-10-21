// Slice s00410080: VC7.1 <vector> instantiations (continued from s0040f9a0): the tail of
// vector<E96>::_Insert_n, vector<E32>::_Insert_n, vector<E96>::insert(where, val), the _Buy(size_type)
// allocators, vector<E32>::resize and vector<int>::_Insert_n.  Built with /EHa (see s0040f9a0.cpp).
// Each _Insert_n is followed in the binary by its catch(...) funclets and the code of its cold branches
// (0x410080, 0x410304, 0x410770): cl keeps them in the parent's COMDAT, after the main body.
#include <vector>
#include <memory>

// Element types (sizes from the instantiations; field meaning unknown).
struct E8   { int v[2]; };     // 0x08
struct E20  { int v[5]; };     // 0x14
struct E32  { int v[8]; };     // 0x20
struct E48  { int v[12]; };    // 0x30
struct E96  { int v[24]; };    // 0x60

// Targeted explicit instantiations (a whole-class `template class` also emits members the original does not
// have here, which only adds unresolved references for the equivalence harness).
// 0x4101a0 vector<E32>::_Insert_n  (+ 0x410304: catch funclets and cold branches in the same COMDAT)
template void std::vector<E32>::_Insert_n(std::vector<E32>::iterator, size_t, const E32&);
// 0x4103f0 vector<E96>::insert(where, val)
template std::vector<E96>::iterator std::vector<E96>::insert(std::vector<E96>::iterator, const E96&);
// 0x410460, 0x4104b0, 0x410500, 0x4105c0, 0x410860: vector<T>::_Buy(size_type)
template bool std::vector<E96>::_Buy(size_t);
template bool std::vector<E48>::_Buy(size_t);
template bool std::vector<E20>::_Buy(size_t);
template bool std::vector<E32>::_Buy(size_t);
template bool std::vector<E8>::_Buy(size_t);
// 0x410550 vector<E32>::resize(n, val)  (val passed by value on the stack)
template void std::vector<E32>::resize(size_t, E32);
// 0x410610 vector<int>::_Insert_n  (+ 0x410770 funclets/cold branches)
template void std::vector<int>::_Insert_n(std::vector<int>::iterator, size_t, const int&);
