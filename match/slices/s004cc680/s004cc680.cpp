// Slice s004cc680: out-of-line std::vector<T> copy constructors, copy-assignment operators,
// insert(iterator, const T&) and resize(size_type, const T&) for four element types:
//   tstring (0x10, non-trivial copy ctor 0x42c110 / operator= 0x42bc50 / dtor 0x42bc40)
//   E56     (0x38, non-trivial copy ctor / operator= / dtor)
//   E516    (0x204, trivial)
//   R44     (0x2c, trivial)
// The vectors are VC7.1's <vector> instantiated explicitly; every out-of-line member that the
// original emitted (copy ctor + its catch funclet, operator=, insert, resize) falls out of that.
#include <vector>
#include <memory>
#include <stdlib.h>

// 0x10-byte string type. Its copy ctor / operator= / dtor are out-of-line in the original.
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                            // 0x42bb50
    tstring(const tstring&);              // 0x42c110
    tstring& operator=(const tstring&);   // 0x42bc50
    ~tstring();                           // 0x42bc40
};

// 0x38-byte record: a tstring followed by five raw pointers that a user destructor frees.
struct E56 {
    tstring s;
    int mid[5];
    void* q[5];
    ~E56() { for (int i = 0; i < 5; ++i) free(q[i]); }
};

// 0x204-byte record (trivially copyable / destructible).
struct E516 { int v[0x81]; };

// 0x2c-byte record (trivially copyable / destructible).
struct R44 { int v[11]; };

template class std::vector<tstring>;
template class std::vector<E56>;
template class std::vector<E516>;
template class std::vector<R44>;
