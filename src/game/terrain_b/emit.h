// emit.h: the particle emitter (0x1bc bytes) and the emitter definition table, as far as the mission's ambient
// effects use them: EmitTable::spawn (0x479d20) allocates and sets one up, Mission::post_spawn (0x47a0c0) scales and
// places the fog emitters. src/game/agent3/agent_fx_sound.h declares the other fields of the same record.
#ifndef TERRAIN_B_EMIT_H
#define TERRAIN_B_EMIT_H
#include "../../rgl/rgl_math.h"

struct Emit {
    char pad0[0x10];
    int f10, f14, f18;                                  // +0x10 (0x479d20 zeroes them)
    char pad1c[0x3c - 0x1c];
    rgl::vector4 v3c;                                   // +0x3c scaled by post_spawn
    rgl::vector4 v4c;                                   // +0x4c
    rgl::vector4 v5c;                                   // +0x5c
    char pad6c[0xec - 0x6c];
    float height;                                       // +0xec height offset of the placed emitter
    char padf0[0x1b8 - 0xf0];
    int owner;                                          // +0x1b8
    Emit(const int* o) { f10 = 0; f14 = 0; f18 = 0; owner = *o; init(); }
    void init();                                        // 0x420b40
};
struct EmitDef;
struct EmitTable {
    EmitDef* defs;                                      // +0
    int count;                                          // +4
    void setup(Emit* e, int idx, const rgl::vector4* pos);      // 0x4424c0
    Emit* spawn(int* idx, const rgl::vector4* pos, const int* owner);   // 0x479d20
};
extern EmitTable g_emit_defs;                           // 0x8ed5090
extern int g_emit_id[];                                 // 0x8ed5098 (ids of the ambient emitters: dense fog 4, thin 6)
extern int g_fog_id_a;                                  // 0x8ed50b4 (ground fog layer)
extern int g_fog_id_b;                                  // 0x8ed50b8
extern int g_fog_id_c;                                  // 0x8ed50bc
extern int g_env_on;                                    // 0x5aa028 (environmental effects)
extern int g_fog_density;                               // 0x5aa03c (>= 10: build the dust grid)

// the 0x4c-byte rows at g_env_rows (0x5e28f4): camera rows (post_spawn) and hourly ambient rows (setup_ambient)
struct EnvRow {
    char pad0[0x10];
    int f10;                                            // +0x10
    char pad14[0x20 - 0x14];
    rgl::vector4 v20;                                   // +0x20 camera focus (x, y, z) / wind strength per axis
    rgl::vector4 base;                                  // +0x30 (the 4th lane is read by post_spawn)
    float f40, f44, f48;                                // +0x40
    rgl::vector4 get_dir(int idx, int day);             // 0x463610 (returns by hidden pointer)
};
extern EnvRow* g_env_rows;                              // 0x5e28f4

#endif // TERRAIN_B_EMIT_H
