// trigger_records.h (was slice s004d0620): creation of the mission-template trigger records (one heap object per record).
// Every record starts with the same head: vtable 0x5947d8, a zero word at +4, the 0x63 status word at +8
// and a tstring name at +0x1c; a derived record then writes its own vtable on top. Records built by
// constructors of other slices are declared opaque (size only); the others have their (tiny) constructors
// inlined into this function.
// NOTE: the original brackets the body with two inline-asm data tags (jmp +0x10 over "WL  ", 6, 0.. at the
// start; jmp +0x15 over "WL  ", 7, .. before the epilogue) that cannot be emitted from C++.
#ifndef OPTIONS_PANEL_TRIGGER_RECORDS_H
#define OPTIONS_PANEL_TRIGGER_RECORDS_H
#include <stddef.h>
#include "engine/core/tstring.h"
#include "engine/scene/frustum.h"

void* __cdecl operator new(size_t n);   // 0x53545e (scalar operator new)

#include <string.h>
#include "game/data/head_rec.h"

#include "game/ui/screens/screen_records.h"   // the record classes (vtables 0x595740..0x596c68)
#include "game/ui/screens/conversation/conv_screen.h"      // Rec1208: the conversation screen
#include "game/data/head_rec_rgl_mesh2.h"
#include "game/campaign/campaign_screen.h"          // CampaignScreen
#include "game/ui/screens/character_sheet/char_sheet_panel.h"         // CharSheet
#include "game/editor/terrain_page/terrain_page_game.h"             // TerrainPage
#include "game/ui/screens/face_generator/face_gen.h"                 // FaceGen
#include "game/ui/screens/party_screen_class.h"       // PartyScreen
#include "game/ui/screens/result_window.h"            // ResultWindow
#include "game/data/item_head_records.h"        // ItemRec, ItemRec2
#include "game/data/mission_item_recs.h"         // MissionBlocker, ItemA..ItemD

#include "game/ui/window_mgr.h"   // canonical: g_windows (0xa73980); the records are its slots (see trigger_records.cpp)
extern int g_5e29ac;   // 0x5e29ac

// @ 0x4d0620
void init_mission_trigger_records();
#endif
