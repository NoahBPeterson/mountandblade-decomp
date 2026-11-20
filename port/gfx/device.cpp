// port/gfx/device.cpp: IDirect3DDevice9 state, resource creation, state blocks, present / reset.
// The draw path (render passes, pipelines, uploads) is device_draw.cpp.
#include "gfx_internal.h"
#include "../platform/mb_sdl.h"

namespace mbg {

static inline DWORD fbits(float f) { DWORD d; memcpy(&d, &f, 4); return d; }

void mat_identity(D3DMATRIX& m)
{
    memset(&m, 0, sizeof m);
    m._11 = m._22 = m._33 = m._44 = 1.0f;
}

Device::Device() {}

Device::~Device()
{
    if (gpu) {
        flush(true);
        SDL_WaitForGPUIdle(gpu);
    }
    for (int i = 0; i < MAX_SAMPLERS; i++) safe_release(st.tex[i]);
    for (int i = 0; i < MAX_STREAMS; i++) safe_release(st.streams[i].vb);
    safe_release(st.indices);
    safe_release(st.decl);
    safe_release(st.vs);
    safe_release(st.ps);
    for (Surface*& s : rt) { safe_release(s); s = nullptr; }
    safe_release(ds);
    ds = nullptr;
    destroy_swap_targets();
    ffp.reset();
    for (auto& kv : fvf_decls) kv.second->Release();
    if (gpu) {
        for (auto& kv : pipes) SDL_ReleaseGPUGraphicsPipeline(gpu, kv.second);
        for (auto& kv : samplers) SDL_ReleaseGPUSampler(gpu, kv.second);
        if (dummy_tex) SDL_ReleaseGPUTexture(gpu, dummy_tex);
        if (white_tex) SDL_ReleaseGPUTexture(gpu, white_tex);
        for (SDL_GPUBuffer* b : retired) SDL_ReleaseGPUBuffer(gpu, b);
        if (window) SDL_ReleaseWindowFromGPUDevice(gpu, window);
    }
    if (cursor) SDL_DestroyCursor(cursor);
    safe_release(static_cast<IDirect3D9*>(d3d));
}

static SDL_GPUPresentMode present_mode(SDL_GPUDevice* g, SDL_Window* w, UINT interval)
{
    if (interval == D3DPRESENT_INTERVAL_IMMEDIATE) {
        if (SDL_WindowSupportsGPUPresentMode(g, w, SDL_GPU_PRESENTMODE_IMMEDIATE)) return SDL_GPU_PRESENTMODE_IMMEDIATE;
        if (SDL_WindowSupportsGPUPresentMode(g, w, SDL_GPU_PRESENTMODE_MAILBOX)) return SDL_GPU_PRESENTMODE_MAILBOX;
    }
    return SDL_GPU_PRESENTMODE_VSYNC;
}

bool Device::init(D3D9* parent, HWND focus, DWORD flags, D3DPRESENT_PARAMETERS* p)
{
    d3d = parent;
    d3d->AddRef();
    behavior = flags;
    gpu = shared_gpu_device();
    if (!gpu) return false;
    hwnd = p->hDeviceWindow ? p->hDeviceWindow : focus;
    window = mbp_sdl_window((struct HWND__*)hwnd);
    if (!window) {
        log("CreateDevice: no SDL window behind HWND %p", (void*)hwnd);
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(gpu, window)) {
        log("SDL_ClaimWindowForGPUDevice: %s", SDL_GetError());
        window = nullptr;
        return false;
    }
    pp = *p;
    if (pp.BackBufferWidth == 0 || pp.BackBufferHeight == 0) {
        int w = 0, h = 0;
        SDL_GetWindowSize(window, &w, &h);
        if (pp.BackBufferWidth == 0) pp.BackBufferWidth = (UINT)std::max(1, w);
        if (pp.BackBufferHeight == 0) pp.BackBufferHeight = (UINT)std::max(1, h);
    }
    if (pp.BackBufferFormat == D3DFMT_UNKNOWN) pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    if (pp.BackBufferCount == 0) pp.BackBufferCount = 1;
    *p = pp;
    SDL_SetGPUSwapchainParameters(gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, present_mode(gpu, window, pp.PresentationInterval));
    if (!pp.Windowed) SDL_SetWindowFullscreen(window, true);

    // dummy textures for unbound samplers
    SDL_GPUTextureCreateInfo ci{};
    ci.type = SDL_GPU_TEXTURETYPE_2D;
    ci.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    ci.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    ci.width = ci.height = 1;
    ci.layer_count_or_depth = 1;
    ci.num_levels = 1;
    dummy_tex = SDL_CreateGPUTexture(gpu, &ci);
    white_tex = SDL_CreateGPUTexture(gpu, &ci);
    {
        static const uint8_t black[4] = {0, 0, 0, 255}, white[4] = {255, 255, 255, 255};
        for (int i = 0; i < 2; i++) {
            PendingUpload u{};
            u.is_tex = true;
            u.raw_tex = i ? white_tex : dummy_tex;
            u.w = u.h = 1;
            u.staging_off = stage_bytes(i ? white : black, 4);
            u.size = 4;
            u.row_pixels = 1;
            u.rows = 1;
            uploads.push_back(u);
        }
    }
    if (!create_swap_targets()) return false;
    ffp = std::make_unique<Ffp>(this);
    reset_state();
    ps_consts.data = st.psc;
    ps_consts.size = 32 * 16;                  // the c0..c31 block of translated pixel shaders (asm.cpp)
    ps_consts.version = 1;
    vsdev_src.data = &vsdev; vsdev_src.size = sizeof vsdev;
    psdev_src.data = &psdev; psdev_src.size = sizeof psdev;
    return true;
}

bool Device::create_swap_targets()
{
    UINT samples = pp.MultiSampleType >= D3DMULTISAMPLE_2_SAMPLES ? (UINT)pp.MultiSampleType : 1;
    auto mk = [&](D3DFORMAT f, DWORD usage, UINT s) -> Surface* {
        auto img = make_image(pp.BackBufferWidth, pp.BackBufferHeight, 1, usage, f, D3DPOOL_DEFAULT, s);
        if (!img) return nullptr;
        Surface* surf = new Surface();
        surf->dev = this;
        surf->img = img;
        return surf;
    };
    D3DFORMAT bbf = pp.BackBufferFormat == D3DFMT_A8R8G8B8 ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
    backbuffer = mk(bbf, D3DUSAGE_RENDERTARGET, samples);
    frontbuffer = mk(bbf, D3DUSAGE_RENDERTARGET, samples);   // Present swaps the two images: keep them alike
    if (!backbuffer || !frontbuffer) return false;
    if (pp.EnableAutoDepthStencil) {
        auto_depth = mk(pp.AutoDepthStencilFormat ? pp.AutoDepthStencilFormat : D3DFMT_D24S8, D3DUSAGE_DEPTHSTENCIL, backbuffer->img->samples);
        if (!auto_depth) return false;
    }
    set_ref(rt[0], backbuffer);
    set_ref(ds, auto_depth);
    pass_targets_dirty = true;
    return true;
}

void Device::destroy_swap_targets()
{
    if (rt[0] == backbuffer) set_ref(rt[0], (Surface*)nullptr);
    if (ds == auto_depth) set_ref(ds, (Surface*)nullptr);
    safe_release(backbuffer);
    safe_release(frontbuffer);
    safe_release(auto_depth);
    backbuffer = frontbuffer = auto_depth = nullptr;
}

void Device::reset_state()
{
    DWORD* rs = st.rs;
    memset(st.rs, 0, sizeof st.rs);
    rs[D3DRS_ZENABLE] = pp.EnableAutoDepthStencil ? D3DZB_TRUE : D3DZB_FALSE;
    rs[D3DRS_FILLMODE] = D3DFILL_SOLID;
    rs[D3DRS_SHADEMODE] = D3DSHADE_GOURAUD;
    rs[D3DRS_ZWRITEENABLE] = TRUE;
    rs[D3DRS_LASTPIXEL] = TRUE;
    rs[D3DRS_SRCBLEND] = D3DBLEND_ONE;
    rs[D3DRS_DESTBLEND] = D3DBLEND_ZERO;
    rs[D3DRS_CULLMODE] = D3DCULL_CCW;
    rs[D3DRS_ZFUNC] = D3DCMP_LESSEQUAL;
    rs[D3DRS_ALPHAFUNC] = D3DCMP_ALWAYS;
    rs[D3DRS_FOGSTART] = fbits(0.0f);
    rs[D3DRS_FOGEND] = fbits(1.0f);
    rs[D3DRS_FOGDENSITY] = fbits(1.0f);
    rs[D3DRS_STENCILFAIL] = rs[D3DRS_STENCILZFAIL] = rs[D3DRS_STENCILPASS] = D3DSTENCILOP_KEEP;
    rs[D3DRS_STENCILFUNC] = D3DCMP_ALWAYS;
    rs[D3DRS_STENCILMASK] = rs[D3DRS_STENCILWRITEMASK] = 0xffffffff;
    rs[D3DRS_TEXTUREFACTOR] = 0xffffffff;
    rs[D3DRS_CLIPPING] = TRUE;
    rs[D3DRS_LIGHTING] = TRUE;
    rs[D3DRS_COLORVERTEX] = TRUE;
    rs[D3DRS_LOCALVIEWER] = TRUE;
    rs[D3DRS_DIFFUSEMATERIALSOURCE] = D3DMCS_COLOR1;
    rs[D3DRS_SPECULARMATERIALSOURCE] = D3DMCS_COLOR2;
    rs[D3DRS_POINTSIZE] = fbits(1.0f);
    rs[D3DRS_POINTSIZE_MIN] = fbits(1.0f);
    rs[166] = fbits(64.0f);                    // POINTSIZE_MAX
    rs[D3DRS_MULTISAMPLEANTIALIAS] = TRUE;
    rs[D3DRS_MULTISAMPLEMASK] = 0xffffffff;
    rs[D3DRS_COLORWRITEENABLE] = 0xf;
    rs[190] = rs[191] = rs[192] = 0xf;         // COLORWRITEENABLE1..3
    rs[193] = 0xffffffff;                      // BLENDFACTOR
    rs[D3DRS_BLENDOP] = D3DBLENDOP_ADD;
    rs[D3DRS_SRCBLENDALPHA] = D3DBLEND_ONE;
    rs[D3DRS_DESTBLENDALPHA] = D3DBLEND_ZERO;
    rs[D3DRS_BLENDOPALPHA] = D3DBLENDOP_ADD;
    rs[186] = rs[187] = rs[188] = rs[189] = D3DSTENCILOP_KEEP;   // CCW stencil ops
    rs[189] = D3DCMP_ALWAYS;                   // CCW_STENCILFUNC
    for (int s = 0; s < MAX_STAGES; s++) {
        DWORD* t = st.tss[s];
        memset(t, 0, sizeof st.tss[s]);
        t[D3DTSS_COLOROP] = s == 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE;
        t[D3DTSS_COLORARG1] = D3DTA_TEXTURE;
        t[D3DTSS_COLORARG2] = D3DTA_CURRENT;
        t[D3DTSS_ALPHAOP] = s == 0 ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
        t[D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
        t[D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
        t[D3DTSS_TEXCOORDINDEX] = (DWORD)s;
        t[D3DTSS_TEXTURETRANSFORMFLAGS] = D3DTTFF_DISABLE;
        t[D3DTSS_COLORARG0] = t[D3DTSS_ALPHAARG0] = t[D3DTSS_RESULTARG] = D3DTA_CURRENT;
    }
    for (int s = 0; s < MAX_SAMPLERS; s++) {
        DWORD* t = st.ss[s];
        memset(t, 0, sizeof st.ss[s]);
        t[D3DSAMP_ADDRESSU] = t[D3DSAMP_ADDRESSV] = t[D3DSAMP_ADDRESSW] = D3DTADDRESS_WRAP;
        t[D3DSAMP_MAGFILTER] = t[D3DSAMP_MINFILTER] = D3DTEXF_POINT;
        t[D3DSAMP_MIPFILTER] = D3DTEXF_NONE;
        t[D3DSAMP_MAXANISOTROPY] = 1;
    }
    for (auto& m : st.world) mat_identity(m);
    mat_identity(st.view);
    mat_identity(st.proj);
    for (auto& m : st.texm) mat_identity(m);
    memset(&st.material, 0, sizeof st.material);
    st.lights.clear();
    memset(st.clip, 0, sizeof st.clip);
    Surface* t = rt[0];
    UINT w = t ? t->width() : pp.BackBufferWidth, h = t ? t->height() : pp.BackBufferHeight;
    st.vp = D3DVIEWPORT9{0, 0, w, h, 0.0f, 1.0f};
    st.scissor = RECT{0, 0, (LONG)w, (LONG)h};
    st.fvf = 0;
    vp_dirty = scissor_dirty = true;
}

// ---- simple queries ----
HRESULT Device::TestCooperativeLevel() { return D3D_OK; }
UINT Device::GetAvailableTextureMem() { return 0x40000000u; }
HRESULT Device::EvictManagedResources() { return D3D_OK; }
HRESULT Device::GetDirect3D(IDirect3D9** pp_)
{
    if (!pp_) return D3DERR_INVALIDCALL;
    *pp_ = d3d;
    d3d->AddRef();
    return D3D_OK;
}
HRESULT Device::GetDeviceCaps(D3DCAPS9* c)
{
    if (!c) return D3DERR_INVALIDCALL;
    fill_caps(c);
    return D3D_OK;
}
HRESULT Device::GetDisplayMode(UINT, D3DDISPLAYMODE* m)
{
    if (!m) return D3DERR_INVALIDCALL;
    SDL_DisplayID did = SDL_GetDisplayForWindow(window);
    const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(did ? did : SDL_GetPrimaryDisplay());
    m->Width = dm ? (UINT)dm->w : pp.BackBufferWidth;
    m->Height = dm ? (UINT)dm->h : pp.BackBufferHeight;
    m->RefreshRate = dm ? (UINT)(dm->refresh_rate + 0.5f) : 60;
    m->Format = D3DFMT_X8R8G8B8;
    return D3D_OK;
}
HRESULT Device::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* p)
{
    if (!p) return D3DERR_INVALIDCALL;
    p->AdapterOrdinal = 0;
    p->DeviceType = D3DDEVTYPE_HAL;
    p->hFocusWindow = hwnd;
    p->BehaviorFlags = behavior;
    return D3D_OK;
}
HRESULT Device::GetRasterStatus(UINT, D3DRASTER_STATUS* p)
{
    if (!p) return D3DERR_INVALIDCALL;
    p->InVBlank = FALSE;
    p->ScanLine = 0;
    return D3D_OK;
}
HRESULT Device::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9**) { return D3DERR_NOTAVAILABLE; }
HRESULT Device::GetSwapChain(UINT, IDirect3DSwapChain9** p) { if (p) *p = nullptr; return D3DERR_NOTAVAILABLE; }

// ---- cursor ----
HRESULT Device::SetCursorProperties(UINT hx, UINT hy, IDirect3DSurface9* bmp)
{
    Surface* s = static_cast<Surface*>(bmp);
    if (!s) return D3DERR_INVALIDCALL;
    UINT w = s->width(), h = s->height();
    std::vector<uint8_t>& sh = s->img->level_shadow(s->level, s->face);
    SDL_Surface* surf = SDL_CreateSurface((int)w, (int)h, SDL_PIXELFORMAT_ARGB8888);
    if (!surf) return D3DERR_INVALIDCALL;
    decode_to_bgra(s->img->format, sh.data(), format_pitch(s->img->format, w), w, h, (uint8_t*)surf->pixels, (UINT)surf->pitch);
    SDL_Cursor* c = SDL_CreateColorCursor(surf, (int)hx, (int)hy);
    SDL_DestroySurface(surf);
    if (!c) return D3DERR_INVALIDCALL;
    if (cursor) SDL_DestroyCursor(cursor);
    cursor = c;
    if (cursor_visible) SDL_SetCursor(cursor);
    return D3D_OK;
}
void Device::SetCursorPosition(int, int, DWORD)
{
    // the D3D9 hardware cursor tracks the system cursor here (it IS the system cursor): nothing to move
}
BOOL Device::ShowCursor(BOOL show)
{
    BOOL was = cursor_visible;
    cursor_visible = show != 0;
    if (cursor_visible) {
        if (cursor) SDL_SetCursor(cursor);
        SDL_ShowCursor();
    } else {
        SDL_HideCursor();
    }
    return was;
}

// ---- reset / present ----
HRESULT Device::Reset(D3DPRESENT_PARAMETERS* p)
{
    if (!p) return D3DERR_INVALIDCALL;
    flush(true);
    for (int i = 0; i < 4; i++) set_ref(rt[i], (Surface*)nullptr);
    set_ref(ds, (Surface*)nullptr);
    destroy_swap_targets();
    pp = *p;
    if (pp.BackBufferWidth == 0 || pp.BackBufferHeight == 0) {
        int w = 0, h = 0;
        SDL_GetWindowSize(window, &w, &h);
        if (pp.BackBufferWidth == 0) pp.BackBufferWidth = (UINT)std::max(1, w);
        if (pp.BackBufferHeight == 0) pp.BackBufferHeight = (UINT)std::max(1, h);
    }
    if (pp.BackBufferFormat == D3DFMT_UNKNOWN) pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    *p = pp;
    SDL_SetWindowFullscreen(window, !pp.Windowed);
    SDL_SetGPUSwapchainParameters(gpu, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, present_mode(gpu, window, pp.PresentationInterval));
    if (!create_swap_targets()) return D3DERR_NOTAVAILABLE;
    for (int i = 0; i < MAX_SAMPLERS; i++) set_ref(st.tex[i], (IDirect3DBaseTexture9*)nullptr);
    for (int i = 0; i < MAX_STREAMS; i++) { set_ref(st.streams[i].vb, (VertexBuffer*)nullptr); st.streams[i] = StreamSrc{}; }
    set_ref(st.indices, (IndexBuffer*)nullptr);
    set_ref(st.decl, (VertexDecl*)nullptr);
    set_ref(st.vs, (VShader*)nullptr);
    set_ref(st.ps, (PShader*)nullptr);
    reset_state();
    return D3D_OK;
}

HRESULT Device::Present(CONST RECT* src, CONST RECT* dst, HWND, CONST RGNDATA*)
{
    end_pass();
    ensure_cmd();
    SDL_GPUTexture* sw = nullptr;
    Uint32 sw_w = 0, sw_h = 0;
    if (SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &sw, &sw_w, &sw_h) && sw) {
        SDL_GPUBlitInfo bi{};
        GpuImage* bb = backbuffer->img.get();
        bi.source.texture = bb->tex;
        bi.source.x = src ? (Uint32)src->left : 0;
        bi.source.y = src ? (Uint32)src->top : 0;
        bi.source.w = src ? (Uint32)(src->right - src->left) : bb->width;
        bi.source.h = src ? (Uint32)(src->bottom - src->top) : bb->height;
        bi.destination.texture = sw;
        bi.destination.x = dst ? (Uint32)dst->left : 0;
        bi.destination.y = dst ? (Uint32)dst->top : 0;
        bi.destination.w = dst ? (Uint32)(dst->right - dst->left) : sw_w;
        bi.destination.h = dst ? (Uint32)(dst->bottom - dst->top) : sw_h;
        bi.load_op = SDL_GPU_LOADOP_DONT_CARE;
        bi.filter = (bi.source.w == bi.destination.w && bi.source.h == bi.destination.h) ? SDL_GPU_FILTER_NEAREST : SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(cmd, &bi);
    }
    flush(false);
    // DISCARD swap: the presented image becomes the front buffer (GetFrontBufferData), the old front the new back
    std::swap(backbuffer->img, frontbuffer->img);
    pass_targets_dirty = true;
    frame++;
    return D3D_OK;
}

