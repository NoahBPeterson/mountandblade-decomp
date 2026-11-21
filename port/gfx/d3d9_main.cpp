// port/gfx/d3d9_main.cpp: Direct3DCreate9 and IDirect3D9 (adapter queries, caps, device creation) over SDL3.
#include "gfx_internal.h"
#include "../platform/mb_sdl.h"
#include <cstdarg>

namespace mbg {

void log(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[mb_gfx] ");
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

bool verbose()
{
    static int v = -1;
    if (v < 0) {
        const char* e = getenv("MBG_VERBOSE");
        v = e && *e && *e != '0';
    }
    return v != 0;
}

uint64_t hash_bytes(const void* p, size_t n, uint64_t h)
{
    const uint8_t* b = (const uint8_t*)p;
    for (size_t i = 0; i < n; i++) {
        h ^= b[i];
        h *= 1099511628211ull;
    }
    return h;
}

uint64_t g_stats[STAT_COUNT];

static SDL_GPUDevice* g_gpu = nullptr;

SDL_GPUDevice* shared_gpu_device()
{
    if (!g_gpu) {
        mbp_sdl_init();   // the platform layer sets the SDL hints (macOS fullscreen) before video starts
        const char* dbg = getenv("MBG_GPU_DEBUG");
        g_gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_SPIRV, dbg && *dbg == '1', nullptr);
        if (!g_gpu) log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        else if (verbose()) log("GPU driver: %s", SDL_GetGPUDeviceDriver(g_gpu));
    }
    return g_gpu;
}

void D3D9::enum_modes()
{
    modes.clear();
    mbp_sdl_init();   // the platform layer sets the SDL hints (macOS fullscreen) before video starts
    SDL_DisplayID disp = SDL_GetPrimaryDisplay();
    int n = 0;
    SDL_DisplayMode** list = SDL_GetFullscreenDisplayModes(disp, &n);
    std::vector<SDL_DisplayMode> tmp;
    for (int i = 0; i < n; i++) tmp.push_back(*list[i]);
    SDL_free(list);
    const SDL_DisplayMode* desk = SDL_GetDesktopDisplayMode(disp);
    if (desk) tmp.push_back(*desk);
    // the classic 4:3 / 16:9 modes as windowed sizes when the display lists few (macOS lists mostly scaled modes)
    static const int extra[][2] = {{640, 480}, {800, 600}, {1024, 768}, {1280, 720}, {1280, 1024}, {1366, 768}, {1600, 900}, {1920, 1080}};
    int dw = desk ? desk->w : 1920, dh = desk ? desk->h : 1080;
    for (auto& e : extra) {
        if (e[0] > dw || e[1] > dh) continue;
        SDL_DisplayMode m{};
        m.w = e[0]; m.h = e[1]; m.refresh_rate = desk ? desk->refresh_rate : 60.0f;
        tmp.push_back(m);
    }
    for (const SDL_DisplayMode& m : tmp) {
        bool dup = false;
        for (const SDL_DisplayMode& o : modes)
            if (o.w == m.w && o.h == m.h && (int)o.refresh_rate == (int)m.refresh_rate) dup = true;
        if (!dup) modes.push_back(m);
    }
    std::sort(modes.begin(), modes.end(), [](const SDL_DisplayMode& a, const SDL_DisplayMode& b) {
        if (a.w != b.w) return a.w < b.w;
        if (a.h != b.h) return a.h < b.h;
        return a.refresh_rate < b.refresh_rate;
    });
}

HRESULT D3D9::GetAdapterIdentifier(UINT Adapter, DWORD, D3DADAPTER_IDENTIFIER9* id)
{
    if (Adapter != 0 || !id) return D3DERR_INVALIDCALL;
    memset(id, 0, sizeof(*id));
    SDL_GPUDevice* g = shared_gpu_device();
    snprintf(id->Driver, sizeof id->Driver, "mb_gfx SDL_GPU %s", g ? SDL_GetGPUDeviceDriver(g) : "none");
    snprintf(id->Description, sizeof id->Description, "SDL_GPU (%s)", g ? SDL_GetGPUDeviceDriver(g) : "none");
    snprintf(id->DeviceName, sizeof id->DeviceName, "\\\\.\\DISPLAY1");
    id->DriverVersion.QuadPart = 0x0006000E000A0000ll;
    id->VendorId = 0x10de;   // what the game's quirk tables treat as the common case
    id->DeviceId = 0x0402;
    id->WHQLLevel = 1;
    return D3D_OK;
}

static bool adapter_format_ok(D3DFORMAT f)
{
    return f == D3DFMT_X8R8G8B8 || f == D3DFMT_A8R8G8B8 || f == D3DFMT_R5G6B5 || f == D3DFMT_X1R5G5B5 || f == D3DFMT_A2R10G10B10;
}

UINT D3D9::GetAdapterModeCount(UINT Adapter, D3DFORMAT Format)
{
    if (Adapter != 0 || !adapter_format_ok(Format)) return 0;
    if (modes.empty()) enum_modes();
    return (UINT)modes.size();
}

HRESULT D3D9::EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode)
{
    if (Adapter != 0 || !pMode || !adapter_format_ok(Format)) return D3DERR_INVALIDCALL;
    if (modes.empty()) enum_modes();
    if (Mode >= modes.size()) return D3DERR_INVALIDCALL;
    pMode->Width = (UINT)modes[Mode].w;
    pMode->Height = (UINT)modes[Mode].h;
    pMode->RefreshRate = (UINT)(modes[Mode].refresh_rate + 0.5f);
    pMode->Format = Format;
    return D3D_OK;
}

