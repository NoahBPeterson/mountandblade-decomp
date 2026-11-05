// emit.h: the particle emitter (0x1bc bytes) and the emitter definition table, as far as the mission's ambient
// effects use them: EmitTable::spawn (0x479d20) allocates and sets one up, Mission::post_spawn (0x47a0c0) scales and
// places the fog emitters. src/game/agent3/agent_fx_sound.h declares the other fields of the same record.
#ifndef TERRAIN_B_EMIT_H
#define TERRAIN_B_EMIT_H
#include "../../rgl/rgl_math.h"

#include "../emit.h"                  // rgl::psys, EmitTable (shared with agent3)






#include "../env_row.h"                                 // EnvRow, g_env_rows (0x5e28f4)

#endif // TERRAIN_B_EMIT_H