HRESULT Device::GetBackBuffer(UINT, UINT idx, D3DBACKBUFFER_TYPE, IDirect3DSurface9** out)
{
    if (!out || idx != 0) return D3DERR_INVALIDCALL;
    *out = backbuffer;
    backbuffer->AddRef();
    return D3D_OK;
}

// ---- resource creation ----
HRESULT Device::CreateTexture(UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT fmt, D3DPOOL pool, IDirect3DTexture9** out, HANDLE*)
{
    if (!out) return D3DERR_INVALIDCALL;
    *out = nullptr;
    if (usage & D3DUSAGE_DEPTHSTENCIL) {
        const FormatInfo* fi = format_info(fmt);
        if (!fi || !fi->depth) return D3DERR_INVALIDCALL;
    }
    auto img = make_image(w, h, levels, usage, fmt, pool);
    if (!img) return D3DERR_INVALIDCALL;
    Texture* t = new Texture();
    t->dev = this;
    t->img = img;
    for (UINT l = 0; l < img->levels; l++) {
        Surface* s = new Surface();
        s->dev = this;
        s->img = img;
        s->level = l;
        s->container = t;
        s->type = D3DRTYPE_TEXTURE;
        t->surfaces.push_back(s);
    }
    *out = t;
    return D3D_OK;
}

