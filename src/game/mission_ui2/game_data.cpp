//-----------------------------------------------------------------------------
// File: game_data.cpp  (0x48fd60..0x490030)
//
// Desc: the game data's party tree operations: re-add the stacks of a party and of its attached parties to a destination
//       party, collect them into it, and the relation between the factions of two parties. Names provisional. Functions in
//       ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP.
//
// Boundary: class changes only (Party | GameData at 0x48fd60, GameData | Party at 0x490030); no forward call in or out.
//-----------------------------------------------------------------------------
#include "../tstring.h"
#include "../../include/party.h"
#include "../../include/game_recs.h"
#include "../../include/game_data.h"

// the party record of an id: the page pointer is added to the scaled slot as an offset (not PartyMgr::get_party's shape)
inline Party* party_at(PartyMgr* m, int k)
{
    return (Party*)((k & 0xf) * 0x5ac8 + (char*)*(m->pages.begin() + (k >> 4)));
}
inline Party* party_of(PartyMgr* m, int id) { return party_at(m, m->index_of(id)); }
#define PARTY(id) party_of(&parties, id)

// @ 0x48fd60
void GameData::fill_parties(int party, Party* dest)
{
    int i;
    for (i = 0; i < PARTY(party)->nstacks; i++) {
        dest->add_members(PARTY(party)->stacks[i].troop, PARTY(party)->stacks[i].count,
                          PARTY(party)->stacks[i].flags, PARTY(party)->stacks[i].wounded, 0);
    }
    unsigned j;
    for (j = 0; j < PARTY(party)->attached.size(); j++)
        fill_parties(PARTY(party)->attached.first[j], dest);
}

// @ 0x48ff70
void GameData::collect_attachments(int party, Party* dest)
{
    dest->nstacks = 0;
    fill_parties(party, dest);
    dest->prepare();
}

// @ 0x48ffa0
float GameData::relation(int a, int b)
{
    if (a == b)
        return 1.0f;
    PartyMgr* pm = &parties;
    return factions[party_of(pm, a)->f70].rel[party_of(pm, b)->f70];
}
