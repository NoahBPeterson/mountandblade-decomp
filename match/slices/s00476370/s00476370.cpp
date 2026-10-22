// Slice s00476370: VC7.1 <vector> instantiations: _Insert_n for the 36-, 24- and 100-byte POD elements
// (with their catch(...) funclets and cold branches at 0x476515, 0x4767de, 0x476a9a). Built with /EHa.
#include <vector>
#include <memory>

struct E24  { int v[6]; };     // 0x18
struct E36  { int v[9]; };     // 0x24
struct E100 { int v[25]; };    // 0x64

// @ 0x476370 (+ 0x476515: catch funclet and cold branches)
template void std::vector<E36>::_Insert_n(std::vector<E36>::iterator, size_t, const E36&);
// @ 0x476620 (+ 0x4767de)
template void std::vector<E24>::_Insert_n(std::vector<E24>::iterator, size_t, const E24&);
// @ 0x4768f0 (+ 0x476a9a)
template void std::vector<E100>::_Insert_n(std::vector<E100>::iterator, size_t, const E100&);