HRESULT Device::CreateVertexBuffer(UINT len, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9** out, HANDLE*)
{
    if (!out || len == 0) return D3DERR_INVALIDCALL;
    VertexBuffer* vb = new VertexBuffer();
    vb->fvf = fvf;
    if (!vb->b.init(this, len, usage, pool, false)) {
        vb->Release();
        *out = nullptr;
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    *out = vb;
    return D3D_OK;
}

HRESULT Device::CreateIndexBuffer(UINT len, DWORD usage, D3DFORMAT fmt, D3DPOOL pool, IDirect3DIndexBuffer9** out, HANDLE*)
{
    if (!out || len == 0 || (fmt != D3DFMT_INDEX16 && fmt != D3DFMT_INDEX32)) return D3DERR_INVALIDCALL;
    IndexBuffer* ib = new IndexBuffer();
    ib->format = fmt;
    if (!ib->b.init(this, len, usage, pool, true)) {
        ib->Release();
        *out = nullptr;
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    *out = ib;
    return D3D_OK;
}

static Surface* new_surface(Device* d, std::shared_ptr<GpuImage> img)
{
    Surface* s = new Surface();
    s->dev = d;
    s->img = std::move(img);
    return s;
}

HRESULT Device::CreateRenderTarget(UINT w, UINT h, D3DFORMAT fmt, D3DMULTISAMPLE_TYPE ms, DWORD, BOOL, IDirect3DSurface9** out, HANDLE*)
{
    if (!out) return D3DERR_INVALIDCALL;
    auto img = make_image(w, h, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT, ms >= 2 ? (UINT)ms : 1);
    if (!img) { *out = nullptr; return D3DERR_NOTAVAILABLE; }
    *out = new_surface(this, img);
    return D3D_OK;
}

HRESULT Device::CreateDepthStencilSurface(UINT w, UINT h, D3DFORMAT fmt, D3DMULTISAMPLE_TYPE ms, DWORD, BOOL, IDirect3DSurface9** out, HANDLE*)
{
    if (!out) return D3DERR_INVALIDCALL;
    const FormatInfo* fi = format_info(fmt);
    if (!fi || !fi->depth) { *out = nullptr; return D3DERR_NOTAVAILABLE; }
    auto img = make_image(w, h, 1, D3DUSAGE_DEPTHSTENCIL, fmt, D3DPOOL_DEFAULT, ms >= 2 ? (UINT)ms : 1);
    if (!img) { *out = nullptr; return D3DERR_NOTAVAILABLE; }
    *out = new_surface(this, img);
    return D3D_OK;
}

HRESULT Device::CreateOffscreenPlainSurface(UINT w, UINT h, D3DFORMAT fmt, D3DPOOL pool, IDirect3DSurface9** out, HANDLE*)
{
    if (!out) return D3DERR_INVALIDCALL;
    auto img = make_image(w, h, 1, 0, fmt, pool);
    if (!img) { *out = nullptr; return D3DERR_INVALIDCALL; }
    img->level_shadow(0);
    *out = new_surface(this, img);
    return D3D_OK;
}

// CPU copy between surfaces of the same format (UpdateSurface / UpdateTexture)
static void copy_surface_cpu(Surface* s, const RECT& sr, Surface* d, POINT dp)
{
    std::vector<uint8_t>& src = s->img->level_shadow(s->level, s->face);
    std::vector<uint8_t>& dst = d->img->level_shadow(d->level, d->face);
    D3DFORMAT f = s->img->format;
    UINT sp = format_pitch(f, s->width()), dpitch = format_pitch(f, d->width());
    bool blk = format_is_block(f);
    int bs = blk ? format_info(f)->block : format_info(f)->bpp;
    int x0 = blk ? sr.left / 4 : sr.left, y0 = blk ? sr.top / 4 : sr.top;
    int x1 = blk ? (sr.right + 3) / 4 : sr.right, y1 = blk ? (sr.bottom + 3) / 4 : sr.bottom;
    int dx = blk ? dp.x / 4 : dp.x, dy = blk ? dp.y / 4 : dp.y;
    for (int y = y0; y < y1; y++)
        memcpy(dst.data() + (size_t)(dy + y - y0) * dpitch + (size_t)dx * bs, src.data() + (size_t)y * sp + (size_t)x0 * bs, (size_t)(x1 - x0) * bs);
}

HRESULT Device::UpdateSurface(IDirect3DSurface9* src, CONST RECT* sr, IDirect3DSurface9* dst, CONST POINT* dp)
{
    Surface* s = static_cast<Surface*>(src);
    Surface* d = static_cast<Surface*>(dst);
    if (!s || !d || s->img->format != d->img->format) return D3DERR_INVALIDCALL;
    RECT r = sr ? *sr : RECT{0, 0, (LONG)s->width(), (LONG)s->height()};
    POINT p = dp ? *dp : POINT{0, 0};
    copy_surface_cpu(s, r, d, p);
    d->img->shadow_valid[d->face * d->img->levels + d->level] = 1;
    if (d->img->tex) queue_texture_upload(d->img, d->level, d->face);
    return D3D_OK;
}

HRESULT Device::UpdateTexture(IDirect3DBaseTexture9* src, IDirect3DBaseTexture9* dst)
{
    Texture* s = static_cast<Texture*>(static_cast<IDirect3DTexture9*>(src));
    Texture* d = static_cast<Texture*>(static_cast<IDirect3DTexture9*>(dst));
    if (!s || !d || s->img->format != d->img->format) return D3DERR_INVALIDCALL;
    UINT n = std::min(s->img->levels, d->img->levels);
    for (UINT l = 0; l < n; l++) {
        Surface* ss = s->surfaces[l];
        Surface* ds_ = d->surfaces[l];
        copy_surface_cpu(ss, RECT{0, 0, (LONG)ss->width(), (LONG)ss->height()}, ds_, POINT{0, 0});
        d->img->shadow_valid[l] = 1;
        if (d->img->tex) queue_texture_upload(d->img, l, 0);
    }
    return D3D_OK;
}

HRESULT Device::GetRenderTargetData(IDirect3DSurface9* rts, IDirect3DSurface9* dst)
{
    Surface* s = static_cast<Surface*>(rts);
    Surface* d = static_cast<Surface*>(dst);
    if (!s || !d || !s->img->tex) return D3DERR_INVALIDCALL;
    if (s->width() != d->width() || s->height() != d->height()) return D3DERR_INVALIDCALL;
    std::vector<uint8_t> raw;
    if (!readback(s->img.get(), s->level, raw)) return D3DERR_DRIVERINTERNALERROR;
    std::vector<uint8_t>& sh = d->img->level_shadow(d->level, d->face);
    if (!convert_from_gpu(d->img->format, s->img->gpu_format, raw.data(), s->width(), s->height(), sh.data(), format_pitch(d->img->format, d->width())))
        return D3DERR_INVALIDCALL;
    d->img->shadow_valid[d->face * d->img->levels + d->level] = 1;
    return D3D_OK;
}

HRESULT Device::GetFrontBufferData(UINT, IDirect3DSurface9* dst)
{
    // D3D9 returns the desktop-sized screen image; the window's client area sits at its screen position
    Surface* d = static_cast<Surface*>(dst);
    if (!d) return D3DERR_INVALIDCALL;
    std::vector<uint8_t> raw;
    GpuImage* fb = frontbuffer->img.get();
    if (!readback(fb, 0, raw)) return D3DERR_DRIVERINTERNALERROR;
    std::vector<uint8_t>& sh = d->img->level_shadow(d->level, d->face);
    std::fill(sh.begin(), sh.end(), 0);
    int wx = 0, wy = 0;
    if (pp.Windowed) SDL_GetWindowPosition(window, &wx, &wy);
    UINT dp = format_pitch(d->img->format, d->width());
    for (UINT y = 0; y < fb->height; y++) {
        int ty = wy + (int)y;
        if (ty < 0 || ty >= (int)d->height()) continue;
        for (UINT x = 0; x < fb->width; x++) {
            int tx = wx + (int)x;
            if (tx < 0 || tx >= (int)d->width()) continue;
            const uint8_t* p = &raw[((size_t)y * fb->width + x) * 4];
            uint8_t* o = &sh[(size_t)ty * dp + (size_t)tx * 4];
            o[0] = p[0]; o[1] = p[1]; o[2] = p[2]; o[3] = 255;
        }
    }
    d->img->shadow_valid[d->face * d->img->levels + d->level] = 1;
    return D3D_OK;
}

HRESULT Device::StretchRect(IDirect3DSurface9* src, CONST RECT* sr, IDirect3DSurface9* dst, CONST RECT* dr, D3DTEXTUREFILTERTYPE f)
{
    Surface* s = static_cast<Surface*>(src);
    Surface* d = static_cast<Surface*>(dst);
    if (!s || !d || !s->img->tex || !d->img->tex) return D3DERR_INVALIDCALL;
    end_pass();
    ensure_cmd();
    SDL_GPUBlitInfo bi{};
    bi.source.texture = s->img->tex;
    bi.source.mip_level = s->level;
    bi.source.x = sr ? (Uint32)sr->left : 0; bi.source.y = sr ? (Uint32)sr->top : 0;
    bi.source.w = sr ? (Uint32)(sr->right - sr->left) : s->width(); bi.source.h = sr ? (Uint32)(sr->bottom - sr->top) : s->height();
    bi.destination.texture = d->img->tex;
    bi.destination.mip_level = d->level;
    bi.destination.x = dr ? (Uint32)dr->left : 0; bi.destination.y = dr ? (Uint32)dr->top : 0;
    bi.destination.w = dr ? (Uint32)(dr->right - dr->left) : d->width(); bi.destination.h = dr ? (Uint32)(dr->bottom - dr->top) : d->height();
    bi.load_op = SDL_GPU_LOADOP_LOAD;
    bi.filter = f == D3DTEXF_POINT || f == D3DTEXF_NONE ? SDL_GPU_FILTER_NEAREST : SDL_GPU_FILTER_LINEAR;
    SDL_BlitGPUTexture(cmd, &bi);
    s->img->used_batch = d->img->used_batch = batch;
    d->img->gpu_newer = true;
    return D3D_OK;
}

HRESULT Device::ColorFill(IDirect3DSurface9* surf, CONST RECT* r, D3DCOLOR c)
{
    Surface* s = static_cast<Surface*>(surf);
    if (!s) return D3DERR_INVALIDCALL;
    Surface* old_rt = rt[0];
    Surface* old_ds = ds;
    D3DVIEWPORT9 ovp = st.vp;
    DWORD sc = st.rs[D3DRS_SCISSORTESTENABLE];
    RECT osc = st.scissor;
    safe_addref(old_rt); safe_addref(old_ds);
    SetRenderTarget(0, s);
    SetDepthStencilSurface(nullptr);
    if (r) {
        st.rs[D3DRS_SCISSORTESTENABLE] = TRUE;
        st.scissor = *r;
    }
    Clear(0, nullptr, D3DCLEAR_TARGET, c, 1.0f, 0);
    SetRenderTarget(0, old_rt);
    SetDepthStencilSurface(old_ds);
    safe_release(old_rt); safe_release(old_ds);
    st.vp = ovp;
    st.rs[D3DRS_SCISSORTESTENABLE] = sc;
    st.scissor = osc;
    vp_dirty = scissor_dirty = true;
    return D3D_OK;
}

// ---- targets ----
HRESULT Device::SetRenderTarget(DWORD idx, IDirect3DSurface9* s)
{
    if (idx >= 4 || (idx == 0 && !s)) return D3DERR_INVALIDCALL;
    Surface* surf = static_cast<Surface*>(s);
    if (idx != 0) { set_ref(rt[idx], surf); return D3D_OK; }   // MRT not used: kept for Get
    if (rt[0] != surf) {
        set_ref(rt[0], surf);
        pass_targets_dirty = true;
    }
    st.vp = D3DVIEWPORT9{0, 0, surf->width(), surf->height(), 0.0f, 1.0f};
    st.scissor = RECT{0, 0, (LONG)surf->width(), (LONG)surf->height()};
    vp_dirty = scissor_dirty = true;
    return D3D_OK;
}
HRESULT Device::GetRenderTarget(DWORD idx, IDirect3DSurface9** out)
{
    if (!out || idx >= 4) return D3DERR_INVALIDCALL;
    *out = rt[idx];
    if (!rt[idx]) return D3DERR_NOTFOUND;
    rt[idx]->AddRef();
    return D3D_OK;
}
HRESULT Device::SetDepthStencilSurface(IDirect3DSurface9* s)
{
    Surface* surf = static_cast<Surface*>(s);
    if (ds != surf) {
        set_ref(ds, surf);
        pass_targets_dirty = true;
    }
    return D3D_OK;
}
HRESULT Device::GetDepthStencilSurface(IDirect3DSurface9** out)
{
    if (!out) return D3DERR_INVALIDCALL;
    *out = ds;
    if (!ds) return D3DERR_NOTFOUND;
    ds->AddRef();
    return D3D_OK;
}
HRESULT Device::BeginScene() { in_scene = true; return D3D_OK; }
HRESULT Device::EndScene() { in_scene = false; return D3D_OK; }

// ---- fixed-function state ----
void Device::record(int kind, DWORD a, DWORD b)
{
    if (rec) rec->items.push_back(SbItem{kind, a, b});
}

HRESULT Device::SetTransform(D3DTRANSFORMSTATETYPE s, CONST D3DMATRIX* m)
{
    if (!m) return D3DERR_INVALIDCALL;
    D3DMATRIX* dst = nullptr;
    DWORD i = (DWORD)s;
    if (i == D3DTS_VIEW) dst = &st.view;
    else if (i == D3DTS_PROJECTION) dst = &st.proj;
    else if (i >= D3DTS_TEXTURE0 && i <= D3DTS_TEXTURE7) dst = &st.texm[i - D3DTS_TEXTURE0];
    else if (i >= 256 && i < 260) dst = &st.world[i - 256];
    else return D3D_OK;
    record(SB_XFORM, i);
    *dst = *m;
    return D3D_OK;
}
HRESULT Device::GetTransform(D3DTRANSFORMSTATETYPE s, D3DMATRIX* m)
{
    if (!m) return D3DERR_INVALIDCALL;
    DWORD i = (DWORD)s;
    if (i == D3DTS_VIEW) *m = st.view;
    else if (i == D3DTS_PROJECTION) *m = st.proj;
    else if (i >= D3DTS_TEXTURE0 && i <= D3DTS_TEXTURE7) *m = st.texm[i - D3DTS_TEXTURE0];
    else if (i >= 256 && i < 260) *m = st.world[i - 256];
    else mat_identity(*m);
    return D3D_OK;
}
HRESULT Device::MultiplyTransform(D3DTRANSFORMSTATETYPE s, CONST D3DMATRIX* m)
{
    if (!m) return D3DERR_INVALIDCALL;
    D3DMATRIX cur, out;
    GetTransform(s, &cur);
    mat_mul(out, *m, cur);
    return SetTransform(s, &out);
}
HRESULT Device::SetViewport(CONST D3DVIEWPORT9* v)
{
    if (!v) return D3DERR_INVALIDCALL;
    record(SB_VP, 0);
    st.vp = *v;
    vp_dirty = true;
    return D3D_OK;
}
HRESULT Device::GetViewport(D3DVIEWPORT9* v)
{
    if (!v) return D3DERR_INVALIDCALL;
    *v = st.vp;
    return D3D_OK;
}
HRESULT Device::SetMaterial(CONST D3DMATERIAL9* m)
{
    if (!m) return D3DERR_INVALIDCALL;
    record(SB_MATERIAL, 0);
    st.material = *m;
    return D3D_OK;
}
HRESULT Device::GetMaterial(D3DMATERIAL9* m)
{
    if (!m) return D3DERR_INVALIDCALL;
    *m = st.material;
    return D3D_OK;
}
HRESULT Device::SetLight(DWORD i, CONST D3DLIGHT9* l)
{
    if (!l) return D3DERR_INVALIDCALL;
    record(SB_LIGHT, i);
    st.lights[i].l = *l;
    return D3D_OK;
}
HRESULT Device::GetLight(DWORD i, D3DLIGHT9* l)
{
    auto it = st.lights.find(i);
    if (!l || it == st.lights.end()) return D3DERR_INVALIDCALL;
    *l = it->second.l;
    return D3D_OK;
}
HRESULT Device::LightEnable(DWORD i, BOOL on)
{
    record(SB_LIGHTEN, i);
    auto it = st.lights.find(i);
    if (it == st.lights.end()) {
        if (!on) return D3D_OK;
        Light L;     // D3D9: enabling an unset light gives the default white directional light along +z
        L.l.Type = D3DLIGHT_DIRECTIONAL;
        L.l.Diffuse = D3DCOLORVALUE{1, 1, 1, 0};
        L.l.Direction = D3DVECTOR{0, 0, 1};
        it = st.lights.emplace(i, L).first;
    }
    it->second.enabled = on != 0;
    return D3D_OK;
}
HRESULT Device::GetLightEnable(DWORD i, BOOL* on)
{
    auto it = st.lights.find(i);
    if (!on || it == st.lights.end()) return D3DERR_INVALIDCALL;
    *on = it->second.enabled ? 128 : 0;
    return D3D_OK;
}
HRESULT Device::SetClipPlane(DWORD i, CONST float* p)
{
    if (i >= 6 || !p) return D3DERR_INVALIDCALL;
    record(SB_CLIP, i);
    memcpy(st.clip[i], p, 16);
    return D3D_OK;
}
HRESULT Device::GetClipPlane(DWORD i, float* p)
{
    if (i >= 6 || !p) return D3DERR_INVALIDCALL;
    memcpy(p, st.clip[i], 16);
    return D3D_OK;
}
HRESULT Device::SetRenderState(D3DRENDERSTATETYPE s, DWORD v)
{
    if ((DWORD)s >= 256) return D3DERR_INVALIDCALL;
    record(SB_RS, s);
    if (s == D3DRS_SCISSORTESTENABLE && st.rs[s] != v) scissor_dirty = true;
    st.rs[s] = v;
    return D3D_OK;
}
HRESULT Device::GetRenderState(D3DRENDERSTATETYPE s, DWORD* v)
{
    if ((DWORD)s >= 256 || !v) return D3DERR_INVALIDCALL;
    *v = st.rs[s];
    return D3D_OK;
}
static DWORD sampler_index(DWORD s)
{
    if (s < 16) return s;
    if (s >= 256 && s <= 260) return 16 + (s - 256) < MAX_SAMPLERS ? 15 : 15;   // D3DDMAPSAMPLER / vertex samplers: unused
    return 0xffffffff;
}
HRESULT Device::GetTexture(DWORD stage, IDirect3DBaseTexture9** out)
{
    DWORD i = sampler_index(stage);
    if (!out || i >= MAX_SAMPLERS) return D3DERR_INVALIDCALL;
    *out = st.tex[i];
    safe_addref(*out);
    return D3D_OK;
}
HRESULT Device::SetTexture(DWORD stage, IDirect3DBaseTexture9* t)
{
    DWORD i = sampler_index(stage);
    if (i >= MAX_SAMPLERS) return D3DERR_INVALIDCALL;
    record(SB_TEX, i);
    set_ref(st.tex[i], t);
    return D3D_OK;
}
HRESULT Device::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE t, DWORD* v)
{
    if (stage >= MAX_STAGES || (DWORD)t >= 33 || !v) return D3DERR_INVALIDCALL;
    *v = st.tss[stage][t];
    return D3D_OK;
}
HRESULT Device::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE t, DWORD v)
{
    if (stage >= MAX_STAGES || (DWORD)t >= 33) return D3DERR_INVALIDCALL;
    record(SB_TSS, stage, t);
    st.tss[stage][t] = v;
    return D3D_OK;
}
HRESULT Device::GetSamplerState(DWORD s, D3DSAMPLERSTATETYPE t, DWORD* v)
{
    DWORD i = sampler_index(s);
    if (i >= MAX_SAMPLERS || (DWORD)t >= 14 || !v) return D3DERR_INVALIDCALL;
    *v = st.ss[i][t];
    return D3D_OK;
}
HRESULT Device::SetSamplerState(DWORD s, D3DSAMPLERSTATETYPE t, DWORD v)
{
    DWORD i = sampler_index(s);
    if (i >= MAX_SAMPLERS || (DWORD)t >= 14) return D3DERR_INVALIDCALL;
    record(SB_SS, i, t);
    st.ss[i][t] = v;
    return D3D_OK;
}
HRESULT Device::SetScissorRect(CONST RECT* r)
{
    if (!r) return D3DERR_INVALIDCALL;
    record(SB_SCISSOR, 0);
    st.scissor = *r;
    scissor_dirty = true;
    return D3D_OK;
}
HRESULT Device::GetScissorRect(RECT* r)
{
    if (!r) return D3DERR_INVALIDCALL;
    *r = st.scissor;
    return D3D_OK;
}

