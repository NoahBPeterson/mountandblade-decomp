// face_gen.h: the face generator's random seed. The race table is include/skin_set.h (SkinSet, 0x1f8 bytes per race: morph
// count, the morph constraints and their relaxation, the pick counts, the hair records); the face record that unpacks a
// 3 x 64-bit key into picks and morph values (0x473800) and blends two parents (breed 0x4746e0) is game/animator.h's
// Animator (g_anim): set / item / cur / variant / material are the picks and the race index, weights the morph values,
// scale and time the weight and age.
#ifndef TERRAIN_B_FACE_GEN_H
#define TERRAIN_B_FACE_GEN_H

extern unsigned g_rand_seed;                    // 0x5d7078

#endif // TERRAIN_B_FACE_GEN_H
