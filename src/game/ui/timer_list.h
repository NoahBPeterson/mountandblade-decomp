// timer_list.h: the sound holder TimerList (count of 0x30-byte TimerChild voices at +0, the child array at +4, a spare word at +8; update
// 0x48add0, destructor 0x4317b0) and its element TimerChild. One definition for ui/mission_ui.h (tick 0x488e90), options_panel/hud.h
// and cls_map.h (the holder inside a MenuKind / RefItem record at +0x14, update(0xc3 / 0xc2 / 0xc1)), ItemKind +0x228 and Rec78 +0x6c
// (the sound sets), PartyTypeDef +0x3c (the keyframe voices) and game+0x139f4; src/rgl/xmlcfg/records.cpp defines the destructor.
#ifndef GAME_UI_TIMER_LIST_H
#define GAME_UI_TIMER_LIST_H
#include "../../include/dialog_rec.h"      // DialogBlock: the voice's sound channel
#include "../../include/timer.h"           // Timer

// A timed voice: a period, one sound channel (0x1c bytes) and a clock; 0x30 bytes (0x488e90, 0x48add0).
struct TimerChild {
    float period;                           // +0x00 seconds between plays; < -1: play when round(period) == the request
    DialogBlock ch;                         // +0x04
    Timer last;                             // +0x20 clock index into g_ticks, +0x28 last play time (0x488e90)
    void tick(int idx);                     // 0x488e90
};

struct TimerList {                          // 0xc bytes
    int count;                              // +0
    TimerChild* items;                      // +4 (delete[]d by the destructor)
    int w8;                                 // +8
    TimerList() { count = 0; items = 0; w8 = 0; }
    ~TimerList();                           // 0x4317b0 (src/rgl/xmlcfg/records.cpp)
    void update(int flag);                  // 0x48add0
};
#endif // GAME_UI_TIMER_LIST_H
