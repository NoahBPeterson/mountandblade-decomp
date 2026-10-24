// party.h: the party record (0x5ac8 bytes): a list of up to 64 troop stacks plus a 64-bit flag word. Members live
// in two modules: troop_inv.cpp (first_flagged 0x463190) and mission_ui.cpp (0x489420..0x489620). Names provisional.
// Layout from 0x489420 (flags at +0xa0), 0x489490..0x489540 (stacks at +0xa8, stride 0x20, count at +0x8a8),
// 0x489540 (dirty byte at +0x8b4, trailer fields at stack+0x88/+0x8c), 0x463190 (stack flags at +0xb0 = stack 0 + 8).
#ifndef GAME_UI_PARTY_H
#define GAME_UI_PARTY_H

#include "../../include/party.h"      // Party / Stack / PartyMgr: the party record (first_flagged 0x463190, set_id 0x489420, scale 0x489460, sum_all 0x489490, sum_head 0x4894b0, sum_flagged 0x4894e0, count_tail 0x489510, remove_row 0x489540, find_or_add 0x489620)

#endif // GAME_UI_PARTY_H
