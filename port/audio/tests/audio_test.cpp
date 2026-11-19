// port/audio/tests/audio_test.cpp: drives the FMOD 3.75 API (mb_audio) offline, renders the mix to a WAV and checks it.
//   mb_audio_test <game dir> <out.wav> [--device]       (--device: also opens the real device briefly at low volume)
#include "../mb_audio.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <dirent.h>
#include <unistd.h>
#include <vector>

struct FSOUND_SAMPLE; struct FSOUND_STREAM;
extern "C" {
signed char FSOUND_Init(int, int, unsigned); void FSOUND_Close(); void FSOUND_Update();
signed char FSOUND_SetMaxHardwareChannels(int); void FSOUND_SetSFXMasterVolume(int); void FSOUND_3D_SetRolloffFactor(float);
void FSOUND_3D_Listener_SetAttributes(const float*, const float*, float, float, float, float, float, float);
FSOUND_SAMPLE* FSOUND_Sample_Load(int, const char*, unsigned, int, int);
signed char FSOUND_Sample_SetDefaults(FSOUND_SAMPLE*, int, int, int, int);
signed char FSOUND_Sample_GetDefaults(FSOUND_SAMPLE*, int*, int*, int*, int*);
unsigned FSOUND_Sample_GetLength(FSOUND_SAMPLE*);
int FSOUND_PlaySoundEx(int, FSOUND_SAMPLE*, void*, signed char);
signed char FSOUND_StopSound(int), FSOUND_SetPaused(int, signed char), FSOUND_SetVolume(int, int), FSOUND_SetFrequency(int, int),
    FSOUND_SetPriority(int, int), FSOUND_SetLoopMode(int, unsigned), FSOUND_SetCurrentPosition(int, unsigned),
    FSOUND_3D_SetAttributes(int, const float*, const float*);
FSOUND_STREAM* FSOUND_Stream_Open(const char*, unsigned, int, int);
signed char FSOUND_Stream_Close(FSOUND_STREAM*);
int FSOUND_Stream_Play(int, FSOUND_STREAM*), FSOUND_Stream_GetOpenState(FSOUND_STREAM*), FSOUND_Stream_GetTime(FSOUND_STREAM*),
    FSOUND_Stream_GetLengthMs(FSOUND_STREAM*);
}

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { fails++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); printf(__VA_ARGS__); printf("\n"); } } while (0)

struct Stat { double rms_l, rms_r, peak; };
static Stat stat(const std::vector<float>& b, size_t f0, size_t f1)
{
    double sl = 0, sr = 0, pk = 0;
    for (size_t i = f0; i < f1; i++) { sl += b[i * 2] * b[i * 2]; sr += b[i * 2 + 1] * b[i * 2 + 1];
        pk = std::fmax(pk, std::fmax(std::fabs(b[i * 2]), std::fabs(b[i * 2 + 1]))); }
    double n = (double)(f1 - f0); return {std::sqrt(sl / n), std::sqrt(sr / n), pk};
}
static void write_wav(const char* path, const std::vector<float>& b, int rate)
{
    FILE* f = fopen(path, "wb"); if (!f) return;
    uint32_t n = (uint32_t)b.size() * 2, v;
    fwrite("RIFF", 1, 4, f); v = 36 + n; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f); uint16_t t = 1, c = 2; fwrite(&t, 2, 1, f); fwrite(&c, 2, 1, f);
    v = (uint32_t)rate; fwrite(&v, 4, 1, f); v = (uint32_t)rate * 4; fwrite(&v, 4, 1, f); t = 4; fwrite(&t, 2, 1, f); t = 16; fwrite(&t, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&n, 4, 1, f);
    for (float x : b) { int16_t s = (int16_t)std::lrintf(x * 32767.f); fwrite(&s, 2, 1, f); }
    fclose(f);
}

