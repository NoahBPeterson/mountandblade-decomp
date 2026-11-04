// quest_page.h: the quest-log page object (the "quest" tab; also the Hud of hud.cpp: QuestPage::sel is Hud::f250, and
// Hud::on_update calls fill()).
#ifndef OPTIONS_PANEL_QUEST_PAGE_H
#define OPTIONS_PANEL_QUEST_PAGE_H
#include "../ui_widget.h"      // MissionUi

struct QuestPage {
    char    pad00[0x18];
    int     menu_option;                            // +0x18 (the option the "back" button picks)
    char    pad1c[0x34 - 0x1c];
    MissionUi* w34;                                    // +0x34  day caption
    MissionUi* w38;                                    // +0x38  toolbar: video
    MissionUi* w3c;                                    // +0x3c  toolbar: options
    MissionUi* w40;                                    // +0x40  toolbar: back
    MissionUi* w44;                                    // +0x44
    MissionUi* w48;                                    // +0x48
    MissionUi* w4c;                                    // +0x4c
    MissionUi* w50;                                    // +0x50
    MissionUi* w54;                                    // +0x54
    MissionUi* w58;                                    // +0x58
    MissionUi* w5c;                                    // +0x5c
    MissionUi* w60;                                    // +0x60
    MissionUi* w64;                                    // +0x64
    MissionUi* w68;                                    // +0x68
    MissionUi* w6c;                                    // +0x6c
    MissionUi* w70;                                    // +0x70
    MissionUi* w74;                                    // +0x74
    MissionUi* w78;                                    // +0x78  a slider (its float is the day scale)
    MissionUi* w7c;                                    // +0x7c
    MissionUi* w80;                                    // +0x80
    MissionUi* w84;                                    // +0x84
    MissionUi* w88;                                    // +0x88
    char    pad8c[0xd4 - 0x8c];
    MissionUi* wd4;                                    // +0xd4  difficulty caption
    char    pad_d8[0x250 - 0xd8];
    int     sel;                                    // +0x250 selected quest

    // the two text boxes of fill() (+0x44 name/date/description, +0x48 progression)
    MissionUi* box1() { return w44; }
    MissionUi* box2() { return w48; }

    void fill();
    void update_days();
    void update_difficulty();
};

#endif
