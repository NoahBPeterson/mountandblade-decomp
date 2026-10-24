// emitter_rec.h: the script emitter record (0x34 bytes; array pointer g_emitters 0xa75eb0, parallel index array g_emitter_idx 0xa75ebc,
// count 0xa75eb4). +4 the name, +0x14 a float (game_2.cpp), +0x18 the event emitter 0x4b09e0 (GameData::emit_event 0x435790).
// ONE definition for src/game (agent/agent.h, inventory_panel, mission5/game_2.h).
#ifndef MB_EMITTER_REC_H
#define MB_EMITTER_REC_H
#include "./tstring.h"

struct EmitSub {
    char emit(int flags, int zero, int* count, __int64* vals, unsigned tag);   // 0x4b09e0
};
struct Emitter {                                 // 0x34 bytes, array g_emitters
    char pad0[4];
    tstring name;                                // +4
    float f14;                                   // +0x14
    EmitSub sub;                                 // +0x18
    char pad19[0x34 - 0x19];
};
extern Emitter* g_emitters;                      // 0xa75eb0  // old name of g_scripts, removed after the names merge
extern Emitter* g_scripts; // 0xa75eb0
extern int g_emitter_idx[];                      // 0xa75ebc

#endif // MB_EMITTER_REC_H
