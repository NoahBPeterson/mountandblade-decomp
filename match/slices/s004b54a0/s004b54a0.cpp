// Slice s004b54a0: VC7.1 <vector> instantiations for a 92-byte record (0x5c, the agent action/event record
// pushed at agent+0x13a60 by 0x4b5fe0/0x4b6070) and a 12-byte record (0xc): _Insert_n with its catch
// funclets, insert(iterator, const T&) and push_back(const T&). Built with /EHa (the /EHsc variant drops the
// EH frame that the original keeps, as in s0040f9a0/s004108b0).
#include <vector>
#include <memory>

// Element types (sizes from the instantiations; field meaning unknown here).
struct E92 { int v[23]; ~E92() {} };     // 0x5c (emptily destructible: the original's _Tidy keeps a loop)
struct E12 { int v[3]; };       // 0x0c

// 0x4b54a0 vector<E92>::_Insert_n (+ 0x4b566f catch funclet and cold branches)
template void std::vector<E92>::_Insert_n(std::vector<E92>::iterator, size_t, const E92&);
// 0x4b5790 vector<E12>::_Insert_n (+ 0x4b5937 catch funclet and cold branches)
template void std::vector<E12>::_Insert_n(std::vector<E12>::iterator, size_t, const E12&);

// 0x4b5a40 vector<E92>::insert(iterator, const E92&)
template std::vector<E92>::iterator std::vector<E92>::insert(std::vector<E92>::iterator, const E92&);
// 0x4b5ab0 vector<E12>::insert(iterator, const E12&)
template std::vector<E12>::iterator std::vector<E12>::insert(std::vector<E12>::iterator, const E12&);
// 0x4b5b20 vector<E92>::push_back(const E92&)
template void std::vector<E92>::push_back(const E92&);
// 0x4b5bb0 vector<E12>::push_back(const E12&)
template void std::vector<E12>::push_back(const E12&);
