// patches/devfast/no_sound_samples.cpp: skip loading the sound effects (patch set "devfast").
//
// CHANGES BEHAVIOUR: music_set::load (0x4c4340) loads all ~360 sound samples through FMOD at start-up, which takes
// ~55 s under Wine. This makes every FSOUND_Sample_Load fail (returns 0, as for a missing file), so the game starts
// without sound effects; music streams through other FMOD calls and still plays. For test runs only.
#include "iat_hook.h"

namespace {

void* __stdcall sample_load(int, const char*, unsigned, int, int)
{
    return 0;
}

struct Start {
    Start() { iat_hook("fmod.dll", "_FSOUND_Sample_Load@20", (void*)sample_load); }
} g_start;

}   // namespace
