// patches/perf_sound/lazy_samples.cpp: load the sound effects lazily / in the background (patch set "perf_sound").
//
// WHAT IT CHANGES. music_set::load (0x4c4340) loads every sound effect (~360 .ogg/.wav) with FSOUND_Sample_Load at
// start-up and then asks FMOD for each sample's length and default frequency; under Wine that is ~58 s. Here the
// start-up call returns at once with a placeholder handle ("stub"); the real FMOD load happens
//   (1) on a low-priority background thread that works through the stubs in creation order while the game loads the
//       rest and sits in the menu (default), and
//   (2) on the game thread, immediately, if a sample is played before the thread got to it (FSOUND_PlaySoundEx).
// No function of the reconstruction is replaced: five of our own fmod.dll import slots are swapped (devfast/iat_hook.h),
// so src/ is untouched. The game only ever hands a sample to FSOUND_PlaySoundEx (ui_widgets_a.cpp play_sound,
// agent_fx_sound.cpp SoundMgr::play) and to the three Sample_* calls music_set::load makes; those are the five hooks:
//   FSOUND_Sample_Load      returns a stub (static array, so a stub is recognised by address, never dereferenced blindly)
//   FSOUND_Sample_SetDefaults  stored in the stub and applied right after the real load (-1 = "leave", as in FMOD)
//   FSOUND_Sample_GetLength / GetDefaults(freq)  answered from the file header (below); other GetDefaults outputs
//                           (vol/pan/pri) force the real load
//   FSOUND_PlaySoundEx      stub -> real sample (loading it now if needed), then the real call
//
// OBSERVABLE DIFFERENCES (read before using; this is why it is a separate set from "perf"):
//  * Length and default frequency come from the file header, not from FMOD: Ogg Vorbis = sample rate of the
//    identification header and the granule position of the last page; PCM WAV = fmt/data chunks. FMOD's own decode can
//    differ by a few samples at the end of an Ogg stream; those two numbers only feed the random start position and the
//    pitch scaling (SoundMgr::play), so a deviation of a few samples is inaudible. Anything the parser does not
//    recognise (ADPCM, odd headers, a stub array that is full) takes the original path: an immediate real load.
//  * A missing file still returns 0 (the game then tries its "sounds\<name>" fallback) because the file is opened to
//    read the header. A file FMOD would reject although the header parses is not noticed at start-up: its sound then
//    silently does not play (PlaySoundEx returns -1) and the fallback path is not tried.
//  * First play of a sample the background thread has not reached yet blocks the game thread for that one load
//    (the same ~150 ms the start-up paid per sample under Wine). Everything else about playing is unchanged.
//  * It assumes FMOD 3.75 calls may come from two threads. FMOD 3.x serialises its API with an internal lock; this was
//    NOT verified with the game here. Set the environment variable MB_SOUND_LAZY=demand to disable the thread (samples
//    then load only when first played, all on the game thread).
//  * lazy_sound.txt in the game folder logs every real load: sample, ms, and which thread (BG / GAME). GAME lines are
//    the stalls the player could see.
// Do not combine with the devfast set's no_sound_samples (it hooks the same import).
#include "../devfast/iat_hook.h"
#include <stdio.h>

namespace {

typedef void* (__stdcall* Load_t)(int, const char*, unsigned, int, int);
typedef int (__stdcall* GetLength_t)(void*);
typedef int (__stdcall* GetDefaults_t)(void*, int*, int*, int*, int*);
typedef int (__stdcall* SetDefaults_t)(void*, int, int, int, int);
typedef int (__stdcall* PlaySoundEx_t)(int, void*, void*, int);

Load_t real_load;
GetLength_t real_getlength;
GetDefaults_t real_getdefaults;
SetDefaults_t real_setdefaults;
PlaySoundEx_t real_play;

struct Lazy {
    char path[MAX_PATH];
    unsigned flags;
    int freq, length;              // from the header
    int def[4];                    // deferred Sample_SetDefaults (-1 = unset)
    void* real;                    // the FMOD sample once loaded
    bool tried;                    // real load attempted
};

const int MAX_STUBS = 1024;
Lazy g_stubs[MAX_STUBS];
volatile LONG g_nstubs;
CRITICAL_SECTION g_cs;             // guards stub state and serialises the real FMOD loads
HANDLE g_thread;
FILE* g_log;
bool g_bg = true;

bool is_stub(const void* p)
{
    return p >= (const void*)g_stubs && p < (const void*)(g_stubs + MAX_STUBS);
}

// ---- header parsing --------------------------------------------------------------------------------------------
bool read_at(HANDLE h, unsigned long long off, void* dst, DWORD n, DWORD* got)
{
    LARGE_INTEGER li;
    li.QuadPart = (LONGLONG)off;
    if (!SetFilePointerEx(h, li, 0, FILE_BEGIN))
        return false;
    return ReadFile(h, dst, n, got, 0) != 0;
}

unsigned rd32(const unsigned char* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24); }

