// scene_file.h: the scene-file container (the .sco reader's target): the prop records, two embedded memory streams
// (ground-paint and terrain blocks), the file name and a loaded flag; the small owner class with an id list; and the
// log hub the loader warns through.
#ifndef TERRAIN_B_SCENE_FILE_H
#define TERRAIN_B_SCENE_FILE_H
#include <vector>
#include "../../rgl/stream.h"
#include "./records.h"

// a std::vector<Rec1c8> whose one-argument resize stays out of line in the original (0x477ea0 in terrain_b.cpp)
struct RecVector : std::vector<Rec1c8> {
    void resize_n(unsigned n);                  // 0x477ea0
};

#include "../../include/logger.h"   // Logger (shared with rgl/scene/render_scene.cpp)
struct LogHub {
    virtual void h0(); virtual void h1(); virtual void h2(); virtual void h3();
    virtual void h4(); virtual void h5(); virtual void h6(); virtual void h7();
    virtual void h8(); virtual void h9();
    virtual Logger* get(const char* msg);       // +0x28
};
extern LogHub g_log;                            // 0x5a9f60

// The scene record (0x494 bytes, table at GameData::levels / game+0x13a50; vtable 0x590904, dtor 0x431e10): the scenes.txt fields
// (mission5/game_2.cpp read_file 0x524d60, ctor 0x5248e0), the .sco file's prop records at +0x104 with the two embedded streams
// at +0x414 / +0x444 and a loaded flag at +0x78 (terrain_b/mission.cpp load 0x479000). ONE definition for src/game: terrain_b
// (named Scene before; collided with party/game.h's Scene) and mission5 (the full record) each saw one half of it; GameData's
// Level494 (src/include/game_data.h) is the same record seen without the vtable.
// the embedded file stream of the scene record (0x30 bytes: vptr, name, state, autoflush, flushed, buf; inline ctor)
struct file_stream_s : stream_base {
    STREAM_SLOTS
    file_stream_s() { state = 0; }
};
#include "../item_ref.h"           // ItemRef (the +0x114 pairs)
#include "../../include/iface.h"        // IfaceB (the polymorphic base of the scene record)
struct SceneFile : IfaceB {
    tstring name10;                                             // +0x10 the scene object file's name (GameData::scene_path 0x463950)
    tstring name;                                               // +0x20 (the .sco warning 0x479000 prints it)
    int f30;                                                    // +0x30 flags (Level494::flags)
    tstring f34;                                                // +0x34
    tstring f44;                                                // +0x44
    float f54, f58, f5c, f60, f64;                              // +0x54
    tstring f68;                                                // +0x68
    char loaded;                                                // +0x78 the .sco was read (0x479000)
    char pad79[3];
    int f7c[0x10];                                              // +0x7c
    int fbc;                                                    // +0xbc
    int fc0[0x10];                                              // +0xc0
    int f100;                                                   // +0x100
    RecVector props;                                            // +0x104 (first at +0x108): the .sco prop records
    ItemRef pairs114[0x40];                                     // +0x114 (item -1, modifier word: ItemRef)
    int ints314[0x40];                                          // +0x314
    file_stream_s s414;                                         // +0x414 ground-paint block (0x479000)
    file_stream_s s444;                                         // +0x444 terrain block
    std::vector<tstring> v474;                                  // +0x474
    tstring f484;                                               // +0x484
    SceneFile();                                                // 0x5248e0
    virtual ~SceneFile();                                       // 0x431e10
    void read_file(stream_base& s);                             // 0x524d60
    void load(stream_base* s, int id);                          // 0x479000
};

// A tiny owner with a std::vector<int> at +0x30 (unrelated to the grid).
struct IdHolder {
    char pad30[0x30];
    std::vector<int> v;                         // +0x30
    void add(int x);                            // 0x478fb0
};

#endif // TERRAIN_B_SCENE_FILE_H
