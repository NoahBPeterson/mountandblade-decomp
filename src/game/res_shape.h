// res_shape.h: the body part resource of a prop (name + collision shape; src/rgl/resource.h's body_part): the shape pointer at
// +0x18 and its out-of-line clone / copy. ONE definition for src/game: agent4 (spawn_prop 0x44c210: clone 0x4285e0, copy 0x439100)
// and terrain_b (mission.cpp 0x479da0 spawn_bodies, 0x47a0c0: clone) each saw a partial view. No includes (shape_base is
// src/include/shape.h's).
#ifndef GAME_RES_SHAPE_H
#define GAME_RES_SHAPE_H

struct shape_base;
struct res_shape {
    char pad[0x18];
    shape_base* sh;                                     // +0x18
    shape_base* clone_shape();                          // 0x4285e0
    shape_base* copy_shape();                           // 0x439100
};

#endif // GAME_RES_SHAPE_H
