// msg_log.h: the on-screen message log (object at 0x5d7080, g_msglog in agent_fx_sound.h): the enabled byte at +0x1714
// and the 32-entry ring at +0xb0b8. ONE definition for src/game (agent3/agent_fx_sound.cpp, which also declares the
// object) and src/rgl/xmlcfg/game_util.cpp (MsgLog::push 0x4334b0). Offsets: push 0x4334b0 (ring); enabled and the ring are also read by agent_fx_sound.cpp.
#ifndef MB_MSG_LOG_H
#define MB_MSG_LOG_H
#include "../game/tstring.h"

struct MsgLog {
    char pad0[0x1714];
    char enabled;                                       // +0x1714
    char pad1715[0xb0b8 - 0x1715];
    struct Ring {
        int count;                                      // +0xb0b8
        int head;                                       // +0xb0bc
        tstring names[32];                              // +0xb0c0
        int vals[32];                                   // +0xb2c0
    } ring;
    void push(const tstring& s, int v);                 // 0x4334b0
};

#endif // MB_MSG_LOG_H
