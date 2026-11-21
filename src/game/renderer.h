// renderer.h: classes of renderer.cpp (0x461630..0x4625d0): the Direct3D 9 render-state cache (renderer, global
// object 0xa17910), the material / mesh-part state applied through it, the view frustum's matrix helpers, the
// device vertex / index buffer pair restored after a device reset, a 2D cell grid and an envelope parameter
// block. Engine (rgl) code by its content (D3D state cache, effect uniforms); placed under src/game for now.
// All names provisional; every offset is read or written by a function of this module (cited by address).
#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "../rgl/rgl_math.h"
#include "./mat4.h"
#include "../include/frustum.h"
#include "shared_vars.h"

// ---- globals -----------------------------------------------------------------------------------------
extern int g_use_shaders;                   // 0x5e28fc
extern ID3DXEffect* g_effect;               // 0x5e293c
extern unsigned g_stage_arg[6];             // 0x5a9c00 D3DTA_* by 3-bit code
void mb_error(const char* fmt, ...);        // 0x42cc80

// ---- render-state cache (global object 0xa17910) ----------------------------------------------------------
#include "../include/renderer.h"                // renderer (shared: state cache here, debug drawer in src/rgl/shapes)

// ---- material / mesh part ------------------------------------------------------------------------------
#include "../rgl/resource.h"           // material: flags20 (bits 28/29: pixel shader), ps +0xbc, bind_pixel_shader 0x461f20

struct mesh_part : skin {            // the material (skin, rgl/resource.h) whose render state 0x461f80 applies: flags bit 3 z-write
                                    // off, bit 4 depth test off, bit 11 normalize normals, bits 12..13 alpha-test mode; its
                                    // shader's flags20 bit 5 turns specular on (spec_coef, color)
    void set_state();               // 0x461f80
};

// ---- view frustum: rgl::frustum of src/include/frustum.h (projection 0x4620d0, view_matrix 0x462170, to_clip 0x4621f0)
using rgl::frustum;
using rgl::plane;
// view_matrix is declared there with a D3DXMATRIX result (terrain_b reads it that way); renderer.cpp builds a mat4 and
// defines the function through this view of the same record
// mat4 from a rotation and an origin as renderer::set_view (0x461630) builds it (rgl::mat4 itself has no constructor)
struct mat4_ro : rgl::mat4 {
    mat4_ro(const rgl::rot3& r, const rgl::vector4& origin) { *(rgl::rot3*)this = r; o = origin; }
};
struct frustum_view : rgl::frustum {
    rgl::mat4 view_matrix();                // 0x462170 (hidden return pointer)
};

// ---- render-queue order: rec_less 0x4622a0 (renderer.cpp) compares the queued meshes' mesh::sortkey ----------
#include "../include/pod_elems.h"                        // E8: a queued mesh (m) and its weight
bool __cdecl rec_less(const E8& a, const E8& b);   // 0x4622a0

// ---- device vertex / index buffer pair (restore after reset)
#include "./dev_buffer.h"

// ---- 2D cell grid (rows of 16-byte cells; dtor 0x41ecc0 in an earlier object) ------------------------------
#include "../include/height_field.h"      // HCell: the 16-byte cell (its constructor is the one `new grid_cell[n]` ran)
#include "../include/render_flags.h"

// (the envelope / fade parameter block at world +0xdc is ViewParams' fog_start..water_fog_density; set_fade 0x4624c0 / set_params 0x462540 are rgl::world members, rgl/scene/world.h)

#endif // GAME_RENDERER_H
