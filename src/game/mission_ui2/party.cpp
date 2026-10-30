//-----------------------------------------------------------------------------
// File: party.cpp  (0x48f790..0x48fd60)
//
// Desc: the game data's party operations: count the members of a troop in a party, add / remove members (reseeding the main
//       party's per-stack random table), fill a party from a party template, and follow the attached-to chain. Names
//       provisional. Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: class changes only (Mission | Party at 0x48f790, Party | GameData at 0x48fd60); no forward call in or out.
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <math.h>
#include "../tstring.h"
#include "../ui/ui_common.h"
#include "../item_ref.h"
#include "../../include/party.h"
#include "../../include/game_recs.h"
#include "../../include/game_data.h"

extern PartyTemplate* g_party_templates;        // 0x5e29f8 the party templates (0xb0 bytes each)

// the six member slots of a party template (+0x40): troop, minimum, maximum and flags (bit 0: prisoners)
struct TplSlot {
    int troop;                                  // +0x00 -1 when the slot is unused (0x48fbd0)
    int f4;                                     // +0x04
    int f8;                                     // +0x08
    int flags;                                  // +0x0c
};
struct PartyTemplateSlots {                     // the slot block of a PartyTemplate (game_recs.h, 0xb0 bytes); not the 0x20-byte mission template TplRec of map_editor/exchange_screens.h
    char pad0[0x40];
    TplSlot slots[6];                           // +0x40 (0x48fbd0)
    char pad_a0[0xb0 - 0xa0];
};

// the party record of an id: the page pointer is added to the scaled slot as an offset (not PartyMgr::get_party's shape)
inline Party* party_at(PartyMgr* m, int k)
{
    return (Party*)((k & 0xf) * 0x5ac8 + (char*)*(m->pages.begin() + (k >> 4)));
}
inline Party* party_of(PartyMgr* m, int id) { return party_at(m, m->index_of(id)); }
#define PARTY(id) party_of(&parties, id)



// @ 0x48f790
int GameData::count_troops(int troop, int party, char prisoners)
{
    int total = 0;
    int i = 0;
    if (PARTY(party)->nstacks > 0) {
        do {
            if (troop == PARTY(party)->stacks[i].troop) {
                if (prisoners != 0) {
                    if ((PARTY(party)->stacks[i].flags & 1) != 0)
                        goto add;
                } else if ((PARTY(party)->stacks[i].flags & 1) == 0) {
add:
                    total += PARTY(party)->stacks[i].count;
                }
            }
            i++;
        } while (i < PARTY(party)->nstacks);
    }
    return total;
}

// @ 0x48f8f0
int GameData::add_members(int party, int troop, int count, char prisoners)
{
    int total = count_troops(troop, party, prisoners);
    int r = PARTY(party)->add_members(troop, count, prisoners != 0, 0, 0);
    if (party == f78 && r >= 0 && r < 64) {
        int cnt = parties.get(party)->stacks[r].count;
        int end = cnt - count;
        int s = cnt < 0 ? 0 : (cnt >= 32 ? 31 : cnt);
        int e = end < 0 ? 0 : (end >= 32 ? 31 : end);
        if (e < s) {
            int* q = &tbl[r][e];
            int n = s - e;
            do {
                *q++ = rand();
            } while (--n != 0);
        }
    }
    return count_troops(troop, party, prisoners) - total;
}

// @ 0x48f9f0
int GameData::remove_troops(int party, int troop, int count, char prisoners, int a5)
{
    int i = 0;
    int found = -1;
    for (; i < PARTY(party)->nstacks; i++) {
        if (troop == PARTY(party)->stacks[i].troop) {
            if ((prisoners && (PARTY(party)->stacks[i].flags & 1))
                || (!prisoners && !(PARTY(party)->stacks[i].flags & 1)))
                found = i;
        }
    }
    if (found >= 0) {
        Stack* s = &PARTY(party)->stacks[found];
        if (s->count < count)
            count = s->count;
        if (party == f78)
            PARTY(party)->consume_stack(found, count, &tbl[0][0], a5);
        else
            PARTY(party)->consume_stack(found, count, 0, a5);
        return count;
    }
    return 0;
}

// @ 0x48fbd0
// The template fraction (player level and the tick counter) is clamped to [0,1] as a repeated expression: the original
// compares the unspilled x87 value, which a named float variable would spill.
#define TEMPLATE_T ((float)troops[player].level * 0.033f + (float)g_ticks[1] * 1e-09f)
void GameData::add_template(int party, int tmpl, int reverse)
{
    PartyTemplateSlots* r = (PartyTemplateSlots*)&g_party_templates[tmpl];
    Party* p = PARTY(party);
    for (int i = 0; i < 6; i++) {
        TplSlot* s = &r->slots[i];
        if (s->troop >= 0) {
            float t = TEMPLATE_T < 0.0f ? 0.0f : (TEMPLATE_T >= 1.0f ? 1.0f : TEMPLATE_T);
            int n = template_count(tmpl, i, t);
            if (n > 0)
                p->add_members(s->troop, n, (s->flags & 1) ? (reverse == 0) : (reverse != 0), 0, 0);
        }
    }
}

// @ 0x48fce0
int GameData::battle_root(int party)
{
    PartyMgr* pm = &parties;
    int cur = party;
    Party* p = party_of(pm, cur);
    while (p->f8c8 >= 0) {
        cur = p->f8c8;
        p = party_of(pm, cur);
    }
    return cur;
}
