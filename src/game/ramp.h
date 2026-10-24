// ramp.h: the colour-ramp / material record (0xb0 bytes; array MatRec* at EChar+0x1c0, character owner, mesh_slot) whose
// tail is the ramp that Ramp::eval 0x463790 samples (n stops of three floats at +0x2c, count at +0xac). ONE definition for
// src/game: terrain_a/game_state.h saw the ramp tail as `Ramp`, mesh_slot/mesh_slot.h the whole record as `MatRec`
// (eval 0x463790 takes this = the record: its first field read is +0xac). No includes (skin is forward-declared).
#ifndef GAME_RAMP_H
#define GAME_RAMP_H

struct skin;
struct Ramp {
    char pad0[0x10];
    skin* res0;                  // 0x10 (mesh_slot/mesh_char.cpp)
    skin* res1;                  // 0x14
    char pad18[0x20 - 0x18];
    int* vals;                   // 0x20
    int* vals_end;               // 0x24
    char pad28[0x2c - 0x28];
    struct Stop { float c[3]; float pad; } stops[8];    // +0x2c (0x463790)
    int n;                       // +0xac (0x463790)
    unsigned eval(float* t);     // 0x463790
};
typedef char ramp_size_check[(sizeof(Ramp) == 0xb0) ? 1 : -1];

#endif // GAME_RAMP_H
