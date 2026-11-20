// curve.h: the animated-parameter record (EnvRec, 0xd8 bytes) of curve.cpp (0x4338e0..0x433bf0) and the string hash
// next to it. Every offset below is read by a function of this module (cited); the rest is padding.
// Curve::phase (0x4231f0) lives in an earlier object (s00422db0) and is only declared here; Curve::step_reached
// (0x433840) is at the end of the previous object in the TU map (see MIGRATION.md) and is declared for completeness.
#ifndef AGENT_CURVE_H
#define AGENT_CURVE_H
#include <math.h>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"

using rgl::vector4;

#include "../../include/quaternion.h"
#include "../../include/timer.h"

#include "../../include/fx.h"
#include "../../include/clock_table.h"

// The 0xd8-byte record is EnvRec (include/fx.h): this module defines its phase helpers (unshape, set_phase) and the
// controller's per-record calls (FxDef::reset_rec / rec_shaped_phase / set_rec_phase on FxDef::env). EnvRec::phase
// 0x4231f0 and shape 0x4232b0 are src/rgl/dynamic/fx_keys.cpp's.

unsigned __stdcall hash_string(const tstring* s);           // 0x433bb0 (h = (c + h * 59) % 3571)

#endif // AGENT_CURVE_H
