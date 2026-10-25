// face_gen.h: the face generator: the race table (FaceRace, 0x1f8 bytes per race: morph count, the morph
// constraints and their relaxation, the pick counts, the hair records) and the face record (FaceGenome) that unpacks
// a 3 x 64-bit key into picks and morph values (0x473800) and blends two parents (breed 0x4746e0).
// The slices called FaceRace MorphSolver / FaceRaceRec, FaceGenome's pick fields f0 f1 f3 f5 and its age fields age /
// age2 (this header uses the unpack names: weight +0x128, age +0x12c).
#ifndef TERRAIN_B_FACE_GEN_H
#define TERRAIN_B_FACE_GEN_H

struct Term { float coef; int idx; };           // 8 bytes
struct Constraint {                             // 0x204 bytes
    int n;                                      // +0x000 number of terms
    Term t[63];                                 // +0x004
    int mode;                                   // +0x1fc -1: value must stay >= limit, +1: must stay <= limit
    float limit;                                // +0x200
};
#include "../int_vec.h"
struct FaceHairRec {                            // 0xb0 bytes
    char pad0[0x1c];
    IntVec list;                                // +0x1c candidate values (0x4746e0)
    char pad2c[0xb0 - 0x2c];
};
struct FaceRace {                               // 0x1f8 bytes, table at FaceGenome +0x1c
    char pad0[0x3c];
    int nmorph;                                 // +0x3c (0x473800)
    char pad40[0x78 - 0x40];
    int alloc;                                  // +0x78 (std::vector allocator slot)
    Constraint* first;                          // +0x7c morph constraints (0x4735e0)
    Constraint* last;                           // +0x80
    char pad84[4];
    int n_a;                                    // +0x88
    char pad8c[0x18c - 0x8c];
    int n_b;                                    // +0x18c
    int n_c;                                    // +0x190
    int n_d;                                    // +0x194
    char pad198[0x1c0 - 0x198];
    FaceHairRec* recs;                          // +0x1c0 (0x4746e0)
    char pad1c4[0x1f8 - 0x1c4];
    int size() const { return first == 0 ? 0 : (int)(last - first); }   // vector<Constraint>::size (cf. 0x467b20)
    void relax(int skip, float* v);             // 0x4735e0
};
struct FaceGenome {
    int pick_a;                                 // +0x00 (key bits 6..11, minus 1)
    int pick_b;                                 // +0x04 (key bits 0..5, minus 1)
    int race;                                   // +0x08
    int pick_d;                                 // +0x0c (bits 18..23)
    int pad10;
    int pick_c;                                 // +0x14 (bits 12..17; the hair record index)
    char pad18[4];
    FaceRace* races;                            // +0x1c
    char pad20[0x2c - 0x20];
    float morph[63];                            // +0x2c 7 groups of 9
    float weight;                               // +0x128
    float age;                                  // +0x12c
    FaceRace* unpack(const unsigned __int64* key);   // 0x473800 (returns the race record, left in EAX)
    void breed(const unsigned __int64* a, const unsigned __int64* b, int seed);   // 0x4746e0
};
extern unsigned g_mask64;                       // 0x5a8e98 (64)
extern unsigned g_mask8;                        // 0x5a8e80 (8)
extern unsigned g_rand_seed;                    // 0x5d7078

// breed calls unpack and relax through the same addresses from outside their definitions
struct FaceGenExt {
    __forceinline FaceRace* unpack(const unsigned __int64* key) { return ((FaceGenome*)this)->unpack(key); }   // 0x473800
};
struct FaceRaceExt {
    __forceinline void relax(int skip, float* v) { ((FaceRace*)this)->relax(skip, v); }   // 0x4735e0
};

#endif // TERRAIN_B_FACE_GEN_H
