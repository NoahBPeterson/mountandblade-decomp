// Slice s004640d0: ShadowMap read-back via a system-memory D3D9 texture, the sco terrain layer reader,
// the 15-weight blend record and bit-field helpers. Names provisional.
#include "s004640d0.h"

// @ 0x4640d0
void ShadowMap::sync_readback()
{
    if (surf == 0) {
        D3DFORMAT fmt = D3DFMT_R32F;
        if (g_use_hw_shadow)
            fmt = D3DFMT_R16F;
        IDirect3DTexture9* tex = 0;
        HRESULT hr = g_device->CreateTexture(w, h, 1, D3DUSAGE_DYNAMIC, fmt, D3DPOOL_SYSTEMMEM, &tex, 0);
        if (hr < 0) {
            g_flag_5aa084 = 0;
            surf = 0;
        } else {
            tex->GetSurfaceLevel(0, &surf);
        }
        if (tex)
            tex->Release();
        if (surf == 0)
            return;
    }
    if (target != 0) {
        IDirect3DSurface9* src;
        (*target->refs[0]->tex)->GetSurfaceLevel(0, &src);
        g_device->GetRenderTargetData(src, surf);
        src->Release();
    }
}

// @ 0x4641a0
void ShadowMap::read_pixel(int unused, int x, int y, unsigned* a, unsigned* b)
{
    D3DLOCKED_RECT lr;
    RECT r;
    r.left = x;
    r.right = x + 1;
    r.top = y;
    r.bottom = y + 1;
    surf->LockRect(&lr, &r, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK);
    *a = *(unsigned*)lr.pBits;
    *b = *(unsigned*)lr.pBits;
    surf->UnlockRect();
}

// @ 0x464200
void TerrainDescEntry::read(stream_base* s, Dims* d)
{
    s->r_int(&type);
    s->r_tstr(&name);
    int idx = -1;
    int i = 0;
    int p = (int)g_ground_types;
    do {
        if (((GroundType*)p)->name.equals(name))
            idx = i;
        p += 0x20;
        i++;
    } while (p < (int)(g_ground_types + 11));
    if (idx >= 0)
        type = idx;
    int present;
    s->r_int(&present);
    if (present == 0) {
        if (data != 0)
            delete[] data;
        data = 0;
        return;
    }
    if (data == 0)
        data = new float[0x2b110];
    bool value_run = false;
    int run_a = 0, run_b = 0;
    int y = 0;
    s->r_int(&run_a);
    if (run_a == 0) {
        value_run = true;
        s->r_int(&run_b);
    }
    bool need = false;
    for (y = 0; y < d->h; y++) {
        for (int x = 0; x < d->w; x++) {
            int idx2 = x * 0x1a4 + y;
            if (value_run) {
                if (need) {
                    s->r_int(&run_b);
                    need = false;
                }
                if (type >= 0) {
                    unsigned char c;
                    s->r_byte((char*)&c);
                    data[idx2] = (float)c * 0.003921569f;
                } else {
                    s->r_float(&data[idx2]);
                }
                run_b--;
                if (run_b <= 0) {
                    value_run = false;
                    need = true;
                }
            } else {
                if (need) {
                    s->r_int(&run_a);
                    need = false;
                }
                if (type == -0x3115)
                    ((int*)data)[idx2] = -1;
                else
                    ((int*)data)[idx2] = 0;
                run_a--;
                if (run_a <= 0) {
                    value_run = true;
                    need = true;
                }
            }
        }
    }
}

// @ 0x464400
void TerrainDesc::init()
{
    for (int i = 0; i < 13; i++) {
        if (layers[i].data)
            delete[] layers[i].data;
        layers[i].data = 0;
        layers[i].type = -1;
    }
    n = 0;
}

// @ 0x464440
void TerrainDesc::read(stream_base* s)
{
    s->r_int(&n);
    if (n > 13)
        n = 13;
    n_minus2 = n - 2;
    n_minus1 = n - 1;
    s->r_int(&dims.w);
    s->r_int(&dims.h);
    if (dims.w > 0x1a4 || dims.h > 0x1a4)
        mb_error("Illegal terrain size in sco file");
    for (int i = 0; i < n; i++)
        layers[i].read(s, &dims);
}

