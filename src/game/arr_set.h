// arr_set.h: ArrSet, a name and five (count, int array) pairs (0x38 bytes), and the allocator stub that constructs
// one. Copy ctor 0x4636d0 and dtor 0x463760 are the COMDAT copies of src/game/terrain_a/game_state.cpp (the first
// object that has them, so the linker keeps those); src/game/terrain_b/terrain_b.cpp sees the dtor as an inline
// function and expands it inside its std::vector<ArrSet> instantiations (_Destroy_range, erase, _Insert_n, resize).
// Both views are one class: game_state.cpp and terrain_a.cpp define ARRSET_DTOR_OUT_OF_LINE before including
// this header (terrain_a.cpp only calls 0x463760; game_state.cpp defines it), terrain_b does not.
#ifndef GAME_ARR_SET_H
#define GAME_ARR_SET_H
#include "./tstring.h"

struct ArrSet {
    tstring name;                               // +0
    int cnt[5];                                 // +0x10
    int* arr[5];                                // +0x24
    ArrSet() { for (int i = 0; i < 5; i++) { arr[i] = 0; cnt[i] = 0; } }   // inline (vector<ArrSet>::resize(n) in 0x47de60)
    ArrSet(const ArrSet& o);                    // 0x4636d0
#ifdef ARRSET_DTOR_OUT_OF_LINE
    ~ArrSet();                                  // 0x463760
#else
    ~ArrSet() { for (int i = 0; i < 5; i++) if (arr[i]) delete[] arr[i]; }       // 0x463760
#endif
};

// the allocator of std::vector<ArrSet>: construct is 0x46b6c0 (terrain_a.cpp), terrain_b only names the type
struct allocArr { void construct(ArrSet* p, const ArrSet& v); };           // 0x46b6c0  // old name of construct_arrset, removed after the names merge

#endif // GAME_ARR_SET_H
