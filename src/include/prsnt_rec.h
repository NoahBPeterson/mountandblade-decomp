// prsnt_rec.h: PresentationRec, one presentation of presentations.txt (0x24 bytes): the records of g_presentations
// (PresentationTable, prsnt_table.h; built by fill_records 0x4c6950, read by PresentationRec::read 0x4c23f0). The menu code
// reads the same records (options_panel/hud.h MenuKind, cls_map/cls_map.h MenuKindRec: the trigger block as a TimerList).
#ifndef MB_PRSNT_REC_H
#define MB_PRSNT_REC_H
#include "../game/tstring.h"
#include "../game/trigger_vecs.h"   // GameVec

struct stream_base;
struct PresentationRec {    // module-file presentation (0x4c23f0)
    tstring name;           // +0x00
    int     flags;          // +0x10
    GameVec triggers;       // +0x14
    int     f20;            // +0x20
    void read(stream_base& s);                  // 0x4c23f0 (ret 4; game/triggers/trigger_vec.cpp)
};

#endif // MB_PRSNT_REC_H
