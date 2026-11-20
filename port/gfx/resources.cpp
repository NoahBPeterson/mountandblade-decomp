// port/gfx/resources.cpp: textures, surfaces, vertex / index buffers, vertex declarations, queries.
#include "gfx_internal.h"

namespace mbg {

// ---- GpuImage ----
GpuImage::~GpuImage()
{
    SDL_GPUDevice* g = shared_gpu_device();
    if (g) {
        if (tex) SDL_ReleaseGPUTexture(g, tex);
        if (msaa) SDL_ReleaseGPUTexture(g, msaa);
    }
}

std::vector<uint8_t>& GpuImage::level_shadow(UINT l, UINT face)
{
    UINT i = face * levels + l;
    if (shadow.size() < (size_t)levels * faces) {
        shadow.resize((size_t)levels * faces);
        shadow_valid.resize((size_t)levels * faces, 0);
    }
    std::vector<uint8_t>& s = shadow[i];
    if (s.empty()) s.assign(format_level_size(format, level_w(l), level_h(l)), 0);
    return s;
}

std::shared_ptr<GpuImage> Device::make_image(UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT fmt, D3DPOOL pool, UINT samples, UINT faces)
{
    const FormatInfo* fi = format_info(fmt);
    if (!fi || w == 0 || h == 0) return nullptr;
    auto img = std::make_shared<GpuImage>();
    img->dev = this;
    img->format = fmt;
    img->width = w;
    img->height = h;
    img->usage = usage;
    img->pool = pool;
    img->faces = faces;
    img->is_depth = fi->depth;
    UINT full = 1;
    for (UINT s = std::max(w, h); s > 1; s >>= 1) full++;
    if (levels == 0) levels = full;
    levels = std::min(levels, full);
    if (usage & D3DUSAGE_AUTOGENMIPMAP) { img->autogen = true; levels = full; }
    img->levels = levels;
    img->samples = samples;
    if (pool == D3DPOOL_SYSTEMMEM || pool == D3DPOOL_SCRATCH) return img;   // CPU only

    SDL_GPUTextureFormat gf = fi->gpu;
    SDL_GPUTextureUsageFlags gu = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    if (fi->depth) gu = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    if ((usage & D3DUSAGE_RENDERTARGET) || img->autogen) gu |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    if (fi->depth) {
        if (!SDL_GPUTextureSupportsFormat(gpu, gf, SDL_GPU_TEXTURETYPE_2D, gu)) {
            static const SDL_GPUTextureFormat alts[] = {SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT, SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
                                                       SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREFORMAT_D16_UNORM};
            gf = SDL_GPU_TEXTUREFORMAT_INVALID;
            for (SDL_GPUTextureFormat a : alts)
                if (SDL_GPUTextureSupportsFormat(gpu, a, SDL_GPU_TEXTURETYPE_2D, gu) && (fi->stencil ? gpu_has_stencil(a) : true)) { gf = a; break; }
            if (gf == SDL_GPU_TEXTUREFORMAT_INVALID) return nullptr;
        }
    } else if (!SDL_GPUTextureSupportsFormat(gpu, gf, SDL_GPU_TEXTURETYPE_2D, gu)) {
        if (gu & SDL_GPU_TEXTUREUSAGE_COLOR_TARGET) return nullptr;
        gf = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;      // decode on the CPU (DXTn on GPUs without BC, A8 ...)
        img->cpu_convert = true;
    }
    img->gpu_format = gf;
    SDL_GPUTextureCreateInfo ci{};
    ci.type = SDL_GPU_TEXTURETYPE_2D;
    ci.format = gf;
    ci.usage = gu;
    ci.width = w;
    ci.height = h;
    ci.layer_count_or_depth = 1;
    ci.num_levels = levels;
    ci.sample_count = SDL_GPU_SAMPLECOUNT_1;
    img->tex = SDL_CreateGPUTexture(gpu, &ci);
    if (!img->tex) {
        log("SDL_CreateGPUTexture %ux%u fmt %d: %s", w, h, (int)fmt, SDL_GetError());
        return nullptr;
    }
    if (samples > 1) {
        ci.num_levels = 1;
        ci.usage = fi->depth ? SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET : SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        ci.sample_count = samples >= 8 ? SDL_GPU_SAMPLECOUNT_8 : samples >= 4 ? SDL_GPU_SAMPLECOUNT_4 : SDL_GPU_SAMPLECOUNT_2;
        img->msaa = SDL_CreateGPUTexture(gpu, &ci);
        if (!img->msaa) img->samples = 1;
    }
    return img;
}

// ---- Surface ----
ULONG Surface::AddRef()
{
    if (container) return container->AddRef();
    return ++refs;
}
ULONG Surface::Release()
{
    if (container) return container->Release();
    ULONG r = --refs;
    if (r == 0) delete this;
    return r;
}
HRESULT Surface::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = dev; dev->AddRef(); return D3D_OK; }
HRESULT Surface::GetContainer(REFIID, void** pp)
{
    if (!pp) return D3DERR_INVALIDCALL;
    if (container) { *pp = static_cast<IDirect3DTexture9*>(container); container->AddRef(); return D3D_OK; }
    *pp = static_cast<IDirect3DDevice9*>(dev);
    dev->AddRef();
    return D3D_OK;
}
HRESULT Surface::GetDesc(D3DSURFACE_DESC* d)
{
    if (!d) return D3DERR_INVALIDCALL;
    d->Format = img->format;
    d->Type = D3DRTYPE_SURFACE;
    d->Usage = img->usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL | D3DUSAGE_DYNAMIC);
    d->Pool = img->pool;
    d->MultiSampleType = img->samples > 1 ? (D3DMULTISAMPLE_TYPE)img->samples : D3DMULTISAMPLE_NONE;
    d->MultiSampleQuality = 0;
    d->Width = width();
    d->Height = height();
    return D3D_OK;
}

