// port/gfx/d3dx_misc.cpp: the D3DX helpers outside effects / shader assembly: matrix and plane math, ID3DXBuffer,
// D3DXCreateTexture, D3DXCreateTextureFromFileExA (DDS with D3DX_SKIP_DDS_MIP_LEVELS, TGA, BMP, PNG) and
// D3DXSaveSurfaceToFileA (BMP, PNG, TGA, DDS), plus the game-path resolver (backslashes, Windows case rules).
#include "gfx_internal.h"
#include <dirent.h>
#include <sys/stat.h>

namespace mbg {

// ---- paths ----
static bool exists(const std::string& p)
{
    struct stat sb;
    return stat(p.c_str(), &sb) == 0;
}

std::string resolve_path(const char* path)
{
    std::string p = path ? path : "";
    for (char& c : p)
        if (c == '\\') c = '/';
    if (p.empty() || exists(p)) return p;
    // walk the components case-insensitively
    std::string cur = p[0] == '/' ? "/" : "";
    size_t i = p[0] == '/' ? 1 : 0;
    while (i <= p.size()) {
        size_t j = p.find('/', i);
        if (j == std::string::npos) j = p.size();
        std::string comp = p.substr(i, j - i);
        i = j + 1;
        if (comp.empty() || comp == ".") continue;
        std::string base = cur.empty() ? "." : cur;
        std::string next = cur + comp;
        if (!exists(next)) {
            DIR* d = opendir(base.c_str());
            bool found = false;
            if (d) {
                while (dirent* e = readdir(d)) {
                    if (strcasecmp(e->d_name, comp.c_str()) == 0) {
                        next = cur + e->d_name;
                        found = true;
                        break;
                    }
                }
                closedir(d);
            }
            if (!found) return p;
        }
        cur = next;
        if (j < p.size()) cur += "/";
    }
    return cur;
}

bool read_file(const char* path, std::vector<uint8_t>& out)
{
    std::string p = resolve_path(path);
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return false; }
    out.resize((size_t)n);
    size_t got = n ? fread(out.data(), 1, (size_t)n, f) : 0;
    fclose(f);
    return got == (size_t)n;
}

// ---- math ----
void mat_mul(D3DMATRIX& out, const D3DMATRIX& a, const D3DMATRIX& b)
{
    D3DMATRIX r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
    out = r;
}

bool mat_inverse(D3DMATRIX& out, float* det_out, const D3DMATRIX& mm)
{
    const float* m = &mm._11;
    float inv[16];
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (det_out) *det_out = det;
    if (det == 0.0f) return false;
    float id = 1.0f / det;
    for (int i = 0; i < 16; i++) (&out._11)[i] = inv[i] * id;
    return true;
}

} // namespace mbg

using namespace mbg;

