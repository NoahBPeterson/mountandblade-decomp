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

// The 0xb0-byte record of Pool<RecJ> is the shared RecJ (src/include/recj.h): IfaceB's f4 is the in-use flag, f8 the id, fc the serial.
#include "../../include/recj.h"

#endif // TERRAIN_B_POOL_H
