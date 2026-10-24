// model_b.h: the 0x1f8-byte model record (constructor 0x4d3640, destructor 0x4d3720; relax 0x4735e0, read 0x4e7b30). One
// definition for options_panel/ragdoll_models.cpp (the constructor, with the field defaults) and inventory_panel/widget_text.cpp
// (vector<model_b>::resize 0x50d170 builds and copies elements of it). Same class by: both name the constructor 0x4d3640, the
// tstrings at +0 and +0x28 and the vector at +0x78 (widget_text's morph_first/last at +0x7c/+0x80 are its first/last).
#ifndef GAME_MODEL_B_H
#define GAME_MODEL_B_H
#include "tstring.h"

// the engine's growable array: allocator slot, then begin/last/end (begin at +4); the ctor clears the pointers
template<class T> struct vec {
    char al;                                        // +0
    T* b;                                           // +4
    T* e;                                           // +8
    T* cap;                                         // +0xc
    vec() : b(0), e(0), cap(0) {}
    unsigned size() const { return b ? (unsigned)(e - b) : 0u; }
    T& operator[](unsigned i) { return b[i]; }
};

struct model_b {
    tstring a;                                      // +0 name
    char pad10[4];
    int f14, f18, f1c, f20, f24;                    // +0x14
    tstring b;                                      // +0x28 texture name
    char pad38[4];
    int f3c, f40, f44;                              // +0x3c
    vec<int> v48;                                   // +0x48
    vec<int> v58;                                   // +0x58
    vec<int> v68;                                   // +0x68
    vec<int> v78;                                   // +0x78 (first +0x7c, last +0x80: the morph list)
    char pad88[0x19c - 0x88];
    vec<int> v19c;                                  // +0x19c
    vec<int> v1ac;                                  // +0x1ac
    vec<int> v1bc;                                  // +0x1bc
    int neg[7];                                     // +0x1cc
    int f1e8;                                       // +0x1e8
    float f1ec;                                     // +0x1ec
    int f1f0, f1f4;                                 // +0x1f0
    model_b();                                      // 0x4d3640
    ~model_b();                                     // 0x4d3720
    void relax(int skip, float* v);                 // 0x4735e0
    void read(void* stream);                        // 0x4e7b30
};
#endif
