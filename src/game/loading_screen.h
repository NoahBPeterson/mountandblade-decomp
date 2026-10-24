// loading_screen.h: the loading-screen object (a screen like the menus; one definition for cls_map, inventory_panel/game_app.cpp
// and inventory_panel/loading_screen.cpp). init 0x4daa10 (cls_map.cpp) builds the overlay window (+0x2c) and three child widgets
// and clears step / total (+0x3c / +0x40); update 0x50f390 (game_app.cpp) sets the text of the widget at +0x30 and the value of the
// one at +0x38 from the step counter; advance 0x50f440 (loading_screen.cpp) runs one loading step on the same step / total
// (+0x3c / +0x40) and keeps the module-info table pointer at +0x44.
#ifndef GAME_LOADING_SCREEN_H
#define GAME_LOADING_SCREEN_H
#include "./tstring.h"
struct ContainerBase;
struct MissionUi;
struct LoadingScreen {
    char pad0[0x2c];
    ContainerBase* overlay;                     // +0x2c the window (vtable 0x592848)
    MissionUi* hint;                            // +0x30 the hint line (make_hint); update 0x50f390 sets its text
    MissionUi* text;                            // +0x34 the random "ui_hint" text line (flag 0x100)
    MissionUi* logo;                            // +0x38 the "warrider_logo" picture; update 0x50f390 sets its value
    int step;                                   // +0x3c (init 0x4daa10 clears it; advance 0x50f440, update 0x50f390)
    int total;                                  // +0x40
    char* f44;                                  // +0x44 (advance 0x50f440)
    int f48;                                    // +0x48 (advance 0x50f440)
    void init();                                // 0x4daa10
    void update(int unused);                    // 0x50f390
    bool advance(int* step, int* total, tstring* text);   // 0x50f440
};
#endif // GAME_LOADING_SCREEN_H