HRESULT Surface::LockRect(D3DLOCKED_RECT* lr, CONST RECT* r, DWORD flags)
{
    if (!lr || locked) return D3DERR_INVALIDCALL;
    // a render target's contents live on the GPU: read them back first
    if (img->tex && (img->gpu_newer || !(img->shadow.size() > face * img->levels + level && img->shadow_valid[face * img->levels + level]))
        && (img->usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) && !(flags & D3DLOCK_DISCARD)) {
        std::vector<uint8_t> raw;
        std::vector<uint8_t>& sh = img->level_shadow(level, face);
        if (!img->is_depth && dev->readback(img.get(), level, raw))
            convert_from_gpu(img->format, img->gpu_format, raw.data(), width(), height(), sh.data(), format_pitch(img->format, width()));
        img->shadow_valid[face * img->levels + level] = 1;
    }
    std::vector<uint8_t>& sh = img->level_shadow(level, face);
    UINT pitch = format_pitch(img->format, width());
    RECT full{0, 0, (LONG)width(), (LONG)height()};
    lock_rect = r ? *r : full;
    size_t off;
    if (format_is_block(img->format))
        off = (size_t)(lock_rect.top / 4) * pitch + (size_t)(lock_rect.left / 4) * format_info(img->format)->block;
    else
        off = (size_t)lock_rect.top * pitch + (size_t)lock_rect.left * format_info(img->format)->bpp;
    lr->Pitch = (INT)pitch;
    lr->pBits = sh.data() + off;
    lock_readonly = (flags & D3DLOCK_READONLY) != 0;
    locked = true;
    return D3D_OK;
}

HRESULT Surface::UnlockRect()
{
    if (!locked) return D3DERR_INVALIDCALL;
    locked = false;
    img->shadow_valid.resize((size_t)img->levels * img->faces, 0);
    img->shadow_valid[face * img->levels + level] = 1;
    if (!lock_readonly && img->tex) {
        img->gpu_newer = false;
        dev->queue_texture_upload(img, level, face);
    }
    return D3D_OK;
}

// ---- Texture ----
Texture::~Texture()
{
    for (Surface* s : surfaces) {
        s->container = nullptr;
        delete s;
    }
}
HRESULT Texture::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = dev; dev->AddRef(); return D3D_OK; }
void Texture::GenerateMipSubLevels()
{
    if (img->autogen && img->tex) {
        dev->end_pass();
        dev->ensure_cmd();
        SDL_GenerateMipmapsForGPUTexture(dev->cmd, img->tex);
    }
}
HRESULT Texture::GetLevelDesc(UINT level, D3DSURFACE_DESC* d)
{
    if (level >= surfaces.size()) return D3DERR_INVALIDCALL;
    return surfaces[level]->GetDesc(d);
}
HRESULT Texture::GetSurfaceLevel(UINT level, IDirect3DSurface9** pp)
{
    if (!pp || level >= surfaces.size()) return D3DERR_INVALIDCALL;
    *pp = surfaces[level];
    AddRef();
    return D3D_OK;
}
HRESULT Texture::LockRect(UINT level, D3DLOCKED_RECT* lr, CONST RECT* r, DWORD flags)
{
    if (level >= surfaces.size()) return D3DERR_INVALIDCALL;
    return surfaces[level]->LockRect(lr, r, flags);
}
HRESULT Texture::UnlockRect(UINT level)
{
    if (level >= surfaces.size()) return D3DERR_INVALIDCALL;
    HRESULT hr = surfaces[level]->UnlockRect();
    if (level == 0 && img->autogen && img->tex) GenerateMipSubLevels();
    return hr;
}

