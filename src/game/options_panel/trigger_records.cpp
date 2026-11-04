#include "./trigger_records.h"
#include "../game_globals.h"   // g_trial_mode (canonical)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers

// ---- data this module owns (.bss, no initialiser; sizes from the code) ----
int g_5e29ac;                                       // 0x5e29ac


// @ 0x4d0620
void init_mission_trigger_records()
{
    WL_MARKER(6)
    slot_as<head_cam>(7) = new head_cam;
    slot_as<ResultWindow>(0) = new ResultWindow;
    slot_as<C4c3050>(1) = new C4c3050;
    slot_as<ItemRec>(4) = new ItemRec;
    slot_as<C4c3290>(2) = new C4c3290;
    slot_as<C4c2fb0>(3) = new C4c2fb0;
    slot_as<ItemRec2>(5) = new ItemRec2;
    slot_as<Rec118>(6) = new Rec118;
    slot_as<Recacc>(8) = new Recacc;
    slot_as<Rec38>(9) = new Rec38;
    slot_as<Rec44>(10) = new Rec44;
    slot_as<Rec34>(11) = new Rec34;
    slot_as<C4c3160>(12) = new C4c3160;
    slot_as<Rec454>(13) = new Rec454;
    slot_as<MissionBlocker>(14) = new MissionBlocker;
    slot_as<head_obj2>(15) = new head_obj2;
    slot_as<Rec1208>(16) = new Rec1208;
    slot_as<Rec3c>(17) = new Rec3c;
    slot_as<Recd8>(18) = new Recd8;
    slot_as<Recf0>(19) = new Recf0;
    slot_as<Rec1bc>(20) = new Rec1bc;
    slot_as<C4c31c0>(21) = new C4c31c0;
    slot_as<Rec23c>(22) = new Rec23c;
    slot_as<ItemA>(23) = new ItemA;
    slot_as<Rec48>(24) = new Rec48;
    slot_as<Rec58>(25) = new Rec58;
    slot_as<ItemD>(26) = new ItemD;
    slot_as<ItemB>(27) = new ItemB;
    slot_as<ItemC>(28) = new ItemC;
    slot_as<Rec428>(29) = new Rec428;
    g_5e29ac = g_trial_mode;
}
