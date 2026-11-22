// new_game_setup.h: declarations of src/game/campaign/new_game_setup.cpp (TU 114 "troop_rec", 0x51eb90..0x520b20): the scene half of the
// save loader (Scene::read), the new-game start-up (Game::start_new_game, more_start), the troop item / level helpers and the
// animator re-seed. The classes are the shared ones (Game / Scene in src/game/campaign/game_state_load.h, Troop / Troop in
// src/game/data/troop.h, ItemRef, Animator in src/game/mission/mesh_slot.h); this header adds only local views of the
// regions those headers leave as padding (named View) and the globals this object touches. Offsets cite the function
// that proves them.
#ifndef MISSION5_TROOP_REC_H
#define MISSION5_TROOP_REC_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "game/campaign/game_state_load.h"
#include "game/data/item_ref_game_ui.h"
#include "game/data/skill_cap.h"
#include "game/data/troop.h"
#include "engine/core/timer.h"
#include "game/data/item_kind_include.h"
#include "game/data/game_recs.h"
#include "game/mission/mesh_slot.h"        // Animator (g_anim, 0x8ed4f60)

void __cdecl mb_error(const char* fmt, ...);        // 0x42cc80
float frand();                                      // 0x40e400 (returns st0)

// ---- the text reader the new-game start builds on its stack (vtable 0x54ac88, tstream3; constructor inline) -----
#include "engine/io/stream_inline.h"

// ---- the delay list at Game +0x139f4 (SimpleTriggerVec): 0x30-byte items {delay, ..., timer at +0x20} --------------------

// ---- the record tables the start-up clears ---------------------------------------------------------------------
// the records' slots (SlotArray): VC7.1's clear() is _Tidy(), out of line here (the frameless body /OPT:ICF folded into
// 0x41f4b0, entity.cpp's std::vector<entity*>::_Tidy)
#include <vector>
template<> void std::vector<__int64, std::allocator<__int64> >::clear();   // 0x41f4b0

// ---- the item stack helpers of the troop item passes -----------------------------------------------------------------
extern const float g_half;                                // 0x549d70 (0.5f)
extern float g_inv_gain;                            // 0x590b10 (0.13f)
extern float g_start_time;                          // 0x59bffc
#include "game/config/config.h"

// the 4-bit skill fields of a troop's first word (+0x68), as the start-up writes them
struct NibWord {
    unsigned n0 : 4, n1 : 4, n2 : 4, n3 : 4, n4 : 4, n5 : 4, n6 : 4, n7 : 4;
};

#endif // MISSION5_TROOP_REC_H
