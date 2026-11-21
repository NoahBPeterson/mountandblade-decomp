// data_files.h: the std::vector<tstring> instances the game-data loader (game_2.cpp: Game::load_game_data) and src/rgl/xmlcfg/records.cpp
// reach out of line: the loader's records (Troop, QuestRec, FactionRec: troop.h / game_recs.h) call out of line.
#ifndef MISSION5_DATA_FILES_H
#define MISSION5_DATA_FILES_H
#include <vector>
#include "../tstring.h"
#include "../../include/note_block.h"
#include "../item_ref.h"
#include "../../include/ptr_tag.h"             // ptr_tag: std::_Nonscalar_ptr_iterator_tag (empty, passed by value)
#include "../alloc_construct.h"                // alloc_tstring 0x436c90
struct stream_base;

// The out-of-line instances of std::vector<tstring> that other objects define (agent_combat.cpp 0x43e310, gui_manager 0x42d360 and
// 0x42d390) are called through their std:: names: an explicit specialization declared without a definition makes every use a call.
namespace std {
template<> void vector<tstring, allocator<tstring> >::_Insert_n(vector<tstring, allocator<tstring> >::iterator, size_t, const tstring&);   // 0x43e310
template<> void vector<tstring, allocator<tstring> >::_Destroy(tstring*, tstring*);                                                        // 0x42d360
template<> void vector<tstring, allocator<tstring> >::_Tidy();                                                                             // 0x42d390
}
// The faction record's note-name vector is the real std::vector<tstring>: game_2.cpp holds the out-of-line reserve instance
// (0x524bc0) with its _Uninit_copy (0x524cd0); push_back goes through the library's inline fill and _Insert_n 0x43e310.

#endif // MISSION5_DATA_FILES_H
