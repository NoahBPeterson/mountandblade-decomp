// The notes block of Troop, FactionRec, QuestRec and Party (and mission5/data_files.h's file records): eight report
// lines with the day each was written and the record's tableau mesh. read_soa 0x51e1f0 / write_soa 0x4e9620 take it
// whole. 0xa4 bytes at 32 bits.
#ifndef MB_NOTE_BLOCK_H
#define MB_NOTE_BLOCK_H

#include "../game/tstring.h"

struct SoaBlock {
    int times[8];                           // +0     the day of each note (g_game->string_count), -1 = hidden
    tstring texts[8];                       // +0x20  the note lines
    int tableau;                            // +0xa0  add_*_note_tableau_mesh, -1 = none
    SoaBlock() {}
    explicit SoaBlock(int tableau) : tableau(tableau) {}   // Party::Party 0x4934d0 (-1, stored between the notes and ignore_timer)
};

#endif
