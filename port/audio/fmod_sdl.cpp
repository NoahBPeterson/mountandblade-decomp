// port/audio/fmod_sdl.cpp: the FMOD 3.75 subset Mount & Blade imports (the 26 of tools/relink/glue.py's import
// library), implemented over SDL3 audio + stb_vorbis with a small software mixer (ticket 010). src/ keeps calling
// FSOUND_*; these definitions replace port/stubs/fmod_stubs.cpp.
//
// Model (FMOD 3 semantics the game depends on):
//  * channel handle = (serial << 12) | channel index; a handle whose channel was reused or finished is stale and every
//    call on it fails. FSOUND_FREE (-1) in Play picks a free channel, else steals the lowest-priority one (priority 0..255,
//    255 = most important); FSOUND_ALL (-3) addresses every channel. Bare indices (< 4096) are accepted as in FMOD.
//  * volume 0..255, pan 0..255 (128 centre), frequency in Hz, loop mode 1 = off / 2 = normal (FSOUND_LOOP_*).
//  * samples: decoded at load to int16 (Ogg through stb_vorbis, WAV through wav.cpp); load flag 0x2000 = FSOUND_2D (ignores
//    3D attributes), 0x1000 = FSOUND_HW3D (3D: positioned, attenuated, panned by azimuth), 0x8000 = FSOUND_LOADMEMORY.
//  * 3D: left-handed listener (right = up x forward), inverse-distance rolloff gain = min / (min + rolloff * (d - min))
//    with d clamped to [min, max] (defaults 1 and 1e9), set by FSOUND_3D_SetRolloffFactor. Doppler is not modelled
//    (velocities are accepted and ignored).
//  * streams: decoded by a worker thread into a stereo int16 ring; FSOUND_Stream_Open is non-blocking when mode has
//    FSOUND_NONBLOCKING (0x1000000): GetOpenState is -2 while loading, 0 ready, -1 failed. Mode bit 0x2 loops.
//  * path names use '\' and any case (the original ran on Windows): resolved case-insensitively against the cwd.
#include "mb_audio.h"
#include "wav.h"
#include <SDL3/SDL.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_PUSHDATA_API
extern "C" {
#include "third_party/stb_vorbis.c"
}
#undef STB_VORBIS_HEADER_ONLY

#if defined(__i386__)
#define F_API __attribute__((stdcall))
#else
#define F_API
#endif

namespace fs = std::filesystem;

namespace {
bool g_simd = true;
constexpr int kFree = -1, kAll = -3;
constexpr unsigned kLoopNormal = 2, kLoadMemory = 0x8000, kHw3d = 0x1000, k2d = 0x2000, kNonBlocking = 0x1000000;
constexpr size_t kRingFrames = 1 << 16;
constexpr int kChunk = 2048;

struct Sample {
    std::vector<int16_t> pcm;
    int channels = 1; size_t frames = 0; int rate = 44100;
    int defvol = 255, defpan = 128, defpri = 255, deffreq = 44100;
    bool is3d = false;
    float mindist = 1.f, maxdist = 1e9f;
    // lazy decode: Sample_Load reads only the header (length / rate / channels); PCM is decoded on first play
    std::string path; std::mutex dmu; std::atomic<bool> decoded{false};
};

struct Stream {
    std::string path; std::vector<uint8_t> mem; bool from_memory = false;
    std::atomic<int> state{-2};
    std::atomic<bool> quit{false}, loop{false}, eof{false}, ended{false};
    std::thread th;
    int rate = 44100; uint64_t total = 0;
    std::vector<int16_t> ring;                 // kRingFrames stereo frames
    std::atomic<uint64_t> wr{0}, rd{0};
    std::atomic<uint64_t> played{0};           // source frames consumed
    int vol = 255;
};

struct Voice {
    bool used = false; uint32_t serial = 0;
    Sample* smp = nullptr; Stream* str = nullptr;
    double pos = 0; int freq = 44100; int vol = 255, pan = 128, pri = 255;
    bool paused = false, loop = false, is3d = false;
    float p[3] = {0, 0, 0};
    float gl = 0, gr = 0; bool primed = false;   // last block's gains, for ramping
    double frac = 0;                             // stream: fractional position into the ring
};

struct Mixer {
    std::mutex mu;
    bool inited = false, offline = false;
    int rate = 44100, nvoices = 64, master = 255;
    float rolloff = 1.f;
    float lpos[3] = {0, 0, 0}, lfwd[3] = {0, 0, 1}, lup[3] = {0, 1, 0};
    std::vector<Voice> v;
    uint32_t serial = 0;
    std::vector<std::unique_ptr<Sample>> samples;
    std::vector<std::unique_ptr<Stream>> streams;
    SDL_AudioStream* dev = nullptr;
    bool sdl_audio = false;
} M;

bool g_offline_request = false;

// ---- files ------------------------------------------------------------------------------------------------------
std::string resolve(const char* name)
{
    std::string s = name ? name : "";
    for (auto& c : s) if (c == '\\') c = '/';
    std::error_code ec;
    if (fs::exists(s, ec)) return s;
    fs::path cur = (!s.empty() && s[0] == '/') ? fs::path("/") : fs::path(".");
    size_t i = 0;
    while (i < s.size()) {
        size_t j = s.find('/', i); if (j == std::string::npos) j = s.size();
        std::string comp = s.substr(i, j - i); i = j + 1;
        if (comp.empty() || comp == ".") continue;
        fs::path direct = cur / comp;
        if (fs::exists(direct, ec)) { cur = direct; continue; }
        bool found = false;
        for (auto& e : fs::directory_iterator(cur, ec)) {
            std::string n = e.path().filename().string();
            if (n.size() == comp.size() && SDL_strcasecmp(n.c_str(), comp.c_str()) == 0) { cur = e.path(); found = true; break; }
        }
        if (!found) return std::string();
    }
    return cur.string();
}

bool read_file(const std::string& path, std::vector<uint8_t>& out)
{
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return false;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return false; }
    out.resize((size_t)n);
    bool ok = n == 0 || fread(out.data(), 1, (size_t)n, f) == (size_t)n;
    fclose(f); return ok;
}