bool parse_header(const char* path, int* freq, int* length)
{
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER sz;
    bool ok = false;
    unsigned char head[4096];
    DWORD got = 0;
    if (GetFileSizeEx(h, &sz) && read_at(h, 0, head, sizeof head, &got) && got >= 64) {
        if (memcmp(head, "OggS", 4) == 0) {
            int nseg = head[26];
            unsigned p = 27 + nseg;
            if (p + 16 <= got && head[p] == 1 && memcmp(head + p + 1, "vorbis", 6) == 0) {
                int rate = (int)rd32(head + p + 12);
                static unsigned char tail[65536];
                unsigned long long n = sz.QuadPart < (long long)sizeof tail ? (unsigned long long)sz.QuadPart : sizeof tail;
                DWORD g2 = 0;
                if (rate > 0 && read_at(h, (unsigned long long)sz.QuadPart - n, tail, (DWORD)n, &g2) && g2 == n) {
                    int last = -1;
                    for (int i = (int)g2 - 27; i >= 0; i--)
                        if (tail[i] == 'O' && memcmp(tail + i, "OggS", 4) == 0) { last = i; break; }
                    if (last >= 0) {
                        unsigned lo = rd32(tail + last + 6), hi = rd32(tail + last + 10);
                        if (hi == 0 && (int)lo > 0) {
                            *freq = rate;
                            *length = (int)lo;
                            ok = true;
                        }
                    }
                }
            }
        } else if (memcmp(head, "RIFF", 4) == 0 && memcmp(head + 8, "WAVE", 4) == 0) {
            unsigned o = 12;
            unsigned tag = 0, rate = 0, align = 0;
            while (o + 8 <= got) {
                unsigned csz = rd32(head + o + 4);
                if (memcmp(head + o, "fmt ", 4) == 0 && o + 8 + 16 <= got) {
                    tag = head[o + 8] | (head[o + 9] << 8);
                    rate = rd32(head + o + 12);
                    align = head[o + 20] | (head[o + 21] << 8);
                } else if (memcmp(head + o, "data", 4) == 0) {
                    unsigned long long avail = (unsigned long long)sz.QuadPart - (o + 8);
                    unsigned long long dsz = csz < avail ? csz : avail;
                    if (tag == 1 && rate > 0 && align > 0) {
                        *freq = (int)rate;
                        *length = (int)(dsz / align);
                        ok = true;
                    }
                    break;
                }
                o += 8 + csz + (csz & 1);
            }
        }
    }
    CloseHandle(h);
    return ok;
}

// ---- real loading ------------------------------------------------------------------------------------------------
void* resolve(Lazy* z, bool game_thread)
{
    EnterCriticalSection(&g_cs);
    if (!z->tried) {
        z->tried = true;
        DWORD t0 = GetTickCount();
        z->real = real_load(-1, z->path, z->flags, 0, 0);
        if (z->real && (z->def[0] != -1 || z->def[1] != -1 || z->def[2] != -1 || z->def[3] != -1))
            real_setdefaults(z->real, z->def[0], z->def[1], z->def[2], z->def[3]);
        if (g_log) {
            fprintf(g_log, "%s %lu ms %s%s\n", game_thread ? "GAME" : "BG  ", GetTickCount() - t0, z->path, z->real ? "" : "  FAILED");
            fflush(g_log);
        }
    }
    void* r = z->real;
    LeaveCriticalSection(&g_cs);
    return r;
}

DWORD WINAPI preload(LPVOID)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    LONG i = 0;
    for (;;) {
        if (i < g_nstubs)
            resolve(&g_stubs[i++], false);
        else
            Sleep(200);
    }
}