// ---- vertex input ----
HRESULT Device::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* e, IDirect3DVertexDeclaration9** out)
{
    if (!e || !out) return D3DERR_INVALIDCALL;
    VertexDecl* d = new VertexDecl();
    d->dev = this;
    for (; e->Stream != 0xff; e++) {
        d->elems.push_back(*e);
        if (e->Usage == D3DDECLUSAGE_POSITIONT) d->rhw = true;
    }
    d->hash = hash_bytes(d->elems.data(), d->elems.size() * sizeof(D3DVERTEXELEMENT9));
    *out = d;
    return D3D_OK;
}
VertexDecl* Device::decl_for_fvf(DWORD fvf)
{
    auto it = fvf_decls.find(fvf);
    if (it != fvf_decls.end()) return it->second;
    std::vector<D3DVERTEXELEMENT9> el;
    decl_from_fvf(fvf, el);
    el.push_back(D3DVERTEXELEMENT9 D3DDECL_END());
    IDirect3DVertexDeclaration9* d = nullptr;
    CreateVertexDeclaration(el.data(), &d);
    VertexDecl* vd = static_cast<VertexDecl*>(d);
    vd->fvf = fvf;
    fvf_decls[fvf] = vd;
    return vd;
}
HRESULT Device::SetVertexDeclaration(IDirect3DVertexDeclaration9* d)
{
    record(SB_DECL, 0);
    set_ref(st.decl, static_cast<VertexDecl*>(d));
    st.fvf = st.decl ? st.decl->fvf : 0;
    return D3D_OK;
}
HRESULT Device::GetVertexDeclaration(IDirect3DVertexDeclaration9** out)
{
    if (!out) return D3DERR_INVALIDCALL;
    *out = st.decl;
    safe_addref(st.decl);
    return D3D_OK;
}
HRESULT Device::SetFVF(DWORD fvf)
{
    record(SB_FVF, 0);
    if (fvf == 0) return D3D_OK;
    set_ref(st.decl, decl_for_fvf(fvf));
    st.fvf = fvf;
    return D3D_OK;
}
HRESULT Device::GetFVF(DWORD* f)
{
    if (!f) return D3DERR_INVALIDCALL;
    *f = st.fvf;
    return D3D_OK;
}
HRESULT Device::SetStreamSource(UINT n, IDirect3DVertexBuffer9* vb, UINT off, UINT stride)
{
    if (n >= MAX_STREAMS) return n < 16 ? D3D_OK : D3DERR_INVALIDCALL;
    record(SB_STREAM, n);
    set_ref(st.streams[n].vb, static_cast<VertexBuffer*>(vb));
    st.streams[n].offset = off;
    st.streams[n].stride = stride;
    return D3D_OK;
}
HRESULT Device::GetStreamSource(UINT n, IDirect3DVertexBuffer9** vb, UINT* off, UINT* stride)
{
    if (n >= MAX_STREAMS || !vb) return D3DERR_INVALIDCALL;
    *vb = st.streams[n].vb;
    safe_addref(st.streams[n].vb);
    if (off) *off = st.streams[n].offset;
    if (stride) *stride = st.streams[n].stride;
    return D3D_OK;
}
HRESULT Device::SetStreamSourceFreq(UINT n, UINT s)
{
    if (n >= MAX_STREAMS) return D3DERR_INVALIDCALL;
    st.streams[n].freq = s;
    return D3D_OK;
}
HRESULT Device::GetStreamSourceFreq(UINT n, UINT* s)
{
    if (n >= MAX_STREAMS || !s) return D3DERR_INVALIDCALL;
    *s = st.streams[n].freq;
    return D3D_OK;
}
HRESULT Device::SetIndices(IDirect3DIndexBuffer9* ib)
{
    record(SB_INDICES, 0);
    set_ref(st.indices, static_cast<IndexBuffer*>(ib));
    return D3D_OK;
}
HRESULT Device::GetIndices(IDirect3DIndexBuffer9** ib)
{
    if (!ib) return D3DERR_INVALIDCALL;
    *ib = st.indices;
    safe_addref(st.indices);
    return D3D_OK;
}

