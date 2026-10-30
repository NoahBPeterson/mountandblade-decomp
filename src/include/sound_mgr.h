// sound_mgr.h: the FMOD-backed sound player (global g_sound / g_sound_mgr / SOUNDMGR at 0x5aa128) with its sound definitions and groups.
// ONE definition for src/game/agent3 (agent_fx_sound.cpp: play_slot 0x4428a0, SoundDef/SoundGroup scales), agent5 (agent_hud: start 0x4428a0 =
// play_slot, stop_all 0x494310), party2 (play 0x442650) and inventory_panel (play 0x442650). The includer must have declared vector4
// (rgl::vector4 of rgl_math.h or of match/include/rgl.h) and <vector>.
#ifndef MB_SOUND_MGR_H
#define MB_SOUND_MGR_H
#include <vector>
#include "./rgl_base.h"   // rgl::vector4 (whichever declaration the includer uses)

struct SoundDef {                                       // 0x14 bytes
    void* sample;                                       // +0
    int f4;                                             // +4 (bits 4..7: distance range)
    int id;                                             // +8
    int freq;                                           // +0xc
    int length;                                         // +0x10
    float range_scale();                                // 0x4355f0 (fastcall/thiscall)
};
struct SoundGroup {                                     // 0x98 bytes
    char pad0[0x10];
    unsigned flags;                                     // +0x10 (2: looping, 4: random start, bits 8..11: volume)
    int var[32];                                        // +0x14 variations (def indices)
    int nvar;                                           // +0x94
    float volume_scale();                               // 0x435620
};
struct SoundMgr {                                       // the sound player (g_sound 0x5aa128)
    char pad0[4];
    SoundDef* defs;                                     // +4
    int ngroups;                                        // +8
    SoundGroup* groups;                                 // +0xc
    char pad10[0x40 - 0x10];
    rgl::vector4 listener;                              // +0x40 (the listener position; 0x5aa138 is g_listener of agent_hud)
    int slot[(0x248 - 0x50) / 4];                       // +0x50 group index by sound id
    std::vector<int> channels;                          // +0x248 looping channels
    int play(int idx, int flags, const float* pos, float freq, int* out_id);   // 0x442650 (ret 0x14)
    int play_slot(int n, int flags, const float* pos, float freq, int* out_id);   // 0x4428a0 (ret 0x14; agent5/agent_hud called it start)
    void stop_all();                                    // 0x494310 (agent5/agent_hud.cpp)
};

extern SoundMgr g_sound_mgr; // 0x5aa128
#endif // MB_SOUND_MGR_H
