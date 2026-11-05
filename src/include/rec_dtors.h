// rec_dtors.h: RecD (0x78 bytes), RecI (0x68), RecK (0x34) and OwnedRec48 (0x48), engine records whose vector deleting
// destructors (??_E) are 0x431480 / 0x431810 / 0x4318c0 / 0x431960, with SubA / SubB, the (array, name) pairs they embed.
// ONE definition for src/rgl/xmlcfg (records.cpp: emits the destructors), src/rgl/mesh2 (rec_tables.cpp deletes a RecD,
// gui_edit.cpp a RecI), src/game/static_init (RecK) and src/game/cls_map (OwnedRec48): each user writes `delete[]`, which
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

// 0x78 bytes: two (array, name) pairs. ~RecD 0x431440, vector deleting destructor 0x431480.
struct RecD {
    int pad[4];
    SubA s1;            // +0x10
    SubA s2;            // +0x2c
    int tail[12];
    ~RecD();   // 0x431440
    void* scalar_delete(unsigned flags);        // forwards to ??_ERecD 0x431480 (records.cpp): rec_tables.cpp (elem58) and world_data.cpp call it because `delete[]` there changes the registers of the matched functions
};
// 0x68 bytes: three names, two pairs. ~RecI 0x431660, vector deleting destructor 0x431810.
struct RecI {
    tstring n0, n1, n2; // +0, +0x10, +0x20
    SubA s1;            // +0x30
    SubA s2;            // +0x4c
    ~RecI();
    void* destroy(unsigned flags);              // forwards to ??_ERecI 0x431810 (records.cpp): gui_edit.cpp (elem60), same reason
};

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
