// rec_dtors.h: OwnedRec48 (0x48), the engine record whose vector deleting destructor (??_E) is 0x431960. ONE definition
// for src/rgl/xmlcfg (records.cpp: emits the destructor) and src/game/cls_map (OwnedRec48): each user writes `delete[]`,
// which calls the one ??_E.
#ifndef MB_REC_DTORS_H
#define MB_REC_DTORS_H
#include <new>
#include <vector>
#include "../game/tstring.h"
#include "./poly.h"          // Poly: the owned object of OwnedRec48

// (RecD / RecI were TriggerRec (include/trigger_rec.h) and menu_item (game/menu_rec.h): their destructors 0x431440 / 0x431660 and
// vector deleting destructors 0x431480 / 0x431810 are the compiler's, rows in game/triggers/trigger_vec.cpp)

// (RecK, the 0x34-byte record of the vector deleting destructor 0x4318c0, is include/rec34.h's rec34: records.cpp emits it)

struct Owner { Poly* p; ~Owner() { delete p; } };

// 0x48 bytes: name, vector<int> at +0x10, owned polymorphic object at +0x24. Vector deleting dtor 0x431960.
struct OwnedRec48 {
    tstring name;               // +0
    std::vector<int> v;         // +0x10
    int pad20;
    Owner owned;                // +0x24
    char tail[0x20];
};

// std::vector<tstring>::_Tidy stays out of line (0x42d390, gui_manager.cpp's instance): the members' destructors call it
namespace std { template<> void vector<tstring, allocator<tstring> >::_Tidy(); }   // 0x42d390

// (the destructors of the troops.txt / quests.txt / factions.txt records, 0x431c40 / 0x431b10 / 0x432010, belong to TroopFile / QuestFile /
// FactionFile: src/game/mission5/data_files.h)

#endif // MB_REC_DTORS_H
