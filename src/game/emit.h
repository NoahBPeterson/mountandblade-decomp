// emit.h: the emitter definition table (EmitTable, g_emit_defs
// 0x8ed5090 with its 0xc0-byte EmitDef records). ONE definition for src/game/agent3 (agent_fx_sound.cpp: EmitTable::setup
// 0x4424c0 fills an emitter from a definition, the sound / fx members) and src/game/terrain_b (mission.cpp:
// EmitTable::spawn 0x479d20 allocates and sets one up, Mission::post_spawn 0x47a0c0 scales and places the fog emitters).
// Offsets cite the function that reads or writes them. Needs rgl::vector4 (rgl_base.h picks the module's own).
// Names kept as the manifests mangle them (EmitTable).
#ifndef GAME_EMIT_H
#define GAME_EMIT_H
#include "../include/rgl_base.h"
#include "../include/mat4.h"
#include "./tstring.h"
#include "../rgl/particles/mesh_obj.h"   // first: it has the full rgl::emitter, emitter.h only declares the member psys needs
#include "../rgl/scene/emitter.h"    // rgl::psys: the particle emitter (0x1bc bytes; one definition for src/rgl and src/game)

struct tstream3;

namespace rgl { struct mesh; }
struct EmitDef {                                        // 0xc0-byte definition record
    char pad0[0x10];
    int f10;                                            // +0x10 the emitter's flags (psys::flags)
    tstring name;                                       // +0x14
    float f24;                                          // +0x24 psys::f198
    char pad28[0x30 - 0x28];
    int f30;                                            // +0x30
    float f34, f38, f3c, f40;                           // +0x34 psys::f24 (drag), fd4, f1c (noise scale), f20 (noise amount)
    rgl::float_pair t[5][2];                            // +0x44 (rows of 0x10: the five (t0,v0,t1,v1) curves of psys::curve)
    rgl::vector4 a;                                     // +0x94
    rgl::vector4 b;                                     // +0xa4
    int c;                                              // +0xb4
    int fb8, fbc;                                       // +0xb8
};
struct stream_base;
struct EmitTable {                                      // 0x34 bytes (g_emit_defs 0x8ed5090): the particle-system table, one object
    EmitDef* defs;                                      // +0
    int count;                                          // +4
    int used[11];                                       // +8 the definition index of each of the engine's 11 named effects (g_psys_names, load_used 0x4bdb60, -1 = missing):
                                                        //    [0] / [1] the two scene emitters (mesh_slot/mesh_start.cpp 0x484590), [2] / [3] the default voice effects (agent_logic),
                                                        //    [4] dense fog and the default effect agent_logic compares against, [6] thin fog, [7..9] the three ground fog layers
    ~EmitTable();                                       // 0x4c1f20 (the atexit destructor; triggers/trigger_vec.cpp)
    void load_used();                                   // 0x4bdb60 (config/config.cpp)
    void resize(stream_base& s);                        // 0x4c1f70 (triggers/trigger_vec.cpp: reads the count and every record)
    void setup(rgl::psys* e, int idx, const rgl::vector4* pos);                  // 0x4424c0
    rgl::psys* spawn(int* idx, const rgl::vector4* pos, const int* owner);       // 0x479d20 (mission.cpp)
};
extern EmitTable g_emit_defs;                           // 0x8ed5090
#endif // GAME_EMIT_H
