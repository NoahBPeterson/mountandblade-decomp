// terrain_scene_renderer.h: shared declarations of terrain_scene_renderer.cpp (0x46e590..0x478830): the second part of the terrain object's
// translation unit (TU 61 of work/migrate/tu_map.csv). One header per class lives next to this file; this one pulls
// them together and declares the globals and external functions the module uses.
#ifndef TERRAIN_B_H
#define TERRAIN_B_H
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <list>
#include "engine/math/rgl_math.h"
#include "engine/render/mesh.h"
#include "engine/core/tstring.h"
#include "engine/io/stream.h"
#include "game/terrain/render_batch.h"
#include "game/terrain/poly_mesh.h"
#include "game/terrain/cell_grid.h"
#include "game/terrain/terrain_game_terrain_b.h"
#include "game/terrain/records.h"
#include "game/terrain/game_data.h"
#include "game/terrain/face.h"
#include "game/terrain/troop_mgr.h"
#include "game/terrain/shadow_map.h"
#include "engine/core/pool.h"      // Pool<T>: the paged record pool
#include "game/agent/missile.h"   // Missile, the record of Pool<Missile> (Mission::poolJ)
#include "game/terrain/agent_rec.h"
#include "game/mission/scene_container_grid.h"
#include "game/terrain/physics.h"
#include "game/agent/missile.h"                   // Missile (shared with agent4 / terrain_a): settle 0x472650 is defined in poly_mesh.cpp
#include "game/terrain/face_gen.h"

// ---- small inline vector helpers of the object's float code (3-component results; the 4th lane is left alone) ------
static inline float dot3(const rgl::vector4& a, const rgl::vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline void vscale_to(rgl::vector4& r, const rgl::vector4& v, float k) { r.x = v.x * k; r.y = v.y * k; r.z = v.z * k; }
static inline rgl::vector4 vadd3(const rgl::vector4& a, const rgl::vector4& b) { rgl::vector4 r; r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; return r; }
static inline float max2(float a, float b) { return a > b ? a : b; }

// ---- globals ---------------------------------------------------------------------------------------------------------
#include "engine/core/clock_table.h"   // g_ticks (the clock table)
extern int g_use_shaders;                       // 0x5e28fc programmable pipeline in use / detail resources present
extern ptype* g_ptypes;                         // 0x5e28f8 particle / prop type table

#include "engine/render/shadow_map.h"
#include "engine/render/render_flags.h"
#include "game/common/level_global.h"      // g_level (0xa732b4)

extern unsigned __int64 g_bit[];                // 0x5a8e68 (g_bit[i] == 1 << i)

rgl::vector4 __stdcall wind_noise();             // 0x463a80 (hidden return pointer)
void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0

#endif // TERRAIN_B_H
