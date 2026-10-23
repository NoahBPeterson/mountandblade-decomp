// terrain_b.h: shared declarations of terrain_b.cpp (0x46e590..0x478830): the second part of the terrain object's
// translation unit (TU 61 of work/migrate/tu_map.csv). One header per class lives next to this file; this one pulls
// them together and declares the globals and external functions the module uses.
#ifndef TERRAIN_B_H
#define TERRAIN_B_H
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <list>
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"
#include "../tstring.h"
#include "../../rgl/stream.h"
#include "./render_batch.h"
#include "./poly_mesh.h"
#include "./cell_grid.h"
#include "./terrain.h"
#include "./records.h"
#include "./game_data.h"
#include "./face.h"
#include "./troop_mgr.h"
#include "./shadow_map.h"
#include "./pool.h"
#include "./agent_rec.h"
#include "./mission.h"
#include "./physics.h"
#include "./missile.h"
#include "./face_gen.h"

// ---- small inline vector helpers of the object's float code (3-component results; the 4th lane is left alone) ------
static inline float dot3(const rgl::vector4& a, const rgl::vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline void vscale_to(rgl::vector4& r, const rgl::vector4& v, float k) { r.x = v.x * k; r.y = v.y * k; r.z = v.z * k; }
static inline rgl::vector4 vadd3(const rgl::vector4& a, const rgl::vector4& b) { rgl::vector4 r; r.x = a.x + b.x; r.y = a.y + b.y; r.z = a.z + b.z; return r; }
static inline rgl::vector4 vsub3(const rgl::vector4& a, const rgl::vector4& b) { rgl::vector4 r; r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; return r; }
static inline float max2(float a, float b) { return a > b ? a : b; }

// ---- views that add members to shared classes (a class cannot be extended from outside) -------------------------------
struct mesh_flip : rgl::mesh {
    void flip_faces();                          // 0x4779b0 (rgl::mesh::flip_faces: reverse every polygon's winding)
};

// ---- globals ---------------------------------------------------------------------------------------------------------
extern __int64 g_ticks[];                       // 0x5e2948 clock table, 1e-5 s ticks (the slices' g_time / g_ticks)
extern int g_use_shaders;                       // 0x5e28fc programmable pipeline in use / detail resources present
extern int g_mesh_quality;                      // 0x5aa084 (2 = high detail; shader quality)
extern ptype* g_ptypes;                         // 0x5e28f8 particle / prop type table
extern ShadowMap g_probe;                          // 0xa70248
extern CellGrid* g_level;                       // 0xa732b4
extern unsigned g_field_mask;                   // 0x5a8ea0 (field values are clamped to g_field_mask - 1)
extern unsigned __int64 g_bit[];                // 0x5a8e68 (g_bit[i] == 1 << i)

rgl::vector4 __stdcall wind_noise();             // 0x463a80 (hidden return pointer)
bool __stdcall ground_height(CellGrid* lv, const float* p, float* out);   // 0x470f40
void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0

#endif // TERRAIN_B_H