bool is_ogg(const uint8_t* p, size_t n) { return n >= 4 && !memcmp(p, "OggS", 4); }

bool decode_any(const uint8_t* p, size_t n, mbaudio::Pcm& out)
{
    if (is_ogg(p, n)) {
        int ch = 0, rate = 0; short* pcm = nullptr;
        int frames = stb_vorbis_decode_memory(p, (int)n, &ch, &rate, &pcm);
        if (frames <= 0 || !pcm) { free(pcm); return false; }
        int oc = ch > 2 ? 2 : ch;
        out.channels = oc; out.rate = rate; out.frames = (size_t)frames; out.data.resize((size_t)frames * oc);
        for (int i = 0; i < frames; i++) for (int c = 0; c < oc; c++) out.data[(size_t)i * oc + c] = pcm[(size_t)i * ch + c];
        free(pcm); return true;
    }
    return mbaudio::decode_wav(p, n, out);
}

// ---- channels ---------------------------------------------------------------------------------------------------
Voice* find(int h)
{
    if (!M.inited || h < 0) return nullptr;
    int idx = h & 0xfff; uint32_t ser = (uint32_t)h >> 12;
    if (idx >= (int)M.v.size()) return nullptr;
    Voice& v = M.v[idx];
    if (!v.used) return nullptr;
    if (ser != 0 && ser != v.serial) return nullptr;
    return &v;
}

void free_voice(Voice& v)
{
    if (v.str) { v.str->ended = true; if (!v.str->loop) v.str->played = v.str->total; }
    v.used = false; v.smp = nullptr; v.str = nullptr;
}

// returns channel index or -1
int alloc_voice(int req, int pri)
{
    if (req >= 0 && (req & 0xfff) < (int)M.v.size()) {   // explicit channel: take it over
        int idx = req & 0xfff; if (M.v[idx].used) free_voice(M.v[idx]); return idx;
    }
    int best = -1;
    for (int i = 0; i < (int)M.v.size(); i++) if (!M.v[i].used) return i;
    for (int i = 0; i < (int)M.v.size(); i++)
        if (M.v[i].pri <= pri && (best < 0 || M.v[i].pri < M.v[best].pri)) best = i;
    if (best >= 0) free_voice(M.v[best]);
    return best;
}

int start_voice(int req, Sample* s, Stream* st, bool paused)
{
    int pri = s ? s->defpri : 255;
    int idx = alloc_voice(req, pri);
    if (idx < 0) return -1;
    Voice& v = M.v[idx];
    v = Voice();
    v.used = true; v.serial = (++M.serial) & 0x7ffff; if (!v.serial) v.serial = (++M.serial) & 0x7ffff;
    v.smp = s; v.str = st; v.paused = paused;
    if (s) { v.freq = s->deffreq; v.vol = s->defvol; v.pan = s->defpan; v.pri = s->defpri; v.is3d = s->is3d; }
    else { v.freq = st->rate; v.vol = 255; v.pan = 128; v.pri = 255; v.loop = st->loop; }
    return (int)((v.serial << 12) | (uint32_t)idx);
}

