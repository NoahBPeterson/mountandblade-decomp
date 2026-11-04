// fmod_api.h: the part of the FMOD 3 API (fmod.dll, linked through fmod.lib) the game calls, with fmod.h's own signatures.
// FMOD exports stdcall C functions (F_API = __stdcall): _FSOUND_Close@0, _FSOUND_Sample_Load@20, ... ONE declaration for
// every caller (src/rgl/mesh2/rec_tables, cls_map, world_data, agent_hud, agent_fx_sound, ui_widgets_a, screen_camera).
#ifndef MB_FMOD_API_H
#define MB_FMOD_API_H

struct FSOUND_SAMPLE;

extern "C" {
void           __stdcall FSOUND_Close();                                                                        // 0x534336
FSOUND_SAMPLE* __stdcall FSOUND_Sample_Load(int index, const char* name_or_data, unsigned int inputmode, int offset,
                                            int length);                                                        // 0x534330
signed char    __stdcall FSOUND_SetVolume(int channel, int vol);                                                // 0x5342ca
signed char    __stdcall FSOUND_Sample_SetDefaults(FSOUND_SAMPLE* sptr, int deffreq, int defvol, int defpan, int defpri);  // 0x53432a
unsigned int   __stdcall FSOUND_Sample_GetLength(FSOUND_SAMPLE* sptr);                                          // 0x534324
signed char    __stdcall FSOUND_Sample_GetDefaults(FSOUND_SAMPLE* sptr, int* deffreq, int* defvol, int* defpan,
                                                   int* defpri);                                                // 0x53431e
void           __stdcall FSOUND_3D_Listener_SetAttributes(const float* pos, const float* vel, float fx, float fy, float fz,
                                                          float tx, float ty, float tz);                        // 0x5342dc
}

#endif // MB_FMOD_API_H
