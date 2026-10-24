// common.h: types and globals shared by the three modules of src/game/terrain_a (wind.cpp, game_state.cpp,
// terrain_a.cpp): the plain-old-data element types of the std::vector instantiations that sit in
// the terrain object's translation unit, the engine globals they read, and the noise / random helpers.
// Names provisional. Every VA comment cites the function (or table) the declaration was verified against.
#ifndef TERRAIN_A_COMMON_H
#define TERRAIN_A_COMMON_H
#include <stddef.h>
#include "../../rgl/rgl_math.h"

// ---- plain element types of the vector<T> instantiations (sizes verified by the stride of the helpers) ----------
struct elem24 { int a[6]; };               // 0x18: copy 0x461230, copy_backward 0x461280, fill 0x467ba0, uninit_copy 0x4681b0
#include "../../include/pod_elems.h"   // E8 / E12 / E100 / rec36 / rec136 (shared)
struct elem52 { int a[13]; };              // 0x34: fill 0x467bf0, copy_backward 0x467c20, uninit_copy 0x468290
struct particle100 { int a[25]; };         // 0x64: uninit_copy 0x468220 (not the 0xa0-byte terrain particle)
struct s516 { int a[129]; };               // 0x204: vec516::size 0x467b20
typedef bool (*Pr8)(const E8&, const E8&);

// ---- engine globals ---------------------------------------------------------------------------------------------
extern __int64 g_ticks[];                   // 0x5e2948 clock table, 1e-5 s ticks (indexed by a clock id)
extern __int64 g_ticks_dt[];                // 0x5e2970 per-clock last step (ticks)
extern float g_step_tbl[];                  // 0x5a9c20 per-clock step (seconds)

// cdecl helpers of earlier objects
void __cdecl noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0
void mb_error(const char* fmt, ...);                                // 0x42cc80
int __cdecl rand_below(const int* n);                               // 0x432580

#endif // TERRAIN_A_COMMON_H
