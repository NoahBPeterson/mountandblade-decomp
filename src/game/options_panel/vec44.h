// vec44.h: the growable array of 0x2c-byte records (VC7.1 std::vector layout: allocator word, begin, end, capacity end) whose
// push_back is defined in mission_items.cpp (0x4cefe0) and called by the ragdoll bone loader (ragdoll_models.cpp: RagBone::bodies).
#ifndef OPTIONS_PANEL_VEC44_H
#define OPTIONS_PANEL_VEC44_H
#include <stddef.h>

struct vec44 {
    char pad0[4];
    char* begin;                                                // +0x04
    char* end;                                                  // +0x08
    char* cap;                                                  // +0x0c
    int size() { if (!begin) return 0; return (int)((end - begin) / (ptrdiff_t)0x2c); }
    unsigned capacity() { if (!begin) return 0; return (unsigned)((cap - begin) / (ptrdiff_t)0x2c); }
    void push_back(char* v);                                    // 0x4cefe0 (ret 4)
};

#endif
