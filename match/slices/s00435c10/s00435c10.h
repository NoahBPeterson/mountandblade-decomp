// Shared declarations of slice s00435c10 (provisional names; evidence in comments).
#ifndef S00435C10_H
#define S00435C10_H
#include "rgl.h"
#include <math.h>
using rgl::vector4;
using rgl::matrix;

// item kind record (0x238 bytes, array at 0x5e29d0), see s004333c0
struct ItemKind {
    char pad0[0x1c8];
    unsigned type;                      // +0x1c8 (low byte = kind)
    char pad1cc[0x208 - 0x1cc];
    float f208;                         // +0x208 (value / weight)
    char pad20c[0x238 - 0x20c];
};
extern ItemKind* g_items;               // 0x5e29d0

// mission object at 0x5e292c (s00432130)
struct Mission { char pad[0x5c]; int f5c; };
extern Mission* g_mission;              // 0x5e292c

// equipped item entry: item id + modifier bits (s00432130)
struct Pair {
    int a, b;
    int value() const;                  // 0x434b70 (item value after the modifier, thiscall)
};

struct Obj_a0 { char pad[0xa0]; vector4 d; };      // +0xa0 direction vector (used by 0x436130)

struct Z_ {
    char pad0[0x46c];
    float f46c, f470, f474;
    char pad478[0xa00 - 0x478];
    float fa00, fa04, fa08;
    char pada0c[0x1170 - 0xa0c];
    vector4 v1170;
};
struct Y_ { char pad[4]; Z_* f4; };
struct X_ { char pad[0x304]; Y_* f304; };
// camera parameter block written by Agent::camera_params
struct CamParams {
    char pad[0x40];
    vector4 v40;                        // +0x40 camera position offset
    vector4 v50;                        // +0x50 target offset
    float f60;                          // +0x60
};

struct Agent {
    char pad0[8];
    int f8;                             // +8
    char padc[0x10 - 0xc];
    int f10;                            // +0x10
    int f14;                            // +0x14
    char pad18[0x2c - 0x18];
    int f2c;                            // +0x2c
    char pad30[0x60 - 0x30];
    vector4 vel;                        // +0x60 (velocity)
    char pad70[0x80 - 0x70];
    float f80;                          // +0x80
    char pad84[0x98 - 0x84];
    vector4 v98;                        // +0x98 (eye offset?)
    char pada8[0xbc - 0xa8];
    Obj_a0* fbc;                        // +0xbc
    char padc0[0xe8 - 0xc0];
    unsigned fe8;                       // +0xe8
    char padec[0x1d0 - 0xec];
    Pair pairs[11];                     // +0x1d0
    int pad228;
    int slots[5];                       // +0x22c (slots[4] at +0x23c)
    char pad240[0x250 - 0x240];
    X_* f250;                    // +0x250 -> +0x304 -> +4 -> Z_
    char pad254[0x55d8 - 0x254];
    float dirx, diry, dirz;             // +0x55d8
    char pad55e4[0x56cc - 0x55e4];
    float f56cc;                        // +0x56cc
    char pad56d0[0x57d8 - 0x56d0];
    char f57d8;                         // +0x57d8
    char pad57d9[0x5830 - 0x57d9];
    int f5830;                          // +0x5830
    void camera_params(CamParams* p);        // 0x435c10
    void steer(float dt);                           // 0x435f10
    Pair best_weapon_slot() const;                  // 0x435fc0
    float equipment_value() const;                  // 0x436050
    void update_pitch(float dt);                    // 0x436130
};
#endif