// ---- stream decoder thread --------------------------------------------------------------------------------------
void stream_thread(Stream* st)
{
    stb_vorbis* vb = nullptr; mbaudio::Pcm pcm; bool is_pcm = false; int vch = 2;
    std::vector<uint8_t> file;
    if (st->from_memory) file = st->mem;
    else {
        std::string p = resolve(st->path.c_str());
        if (p.empty() || !read_file(p, file)) { st->state = -1; return; }
    }
    if (is_ogg(file.data(), file.size())) {
        int err = 0;
        vb = stb_vorbis_open_memory(file.data(), (int)file.size(), &err, nullptr);   // `file` outlives vb (this frame)
        if (!vb) { st->state = -1; return; }
        stb_vorbis_info info = stb_vorbis_get_info(vb);
        st->rate = (int)info.sample_rate; vch = info.channels;
        st->total = stb_vorbis_stream_length_in_samples(vb);
    } else {
        if (!mbaudio::decode_wav(file.data(), file.size(), pcm)) { st->state = -1; return; }
        is_pcm = true; st->rate = pcm.rate; st->total = pcm.frames; vch = pcm.channels;
    }
    st->ring.assign(kRingFrames * 2, 0);
    size_t pcm_pos = 0;
    std::vector<int16_t> tmp((size_t)kChunk * 2);
    bool ready = false;
    while (!st->quit) {
        uint64_t w = st->wr.load(std::memory_order_relaxed), r = st->rd.load(std::memory_order_acquire);
        if (!st->eof && kRingFrames - (w - r) >= (uint64_t)kChunk) {
            int n = 0;
            if (is_pcm) {
                size_t left = pcm.frames - pcm_pos; n = (int)(left < (size_t)kChunk ? left : kChunk);
                for (int i = 0; i < n; i++) {
                    int16_t a = pcm.data[(pcm_pos + i) * pcm.channels], b = pcm.channels > 1 ? pcm.data[(pcm_pos + i) * pcm.channels + 1] : a;
                    tmp[i * 2] = a; tmp[i * 2 + 1] = b;
                }
                pcm_pos += n;
                if (n == 0 && st->loop && pcm.frames) pcm_pos = 0;
            } else {
                n = stb_vorbis_get_samples_short_interleaved(vb, 2, tmp.data(), kChunk * 2);
                if (n == 0 && st->loop) { if (!stb_vorbis_seek_start(vb)) { } n = stb_vorbis_get_samples_short_interleaved(vb, 2, tmp.data(), kChunk * 2); }
            }
            (void)vch;
            if (n <= 0) { st->eof = true; }
            else {
                for (int i = 0; i < n; i++) {
                    size_t slot = (size_t)((w + i) % kRingFrames) * 2;
                    st->ring[slot] = tmp[i * 2]; st->ring[slot + 1] = tmp[i * 2 + 1];
                }
                st->wr.store(w + n, std::memory_order_release);
            }
            if (!ready && (st->eof || st->wr.load() >= 16384)) { ready = true; st->state = 0; }
            continue;
        }
        if (!ready && st->eof) { ready = true; st->state = 0; }
        SDL_Delay(4);
    }
    if (vb) stb_vorbis_close(vb);
}

// ---- mixing -----------------------------------------------------------------------------------------------------
inline float law_l(int pan) { float g = (255 - pan) / 127.5f; return g > 1.f ? 1.f : g < 0.f ? 0.f : g; }
inline float law_r(int pan) { float g = pan / 127.5f; return g > 1.f ? 1.f : g < 0.f ? 0.f : g; }

void voice_gains(const Voice& v, float& gl, float& gr)
{
    float g = (v.vol / 255.f) * (M.master / 255.f);
    int pan = v.pan;
    if (v.is3d) {
        float d[3] = {v.p[0] - M.lpos[0], v.p[1] - M.lpos[1], v.p[2] - M.lpos[2]};
        float dist = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        const float* f = M.lfwd; const float* u = M.lup;
        float right[3] = {u[1] * f[2] - u[2] * f[1], u[2] * f[0] - u[0] * f[2], u[0] * f[1] - u[1] * f[0]};   // up x fwd
        float rl = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
        float mn = v.smp ? v.smp->mindist : 1.f, mx = v.smp ? v.smp->maxdist : 1e9f;
        float dc = dist < mn ? mn : dist > mx ? mx : dist;
        g *= mn / (mn + M.rolloff * (dc - mn));
        float x = 0.f;
        if (dist > 1e-4f && rl > 1e-6f) { x = (d[0] * right[0] + d[1] * right[1] + d[2] * right[2]) / (dist * rl); }
        // sources close to the head pan less (the smoothed region is one min-distance wide)
        if (dist < mn) x *= dist / mn;
        pan = (int)(127.5f + 127.5f * x);
    }
    gl = g * law_l(pan); gr = g * law_r(pan);
}

