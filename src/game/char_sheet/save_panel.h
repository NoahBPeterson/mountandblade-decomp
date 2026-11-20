#ifndef SAVE_PANEL_H
#define SAVE_PANEL_H
// Slice s004ea0c0: skin/bone rendering helpers of the save/load screen and the save-slot file logic.
//   0x4ea0c0  draw the skinned model of a party/troop preview (scaled per bone)
//   0x4ea290  save/load screen update (button handling, save, delete confirmation)
//   0x4ea850  select a save slot (show its two panels, hide the others)
// Names provisional; layouts follow the verified neighbouring slices.
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "../../rgl/rgl_math.h"

using rgl::vector4;

// ---- engine string (16 bytes) ---------------------------------------------------------------------------
#include "../tstring.h"

// ---- matrices / mesh (rgl) ------------------------------------------------------------------------------
#include "../../include/mat4.h"
using rgl::mat3;
using rgl::mat4;


#include <vector>
struct vertex60;
#include "../../rgl/mesh.h"
using rgl::mesh;
#include "../placed_obj.h"                              // PlacedObj: select_entity's object (scale, ent)
#include "../../rgl/scene/scene_entity.h"               // rgl::entity: its meshes

#endif
