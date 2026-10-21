// Slice s004108b0: VC7.1 <vector> instantiations (continued from s00410080): _Insert_n for the 48-, 8- and
// 100-byte elements with their catch funclets/cold tails (0x410a57, 0x410cc6, 0x410f6a), and the copy
// constructor of vector<E8> with its catch(...) { _Tidy(); throw; } funclet (0x411139). Built with /EHa.
#include <vector>
#include <memory>

// Element types (sizes from the instantiations; field meaning unknown).
struct E8   { int v[2]; };     // 0x08
struct E20  { int v[5]; };     // 0x14
struct E32  { int v[8]; };     // 0x20
struct E48  { int v[12]; };    // 0x30
struct E96  { int v[24]; };    // 0x60
struct E100 { int v[25]; };    // 0x64

// 0x4108b0 vector<E48>::_Insert_n (+ 0x410a57 funclets/cold branches)
template void std::vector<E48>::_Insert_n(std::vector<E48>::iterator, size_t, const E48&);
// 0x410b60 vector<E8>::_Insert_n (+ 0x410cc6)
template void std::vector<E8>::_Insert_n(std::vector<E8>::iterator, size_t, const E8&);
// 0x410dc0 vector<E100>::_Insert_n (+ 0x410f6a)
template void std::vector<E100>::_Insert_n(std::vector<E100>::iterator, size_t, const E100&);
// 0x411080 vector<E8> copy constructor (+ 0x411139 catch funclet)
template std::vector<E8>::vector(const std::vector<E8>&);
