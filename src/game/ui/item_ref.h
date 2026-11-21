// item_ref.h: classes of item_ref.cpp (0x4625d0..0x462ac0): the sound-slot scheduler, the item kind record and
// the (item, modifier) pair that refers to one. Other modules of src/game/ui include this header for ItemKind
// and ItemRef. All names provisional; every offset is read by a function of the modules (cited by address).
#ifndef GAME_UI_ITEM_REF_H
#define GAME_UI_ITEM_REF_H
#include "../tstring.h"
#include "./ui_common.h"
#include "../../include/timer.h"

// item type / modifier constants (itp_*, imod_*), ItemMesh, ItemKind and ItemRef are the shared headers' (g_items 0x5e29d0)
#include "../../include/item_kind.h"
#include "../item_ref.h"

int __cdecl round_float(float* f);          // 0x432540 (round to nearest: floor(x) + (x - floor(x) > 0.5))

// ---- sound slots (0x4625d0..0x4627f0) --------------------------------------------------------------------
#include "../../include/dialog_rec.h"   // DialogBlock: the sound channels (0x1c bytes, play 0x4875e0, emit 0x4b09e0)
// The scheduler slot is the trigger record (TriggerRec, include/trigger_rec.h: update 0x4625d0) and the bank its list
// (TriggerVec, game/trigger_vecs.h: update_all 0x462760, randomize 0x462790); the slices called them snd_slot / snd_bank.
struct TriggerRec;
struct TriggerVec;
typedef TriggerRec snd_slot;                // (mission5/mission_5.cpp still spells them so; it includes the two headers)
typedef TriggerVec snd_bank;

// (the sort weights are g_item_mods[modifier].price / .rarity, include/item_mod.h: the original's 0xa75f90 is that table's +0x20)

#endif // GAME_UI_ITEM_REF_H
