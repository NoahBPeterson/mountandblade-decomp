// animator.h: the character-builder / animation state (global g_anim at 0x8ed4f60). ONE definition for mesh_slot
// (mesh_char / mesh_terrain / mesh_appear), mission5 (troop_rec: reset), char_sheet (facegen_misc, facegen_init, cam_view)
// and inventory_panel (view_open). The universes of those TUs differ (rgl_math.h / rgl.h), so this header names no engine
// type: rgl::lod_mesh and Skin are forward declarations, and the PackedState it fills is defined here.
// Offsets: 0x473800 set_anim_a, 0x4746e0 set_anim_b, 0x47fb10 collect (mesh_char.cpp), 0x4eb320 randomise and 0x4eb660
// pack_state (facegen_misc.cpp), 0x520520 reset (mission5/troop_rec.cpp). The face-generator window (0x4eb270) writes the
// first words from its sliders (set = skin-1, item = hair-1, variant = beard % div, material = colour).
#ifndef MB_GAME_ANIMATOR_H
#define MB_GAME_ANIMATOR_H
#include <stddef.h>
#include "../include/layout32.h"
#include "../include/skin.h"   // SkinVec (the skin vector), Skin

namespace rgl { struct lod_mesh; }
struct stream_base;

struct PackedState {                                      // the 32 bytes at troop record +0x3c0 = Troop::face_keys (pack_state 0x4eb660)
    union {
        struct {
            unsigned __int64 header;                       // item + 1 (bits 0..5), set + 1 (6..11), material (12..17), variant (18..23), time (24..29), scale (30..35), each in 6 bits
            unsigned __int64 weights_a;                    // the face weights, 3 bits each, 21 per word (Animator::pack_state: i * 3 below 0x3f)
            unsigned __int64 weights_b;                    // the next 21 weights (and the shift overflow of the pack loop, which never reaches a fourth word)
            unsigned __int64 weights_c;                    // cleared by pack_state; the face generator copies it to the record
        };
        unsigned __int64 word[4];                          // the same four words (the hex face code, the record copy)
    };
};

struct Animator {
    int set;                     // 0x00 (facegen: f350 - 1)
    int item;                    // 0x04 (facegen: f354 - 1)
    int cur;                     // 0x08 (g_cur, 0x8ed4f68: the skin set index; 0x4eb660 / 0x4eb320 scale it by 0x1f8)
    int variant;                 // 0x0c
    int f10;                     // 0x10 (facegen: the same value as variant)
    int material;                // 0x14
    SkinVec skin_vec;            // 0x18 the skin vector (0x8ed4f78; first at 0x8ed4f7c, last 0x8ed4f80): the compiler registers its destructor 0x4eb810 with atexit (0x548510)
    int char_count;              // 0x28 the number of skin sets (0x8ed4f88)
    float weights[63];           // 0x2c (one weight per bone / deform key, up to +0x128)
    float scale;                 // 0x128
    float time;                  // 0x12c
    void read(stream_base* s);                   // 0x50d520 (inventory_panel/widget_text.cpp: skins.txt)
    void randomise();                            // 0x4eb320
    void collect(char flag, rgl::lod_mesh** a, rgl::lod_mesh** b, rgl::lod_mesh** c);   // 0x47fb10 (ret 0x10)
    Skin* unpack(const unsigned __int64* key);       // 0x473800 (returns the race record, left in EAX)
    void breed(const unsigned __int64* a, const unsigned __int64* b, int seed);   // 0x4746e0
    void pack_state(PackedState* out);           // 0x4eb660 (ret 4; mission5 reset calls it as its closing step)
    void reset(void* a, void* b);                // 0x520520 (ret 8; src/game/mission5: re-seeds the axes and smooths the 63 weights)
    void get_text(int skin, int key, tstring* out);   // 0x4f5d90 (char_sheet/editor_ui.cpp): face key `key` of skin `skin`, translated
    // the skin table as the agents read it (agent4/agent_logic.cpp): voice sounds and blood effects per skin, by index
    int voice(unsigned skin, int kind)          // Agent::play_class_sound 0x4459c0 (inline): the skin's sound for the voice kind, -1 past the table
    {
        if (skin < (unsigned)char_count)
            return skin_vec[skin].voices.v[kind];
        return -1;
    }
    int blood1(unsigned skin);                  // 0x43b100 the skin's first blood effect, 0 past the table
    int blood2(unsigned skin);                  // 0x43b130 the second
};
MB_LAYOUT32_ASSERT(animator_weights_check, (offsetof(Animator, scale) == 0x128));
extern Animator g_anim;          // 0x8ed4f60 the one object (the skin table / character builder; defined in char_sheet/facegen_misc.cpp: every word of it is a member, no separate label)

#endif // MB_GAME_ANIMATOR_H