// ---- hooks ------------------------------------------------------------------------------------------------------
void* __stdcall hook_load(int index, const char* name, unsigned flags, int offset, int length)
{
    int freq, len;
    if (index != -1 || offset != 0 || length != 0 || (flags & 0x8000) || !name || strlen(name) >= MAX_PATH)
        return real_load(index, name, flags, offset, length);
    if (GetFileAttributesA(name) == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (!parse_header(name, &freq, &len))
        return real_load(index, name, flags, offset, length);
    EnterCriticalSection(&g_cs);
    LONG n = g_nstubs;
    Lazy* z = 0;
    if (n < MAX_STUBS) {
        z = &g_stubs[n];
        strcpy(z->path, name);
        z->flags = flags;
        z->freq = freq;
        z->length = len;
        z->def[0] = z->def[1] = z->def[2] = z->def[3] = -1;
        z->real = 0;
        z->tried = false;
        g_nstubs = n + 1;
        if (g_bg && !g_thread)
            g_thread = CreateThread(0, 0, preload, 0, 0, 0);
    }
    LeaveCriticalSection(&g_cs);
    return z ? (void*)z : real_load(index, name, flags, offset, length);
}

int __stdcall hook_setdefaults(void* s, int freq, int vol, int pan, int pri)
{
    if (!is_stub(s))
        return real_setdefaults(s, freq, vol, pan, pri);
    Lazy* z = (Lazy*)s;
    EnterCriticalSection(&g_cs);
    if (z->tried) {
        void* r = z->real;
        LeaveCriticalSection(&g_cs);
        return r ? real_setdefaults(r, freq, vol, pan, pri) : 0;
    }
    if (freq != -1) z->def[0] = freq;
    if (vol != -1) z->def[1] = vol;
    if (pan != -1) z->def[2] = pan;
    if (pri != -1) z->def[3] = pri;
    LeaveCriticalSection(&g_cs);
    return 1;
}

int __stdcall hook_getlength(void* s)
{
    return is_stub(s) ? ((Lazy*)s)->length : real_getlength(s);
}

int __stdcall hook_getdefaults(void* s, int* freq, int* vol, int* pan, int* pri)
{
    if (!is_stub(s))
        return real_getdefaults(s, freq, vol, pan, pri);
    Lazy* z = (Lazy*)s;
    if (!vol && !pan && !pri) {
        if (freq)
            *freq = z->def[0] != -1 ? z->def[0] : z->freq;
        return 1;
    }
    void* r = resolve(z, true);
    return r ? real_getdefaults(r, freq, vol, pan, pri) : 0;
}

int __stdcall hook_play(int channel, void* s, void* dsp, int paused)
{
    if (is_stub(s)) {
        s = resolve((Lazy*)s, true);
        if (!s)
            return -1;
    }
    return real_play(channel, s, dsp, paused);
}

struct Start {
    Start()
    {
        char v[16];
        if (GetEnvironmentVariableA("MB_SOUND_LAZY", v, sizeof v) && _stricmp(v, "demand") == 0)
            g_bg = false;
        InitializeCriticalSection(&g_cs);
        g_log = fopen("lazy_sound.txt", "w");
        real_load = (Load_t)iat_hook("fmod.dll", "_FSOUND_Sample_Load@20", (void*)hook_load);
        real_getlength = (GetLength_t)iat_hook("fmod.dll", "_FSOUND_Sample_GetLength@4", (void*)hook_getlength);
        real_getdefaults = (GetDefaults_t)iat_hook("fmod.dll", "_FSOUND_Sample_GetDefaults@20", (void*)hook_getdefaults);
        real_setdefaults = (SetDefaults_t)iat_hook("fmod.dll", "_FSOUND_Sample_SetDefaults@20", (void*)hook_setdefaults);
        real_play = (PlaySoundEx_t)iat_hook("fmod.dll", "_FSOUND_PlaySoundEx@16", (void*)hook_play);
        if (!real_load || !real_getlength || !real_getdefaults || !real_setdefaults || !real_play) {
            // an import is missing: undo nothing (the slots already swapped keep working only if all five are hooked),
            // so refuse to run half-hooked
            MessageBoxA(0, "perf_sound: an fmod.dll import is missing", "perf_sound", MB_OK);
            ExitProcess(1);
        }
    }
} g_start;

}   // namespace
