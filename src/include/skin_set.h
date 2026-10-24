// skin_set.h: the skin record table at *0x8ed4f7c (g_skins_begin; g_skins_end 0x8ed4f80), 0x1f8-byte records. ONE definition for
// src/rgl/mesh2 (gui_edit.cpp: the language export walks elems, translated 0x4c6080) and src/game/char_sheet (facegen_init /
// facegen_misc / cam_view: deform-key count, first face, divisor, regen 0x4735e0).
// Offsets: +0x3c count (0x4f4a48, 0x4eb3d0), +0x40 elems (0x4c8ca0 export), +0x88 first_face (0x4f5448: the imul by 0x1f8 and
// the load at +0x88), +0x194 div, regen 0x4735e0 (ret 8).
#ifndef MB_SKIN_SET_H
#define MB_SKIN_SET_H
#include "../game/tstring.h"

struct SkinElem {
    tstring name;                                   // +0x00
    tstring key;                                    // +0x10
    char pad20[0x34 - 0x20];
    tstring translated();                           // 0x4c6080
};
struct SkinSet {
    char pad0[0x3c];
    int n_elems;                                    // +0x3c number of deform keys / language elements
    SkinElem* elems;                                // +0x40
    char pad44[0x88 - 0x44];
    char* first_face;                               // +0x88 (null: no beard/face variants)
    char pad8c[0x194 - 0x8c];
    int div;                                        // +0x194
    char pad198[0x1f8 - 0x198];
    void regen(int arg, float* values);             // 0x4735e0 (thiscall, ret 8)
    void destroy();                                 // 0x4d3720 (element destructor of the 0x1f8-byte skin records; the slice called them Big)
};
extern SkinSet* g_skins_begin;                      // 0x8ed4f7c
extern SkinSet* g_skins_end;                        // 0x8ed4f80

#endif // MB_SKIN_SET_H
