// port/stubs/fmod_stubs.cpp: FMOD 3.75 (fmod.dll) stand-in for the portable build, ticket 004 milestone 2. Every entry
// point the game imports (the 26 of tools/relink/glue.py's fmod import library) exists and fails: FSOUND_Init reports
// failure, loads and opens return NULL, playback returns -1, setters are no-ops. Ticket 010 replaces this with SDL3
// audio + stb_vorbis behind the same names.
//
// FMOD exports stdcall functions (F_API = __stdcall): the callee pops its arguments, so each stub takes exactly the
// parameter list of fmod.h 3.75 (the import library's @N byte counts: FSOUND_Init@12, FSOUND_Sample_Load@20, ...).
// This TU deliberately includes no src/ header (src/ declares a few of these with differing return types per TU).
struct FSOUND_SAMPLE;
struct FSOUND_STREAM;
struct FSOUND_DSPUNIT;

#define F_API __stdcall

extern "C" {

signed char    F_API FSOUND_Init(int mixrate, int maxsoftwarechannels, unsigned int flags) { return 0; }
void           F_API FSOUND_Close() {}
void           F_API FSOUND_Update() {}
signed char    F_API FSOUND_SetMaxHardwareChannels(int max) { return 0; }
void           F_API FSOUND_SetSFXMasterVolume(int volume) {}
void           F_API FSOUND_3D_SetRolloffFactor(float rolloffscale) {}
void           F_API FSOUND_3D_Listener_SetAttributes(const float* pos, const float* vel, float fx, float fy, float fz,
                                                      float tx, float ty, float tz) {}

FSOUND_SAMPLE* F_API FSOUND_Sample_Load(int index, const char* name_or_data, unsigned int inputmode, int offset,
                                        int length) { return 0; }
signed char    F_API FSOUND_Sample_SetDefaults(FSOUND_SAMPLE* sptr, int deffreq, int defvol, int defpan, int defpri)
{ return 0; }
signed char    F_API FSOUND_Sample_GetDefaults(FSOUND_SAMPLE* sptr, int* deffreq, int* defvol, int* defpan, int* defpri)
{ return 0; }
unsigned int   F_API FSOUND_Sample_GetLength(FSOUND_SAMPLE* sptr) { return 0; }

int            F_API FSOUND_PlaySoundEx(int channel, FSOUND_SAMPLE* sptr, FSOUND_DSPUNIT* dsp, signed char startpaused)
{ return -1; }
signed char    F_API FSOUND_StopSound(int channel) { return 0; }
signed char    F_API FSOUND_SetPaused(int channel, signed char paused) { return 0; }
signed char    F_API FSOUND_SetVolume(int channel, int vol) { return 0; }
signed char    F_API FSOUND_SetFrequency(int channel, int freq) { return 0; }
signed char    F_API FSOUND_SetPriority(int channel, int priority) { return 0; }
signed char    F_API FSOUND_SetLoopMode(int channel, unsigned int loopmode) { return 0; }
signed char    F_API FSOUND_SetCurrentPosition(int channel, unsigned int offset) { return 0; }
signed char    F_API FSOUND_3D_SetAttributes(int channel, const float* pos, const float* vel) { return 0; }

FSOUND_STREAM* F_API FSOUND_Stream_Open(const char* name_or_data, unsigned int mode, int offset, int length)
{ return 0; }
signed char    F_API FSOUND_Stream_Close(FSOUND_STREAM* stream) { return 0; }
int            F_API FSOUND_Stream_Play(int channel, FSOUND_STREAM* stream) { return -1; }
int            F_API FSOUND_Stream_GetOpenState(FSOUND_STREAM* stream) { return -1; }   // -1: open failed
int            F_API FSOUND_Stream_GetTime(FSOUND_STREAM* stream) { return 0; }
int            F_API FSOUND_Stream_GetLengthMs(FSOUND_STREAM* stream) { return 0; }

} // extern "C"