// ---- buffers ----
bool Buffer::init(Device* d, UINT len, DWORD u, D3DPOOL p, bool is_index)
{
    dev = d;
    size = std::max(len, 4u);
    usage = u;
    pool = p;
    index = is_index;
    shadow.assign(size, 0);
    if (usage & D3DUSAGE_DYNAMIC) mirror.assign(size, 0);
    if (pool != D3DPOOL_SYSTEMMEM && pool != D3DPOOL_SCRATCH) {
        gpu = dev->create_buffer(size, index);
        if (!gpu) return false;
        if (!mirror.empty()) dev->queue_buffer_upload(gpu, 0, shadow.data(), size);   // GPU storage = mirror (zeros)
    }
    return true;
}
void Buffer::release_gpu()
{
    if (gpu && dev) dev->retire_buffer(gpu);
    gpu = nullptr;
}
HRESULT Buffer::lock(UINT off, UINT sz, void** pp, DWORD flags)
{
    if (!pp) return D3DERR_INVALIDCALL;
    if (off > size) return D3DERR_INVALIDCALL;
    if (sz == 0 || off + sz > size) sz = size - off;
    lock_off = off;
    lock_size = sz;
    lock_flags = flags;
    lock_count++;
    *pp = shadow.data() + off;
    return D3D_OK;
}
HRESULT Buffer::unlock()
{
    if (lock_count <= 0) return D3DERR_INVALIDCALL;
    lock_count--;
    if (!gpu || (lock_flags & D3DLOCK_READONLY) || lock_size == 0) return D3D_OK;
    // the range to send: the locked range, trimmed to the bytes that differ from what the GPU storage holds (dynamic
    // buffers are often locked whole with DISCARD and refilled with mostly the same data)
    uint32_t a = lock_off, b = lock_off + lock_size;
    if (!mirror.empty()) {
        while (a < b && shadow[a] == mirror[a]) a++;
        while (b > a && shadow[b - 1] == mirror[b - 1]) b--;
    }
    bool in_flight = used_batch == dev->batch && !(lock_flags & D3DLOCK_NOOVERWRITE);
    if (in_flight) {
        // a pending draw reads the current storage: give the buffer fresh storage (what a D3D9 driver does on DISCARD),
        // seeded on the GPU with the old contents so only the changed bytes travel
        SDL_GPUBuffer* nb = dev->create_buffer(size, index);
        if (!nb) return D3DERR_OUTOFVIDEOMEMORY;
        if (!mirror.empty()) dev->queue_buffer_copy(gpu, nb, size);
        else { a = 0; b = size; }
        dev->retire_buffer(gpu);
        gpu = nb;
        used_batch = 0;
    }
    if (a < b) {
        dev->queue_buffer_upload(gpu, a, shadow.data() + a, b - a);
        if (!mirror.empty()) memcpy(mirror.data() + a, shadow.data() + a, b - a);
    }
    return D3D_OK;
}
HRESULT VertexBuffer::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = b.dev; b.dev->AddRef(); return D3D_OK; }
HRESULT VertexBuffer::GetDesc(D3DVERTEXBUFFER_DESC* d)
{
    if (!d) return D3DERR_INVALIDCALL;
    d->Format = D3DFMT_VERTEXDATA; d->Type = D3DRTYPE_VERTEXBUFFER; d->Usage = b.usage; d->Pool = b.pool; d->Size = b.size; d->FVF = fvf;
    return D3D_OK;
}
HRESULT IndexBuffer::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = b.dev; b.dev->AddRef(); return D3D_OK; }
HRESULT IndexBuffer::GetDesc(D3DINDEXBUFFER_DESC* d)
{
    if (!d) return D3DERR_INVALIDCALL;
    d->Format = format; d->Type = D3DRTYPE_INDEXBUFFER; d->Usage = b.usage; d->Pool = b.pool; d->Size = b.size;
    return D3D_OK;
}

// ---- vertex declarations ----
const D3DVERTEXELEMENT9* VertexDecl::find(BYTE usage, BYTE idx) const
{
    for (const D3DVERTEXELEMENT9& e : elems)
        if (e.Usage == usage && e.UsageIndex == idx) return &e;
    return nullptr;
}
HRESULT VertexDecl::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = dev; dev->AddRef(); return D3D_OK; }
HRESULT VertexDecl::GetDeclaration(D3DVERTEXELEMENT9* p, UINT* n)
{
    if (!n) return D3DERR_INVALIDCALL;
    UINT cnt = (UINT)elems.size() + 1;
    if (p) {
        for (UINT i = 0; i < elems.size(); i++) p[i] = elems[i];
        p[elems.size()] = D3DVERTEXELEMENT9 D3DDECL_END();
    }
    *n = cnt;
    return D3D_OK;
}