// ---- shaders ----
HRESULT Device::CreateVertexShader(CONST DWORD*, IDirect3DVertexShader9** out)
{
    // src/ creates no vertex shader from bytecode (its vertex programs come from the effect)
    if (out) *out = nullptr;
    return D3DERR_INVALIDCALL;
}
HRESULT Device::SetVertexShader(IDirect3DVertexShader9* s)
{
    record(SB_VS, 0);
    set_ref(st.vs, static_cast<VShader*>(s));
    return D3D_OK;
}
HRESULT Device::GetVertexShader(IDirect3DVertexShader9** out)
{
    if (!out) return D3DERR_INVALIDCALL;
    *out = st.vs;
    safe_addref(st.vs);
    return D3D_OK;
}
HRESULT Device::CreatePixelShader(CONST DWORD* fn, IDirect3DPixelShader9** out)
{
    if (!fn || !out) return D3DERR_INVALIDCALL;
    *out = nullptr;
    std::string hlsl, err;
    uint32_t inputs = 0;
    if (!translate_ps_tokens(fn, hlsl, err, &inputs)) {
        log("CreatePixelShader: %s", err.c_str());
        return D3DERR_INVALIDCALL;
    }
    PShader* ps = new PShader();
    ps->dev = this;
    ps->inputs = inputs;
    const DWORD* p = fn;
    while (*p != 0x0000ffff) p++;
    ps->tokens.assign(fn, p + 1);
    if (!compile_shader(gpu, hlsl, "mbp_ps", Stage::Fragment, ps->cs)) {
        log("CreatePixelShader compile: %s\n%s", ps->cs.error.c_str(), hlsl.c_str());
        ps->Release();
        return D3DERR_INVALIDCALL;
    }
    ps->stage_of_binding = ps->cs.refl.sampler_global;
    ps->ubo0 = &ps_consts;
    *out = ps;
    return D3D_OK;
}
HRESULT Device::SetPixelShader(IDirect3DPixelShader9* s)
{
    record(SB_PS, 0);
    set_ref(st.ps, static_cast<PShader*>(s));
    return D3D_OK;
}
HRESULT Device::GetPixelShader(IDirect3DPixelShader9** out)
{
    if (!out) return D3DERR_INVALIDCALL;
    *out = st.ps;
    safe_addref(st.ps);
    return D3D_OK;
}

