// animator.h: the character-builder / animation state (global g_anim at 0x8ed4f60). ONE definition for mesh_slot
// (mesh_char / mesh_terrain / mesh_appear), mission5 (troop_rec: reset), char_sheet (facegen_misc, facegen_init, cam_view)
// and inventory_panel (view_open). The universes of those TUs differ (rgl_math.h / rgl.h), so this header names no engine
// type: rgl::lod_mesh, EChar and SkinSet are forward declarations, and the PackedState it fills is defined here.
// Offsets: 0x473800 set_anim_a, 0x4746e0 set_anim_b, 0x47fb10 collect (mesh_char.cpp), 0x4eb320 randomise and 0x4eb660
// pack_state (facegen_misc.cpp), 0x520520 reset (mission5/troop_rec.cpp). The face-generator window (0x4eb270) writes the
// first words from its sliders (set = skin-1, item = hair-1, variant = beard % div, material = colour).
#ifndef MB_GAME_ANIMATOR_H
#define MB_GAME_ANIMATOR_H
#include <stddef.h>

namespace rgl { struct lod_mesh; }
struct EChar;
struct SkinSet;

struct PackedState { unsigned __int64 a, b, c, d; };    // the 32 bytes at troop record +0x3c0 (pack_state 0x4eb660)

struct Animator {
    int set;                     // 0x00 (facegen: f350 - 1)
    int item;                    // 0x04 (facegen: f354 - 1)
    int cur;                     // 0x08 (g_cur, 0x8ed4f68: the skin set index; 0x4eb660 / 0x4eb320 scale it by 0x1f8)
    int variant;                 // 0x0c
    int f10;                     // 0x10 (facegen: the same value as variant)
    int material;                // 0x14
    char pad18[4];
    union {
        EChar* chars;            // 0x1c (mesh_slot view)
        SkinSet* skins;          // 0x1c (g_skins_begin, 0x8ed4f7c: 0x1f8-byte sets; face-gen view)
    };
    char pad20[8];
    int char_count;              // 0x28 (g_char_count 0x8ed4f88 is the same word)
    float weights[63];           // 0x2c (one weight per bone / deform key, up to +0x128)
    float scale;                 // 0x128
    float time;                  // 0x12c
    void randomise();                            // 0x4eb320
    void collect(char flag, rgl::lod_mesh** a, rgl::lod_mesh** b, rgl::lod_mesh** c);   // 0x47fb10 (ret 0x10)
    void set_anim_a(void* a);                    // 0x473800  // old name of FaceGenome::unpack, removed after the names merge
    void set_anim_b(void* a, void* b, int kind); // 0x4746e0  // old name of FaceGenome::breed, removed after the names merge
    void pack_state(PackedState* out);           // 0x4eb660 (ret 4; mission5 reset calls it as its closing step)
    void reset(void* a, void* b);                // 0x520520 (ret 8; src/game/mission5: re-seeds the axes and smooths the 63 weights)
};
typedef char animator_weights_check[(offsetof(Animator, scale) == 0x128) ? 1 : -1];
extern Animator g_anim;          // 0x8ed4f60  // old name of g_anim_table, removed after the names merge

#endif // MB_GAME_ANIMATOR_H
