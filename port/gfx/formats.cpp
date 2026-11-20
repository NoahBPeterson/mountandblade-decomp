// port/gfx/formats.cpp: D3DFORMAT <-> SDL_GPU texture formats, row pitches, CPU conversions and DXTn decoding.
#include "gfx_internal.h"

namespace mbg {

#define G(x) SDL_GPU_TEXTUREFORMAT_##x
static const FormatInfo k_formats[] = {
    // d3d                     gpu                        bpp blk  depth stencil convert
    {D3DFMT_A8R8G8B8,          G(B8G8R8A8_UNORM),          4, 0, false, false, false},
    {D3DFMT_X8R8G8B8,          G(B8G8R8A8_UNORM),          4, 0, false, false, true},
    {D3DFMT_A8B8G8R8,          G(R8G8B8A8_UNORM),          4, 0, false, false, false},
    {D3DFMT_X8B8G8R8,          G(R8G8B8A8_UNORM),          4, 0, false, false, true},
    {D3DFMT_R8G8B8,            G(B8G8R8A8_UNORM),          3, 0, false, false, true},
    {D3DFMT_R5G6B5,            G(B8G8R8A8_UNORM),          2, 0, false, false, true},
    {D3DFMT_X1R5G5B5,          G(B8G8R8A8_UNORM),          2, 0, false, false, true},
    {D3DFMT_A1R5G5B5,          G(B8G8R8A8_UNORM),          2, 0, false, false, true},
    {D3DFMT_A4R4G4B4,          G(B8G8R8A8_UNORM),          2, 0, false, false, true},
    {D3DFMT_X4R4G4B4,          G(B8G8R8A8_UNORM),          2, 0, false, false, true},
    {D3DFMT_R3G3B2,            G(B8G8R8A8_UNORM),          1, 0, false, false, true},
    {D3DFMT_A8R3G3B2,          G(B8G8R8A8_UNORM),          2, 0, false, false, true},
    {D3DFMT_A8,                G(A8_UNORM),                1, 0, false, false, false},
    {D3DFMT_L8,                G(B8G8R8A8_UNORM),          1, 0, false, false, true},
    {D3DFMT_A8L8,              G(B8G8R8A8_UNORM),          2, 0, false, false, true},
    {D3DFMT_A4L4,              G(B8G8R8A8_UNORM),          1, 0, false, false, true},
    {D3DFMT_L16,               G(R16_UNORM),               2, 0, false, false, false},
    {D3DFMT_V8U8,              G(R8G8_SNORM),              2, 0, false, false, false},
    {D3DFMT_Q8W8V8U8,          G(R8G8B8A8_SNORM),          4, 0, false, false, false},
    {D3DFMT_G16R16,            G(R16G16_UNORM),            4, 0, false, false, false},
    {D3DFMT_A16B16G16R16,      G(R16G16B16A16_UNORM),      8, 0, false, false, false},
    {D3DFMT_A2B10G10R10,       G(R10G10B10A2_UNORM),       4, 0, false, false, false},
    {D3DFMT_A2R10G10B10,       G(R10G10B10A2_UNORM),       4, 0, false, false, true},
    {D3DFMT_R16F,              G(R16_FLOAT),               2, 0, false, false, false},
    {D3DFMT_G16R16F,           G(R16G16_FLOAT),            4, 0, false, false, false},
    {D3DFMT_A16B16G16R16F,     G(R16G16B16A16_FLOAT),      8, 0, false, false, false},
    {D3DFMT_R32F,              G(R32_FLOAT),               4, 0, false, false, false},
    {D3DFMT_G32R32F,           G(R32G32_FLOAT),            8, 0, false, false, false},
    {D3DFMT_A32B32G32R32F,     G(R32G32B32A32_FLOAT),     16, 0, false, false, false},
    {D3DFMT_DXT1,              G(BC1_RGBA_UNORM),          0, 8, false, false, false},
    {D3DFMT_DXT2,              G(BC2_RGBA_UNORM),          0, 16, false, false, false},
    {D3DFMT_DXT3,              G(BC2_RGBA_UNORM),          0, 16, false, false, false},
    {D3DFMT_DXT4,              G(BC3_RGBA_UNORM),          0, 16, false, false, false},
    {D3DFMT_DXT5,              G(BC3_RGBA_UNORM),          0, 16, false, false, false},
    {D3DFMT_D16,               G(D16_UNORM),               2, 0, true, false, false},
    {D3DFMT_D16_LOCKABLE,      G(D16_UNORM),               2, 0, true, false, false},
    {D3DFMT_D24S8,             G(D24_UNORM_S8_UINT),       4, 0, true, true, false},
    {D3DFMT_D24X8,             G(D24_UNORM_S8_UINT),       4, 0, true, true, false},
    {D3DFMT_D24X4S4,           G(D24_UNORM_S8_UINT),       4, 0, true, true, false},
    {D3DFMT_D15S1,             G(D24_UNORM_S8_UINT),       2, 0, true, true, false},
    {D3DFMT_D24FS8,            G(D24_UNORM_S8_UINT),       4, 0, true, true, false},
    {D3DFMT_D32,               G(D32_FLOAT),               4, 0, true, false, false},
    {D3DFMT_D32F_LOCKABLE,     G(D32_FLOAT),               4, 0, true, false, false},
};
#undef G

const FormatInfo* format_info(D3DFORMAT f)
{
    for (const FormatInfo& fi : k_formats)
        if (fi.d3d == f) return &fi;
    return nullptr;
}

bool format_is_block(D3DFORMAT f)
{
    const FormatInfo* fi = format_info(f);
    return fi && fi->block;
}

UINT format_pitch(D3DFORMAT f, UINT w)
{
    const FormatInfo* fi = format_info(f);
    if (!fi) return w * 4;
    if (fi->block) return std::max(1u, (w + 3) / 4) * fi->block;
    return w * fi->bpp;
}

UINT format_rows(D3DFORMAT f, UINT h)
{
    return format_is_block(f) ? std::max(1u, (h + 3) / 4) : h;
}

UINT format_level_size(D3DFORMAT f, UINT w, UINT h) { return format_pitch(f, w) * format_rows(f, h); }

// ---- DXTn ----
static inline void rgb565(uint16_t c, uint8_t* bgra)
{
    uint8_t r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
    bgra[2] = (uint8_t)((r << 3) | (r >> 2));
    bgra[1] = (uint8_t)((g << 2) | (g >> 4));
    bgra[0] = (uint8_t)((b << 3) | (b >> 2));
    bgra[3] = 255;
}

static void decode_color_block(const uint8_t* b, uint8_t out[16][4], bool dxt1)
{
    uint16_t c0 = (uint16_t)(b[0] | (b[1] << 8)), c1 = (uint16_t)(b[2] | (b[3] << 8));
    uint8_t pal[4][4];
    rgb565(c0, pal[0]);
    rgb565(c1, pal[1]);
    if (c0 > c1 || !dxt1) {
        for (int i = 0; i < 3; i++) {
            pal[2][i] = (uint8_t)((2 * pal[0][i] + pal[1][i] + 1) / 3);
            pal[3][i] = (uint8_t)((pal[0][i] + 2 * pal[1][i] + 1) / 3);
        }
        pal[2][3] = pal[3][3] = 255;
    } else {
        for (int i = 0; i < 3; i++) {
            pal[2][i] = (uint8_t)((pal[0][i] + pal[1][i]) / 2);
            pal[3][i] = 0;
        }
        pal[2][3] = 255;
        pal[3][3] = 0;
    }
    uint32_t bits = (uint32_t)b[4] | ((uint32_t)b[5] << 8) | ((uint32_t)b[6] << 16) | ((uint32_t)b[7] << 24);
    for (int i = 0; i < 16; i++) {
        memcpy(out[i], pal[(bits >> (2 * i)) & 3], 4);
    }
}

void decode_dxt_block(D3DFORMAT f, const uint8_t* blk, uint8_t out[16][4])
{
    if (f == D3DFMT_DXT1) {
        decode_color_block(blk, out, true);
        return;
    }
    decode_color_block(blk + 8, out, false);
    if (f == D3DFMT_DXT2 || f == D3DFMT_DXT3) {
        for (int i = 0; i < 16; i++) {
            uint8_t a = (blk[i / 2] >> ((i & 1) * 4)) & 15;
            out[i][3] = (uint8_t)(a * 17);
        }
    } else {
        uint8_t a0 = blk[0], a1 = blk[1];
        uint8_t pal[8];
        pal[0] = a0;
        pal[1] = a1;
        if (a0 > a1) {
            for (int i = 1; i < 7; i++) pal[i + 1] = (uint8_t)(((7 - i) * a0 + i * a1 + 3) / 7);
        } else {
            for (int i = 1; i < 5; i++) pal[i + 1] = (uint8_t)(((5 - i) * a0 + i * a1 + 2) / 5);
            pal[6] = 0;
            pal[7] = 255;
        }
        uint64_t bits = 0;
        for (int i = 0; i < 6; i++) bits |= (uint64_t)blk[2 + i] << (8 * i);
        for (int i = 0; i < 16; i++) out[i][3] = pal[(bits >> (3 * i)) & 7];
    }
}

bool decode_to_bgra(D3DFORMAT f, const uint8_t* src, UINT src_pitch, UINT w, UINT h, uint8_t* dst, UINT dst_pitch)
{
    if (format_is_block(f)) {
        const FormatInfo* fi = format_info(f);
        UINT bw = (w + 3) / 4, bh = (h + 3) / 4;
        for (UINT by = 0; by < bh; by++)
            for (UINT bx = 0; bx < bw; bx++) {
                uint8_t px[16][4];
                decode_dxt_block(f, src + by * src_pitch + bx * fi->block, px);
                for (int i = 0; i < 16; i++) {
                    UINT x = bx * 4 + (i & 3), y = by * 4 + (i >> 2);
                    if (x < w && y < h) memcpy(dst + y * dst_pitch + x * 4, px[i], 4);
                }
            }
        return true;
    }
    for (UINT y = 0; y < h; y++) {
        const uint8_t* s = src + y * src_pitch;
        uint8_t* d = dst + y * dst_pitch;
        for (UINT x = 0; x < w; x++, d += 4) {
            switch (f) {
            case D3DFMT_A8R8G8B8: memcpy(d, s + 4 * x, 4); break;
            case D3DFMT_X8R8G8B8: memcpy(d, s + 4 * x, 3); d[3] = 255; break;
            case D3DFMT_A8B8G8R8: d[0] = s[4 * x + 2]; d[1] = s[4 * x + 1]; d[2] = s[4 * x]; d[3] = s[4 * x + 3]; break;
            case D3DFMT_X8B8G8R8: d[0] = s[4 * x + 2]; d[1] = s[4 * x + 1]; d[2] = s[4 * x]; d[3] = 255; break;
            case D3DFMT_R8G8B8: d[0] = s[3 * x]; d[1] = s[3 * x + 1]; d[2] = s[3 * x + 2]; d[3] = 255; break;
            case D3DFMT_R5G6B5: { uint16_t c; memcpy(&c, s + 2 * x, 2); rgb565(c, d); break; }
            case D3DFMT_X1R5G5B5:
            case D3DFMT_A1R5G5B5: {
                uint16_t c; memcpy(&c, s + 2 * x, 2);
                uint8_t r = (c >> 10) & 31, g = (c >> 5) & 31, b = c & 31;
                d[2] = (uint8_t)((r << 3) | (r >> 2)); d[1] = (uint8_t)((g << 3) | (g >> 2)); d[0] = (uint8_t)((b << 3) | (b >> 2));
                d[3] = (f == D3DFMT_X1R5G5B5 || (c & 0x8000)) ? 255 : 0;
                break;
            }
            case D3DFMT_A4R4G4B4:
            case D3DFMT_X4R4G4B4: {
                uint16_t c; memcpy(&c, s + 2 * x, 2);
                d[3] = f == D3DFMT_X4R4G4B4 ? 255 : (uint8_t)(((c >> 12) & 15) * 17);
                d[2] = (uint8_t)(((c >> 8) & 15) * 17); d[1] = (uint8_t)(((c >> 4) & 15) * 17); d[0] = (uint8_t)((c & 15) * 17);
                break;
            }
            case D3DFMT_R3G3B2:
            case D3DFMT_A8R3G3B2: {
                uint16_t c = f == D3DFMT_R3G3B2 ? (uint16_t)(0xff00 | s[x]) : (uint16_t)(s[2 * x] | (s[2 * x + 1] << 8));
                uint8_t r = (c >> 5) & 7, g = (c >> 2) & 7, b = c & 3;
                d[2] = (uint8_t)(r * 255 / 7); d[1] = (uint8_t)(g * 255 / 7); d[0] = (uint8_t)(b * 85); d[3] = (uint8_t)(c >> 8);
                break;
            }
            case D3DFMT_A8: d[0] = d[1] = d[2] = 0; d[3] = s[x]; break;
            case D3DFMT_L8: d[0] = d[1] = d[2] = s[x]; d[3] = 255; break;
            case D3DFMT_A8L8: d[0] = d[1] = d[2] = s[2 * x]; d[3] = s[2 * x + 1]; break;
            case D3DFMT_A4L4: d[0] = d[1] = d[2] = (uint8_t)((s[x] & 15) * 17); d[3] = (uint8_t)((s[x] >> 4) * 17); break;
            case D3DFMT_L16: d[0] = d[1] = d[2] = s[2 * x + 1]; d[3] = 255; break;
            case D3DFMT_A2R10G10B10:
            case D3DFMT_A2B10G10R10: {
                uint32_t c; memcpy(&c, s + 4 * x, 4);
                uint8_t lo = (uint8_t)((c >> 2) & 255), mid = (uint8_t)((c >> 12) & 255), hi = (uint8_t)((c >> 22) & 255);
                if (f == D3DFMT_A2R10G10B10) { d[0] = lo; d[1] = mid; d[2] = hi; }
                else { d[2] = lo; d[1] = mid; d[0] = hi; }
                d[3] = (uint8_t)((c >> 30) * 85);
                break;
            }
            case D3DFMT_R32F: { float v; memcpy(&v, s + 4 * x, 4); uint8_t c = (uint8_t)(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); d[0] = d[1] = d[2] = c; d[3] = 255; break; }
            default: return false;
            }
        }
    }
    return true;
}

bool convert_to_gpu(D3DFORMAT f, SDL_GPUTextureFormat gpu, const uint8_t* src, UINT w, UINT h, std::vector<uint8_t>& out)
{
    UINT pitch = format_pitch(f, w);
    if (gpu == SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM && f != D3DFMT_A8R8G8B8) {
        out.resize((size_t)w * h * 4);
        return decode_to_bgra(f, src, pitch, w, h, out.data(), w * 4);
    }
    if (gpu == SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM && f == D3DFMT_X8B8G8R8) {
        out.assign(src, src + (size_t)pitch * h);
        for (size_t i = 3; i < out.size(); i += 4) out[i] = 255;
        return true;
    }
    if (gpu == SDL_GPU_TEXTUREFORMAT_R10G10B10A2_UNORM && f == D3DFMT_A2R10G10B10) {
        out.resize((size_t)pitch * h);
        for (size_t i = 0; i < (size_t)w * h; i++) {
            uint32_t c; memcpy(&c, src + 4 * i, 4);
            uint32_t b = c & 1023, g = (c >> 10) & 1023, r = (c >> 20) & 1023, a = c >> 30;
            uint32_t o = r | (g << 10) | (b << 20) | (a << 30);
            memcpy(&out[4 * i], &o, 4);
        }
        return true;
    }
    out.assign(src, src + (size_t)format_level_size(f, w, h));
    return true;
}

bool convert_from_gpu(D3DFORMAT f, SDL_GPUTextureFormat gpu, const uint8_t* src, UINT w, UINT h, uint8_t* dst, UINT dst_pitch)
{
    if (gpu == SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM) {
        for (UINT y = 0; y < h; y++) {
            const uint8_t* s = src + (size_t)y * w * 4;
            uint8_t* d = dst + (size_t)y * dst_pitch;
            for (UINT x = 0; x < w; x++, s += 4) {
                switch (f) {
                case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8: memcpy(d + 4 * x, s, 4); break;
                case D3DFMT_A8B8G8R8: case D3DFMT_X8B8G8R8: d[4 * x] = s[2]; d[4 * x + 1] = s[1]; d[4 * x + 2] = s[0]; d[4 * x + 3] = s[3]; break;
                case D3DFMT_R8G8B8: memcpy(d + 3 * x, s, 3); break;
                case D3DFMT_R5G6B5: { uint16_t c = (uint16_t)(((s[2] >> 3) << 11) | ((s[1] >> 2) << 5) | (s[0] >> 3)); memcpy(d + 2 * x, &c, 2); break; }
                case D3DFMT_X1R5G5B5: case D3DFMT_A1R5G5B5: { uint16_t c = (uint16_t)(((s[3] >= 128) << 15) | ((s[2] >> 3) << 10) | ((s[1] >> 3) << 5) | (s[0] >> 3)); memcpy(d + 2 * x, &c, 2); break; }
                case D3DFMT_A4R4G4B4: case D3DFMT_X4R4G4B4: { uint16_t c = (uint16_t)(((s[3] >> 4) << 12) | ((s[2] >> 4) << 8) | ((s[1] >> 4) << 4) | (s[0] >> 4)); memcpy(d + 2 * x, &c, 2); break; }
                case D3DFMT_L8: d[x] = s[2]; break;
                case D3DFMT_A8L8: d[2 * x] = s[2]; d[2 * x + 1] = s[3]; break;
                default: return false;
                }
            }
        }
        return true;
    }
    // same layout
    UINT pitch = format_pitch(f, w);
    UINT rows = format_rows(f, h);
    for (UINT y = 0; y < rows; y++) memcpy(dst + (size_t)y * dst_pitch, src + (size_t)y * pitch, pitch);
    return true;
}

} // namespace mbg
