// prsnt_rec.h: PresentationRec, one presentation of presentations.txt (0x24 bytes): the records of g_presentations
// (PresentationTable, prsnt_table.h; built by fill_records 0x4c6950, read by PresentationRec::read 0x4c23f0). ONE definition: the
// menus (winmain_and_menu_screens.cpp widget_mgr::init / activate) and the presentation's per-frame handler (game/ui/hud/mission_hud_buttons.h) read the same records
// (were the MenuKindRec / MenuKind / RefItem views).
#ifndef MB_PRSNT_REC_H
#define MB_PRSNT_REC_H
#include "engine/core/tstring.h"
#include "game/data/trigger_vecs.h"   // SimpleTriggerVec

struct stream_base;
struct PresentationRec {    // module-file presentation (0x4c23f0)
    tstring name;           // +0x00
    int     flags;          // +0x10 prsntf_* (bit 0 read-only: widget_mgr::activate copies them)
    SimpleTriggerVec triggers;       // +0x14 its triggers (ti_on_presentation_load -60, run -61, event -62 / -63: update(id))
    int     mesh;           // +0x20 the background mesh (0 = none; widget_mgr::init 0x4dd920)
    void read(stream_base& s);                  // 0x4c23f0 (ret 4; game/app/options_dialog_and_module_tables.cpp)
};

#endif // MB_PRSNT_REC_H
