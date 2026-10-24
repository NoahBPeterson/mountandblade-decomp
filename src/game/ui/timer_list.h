// timer_list.h: the sound holder TimerList (count of 0x30-byte TimerChild voices at +0, the child array at +4; update
// 0x48add0). One definition for ui/mission_ui.h (TimerChild layout, tick 0x488e90) and options_panel/hud.h (the holder
// inside a MenuKind at +0x14, update(0xc3 / 0xc2 / 0xc1)); party2 reaches the same object at game+0x139f4.
#ifndef GAME_UI_TIMER_LIST_H
#define GAME_UI_TIMER_LIST_H
struct TimerChild;
struct TimerList {
    int count;                              // +0
    TimerChild* items;                      // +4
    void update(int flag);                  // 0x48add0
};
#endif // GAME_UI_TIMER_LIST_H