void mix_block(float* out, int frames)   // mu held; out is zeroed by the caller
{
    for (auto& v : M.v) {
        if (!v.used || v.paused) continue;
        float tl, tr; voice_gains(v, tl, tr);
        if (!v.primed) { v.gl = tl; v.gr = tr; v.primed = true; }
        float sl = v.gl, sr = v.gr;
        float dl = (tl - v.gl) / frames, dr = (tr - v.gr) / frames;
        if (v.smp) {
            const Sample& s = *v.smp; const int ch = s.channels; const int16_t* d = s.pcm.data();
            double step = (double)v.freq / M.rate;
            int i = 0;
            if (g_simd && frames >= 8) {   // SIMD fast path: whole 4-frame groups that stay inside the sample (no wrap / end)
                typedef float f4 __attribute__((ext_vector_type(4)));
                int n4 = frames & ~3;
                double lastpos = v.pos + (double)(n4 - 1) * step;
                if (lastpos + 2.0 < (double)s.frames && step < 64.0) {
                    const double K = 4294967296.0;
                    uint64_t pf = (uint64_t)(v.pos * K), sf = (uint64_t)(step * K);
                    const f4 kk = {1.f, 2.f, 3.f, 4.f}; const float inv = 1.f / 32768.f;
                    const bool mono = ch == 1, mix3d = v.is3d;
                    for (; i < n4; i += 4) {
                        f4 x0, x1, y0, y1; uint64_t q = pf;
                        for (int k = 0; k < 4; k++, q += sf) {
                            size_t i0 = (size_t)(q >> 32);
                            if (mono) { x0[k] = d[i0]; x1[k] = d[i0 + 1]; }
                            else { x0[k] = d[i0 * 2]; x1[k] = d[i0 * 2 + 2]; y0[k] = d[i0 * 2 + 1]; y1[k] = d[i0 * 2 + 3]; }
                        }
                        f4 fr; q = pf;
                        for (int k = 0; k < 4; k++, q += sf) fr[k] = (float)(uint32_t)(q & 0xffffffffu) * (1.f / 4294967296.f);
                        f4 a = x0 + (x1 - x0) * fr, b = a;
                        if (!mono) { b = y0 + (y1 - y0) * fr; if (mix3d) a = b = 0.5f * (a + b); }
                        f4 gL = sl + dl * kk, gR = sr + dr * kk;
                        a *= gL * inv; b *= gR * inv;
                        float* o = out + (size_t)i * 2;
                        f4 lo = {a[0], b[0], a[1], b[1]}, hi = {a[2], b[2], a[3], b[3]};
                        f4 o0 = {o[0], o[1], o[2], o[3]}, o1 = {o[4], o[5], o[6], o[7]};
                        o0 += lo; o1 += hi;
                        o[0] = o0[0]; o[1] = o0[1]; o[2] = o0[2]; o[3] = o0[3]; o[4] = o1[0]; o[5] = o1[1]; o[6] = o1[2]; o[7] = o1[3];
                        pf += 4 * sf;
                    }
                    sl += dl * (float)n4; sr += dr * (float)n4;
                    v.pos = (double)pf / K;
                }
            }
            for (; i < frames; i++) {
                if (v.pos >= (double)s.frames) {
                    if (v.loop && s.frames) v.pos = std::fmod(v.pos, (double)s.frames);
                    else { free_voice(v); break; }
                }
                size_t i0 = (size_t)v.pos; float fr = (float)(v.pos - (double)i0);
                size_t i1 = i0 + 1; if (i1 >= s.frames) i1 = v.loop ? 0 : i0;
                float a, b;
                if (ch == 1) { float x0 = d[i0], x1 = d[i1]; a = b = x0 + (x1 - x0) * fr; }
                else {
                    float l0 = d[i0 * 2], l1 = d[i1 * 2], r0 = d[i0 * 2 + 1], r1 = d[i1 * 2 + 1];
                    a = l0 + (l1 - l0) * fr; b = r0 + (r1 - r0) * fr;
                    if (v.is3d) a = b = 0.5f * (a + b);
                }
                sl += dl; sr += dr;
                out[i * 2] += a * (1.f / 32768.f) * sl; out[i * 2 + 1] += b * (1.f / 32768.f) * sr;
                v.pos += step;
            }
        } else if (v.str) {
            Stream& st = *v.str; double step = (double)v.freq / M.rate;
            uint64_t rd = st.rd.load(std::memory_order_relaxed);
            for (int i = 0; i < frames; i++) {
                uint64_t wr = st.wr.load(std::memory_order_acquire);
                uint64_t avail = wr - rd;
                bool eof = st.eof.load(std::memory_order_acquire);
                if (avail == 0 && eof) { st.rd.store(rd, std::memory_order_release); free_voice(v); goto next_voice; }
                if (avail < 2 && !eof) break;   // underrun: wait for the decoder
                const int16_t* r0 = &st.ring[(size_t)(rd % kRingFrames) * 2];
                const int16_t* r1 = avail >= 2 ? &st.ring[(size_t)((rd + 1) % kRingFrames) * 2] : r0;
                float fr = (float)v.frac;
                float a = r0[0] + (r1[0] - r0[0]) * fr, b = r0[1] + (r1[1] - r0[1]) * fr;
                sl += dl; sr += dr;
                out[i * 2] += a * (1.f / 32768.f) * sl; out[i * 2 + 1] += b * (1.f / 32768.f) * sr;
                v.frac += step;
                uint64_t adv = (uint64_t)v.frac; v.frac -= (double)adv;
                uint64_t cap = avail < adv ? avail : adv; rd += cap;
                st.played.fetch_add(cap, std::memory_order_relaxed);
            }
            st.rd.store(rd, std::memory_order_release);
            if (st.total && st.loop && st.played.load() >= st.total) st.played -= st.total;
        }
        if (v.used) { v.gl = tl; v.gr = tr; }
    next_voice:;
    }
}

