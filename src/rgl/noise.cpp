// noise.cpp (0x414ae0..0x4150ef): 3-D gradient noise and its fractal (octave) sum. Used by the particle
// code's neighbours and by much of the game (0x461570, 0x463a80, ...). Migrated from slice s00414960.
// Boundaries: content only (no call crosses either side). The noise tables (0x5a8160 .data, 0x5a8260 .data,
// 0x5d6f98 .bss) are probably defined in this object; they stay extern until their contents are migrated.
#include <math.h>
#include "./noise.h"

// ---- gradient noise (Perlin-style, cubic fade 1 - 3t^2 + 2t^3) ----------------------------------

// @ 0x414ae0
// Output is a per-component weighted sum (not a dot product) over the 8 lattice corners.
void noise_gradient(float scale, const float* p, float* out)
{
    out[2] = 0.0f;
    out[1] = 0.0f;
    out[0] = 0.0f;
    if (scale != 0.0) {
        float inv = 1.0f / scale;
        float x = (p[0] + 1.0f) * inv;
        float y = (p[1] + 1.0f) * inv;
        float z = (p[2] + 1.0f) * inv;
        int ix = (int)floor(x);
        x = x - (float)ix;
        int iy = (int)floor(y);
        y = y - (float)iy;
        int iz = (int)floor(z);
        z = z - (float)iz;
        float xm = x - 1.0f;
        float ym = y - 1.0f;
        float zm = z - 1.0f;
        float x2 = x * x, y2 = y * y, z2 = z * z, xm2 = xm * xm, ym2 = ym * ym, zm2 = zm * zm;
        float x3 = x2 * x, y3 = y2 * y, z3 = z2 * z, xm3 = xm2 * xm, ym3 = ym2 * ym, zm3 = zm2 * zm;
        float wx0 = (x3 + x3) + (1.0f - x2 * 3.0f);
        float wy0 = (y3 + y3) + (1.0f - y2 * 3.0f);
        float wz0 = (z3 + z3) + (1.0f - z2 * 3.0f);
        float wx1 = (1.0f - xm2 * 3.0f) - (xm3 + xm3);
        float wy1 = (1.0f - ym2 * 3.0f) - (ym3 + ym3);
        float wz1 = (1.0f - zm2 * 3.0f) - (zm3 + zm3);
        unsigned b00 = g_noise_perm[(g_noise_perm[ix & 0xff] + iy) & 0xff];
        unsigned b01 = g_noise_perm[(g_noise_perm[ix & 0xff] + 1 + iy) & 0xff];
        unsigned b10 = g_noise_perm[(g_noise_perm[(ix + 1) & 0xff] + iy) & 0xff];
        unsigned b11 = g_noise_perm[(g_noise_perm[(ix + 1) & 0xff] + 1 + iy) & 0xff];
        unsigned k0 = iz & 0xff;
        unsigned k1 = (iz + 1) & 0xff;
        float w;
        const float* g;
        w = ((wz0)*( wy0)) * wx0;
        g = g_noise_grad[g_noise_perm[(g_noise_seed + k0 + b00) & 0xff]];
        out[0] = (w * g[0]) * x + out[0];
        out[1] = (w * g[1]) * y + out[1];
        out[2] = (w * g[2]) * z + out[2];
        w = ((wz1)*( wy0)) * wx0;
        g = g_noise_grad[g_noise_perm[(g_noise_seed + k1 + b00) & 0xff]];
        out[0] = (w * g[0]) * x + out[0];
        out[1] = (w * g[1]) * y + out[1];
        out[2] = (w * g[2]) * zm + out[2];
        w = ((wy1)*( wz0)) * wx0;
        g = g_noise_grad[g_noise_perm[(g_noise_seed + k0 + b01) & 0xff]];
        out[0] = (w * g[0]) * x + out[0];
        out[1] = (w * g[1]) * ym + out[1];
        out[2] = (w * g[2]) * z + out[2];
        w = ((wz1)*( wy1)) * wx0;
        g = g_noise_grad[g_noise_perm[(g_noise_seed + k1 + b01) & 0xff]];
        out[0] = (w * g[0]) * x + out[0];
        out[1] = (w * g[1]) * ym + out[1];
        out[2] = (w * g[2]) * zm + out[2];
        w = ((wx1)*( wz0)) * wy0;
        g = g_noise_grad[g_noise_perm[(g_noise_seed + k0 + b10) & 0xff]];
        out[0] = (w * g[0]) * xm + out[0];
        out[1] = (w * g[1]) * y + out[1];
        out[2] = (w * g[2]) * z + out[2];
        w = ((wz1)*( wx1)) * wy0;
        g = g_noise_grad[g_noise_perm[(g_noise_seed + k1 + b10) & 0xff]];
        out[0] = (w * g[0]) * xm + out[0];
        out[1] = (w * g[1]) * y + out[1];
        out[2] = (w * g[2]) * zm + out[2];
        w = ((wy1)*( wx1)) * wz0;
        g = g_noise_grad[g_noise_perm[(g_noise_seed + k0 + b11) & 0xff]];
        out[0] = (w * g[0]) * xm + out[0];
        out[1] = (w * g[1]) * ym + out[1];
        out[2] = (w * g[2]) * z + out[2];
        w = ((wz1)*( wy1)) * wx1;
        g = g_noise_grad[g_noise_perm[(g_noise_seed + k1 + b11) & 0xff]];
        out[0] = (w * g[0]) * xm + out[0];
        out[1] = (w * g[1]) * ym + out[1];
        out[2] = (w * g[2]) * zm + out[2];
    }
}

// @ 0x414fc0
// The original keeps amp, total, the gradient (24 bytes, the callee fills 12) and the z product in one
// 0x28-byte frame; the struct reproduces that layout.
void noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out)
{
    struct { float amp, total; float g[6]; float z; float pad; } L;
    out[2] = 0.0f;
    out[1] = 0.0f;
    L.amp = 1.0f;
    L.total = 0.0f;
    out[0] = 0.0f;
    if (octaves >= 0) {
        int n = octaves + 1;
        do {
            noise_gradient(L.amp * freq, p, L.g);
            if (sharpen) {
                float s = (L.g[0] + L.g[2]) + L.g[1];
                if (s < 0.0)
                    s = s - 2.0f;
                else
                    s = s + 2.0f;
                L.g[0] = L.g[0] * s;
                L.g[1] = L.g[1] * s;
                L.g[2] = L.g[2] * s;
            }
            n--;
            float x = L.g[0] * L.amp;
            float y = L.g[1] * L.amp;
            L.z = L.g[2] * L.amp;
            out[0] = x + out[0];
            out[1] = y + out[1];
            out[2] = L.z + out[2];
            L.total = L.total + L.amp;
            L.amp = L.amp * gain;
            gain = 0.61f;
        } while (n != 0);
    }
    L.total = 1.0f / L.total;
    out[0] = L.total * out[0];
    out[1] = L.total * out[1];
    out[2] = L.total * out[2];
}
