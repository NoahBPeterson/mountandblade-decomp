// port/audio/wav.h: minimal RIFF/WAVE reader (PCM 8/16/24/32, float32; mono/stereo(+downmix of >2) -> int16).
#pragma once
#include <cstdint>
#include <vector>
namespace mbaudio {
struct Pcm { std::vector<int16_t> data; int channels = 0; int rate = 0; size_t frames = 0; };
bool decode_wav(const uint8_t* p, size_t n, Pcm& out);
}
