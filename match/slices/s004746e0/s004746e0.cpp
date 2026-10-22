// Slice s004746e0: face generator "breed": random blend of two parent faces' genes (provisional names).
#include <math.h>

struct IntVec { int pad; int* first; int* last; int* endcap; };   // VC7.1 std::vector<int> layout
struct FaceHairRec {            // 0xb0 bytes
    char pad0[0x1c];
    IntVec list;                // +0x1c candidate values
    char pad2c[0xb0 - 0x2c];
};
struct FaceRaceRec {            // 0x1f8 bytes; the record list sits behind a pointer at +0x1c0
    char pad0[0x1c0];
    FaceHairRec* recs;
    char pad1c4[0x1f8 - 0x1c4];
    void apply(int key, float* morphs);                     // 0x4735e0
};
struct FaceGen {
    int   f0;           // +0x00
    int   f1;           // +0x04
    int   race;         // +0x08
    int   f3;           // +0x0c
    int   f4;           // +0x10
    int   f5;           // +0x14  hair record index
    int   f6;           // +0x18
    char* tbl;          // +0x1c  race table
    char  pad20[0x2c - 0x20];
    float morph[63];    // +0x2c  7 groups of 9
    float age;          // +0x128
    float age2;         // +0x12c

    void load(int which);                                   // 0x473800
    void breed(int a, int b, int seed);                     // 0x4746e0
};
extern unsigned g_rand_seed;                                // 0x5d7078

#define RAND() (g_rand_seed = (g_rand_seed * 0x1c13 - 0xd098556) * 0x1105)
#define RND01() ((float)(RAND() % 0x3dc9) * 6.322312e-05f)

static __forceinline int round_int(const float& x)
{
    float fl = floor(x);
    int r = (int)fl;
    return (x - fl <= 0.5f) ? r : r + 1;
}
static __forceinline int lerp_int(int a, const int& b)
{
    float t = RND01() * (b - a); return a + round_int(t);
}

#define MIX(k) { RAND(); float lo = save[i + k]; morph[i + k] = (morph[i + k] - lo) * ((float)(RAND() % 0x3dc9) * 0.00012644623f) * 0.5f + lo; }

// @ 0x4746e0
void FaceGen::breed(int a, int b, int seed)
{
    g_rand_seed = (seed + 0x8a767) * 0x1ddcf5f + 0x2f784719;
    load(a);
    int s3 = f3; float a0 = age; float a1 = age2;
    int s1 = f1; int s0 = f0; int s5 = f5;
    float save[63];
    int i;
    for (i = 0; i < 63; i += 9) {
        save[i] = morph[i]; save[i+1] = morph[i+1]; save[i+2] = morph[i+2];
        save[i+3] = morph[i+3]; save[i+4] = morph[i+4]; save[i+5] = morph[i+5];
        save[i+6] = morph[i+6]; save[i+7] = morph[i+7]; save[i+8] = morph[i+8];
    }
    load(b);
    f1 = lerp_int(s1, f1);
    f0 = lerp_int(s0, f0);
    f5 = lerp_int(s5, f5);
    f3 = -1;
    FaceHairRec* r = &((FaceRaceRec*)(tbl + race * 0x1f8))->recs[f5];
    IntVec* v = &r->list;
    int n;
    if (v->first == 0 || (n = (int)(v->last - v->first)) <= 0) {
        f3 = lerp_int(s3, f3);
    } else {
        RAND();
        f3 = ((FaceRaceRec*)(tbl + race * 0x1f8))->recs[f5].list.first[g_rand_seed % (unsigned)n];
    }
    age = (float)(RAND() % 0x3dc9) * (age - a0) * 6.322312e-05f + a0;
    age2 = (float)(RAND() % 0x3dc9) * (age2 - a1) * 6.322312e-05f + a1;
    for (i = 0; i < 63; i += 9) {
        MIX(0) MIX(1) MIX(2) MIX(3) MIX(4) MIX(5) MIX(6) MIX(7) MIX(8)
    }
    ((FaceRaceRec*)(tbl + race * 0x1f8))->apply(-1, morph);
}
