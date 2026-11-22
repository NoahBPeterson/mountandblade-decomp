// trigger_vecs.h: the two counted trigger lists of the module files: TriggerVec (the module triggers, records of 0x78 bytes: TriggerRec of
// game/data/trigger_rec.h) and SimpleTriggerVec (the simple triggers, entries of 0x30 bytes: trigger_entry of game/data/trigger_entry.h).
// Both are {count, data, clock}: the clock is the one the records' timers start on (the game object builds both on clock 1, the
// module records on clock 0). ONE definition for the game object's lists (Game / GameData +0x139e8 / +0x139f4), a mission template's
// triggers, a presentation's, a scene prop's, an item kind's and a map icon's trigger lists; the readers 0x4c1c90 / 0x4c1e10, the stamp
// helpers and the savegame reader / writer are their members.
#ifndef GAME_TRIGGER_VECS_H
#define GAME_TRIGGER_VECS_H
struct stream_base;
struct stream_base;
struct TriggerRec;
struct trigger_entry;
struct TriggerVec {         // trigger records of 0x78 bytes (TriggerRec, game/data/trigger_rec.h)
    int         count;      // +0 (the savegame reader 0x51dd60: the file's count may not exceed it)
    TriggerRec* data;       // +4
    int         cur;        // +8 the clock the records' timers are stamped on (resize 0x4c1c90)
    // inline (the module loader's `new MissionTemplate[n]`, 0x50e7ef, and the game object's constructor 0x51cfa0 with clock 1: +4, +8, +0)
    TriggerVec(int clock = 0) { data = 0; cur = clock; count = 0; }
    void resize(stream_base& s, const char* name);  // 0x4c1c90 (ret 8)
    void stamp(int c);                          // 0x4c1d70
    void update_all(int arg);                   // 0x462760 (game/data/item_ref.cpp: every trigger's update)
    void randomize();                           // 0x462790 (game/data/item_ref.cpp: each check timer set back by a random part of its interval)
    void read(stream_base* s);                  // 0x51dd60 (game/campaign/game_state_load.cpp: the savegame's trigger states)
    __declspec(noinline) void write(stream_base* out);   // 0x4e90d0 (game/save/save_state.cpp)
};
struct SimpleTriggerVec {   // simple trigger entries of 0x30 bytes (trigger_entry)
    int            count;   // +0
    trigger_entry* data;    // +4 (delete[]d by the destructor)
    int            clock;   // +8 the clock the entries' timers start on (resize 0x4c1e10)
    // inline (the records' constructors: +0, +4, +8; the game object's 0x51cfa0 with clock 1)
    SimpleTriggerVec(int c = 0) { count = 0; data = 0; clock = c; }
    ~SimpleTriggerVec();                        // 0x4317b0 (game/data/record_destructors.cpp)
    int  resize(stream_base& s, const char* name, int extra);   // 0x4c1e10 (ret 0xc)
    void stamp(int c);                          // 0x4c1ee0
    void update(int flag);                      // 0x48add0 (game/ui/widgets/widget_base.cpp: every entry's tick; ret 4)
    void read(stream_base* s);                  // 0x51de00 (game/campaign/game_state_load.cpp: the savegame's timer stamps)
    __declspec(noinline) void write(stream_base* out);   // 0x4e9150 (game/save/save_state.cpp)
    void arm();                                 // 0x51f7c0 (game/campaign/new_game_setup.cpp: each timer set back by a random part of its interval)
};
#endif
