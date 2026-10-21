// Slice s00414960: gradient-noise helpers, small file utilities, the text stream's base class and
// the stream's read-from-file opener. Declarations shared with s00415320 live in mbstream.h.
#include <windows.h>
#include <shlobj.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include "mbstream.h"
#include "../s00413e50/psys.h"

// @ 0x4150f0
named_object::~named_object() {}

// @ 0x415100
void mb_snprintf(char* buf, size_t n, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, n, fmt, ap);
}

// @ 0x415120
bool mb_file_exists(const char* path)
{
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}

// @ 0x415160
FILE* mb_open_user_file(const char* name, const char* mode)
{
    char path[260];
    char folder[260];
    SHGetFolderPathA(0, CSIDL_APPDATA, 0, 0, folder);
    sprintf(path, "%s\\Mount&Blade", folder);
    CreateDirectoryA(path, 0);
    sprintf(path, "%s\\Mount&Blade\\%s", folder, name);
    return fopen(path, mode);
}

// @ 0x4151e0
void tstring::read_from(FILE* f, size_t n)
{
    reserve(n + 1);
    size_t got = fread(buf, 1, n, f);
    buf[got] = 0;
    len = got;
}

// @ 0x415220
void stream_base::reset()
{
    buf.clear();
    buf.rewind();
}

// @ 0x415240
void stream_base::rewind() { buf.rewind(); }

// @ 0x415250
bool stream_base::at_end() { return buf.pos >= buf.len; }

// @ 0x415270
void stream_base::set_autoflush(char f) { autoflush = f; }

// @ 0x415280
char stream_base::get_autoflush() { return autoflush; }

// 0x408020 (shared by ICF with other empty `return 0` functions)
int stream_base::flush() { return 0; }

// @ 0x415290
bool text_stream::open_read(const char* path)
{
    name.assign(path);
    fp = fopen(path, "rb");
    if (fp)
        failed = 0;
    else
        failed = 1;
    if (!failed) {
        size_t n = mb_file_size(fp);
        buf.read_from(fp, n);
        fclose(fp);
    }
    return !failed;
}

// ---- gradient noise (Perlin-style, cubic fade 1 - 3t^2 + 2t^3) ----------------------------------
extern unsigned char g_noise_perm[];   // 0x5a8160 permutation table (256 entries, indices are masked with 0xff)
extern float g_noise_grad[][3];        // 0x5a8260 12-byte gradient vectors
extern int g_noise_seed;               // 0x5d6f98

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

// @ 0x414960
// Builds the frame the particles of this system are oriented by (s = axis-x-view, f = u x s, u = axis),
// looks the output mesh up on first use and emits every particle.
void psys::update(scene2* sc)
{
    view_t* v = sc->view;
    if (v) {
        if (cache == 0)
            cache = v->get_mesh(res);
        mat4 m;
        m.u = axis;
        m.f = v->axis;
        vector4 t, t2;
        t.x = ((m.f.y)*( m.u.z)) - ((m.u.y)*( m.f.z));
        t.y = ((m.f.z)*( m.u.x)) - ((m.u.z)*( m.f.x));
        t.z = ((m.f.x)*( m.u.y)) - ((m.f.y)*( m.u.x));
        m.s = t;
        m.s.normalize();
        t2.x = ((m.s.z)*( m.u.y)) - ((m.s.y)*( m.u.z));
        t2.y = ((m.u.z)*( m.s.x)) - ((m.s.z)*( m.u.x));
        t2.z = ((m.s.y)*( m.u.x)) - ((m.s.x)*( m.u.y));
        m.f = t2;
        if (begin != end) {
            particle* q = begin;
            do {
                emit(q, &m, cache, (const float*)&v->eye);
                q++;
            } while (q != end);
        }
    }
}