#define CONST_IMPL(Name, arr, type, count)                                                              \
    HRESULT Device::Set##Name(UINT start, CONST type* p, UINT n)                                       \
    {                                                                                                   \
        if (!p || start + n > count) return D3DERR_INVALIDCALL;                                         \
        memcpy(&arr[start], p, n * sizeof(arr[0]));                                                     \
        return D3D_OK;                                                                                  \
    }                                                                                                   \
    HRESULT Device::Get##Name(UINT start, type* p, UINT n)                                             \
    {                                                                                                   \
        if (!p || start + n > count) return D3DERR_INVALIDCALL;                                         \
        memcpy(p, &arr[start], n * sizeof(arr[0]));                                                     \
        return D3D_OK;                                                                                  \
    }
CONST_IMPL(VertexShaderConstantF, st.vsc, float, 256)
CONST_IMPL(VertexShaderConstantI, st.vsi, int, 16)
CONST_IMPL(VertexShaderConstantB, st.vsb, BOOL, 16)
CONST_IMPL(PixelShaderConstantI, st.psi, int, 16)
CONST_IMPL(PixelShaderConstantB, st.psb, BOOL, 16)
#undef CONST_IMPL
HRESULT Device::SetPixelShaderConstantF(UINT start, CONST float* p, UINT n)
{
    if (!p || start + n > 224) return D3DERR_INVALIDCALL;
    record(SB_PSC, start, n);
    memcpy(&st.psc[start], p, n * 16);
    ps_consts.version = ++ps_consts_ver;
    return D3D_OK;
}
HRESULT Device::GetPixelShaderConstantF(UINT start, float* p, UINT n)
{
    if (!p || start + n > 224) return D3DERR_INVALIDCALL;
    memcpy(p, &st.psc[start], n * 16);
    return D3D_OK;
}

