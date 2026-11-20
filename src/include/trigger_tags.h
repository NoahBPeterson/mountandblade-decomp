// trigger_tags.h: the two trigger lists of the game object (party/game.h Game, include/game_data.h GameData): the module triggers
// (TriggerTag, +0x139e8) and the simple triggers (SimpleTag, +0x139f4), each a capacity, the array and a constant 1 stored by the
// game's constructor 0x51cfa0 (the inline constructors keep its store order). One definition for both views of the object.
#ifndef MB_TRIGGER_TAGS_H
#define MB_TRIGGER_TAGS_H
struct stream_base;
struct TriggerRec;                          // party/game.h: a module trigger (0x78 bytes)
struct SimpleRec;                           // party/game.h: a simple trigger (0x30 bytes)
struct TriggerTag {                         // Game +0x139e8: capacity, array, constant 1 (inline ctor stores +4, +8, +0)
    int count;                              // +0   0x51dd60 (the file's count may not exceed it)
    TriggerRec* array;                      // +4
    int one;                                // +8   0x51cfa0
    TriggerTag() { array = 0; one = 1; count = 0; }
    void read(stream_base* s);              // 0x51dd60
    __declspec(noinline) void write(struct OutStream* out);   // 0x4e90d0 (char_sheet/save_state.cpp)
};
struct SimpleTag {                          // Game +0x139f4 (inline ctor stores +0, +4, +8)
    int count;                              // +0   0x51de00
    SimpleRec* array;                       // +4
    int one;                                // +8   0x51cfa0
    SimpleTag() { count = 0; array = 0; one = 1; }
    void read(stream_base* s);              // 0x51de00
    __declspec(noinline) void write(struct OutStream* out);   // 0x4e9150 (char_sheet/save_state.cpp)
    void arm();                             // 0x51f7c0 (src/game/mission5: re-arms the 0x30-byte delay items, timer at +0x20)
};
#endif // MB_TRIGGER_TAGS_H
