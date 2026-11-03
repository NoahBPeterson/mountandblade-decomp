#include "./trigger_records.h"
#include "../game_globals.h"   // g_trial_mode (canonical)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers

// ---- data this module owns (.bss, no initialiser; sizes from the code) ----
int g_5e29ac;                                       // 0x5e29ac


// @ 0x4d0620
void init_mission_trigger_records()
{
    WL_MARKER(6)
    slot_as<RecCam>(7) = new RecCam;
    slot_as<RecMenu>(0) = new RecMenu;
    slot_as<RecProfile>(1) = new RecProfile;
    slot_as<RecItem90>(4) = new RecItem90;
    slot_as<Rec98b>(2) = new Rec98b;
    slot_as<Rec90b>(3) = new Rec90b;
    slot_as<Rec80>(5) = new Rec80;
    slot_as<Rec118>(6) = new Rec118;
    slot_as<Recacc>(8) = new Recacc;
    slot_as<Rec38>(9) = new Rec38;
    slot_as<Rec44>(10) = new Rec44;
    slot_as<Rec34>(11) = new Rec34;
    slot_as<Rec3910>(12) = new Rec3910;
    slot_as<Rec454>(13) = new Rec454;
    slot_as<RecBlocker>(14) = new RecBlocker;
    slot_as<RecObj2>(15) = new RecObj2;
    slot_as<Rec1208>(16) = new Rec1208;
    slot_as<Rec3c>(17) = new Rec3c;
    slot_as<Recd8>(18) = new Recd8;
    slot_as<Recf0>(19) = new Recf0;
    slot_as<Rec1bc>(20) = new Rec1bc;
    slot_as<Rec238>(21) = new Rec238;
    slot_as<Rec23c>(22) = new Rec23c;
    slot_as<RecItemA>(23) = new RecItemA;
    slot_as<Rec48>(24) = new Rec48;
    slot_as<Rec58>(25) = new Rec58;
    slot_as<RecItemD>(26) = new RecItemD;
    slot_as<RecItemB>(27) = new RecItemB;
    slot_as<RecItemC>(28) = new RecItemC;
    slot_as<Rec428>(29) = new Rec428;
    g_5e29ac = g_trial_mode;
}
