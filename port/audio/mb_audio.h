// port/audio/mb_audio.h: extras beside the FMOD 3.75 C API (which is declared by src/ itself, src/include/fmod_api.h
// and per-TU externs). Test / tooling hooks only; the game never calls these.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
// Offline mode: no audio device is opened by FSOUND_Init; the caller pulls the mix with mb_audio_render. Call BEFORE
// FSOUND_Init. Used by tests and for a silent-machine fallback.
void mb_audio_set_offline(int on);
// Enable (default) / disable the vectorised sample-voice mixer (scalar reference path when off).
void mb_audio_set_simd(int on);
// Background decoding of loaded samples (default on; call before the first Sample_Load to disable).
void mb_audio_set_background_decode(int on);
// Mix `frames` stereo float frames (interleaved L,R, clipped to [-1,1]) into out. Works in both modes (in device
// mode it competes with the device callback; use offline mode for deterministic output).
void mb_audio_render(float* out, int frames);
// Number of voices currently playing (not counting free channels).
int mb_audio_active_voices(void);
// Output rate the mixer runs at.
int mb_audio_mixrate(void);
#ifdef __cplusplus
}
#endif
