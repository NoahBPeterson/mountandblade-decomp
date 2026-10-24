#ifndef SAVE_PANEL_H
#define SAVE_PANEL_H
// Slice s004ea0c0: skin/bone rendering helpers of the save/load screen and the save-slot file logic.
//   0x4ea0c0  draw the skinned model of a party/troop preview (scaled per bone)
//   0x4ea290  save/load screen update (button handling, save, delete confirmation)
//   0x4ea850  select a save slot (show its two panels, hide the others)
// Names provisional; layouts follow the verified neighbouring slices.
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- engine string (16 bytes) ---------------------------------------------------------------------------
#include "../tstring.h"

// ---- matrices / mesh (rgl) ------------------------------------------------------------------------------
#include "../../include/mat4.h"
using rgl::mat3;
using rgl::mat4;

mat3* identity3(mat3* out);                             // 0x40cd60 (cdecl; returns out)

struct vertex60;
#include "../rgl_mesh.h"
using rgl::mesh;
struct skin_rec {
    mesh* m;                                            // +0
    char pad4[0xcc - 4];
    mat4 mat;                                           // +0xcc
};
struct skin_data {
    char pad0[0x11c];
    skin_rec** begin;                                   // +0x11c
    skin_rec** end;                                     // +0x120
};
struct skin_owner {
    char pad0[0x158];
    float f158;                                         // +0x158
    float f15c;                                         // +0x15c
    float f160;                                         // +0x160
    char pad164[0x1b4 - 0x164];
    skin_data* skins;                                   // +0x1b4
};

#endif
