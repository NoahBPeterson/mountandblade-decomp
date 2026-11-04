// emit.h: the particle emitter (0x1bc bytes) and the emitter definition table, as far as the mission's ambient
// effects use them: EmitTable::spawn (0x479d20) allocates and sets one up, Mission::post_spawn (0x47a0c0) scales and
// places the fog emitters. src/game/agent3/agent_fx_sound.h declares the other fields of the same record.
#ifndef TERRAIN_B_EMIT_H
#define TERRAIN_B_EMIT_H
#include "../../rgl/rgl_math.h"

#include "../emit.h"                  // Emit, EmitTable (shared with agent3)
extern int g_emit_id[];                                 // 0x8ed5098 (ids of the ambient emitters: dense fog 4, thin 6)



#include "../../include/emit_ids.h"



#include "../env_row.h"                                 // EnvRow, g_env_rows (0x5e28f4)

#endif // TERRAIN_B_EMIT_H
