// trigger_entry.h: one simple trigger (simple_triggers.txt; 0x30 bytes): the interval, the statement block and the clock it is
// timed on. The records of GameVec (game/trigger_vecs.h) and of the game's simple-trigger list (party/game.h SimpleRec, Game +0x139f4).
// Moved out of game/config/config.h so the game's headers can use the real record.
#ifndef MB_TRIGGER_ENTRY_H
#define MB_TRIGGER_ENTRY_H
#include "./dialog_rec.h"                           // DialogBlock

struct stream_base;
struct trigger_entry {
    float interval;                                 // +0 the simple trigger's interval
    DialogBlock block;                                    // +4
    int clock;                                      // +0x20 clock index (ctor 0x4c1de0, stamp 0x4c1ee0)
    int pad24;
    __int64 t;                                      // +0x28 the clock reading taken at it
    trigger_entry();                                // 0x4c1de0 (src/game/triggers)
    void read(stream_base& s, const char* name, int idx);   // 0x4bd910
};

#endif // MB_TRIGGER_ENTRY_H
