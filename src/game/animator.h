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
    int skins_alloc;             // 0x18 the allocator dword of the skin vector (0x8ed4f78: its atexit destructor 0x548510 runs BigArray::destroy 0x4eb810 on &skins_alloc)
    union {
        EChar* chars;            // 0x1c the vector's first element (mesh_slot view; 0x8ed4f7c)
        SkinSet* skins;          // 0x1c the same pointer, the face-gen / language-export view: 0x1f8-byte sets
    };
    SkinSet* skins_end;          // 0x20 the vector's last (0x8ed4f80)
    char pad24[4];               // 0x24 its end of storage
    int char_count;              // 0x28 the number of skin sets (0x8ed4f88)
    float weights[63];           // 0x2c (one weight per bone / deform key, up to +0x128)
    float scale;                 // 0x128
    float time;                  // 0x12c
    void randomise();                            // 0x4eb320
    void collect(char flag, rgl::lod_mesh** a, rgl::lod_mesh** b, rgl::lod_mesh** c);   // 0x47fb10 (ret 0x10)
    void unpack(void* a);                    // 0x473800
    void breed(void* a, void* b, int kind); // 0x4746e0
    void pack_state(PackedState* out);           // 0x4eb660 (ret 4; mission5 reset calls it as its closing step)
    void reset(void* a, void* b);                // 0x520520 (ret 8; src/game/mission5: re-seeds the axes and smooths the 63 weights)
};
typedef char animator_weights_check[(offsetof(Animator, scale) == 0x128) ? 1 : -1];
extern Animator g_anim;          // 0x8ed4f60 the one object (the skin table / character builder; defined in char_sheet/facegen_misc.cpp: every word of it is a member, no separate label)

#endif // MB_GAME_ANIMATOR_H