int main(int argc, char** argv)
{
    if (argc < 3) { printf("usage: %s <game dir> <out.wav> [--device]\n", argv[0]); return 2; }
    if (chdir(argv[1])) { printf("cannot chdir %s\n", argv[1]); return 2; }
    bool device = argc > 3 && !strcmp(argv[3], "--device");
    mb_audio_set_offline(1);
    CHECK(FSOUND_Init(0xac44, 0x40, 0) == 1, "init");
    FSOUND_SetMaxHardwareChannels(0); FSOUND_SetSFXMasterVolume(255); FSOUND_3D_SetRolloffFactor(0.5f);
    const int R = mb_audio_mixrate(); CHECK(R == 44100, "rate %d", R);

    // samples: the game's own path spelling (backslashes, wrong case), 2D and 3D
    FSOUND_SAMPLE* hit = FSOUND_Sample_Load(-1, "sounds\\ARROW_FLY_1.ogg", 0x2000, 0, 0);
    FSOUND_SAMPLE* wav = FSOUND_Sample_Load(-1, "Sounds\\s_arrow_hit.wav", 0x1000, 0, 0);
    FSOUND_SAMPLE* bad = FSOUND_Sample_Load(-1, "sounds\\does_not_exist.ogg", 0x1000, 0, 0);
    CHECK(hit && wav && !bad, "load %p %p %p", (void*)hit, (void*)wav, (void*)bad);
    if (!hit || !wav) return 1;
    unsigned hlen = FSOUND_Sample_GetLength(hit), wlen = FSOUND_Sample_GetLength(wav);
    int hf = 0, wf = 0; FSOUND_Sample_GetDefaults(hit, &hf, 0, 0, 0); FSOUND_Sample_GetDefaults(wav, &wf, 0, 0, 0);
    printf("ogg arrow_fly_1: %u frames @%d Hz (%.2f s); wav s_arrow_hit: %u frames @%d Hz\n", hlen, hf, hlen / (double)hf, wlen, wf);
    CHECK(hlen > 1000 && hf >= 8000 && wlen > 1000 && wf == 44100, "lengths");
    FSOUND_Sample_SetDefaults(hit, -1, -1, -1, 200);
    int pri = 0; FSOUND_Sample_GetDefaults(hit, 0, 0, 0, &pri); CHECK(pri == 200, "defpri %d", pri);

    FSOUND_STREAM* mus = FSOUND_Stream_Open("music\\Ambushed_By_Khergit.ogg", 0x1000000, 0, 0);
    CHECK(mus != nullptr, "stream open");
    FSOUND_STREAM* nope = FSOUND_Stream_Open("music\\nope.ogg", 0, 0, 0);
    CHECK(nope == nullptr, "blocking open of a missing file returns NULL");
    int st = -2, spin = 0;
    while ((st = FSOUND_Stream_GetOpenState(mus)) == -2 && spin++ < 3000) usleep(1000);
    CHECK(st == 0, "open state %d", st);
    int lenms = FSOUND_Stream_GetLengthMs(mus); printf("music length %d ms\n", lenms); CHECK(lenms > 2000, "len");

    std::vector<float> mix; auto render = [&](double sec) { size_t n = (size_t)(sec * R); size_t o = mix.size() / 2; mix.resize((o + n) * 2);
        for (size_t d = 0; d < n; d += 882) { size_t k = n - d < 882 ? n - d : 882; mb_audio_render(&mix[(o + d) * 2], (int)k); usleep(3000); }   // chunked: the stream decoder thread is real time
        return o; };

    {   // load every sample under Sounds/ the way the game does (time + count)
        auto t0 = std::chrono::steady_clock::now(); int ok = 0, tot = 0; size_t frames = 0;
        if (DIR* d = opendir("Sounds")) { while (dirent* e = readdir(d)) { std::string n = e->d_name; if (n.size() < 5 || (n.substr(n.size() - 4) != ".ogg" && n.substr(n.size() - 4) != ".wav")) continue;
            tot++; FSOUND_SAMPLE* s = FSOUND_Sample_Load(-1, ("sounds\\" + n).c_str(), 0x1000, 0, 0); if (s) { ok++; frames += FSOUND_Sample_GetLength(s); } } closedir(d); }
        double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        printf("loaded %d/%d samples from Sounds/ in %.2f s (%.1f MB int16 PCM)\n", ok, tot, sec, frames * 2 / 1e6);
        CHECK(tot > 300 && ok == tot, "all samples load");
    }
    // 1. silence before anything plays
    size_t o0 = render(0.2); Stat s0 = stat(mix, o0, o0 + (size_t)(0.2 * R)); CHECK(s0.peak == 0.f, "silence peak %f", s0.peak);

    // 2. music stream alone
    int mch = FSOUND_Stream_Play(-1, mus); CHECK(mch >= 0, "stream play %d", mch);
    FSOUND_SetVolume(mch, 128);
    size_t o1 = render(1.0); Stat s1 = stat(mix, o1, o1 + R);
    printf("music alone: rms L %.4f R %.4f peak %.3f, stream time %d ms\n", s1.rms_l, s1.rms_r, s1.peak, FSOUND_Stream_GetTime(mus));
    CHECK(s1.rms_l > 0.005 && s1.rms_r > 0.005 && s1.peak < 0.95, "music level");
    int t1 = FSOUND_Stream_GetTime(mus); CHECK(t1 > 900 && t1 < 1100, "stream time %d after 1 s", t1);

    // 3. several channels: looped 2D ogg, 3D wav left and right of the listener, paused-start then unpaused
    const float lpos[3] = {0, 0, 0};
    FSOUND_3D_Listener_SetAttributes(lpos, 0, 0, 0, 1, 0, 1, 0);   // forward +z, up +y: right = +x
    int c2d = FSOUND_PlaySoundEx(-1, hit, 0, 1); CHECK(c2d >= 0, "play 2d");
    FSOUND_3D_SetAttributes(c2d, 0, 0); FSOUND_SetVolume(c2d, 200); FSOUND_SetLoopMode(c2d, 2); FSOUND_SetPriority(c2d, 255);
    size_t o2 = render(0.1); Stat sp = stat(mix, o2, o2 + (size_t)(0.1 * R));   // still paused: music only
    FSOUND_SetPaused(c2d, 0);
    size_t o3 = render(0.5); Stat s3 = stat(mix, o3, o3 + (size_t)(0.5 * R));
    printf("music+looped 2D: rms L %.4f R %.4f (music alone %.4f / %.4f; paused phase %.4f)\n", s3.rms_l, s3.rms_r, s1.rms_l, s1.rms_r, sp.rms_l);
    CHECK(s3.rms_l > s1.rms_l * 1.05, "2D sample audible over music");
    CHECK(mb_audio_active_voices() == 2, "voices %d", mb_audio_active_voices());

    FSOUND_StopSound(mch);   // music off so the 3D check hears only the effects
    float pr[3] = {10, 0, 2}, pl[3] = {-10, 0, 2};
    int c3r = FSOUND_PlaySoundEx(-1, wav, 0, 1); FSOUND_3D_SetAttributes(c3r, pr, 0); FSOUND_SetVolume(c3r, 255); FSOUND_SetPaused(c3r, 0);
    FSOUND_StopSound(c2d);
    size_t o4 = render(0.12); Stat sr = stat(mix, o4, o4 + (size_t)(0.12 * R));
    FSOUND_StopSound(c3r);
    int c3l = FSOUND_PlaySoundEx(-1, wav, 0, 1); FSOUND_3D_SetAttributes(c3l, pl, 0); FSOUND_SetVolume(c3l, 255); FSOUND_SetPaused(c3l, 0);
    size_t o5 = render(0.12); Stat sl = stat(mix, o5, o5 + (size_t)(0.12 * R));
    printf("3D right source: L %.4f R %.4f   3D left source: L %.4f R %.4f\n", sr.rms_l, sr.rms_r, sl.rms_l, sl.rms_r);
    CHECK(sr.rms_r > 3 * sr.rms_l, "right source pans right"); CHECK(sl.rms_l > 3 * sl.rms_r, "left source pans left");
    // attenuation with distance (rolloff 0.5): 10 m vs 100 m
    float p10[3] = {0, 0, 10}, p100[3] = {0, 0, 100};
    auto level_at = [&](const float* p) { int c = FSOUND_PlaySoundEx(-1, wav, 0, 1); FSOUND_3D_SetAttributes(c, p, 0); FSOUND_SetPaused(c, 0);
        size_t o = render(0.1); Stat s = stat(mix, o, o + (size_t)(0.1 * R)); FSOUND_StopSound(c); return s.rms_l; };
    render(0.3);
    double l10 = level_at(p10), l100 = level_at(p100);
    printf("3D level at 10: %.5f  at 100: %.5f  ratio %.2f (expected ~%.2f)\n", l10, l100, l10 / l100, (1 + 0.5 * 99) / (1 + 0.5 * 9));
    CHECK(l10 / l100 > 7.0 && l10 / l100 < 10.5, "rolloff ratio");

    // 4. stale handles, FSOUND_ALL, frequency, position, priority stealing
    int a = FSOUND_PlaySoundEx(-1, wav, 0, 0); FSOUND_StopSound(a);
    int b = FSOUND_PlaySoundEx(-1, wav, 0, 0);
    CHECK(a != b && (a & 0xfff) == (b & 0xfff), "channel reuse gives a new handle (%x %x)", a, b);
    CHECK(FSOUND_SetVolume(a, 10) == 0 && FSOUND_StopSound(a) == 0, "stale handle rejected");
    CHECK(FSOUND_SetFrequency(b, 22050) == 1 && FSOUND_SetCurrentPosition(b, 100) == 1, "freq/pos");
    FSOUND_StopSound(-3); CHECK(mb_audio_active_voices() == 0, "stop all leaves %d", mb_audio_active_voices());
    std::vector<int> hs; for (int i = 0; i < 64; i++) { int h = FSOUND_PlaySoundEx(-1, wav, 0, 1); hs.push_back(h); FSOUND_SetPriority(h, i); }
    CHECK(mb_audio_active_voices() == 64, "64 voices");
    FSOUND_Sample_SetDefaults(wav, -1, -1, -1, 255); int hn = FSOUND_PlaySoundEx(-1, wav, 0, 1);
    CHECK(hn >= 0 && FSOUND_SetVolume(hs[0], 5) == 0 && FSOUND_SetVolume(hs[63], 5) == 1, "steal lowest priority (handle 0 gone)");
    FSOUND_Sample_SetDefaults(wav, -1, -1, -1, 0); int hz = FSOUND_PlaySoundEx(-1, wav, 0, 1);
    CHECK(hz < 0 || FSOUND_SetVolume(hn, 5) == 1, "low priority cannot steal a more important voice");
    FSOUND_StopSound(-3);

    // 5. one-shot ends by itself and frees its channel; stream with end-of-stream
    int one = FSOUND_PlaySoundEx(-1, wav, 0, 0); render(wlen / (double)R + 0.1);
    CHECK(FSOUND_SetVolume(one, 100) == 0, "finished one-shot frees channel");
    FSOUND_Stream_Close(mus);

    // 6. level check of the whole render
    Stat all = stat(mix, 0, mix.size() / 2); printf("whole render: %.2f s, peak %.3f\n", mix.size() / 2.0 / R, all.peak);
    CHECK(all.peak > 0.05 && all.peak <= 1.0, "peak");
    write_wav(argv[2], mix, R);
    FSOUND_Close();

    if (device) {   // real device path, very quiet
        mb_audio_set_offline(0);
        if (FSOUND_Init(0xac44, 0x40, 0) == 1) {
            FSOUND_SetSFXMasterVolume(12);
            FSOUND_SAMPLE* s = FSOUND_Sample_Load(-1, "sounds\\s_arrow_hit.wav", 0x2000, 0, 0);
            int c = FSOUND_PlaySoundEx(-1, s, 0, 0); usleep(400000);
            CHECK(c >= 0, "device play"); FSOUND_Close(); printf("device mode ok\n");
        } else printf("device init failed (no audio device?)\n");
    }
    printf(fails ? "RESULT: %d FAILURES\n" : "RESULT: all checks passed\n", fails);
    return fails ? 1 : 0;
}
