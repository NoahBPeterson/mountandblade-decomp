// sound_mgr.h: the FMOD-backed sound player (global g_sound / g_sound_mgr / SOUNDMGR at 0x5aa128) with its sound definitions and groups.
// ONE definition for src/game/agent3 (agent_fx_sound.cpp: play_slot 0x4428a0, SoundDef/SoundGroup scales), agent5 (agent_hud: start 0x4428a0 =
// play_slot, stop_all 0x494310), party2 (play 0x442650) and inventory_panel (play 0x442650). The includer must have declared vector4
// (rgl::vector4 of rgl_math.h or of match/include/rgl.h) and <vector>.
#ifndef MB_SOUND_MGR_H
#define MB_SOUND_MGR_H
namespace rgl { struct vector4; }
#include <vector>
#include "engine/core/tstring.h"
#include "engine/core/rgl_base.h"   // rgl::vector4 (whichever declaration the includer uses)

struct FSOUND_SAMPLE;                                   // engine/audio/fmod_api.h
struct stream_base;
struct SoundDef {                                       // 0x14 bytes: one sample of sounds.txt (SoundMgr::load 0x4c4340)
    FSOUND_SAMPLE* sample;                              // +0
    int level;                                          // +4 file bits: bit 0 = looped, bits 4..7 = distance range / channel class
    float ratio;                                        // +8 length / default frequency
    int freq;                                           // +0xc default frequency
    int length;                                         // +0x10 sample length
    SoundDef() : sample(0), level(0), ratio(0.0f), length(0) {}    // +0xc is left alone
    float range_scale();                                // 0x4355f0 (fastcall/thiscall)
};
struct SoundGroup {                                     // 0x98 bytes
    tstring name;                                       // +0
    int flags;                                          // +0x10 (2: looping, 4: random start, bits 8..11: volume; read signed by volume_scale 0x435620)
    int var[32];                                        // +0x14 variations (def indices)
    int nvar;                                           // +0x94
    void load(stream_base& a);                          // 0x4be200 (game/config/config.cpp: name, flags, the variations)
    float volume_scale();                               // 0x435620
};
struct SoundMgr {                                       // the sound player (g_sound 0x5aa128)
    int ndefs;                                          // +0
    SoundDef* defs;                                     // +4
    int ngroups;                                        // +8
    SoundGroup* groups;                                 // +0xc
    rgl::matrix frame;                                  // +0x10 the listener frame (s, f, u rows; the position o is at +0x40)
    int slot[(0x248 - 0x50) / 4];                       // +0x50 group index by sound id (the game's named sounds: slot[0x11] = pain, slot[1] = level up)
    std::vector<int> channels;                          // +0x248 looping channels
    SoundMgr();                                         // (game/editor/editor_selection_and_translation.cpp) the original image holds slot[125] (+0x244) = 32 before any code runs
    ~SoundMgr();                                        // 0x4c6aa0 (game/editor/editor_selection_and_translation.cpp): frees defs and groups; channels by its member destructor
    int play(int idx, int flags, const rgl::vector4* pos, float freq, float* out_duration);   // 0x442650 (ret 0x14; out_duration: the sample's length in seconds, SoundDef::ratio)
    int play_slot(int n, int flags, const rgl::vector4* pos, float freq, float* out_duration);   // 0x4428a0 (ret 0x14; agent5/agent_hud called it start)
    void play_ui(int sound, unsigned flags);            // 0x4942f0 (ret 8; game_data_party_quest.cpp: plays a sound at the listener position)
    void stop_all();                                    // 0x494310 (game_data_party_quest.cpp)
    void load(stream_base* s, int load_samples, const tstring* dir);   // 0x4c4340 (game/data/module_option_records.cpp; thiscall, ret 0xc): sounds.txt
    void load_used();                                   // 0x4be270 (game/config/config.cpp): the group of every named sound code, into slot[]
};

extern SoundMgr g_sound_mgr; // 0x5aa128 (defined in game/editor/editor_selection_and_translation.cpp)
#endif // MB_SOUND_MGR_H
