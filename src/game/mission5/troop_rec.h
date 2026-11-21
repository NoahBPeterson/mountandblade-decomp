// troop_rec.h: declarations of src/game/mission5/troop_rec.cpp (TU 114 "troop_rec", 0x51eb90..0x520b20): the scene half of the
// save loader (Scene::read), the new-game start-up (Game::start_new_game, more_start), the troop item / level helpers and the
// animator re-seed. The classes are the shared ones (Game / Scene in src/game/party/game.h, Troop / TroopRec in
// src/include/troop.h, ItemRef, Animator in src/game/mesh_slot/mesh_slot.h); this header adds only local views of the
// regions those headers leave as padding (named View) and the globals this object touches. Offsets cite the function
// that proves them.
#ifndef MISSION5_TROOP_REC_H
#define MISSION5_TROOP_REC_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../party/game.h"
#include "../ui/item_ref.h"
#include "../skill_cap.h"
#include "../../include/troop.h"
#include "../../include/timer.h"
#include "../../include/item_kind.h"
#include "../../include/game_recs.h"
#include "../mesh_slot/mesh_slot.h"        // Animator (g_anim, 0x8ed4f60)

void __cdecl mb_error(const char* fmt, ...);        // 0x42cc80
float frand();                                      // 0x40e400 (returns st0)

// ---- the text reader the new-game start builds on its stack (vtable 0x54ac88, tstream3; constructor inline) -----
#include "../../rgl/stream_inline.h"

// ---- the delay list at Game +0x139f4 (SimpleTriggerVec): 0x30-byte items {delay, ..., timer at +0x20} --------------------

// ---- the record tables the start-up clears ---------------------------------------------------------------------
// the records' slots (SlotArray): VC7.1's clear() is _Tidy(), out of line here (the frameless body /OPT:ICF folded into
// 0x41f4b0, entity.cpp's vec<mface>::clear)
#include <vector>
template<> void std::vector<__int64, std::allocator<__int64> >::clear();   // 0x41f4b0

// ---- the item stack helpers of the troop item passes -----------------------------------------------------------------
extern const float g_half;                                // 0x549d70 (0.5f)
extern float g_inv_gain;                            // 0x590b10 (0.13f)
extern float g_start_time;                          // 0x59bffc
#include "../config/config.h"

// the 4-bit skill fields of a troop's first word (+0x68), as the start-up writes them
struct NibWord {
    unsigned n0 : 4, n1 : 4, n2 : 4, n3 : 4, n4 : 4, n5 : 4, n6 : 4, n7 : 4;
};

#endif // MISSION5_TROOP_REC_H
