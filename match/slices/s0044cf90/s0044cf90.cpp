// Slice s0044cf90: vector<E56>::insert(where, val) (VC7.1 <vector>; iterator returned through a hidden pointer).
#include <vector>
#include <memory>
#include <new>

// 0x38-byte record: see s0043edf0 / s0043e514.
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o);         // 0x43f510
};

// @ 0x44cf90
template std::vector<E56>::iterator std::vector<E56>::insert(std::vector<E56>::iterator, const E56&);
// 0x445ea0
template void std::vector<E56>::_Insert_n(std::vector<E56>::iterator, size_t, const E56&);
