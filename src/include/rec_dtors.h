// rec_dtors.h: RecK (0x34 bytes) and OwnedRec48 (0x48), engine records whose vector deleting destructors (??_E) are
// 0x4318c0 / 0x431960, with SubA / SubB, the (array, name) pairs they embed. ONE definition for src/rgl/xmlcfg (records.cpp:
// emits the destructors), src/game/static_init (RecK) and src/game/cls_map (OwnedRec48): each user writes `delete[]`, which
// calls the one ??_E.
#ifndef MB_REC_DTORS_H
#define MB_REC_DTORS_H
#include <new>
#include <vector>
#include "../game/tstring.h"
#include "./poly.h"          // Poly: the owned object of OwnedRec48

// A heap array pointer followed by two words and a name (0x1c bytes). Two destructor flavours occur: a
// null-checked free (SubA: 0x431440, 0x431480) and a bare `delete[]` (SubB: 0x4315d0), so two source types.
struct SubA {
    int* ptr;           // +0
    int a, b;
    tstring name;       // +0xc
    ~SubA() { if (ptr) delete[] ptr; }
};
struct SubB {
    int* ptr;
    int a, b;
    tstring name;
    ~SubB() { delete[] ptr; }
};

// (RecD / RecI were TriggerRec (include/trigger_rec.h) and menu_item (game/menu_rec.h): their destructors 0x431440 / 0x431660 and
// vector deleting destructors 0x431480 / 0x431810 are the compiler's, rows in game/triggers/trigger_vec.cpp)

// 0x34 bytes: a name at +4 and a pair at +0x18; vector deleting destructor 0x4318c0.
struct RecK {
    int head;
    tstring name;       // +4
    int pad;
    SubA s;             // +0x18
};
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
