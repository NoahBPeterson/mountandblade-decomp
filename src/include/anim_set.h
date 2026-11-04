// anim_set.h: AnimSet, the animation definitions of actions.txt (g_anim_set 0xa76628, 8 bytes): a pointer to the array of 0x30-byte
// records (RecH, rgl/xmlcfg/records.h; the agent code reads them as AnimDefV / AnimDefW, the camera as Group) and their count
// (0xa7662c, the bound of the scripts' check_animation 0x49d4b0). The mission agents keep a pointer to the set itself (+0x12c).
#ifndef MB_ANIM_SET_H
#define MB_ANIM_SET_H

struct stream_base;
struct RecH;
struct AnimSet {                                    // 8 bytes
    RecH* data;                                     // +0 (array new, count in front)
    int   count;                                    // +4
    ~AnimSet();                                     // 0x4c2200 (game/triggers/trigger_vec.cpp): destroys the records back to front; the atexit destructor
};
extern AnimSet g_anim_set;                          // 0xa76628 (defined in game/triggers/trigger_vec.cpp)
int __cdecl entry_vec_resize(stream_base* s, AnimSet* self);   // 0x4c2250 (game/triggers/trigger_vec.cpp: reads the count and every record; cdecl, the stream first)

#endif // MB_ANIM_SET_H
