// sound_mgr.h: the FMOD-backed sound player (global g_sound / g_sound_mgr / SOUNDMGR at 0x5aa128) with its sound definitions and groups.
// ONE definition for src/game/agent3 (agent_fx_sound.cpp: play_slot 0x4428a0, SoundDef/SoundGroup scales), agent5 (agent_hud: start 0x4428a0 =
// play_slot, stop_all 0x494310), party2 (play 0x442650) and inventory_panel (play 0x442650). The includer must have declared vector4
// (rgl::vector4 of rgl_math.h or of match/include/rgl.h) and <vector>.
#ifndef MB_SOUND_MGR_H
#define MB_SOUND_MGR_H
#include <vector>
#include "../game/tstring.h"
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
    tstring name;                                       // +0
    unsigned flags;                                     // +0x10 (2: looping, 4: random start, bits 8..11: volume)
    int var[32];                                        // +0x14 variations (def indices)
    int nvar;                                           // +0x94
    float volume_scale();                               // 0x435620
};
struct SoundMgr {                                       // the sound player (g_sound 0x5aa128)
    int ndefs;                                          // +0 (music_set::count: rec_tables.h)
    SoundDef* defs;                                     // +4
    int ngroups;                                        // +8
    SoundGroup* groups;                                 // +0xc
    rgl::matrix frame;                                  // +0x10 the listener frame (s, f, u rows; the position o is at +0x40)
    int slot[(0x248 - 0x50) / 4];                       // +0x50 group index by sound id (the game's named sounds: slot[0x11] = pain, slot[1] = level up)
    std::vector<int> channels;                          // +0x248 looping channels
    ~SoundMgr();                                        // 0x4c6aa0 (rgl/mesh2/gui_edit.cpp): frees defs and groups; channels by its member destructor
    int play(int idx, int flags, const float* pos, float freq, int* out_id);   // 0x442650 (ret 0x14)
    int play_slot(int n, int flags, const float* pos, float freq, int* out_id);   // 0x4428a0 (ret 0x14; agent5/agent_hud called it start)
    void play_ui(int sound, unsigned flags);            // 0x4942f0 (ret 8; world_data.cpp: plays a sound at the listener position)
    void stop_all();                                    // 0x494310 (world_data.cpp)
};

extern SoundMgr g_sound_mgr; // 0x5aa128 (defined in rgl/mesh2/gui_edit.cpp)
#endif // MB_SOUND_MGR_H