HRESULT D3D9::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode)
{
    if (Adapter != 0 || !pMode) return D3DERR_INVALIDCALL;
    mbp_sdl_init();   // the platform layer sets the SDL hints (macOS fullscreen) before video starts
    const SDL_DisplayMode* m = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
    pMode->Width = m ? (UINT)m->w : 1920;
    pMode->Height = m ? (UINT)m->h : 1080;
    pMode->RefreshRate = m ? (UINT)(m->refresh_rate + 0.5f) : 60;
    pMode->Format = D3DFMT_X8R8G8B8;
    return D3D_OK;
}

HRESULT D3D9::CheckDeviceType(UINT Adapter, D3DDEVTYPE type, D3DFORMAT adapter_fmt, D3DFORMAT bb_fmt, BOOL)
{
    if (Adapter != 0 || type != D3DDEVTYPE_HAL) return D3DERR_NOTAVAILABLE;
    if (!adapter_format_ok(adapter_fmt)) return D3DERR_NOTAVAILABLE;
    if (bb_fmt != D3DFMT_UNKNOWN && bb_fmt != D3DFMT_X8R8G8B8 && bb_fmt != D3DFMT_A8R8G8B8 && bb_fmt != adapter_fmt) return D3DERR_NOTAVAILABLE;
    return D3D_OK;
}

HRESULT D3D9::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE type, D3DFORMAT, DWORD usage, D3DRESOURCETYPE rtype, D3DFORMAT f)
{
    if (Adapter != 0 || type != D3DDEVTYPE_HAL) return D3DERR_NOTAVAILABLE;
    const FormatInfo* fi = format_info(f);
    if (!fi) return D3DERR_NOTAVAILABLE;
    if (usage & D3DUSAGE_DEPTHSTENCIL) {
        // depth surfaces yes; depth TEXTURES (hardware shadow-map compare) are not offered: the effect's PCF_NVIDIA path
        // samples them with tex2Dproj compare semantics, so the game uses its R32F shadow map path instead
        if (!fi->depth || rtype == D3DRTYPE_TEXTURE) return D3DERR_NOTAVAILABLE;
        return D3D_OK;
    }
    if (fi->depth) return D3DERR_NOTAVAILABLE;
    if (usage & D3DUSAGE_RENDERTARGET) {
        if (fi->block || fi->convert) {
            if (f != D3DFMT_X8R8G8B8 && f != D3DFMT_X8B8G8R8) return D3DERR_NOTAVAILABLE;
        }
        if (f == D3DFMT_A8 || f == D3DFMT_V8U8 || f == D3DFMT_Q8W8V8U8) return D3DERR_NOTAVAILABLE;
    }
    if ((usage & D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING) && (f == D3DFMT_R32F || f == D3DFMT_G32R32F || f == D3DFMT_A32B32G32R32F))
        return D3D_OK;   // Metal / Vulkan blend float targets
    if (rtype == D3DRTYPE_VOLUMETEXTURE || rtype == D3DRTYPE_CUBETEXTURE) return D3DERR_NOTAVAILABLE;
    return D3D_OK;
}

