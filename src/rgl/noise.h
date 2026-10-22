// noise.h: gradient noise (src/rgl/noise.cpp, 0x414ae0..0x4150ef). Names provisional.
#ifndef RGL_NOISE_H
#define RGL_NOISE_H

extern unsigned char g_noise_perm[];   // 0x5a8160 permutation table (256 entries, indices are masked with 0xff)
extern float g_noise_grad[][3];        // 0x5a8260 12-byte gradient vectors
extern int g_noise_seed;               // 0x5d6f98

// Per-component weighted sum of the 8 lattice corners' gradients at p / scale (cubic fade 1 - 3t^2 + 2t^3).
void noise_gradient(float scale, const float* p, float* out);                                    // 0x414ae0
// octaves + 1 octaves of noise_gradient (gain applies from the second octave on; then 0.61), normalised.
void noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out); // 0x414fc0

#endif