// @ 0x4644e0
void Blend::init(const float* q)
{
    p = *(const rgl::vector4*)q;
    f1c = 0.0f;
    f18 = 1.0f;
    f14 = -1;
    f10 = -1;
    v30 = rgl::vector4(q[0] * 0.7f, q[1] * 0.7f, 0.0f);
    f44 = -1;
    v30.w = 1.0f;
    for (int i = 1; i < 15; i++)
        w[i] = 0.0f;
    f88 = 0.0f;
    w[0] = 1.0f;
    f90 = 1.0f;
    f84 = p.z;
    f8c = -1;
}

// @ 0x4645b0
bool Blend::visible(int i)
{
    bool ok = true;
    if (w[i] < 0.01f)
        return false;
    float prod = 1.0f;
    for (int j = i + 1; j < 15; j++) {
        prod = (1.0f - w[j]) * prod;
        if (prod < 0.01f)
            ok = false;
    }
    return ok;
}

// @ 0x4646b0
int Blend::dominant()
{
    int r = 0;
    for (int j = 1; j < 15; j++) {
        if (w[j] > 0.4f)
            r = j;
    }
    return r;
}

// @ 0x464760
void Seg::init()
{
    f0 = 0;
    f4 = 0;
    f8 = 0;
    f34 = 0;
    a = rgl::vector4(0.0f, 0.0f, 1.0f);
    b = rgl::vector4(0.0f, 0.0f, 1.0f);
    f2c = -1;
    f30 = -1;
}

// @ 0x464960
void BitWords::set(const unsigned& lo, const int& hi, const unsigned& v)
{
    unsigned a = lo;
    int b = hi;
    unsigned mask = (unsigned)g_pow2[b - a] - 1;
    unsigned vv = v;
    if (vv > mask)
        vv = mask;
    unsigned sh = a & 0x1f;
    unsigned idx = a >> 5;
    unsigned old = w[idx];
    w[idx] = (~(mask << sh) & old) | (vv << sh);
}

// @ 0x4649b0
unsigned BitWords::get(const unsigned& lo, const int& hi)
{
    unsigned mask = (unsigned)g_pow2[hi - lo] - 1;
    unsigned sh = lo & 0x1f;
    return ((w[lo >> 5] & (mask << sh)) >> sh) & mask;
}

// @ 0x4649f0
void Pct::set_pct(const float& f)
{
    float x = f * 100.0f;
    float fl = (float)floor(x);
    int i = (int)fl;
    if (!(x - fl <= 0.5f))
        i++;
    if (i < 0)
        i = 0;
    else if (i >= 100)
        i = 99;
    unsigned mask = g_mask7 - 1;
    if ((unsigned)i > mask)
        i = mask;
    bits = (~mask & bits) | i;
}

// ---------------------------------------------------------------------------------------------------------
// Terrain::reset: the original brackets the scalar-reset part with two inline-asm data markers
// (jmp +0x10; "WL  ", 12 resp. 13, 0, "WL  "). Fields are reached by offset.
// ---------------------------------------------------------------------------------------------------------
#define BW(o) (*(int*)((char*)this + (o)))

// @ 0x4647e0
void Terrain::reset()
{
    int* a = (int*)((char*)this + 0x2c);
    int* b = (int*)((char*)this + 0x845da54);
    for (int i = 0; i < 15; i++) {
        a[-1] = 0;
        a[0] = 0;
        a[1] = 0;
        a[2] = 0;
        a[3] = 0;
        a[4] = 0x3f800000;
        for (int k = 0; k < 25; k++)
            b[k] = 0;
        a += 10;
        b += 25;
    }
    for (int o = 0x845d9f0; o <= 0x845da50; o += 4)
        BW(o) = 0;
    BW(0x5c3b8f4) = 0;
    BW(0x843b8f8) = 0;
    BW(0xc3b0e4) = 0;
    BW(0x343b0e8) = 0;
    BW(0x343b4ec) = 0;
    BW(0x5c3b4f0) = 0;
    BW(0x845d97c) = 0;
    BW(0x845d980) = 0;
}
