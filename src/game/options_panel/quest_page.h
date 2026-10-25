// quest_page.h: the quest-log page object (the "quest" tab; also the Hud of hud.cpp: QuestPage::sel is Hud::f250, and
// Hud::on_update calls fill()).
#ifndef OPTIONS_PANEL_QUEST_PAGE_H
#define OPTIONS_PANEL_QUEST_PAGE_H
#include "../ui_widget.h"      // Widget

struct QuestPage {
    char    pad00[0x18];
    int     menu_option;                            // +0x18 (the option the "back" button picks)
    char    pad1c[0x34 - 0x1c];
    Widget* w34;                                    // +0x34  day caption
    Widget* w38;                                    // +0x38  toolbar: video
    Widget* w3c;                                    // +0x3c  toolbar: options
    Widget* w40;                                    // +0x40  toolbar: back
    Widget* w44;                                    // +0x44
    Widget* w48;                                    // +0x48
    Widget* w4c;                                    // +0x4c
    Widget* w50;                                    // +0x50
    Widget* w54;                                    // +0x54
    Widget* w58;                                    // +0x58
    Widget* w5c;                                    // +0x5c
    Widget* w60;                                    // +0x60
    Widget* w64;                                    // +0x64
    Widget* w68;                                    // +0x68
    Widget* w6c;                                    // +0x6c
    Widget* w70;                                    // +0x70
    Widget* w74;                                    // +0x74
    Widget* w78;                                    // +0x78  a slider (its float is the day scale)
    Widget* w7c;                                    // +0x7c
    Widget* w80;                                    // +0x80
    Widget* w84;                                    // +0x84
    Widget* w88;                                    // +0x88
    char    pad8c[0xd4 - 0x8c];
    Widget* wd4;                                    // +0xd4  difficulty caption
    char    pad_d8[0x250 - 0xd8];
    int     sel;                                    // +0x250 selected quest

    // the two text boxes of fill() (+0x44 name/date/description, +0x48 progression)
    Widget* box1() { return w44; }
    Widget* box2() { return w48; }

    void fill();
    void update_days();
    void update_difficulty();
    void update_settings(int reason);
};

#endif
