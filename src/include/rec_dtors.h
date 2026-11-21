// rec_dtors.h: the out-of-line std::vector<tstring>::_Tidy the record destructors of src/rgl/xmlcfg, src/game/world_data and
// src/game/options_panel call, and where their records' vector deleting destructors live.
#ifndef MB_REC_DTORS_H
#define MB_REC_DTORS_H
#include <new>
#include <vector>
#include "../game/tstring.h"

// (RecD / RecI were TriggerRec (include/trigger_rec.h) and menu_item (game/menu_rec.h): their destructors 0x431440 / 0x431660 and
// vector deleting destructors 0x431480 / 0x431810 are the compiler's, rows in game/triggers/trigger_vec.cpp)

// (RecK, the 0x34-byte record of the vector deleting destructor 0x4318c0, is include/rec34.h's rec34: records.cpp emits it)

// (the 0x48-byte record of the vector deleting destructor 0x431960 is game/ptype.h's AnimBox: records.cpp emits it)

// std::vector<tstring>::_Tidy stays out of line (0x42d390, gui_manager.cpp's instance): the members' destructors call it
namespace std { template<> void vector<tstring, allocator<tstring> >::_Tidy(); }   // 0x42d390

// (the destructors of the troops.txt / quests.txt / factions.txt records, 0x431c40 / 0x431b10 / 0x432010, belong to TroopFile / QuestFile /
// FactionFile: src/game/mission5/data_files.h)

#endif // MB_REC_DTORS_H
