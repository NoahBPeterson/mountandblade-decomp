// anim_set.h: the animations of actions.txt, ONE definition for src/game and src/rgl: AnimSet (g_anim_set 0xa76628, 8 bytes:
// the array of AnimDef and its count, the bound of the scripts' check_animation 0x49d4b0), AnimDef (0x30 bytes: one animation,
// read by read_entry 0x4c2170, vector deleting destructor 0x4315d0) and SubAnim (0x40 bytes: one of its sequences, read by
// SubAnim::read 0x4bdc00). The mission agents keep a pointer to the set itself (Agent::defs +0x12c).
#ifndef MB_ANIM_SET_H
#define MB_ANIM_SET_H
#include "engine/core/rgl_base.h"
#include "engine/core/tstring.h"

struct stream_base;
struct skel_anim;                                   // engine/resource/resource.h

struct SubAnim {                                    // 0x40 bytes: one sequence of an animation
    skel_anim* anim;                                // +0 the skeleton animation (resmgr::get_skeleton_anim)
    int f4;                                         // +4
    unsigned flags;                                 // +8 (0x10000000 loop, 0x100 footstep dust, 0x200 sound; Agent::start_channel 0x43a980)
    float scale;                                    // +0xc duration
    int a, b;                                       // +0x10 / +0x14 the key range
    unsigned event_bytes;                           // +0x18 four event phases in 1/255 (0 = none)
    float events[4];                                // +0x1c the event phases (-1 = none; Agent::update 0x45f660)
    rgl::vector4 displacement;                      // +0x2c the root displacement over the sequence (0x43d040, 0x45d820)
    float offset;                                   // +0x3c
    static void* read(stream_base& a, SubAnim* o);  // 0x4bdc00 (game/config/config.cpp; cdecl (stream, record))
};

struct AnimDef {                                    // 0x30 bytes: one animation
    tstring name;                                   // +0
    SubAnim* subs;                                  // +0x10 the sequences (array new)
    int nsubs;                                      // +0x14
    unsigned flags;                                 // +0x18 (top byte: the priority when set)
    tstring f1c;                                    // +0x1c
    int priority;                                   // +0x2c (100 unless the flags' top byte is set; AnimSlot::f28)
    AnimDef() { subs = 0; }                         // (entry_vec_resize's array new stores the null array pointer)
    ~AnimDef() { delete[] subs; }
};

struct AnimSet {                                    // 8 bytes
    AnimDef* data;                                  // +0 (array new, count in front)
    int   count;                                    // +4
    ~AnimSet();                                     // 0x4c2200 (game/app/options_dialog_and_module_tables.cpp): destroys the records back to front; the atexit destructor
};
extern AnimSet g_anim_set;                          // 0xa76628 (defined in game/app/options_dialog_and_module_tables.cpp)
stream_base* __cdecl entry_vec_resize(stream_base* s, AnimSet* self);   // 0x4c2250 (game/app/options_dialog_and_module_tables.cpp: reads the count and every record; cdecl, the stream first)

#endif // MB_ANIM_SET_H
