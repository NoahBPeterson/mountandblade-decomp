// emit.h: the particle emitter (0x1bc bytes) and the emitter definition table, as far as the mission's ambient
// effects use them: EmitTable::spawn (0x479d20) allocates and sets one up, Mission::post_spawn (0x47a0c0) scales and
// places the fog emitters. src/game/agent3/agent_fx_sound.h declares the other fields of the same record.
#ifndef TERRAIN_B_EMIT_H
#define TERRAIN_B_EMIT_H
#include "../../rgl/rgl_math.h"

#include "../emit.h"                  // Emit, EmitTable (shared with agent3)
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
