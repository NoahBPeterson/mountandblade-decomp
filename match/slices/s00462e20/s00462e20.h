// Slice s00462e20: shared declarations. ALL NAMES PROVISIONAL.
// - Inv: a "loot / stock" container of item slots (0xb4 array of (item, modifier) pairs, 0x4a4 array of equipped
//   slots) with a random picker (0x462e20).
// - a record reader (0x4631c0), the 0x5374-byte object ctor (0x463370), a 5-array holder (copy ctor/dtor).
#ifndef S00462E20_H
#define S00462E20_H
#include <stddef.h>
#include <new>             // declares operator new[]/delete[]: without it cl calls operator new/delete for POD arrays
#include "rgl.h"
#include "tstring.h"

struct stream_base {
    virtual void reset();                                  // 0
    virtual void rewind();                                 // 1
    virtual stream_base& w_stream(stream_base* o) = 0;     // 2
    virtual stream_base& w_float(float* v) = 0;            // 3
    virtual stream_base& w_bool(char* v) = 0;              // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5
    virtual stream_base& w_i64(__int64* v) = 0;            // 6
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7
    virtual stream_base& w_int(int* v) = 0;                // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9
    virtual stream_base& w_cstr(const char* s) = 0;        // 10
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 11
    virtual stream_base& slot12(void* v) = 0;              // 12
    virtual stream_base& r_stream(stream_base* o) = 0;     // 13
    virtual stream_base& r_float(float* v) = 0;            // 14
    virtual stream_base& r_bool(char* v) = 0;              // 15
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17
    virtual stream_base& r_i64(__int64* v) = 0;            // 18
    virtual stream_base& r_int(int* v) = 0;                // 19
    virtual stream_base& r_byte(char* v) = 0;              // 20
    virtual stream_base& r_tstr(tstring* v) = 0;           // 21
};

struct ItemKind {                                          // 0x238 bytes (array at 0x5e29d0)
    char pad0[0x1c8];
    unsigned type_flags;                                   // +0x1c8 (low byte = item type)
    char pad1cc[0x1d8 - 0x1cc];
    int value;                                             // +0x1d8
    char pad1dc[0x238 - 0x1dc];
};
extern ItemKind* g_items;                                  // 0x5e29d0
extern unsigned g_rand_seed;                               // 0x5d7078

struct Slot { int item; int modifier; };                   // 8 bytes

struct Inv {
    char pad0[0x30];
    __int64 flags;                                         // +0x30 (bit 0x10: fixed stock)
    char pad38[0xb4 - 0x38];
    Slot slots[0x7e];                                      // +0xb4 (count from count())
    Slot equip[16];                                        // +0x4a4
    int count();                                           // 0x462b60
    bool fits(Slot* s, int equip_slot);                    // 0x462c80
    bool matches(int idx, int p3, unsigned must, unsigned mustnot, unsigned type, unsigned t2, unsigned t3);   // 0x462da0
    int pick(int seed, int p3, unsigned must, unsigned mustnot, unsigned type, unsigned t2, unsigned t3, char cheapest);   // 0x462e20
    int find_free();                                       // 0x463030
    bool add(Slot* s);                                     // 0x463070
    void unequip(int i);                                   // 0x4630d0
    bool equip_best(int seed, int equip_slot, unsigned must, unsigned mustnot, unsigned type, unsigned t2, unsigned t3, char cheapest);   // 0x463100
};

// 0x463190: holder of 0x20-byte records at +0xb0 (count at +0x8a8)
struct RecHolder {
    char pad0[0xb0];
    struct Rec { unsigned char flags; char pad[0x1f]; } recs[1];    // +0xb0
    char pad[0x8a8 - 0xd0];
    int nrecs;                                             // +0x8a8
    int first_flagged();                                   // 0x463190
};

// 0x78-byte table of named records (table at 0x5e29dc, count at 0x5e29e0)
struct Rec78 {
    tstring name;                                          // +0
    char pad10[4];
    int f14;                                               // +0x14
    char pad18[0x78 - 0x18];
};
extern Rec78* g_tbl78;                                     // 0x5e29dc
extern int g_tbl78_count;                                  // 0x5e29e0
int find_in_table78(const tstring* name);                  // 0x4352a0 (cdecl)

stream_base& r_rotation(stream_base& s, rgl::matrix* m);   // 0x415e70 (cdecl)

// 0x4631c0: a placed object record read from a stream (sprite-like). Offsets from the reader.
struct Placed {
    char pad0[0x18];
    rgl::matrix m18;                                       // +0x18 (origin at +0x48)
    char pad58[0x98 - 0x58];
    rgl::matrix m98;                                       // +0x98 (copy of m18)
    char padd8[0x158 - 0xd8];
    float f158, f15c, f160;                                // +0x158
    char pad164[0x188 - 0x164];
    int f188, f18c;                                        // +0x188 / +0x18c
    tstring name;                                          // +0x190
    int f1a0, f1a4;                                        // +0x1a0 / +0x1a4
    char pad1a8[0x1bc - 0x1a8];
    float f1bc;                                            // +0x1bc
    void read(stream_base* s, int version);                // 0x4631c0
};

// 0x463760 / 0x4636d0: name + five (count, int array) pairs
struct ArrSet {
    tstring name;                                          // +0
    int cnt[5];                                            // +0x10
    int* arr[5];                                           // +0x24
    ArrSet(const ArrSet& o);                               // 0x4636d0
    ~ArrSet();                                             // 0x463760
};

extern __int64 g_ticks[];                                  // 0x5e2948 clock table (100 kHz ticks)
rgl::matrix* identity_pos(rgl::matrix* out);               // 0x40cd00 (cdecl; returns out)

// 0x463370: constructor of a 0x5374-byte object (a camera / view-state block; fields reached by offset)
struct ViewState {
    void init();                                           // 0x463370
    rgl::vector4 get_dir(int a, int b);                    // 0x463610
};
#endif
