// trigger_entry.h: one simple trigger (simple_triggers.txt; 0x30 bytes): the interval, the statement block and the clock it is
// timed on. The records of GameVec (game/trigger_vecs.h) and of the game's simple-trigger list (party/game.h SimpleRec, Game +0x139f4).
// Moved out of game/config/config.h so the game's headers can use the real record.
#ifndef MB_TRIGGER_ENTRY_H
#define MB_TRIGGER_ENTRY_H
#include "./dialog_rec.h"                           // DialogBlock
#include "./timer.h"                                // Timer

struct stream_base;
struct trigger_entry {
    float interval;                                 // +0 the simple trigger's interval
    DialogBlock block;                                    // +4
    Timer timer;                                    // +0x20 the clock index and the reading taken at it (ctor 0x4c1de0, stamp 0x4c1ee0)
    trigger_entry();                                // 0x4c1de0 (src/game/triggers)
    void read(stream_base& s, const char* name, int idx);   // 0x4bd910
};

#endif // MB_TRIGGER_ENTRY_H
