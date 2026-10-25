#include "./trigger_records.h"

// @ 0x4d0620
void init_mission_trigger_records()
{
    g_rec_9c = new RecCam;
    (*(RecMenu**)&g_windows.arr[0]) = new RecMenu;
    (*(RecProfile**)&g_windows.arr[1]) = new RecProfile;
    g_rec_90 = new RecItem90;
    g_rec_88 = new Rec98b;
    (*(Rec90b**)&g_windows.arr[3]) = new Rec90b;
    (*(Rec80**)&g_windows.arr[5]) = new Rec80;
    (*(Rec118**)&g_windows.arr[6]) = new Rec118;
    (*(Recacc**)&g_windows.arr[8]) = new Recacc;
    g_rec_a4 = new Rec38;
    (*(Rec44**)&g_windows.arr[10]) = new Rec44;
    (*(Rec34**)&g_windows.arr[11]) = new Rec34;
    (*(Rec3910**)&g_windows.arr[12]) = new Rec3910;
    (*(Rec454**)&g_windows.arr[13]) = new Rec454;
    (*(RecBlocker**)&g_windows.arr[14]) = new RecBlocker;
    (*(RecObj2**)&g_windows.arr[15]) = new RecObj2;
    (*(Rec1208**)&g_windows.arr[16]) = new Rec1208;
    g_rec_c4 = new Rec3c;
    (*(Recd8**)&g_windows.arr[18]) = new Recd8;
    (*(Recf0**)&g_windows.arr[19]) = new Recf0;
    (*(Rec1bc**)&g_windows.arr[20]) = new Rec1bc;
    (*(Rec238**)&g_windows.arr[21]) = new Rec238;
    (*(Rec23c**)&g_windows.arr[22]) = new Rec23c;
    (*(RecItemA**)&g_windows.arr[23]) = new RecItemA;
    g_army_state = new Rec48;
    (*(Rec58**)&g_windows.arr[25]) = new Rec58;
    (*(RecItemD**)&g_windows.arr[26]) = new RecItemD;
    (*(RecItemB**)&g_windows.arr[27]) = new RecItemB;
    (*(RecItemC**)&g_windows.arr[28]) = new RecItemC;
    g_menu_state = new Rec428;
    g_5e29ac = g_trial_mode;
}
