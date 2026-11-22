// trigger_entry.h: one simple trigger (simple_triggers.txt; 0x30 bytes): the interval, the statement block and the clock it is
// timed on. The records of SimpleTriggerVec (game/data/trigger_vecs.h) and of the game's simple-trigger list (game/campaign/game_state_load.h trigger_entry, Game +0x139f4).
// Moved out of game/config/config.h so the game's headers can use the real record.
#ifndef MB_TRIGGER_ENTRY_H
#define MB_TRIGGER_ENTRY_H
#include "game/dialog/dialog_rec.h"                           // DialogBlock
#include "engine/core/timer.h"                                // Timer

struct stream_base;
struct trigger_entry {
    float interval;                                 // +0 the simple trigger's interval
    DialogBlock block;                                    // +4
    Timer timer;                                    // +0x20 the clock index and the reading taken at it (ctor 0x4c1de0, stamp 0x4c1ee0)
    trigger_entry();                                // 0x4c1de0 (src/game/triggers)
    void read(stream_base& s, const char* name, int idx);   // 0x4bd910
    void tick(int idx);                             // 0x488e90 (game/ui/widgets/widget_base.cpp): an event entry (interval < -1) runs on its event,
                                                    // a timed one once interval seconds have passed since timer's start
};

#endif // MB_TRIGGER_ENTRY_H