HRESULT Device::CreateQuery(D3DQUERYTYPE t, IDirect3DQuery9** out)
{
    if (t != D3DQUERYTYPE_EVENT && t != D3DQUERYTYPE_OCCLUSION) return D3DERR_NOTAVAILABLE;
    if (!out) return D3D_OK;     // support probe
    Query* q = new Query();
    q->dev = this;
    q->type = t;
    *out = q;
    return D3D_OK;
}

// ---- state blocks ----
StateBlock::~StateBlock()
{
    if (!st) return;
    for (auto& t : st->tex) safe_release(t);
    for (auto& s : st->streams) safe_release(s.vb);
    safe_release(st->indices);
    safe_release(st->decl);
    safe_release(st->vs);
    safe_release(st->ps);
}
HRESULT StateBlock::GetDevice(IDirect3DDevice9** pp) { if (!pp) return D3DERR_INVALIDCALL; *pp = dev; dev->AddRef(); return D3D_OK; }

static void sb_all_items(std::vector<SbItem>& v, D3DSTATEBLOCKTYPE type, const DevState& st)
{
    bool pix = type == D3DSBT_ALL || type == D3DSBT_PIXELSTATE;
    bool vert = type == D3DSBT_ALL || type == D3DSBT_VERTEXSTATE;
    for (DWORD i = 0; i < 256; i++) v.push_back({SB_RS, i, 0});
    for (DWORD s = 0; s < MAX_STAGES; s++)
        for (DWORD t = 1; t < 33; t++) v.push_back({SB_TSS, s, t});
    for (DWORD s = 0; s < MAX_SAMPLERS; s++)
        for (DWORD t = 1; t < 14; t++) v.push_back({SB_SS, s, t});
    if (pix) v.push_back({SB_PS, 0, 0});
    if (vert) {
        v.push_back({SB_VS, 0, 0});
        v.push_back({SB_DECL, 0, 0});
        for (auto& kv : st.lights) { v.push_back({SB_LIGHT, kv.first, 0}); v.push_back({SB_LIGHTEN, kv.first, 0}); }
    }
    if (type == D3DSBT_ALL) {
        for (DWORD s = 0; s < MAX_SAMPLERS; s++) v.push_back({SB_TEX, s, 0});
        DWORD xf[] = {2, 3, 16, 17, 18, 19, 20, 21, 22, 23, 256, 257, 258, 259};
        for (DWORD x : xf) v.push_back({SB_XFORM, x, 0});
        v.push_back({SB_MATERIAL, 0, 0});
        v.push_back({SB_VP, 0, 0});
        v.push_back({SB_SCISSOR, 0, 0});
        for (DWORD s = 0; s < MAX_STREAMS; s++) v.push_back({SB_STREAM, s, 0});
        v.push_back({SB_INDICES, 0, 0});
        for (DWORD i = 0; i < 6; i++) v.push_back({SB_CLIP, i, 0});
    }
}

