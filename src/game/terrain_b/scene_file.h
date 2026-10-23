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

struct Logger {
    virtual void l0(); virtual void l1(); virtual void l2(); virtual void l3();
    virtual void l4(); virtual void l5(); virtual void l6(); virtual void l7();
    virtual void l8(); virtual void l9();
    virtual Logger* get(const char* msg);       // +0x28
    virtual void put(const tstring* s);         // +0x2c
};
struct LogHub {
    virtual void h0(); virtual void h1(); virtual void h2(); virtual void h3();
    virtual void h4(); virtual void h5(); virtual void h6(); virtual void h7();
    virtual void h8(); virtual void h9();
    virtual Logger* get(const char* msg);       // +0x28
};
extern LogHub g_log;                            // 0x5a9f60

// The scene container: records at +0x104, two embedded streams at +0x414 / +0x444, a name at +0x20 and a loaded
// flag at +0x78.
struct SceneFile {                              // the .sco scene file record (named Scene before; collided with party/game.h's Scene)
    char pad20[0x20];
    tstring name;                               // +0x20
    char pad30[0x78 - 0x30];
    char loaded;                                // +0x78
    char pad79[0x104 - 0x79];
    RecVector props;                            // +0x104 (first at +0x108)
    char pad114[0x414 - 0x114];
    mem_stream s414;                            // +0x414 (0x30 bytes)
    mem_stream s444;                            // +0x444
    void load(stream_base* s, int id);          // 0x479000
};

// A tiny owner with a std::vector<int> at +0x30 (unrelated to the grid).
struct IdHolder {
    char pad30[0x30];
    std::vector<int> v;                         // +0x30
    void add(int x);                            // 0x478fb0
};

#endif // TERRAIN_B_SCENE_FILE_H