UINT fvf_stride(DWORD fvf)
{
    UINT s = 0;
    switch (fvf & D3DFVF_POSITION_MASK) {
    case D3DFVF_XYZ: s += 12; break;
    case D3DFVF_XYZRHW: s += 16; break;
    case D3DFVF_XYZB1: s += 16; break;
    case 0x8: s += 20; break;   // XYZB2
    case 0xa: s += 24; break;
    case 0xc: s += 28; break;
    case 0xe: s += 32; break;
    case 0x4002: s += 16; break; // XYZW
    }
    if (fvf & D3DFVF_NORMAL) s += 12;
    if (fvf & D3DFVF_PSIZE) s += 4;
    if (fvf & D3DFVF_DIFFUSE) s += 4;
    if (fvf & D3DFVF_SPECULAR) s += 4;
    UINT ntex = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (UINT i = 0; i < ntex; i++) {
        UINT f = (fvf >> (16 + 2 * i)) & 3;     // D3DFVF_TEXCOORDSIZEn: 0 = 2, 1 = 3, 2 = 4, 3 = 1
        static const UINT sz[4] = {8, 12, 16, 4};
        s += sz[f];
    }
    return s;
}

void decl_from_fvf(DWORD fvf, std::vector<D3DVERTEXELEMENT9>& out)
{
    out.clear();
    WORD off = 0;
    auto add = [&](BYTE type, BYTE usage, BYTE idx, WORD size) {
        D3DVERTEXELEMENT9 e{0, off, type, D3DDECLMETHOD_DEFAULT, usage, idx};
        out.push_back(e);
        off = (WORD)(off + size);
    };
    DWORD pos = fvf & D3DFVF_POSITION_MASK;
    if (pos == D3DFVF_XYZRHW) add(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_POSITIONT, 0, 16);
    else if (pos == 0x4002) add(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_POSITION, 0, 16);
    else if (pos) {
        add(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0, 12);
        int nb = pos == D3DFVF_XYZB1 ? 1 : pos == 0x8 ? 2 : pos == 0xa ? 3 : pos == 0xc ? 4 : pos == 0xe ? 5 : 0;
        if (nb) {
            static const BYTE t[6] = {0, D3DDECLTYPE_FLOAT1, D3DDECLTYPE_FLOAT2, D3DDECLTYPE_FLOAT3, D3DDECLTYPE_FLOAT4, D3DDECLTYPE_FLOAT4};
            add(t[nb], D3DDECLUSAGE_BLENDWEIGHT, 0, (WORD)(4 * nb));
        }
    }
    if (fvf & D3DFVF_NORMAL) add(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0, 12);
    if (fvf & D3DFVF_PSIZE) add(D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_PSIZE, 0, 4);
    if (fvf & D3DFVF_DIFFUSE) add(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0, 4);
    if (fvf & D3DFVF_SPECULAR) add(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 1, 4);
    UINT ntex = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (UINT i = 0; i < ntex; i++) {
        UINT f = (fvf >> (16 + 2 * i)) & 3;
        static const BYTE t[4] = {D3DDECLTYPE_FLOAT2, D3DDECLTYPE_FLOAT3, D3DDECLTYPE_FLOAT4, D3DDECLTYPE_FLOAT1};
        static const WORD sz[4] = {8, 12, 16, 4};
        add(t[f], D3DDECLUSAGE_TEXCOORD, (BYTE)i, sz[f]);
    }
}

// ---- query ----
HRESULT Query::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = dev; dev->AddRef(); return D3D_OK; }
HRESULT Query::GetData(void* p, DWORD n, DWORD)
{
    if (p && n >= sizeof(BOOL) && type == D3DQUERYTYPE_EVENT) *(BOOL*)p = TRUE;
    else if (p && n >= sizeof(DWORD)) *(DWORD*)p = 0;
    return S_OK;
}

HRESULT VShader::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = dev; dev->AddRef(); return D3D_OK; }
HRESULT PShader::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = dev; dev->AddRef(); return D3D_OK; }
HRESULT PShader::GetFunction(void* p, UINT* sz)
{
    if (!sz) return D3DERR_INVALIDCALL;
    UINT n = (UINT)(tokens.size() * 4);
    if (p) memcpy(p, tokens.data(), std::min(*sz, n));
    *sz = n;
    return D3D_OK;
}

} // namespace mbg