extern "C" {

D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* a, const D3DXMATRIX* b)
{
    mat_mul(*pOut, *a, *b);
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM)
{
    D3DMATRIX r;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) r.m[i][j] = pM->m[j][i];
    *(D3DMATRIX*)pOut = r;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX* pOut, FLOAT* pDet, const D3DXMATRIX* pM)
{
    D3DMATRIX r;
    if (!mat_inverse(r, pDet, *pM)) return nullptr;
    *(D3DMATRIX*)pOut = r;
    return pOut;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterRH(D3DXMATRIX* o, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    memset(o, 0, sizeof(D3DMATRIX));
    o->_11 = 2.0f * zn / (r - l);
    o->_22 = 2.0f * zn / (t - b);
    o->_31 = (l + r) / (r - l);
    o->_32 = (t + b) / (t - b);
    o->_33 = zf / (zn - zf);
    o->_34 = -1.0f;
    o->_43 = zn * zf / (zn - zf);
    return o;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterRH(D3DXMATRIX* o, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    memset(o, 0, sizeof(D3DMATRIX));
    o->_11 = 2.0f / (r - l);
    o->_22 = 2.0f / (t - b);
    o->_33 = 1.0f / (zn - zf);
    o->_41 = (l + r) / (l - r);
    o->_42 = (t + b) / (b - t);
    o->_43 = zn / (zn - zf);
    o->_44 = 1.0f;
    return o;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH(D3DXMATRIX* o, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    memset(o, 0, sizeof(D3DMATRIX));
    o->_11 = 2.0f / (r - l);
    o->_22 = 2.0f / (t - b);
    o->_33 = 1.0f / (zf - zn);
    o->_41 = (l + r) / (l - r);
    o->_42 = (t + b) / (b - t);
    o->_43 = zn / (zn - zf);
    o->_44 = 1.0f;
    return o;
}

D3DXPLANE* WINAPI D3DXPlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* p, const D3DXMATRIX* m)
{
    D3DXPLANE r;
    r.a = p->a * m->_11 + p->b * m->_21 + p->c * m->_31 + p->d * m->_41;
    r.b = p->a * m->_12 + p->b * m->_22 + p->c * m->_32 + p->d * m->_42;
    r.c = p->a * m->_13 + p->b * m->_23 + p->c * m->_33 + p->d * m->_43;
    r.d = p->a * m->_14 + p->b * m->_24 + p->c * m->_34 + p->d * m->_44;
    *pOut = r;
    return pOut;
}

} // extern "C"

namespace mbg {

// ---- ID3DXBuffer ----
struct XBuffer final : public Com<ID3DXBuffer> {
    std::vector<uint8_t> data;
    LPVOID STDMETHODCALLTYPE GetBufferPointer() override { return data.data(); }
    DWORD STDMETHODCALLTYPE GetBufferSize() override { return (DWORD)data.size(); }
};
ID3DXBuffer* make_buffer(const void* p, size_t n)
{
    XBuffer* b = new XBuffer();
    b->data.assign((const uint8_t*)p, (const uint8_t*)p + n);
    if (b->data.empty()) b->data.push_back(0);
    return b;
}

// ---- image decoding ----
struct Image {
    D3DFORMAT fmt = D3DFMT_UNKNOWN;
    UINT w = 0, h = 0;
    std::vector<std::vector<uint8_t>> levels;   // D3D layout
    D3DXIMAGE_FILEFORMAT file = D3DXIFF_DDS;
};

static uint32_t rd32(const uint8_t* p) { uint32_t v; memcpy(&v, p, 4); return v; }

static D3DFORMAT dds_format(const uint8_t* pf, int& bpp)
{
    uint32_t flags = rd32(pf + 4), fourcc = rd32(pf + 8), bits = rd32(pf + 12);
    uint32_t r = rd32(pf + 16), g = rd32(pf + 20), b = rd32(pf + 24), a = rd32(pf + 28);
    bpp = (int)bits;
    if (flags & 4) {
        switch (fourcc) {
        case MAKEFOURCC('D', 'X', 'T', '1'): return D3DFMT_DXT1;
        case MAKEFOURCC('D', 'X', 'T', '2'): return D3DFMT_DXT2;
        case MAKEFOURCC('D', 'X', 'T', '3'): return D3DFMT_DXT3;
        case MAKEFOURCC('D', 'X', 'T', '4'): return D3DFMT_DXT4;
        case MAKEFOURCC('D', 'X', 'T', '5'): return D3DFMT_DXT5;
        default: if (fourcc < 256) return (D3DFORMAT)fourcc; return D3DFMT_UNKNOWN;
        }
    }
    bool has_a = (flags & 1) != 0;
    if (bits == 32) {
        if (r == 0xff0000 && g == 0xff00 && b == 0xff) return has_a && a ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
        if (r == 0xff && g == 0xff00 && b == 0xff0000) return has_a && a ? D3DFMT_A8B8G8R8 : D3DFMT_X8B8G8R8;
        if (r == 0x3ff00000 && g == 0xffc00 && b == 0x3ff) return D3DFMT_A2R10G10B10;
        if (r == 0x3ff && g == 0xffc00 && b == 0x3ff00000) return D3DFMT_A2B10G10R10;
        if (r == 0xffff && g == 0xffff0000) return D3DFMT_G16R16;
    } else if (bits == 24) {
        if (r == 0xff0000) return D3DFMT_R8G8B8;
    } else if (bits == 16) {
        if (r == 0xf800 && g == 0x7e0 && b == 0x1f) return D3DFMT_R5G6B5;
        if (r == 0x7c00 && g == 0x3e0 && b == 0x1f) return has_a && a ? D3DFMT_A1R5G5B5 : D3DFMT_X1R5G5B5;
        if (r == 0xf00 && g == 0xf0 && b == 0xf) return has_a && a ? D3DFMT_A4R4G4B4 : D3DFMT_X4R4G4B4;
        if (r == 0xff && a == 0xff00) return D3DFMT_A8L8;
        if (r == 0xffff) return D3DFMT_L16;
    } else if (bits == 8) {
        if (a == 0xff && !r) return D3DFMT_A8;
        if (r == 0xff) return D3DFMT_L8;
        if (r == 0xe0 && g == 0x1c && b == 0x3) return D3DFMT_R3G3B2;
    }
    return D3DFMT_UNKNOWN;
}

static bool load_dds(const std::vector<uint8_t>& f, UINT skip, Image& img, UINT* file_levels)
{
    if (f.size() < 128 || rd32(f.data()) != MAKEFOURCC('D', 'D', 'S', ' ')) return false;
    const uint8_t* h = f.data() + 4;
    UINT height = rd32(h + 8), width = rd32(h + 12);
    UINT mips = (rd32(h + 4) & 0x20000) ? std::max(1u, rd32(h + 24)) : 1;
    uint32_t caps2 = rd32(h + 108);
    if (caps2 & 0x200) return false;                   // cube maps: src/ loads none
    int bpp = 0;
    D3DFORMAT fmt = dds_format(h + 72, bpp);
    if (fmt == D3DFMT_UNKNOWN || !format_info(fmt)) return false;
    size_t off = 128;
    if (rd32(h + 80) == MAKEFOURCC('D', 'X', '1', '0')) off += 20;
    *file_levels = mips;
    skip = std::min(skip, mips - 1);
    img.fmt = fmt;
    img.file = D3DXIFF_DDS;
    UINT w = width, hh = height;
    for (UINT l = 0; l < mips; l++) {
        size_t sz = format_level_size(fmt, w, hh);
        if (off + sz > f.size()) {
            if (l == 0) return false;
            break;
        }
        if (l >= skip) {
            if (img.levels.empty()) { img.w = w; img.h = hh; }
            img.levels.emplace_back(f.begin() + (long)off, f.begin() + (long)(off + sz));
        }
        off += sz;
        w = std::max(1u, w / 2);
        hh = std::max(1u, hh / 2);
    }
    return !img.levels.empty();
}

static bool load_tga(const std::vector<uint8_t>& f, Image& img)
{
    if (f.size() < 18) return false;
    uint8_t idlen = f[0], cmtype = f[1], type = f[2];
    UINT w = f[12] | (f[13] << 8), h = f[14] | (f[15] << 8);
    uint8_t bits = f[16], desc = f[17];
    if (cmtype != 0 || (type != 2 && type != 3 && type != 10 && type != 11) || !w || !h) return false;
    if (bits != 8 && bits != 24 && bits != 32 && bits != 16) return false;
    size_t p = 18 + idlen;
    UINT bp = bits / 8;
    std::vector<uint8_t> raw((size_t)w * h * bp);
    if (type == 2 || type == 3) {
        if (p + raw.size() > f.size()) return false;
        memcpy(raw.data(), f.data() + p, raw.size());
    } else {
        size_t o = 0;
        while (o < raw.size() && p < f.size()) {
            uint8_t c = f[p++];
            UINT n = (c & 0x7f) + 1;
            if (c & 0x80) {
                if (p + bp > f.size()) return false;
                for (UINT i = 0; i < n && o < raw.size(); i++, o += bp) memcpy(&raw[o], &f[p], bp);
                p += bp;
            } else {
                size_t sz = (size_t)n * bp;
                if (p + sz > f.size()) return false;
                memcpy(&raw[o], &f[p], std::min(sz, raw.size() - o));
                o += sz;
                p += sz;
            }
        }
    }
    std::vector<uint8_t> out((size_t)w * h * 4);
    bool top = (desc & 0x20) != 0;
    for (UINT y = 0; y < h; y++) {
        UINT sy = top ? y : h - 1 - y;
        for (UINT x = 0; x < w; x++) {
            const uint8_t* s = &raw[((size_t)sy * w + x) * bp];
            uint8_t* d = &out[((size_t)y * w + x) * 4];
            if (bp == 1) { d[0] = d[1] = d[2] = s[0]; d[3] = 255; }
            else if (bp == 2) {
                uint16_t c = (uint16_t)(s[0] | (s[1] << 8));
                d[2] = (uint8_t)(((c >> 10) & 31) * 255 / 31); d[1] = (uint8_t)(((c >> 5) & 31) * 255 / 31); d[0] = (uint8_t)((c & 31) * 255 / 31);
                d[3] = (c & 0x8000) ? 255 : 0;
            } else { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = bp == 4 ? s[3] : 255; }
        }
    }
    img.fmt = bp == 4 || bp == 2 ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
    img.w = w;
    img.h = h;
    img.file = D3DXIFF_TGA;
    img.levels.push_back(std::move(out));
    return true;
}

static bool load_sdl(const std::vector<uint8_t>& f, Image& img)
{
    SDL_IOStream* io = SDL_IOFromConstMem(f.data(), f.size());
    if (!io) return false;
    SDL_Surface* s = nullptr;
    bool png = f.size() > 8 && f[0] == 0x89 && f[1] == 'P';
    if (png) s = SDL_LoadPNG_IO(io, true);
    else s = SDL_LoadBMP_IO(io, true);
    if (!s) return false;
    SDL_Surface* c = SDL_ConvertSurface(s, SDL_PIXELFORMAT_ARGB8888);
    SDL_DestroySurface(s);
    if (!c) return false;
    img.w = (UINT)c->w;
    img.h = (UINT)c->h;
    img.fmt = D3DFMT_A8R8G8B8;
    img.file = png ? D3DXIFF_PNG : D3DXIFF_BMP;
    std::vector<uint8_t> out((size_t)img.w * img.h * 4);
    for (UINT y = 0; y < img.h; y++) memcpy(&out[(size_t)y * img.w * 4], (uint8_t*)c->pixels + (size_t)y * c->pitch, (size_t)img.w * 4);
    SDL_DestroySurface(c);
    img.levels.push_back(std::move(out));
    return true;
}

// box-filter the next mip level of a BGRA8 image
static std::vector<uint8_t> half_bgra(const std::vector<uint8_t>& src, UINT w, UINT h, UINT& nw, UINT& nh)
{
    nw = std::max(1u, w / 2);
    nh = std::max(1u, h / 2);
    std::vector<uint8_t> o((size_t)nw * nh * 4);
    for (UINT y = 0; y < nh; y++)
        for (UINT x = 0; x < nw; x++)
            for (int c = 0; c < 4; c++) {
                UINT x0 = std::min(2 * x, w - 1), x1 = std::min(2 * x + 1, w - 1), y0 = std::min(2 * y, h - 1), y1 = std::min(2 * y + 1, h - 1);
                int s = src[((size_t)y0 * w + x0) * 4 + c] + src[((size_t)y0 * w + x1) * 4 + c] + src[((size_t)y1 * w + x0) * 4 + c] + src[((size_t)y1 * w + x1) * 4 + c];
                o[((size_t)y * nw + x) * 4 + c] = (uint8_t)((s + 2) / 4);
            }
    return o;
}

} // namespace mbg

extern "C" {

HRESULT WINAPI D3DXCreateTexture(LPDIRECT3DDEVICE9 dev, UINT w, UINT h, UINT mips, DWORD usage, D3DFORMAT fmt, D3DPOOL pool, LPDIRECT3DTEXTURE9* out)
{
    if (!dev || !out) return D3DERR_INVALIDCALL;
    if (w == 0 || w == D3DX_DEFAULT) w = 256;
    if (h == 0 || h == D3DX_DEFAULT) h = 256;
    if (mips == D3DX_DEFAULT) mips = 0;
    if (fmt == D3DFMT_UNKNOWN) fmt = D3DFMT_A8R8G8B8;
    HRESULT hr = dev->CreateTexture(w, h, mips, usage, fmt, pool, out, nullptr);
    if (FAILED(hr) && !(usage & (D3DUSAGE_DEPTHSTENCIL | D3DUSAGE_RENDERTARGET)) && fmt != D3DFMT_A8R8G8B8)
        hr = dev->CreateTexture(w, h, mips, usage, D3DFMT_A8R8G8B8, pool, out, nullptr);   // D3DX picks the closest format
    return hr;
}

HRESULT WINAPI D3DXCreateTextureFromFileExA(LPDIRECT3DDEVICE9 dev, LPCSTR file, UINT w, UINT h, UINT mips, DWORD usage, D3DFORMAT fmt,
                                            D3DPOOL pool, DWORD filter, DWORD mip_filter, D3DCOLOR key, D3DXIMAGE_INFO* info,
                                            PALETTEENTRY*, LPDIRECT3DTEXTURE9* out)
{
    (void)filter;
    if (!dev || !file || !out) return D3DERR_INVALIDCALL;
    *out = nullptr;
    std::vector<uint8_t> data;
    if (!read_file(file, data)) return D3DERR_NOTAVAILABLE;
    UINT skip = (mip_filter == D3DX_DEFAULT) ? 0 : (mip_filter >> D3DX_SKIP_DDS_MIP_LEVELS_SHIFT) & D3DX_SKIP_DDS_MIP_LEVELS_MASK;
    Image img;
    UINT file_levels = 1;
    bool ok = load_dds(data, skip, img, &file_levels);
    if (!ok) { ok = load_tga(data, img); file_levels = 1; }
    if (!ok) { ok = load_sdl(data, img); file_levels = 1; }
    if (!ok) {
        log("D3DXCreateTextureFromFileExA: cannot decode %s", file);
        return D3DERR_INVALIDCALL;
    }
    if (info) {
        info->Width = img.w << std::min(skip, file_levels - 1);
        info->Height = img.h << std::min(skip, file_levels - 1);
        info->Depth = 1;
        info->MipLevels = file_levels;
        info->Format = img.fmt;
        info->ResourceType = D3DRTYPE_TEXTURE;
        info->ImageFileFormat = img.file;
    }
    // what the caller asked for
    D3DFORMAT dfmt = (fmt == D3DFMT_UNKNOWN || fmt == D3DFMT_FROM_FILE) ? img.fmt : fmt;
    UINT dw = (w == D3DX_DEFAULT || w == D3DX_DEFAULT_NONPOW2 || w == D3DX_FROM_FILE || w == 0) ? img.w : w;
    UINT dh = (h == D3DX_DEFAULT || h == D3DX_DEFAULT_NONPOW2 || h == D3DX_FROM_FILE || h == 0) ? img.h : h;
    bool same = dfmt == img.fmt && dw == img.w && dh == img.h && key == 0;
    UINT full = 1;
    for (UINT s = std::max(dw, dh); s > 1; s >>= 1) full++;
    UINT levels;
    if (mips == D3DX_FROM_FILE) levels = (UINT)img.levels.size();
    else if (mips == 0 || mips == D3DX_DEFAULT) levels = full;
    else levels = std::min(mips, full);
    if (!same) {
        // resample / convert through BGRA8 (format conversion, colour key, resize by nearest)
        std::vector<uint8_t> bgra((size_t)img.w * img.h * 4);
        decode_to_bgra(img.fmt, img.levels[0].data(), format_pitch(img.fmt, img.w), img.w, img.h, bgra.data(), img.w * 4);
        if (key) {
            for (size_t i = 0; i < bgra.size(); i += 4) {
                uint32_t c; memcpy(&c, &bgra[i], 4);
                if ((c | 0xff000000) == (key | 0xff000000)) { uint32_t z = 0; memcpy(&bgra[i], &z, 4); }
            }
        }
        if (dw != img.w || dh != img.h) {
            std::vector<uint8_t> rs((size_t)dw * dh * 4);
            for (UINT y = 0; y < dh; y++)
                for (UINT x = 0; x < dw; x++)
                    memcpy(&rs[((size_t)y * dw + x) * 4], &bgra[(((size_t)y * img.h / dh) * img.w + (size_t)x * img.w / dw) * 4], 4);
            bgra.swap(rs);
        }
        if (format_is_block(dfmt) || !format_info(dfmt)) dfmt = D3DFMT_A8R8G8B8;
        img.levels.clear();
        img.w = dw;
        img.h = dh;
        img.fmt = D3DFMT_A8R8G8B8;
        img.levels.push_back(std::move(bgra));
        if (dfmt != D3DFMT_A8R8G8B8 && dfmt != D3DFMT_X8R8G8B8) dfmt = D3DFMT_A8R8G8B8;
        img.fmt = dfmt;
    }
    // fill the missing mip levels (uncompressed: box filter; DXT: keep what the file has)
    while (img.levels.size() < levels) {
        if (format_is_block(img.fmt) || (img.fmt != D3DFMT_A8R8G8B8 && img.fmt != D3DFMT_X8R8G8B8)) { levels = (UINT)img.levels.size(); break; }
        UINT l = (UINT)img.levels.size() - 1;
        UINT lw = std::max(1u, img.w >> l), lh = std::max(1u, img.h >> l), nw, nh;
        img.levels.push_back(half_bgra(img.levels.back(), lw, lh, nw, nh));
    }
    levels = std::min<UINT>(levels, (UINT)img.levels.size());
    IDirect3DTexture9* t = nullptr;
    HRESULT hr = dev->CreateTexture(img.w, img.h, levels, usage, img.fmt, pool, &t, nullptr);
    if (FAILED(hr)) return hr;
    for (UINT l = 0; l < levels; l++) {
        D3DLOCKED_RECT lr;
        if (FAILED(t->LockRect(l, &lr, nullptr, 0))) continue;
        UINT lw = std::max(1u, img.w >> l), lh = std::max(1u, img.h >> l);
        UINT pitch = format_pitch(img.fmt, lw), rows = format_rows(img.fmt, lh);
        for (UINT r = 0; r < rows; r++) memcpy((uint8_t*)lr.pBits + (size_t)r * lr.Pitch, img.levels[l].data() + (size_t)r * pitch, pitch);
        t->UnlockRect(l);
    }
    *out = t;
    return D3D_OK;
}

HRESULT WINAPI D3DXSaveSurfaceToFileA(LPCSTR file, D3DXIMAGE_FILEFORMAT fmt, LPDIRECT3DSURFACE9 surf, CONST PALETTEENTRY*, CONST RECT* rect)
{
    Surface* s = static_cast<Surface*>(surf);
    if (!file || !s) return D3DERR_INVALIDCALL;
    D3DLOCKED_RECT lr;
    if (FAILED(s->LockRect(&lr, nullptr, D3DLOCK_READONLY))) return D3DERR_INVALIDCALL;
    UINT W = s->width(), H = s->height();
    RECT r = rect ? *rect : RECT{0, 0, (LONG)W, (LONG)H};
    r.left = std::max<LONG>(0, r.left); r.top = std::max<LONG>(0, r.top);
    r.right = std::min<LONG>((LONG)W, r.right); r.bottom = std::min<LONG>((LONG)H, r.bottom);
    std::vector<uint8_t> full((size_t)W * H * 4);
    bool ok = decode_to_bgra(s->img->format, (const uint8_t*)lr.pBits, (UINT)lr.Pitch, W, H, full.data(), W * 4);
    s->UnlockRect();
    if (!ok || r.right <= r.left || r.bottom <= r.top) return D3DERR_INVALIDCALL;
    int cw = r.right - r.left, ch = r.bottom - r.top;
    std::string path = file;
    for (char& c : path) if (c == '\\') c = '/';
    if (fmt == D3DXIFF_DDS) {
        FILE* f = fopen(path.c_str(), "wb");
        if (!f) return D3DERR_INVALIDCALL;
        uint32_t hdr[32] = {};
        hdr[0] = MAKEFOURCC('D', 'D', 'S', ' ');
        hdr[1] = 124; hdr[2] = 0x1 | 0x2 | 0x4 | 0x1000 | 0x8; hdr[3] = (uint32_t)ch; hdr[4] = (uint32_t)cw; hdr[5] = (uint32_t)cw * 4;
        hdr[19] = 32; hdr[20] = 0x41; hdr[22] = 32; hdr[23] = 0xff0000; hdr[24] = 0xff00; hdr[25] = 0xff; hdr[26] = 0xff000000; hdr[27] = 0x1000;
        fwrite(hdr, 4, 32, f);
        for (int y = 0; y < ch; y++) fwrite(&full[((size_t)(r.top + y) * W + r.left) * 4], 4, (size_t)cw, f);
        fclose(f);
        return D3D_OK;
    }
    SDL_Surface* out = SDL_CreateSurface(cw, ch, SDL_PIXELFORMAT_ARGB8888);
    if (!out) return D3DERR_INVALIDCALL;
    for (int y = 0; y < ch; y++) {
        uint8_t* d = (uint8_t*)out->pixels + (size_t)y * out->pitch;
        memcpy(d, &full[((size_t)(r.top + y) * W + r.left) * 4], (size_t)cw * 4);
        if (s->img->format == D3DFMT_X8R8G8B8 || fmt == D3DXIFF_BMP)
            for (int x = 0; x < cw; x++) d[x * 4 + 3] = 255;
    }
    bool saved = false;
    if (fmt == D3DXIFF_BMP || fmt == D3DXIFF_DIB) {
        SDL_Surface* rgb = SDL_ConvertSurface(out, SDL_PIXELFORMAT_XRGB8888);   // a 32-bit BMP without alpha, like D3DX writes
        saved = rgb && SDL_SaveBMP(rgb, path.c_str());
        if (rgb) SDL_DestroySurface(rgb);
    } else if (fmt == D3DXIFF_PNG) {
        saved = SDL_SavePNG(out, path.c_str());
    } else if (fmt == D3DXIFF_TGA) {
        FILE* f = fopen(path.c_str(), "wb");
        if (f) {
            uint8_t h[18] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, (uint8_t)cw, (uint8_t)(cw >> 8), (uint8_t)ch, (uint8_t)(ch >> 8), 32, 0x28};
            fwrite(h, 1, 18, f);
            for (int y = 0; y < ch; y++) fwrite((uint8_t*)out->pixels + (size_t)y * out->pitch, 4, (size_t)cw, f);
            fclose(f);
            saved = true;
        }
    } else {
        log("D3DXSaveSurfaceToFileA: image format %d not written by this port (src/ saves BMP)", (int)fmt);
    }
    SDL_DestroySurface(out);
    return saved ? D3D_OK : D3DERR_INVALIDCALL;
}

} // extern "C"
