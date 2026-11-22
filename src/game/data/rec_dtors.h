// rec_dtors.h: the out-of-line std::vector<tstring>::_Tidy the record destructors of src/rgl/xmlcfg, src/game/world_data and
// src/game/options_panel call, and where their records' vector deleting destructors live.
#ifndef MB_REC_DTORS_H
#define MB_REC_DTORS_H
#include <new>
#include <vector>
#include "engine/core/tstring.h"

// (RecD / RecI were TriggerRec (game/data/trigger_rec.h) and menu_item (game/data/menu_rec.h): their destructors 0x431440 / 0x431660 and
// vector deleting destructors 0x431480 / 0x431810 are the compiler's, rows in game/app/options_dialog_and_module_tables.cpp)

// (RecK, the 0x34-byte record of the vector deleting destructor 0x4318c0, is game/data/rec34.h's rec34: record_destructors.cpp emits it)

// (the 0x48-byte record of the vector deleting destructor 0x431960 is game/data/ptype.h's AnimBox: record_destructors.cpp emits it)

// std::vector<tstring>::_Tidy stays out of line (0x42d390, gui_manager.cpp's instance): the members' destructors call it
namespace std { template<> void vector<tstring, allocator<tstring> >::_Tidy(); }   // 0x42d390

// (the destructors of the troops.txt / quests.txt / factions.txt records, 0x431c40 / 0x431b10 / 0x432010, belong to Troop / QuestRec / FactionRec: src/game/data/record_destructors.cpp)

#endif // MB_REC_DTORS_H