void render_locked(float* out, int frames)
{
    for (int done = 0; done < frames;) {
        int n = frames - done; if (n > 1024) n = 1024;
        float* o = out + (size_t)done * 2;
        std::memset(o, 0, sizeof(float) * 2 * n);
        mix_block(o, n);
        for (int i = 0; i < n * 2; i++) o[i] = o[i] > 1.f ? 1.f : o[i] < -1.f ? -1.f : o[i];
        done += n;
    }
}

void SDLCALL dev_cb(void*, SDL_AudioStream* s, int additional, int)
{
    std::vector<float> buf(2048 * 2);
    while (additional > 0) {
        int frames = additional / 8; if (frames > 2048) frames = 2048; if (frames <= 0) frames = 1;
        { std::lock_guard<std::mutex> g(M.mu); render_locked(buf.data(), frames); }
        SDL_PutAudioStreamData(s, buf.data(), frames * 8);
        additional -= frames * 8;
    }
}

} // namespace

// ---- extras -----------------------------------------------------------------------------------------------------
extern "C" {
void mb_audio_set_simd(int on) { g_simd = on != 0; }
void mb_audio_set_offline(int on) { g_offline_request = on != 0; }
void mb_audio_render(float* out, int frames) { std::lock_guard<std::mutex> g(M.mu); render_locked(out, frames); }
int mb_audio_active_voices(void)
{
    std::lock_guard<std::mutex> g(M.mu); int n = 0; for (auto& v : M.v) n += v.used; return n;
}
int mb_audio_mixrate(void) { return M.rate; }

// ---- FMOD 3.75 API ----------------------------------------------------------------------------------------------
signed char F_API FSOUND_Init(int mixrate, int maxsoftwarechannels, unsigned int flags)
{
    std::lock_guard<std::mutex> g(M.mu);
    if (M.inited) return 1;
    M.rate = mixrate > 0 ? mixrate : 44100;
    M.nvoices = maxsoftwarechannels < 64 ? 64 : maxsoftwarechannels > 4095 ? 4095 : maxsoftwarechannels;
    M.v.assign((size_t)M.nvoices, Voice());
    M.master = 255; M.rolloff = 1.f;
    M.offline = g_offline_request;
    if (!M.offline) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return 0;
        M.sdl_audio = true;
        SDL_AudioSpec spec; spec.format = SDL_AUDIO_F32; spec.channels = 2; spec.freq = M.rate;
        M.dev = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, dev_cb, nullptr);
        if (!M.dev) { SDL_QuitSubSystem(SDL_INIT_AUDIO); M.sdl_audio = false; return 0; }
        SDL_ResumeAudioStreamDevice(M.dev);
    }
    M.inited = true;
    return 1;
}

void F_API FSOUND_Close()
{
    SDL_AudioStream* dev; bool sdl;
    std::vector<std::unique_ptr<Stream>> streams;
    {
        std::lock_guard<std::mutex> g(M.mu);
        if (!M.inited) return;
        M.inited = false; dev = M.dev; M.dev = nullptr; sdl = M.sdl_audio; M.sdl_audio = false;
        streams = std::move(M.streams); M.streams.clear();
    }
    if (dev) SDL_DestroyAudioStream(dev);   // joins the callback before we free anything it touches
    if (sdl) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    for (auto& s : streams) { s->quit = true; if (s->th.joinable()) s->th.join(); }
    std::lock_guard<std::mutex> g(M.mu);
    M.v.clear(); M.samples.clear();
}

