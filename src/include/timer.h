// timer.h: the engine's 16-byte tick timer: a clock index into the 64-bit clock table (global 0x5e2948, 100 kHz
// units, 1e-5 s per tick) and the start tick. Global namespace (manifest rows of src/rgl/xmlcfg/game_util bind it).
// Embedded as a 0x10-byte member: src/game/terrain_b/mission.h (Mission::tm +0x5de0, next field +0x5df0),
// src/game/agent2/agent_types.h (Agent::timer +0x57f0, next pad at +0x5800), src/game/script/script_interpreter.h
// (Party::ignore_player_timer +0x5ab0, set by party_ignore_player 0x4a7b0c).
//   restart     0x432bf0: start = clock tick
//   elapsed     0x432c10: (float)(ticks[clock] - start) * 1e-5f  (callers inline the arithmetic; the callable
//                         copy is the out-of-line function of game_util.cpp)
//   set_delay   0x42cd20 (ret 4), defined in src/game/strat/gui_manager.cpp as gui_timer::set_delay
// Not merged: terrain_b/agent_rec.h's Timer16 keeps the start tick as two dwords on purpose (the agent's view
// sub-object is 0x5374 bytes long, so its timers are only 4-byte aligned; an __int64 member would pad them).
#ifndef MB_TIMER_H
#define MB_TIMER_H

struct Timer {
    int clock;                              // +0 index into g_time (0x5e2948: 64-bit counters, 100 kHz units)
    int pad4;
    __int64 start;                          // +8
    void restart();                         // 0x432bf0
    float elapsed() const;                  // 0x432c10
    void set_delay(float seconds);          // 0x42cd20 (ret 4)
};

// a Timer followed by an armed flag (game_util.cpp: 0x432da0 arms it once, 0x432de0 disarms it when it has run out)
struct ActiveTimer : Timer {
    char active;                            // +0x10
    void begin(float seconds);              // 0x432da0
    bool expired();                         // 0x432de0
};

#endif // MB_TIMER_H
