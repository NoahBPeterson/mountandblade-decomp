# 010 Audio: replace FMOD 3.75 with SDL3 audio streams + a small mixer

Area: audio | Effort: M | Impact: high (also removes a ~55 s Wine start-up cost, see 013) | Depends on: 004, 007

## Summary

FMOD 3.75 (proprietary, 32-bit Windows `fmod.dll`) is used through 26 functions. Assets are all Ogg Vorbis or WAV. Implement the 26 functions' semantics in a ~800-line
mixer on `SDL_AudioStream` + `stb_vorbis`.

## Evidence

- Imports (work/mb.analysis.bin, 26): Init, Close, Update, SetMaxHardwareChannels, SetSFXMasterVolume, Sample_Load/SetDefaults/GetDefaults/GetLength, PlaySoundEx, StopSound, SetVolume,
  SetFrequency, SetPriority, SetPaused, SetLoopMode, SetCurrentPosition, 3D_SetAttributes, 3D_Listener_SetAttributes, 3D_SetRolloffFactor, Stream_Open/Play/Close/GetOpenState/GetLengthMs/GetTime.
- Source uses: ~88 `FSOUND_*` tokens in 18 files; `FSOUND_Sample_Load(-1, buf, (level & 1) ? 0x2000 : 0x1000, 0, 0)` at src/rgl/mesh2/rec_tables.cpp:679,683 (flags 0x1000 = hardware 3D, 0x2000 = 2D);
  `sound_engine::tick`; `SndSlotS::update`.
- Assets in the Steam install: `Sounds/` 371 `.ogg` + 13 `.wav` (9.4 MB, ~360 loaded at start), `music/` 63 `.ogg`. No MP3.
- Cost today: loading ~360 samples takes ~58 s under CrossOver (mod_timing.txt `sounds.txt 57823..58165 ms`; patches/devfast/no_sound_samples.cpp documents ~55 s). A decoded-on-load, in-memory sample table on native should take well under 1 s.
- Profile (181.8 s run under CrossOver): 29.4% of main-thread samples are inside `fmod.dll`.

## Detailed plan

1. `port/audio.h`: keep the FMOD-shaped C API (`mb_FSOUND_*`) so the 18 call-site files change only their include; later refactor into `audio::Voice` handles.
2. Backend: one `SDL_AudioStream` per device, mixer callback sums up to `SetMaxHardwareChannels` voices (default 32; make it 64+, modding), per-voice volume, pan, pitch (`SetFrequency`: linear resample),
   priority-based voice stealing, loop mode, pause, seek (`SetCurrentPosition`).
3. 3D: listener position/orientation/velocity; distance attenuation with FMOD 3 rolloff (`SetRolloffFactor`, inverse-distance model, min/max distance from `Sample_SetDefaults`), stereo panning by azimuth. Doppler optional (FMOD 3 doppler existed; the game sets velocities).
4. Streams: music via `stb_vorbis` pull decoding on a worker thread (`Stream_Open` returns openstate flags polled by `Stream_GetOpenState` - model async open), `Stream_GetTime/LengthMs`.
5. Decode samples at load into 16-bit PCM (or float); ~9.4 MB of Ogg decodes to ~100 MB PCM? measure; if too large keep Ogg compressed and decode on first play (ties to 013 lazy load).
6. `disable_sound` / `disable_music` config keys keep working (Config::set, config.cpp:645).

## Code sketch

```cpp
// port/audio_mixer.cpp (shape)
struct Voice { const int16_t* pcm; size_t frames, pos; float vol, pan, step; int prio; bool loop, paused, is3d; vec3 p; };
static void SDLCALL cb(void*, SDL_AudioStream* s, int need, int) { /* sum voices into float buffer, SDL_PutAudioStreamData(s, buf, bytes) */ }
FSOUND_SAMPLE* mb_FSOUND_Sample_Load(int idx, const char* path, unsigned flags, int off, int len);   // decodes with stb_vorbis_decode_filename
```

## Risks

- Subjective mix differences (rolloff curves, priority stealing, doppler): compare by ear against the CrossOver build; make constants tunable.
- Latency/underruns on the mixer thread: keep it lock-free for voice start/stop (SPSC queue).
- Some mods ship 22/44.1 kHz mono WAVs of odd formats: test `stb_vorbis` + `drwav`.

## Acceptance

All Native sounds/music play with correct looping and 3D position in a battle; load time for samples < 2 s; no FMOD symbol in the link; sound can be disabled by config.

## Effort

M (3-5 wk).
