// skin_set.h: the skin record table at *0x8ed4f7c (g_anim.skin_vec.first; g_anim.skin_vec.last 0x8ed4f80), 0x1f8-byte records. ONE definition for
// src/rgl/mesh2 (gui_edit.cpp: the language export walks elems, translated 0x4c6080) and src/game/char_sheet (facegen_init /
// facegen_misc / cam_view: deform-key count, first face, divisor, regen 0x4735e0).
// Offsets: +0x3c count (0x4f4a48, 0x4eb3d0), +0x40 elems (0x4c8ca0 export), +0x88 first_face (0x4f5448: the imul by 0x1f8 and
// the load at +0x88), +0x194 div, regen 0x4735e0 (ret 8).
#ifndef MB_SKIN_SET_H
#define MB_SKIN_SET_H
#include "../game/tstring.h"
#include "../game/int_vec.h"   // IntVec (FaceHairRec::list)

struct EChar;
struct SkinSet;
// the skin vector of the Animator (g_anim +0x18): a VC7.1 std::vector<SkinSet> (allocator word, first, last, end of storage), its
// destructor the atexit function 0x548510 reaches. The mesh code reads the same first pointer as an array of EChar.
struct SkinVec {
    int alloc;                                      // +0 the allocator word
    union {
        SkinSet* first;                             // +4 the face-gen / language-export view: 0x1f8-byte sets
        EChar* chars;                               // +4 the mesh_slot view
    };
    SkinSet* last;                                  // +8
    SkinSet* end;                                   // +0xc end of storage
    ~SkinVec();                                     // 0x4eb810 (game/char_sheet/facegen_misc.cpp): destroys the sets, frees the storage
};
struct SkinElem {
    tstring name;                                   // +0x00
    tstring key;                                    // +0x10
    char pad20[0x34 - 0x20];
    tstring translated();                           // 0x4c6080
};
// the face generator part of a race record (terrain_b.cpp: SkinSet::relax 0x4735e0, Animator::unpack 0x473800, Animator::breed 0x4746e0)
struct Term { float coef; int idx; };           // 8 bytes
struct Constraint {                             // 0x204 bytes
    int n;                                      // +0x000 number of terms
    Term t[63];                                 // +0x004
    int mode;                                   // +0x1fc -1: value must stay >= limit, +1: must stay <= limit
    float limit;                                // +0x200
};
struct FaceHairRec {                            // 0xb0 bytes
    char pad0[0x1c];
    IntVec list;                                // +0x1c candidate values (0x4746e0)
    char pad2c[0xb0 - 0x2c];
};
struct SkinSet {                                // 0x1f8 bytes: one race (the table is g_anim.skin_vec)
    char pad0[0x3c];
    int n_elems;                                // +0x3c number of deform keys / language elements (the morph count of unpack 0x473800)
    SkinElem* elems;                            // +0x40
    char pad44[0x78 - 0x44];
    int alloc;                                  // +0x78 (std::vector allocator slot)
    Constraint* first;                          // +0x7c morph constraints (0x4735e0)
    Constraint* last;                           // +0x80
    char pad84[4];
    union {
        char* first_face;                       // +0x88 (null: no beard/face variants)
        int n_a;                                // +0x88 number of picks (unpack 0x473800)
    };
    char pad8c[0x18c - 0x8c];
    int n_b;                                    // +0x18c
    int n_c;                                    // +0x190
    union {
        int div;                                // +0x194
        int n_d;                                // +0x194
    };
    char pad198[0x1c0 - 0x198];
    FaceHairRec* recs;                          // +0x1c0 (0x4746e0)
    char pad1c4[0x1f8 - 0x1c4];
    int size() const { return first == 0 ? 0 : (int)(last - first); }   // vector<Constraint>::size (cf. 0x467b20)
    void relax(int skip, float* v);             // 0x4735e0 (thiscall, ret 8)
    void destroy();                             // 0x4d3720 (element destructor of the 0x1f8-byte skin records; the slice called them Big)
};
typedef char skin_set_size_check[sizeof(SkinSet) == 0x1f8 ? 1 : -1];

#endif // MB_SKIN_SET_H