void F_API FSOUND_Update() {}   // the mixer runs on its own thread; 3D state is applied at mix time

signed char F_API FSOUND_SetMaxHardwareChannels(int) { return 1; }   // all mixing is software

void F_API FSOUND_SetSFXMasterVolume(int volume)
{
    std::lock_guard<std::mutex> g(M.mu); M.master = volume < 0 ? 0 : volume > 255 ? 255 : volume;
}

void F_API FSOUND_3D_SetRolloffFactor(float r)
{
    std::lock_guard<std::mutex> g(M.mu); M.rolloff = r < 0.f ? 0.f : r;
}

void F_API FSOUND_3D_Listener_SetAttributes(const float* pos, const float* vel, float fx, float fy, float fz,
                                            float tx, float ty, float tz)
{
    (void)vel;
    std::lock_guard<std::mutex> g(M.mu);
    if (pos) { M.lpos[0] = pos[0]; M.lpos[1] = pos[1]; M.lpos[2] = pos[2]; }
    M.lfwd[0] = fx; M.lfwd[1] = fy; M.lfwd[2] = fz; M.lup[0] = tx; M.lup[1] = ty; M.lup[2] = tz;
}

struct FSOUND_SAMPLE;
struct FSOUND_STREAM;
struct FSOUND_DSPUNIT;

// Ogg Vorbis header without setting up a decoder: channels/rate from the identification header in the first page,
// length from the granule position of the last page. False if anything looks off (the caller falls back to stb_vorbis).
bool ogg_quick_info(const std::string& path, int& ch, int& rate, size_t& frames)
{
    FILE* f = fopen(path.c_str(), "rb"); if (!f) return false;
    bool ok = false; uint8_t h[512];
    size_t n = fread(h, 1, sizeof h, f);
    if (n > 40 && !memcmp(h, "OggS", 4)) {
        size_t pk = 27 + h[26];
        if (pk + 16 <= n && h[pk] == 1 && !memcmp(h + pk + 1, "vorbis", 6)) {
            ch = h[pk + 11]; rate = (int)(h[pk + 12] | (h[pk + 13] << 8) | (h[pk + 14] << 16) | ((uint32_t)h[pk + 15] << 24));
            fseek(f, 0, SEEK_END); long end = ftell(f);
            long from = end > 65536 ? end - 65536 : 0; fseek(f, from, SEEK_SET);
            std::vector<uint8_t> t((size_t)(end - from));
            if (fread(t.data(), 1, t.size(), f) == t.size()) {
                for (size_t i = t.size(); i >= 27 + 1; i--) {
                    size_t k = i - 27 - 1 + 1;   // candidate page start
                    if (k + 14 <= t.size() && !memcmp(&t[k], "OggS", 4)) {
                        uint64_t g = 0; for (int b = 7; b >= 0; b--) g = (g << 8) | t[k + 6 + b];
                        if (g != ~(uint64_t)0 && g > 0) { frames = (size_t)g; ok = ch >= 1 && rate > 0; }
                        break;
                    }
                }
            }
        }
    }
    fclose(f); return ok;
}

bool ensure_decoded(Sample* s)
{
    if (s->decoded.load(std::memory_order_acquire)) return true;
    std::lock_guard<std::mutex> g(s->dmu);
    if (s->decoded.load()) return true;
    std::vector<uint8_t> file; mbaudio::Pcm pcm;
    if (!read_file(s->path, file) || !decode_any(file.data(), file.size(), pcm) || pcm.frames == 0) return false;
    s->pcm = std::move(pcm.data); s->channels = pcm.channels; s->frames = pcm.frames;
    s->decoded.store(true, std::memory_order_release);
    return true;
}

