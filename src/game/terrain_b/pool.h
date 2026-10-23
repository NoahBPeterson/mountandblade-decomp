// pool.h: the id pool (pages of 16 records, a free-id list) behind the mission's agent table (Pool<MAgent>, 0x28
// bytes at Mission +4) and the 0xb0-byte record pool (Pool<RecJ> at Mission +0x30). reset() 0x477f00 / 0x477f90 and
// alloc() 0x478020 are explicit specialisations in terrain_b.cpp; next_used 0x474300 (RecJ) and 0x43b160 (MAgent).
#ifndef TERRAIN_B_POOL_H
#define TERRAIN_B_POOL_H
#include <vector>

template<class T> struct Pool {
    int next_id;                                // +0x00 (the table's record count)
    std::vector<int> free_ids;                  // +0x04
    std::vector<T*> pages;                      // +0x14 (first at +0x18; record i is pages[i >> 4][i & 15])
    int serial;                                 // +0x24
    bool valid(int i) { return i < next_id; }
    void next_used(unsigned* i);                // 0x474300 (RecJ); the MAgent instance is declared in mission.h (0x43b160)
    __forceinline unsigned first_used()         // 0x43b1b0 (inlined here)
    {
        unsigned i = 0;
        if (0 < next_id) {
            do {
                if (pages[i >> 4][i & 0xf].used != 0) break;
                i++;
            } while ((int)i < next_id);
        }
        return i;
    }
    void reset();
    int alloc();
};

// 0xb0-byte record (the slices' RecJ / PoolElem): +4 in-use flag, +8 id, +0xc serial number.
struct RecJ {
    int f0, used, id, serial;
    char pad[0xb0 - 0x10];
    RecJ();                                     // 0x4377d0
};

#endif // TERRAIN_B_POOL_H
