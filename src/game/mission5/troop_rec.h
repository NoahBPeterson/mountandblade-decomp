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

// ---- the text reader the new-game start builds on its stack (vtable 0x54ac88; inline ctor, as in module_load.h) -----
struct TextReader : file_reader_base {
    STREAM_SLOTS
    TextReader() {}
};

// ---- the scene's party table (Game +0xf8): the regions game.h's Scene leaves as padding ----------------------------
// free index list (IntVec, +0x138bc: allocator word, first, last, end; game.h's Zero3 f138c0 overlays its first three
// words) and the page array (PtrVec, +0x138cc); count at +0x138b8 (Scene::f138b8). 0x51eb90, 0x51eda0.
struct SceneView {
    int cells[0x2717 * 2];                          // +0        id hash: (party id, record index) pairs, -1 when empty (0x51eda0)
    int count;                                      // +0x138b8  0x51eb90
    IntVec free_ids;                                // +0x138bc  0x51eb90, 0x51eda0
    PtrVec pages;                                   // +0x138cc  0x51eb90 (page of 16 records of 0x5ac8 bytes)
    int f138dc;                                     // +0x138dc  0x51eb90 (last id read)
};

// the 0x5ac8-byte party record, seen as bytes: read() fills it field by field (0x51ee80). Rec5ac8 of
// src/game/char_sheet/save_records.h is the same record (its header cannot be included: it pulls in match/include/rgl.h).
struct PartyRecIO {
    char data[0x5ac8];
    __declspec(noinline) void read(stream_base* s, int flag);   // 0x51ee80 (ret 8)
};

// ---- the delay list at Game +0x139f4 (SimpleTag): 0x30-byte items {delay, ..., timer at +0x20} --------------------
struct DelayItem { float delay; char pad[0x1c]; Timer timer; };       // 0x30 bytes (0x51f7c0)

// ---- the record tables the start-up clears ---------------------------------------------------------------------
struct VecView { void* v0; void* v1; void* v2; void clear(); };         // std::vector<...>::clear 0x41f4b0 (shared out-of-line instance)

// ---- the item stack helpers of the troop item passes -----------------------------------------------------------------
extern float g_inv_gain;                            // 0x590b10 (0.13f)
extern float g_half;                                // 0x549d70 (0.5f)
extern float g_start_time;                          // 0x59bffc
extern int g_mode;                                  // 0x5a9fac

// the 4-bit skill fields of a troop's first word (+0x68), as the start-up writes them
struct NibWord {
    unsigned n0 : 4, n1 : 4, n2 : 4, n3 : 4, n4 : 4, n5 : 4, n6 : 4, n7 : 4;
};

#endif // MISSION5_TROOP_REC_H