HRESULT D3D9::CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE type, D3DFORMAT f, BOOL, D3DMULTISAMPLE_TYPE ms, DWORD* q)
{
    if (Adapter != 0 || type != D3DDEVTYPE_HAL) return D3DERR_NOTAVAILABLE;
    if (q) *q = 1;
    if (ms == D3DMULTISAMPLE_NONE) return D3D_OK;
    if (ms == D3DMULTISAMPLE_NONMASKABLE) { if (q) *q = 2; return D3D_OK; }
    const FormatInfo* fi = format_info(f);
    if (!fi) return D3DERR_NOTAVAILABLE;
    if (ms == D3DMULTISAMPLE_2_SAMPLES || ms == D3DMULTISAMPLE_4_SAMPLES || ms == D3DMULTISAMPLE_8_SAMPLES) {
        SDL_GPUDevice* g = shared_gpu_device();
        SDL_GPUSampleCount sc = ms == 2 ? SDL_GPU_SAMPLECOUNT_2 : ms == 4 ? SDL_GPU_SAMPLECOUNT_4 : SDL_GPU_SAMPLECOUNT_8;
        if (g && SDL_GPUTextureSupportsSampleCount(g, fi->depth ? fi->gpu : SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM, sc)) return D3D_OK;
    }
    return D3DERR_NOTAVAILABLE;
}

HRESULT D3D9::CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE type, D3DFORMAT, D3DFORMAT, D3DFORMAT ds)
{
    if (Adapter != 0 || type != D3DDEVTYPE_HAL) return D3DERR_NOTAVAILABLE;
    const FormatInfo* fi = format_info(ds);
    return fi && fi->depth ? D3D_OK : D3DERR_NOTAVAILABLE;
}

HRESULT D3D9::CheckDeviceFormatConversion(UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT) { return D3D_OK; }

