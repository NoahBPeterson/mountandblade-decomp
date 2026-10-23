// anim_slot.h: the agent's animation / order slot (2 x 0x2c bytes at agent+0xd4), ONE definition for
// src/game/agent2 (agent_stats.cpp holds the out-of-line AnimSlot::set 0x4378a0; agent_ai.cpp 0x43b100.. walks the
// slots) and src/game/agent3 (agent_fx_sound.cpp 0x444370 has set() inlined; it also reads anim[1].cur).
// set(): agent_stats.cpp (the TU of 0x4378a0) defines ANIM_SLOT_SET_OUT_OF_LINE before including this header so that
// the member stays a real function there; every other TU gets the body inline, as cl inlined it into 0x444370.
#ifndef GAME_ANIM_SLOT_H
#define GAME_ANIM_SLOT_H
struct AnimSlot {
    int cur;                                            // +0 current animation id (-1 none)
    int next;                                           // +4
    int sub;                                            // +8
    unsigned prev_mask;                                 // +0xc
    int f10;                                            // +0x10
    unsigned events;                                    // +0x14 priority word (low 17 bits; 0x444370)
    float scale;                                        // +0x18
    float f1c;                                          // +0x1c (agent_ai.cpp 0x43c9c0 passes it to Channel::set_phase)
    float rate;                                         // +0x20 (0x444370)
    float phase;                                        // +0x24 (>= 0: pending phase for set_rec_phase)
    int f28;                                            // +0x28
#ifdef ANIM_SLOT_SET_OUT_OF_LINE
    __declspec(noinline) bool set(int id, unsigned prio);   // 0x4378a0 (defined in agent_stats.cpp)
#else
    // 0x4378a0 as cl inlines it into 0x444370: accept a new animation when its priority is not lower
    bool set(int id, unsigned prio)
    {
        if ((prio & 0x1ffff) < (events & 0x1ffff) && (cur != -1 || next != -1))
            return false;
        next = id;
        events = prio;
        if (id == -1)
            events = 0;
        return true;
    }
#endif
};
#endif // GAME_ANIM_SLOT_H
