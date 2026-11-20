// trigger_rec.h: TriggerRec, the module trigger (0x78 bytes: triggers.txt and a mission template's triggers). ONE definition
// for party/game.h (Game's trigger list +0x139e8), mode_rec.h (MissionTemplate::triggers, deleted by ~MissionTemplate),
// triggers/trigger_vec.cpp (the constructor 0x4c1c20 and TriggerVec::resize 0x4c1c90, whose `delete[]` emits the implicit
// destructor 0x431440 and the vector deleting destructor 0x431480) and world_data.cpp (GameData::world_free).
#ifndef MB_TRIGGER_REC_H
#define MB_TRIGGER_REC_H
#include "./dialog_rec.h"                   // DialogBlock: the condition and consequence blocks
#include "./timer.h"                        // gui_timer: the three interval timers
struct stream_base;
struct OutStream;                           // char_sheet/save_state.h: the save-game writer

struct TriggerRec {                         // 0x78 bytes: element of the trigger list (+0x139e8); the module-file trigger record
    float check_interval;                   // +0x00  the module trigger's (check, delay, rearm) intervals in the triggers.txt order,
                                            //        read first by trigger_rec::read 0x4bd830 (src/game/config); <= -0.1: an event
                                            //        trigger, its rounded value is the event id (update 0x4625d0)
    float delay_interval;                   // +0x04
    float rearm_interval;                   // +0x08
    int state;                              // +0x0c  0 waiting, 1 condition passed (delaying), 2 rearming (update; 0x51ddc0 r_int; 0x4c1c20 clears it last)
    DialogBlock cond;                       // +0x10  condition block: items +0x10, count +0x14, done +0x18, text +0x1c (ctor 0x4c1c20)
    DialogBlock cons;                       // +0x2c  consequence block: items +0x2c, count +0x30, done +0x34, text +0x38
    gui_timer check_timer;                  // +0x48  since the last check (the start is read by 0x51ddc0, stamped by 0x4c1d70)
    gui_timer delay_timer;                  // +0x58  since the condition passed
    gui_timer rearm_timer;                  // +0x68  since the consequences ran
    TriggerRec();                           // 0x4c1c20 (src/game/triggers/trigger_vec.cpp)
    // implicit destructor 0x431440 (the blocks' `delete[] items`, their texts), vector deleting destructor 0x431480
    __declspec(noinline) void read(stream_base* s);        // 0x51ddc0
    __declspec(noinline) void write(OutStream* out);       // 0x4e9110 (char_sheet/save_state.cpp)
    void update(int arg);                   // 0x4625d0 (ui/item_ref.cpp: the check / delay / rearm state machine; arg is the event)
};

#endif // MB_TRIGGER_REC_H
