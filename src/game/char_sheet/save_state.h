#ifndef SAVE_STATE_H
#define SAVE_STATE_H
// Slice s004e8f10 (batch ds1): the game-state serialiser. Game::save_state (0x4e8f10) walks the whole game
// object in a fixed order and hands each block to the binary writer; the rest of the range is the family of
// per-block writers it (and the next slice) uses: the 8-byte element vector writer (0x4e94b0), the
// count+pointer+stride lists (0x4e90d0 / 0x4e9150 / 0x4e9550 / 0x4e9670 / 0x4e96c0 / 0x4e9790), the fixed
// arrays (0x4e9510 / 0x4e95a0 / 0x4e97e0) and the struct-of-arrays helper (0x4e9620).
// Every offset below is read off the disassembly of this range; names are provisional.
// The stream is the binary writer of the save files: slot 0xc writes an int, 0x10 writes a pointer-sized
// int, 0x14 writes a 64-bit value, 0x18 writes an 8-byte element, 0x1c writes a count, 0x20 writes raw bytes,
// 0x28 writes a C string and 0x2c writes a nested structure. Every call returns the stream (eax).
#include <windows.h>
#include <new>
#include "../../rgl/rgl_math.h"

#define MB_NOTHROW throw()

// ---- tstring (see s0042aaf0 / s004e2ea0) ----------------------------------------------------------------
#include "../tstring.h"

// ---- the writer: src/game/out_stream.h ---------------------------------------------------------------------
#include "../out_stream.h"

#include "../../include/fx.h"                        // g_ticks (0x5e2948): the clock table, g_ticks[1] is the 64-bit counter at 0x5e2950

// the record the scalar block writes: src/game/track_rec.h; the 8-byte element vector: src/game/slot_array.h; the 0x508-byte
// game record (GameRecord in the slices) is the troop record, src/include/troop.h
#include "../track_rec.h"
#include "../slot_array.h"
#include "../../include/troop.h"

// the 0xb0-byte records of g_party_templates are PartyTemplate (include/game_recs.h: write 0x4e9510)

// the 0x138-byte records at +0x13a58 are QuestRec (include/game_recs.h: write 0x4e95a0)

// the 0x300-byte records at +0x13a48 are FactionRec (include/game_recs.h: write300 0x4e9710)

// the trigger lists' writers are TriggerVec::write / SimpleTriggerVec::write (include/trigger_tags.h), TriggerRec::write (party/game.h)

// ---- the game object (see s00473a60 / s004e85d0) ---------------------------------------------------------

#include "../../include/game_data.h"            // GameData (g_game, 0x5e2930): the one definition (was a Game view here)
#include "../party/game.h"                       // TriggerRec, trigger_entry (the trigger records the writers walk)
#include "../event.h"                         // Event: GameData::events is the Pool<Event> (write_table 0x4e99d0)


#include "../game_globals.h"
#include "../../include/item_kind.h"
#include "../../include/game_recs.h"
extern ItemKind* g_items;                           // 0x5e29d0 (array of 0x238-byte item kinds)


// ---- the sub-writers ------------------------------------------------------------------------------------
struct SoaBlock;
OutStream* write_soa(OutStream* out, SoaBlock* b);   // 0x4e9620 (free, two stack args)

#endif
