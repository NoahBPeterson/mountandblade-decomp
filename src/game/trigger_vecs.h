// trigger_vecs.h: the two counted record vectors of the module-file trigger tables: TriggerVec (records of 0x78 bytes, TriggerRec of
// party/game.h) and GameVec (entries of 0x30 bytes, trigger_entry of config/config.h). Both are {count, data, cursor}. One definition for
// triggers/trigger_vec.h (the readers 0x4c1c90 / 0x4c1e10 and the stamp helpers) and mission5/game_2.h (Game::load, which resizes the
// vectors at Game +0x139e8 / +0x139f4 through the same two functions): same addresses, same {count, array, cursor} layout.
#ifndef GAME_TRIGGER_VECS_H
#define GAME_TRIGGER_VECS_H
struct stream_base;
struct TriggerRec;
struct trigger_entry;
struct TriggerVec {         // trigger records of 0x78 bytes (TriggerRec, party/game.h)
    int         count;      // +0
    TriggerRec* data;       // +4
    int         cur;        // +8
    void resize(stream_base& s, const char* name);  // 0x4c1c90 (ret 8)
    void stamp(int c);                          // 0x4c1d70
};
struct GameVec {            // trigger entries of 0x30 bytes (trigger_entry, config/config.h)
    int            count;   // +0
    trigger_entry* data;    // +4
    int            cur;     // +8
    int  resize(stream_base& s, const char* name, int extra);   // 0x4c1e10 (ret 0xc)
    void stamp(int c);                          // 0x4c1ee0
};
#endif
