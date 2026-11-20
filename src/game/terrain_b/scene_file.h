// scene_file.h: the scene-file container (the .sco reader's target): the prop records, two embedded memory streams
// (ground-paint and terrain blocks), the file name and a loaded flag; the small owner class with an id list; and the
// log hub the loader warns through.
#ifndef TERRAIN_B_SCENE_FILE_H
#define TERRAIN_B_SCENE_FILE_H
#include <vector>
#include "../../rgl/stream.h"
#include "../placed_obj.h"         // PlacedObj: the .sco prop records

// a std::vector<PlacedObj> whose one-argument resize stays out of line in the original (0x477ea0 in terrain_b.cpp)
// (resize(n) is the out-of-line instance of terrain_b.cpp, 0x477ea0: mission.cpp declares it as an explicit specialization)
struct RecVector : std::vector<PlacedObj> {
};

// The scene record (0x494 bytes, table at GameData::levels / game+0x13a50; vtable 0x590904, dtor 0x431e10): the scenes.txt fields
// (mission5/game_2.cpp read_file 0x524d60, ctor 0x5248e0), the .sco file's prop records at +0x104 with the two embedded streams
// at +0x414 / +0x444 and a loaded flag at +0x78 (terrain_b/mission.cpp load 0x479000). ONE definition for src/game: terrain_b
// (named Scene before; collided with party/game.h's Scene) and mission5 (the full record) each saw one half of it; GameData's
// Level494 (src/include/game_data.h) is the same record seen without the vtable.
// the embedded file stream of the scene record (0x30 bytes: vptr, name, state, autoflush, flushed, buf; inline ctor)
// (the TUs that build or destroy the record include rgl/stream_inline.h next to this header: party/game.cpp, which only reads
// the slots, must not see the inline stream constructors)
// vtable 0x54acf8: bin_stream's (rgl/stream.h): mem_stream's slots without its user-declared destructor, so the scene record's destructor stores no vptr for it
struct file_stream_s : bin_stream {
    file_stream_s() { state = 0; }
};
#include "../item_ref.h"           // ItemRef (the +0x114 pairs)
#include "../../include/iface.h"        // IfaceB (the polymorphic base of the scene record)
struct Visitor { int troop; int dna; };     // 8 bytes: a visitor slot of a scene (set_visitor 0x4a4bff; dna 0xffff2f33 = none)
struct SceneFlags {                         // the scene flags as bits (Mission::start_setup 0x484590 tests and copies them)
    unsigned b0 : 1;                        // 0x1: indoors (no weather / sun, no fog sheet)
    unsigned pad1 : 7;
    unsigned b8 : 1;                        // 0x100: outdoor terrain (a flat level from the terrain code, no body)
    unsigned b9 : 1;                        // 0x200: random weather
    unsigned b10 : 1;                       // 0x400: custom camera slots
    unsigned pad11 : 21;
};     // 8 bytes: a visitor slot of a scene (set_visitor 0x4a4bff; dna 0xffff2f33 = none)
struct SceneFile : IfaceB {
    tstring name10;                                             // +0x10 the scene object file's name (GameData::scene_path 0x463950)
    tstring name;                                               // +0x20 (the .sco warning 0x479000 prints it)
    union {
        int flags;                                              // +0x30 (0x1: indoors, 0x100: outdoor terrain; 0x479e20, 0x47a0c0, 0x4792d0)
        SceneFlags bits;                                        // +0x30 the same word as bits (0x484590)
    };
    tstring mesh_name;                                          // +0x34 the interior mesh prefix (build_interior 0x47a800: find_meshes)
    tstring body_name;                                          // +0x44 the collision body (rebuild_nav 0x479e20: get_object60)
    float rect[4];                                              // +0x54 the play area: min x, min y, max x, max y
    float base_z;                                               // +0x64 the water level / ground height of an interior
    tstring terrain_code;                                       // +0x68 (Terrain::parse)
    char loaded;                                                // +0x78 the .sco was read (0x479000)
    char pad79[3];
    int f7c[0x10];                                              // +0x7c
    int fbc;                                                    // +0xbc
    int fc0[0x10];                                              // +0xc0
    int f100;                                                   // +0x100
    RecVector props;                                            // +0x104 (first at +0x108): the .sco prop records
    Visitor visitors[0x40];                                     // +0x114 the visitor slots (troop -1 = none; reset_visitors 0x4a4b58)
    int visitor_counts[0x40];                                   // +0x314 (set_visitors 0x4a4d75)
    file_stream_s s414;                                         // +0x414 ground-paint block (0x479000)
    file_stream_s s444;                                         // +0x444 terrain block
    std::vector<__int64> slots;                                 // +0x474 the scene slots (scene_set_slot 0x4a0906; char_sheet/save_state.cpp writes them with List8::write8 0x4e94b0)
    tstring f484;                                               // +0x484
    SceneFile();                                                // 0x5248e0
    virtual ~SceneFile();                                       // 0x431e10
    void read_file(stream_base& s);                             // 0x524d60
    void load(stream_base* s, int id);                          // 0x479000
};


#endif // TERRAIN_B_SCENE_FILE_H
