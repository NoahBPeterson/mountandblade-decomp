// s004cd010.cpp — slice 115
// Out-of-line members of std::vector<E50>, std::vector<E68>, std::vector<E176>,
// std::vector<scalar4> (element copy ctor / operator= / _Tidy / _Destroy_range /
// _Copy_opt / copy_backward), plus two game constructors.

#include <vector>
#include <memory>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------- tstring
struct tstring {                            // 0x10
    int pos;                                // 0x00
    char* buf;                              // 0x04
    int cap;                                // 0x08
    int len;                                // 0x0c
    tstring();                              // 0x42bbd0
    tstring(const tstring&);                // 0x42c110
    tstring& operator=(const tstring&);     // 0x42bc50
    ~tstring();                             // 0x42bc40
    void assign(const char*);               // 0x42bcb0
};

// ---------------------------------------------------------------- elements
struct scalar4 { int v; };                  // 4-byte trivial element
struct R44 { int v[11]; };                  // 0x2c trivial element
struct b16 { int v[4]; };                   // 0x10 POD block

struct Block { void free_materials(); };    // 0x4c8330 (empty base)

struct M28 {                                // 0x1c member of E68
    char* p;                                // 0x00
    int n;                                  // 0x04
    char c;                                 // 0x08
    tstring s;                              // 0x0c
    M28() {}                                // needed by vector<E68>(size_type)
    M28(const M28&);                        // 0x4bd7a0
    ~M28() { if (p) delete[] p; }
    M28& operator=(const M28& o)
        { p = o.p; n = o.n; c = o.c; s = o.s; return *this; }
};

struct E50 {                                // 0x50
    tstring s;                              // 0x00
    int x0, x1, x2, x3;                     // 0x10
    b16 y;                                  // 0x20
    b16 z;                                  // 0x30
    std::vector<R44> v;                     // 0x40
};

struct E68 : Block {                        // 0x68
    tstring s;                              // 0x00
    int b0, b1, b2, b3, b4;                 // 0x10
    int b5, b6, b7, b8, b9;                 // 0x24
    M28 m;                                  // 0x38
    std::vector<scalar4> v;                 // 0x54
    int t;                                  // 0x64
    ~E68() { free_materials(); }
};

struct E176 {                               // 0xb0
    tstring s;                              // 0x00
    int a;                                  // 0x10
    int b;                                  // 0x14
    int c;                                  // 0x18
    std::vector<int> v;                     // 0x1c
    b16 big[8];                             // 0x2c
    int t;                                  // 0xac
};

template class std::vector<scalar4>;
template class std::vector<E50>;
template class std::vector<E68>;
template class std::vector<E176>;

// ---------------------------------------------------------------- game ctor 0x4cd720
struct U4c {                                // 0x4c repeated sub-object
    int a;                                  // 0x00
    int p04;                                // 0x04 (uninitialised)
    int p08;                                // 0x08 (uninitialised)
    int p0c;                                // 0x0c (uninitialised)
    int d;                                  // 0x10
    int e;                                  // 0x14
    tstring s;                              // 0x18
    int f0;                                 // 0x28
    int f1;                                 // 0x2c
    int g;                                  // 0x30
    int p34;                                // 0x34 (uninitialised)
    int h0;                                 // 0x38
    int h1;                                 // 0x3c
    int h2;                                 // 0x40
    int p44;                                // 0x44 (uninitialised)
    int i1;                                 // 0x48
    U4c() {
        h0 = 0; h1 = 0; h2 = 0;
        a = -1;
        g = 1;
        f0 = 0; f1 = 0;
        d = 0; e = 1;
        i1 = 0;
        s.assign("unknown buffer pair");    // 0x59607c
    }
};

struct Obj720 {
    U4c u[2];
    Obj720();
};
// @ 0x4cd720
Obj720::Obj720() { u[0].g = 0; u[1].g = 1; }

// ---------------------------------------------------------------- game ctor 0x4cd7a0
struct vec4f {
    float x, y, z, w;
    vec4f() {}
    vec4f(float a, float b, float c) : x(a), y(b), z(c) {}
};

struct Obj7a0 {
    tstring s;                              // 0x00
    float f10;                              // 0x10
    float f14;                              // 0x14
    float f18;                              // 0x18
    float f1c;                              // 0x1c
    vec4f v20;                              // 0x20
    vec4f v30;                              // 0x30
    int i40;                                // 0x40 (uninitialised)
    void* p44;                              // 0x44
    void* p48;                              // 0x48
    void* p4c;                              // 0x4c
    Obj7a0();
};
// @ 0x4cd7a0
Obj7a0::Obj7a0() {
    p44 = 0; p48 = 0; p4c = 0;
    f18 = 1.0f; f1c = 1.0f;
    v30 = vec4f(0, 0, 0);
    v20 = vec4f(0, 0, 0);
    f10 = 2.0f; f14 = -0.01f;
}