HRESULT Device::CreateStateBlock(D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9** out)
{
    if (!out) return D3DERR_INVALIDCALL;
    StateBlock* sb = new StateBlock();
    sb->dev = this;
    sb_all_items(sb->items, type, st);
    sb->Capture();
    *out = sb;
    return D3D_OK;
}
HRESULT Device::BeginStateBlock()
{
    if (recording) return D3DERR_INVALIDCALL;
    // D3D9 records without applying: snapshot the whole state, let the recorded calls run, then capture what they set
    // and put the snapshot back (EndStateBlock)
    IDirect3DStateBlock9* pre = nullptr;
    CreateStateBlock(D3DSBT_ALL, &pre);
    rec_pre = static_cast<StateBlock*>(pre);
    rec = new StateBlock();
    rec->dev = this;
    recording = true;
    return D3D_OK;
}
HRESULT Device::EndStateBlock(IDirect3DStateBlock9** out)
{
    if (!recording || !out) return D3DERR_INVALIDCALL;
    recording = false;
    StateBlock* sb = rec;
    rec = nullptr;
    // D3D9 records the values set while recording; the device state kept them too, so capture now
    std::sort(sb->items.begin(), sb->items.end(), [](const SbItem& a, const SbItem& b) {
        if (a.kind != b.kind) return a.kind < b.kind;
        if (a.a != b.a) return a.a < b.a;
        return a.b < b.b;
    });
    sb->items.erase(std::unique(sb->items.begin(), sb->items.end(), [](const SbItem& a, const SbItem& b) {
        return a.kind == b.kind && a.a == b.a && a.b == b.b;
    }), sb->items.end());
    sb->Capture();
    if (rec_pre) {
        rec_pre->Apply();
        rec_pre->Release();
        rec_pre = nullptr;
    }
    *out = sb;
    return D3D_OK;
}

HRESULT StateBlock::Capture()
{
    if (!st) {
        st.reset(new DevState());
    }
    DevState& d = *st;
    const DevState& s = dev->st;
    for (const SbItem& it : items) {
        switch (it.kind) {
        case SB_RS: d.rs[it.a] = s.rs[it.a]; break;
        case SB_TSS: d.tss[it.a][it.b] = s.tss[it.a][it.b]; break;
        case SB_SS: d.ss[it.a][it.b] = s.ss[it.a][it.b]; break;
        case SB_TEX: set_ref(d.tex[it.a], s.tex[it.a]); break;
        case SB_XFORM: dev->GetTransform((D3DTRANSFORMSTATETYPE)it.a, it.a == 2 ? &d.view : it.a == 3 ? &d.proj : it.a < 256 ? &d.texm[it.a - 16] : &d.world[it.a - 256]); break;
        case SB_MATERIAL: d.material = s.material; break;
        case SB_LIGHT: case SB_LIGHTEN: { auto f = s.lights.find(it.a); if (f != s.lights.end()) d.lights[it.a] = f->second; break; }
        case SB_CLIP: memcpy(d.clip[it.a], s.clip[it.a], 16); break;
        case SB_VP: d.vp = s.vp; break;
        case SB_SCISSOR: d.scissor = s.scissor; break;
        case SB_DECL: case SB_FVF: set_ref(d.decl, s.decl); d.fvf = s.fvf; break;
        case SB_STREAM: set_ref(d.streams[it.a].vb, s.streams[it.a].vb); d.streams[it.a].offset = s.streams[it.a].offset; d.streams[it.a].stride = s.streams[it.a].stride; break;
        case SB_INDICES: set_ref(d.indices, s.indices); break;
        case SB_VS: set_ref(d.vs, s.vs); break;
        case SB_PS: set_ref(d.ps, s.ps); break;
        case SB_PSC: memcpy(d.psc[it.a], s.psc[it.a], it.b * 16); break;
        }
    }
    return D3D_OK;
}

HRESULT StateBlock::Apply()
{
    if (!st) return D3D_OK;
    DevState& d = *st;
    Device* v = dev;
    for (const SbItem& it : items) {
        switch (it.kind) {
        case SB_RS: v->SetRenderState((D3DRENDERSTATETYPE)it.a, d.rs[it.a]); break;
        case SB_TSS: v->SetTextureStageState(it.a, (D3DTEXTURESTAGESTATETYPE)it.b, d.tss[it.a][it.b]); break;
        case SB_SS: v->SetSamplerState(it.a, (D3DSAMPLERSTATETYPE)it.b, d.ss[it.a][it.b]); break;
        case SB_TEX: v->SetTexture(it.a, d.tex[it.a]); break;
        case SB_XFORM: v->SetTransform((D3DTRANSFORMSTATETYPE)it.a, it.a == 2 ? &d.view : it.a == 3 ? &d.proj : it.a < 256 ? &d.texm[it.a - 16] : &d.world[it.a - 256]); break;
        case SB_MATERIAL: v->SetMaterial(&d.material); break;
        case SB_LIGHT: { auto f = d.lights.find(it.a); if (f != d.lights.end()) v->SetLight(it.a, &f->second.l); break; }
        case SB_LIGHTEN: { auto f = d.lights.find(it.a); if (f != d.lights.end()) v->LightEnable(it.a, f->second.enabled); break; }
        case SB_CLIP: v->SetClipPlane(it.a, d.clip[it.a]); break;
        case SB_VP: v->SetViewport(&d.vp); break;
        case SB_SCISSOR: v->SetScissorRect(&d.scissor); break;
        case SB_DECL: case SB_FVF: v->SetVertexDeclaration(d.decl); break;
        case SB_STREAM: v->SetStreamSource(it.a, d.streams[it.a].vb, d.streams[it.a].offset, d.streams[it.a].stride); break;
        case SB_INDICES: v->SetIndices(d.indices); break;
        case SB_VS: v->SetVertexShader(d.vs); break;
        case SB_PS: v->SetPixelShader(d.ps); break;
        case SB_PSC: v->SetPixelShaderConstantF(it.a, d.psc[it.a], it.b); break;
        }
    }
    return D3D_OK;
}

} // namespace mbg
