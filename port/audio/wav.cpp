#include "wav.h"
#include <cstring>
#include <cstdio>
namespace mbaudio {
static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t rd16(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }
bool wav_info(const char* path, int& channels, int& rate, size_t& frames)
{
    FILE* f = fopen(path, "rb"); if (!f) return false;
    uint8_t h[12];
    bool ok = false; int fmt = 0, ch = 0, bits = 0;
    if (fread(h, 1, 12, f) == 12 && !memcmp(h, "RIFF", 4) && !memcmp(h + 8, "WAVE", 4)) {
        uint8_t c[8];
        while (fread(c, 1, 8, f) == 8) {
            uint32_t sz = rd32(c + 4);
            if (!memcmp(c, "fmt ", 4) && sz >= 16) {
                uint8_t b[40]; size_t rn = sz < 40 ? sz : 40;
                if (fread(b, 1, rn, f) != rn) break;
                fmt = rd16(b); ch = rd16(b + 2); rate = (int)rd32(b + 4); bits = rd16(b + 14);
                if (fmt == 0xFFFE && rn >= 26) fmt = rd16(b + 24);
                if (sz > rn) fseek(f, (long)(sz - rn), SEEK_CUR);
                if (sz & 1) fseek(f, 1, SEEK_CUR);
            } else if (!memcmp(c, "data", 4)) {
                if (ch < 1 || bits < 8) break;
                long here = ftell(f); fseek(f, 0, SEEK_END); long end = ftell(f);
                size_t avail = (size_t)(end - here); size_t n = sz > avail ? avail : sz;
                frames = n / (size_t)((bits / 8) * ch); channels = ch > 2 ? 2 : ch; ok = rate > 0;
                break;
            } else { fseek(f, (long)sz + (sz & 1), SEEK_CUR); }
        }
    }
    fclose(f); return ok && ((fmt == 1 && (bits == 8 || bits == 16 || bits == 24 || bits == 32)) || (fmt == 3 && bits == 32));
}
bool decode_wav(const uint8_t* p, size_t n, Pcm& out)
{
    if (n < 12 || memcmp(p, "RIFF", 4) || memcmp(p + 8, "WAVE", 4)) return false;
    size_t pos = 12; int fmt = 0, ch = 0, rate = 0, bits = 0; const uint8_t* d = nullptr; size_t dn = 0;
    while (pos + 8 <= n) {
        uint32_t sz = rd32(p + pos + 4); const uint8_t* c = p + pos + 8;
        size_t avail = n - (pos + 8); size_t csz = sz > avail ? avail : sz;
        if (!memcmp(p + pos, "fmt ", 4) && csz >= 16) {
            fmt = rd16(c); ch = rd16(c + 2); rate = (int)rd32(c + 4); bits = rd16(c + 14);
            if (fmt == 0xFFFE && csz >= 26) fmt = rd16(c + 24);   // WAVE_FORMAT_EXTENSIBLE: sub-format tag
        } else if (!memcmp(p + pos, "data", 4)) { d = c; dn = csz; break; }
        pos += 8 + csz + (csz & 1);
    }
    if (!d || ch < 1 || rate <= 0) return false;
    if (!((fmt == 1 && (bits == 8 || bits == 16 || bits == 24 || bits == 32)) || (fmt == 3 && bits == 32))) return false;
    int bps = bits / 8; size_t frames = dn / (size_t)(bps * ch);
    int oc = ch > 2 ? 2 : ch;
    out.data.resize(frames * oc); out.channels = oc; out.rate = rate; out.frames = frames;
    for (size_t i = 0; i < frames; i++) {
        for (int c = 0; c < oc; c++) {
            const uint8_t* s = d + (i * ch + c) * bps; int v;
            if (fmt == 3) { float f; memcpy(&f, s, 4); f *= 32767.f; v = f > 32767.f ? 32767 : f < -32768.f ? -32768 : (int)f; }
            else if (bits == 8) v = ((int)s[0] - 128) << 8;
            else if (bits == 16) v = (int16_t)rd16(s);
            else if (bits == 24) v = (int16_t)rd16(s + 1);
            else v = (int16_t)rd16(s + 2);
            out.data[i * oc + c] = (int16_t)v;
        }
    }
    return true;
}
}
