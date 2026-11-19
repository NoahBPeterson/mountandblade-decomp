// audio_bench <game dir>: Sample_Load startup cost for all of Sounds/ (header only) vs decode-on-first-play, and the
// mixer's ns per voice per output sample, scalar vs SIMD, 32 voices; also checks both paths render the same (<= 1 LSB of int16).
#include "../mb_audio.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <dirent.h>
#include <unistd.h>
struct FSOUND_SAMPLE;
extern "C" {
signed char FSOUND_Init(int, int, unsigned); void FSOUND_Close();
FSOUND_SAMPLE* FSOUND_Sample_Load(int, const char*, unsigned, int, int);
unsigned FSOUND_Sample_GetLength(FSOUND_SAMPLE*);
int FSOUND_PlaySoundEx(int, FSOUND_SAMPLE*, void*, signed char);
signed char FSOUND_SetLoopMode(int, unsigned), FSOUND_SetFrequency(int, int), FSOUND_SetVolume(int, int), FSOUND_StopSound(int),
    FSOUND_3D_SetAttributes(int, const float*, const float*), FSOUND_SetPaused(int, signed char);
}
static double now() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
int main(int argc, char** argv)
{
    if (argc < 2 || chdir(argv[1])) return 2;
    mb_audio_set_offline(1); FSOUND_Init(44100, 64, 0);
    std::vector<FSOUND_SAMPLE*> all; double t0 = now(); size_t frames = 0;
    if (DIR* d = opendir("Sounds")) { while (dirent* e = readdir(d)) { std::string n = e->d_name; if (n.size() < 5) continue; std::string x = n.substr(n.size() - 4);
        if (x != ".ogg" && x != ".wav") continue; if (auto* s = FSOUND_Sample_Load(-1, ("sounds\\" + n).c_str(), 0x1000, 0, 0)) { all.push_back(s); frames += FSOUND_Sample_GetLength(s); } else printf("load failed: %s\n", n.c_str()); } closedir(d); }
    double tl = now() - t0; std::vector<unsigned> len0; for (auto* s : all) len0.push_back(FSOUND_Sample_GetLength(s));
    printf("Sample_Load x%zu (header only, lengths known: %.1f M frames): %.3f s\n", all.size(), frames / 1e6, tl);
    t0 = now(); for (auto* s : all) { int c = FSOUND_PlaySoundEx(-1, s, 0, 1); FSOUND_StopSound(c); }
    { int bad = 0; for (size_t i = 0; i < all.size(); i++) bad += FSOUND_Sample_GetLength(all[i]) != len0[i]; printf("header length != decoded length for %d samples\n", bad); }
    printf("decode all on first play (what eager load used to cost at startup): %.3f s\n", now() - t0);

    FSOUND_SAMPLE* mono = FSOUND_Sample_Load(-1, "sounds\\s_arrow_hit.wav", 0x1000, 0, 0);
    FSOUND_SAMPLE* ster = FSOUND_Sample_Load(-1, "music\\ambushed_by_khergit.ogg", 0x2000, 0, 0);   // long stereo sample
    const int F = 44100 * 4; std::vector<float> a((size_t)F * 2), b((size_t)F * 2);
    for (int pass = 0; pass < 2; pass++) {
        for (int smp = 0; smp < 2; smp++) {
            FSOUND_StopSound(-3);
            for (int i = 0; i < 32; i++) { int c = FSOUND_PlaySoundEx(-1, smp ? ster : mono, 0, 1); FSOUND_SetLoopMode(c, 2); FSOUND_SetFrequency(c, 30000 + i * 1000);
                float p[3] = {(float)(i - 16), 0, 5.f + i}; FSOUND_3D_SetAttributes(c, p, 0); FSOUND_SetVolume(c, 100 + i * 4); FSOUND_SetPaused(c, 0); }
            mb_audio_set_simd(pass); std::vector<float>& o = pass ? b : a;
            double t = now(); for (int k = 0; k < F; k += 1024) mb_audio_render(&o[(size_t)k * 2], k + 1024 > F ? F - k : 1024); double dt = now() - t;
            printf("%s %-6s: %.2f ns per voice-sample (%.0fx realtime for 32 voices)\n", pass ? "SIMD  " : "scalar", smp ? "stereo" : "mono", dt * 1e9 / (32.0 * F), 4.0 / dt);
            if (pass) { /* compare against scalar kept in a */ }
            static std::vector<float> ref[2]; if (!pass) ref[smp] = a; else { double md = 0; for (size_t i = 0; i < b.size(); i++) md = std::fmax(md, std::fabs(b[i] - ref[smp][i]));
                printf("  max |simd - scalar| = %.2e (1 LSB int16 = %.2e)\n", md, 1.0 / 32768); }
        }
    }
    FSOUND_Close(); return 0;
}