FSOUND_SAMPLE* F_API FSOUND_Sample_Load(int index, const char* name_or_data, unsigned int mode, int offset, int length)
{
    (void)index;
    if (!name_or_data) return nullptr;
    auto s = std::make_unique<Sample>();
    s->is3d = !(mode & k2d);
    if (mode & kLoadMemory) {   // caller's buffer may go away: decode now
        const uint8_t* p = (const uint8_t*)name_or_data + (offset > 0 ? offset : 0); size_t n = length > 0 ? (size_t)length : 0;
        mbaudio::Pcm pcm;
        if (!decode_any(p, n, pcm) || pcm.frames == 0) return nullptr;
        s->pcm = std::move(pcm.data); s->channels = pcm.channels; s->frames = pcm.frames; s->rate = s->deffreq = pcm.rate;
        s->decoded = true;
    } else {
        std::string path = resolve(name_or_data);
        if (path.empty()) return nullptr;
        uint8_t magic[4] = {0, 0, 0, 0};
        if (FILE* f = fopen(path.c_str(), "rb")) { size_t r = fread(magic, 1, 4, f); (void)r; fclose(f); } else return nullptr;
        if (is_ogg(magic, 4)) {
            int qc = 0, qr = 0; size_t qf = 0;
            if (ogg_quick_info(path, qc, qr, qf)) { s->channels = qc > 2 ? 2 : qc; s->rate = s->deffreq = qr; s->frames = qf; s->path = path; goto have; }
            int err = 0; stb_vorbis* vb = stb_vorbis_open_filename(path.c_str(), &err, nullptr);
            if (!vb) return nullptr;
            stb_vorbis_info info = stb_vorbis_get_info(vb);
            unsigned len = stb_vorbis_stream_length_in_samples(vb);
            stb_vorbis_close(vb);
            if (info.channels < 1) return nullptr;
            s->channels = info.channels > 2 ? 2 : info.channels; s->rate = s->deffreq = (int)info.sample_rate; s->frames = len;
            if (!len) {   // header gives no length (a few files): decode now to learn it
                s->path = path;
                if (!ensure_decoded(s.get())) return nullptr;
            }
        } else {
            int ch = 0, rate = 0; size_t frames = 0;
            if (!mbaudio::wav_info(path.c_str(), ch, rate, frames) || frames == 0) return nullptr;
            s->channels = ch; s->rate = s->deffreq = rate; s->frames = frames;
        }
        s->path = path;
    }
have:
    Sample* raw = s.get();
    std::lock_guard<std::mutex> g(M.mu);
    M.samples.push_back(std::move(s));
    return (FSOUND_SAMPLE*)raw;
}

signed char F_API FSOUND_Sample_SetDefaults(FSOUND_SAMPLE* sp, int deffreq, int defvol, int defpan, int defpri)
{
    if (!sp) return 0; Sample* s = (Sample*)sp; std::lock_guard<std::mutex> g(M.mu);
    if (deffreq > 0) s->deffreq = deffreq;
    if (defvol >= 0) s->defvol = defvol > 255 ? 255 : defvol;
    if (defpan >= 0) s->defpan = defpan > 255 ? 255 : defpan;
    if (defpri >= 0) s->defpri = defpri > 255 ? 255 : defpri;
    return 1;
}

signed char F_API FSOUND_Sample_GetDefaults(FSOUND_SAMPLE* sp, int* deffreq, int* defvol, int* defpan, int* defpri)
{
    if (!sp) return 0; Sample* s = (Sample*)sp;
    if (deffreq) *deffreq = s->deffreq;
    if (defvol) *defvol = s->defvol;
    if (defpan) *defpan = s->defpan;
    if (defpri) *defpri = s->defpri;
    return 1;
}

unsigned int F_API FSOUND_Sample_GetLength(FSOUND_SAMPLE* sp) { return sp ? (unsigned)((Sample*)sp)->frames : 0; }

int F_API FSOUND_PlaySoundEx(int channel, FSOUND_SAMPLE* sp, FSOUND_DSPUNIT*, signed char startpaused)
{
    if (!sp) return -1;
    if (!ensure_decoded((Sample*)sp)) return -1;   // first play decodes (outside the mixer lock)
    std::lock_guard<std::mutex> g(M.mu);
    if (!M.inited) return -1;
    return start_voice(channel == kFree ? -1 : channel, (Sample*)sp, nullptr, startpaused != 0);
}

#define WITH_CH(call_all, call_one) \
    std::lock_guard<std::mutex> g(M.mu); \
    if (!M.inited) return 0; \
    if (channel == kAll) { for (auto& v : M.v) if (v.used) { call_all; } return 1; } \
    Voice* vp = find(channel); if (!vp) return 0; Voice& v = *vp; call_one; return 1;