void fill_caps(D3DCAPS9* c)
{
    memset(c, 0, sizeof(*c));
    c->DeviceType = D3DDEVTYPE_HAL;
    c->AdapterOrdinal = 0;
    c->Caps = 0x00020000;                         // D3DCAPS_READ_SCANLINE
    c->Caps2 = D3DCAPS2_CANAUTOGENMIPMAP | D3DCAPS2_FULLSCREENGAMMA | 0x80000000 /*DYNAMICTEXTURES*/ | 0x20000000 /*CANMANAGERESOURCE*/;
    c->Caps3 = 0x20 | 0x80 | 0x100;               // ALPHA_FULLSCREEN_FLIP_OR_DISCARD, COPY_TO_VIDMEM, COPY_TO_SYSTEMMEM
    c->PresentationIntervals = D3DPRESENT_INTERVAL_IMMEDIATE | D3DPRESENT_INTERVAL_ONE;
    c->CursorCaps = 1;                            // D3DCURSORCAPS_COLOR
    c->DevCaps = 0x00000010 | 0x00000040 | 0x00000080 | 0x00000200 | 0x00000400 | 0x00002000 | 0x00004000 | 0x00008000 |
                 D3DDEVCAPS_HWTRANSFORMANDLIGHT | 0x00080000 /*DRAWPRIMITIVES2EX*/ | D3DDEVCAPS_PUREDEVICE | 0x00040000 /*HWRASTERIZATION*/;
    c->PrimitiveMiscCaps = 0x2 | 0x4 | 0x10 | 0x20 | 0x40 | 0x80 | 0x100 | 0x800 | 0x4000 | 0x8000 | 0x20000 | 0x40000;
    c->RasterCaps = 0x1 | 0x10 | 0x80 | 0x100 | 0x2000 | 0x8000 | 0x10000 | 0x20000 | 0x40000 | 0x100000 | 0x200000 | 0x1000000 | 0x2000000 | 0x4000000;
    c->ZCmpCaps = 0xff;
    c->SrcBlendCaps = 0x3fff;
    c->DestBlendCaps = 0x3fff;
    c->AlphaCmpCaps = 0xff;
    c->ShadeCaps = 0x8 | 0x200 | 0x4000 | 0x80000;
    c->TextureCaps = 0x1 | 0x4 | 0x8 | 0x40 | 0x400 | 0x800 | 0x4000 | 0x10000 | 0x200000;
    DWORD filt = 0x100 | 0x200 | D3DPTFILTERCAPS_MINFANISOTROPIC | 0x10000 | D3DPTFILTERCAPS_MIPFLINEAR | 0x1000000 | D3DPTFILTERCAPS_MAGFLINEAR | D3DPTFILTERCAPS_MAGFANISOTROPIC;
    c->TextureFilterCaps = filt;
    c->CubeTextureFilterCaps = filt;
    c->VolumeTextureFilterCaps = filt;
    c->TextureAddressCaps = 0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x20;
    c->VolumeTextureAddressCaps = c->TextureAddressCaps;
    c->LineCaps = 0x1 | 0x2 | 0x4 | 0x8 | 0x10;
    c->MaxTextureWidth = 8192;
    c->MaxTextureHeight = 8192;
    c->MaxVolumeExtent = 2048;
    c->MaxTextureRepeat = 8192;
    c->MaxTextureAspectRatio = 8192;
    c->MaxAnisotropy = 16;
    c->MaxVertexW = 1e10f;
    c->GuardBandLeft = -1e8f;
    c->GuardBandTop = -1e8f;
    c->GuardBandRight = 1e8f;
    c->GuardBandBottom = 1e8f;
    c->StencilCaps = 0x1ff;
    c->FVFCaps = 8 | 0x00080000 /*PSIZE*/;
    c->TextureOpCaps = 0x03ffffff;
    c->MaxTextureBlendStages = 8;
    c->MaxSimultaneousTextures = 8;
    c->VertexProcessingCaps = 0x1 | 0x2 | 0x8 | 0x10 | 0x20 | 0x100;
    c->MaxActiveLights = 8;
    c->MaxUserClipPlanes = 1;
    c->MaxVertexBlendMatrices = 1;
    c->MaxVertexBlendMatrixIndex = 0;
    c->MaxPointSize = 1.0f;
    c->MaxPrimitiveCount = 0x00ffffff;
    c->MaxVertexIndex = 0x00ffffff;
    c->MaxStreams = 16;
    c->MaxStreamStride = 255;
    c->VertexShaderVersion = D3DVS_VERSION(2, 0);
    c->MaxVertexShaderConst = 256;
    c->PixelShaderVersion = D3DPS_VERSION(2, 0);
    c->PixelShader1xMaxValue = 8.0f;
    c->DevCaps2 = 0x1 /*STREAMOFFSET*/ | 0x40 /*VERTEXELEMENTSCANSHARESTREAMOFFSET*/;
    c->MasterAdapterOrdinal = 0;
    c->AdapterOrdinalInGroup = 0;
    c->NumberOfAdaptersInGroup = 1;
    c->DeclTypes = 0x3ff;
    c->NumSimultaneousRTs = 1;
    c->StretchRectFilterCaps = 0x100 | 0x200 | 0x1000000 | 0x2000000;
    c->VS20Caps.NumTemps = 32;
    c->VS20Caps.StaticFlowControlDepth = 4;
    c->PS20Caps.NumTemps = 32;
    c->PS20Caps.NumInstructionSlots = 512;
    c->MaxVShaderInstructionsExecuted = 65535;
    c->MaxPShaderInstructionsExecuted = 65535;
}

HRESULT D3D9::GetDeviceCaps(UINT Adapter, D3DDEVTYPE type, D3DCAPS9* pCaps)
{
    if (Adapter != 0 || !pCaps) return D3DERR_INVALIDCALL;
    if (type != D3DDEVTYPE_HAL) return D3DERR_NOTAVAILABLE;
    fill_caps(pCaps);
    return D3D_OK;
}

HRESULT D3D9::CreateDevice(UINT Adapter, D3DDEVTYPE type, HWND focus, DWORD flags, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** out)
{
    if (!out || !pp || Adapter != 0) return D3DERR_INVALIDCALL;
    *out = nullptr;
    if (type != D3DDEVTYPE_HAL) return D3DERR_NOTAVAILABLE;
    Device* d = new Device();
    if (!d->init(this, focus, flags, pp)) {
        d->Release();
        return D3DERR_NOTAVAILABLE;
    }
    *out = d;
    return D3D_OK;
}

} // namespace mbg

extern "C" IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion)
{
    (void)SDKVersion;
    if (!mbp_sdl_init()) {
        mbg::log("SDL video init failed: %s", SDL_GetError());
        return nullptr;
    }
    return new mbg::D3D9();
}

// diagnostics for tests / the integration: draws issued and skipped (shader or pipeline failure), pipelines and shaders
// created, shader compile errors, bytes uploaded
extern "C" unsigned long long mbg_stat(int which) { return which >= 0 && which < mbg::STAT_COUNT ? mbg::g_stats[which] : 0; }
