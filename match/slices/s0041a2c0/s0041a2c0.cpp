// Slice s0041a2c0: std::vector<T>::_Insert_n for a 4-byte scalar, the 12-byte vec_array and the 8-byte E8
// record; std::vector<E44>::_Buy; and the destructor / vector deleting destructor of a 0xdc-byte resource.
// The vector code is VC7.1's <vector> instantiated explicitly, built with the plain flags (no EH switch): the
// STL's explicit try/catch keeps its frame and funclets, but a local with a destructor (the _Tmp copy in
// vector<vec_array>::_Insert_n) is not registered for unwinding, exactly as in the original.
#include <vector>
#include <memory>

struct E8 { int v[2]; };        // 8-byte record (same type as s0040f9a0 / s00419420)
struct E44 { int v[11]; };      // 0x2c-byte record

// 12-byte record: parallel arrays (see s00417850's vec_array). Implicit copy ctor, user destructor.
struct vec16 { float x, y, z, w; };
struct vec_array {
    vec16* v;
    int* idx;
    int n;
    ~vec_array() { if (v) delete[] v; if (idx) delete[] idx; }
    void operator=(const vec_array& o);                              // 0x417850
};

template class std::vector<int>;
template class std::vector<E8>;
template class std::vector<E44>;
template class std::vector<vec_array>;

// rgl string with a read cursor (16 bytes; see mbstream.h): only its destructor is used here.
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    ~tstring();                                // 0x42bc40
};

// 0xdc-byte resource record (provisional): a string at +0x8c and a pointer vector at +0xcc.
struct res_dc {
    char pad0[0x8c];
    tstring name;                              // 0x8c
    char pad1[0xcc - 0x9c];
    std::vector<int> items;                    // 0xcc
    ~res_dc();                                 // 0x41a510
};

// @ 0x41a510
res_dc::~res_dc() {}

// @ 0x41a550
void destroy_res_dc_array(res_dc* p) { delete[] p; }