signed char F_API FSOUND_StopSound(int channel) { WITH_CH(free_voice(v), free_voice(v)) }
signed char F_API FSOUND_SetPaused(int channel, signed char paused) { WITH_CH(v.paused = paused != 0, v.paused = paused != 0) }
signed char F_API FSOUND_SetVolume(int channel, int vol)
{
    int x = vol < 0 ? 0 : vol > 255 ? 255 : vol;
    WITH_CH(v.vol = x, v.vol = x)
}
signed char F_API FSOUND_SetPriority(int channel, int pri)
{
    int x = pri < 0 ? 0 : pri > 255 ? 255 : pri;
    WITH_CH(v.pri = x, v.pri = x)
}
signed char F_API FSOUND_SetFrequency(int channel, int freq)
{
    int f = freq;
    WITH_CH(v.freq = f > 0 ? f : (v.smp ? v.smp->deffreq : v.str->rate), v.freq = f > 0 ? f : (v.smp ? v.smp->deffreq : v.str->rate))
}
signed char F_API FSOUND_SetLoopMode(int channel, unsigned int mode)
{
    bool l = (mode & kLoopNormal) != 0;
    WITH_CH(v.loop = l, (v.loop = l, v.str ? (void)(v.str->loop = l) : (void)0))
}
signed char F_API FSOUND_SetCurrentPosition(int channel, unsigned int offset)
{
    std::lock_guard<std::mutex> g(M.mu);
    Voice* vp = find(channel); if (!vp || !vp->smp) return 0;   // streams are not seekable here
    vp->pos = offset < vp->smp->frames ? offset : 0;
    return 1;
}
signed char F_API FSOUND_3D_SetAttributes(int channel, const float* pos, const float* vel)
{
    (void)vel;
    std::lock_guard<std::mutex> g(M.mu);
    Voice* vp = find(channel); if (!vp) return 0;
    if (pos) { vp->p[0] = pos[0]; vp->p[1] = pos[1]; vp->p[2] = pos[2]; }
    return 1;
}

signed char F_API FSOUND_Stream_Close(FSOUND_STREAM* sp);
FSOUND_STREAM* F_API FSOUND_Stream_Open(const char* name_or_data, unsigned int mode, int offset, int length)
{
    if (!name_or_data) return nullptr;
    auto st = std::make_unique<Stream>();
    if (mode & kLoadMemory) {
        const uint8_t* p = (const uint8_t*)name_or_data + (offset > 0 ? offset : 0);
        st->mem.assign(p, p + (length > 0 ? length : 0)); st->from_memory = true;
    } else st->path = name_or_data;
    st->loop = (mode & kLoopNormal) != 0;
    Stream* raw = st.get();
    {
        std::lock_guard<std::mutex> g(M.mu);
        if (!M.inited) return nullptr;
        M.streams.push_back(std::move(st));
    }
    raw->th = std::thread(stream_thread, raw);
    if (!(mode & kNonBlocking)) while (raw->state.load() == -2) SDL_Delay(1);   // synchronous open
    if (raw->state.load() == -1 && !(mode & kNonBlocking)) { FSOUND_Stream_Close((FSOUND_STREAM*)raw); return nullptr; }
    return (FSOUND_STREAM*)raw;
}

signed char F_API FSOUND_Stream_Close(FSOUND_STREAM* sp)
{
    if (!sp) return 0;
    Stream* st = (Stream*)sp; std::unique_ptr<Stream> mine;
    {
        std::lock_guard<std::mutex> g(M.mu);
        for (auto& v : M.v) if (v.used && v.str == st) free_voice(v);
        for (size_t i = 0; i < M.streams.size(); i++)
            if (M.streams[i].get() == st) { mine = std::move(M.streams[i]); M.streams.erase(M.streams.begin() + i); break; }
    }
    if (!mine) return 0;
    mine->quit = true; if (mine->th.joinable()) mine->th.join();
    return 1;
}

int F_API FSOUND_Stream_Play(int channel, FSOUND_STREAM* sp)
{
    if (!sp) return -1; Stream* st = (Stream*)sp;
    if (st->state.load() != 0) return -1;
    std::lock_guard<std::mutex> g(M.mu);
    if (!M.inited) return -1;
    for (auto& v : M.v) if (v.used && v.str == st) free_voice(v);   // restart: one channel per stream
    st->ended = false;
    return start_voice(channel == kFree ? -1 : channel, nullptr, st, false);
}

int F_API FSOUND_Stream_GetOpenState(FSOUND_STREAM* sp) { return sp ? ((Stream*)sp)->state.load() : -1; }

int F_API FSOUND_Stream_GetLengthMs(FSOUND_STREAM* sp)
{
    if (!sp) return 0; Stream* st = (Stream*)sp;
    return st->state.load() == 0 ? (int)(st->total * 1000 / (uint64_t)st->rate) : 0;
}

int F_API FSOUND_Stream_GetTime(FSOUND_STREAM* sp)
{
    if (!sp) return 0; Stream* st = (Stream*)sp;
    if (st->state.load() != 0) return 0;
    uint64_t p = st->played.load();
    return (int)(p * 1000 / (uint64_t)st->rate);
}

} // extern "C"
