// ptype.h: the particle / ground-spec type record (table g_ptypes at *0x5e28f8, count 0x5e29e8, 0x20 bytes each) and the
// 0x48-byte box record each of its frames points to. ONE definition for src/game: terrain_a (terrain.h), terrain_b
// (mission.cpp spawn_bodies 0x479da0) and char_sheet (dust.h, update_particles 0x4f3e10 / emit_cell 0x4f4060) saw it as
// partial views. The header needs only tstring; the mesh and resource types are forward-declared so that both header
// universes (rgl_math.h and rgl.h) can include it.
#ifndef GAME_PTYPE_H
#define GAME_PTYPE_H
#include "./tstring.h"

struct res_shape;
struct AnimBox {                                // 0x48 (one frame / element of a type)
    char pad0[0x14];
    void** meshes;                              // +0x14 the frame's mesh list (char_sheet dust.cpp 0x4f3e10: dmesh**)
    char pad18[0x20 - 0x18];
    res_shape* res;                             // +0x20 (terrain_b 0x479da0)
    char pad24[4];
    float mn[3];                                // +0x28
    float pad34;
    float mx[3];                                // +0x38
    char pad44[0x48 - 0x44];
};
struct ptype {
    tstring name;                               // +0 (0x4650f0)
    __int64 flags;                              // +0x10 (0x400000 / 0x80000 / 0x1000000 / 0x20000 ...; the high dword's low 16 bits are a density)
    int nframes;                                // +0x18 (0x466cd0, 0x46a270; terrain_b 0x479da0 and dust 0x4f3e10: count of boxes)
    union {
        int f1c;                                // +0x1c
        AnimBox* boxes;                         // +0x1c: nframes boxes of 0x48 bytes
    };
};
extern ptype* g_ptypes;                         // 0x5e28f8
extern int g_ptype_count;                       // 0x5e29e8

#endif // GAME_PTYPE_H
