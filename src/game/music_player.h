// music_player.h: the music player (global g_music, 0xa73a10): ONE definition for config.cpp (stop 0x4be0a0, play 0x4be0e0), triggers
// (constructor 0x4c24c0, advance_piece 0x4c25e0), rgl/mesh2/rec_tables.cpp (tick 0x4c4100), inventory_panel (build 0x50f7e0, GameApp::Update)
// and the script interpreter (situation, track range check).
#ifndef GAME_MUSIC_PLAYER_H
#define GAME_MUSIC_PLAYER_H
#include <stddef.h>
#include "../include/layout32.h"
#include "../include/timer.h"
#include "./tstring.h"
typedef void* FMOD_STREAM;
struct stream_base;                                 // ../rgl/stream.h
struct MusicTrack {                                 // one 0x20-byte track record of music.txt (0x50f840)
    int f0;                                         // +0 flags: 0x80 = always worth restarting, 0x100 = plays once, 0x40 = looped stream,
                                                    //    0x10000000 = in the module's Music folder
    int f4;                                         // +4 the situation modes the track suits (f0 | the second file mask)
    int f8;                                         // +8 the loader's first derived mask (f0 & 0x3f, or 0x3f)
    int fc;                                         // +0xc the culture / category mask
    tstring name;                                   // +0x10 the tune's file name
    __declspec(noinline) void read(stream_base* s); // 0x50f840
};
struct music_player {                               // the music player, global g_music (0xa73a10); 0x100 track records of 0x20 bytes from +8
    int situation;                                  // +0x00 the situation mask the track choice matches (script op music_set_situation; 0 = none)
    int track_count;                                // +0x04 the number of tracks loaded (MusicTable::build 0x50f7e0 reads it; the scripts range-check track ids against it)
    MusicTrack tracks[0x100];                       // +0x08 the track records
    float start[256];                               // +0x2008 start time per record
    int active[2];                                  // +0x2408 per stream slot: playing (slot 1 is the piece being faded)
    int opened[2];                                  // +0x2410 per slot: the stream has opened and plays
    gui_timer t2418;                                    // +0x2418 last advance of the current piece
    FMOD_STREAM stream[2];                          // +0x2428
    int track[2];                                   // +0x2430 per slot: the record index
    int channel[2];                                 // +0x2438 per slot: the FMOD channel (-1 = none)
    int volume[2];                                  // +0x2440 per slot: the last volume set / 10000 for a new piece
    int mask;                                       // +0x2448 the culture / category mask (script op music_set_culture 0x4a1755)
    gui_timer t2450;                                    // +0x2450 start of the new piece
    gui_timer t2460;                                    // +0x2460 fade end
    int fading[2];                                  // +0x2470 per slot: the piece is fading
    char new_piece;                                 // +0x2478
    tstring root;                                   // +0x247c the module's music root
    void stop(int i);                               // 0x4be0a0
    void play(int rec, int* slot);                  // 0x4be0e0
    ~music_player();                                // compiler-called at exit (the old static_dtor_548310: root, then the 0x100 names from the last)
    music_player();                                 // 0x4c24c0 (src/game/triggers: constructs the 0x100 record names, the three clocks, the monitor state)
    void advance_piece();                                  // 0x4c25e0 per-frame monitor of the two stream slots and the track selection
    void __fastcall tick();                         // 0x4c4100 (rgl/mesh2/rec_tables.cpp)
    void build(stream_base* s);                     // 0x50f7e0 (inventory_panel/loading_screen.cpp: reads track_count and the track records)
};
extern music_player g_music;                        // 0xa73a10 (defined in triggers/trigger_vec.cpp; the compiler constructs it and registers the destructor)
// a free inline helper taking the object by reference: Operation::execute 0x49fa10 first-references fewer global
// field views than direct member stores would make (the global symbol-key count fixes g_visitor_scene's sort order)
inline void set_situation(music_player& m, int s) { m.situation = s; }     // script op music_set_situation
inline void set_culture(music_player& m, int c) { m.mask = c; }           // script op music_set_culture 0x4a1755
MB_LAYOUT32_ASSERT(music_player_layout_check, (offsetof(music_player, start) == 0x2008 && offsetof(music_player, active) == 0x2408 && offsetof(music_player, t2460) == 0x2460 && offsetof(music_player, root) == 0x247c && sizeof(music_player) == 0x2490));
#endif // GAME_MUSIC_PLAYER_H
