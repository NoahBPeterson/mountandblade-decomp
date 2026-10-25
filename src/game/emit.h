// emit.h: the particle emitter (Emit, 0x1bc bytes; the rgl name of the same object is rgl::psys in
// src/rgl/scene/emitter.h, constructor / init 0x420b40) and the emitter definition table (EmitTable, g_emit_defs
// 0x8ed5090 with its 0xc0-byte EmitDef records). ONE definition for src/game/agent3 (agent_fx_sound.cpp: EmitTable::setup
// 0x4424c0 fills an emitter from a definition, the sound / fx members) and src/game/terrain_b (mission.cpp:
// EmitTable::spawn 0x479d20 allocates and sets one up, Mission::post_spawn 0x47a0c0 scales and places the fog emitters).
// Offsets cite the function that reads or writes them. Needs rgl::vector4 (rgl_base.h picks the module's own).
// Names kept as the manifests mangle them (Emit, EmitTable).
#ifndef GAME_EMIT_H
#define GAME_EMIT_H
#include "../include/rgl_base.h"
#include "../include/mat4.h"
#include "./tstring.h"

struct tstream3;

// plain 4-float vector for the members that share bytes with other views (rgl::vector4 has constructors)
struct EmitVec4 { float x, y, z, w; };
struct EmitPair { int a, b; };

struct EmitRes {                                        // what Emit::res points to
    char pad0[0x1c];
    unsigned flags;                                     // +0x1c (0x40: follows the camera) (0x442ca0)
};
struct res_obj {                                        // resource returned by resmgr::get_object
    char pad0[0x1c];
    unsigned flags;                                     // +0x1c
    char pad20[0x2c - 0x20];
    EmitRes* f2c;                                       // +0x2c (0x4424c0)
    char pad30[0x4c - 0x30];
    int f4c;                                            // +0x4c mirrored flag (0x44c210)
    char pad50[0xcc - 0x50];
    rgl::matrix xform;                                  // +0xcc local transform of a mesh object (0x44be50)
    int f10c;                                           // +0x10c collision mask (0x44c210)
    res_obj* clone();                                   // 0x41c6d0 (mesh objects)
};
struct EmitDef {                                        // 0xc0-byte definition record
    char pad0[0x10];
    int f10;                                            // +0x10
    tstring name;                                       // +0x14
    int f24;                                            // +0x24
    char pad28[0x30 - 0x28];
    int f30, f34, f38, f3c, f40;                        // +0x30
    EmitPair t[5][2];                                   // +0x44 (rows of 0x10)
    rgl::vector4 a;                                     // +0x94
    rgl::vector4 b;                                     // +0xa4
    int c;                                              // +0xb4
    int fb8, fbc;                                       // +0xb8
};
struct Emit {                                           // particle emitter (0x1bc bytes)
    char pad0[4];
    EmitRes* res;                                       // +4 (0x4424c0 stores res_obj::f2c; 0x442ca0 reads its flags)
    res_obj* obj;                                       // +8
    char padc[4];
    int f10, f14, f18;                                  // +0x10 (0x479d20 zeroes them)
    int f1c, f20, f24;                                  // +0x1c
    union {
        EmitPair t[5][2];                               // +0x28 (0x4424c0)
        struct {
            char pad28[0x3c - 0x28];
            EmitVec4 v3c;                               // +0x3c scaled by post_spawn (0x47a0c0)
            EmitVec4 v4c;                               // +0x4c
            EmitVec4 v5c;                               // +0x5c
        };
    };
    char pad78[0xa0 - 0x78];
    int fa0;                                            // +0xa0
    char pada4[0xc4 - 0xa4];
    rgl::vector4 gravity;                               // +0xc4
    int fd4;                                            // +0xd4
    rgl::vector4 offs;                                  // +0xd8
    unsigned flags;                                     // +0xe8 (bit 0 cleared on spawn)
    float height;                                       // +0xec height offset of the placed emitter (0x47a0c0)
    char padf0[4];
    rgl::mat4 world;                                    // +0xf4
    rgl::mat4 local;                                    // +0x134
    char pad174[0x198 - 0x174];
    int f198;                                           // +0x198
    int f19c, f1a0, f1a4;                               // +0x19c
    int idx;                                            // +0x1a8
    char pad1ac[4];
    int f1b0, f1b4;                                     // +0x1b0
    int owner;                                          // +0x1b8
    Emit(const int* o) { f10 = 0; f14 = 0; f18 = 0; owner = *o; init(); }
    Emit() { f10 = 0; f14 = 0; f18 = 0; owner = 0; init(); }   // mesh_slot/mesh_start.cpp 0x484590: `new Emit` for the scene fog / layer emitters
    void init();                                        // 0x420b40
    void reset_pending(int v);                          // 0x433810 (ret 4; agent4: the dust / splash emitters of a footstep)
    void set_v184(const rgl::vector4* v);               // 0x433740 (agent4: the velocity of an impact particle)
    void set_placement(const rgl::mat4* m, const rgl::vector4* a, const rgl::vector4* b, int c);   // 0x433770
};
struct EmitTable {
    EmitDef* defs;                                      // +0
    int count;                                          // +4
    int idx1, idx2;                                     // +8, +0xc the definition indices of the two scene emitters (mesh_slot/mesh_start.cpp 0x484590)
    void setup(Emit* e, int idx, const rgl::vector4* pos);                  // 0x4424c0
    Emit* spawn(int* idx, const rgl::vector4* pos, const int* owner);       // 0x479d20 (mission.cpp)
    void resize(tstream3* s);                                                 // 0x4c1f70 (inventory_panel/module_files.cpp: the particle-system file loader)
};
extern EmitTable g_emit_defs;                           // 0x8ed5090
#endif // GAME_EMIT_H
