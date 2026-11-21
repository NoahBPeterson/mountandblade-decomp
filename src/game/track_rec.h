// track_rec.h: the 0x5c-byte map track record (GameData::tracks, the vector at +0x13a60: the party-track ribbons and
// the battle tracks). ONE definition for game_data.h / party/game.h (the vector, the save-game reader 0x51e750),
// inventory_panel (the tick that ages the ribbons, 0x518680 .. 0x518990) and char_sheet/save_state (the writer 0x4e9460).
// The readers name the same dwords as floats or ints; they overlay in anonymous unions of plain members.
#ifndef MB_GAME_TRACK_REC_H
#define MB_GAME_TRACK_REC_H
#include "../rgl/rgl_math.h"            // rgl::matrix (the ribbon's frame)
struct stream_base;
struct stream_base;

struct TrackRec {
    union { float x; int f0; };         // +0x00 the map position (r_float; the writer stores the words with w_int)
    union { float y; int f4; };         // +0x04
    union { float z; int f8; };         // +0x08
    float w;                            // +0x0c (the position's 4th lane: whole-vector copies carry it)
    union { float f10; int i10; };      // +0x10 (r_float; 0x4e9460 w_int)
    union {
        float age;                      // +0x14 seconds since the track was laid (0x518680 adds dt)
        int i14;                        // +0x14 as an int (0x4e9460)
    };
    union {
        unsigned flags;                 // +0x18 (0x20000000 seen, 0x10000000 battle track; low byte the size class)
        int i18;                        // +0x18 as the reader's int (0x51e750)
        struct {
            unsigned short w18;         // +0x18
            unsigned char f1a;          // +0x1a (low nibble: the ribbon's lifetime step, 0x518680)
            unsigned char b1b;          // +0x1b
        };
    };
    rgl::matrix frame;                  // +0x1c the ribbon's frame (GameData::init_event 0x4b36a0; drawn by update_tracks 0x5189e0)
    ~TrackRec() {}                      // the original's vector<TrackRec> instances keep the destroy loops (0x4b54a0 _Insert_n, the _Tidy of 0x4b49d0)
    const rgl::vector4& pos() const { return *(const rgl::vector4*)&x; }   // x, y, z, w as the vector4 the readers copy whole
    __declspec(noinline) void read(stream_base* s);   // 0x51e750 (party/game.cpp)
    __declspec(noinline) void write(stream_base* s);    // 0x4e9460 (char_sheet/save_state.cpp, ret 4)
};

#endif // MB_GAME_TRACK_REC_H
