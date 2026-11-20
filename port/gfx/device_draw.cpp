// port/gfx/device_draw.cpp: the draw path of the D3D9 device over SDL_GPU: batches, render passes, uploads, readback,
// pipeline / sampler caches, Clear and the Draw* calls.
#include "gfx_internal.h"

namespace mbg {

static inline float fval(DWORD d) { float f; memcpy(&f, &d, 4); return f; }

// ---- batch machinery ----
void Device::ensure_cmd()
{
    if (!cmd) {
        cmd = SDL_AcquireGPUCommandBuffer(gpu);
        memset(pushed_src, 0, sizeof pushed_src);
        memset(pushed_ver, 0, sizeof pushed_ver);
    }
}

void Device::end_pass()
{
    if (pass) {
        SDL_EndGPURenderPass(pass);
        pass = nullptr;
        bound_pipe = nullptr;
    }
    pass_targets_dirty = true;
}

SDL_GPUBuffer* Device::create_buffer(uint32_t size, bool index)
{
    SDL_GPUBufferCreateInfo ci{};
    ci.usage = index ? SDL_GPU_BUFFERUSAGE_INDEX : SDL_GPU_BUFFERUSAGE_VERTEX;
    ci.size = (size + 3) & ~3u;
    SDL_GPUBuffer* b = SDL_CreateGPUBuffer(gpu, &ci);
    if (!b) log("SDL_CreateGPUBuffer(%u): %s", size, SDL_GetError());
    return b;
}

uint32_t Device::stage_bytes(const void* data, uint32_t size, uint32_t align)
{
    size_t off = (staging.size() + align - 1) & ~(size_t)(align - 1);
    staging.resize(off + size);
    if (data) memcpy(staging.data() + off, data, size);
    return (uint32_t)off;
}

void Device::queue_buffer_upload(SDL_GPUBuffer* buf, uint32_t dst_off, const void* data, uint32_t size)
{
    // SDL requires 4-byte aligned buffer copies: widen the range (the shadow / source covers it)
    uint32_t a0 = dst_off & ~3u;
    uint32_t a1 = (dst_off + size + 3) & ~3u;
    PendingUpload u{};
    u.is_tex = false;
    u.buf = buf;
    u.buf_off = a0;
    u.size = a1 - a0;
    u.staging_off = stage_bytes(nullptr, u.size, 16);
    memset(staging.data() + u.staging_off, 0, u.size);
    memcpy(staging.data() + u.staging_off + (dst_off - a0), data, size);
    uploads.push_back(u);
}

void Device::queue_texture_upload(const std::shared_ptr<GpuImage>& img, UINT level, UINT face)
{
    if (!img->tex) return;
    if (img->used_batch == batch) flush(false);    // a recorded draw samples the old contents
    std::vector<uint8_t>& sh = img->level_shadow(level, face);
    UINT w = img->level_w(level), h = img->level_h(level);
    std::vector<uint8_t> conv;
    const uint8_t* data = sh.data();
    uint32_t size = (uint32_t)sh.size();
    bool block = format_is_block(img->format) && !img->cpu_convert;
    const FormatInfo* fi = format_info(img->format);
    if (img->cpu_convert || fi->convert) {
        convert_to_gpu(img->format, img->gpu_format, sh.data(), w, h, conv);
        data = conv.data();
        size = (uint32_t)conv.size();
    }
    PendingUpload u{};
    u.is_tex = true;
    u.img = img;
    u.level = level;
    u.face = face;
    u.w = w;
    u.h = h;
    u.row_pixels = block ? (w + 3) & ~3u : w;
    u.rows = block ? (h + 3) & ~3u : h;
    u.size = size;
    u.staging_off = stage_bytes(data, size, 16);
    uploads.push_back(u);
}

void Device::flush(bool wait)
{
    end_pass();
    if (!uploads.empty()) {
        SDL_GPUCommandBuffer* uc = SDL_AcquireGPUCommandBuffer(gpu);
        SDL_GPUTransferBufferCreateInfo ti{};
        ti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        ti.size = (Uint32)std::max<size_t>(staging.size(), 16);
        SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(gpu, &ti);
        void* p = tb ? SDL_MapGPUTransferBuffer(gpu, tb, false) : nullptr;
        if (p) {
            memcpy(p, staging.data(), staging.size());
            SDL_UnmapGPUTransferBuffer(gpu, tb);
            SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(uc);
            for (const PendingUpload& u : uploads) {
                if (u.is_tex) {
                    SDL_GPUTextureTransferInfo src{};
                    src.transfer_buffer = tb;
                    src.offset = u.staging_off;
                    src.pixels_per_row = u.row_pixels;
                    src.rows_per_layer = u.rows;
                    SDL_GPUTextureRegion dst{};
                    dst.texture = u.img ? u.img->tex : u.raw_tex;
                    dst.mip_level = u.level;
                    dst.layer = u.face;
                    dst.x = u.x;
                    dst.y = u.y;
                    dst.w = u.w;
                    dst.h = u.h;
                    dst.d = 1;
                    if (dst.texture) SDL_UploadToGPUTexture(cp, &src, &dst, false);
                } else {
                    SDL_GPUTransferBufferLocation src{tb, u.staging_off};
                    SDL_GPUBufferRegion dst{u.buf, u.buf_off, u.size};
                    SDL_UploadToGPUBuffer(cp, &src, &dst, false);
                }
            }
            SDL_EndGPUCopyPass(cp);
        } else {
            log("upload staging failed: %s", SDL_GetError());
        }
        SDL_SubmitGPUCommandBuffer(uc);
        if (tb) SDL_ReleaseGPUTransferBuffer(gpu, tb);
    }
    if (cmd) {
        if (wait) {
            SDL_GPUFence* f = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
            if (f) {
                SDL_WaitForGPUFences(gpu, true, &f, 1);
                SDL_ReleaseGPUFence(gpu, f);
            }
        } else {
            SDL_SubmitGPUCommandBuffer(cmd);
        }
        cmd = nullptr;
    } else if (wait && !uploads.empty()) {
        SDL_WaitForGPUIdle(gpu);
    }
    for (SDL_GPUBuffer* b : retired) SDL_ReleaseGPUBuffer(gpu, b);
    retired.clear();
    if (up_buf) SDL_ReleaseGPUBuffer(gpu, up_buf);
    up_buf = nullptr;
    up_buf_size = up_buf_used = 0;
    uploads.clear();
    staging.clear();
    batch++;
    pass_targets_dirty = true;
}

bool Device::readback(GpuImage* img, UINT level, std::vector<uint8_t>& out)
{
    if (!img || !img->tex || format_is_block(img->format)) return false;
    end_pass();
    ensure_cmd();
    UINT w = img->level_w(level), h = img->level_h(level);
    Uint32 bpp = SDL_GPUTextureFormatTexelBlockSize(img->gpu_format);
    SDL_GPUTransferBufferCreateInfo ti{};
    ti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    ti.size = w * h * bpp;
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(gpu, &ti);
    if (!tb) return false;
    SDL_GPUCopyPass* cp = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureRegion src{};
    src.texture = img->tex;
    src.mip_level = level;
    src.w = w;
    src.h = h;
    src.d = 1;
    SDL_GPUTextureTransferInfo dst{};
    dst.transfer_buffer = tb;
    dst.pixels_per_row = w;
    dst.rows_per_layer = h;
    SDL_DownloadFromGPUTexture(cp, &src, &dst);
    SDL_EndGPUCopyPass(cp);
    flush(true);
    void* p = SDL_MapGPUTransferBuffer(gpu, tb, false);
    bool ok = p != nullptr;
    if (ok) {
        out.assign((uint8_t*)p, (uint8_t*)p + ti.size);
        SDL_UnmapGPUTransferBuffer(gpu, tb);
    }
    SDL_ReleaseGPUTransferBuffer(gpu, tb);
    img->gpu_newer = false;
    return ok;
}

// ---- passes ----
SDL_GPUTextureFormat Device::target_color_format() const
{
    return rt[0] ? rt[0]->img->gpu_format : SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
}

bool Device::begin_pass(bool clear_color, D3DCOLOR color, bool clear_depth, float z, bool clear_stencil, DWORD stencil)
{
    end_pass();
    ensure_cmd();
    if (!rt[0] || !rt[0]->img->tex) return false;
    GpuImage* ci = rt[0]->img.get();
    SDL_GPUColorTargetInfo ct{};
    ct.texture = ci->msaa ? ci->msaa : ci->tex;
    ct.mip_level = ci->msaa ? 0 : rt[0]->level;
    ct.clear_color = SDL_FColor{((color >> 16) & 255) / 255.0f, ((color >> 8) & 255) / 255.0f, (color & 255) / 255.0f, (color >> 24) / 255.0f};
    ct.load_op = clear_color ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    if (ci->msaa) {
        ct.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
        ct.resolve_texture = ci->tex;
        ct.resolve_mip_level = rt[0]->level;
    } else {
        ct.store_op = SDL_GPU_STOREOP_STORE;
    }
    SDL_GPUDepthStencilTargetInfo dt{};
    bool has_ds = ds && ds->img->tex;
    if (has_ds) {
        GpuImage* di = ds->img.get();
        if ((di->msaa != nullptr) != (ci->msaa != nullptr) || di->level_w(ds->level) < rt[0]->width() || di->level_h(ds->level) < rt[0]->height()) {
            has_ds = false;    // D3D9 would refuse to draw with this pair; SDL would fault
        } else {
            dt.texture = di->msaa ? di->msaa : di->tex;
            dt.clear_depth = z;
            dt.load_op = clear_depth ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            dt.store_op = SDL_GPU_STOREOP_STORE;
            dt.stencil_load_op = clear_stencil ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
            dt.stencil_store_op = SDL_GPU_STOREOP_STORE;
            dt.clear_stencil = (Uint8)stencil;
            di->used_batch = batch;
            di->gpu_newer = true;
        }
    }
    pass = SDL_BeginGPURenderPass(cmd, &ct, 1, has_ds ? &dt : nullptr);
    if (!pass) return false;
    ci->used_batch = batch;
    ci->gpu_newer = true;
    pass_rt = rt[0];
    pass_ds = has_ds ? ds : nullptr;
    pass_targets_dirty = false;
    bound_pipe = nullptr;
    vp_dirty = scissor_dirty = true;
    return true;
}

static bool full_rect(const RECT& r, UINT w, UINT h) { return r.left <= 0 && r.top <= 0 && r.right >= (LONG)w && r.bottom >= (LONG)h; }

HRESULT Device::Clear(DWORD count, CONST D3DRECT* rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
    if (!rt[0]) return D3DERR_INVALIDCALL;
    if (!ds) flags &= ~(DWORD)(D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL);
    if (!flags) return D3D_OK;
    UINT w = rt[0]->width(), h = rt[0]->height();
    RECT area{(LONG)st.vp.X, (LONG)st.vp.Y, (LONG)(st.vp.X + st.vp.Width), (LONG)(st.vp.Y + st.vp.Height)};
    if (st.rs[D3DRS_SCISSORTESTENABLE]) {
        area.left = std::max(area.left, st.scissor.left);
        area.top = std::max(area.top, st.scissor.top);
        area.right = std::min(area.right, st.scissor.right);
        area.bottom = std::min(area.bottom, st.scissor.bottom);
    }
    std::vector<D3DRECT> list;
    if (count == 0 || !rects) list.push_back(D3DRECT{area.left, area.top, area.right, area.bottom});
    else
        for (DWORD i = 0; i < count; i++) {
            D3DRECT r = rects[i];
            r.x1 = std::max(r.x1, area.left); r.y1 = std::max(r.y1, area.top);
            r.x2 = std::min(r.x2, area.right); r.y2 = std::min(r.y2, area.bottom);
            if (r.x2 > r.x1 && r.y2 > r.y1) list.push_back(r);
        }
    if (list.empty()) return D3D_OK;
    bool depth_full = !ds || (ds->width() == w && ds->height() == h);
    if (list.size() == 1 && full_rect(RECT{list[0].x1, list[0].y1, list[0].x2, list[0].y2}, w, h) && depth_full) {
        // restart the pass with clear load ops (the cheapest clear on tile GPUs)
        if (!begin_pass(flags & D3DCLEAR_TARGET, color, flags & D3DCLEAR_ZBUFFER, z, flags & D3DCLEAR_STENCIL, stencil))
            return D3DERR_INVALIDCALL;
        return D3D_OK;
    }
    for (const D3DRECT& r : list) clear_quad(flags, color, z, stencil, &r);
    return D3D_OK;
}

// partial clears: a quad with a dedicated pipeline (depth ALWAYS + write, colour write), clipped by the scissor
void Device::clear_quad(DWORD flags, D3DCOLOR color, float z, DWORD stencil, const D3DRECT* r)
{
    if (pass_targets_dirty || !pass) {
        if (!begin_pass(false, 0, false, 1.0f, false, 0)) return;
    }
    static const char* src =
        "[[vk::binding(0,1)]] cbuffer ClearV { float4 cz; };\n"
        "[[vk::binding(0,3)]] cbuffer ClearP { float4 cc; };\n"
        "float4 mbp_vs(uint id : SV_VertexID) : SV_Position { float2 p = float2((id & 1) ? 1.0 : -1.0, (id & 2) ? -1.0 : 1.0); return float4(p, cz.x, 1); }\n"
        "float4 mbp_ps() : SV_Target0 { return cc; }\n";
    static CompiledShader vs, ps;
    if (!vs.shader && !compile_shader(gpu, src, "mbp_vs", Stage::Vertex, vs)) { log("clear vs: %s", vs.error.c_str()); return; }
    if (!ps.shader && !compile_shader(gpu, src, "mbp_ps", Stage::Fragment, ps)) { log("clear ps: %s", ps.error.c_str()); return; }
    PipeKey k{};
    k.vs = vs.shader;
    k.ps = ps.shader;
    k.layout = 0xc1ea7;
    k.prim = 5;
    k.colormask = (flags & D3DCLEAR_TARGET) ? 0xf : 0;
    k.zwrite = (flags & D3DCLEAR_ZBUFFER) != 0;
    k.stencil = (flags & D3DCLEAR_STENCIL) != 0;
    k.has_depth = pass_ds != nullptr;
    k.color_fmt = target_color_format();
    k.depth_fmt = pass_ds ? pass_ds->img->gpu_format : 0;
    k.samples = (uint8_t)(rt[0]->img->msaa ? rt[0]->img->samples : 1);
    auto it = pipes.find(k);
    SDL_GPUGraphicsPipeline* p = nullptr;
    if (it != pipes.end()) p = it->second;
    else {
        SDL_GPUGraphicsPipelineCreateInfo ci{};
        ci.vertex_shader = vs.shader;
        ci.fragment_shader = ps.shader;
        ci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
        ci.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        ci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        ci.rasterizer_state.enable_depth_clip = false;
        ci.multisample_state.sample_count = k.samples >= 8 ? SDL_GPU_SAMPLECOUNT_8 : k.samples >= 4 ? SDL_GPU_SAMPLECOUNT_4 : k.samples >= 2 ? SDL_GPU_SAMPLECOUNT_2 : SDL_GPU_SAMPLECOUNT_1;
        SDL_GPUColorTargetDescription cd{};
        cd.format = (SDL_GPUTextureFormat)k.color_fmt;
        cd.blend_state.color_write_mask = k.colormask;
        cd.blend_state.enable_color_write_mask = true;
        ci.target_info.color_target_descriptions = &cd;
        ci.target_info.num_color_targets = 1;
        if (k.has_depth) {
            ci.target_info.has_depth_stencil_target = true;
            ci.target_info.depth_stencil_format = (SDL_GPUTextureFormat)k.depth_fmt;
            ci.depth_stencil_state.enable_depth_test = k.zwrite;
            ci.depth_stencil_state.enable_depth_write = k.zwrite;
            ci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_ALWAYS;
            if (k.stencil && gpu_has_stencil((SDL_GPUTextureFormat)k.depth_fmt)) {
                ci.depth_stencil_state.enable_stencil_test = true;
                SDL_GPUStencilOpState so{SDL_GPU_STENCILOP_REPLACE, SDL_GPU_STENCILOP_REPLACE, SDL_GPU_STENCILOP_REPLACE, SDL_GPU_COMPAREOP_ALWAYS};
                ci.depth_stencil_state.front_stencil_state = so;
                ci.depth_stencil_state.back_stencil_state = so;
                ci.depth_stencil_state.write_mask = 0xff;
                ci.depth_stencil_state.compare_mask = 0xff;
            }
        }
        p = SDL_CreateGPUGraphicsPipeline(gpu, &ci);
        if (!p) log("clear pipeline: %s", SDL_GetError());
        pipes[k] = p;
    }
    if (!p) return;
    SDL_BindGPUGraphicsPipeline(pass, p);
    bound_pipe = p;
    SDL_GPUViewport v{0, 0, (float)rt[0]->width(), (float)rt[0]->height(), 0.0f, 1.0f};
    SDL_SetGPUViewport(pass, &v);
    SDL_Rect sr{(int)r->x1, (int)r->y1, (int)(r->x2 - r->x1), (int)(r->y2 - r->y1)};
    SDL_SetGPUScissor(pass, &sr);
    vp_dirty = scissor_dirty = true;
    if (k.stencil) SDL_SetGPUStencilReference(pass, (Uint8)stencil);
    float czv[4] = {z, 0, 0, 0};
    float ccv[4] = {((color >> 16) & 255) / 255.0f, ((color >> 8) & 255) / 255.0f, (color & 255) / 255.0f, (color >> 24) / 255.0f};
    SDL_PushGPUVertexUniformData(cmd, 0, czv, sizeof czv);
    SDL_PushGPUFragmentUniformData(cmd, 0, ccv, sizeof ccv);
    pushed_src[0][0] = pushed_src[1][0] = nullptr;
    SDL_DrawGPUPrimitives(pass, 4, 1, 0, 0);
}

// ---- samplers / textures ----
SDL_GPUSampler* Device::sampler_for(DWORD stage)
{
    const DWORD* s = st.ss[stage];
    DWORD key[8] = {s[D3DSAMP_MINFILTER], s[D3DSAMP_MAGFILTER], s[D3DSAMP_MIPFILTER], s[D3DSAMP_ADDRESSU], s[D3DSAMP_ADDRESSV],
                    s[D3DSAMP_ADDRESSW], s[D3DSAMP_MAXANISOTROPY] | (s[D3DSAMP_MAXMIPLEVEL] << 8), s[D3DSAMP_MIPMAPLODBIAS]};
    uint64_t h = hash_bytes(key, sizeof key);
    auto it = samplers.find(h);
    if (it != samplers.end()) return it->second;
    auto addr = [](DWORD a) {
        switch (a) {
        case D3DTADDRESS_WRAP: return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        case D3DTADDRESS_MIRROR: return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
        default: return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        }
    };
    SDL_GPUSamplerCreateInfo ci{};
    bool aniso = key[0] == D3DTEXF_ANISOTROPIC || key[1] == D3DTEXF_ANISOTROPIC;
    ci.min_filter = key[0] == D3DTEXF_POINT || key[0] == D3DTEXF_NONE ? SDL_GPU_FILTER_NEAREST : SDL_GPU_FILTER_LINEAR;
    ci.mag_filter = key[1] == D3DTEXF_POINT || key[1] == D3DTEXF_NONE ? SDL_GPU_FILTER_NEAREST : SDL_GPU_FILTER_LINEAR;
    ci.mipmap_mode = key[2] == D3DTEXF_LINEAR || key[2] == D3DTEXF_ANISOTROPIC ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    ci.address_mode_u = addr(key[3]);
    ci.address_mode_v = addr(key[4]);
    ci.address_mode_w = addr(key[5]);
    ci.mip_lod_bias = fval(key[7]);
    ci.min_lod = (float)s[D3DSAMP_MAXMIPLEVEL];
    ci.max_lod = key[2] == D3DTEXF_NONE ? (float)s[D3DSAMP_MAXMIPLEVEL] : 1000.0f;
    if (aniso && s[D3DSAMP_MAXANISOTROPY] > 1) {
        ci.enable_anisotropy = true;
        ci.max_anisotropy = (float)std::min<DWORD>(16, s[D3DSAMP_MAXANISOTROPY]);
        ci.min_filter = ci.mag_filter = SDL_GPU_FILTER_LINEAR;
    }
    SDL_GPUSampler* smp = SDL_CreateGPUSampler(gpu, &ci);
    samplers[h] = smp;
    return smp;
}

SDL_GPUTexture* Device::texture_for(DWORD stage, bool* bound)
{
    IDirect3DBaseTexture9* bt = st.tex[stage];
    if (bound) *bound = false;
    if (!bt) return dummy_tex;
    Texture* t = static_cast<Texture*>(static_cast<IDirect3DTexture9*>(bt));
    if (!t->img || !t->img->tex) return dummy_tex;
    t->img->used_batch = batch;
    if (bound) *bound = true;
    return t->img->tex;
}

// ---- pipelines ----
static SDL_GPUVertexElementFormat vfmt(BYTE t)
{
    switch (t) {
    case D3DDECLTYPE_FLOAT1: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
    case D3DDECLTYPE_FLOAT2: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    case D3DDECLTYPE_FLOAT3: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    case D3DDECLTYPE_FLOAT4: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    case D3DDECLTYPE_D3DCOLOR: return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    case D3DDECLTYPE_UBYTE4: return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4;
    case D3DDECLTYPE_SHORT2: return SDL_GPU_VERTEXELEMENTFORMAT_SHORT2;
    case D3DDECLTYPE_SHORT4: return SDL_GPU_VERTEXELEMENTFORMAT_SHORT4;
    case D3DDECLTYPE_UBYTE4N: return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    case D3DDECLTYPE_SHORT2N: return SDL_GPU_VERTEXELEMENTFORMAT_SHORT2_NORM;
    case D3DDECLTYPE_SHORT4N: return SDL_GPU_VERTEXELEMENTFORMAT_SHORT4_NORM;
    case D3DDECLTYPE_USHORT2N: return SDL_GPU_VERTEXELEMENTFORMAT_USHORT2_NORM;
    case D3DDECLTYPE_USHORT4N: return SDL_GPU_VERTEXELEMENTFORMAT_USHORT4_NORM;
    case D3DDECLTYPE_FLOAT16_2: return SDL_GPU_VERTEXELEMENTFORMAT_HALF2;
    case D3DDECLTYPE_FLOAT16_4: return SDL_GPU_VERTEXELEMENTFORMAT_HALF4;
    default: return SDL_GPU_VERTEXELEMENTFORMAT_UINT;
    }
}

static SDL_GPUBlendFactor bfac(DWORD b)
{
    switch (b) {
    case D3DBLEND_ZERO: return SDL_GPU_BLENDFACTOR_ZERO;
    case D3DBLEND_ONE: return SDL_GPU_BLENDFACTOR_ONE;
    case D3DBLEND_SRCCOLOR: return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case D3DBLEND_INVSRCCOLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case D3DBLEND_SRCALPHA: return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case D3DBLEND_INVSRCALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case D3DBLEND_DESTALPHA: return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case D3DBLEND_INVDESTALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case D3DBLEND_DESTCOLOR: return SDL_GPU_BLENDFACTOR_DST_COLOR;
    case D3DBLEND_INVDESTCOLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
    case D3DBLEND_SRCALPHASAT: return SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE;
    case D3DBLEND_BLENDFACTOR: return SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
    case 15: return SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
    default: return SDL_GPU_BLENDFACTOR_ONE;
    }
}
static SDL_GPUBlendOp bop(DWORD o)
{
    switch (o) {
    case D3DBLENDOP_SUBTRACT: return SDL_GPU_BLENDOP_SUBTRACT;
    case D3DBLENDOP_REVSUBTRACT: return SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
    case D3DBLENDOP_MIN: return SDL_GPU_BLENDOP_MIN;
    case D3DBLENDOP_MAX: return SDL_GPU_BLENDOP_MAX;
    default: return SDL_GPU_BLENDOP_ADD;
    }
}
static SDL_GPUCompareOp cmpop(DWORD c)
{
    switch (c) {
    case D3DCMP_NEVER: return SDL_GPU_COMPAREOP_NEVER;
    case D3DCMP_LESS: return SDL_GPU_COMPAREOP_LESS;
    case D3DCMP_EQUAL: return SDL_GPU_COMPAREOP_EQUAL;
    case D3DCMP_LESSEQUAL: return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    case D3DCMP_GREATER: return SDL_GPU_COMPAREOP_GREATER;
    case D3DCMP_NOTEQUAL: return SDL_GPU_COMPAREOP_NOT_EQUAL;
    case D3DCMP_GREATEREQUAL: return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
    default: return SDL_GPU_COMPAREOP_ALWAYS;
    }
}
static SDL_GPUStencilOp sop(DWORD o)
{
    switch (o) {
    case D3DSTENCILOP_ZERO: return SDL_GPU_STENCILOP_ZERO;
    case D3DSTENCILOP_REPLACE: return SDL_GPU_STENCILOP_REPLACE;
    case D3DSTENCILOP_INCRSAT: return SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP;
    case D3DSTENCILOP_DECRSAT: return SDL_GPU_STENCILOP_DECREMENT_AND_CLAMP;
    case D3DSTENCILOP_INVERT: return SDL_GPU_STENCILOP_INVERT;
    case D3DSTENCILOP_INCR: return SDL_GPU_STENCILOP_INCREMENT_AND_WRAP;
    case D3DSTENCILOP_DECR: return SDL_GPU_STENCILOP_DECREMENT_AND_WRAP;
    default: return SDL_GPU_STENCILOP_KEEP;
    }
}

SDL_GPUGraphicsPipeline* Device::pipeline_for(const PipeKey& k, const VsVariant& vv, const VertexDecl*, const UINT* strides)
{
    auto it = pipes.find(k);
    if (it != pipes.end()) return it->second;
    PShader* ps = st.ps ? st.ps : ffp->ps();
    SDL_GPUGraphicsPipelineCreateInfo ci{};
    ci.vertex_shader = k.vs;
    ci.fragment_shader = k.ps;
    SDL_GPUVertexBufferDescription vbd[MAX_STREAMS];
    SDL_GPUVertexAttribute va[32];
    Uint32 nb = 0, na = 0;
    bool used[MAX_STREAMS] = {};
    for (const VsInput& in : vv.inputs) {
        if (in.stream >= MAX_STREAMS || na >= 32) continue;
        used[in.stream] = true;
        va[na].location = in.location;
        va[na].buffer_slot = in.stream;
        va[na].format = vfmt(in.type);
        va[na].offset = in.offset;
        na++;
    }
    for (UINT s = 0; s < MAX_STREAMS; s++)
        if (used[s]) {
            vbd[nb] = SDL_GPUVertexBufferDescription{s, strides[s], SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
            nb++;
        }
    ci.vertex_input_state.vertex_buffer_descriptions = vbd;
    ci.vertex_input_state.num_vertex_buffers = nb;
    ci.vertex_input_state.vertex_attributes = va;
    ci.vertex_input_state.num_vertex_attributes = na;
    switch (k.prim) {
    case D3DPT_LINELIST: ci.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST; break;
    case D3DPT_LINESTRIP: ci.primitive_type = SDL_GPU_PRIMITIVETYPE_LINESTRIP; break;
    case D3DPT_TRIANGLESTRIP: ci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP; break;
    case D3DPT_POINTLIST: ci.primitive_type = SDL_GPU_PRIMITIVETYPE_POINTLIST; break;
    default: ci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST; break;
    }
    ci.rasterizer_state.fill_mode = k.fill == D3DFILL_WIREFRAME ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
    // D3D9 culls by screen-space winding (y down): D3DCULL_CCW removes screen-counter-clockwise triangles. SDL_GPU's
    // winding is taken in its y-up NDC, where screen-CCW is clockwise: with front = CCW that is the FRONT face
    // (verified by mb_gfx_test's cull scenario)
    ci.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    ci.rasterizer_state.cull_mode = k.cull == D3DCULL_CCW ? SDL_GPU_CULLMODE_FRONT : k.cull == D3DCULL_CW ? SDL_GPU_CULLMODE_BACK : SDL_GPU_CULLMODE_NONE;
    if (k.bias != 0.0f || k.slope != 0.0f) {
        ci.rasterizer_state.enable_depth_bias = true;
        float units = k.depth_fmt == SDL_GPU_TEXTUREFORMAT_D16_UNORM ? 65535.0f : k.depth_fmt == SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT ? 16777215.0f : 8388608.0f;
        ci.rasterizer_state.depth_bias_constant_factor = k.bias * units;
        ci.rasterizer_state.depth_bias_slope_factor = k.slope;
    }
    ci.rasterizer_state.enable_depth_clip = true;
    ci.multisample_state.sample_count = k.samples >= 8 ? SDL_GPU_SAMPLECOUNT_8 : k.samples >= 4 ? SDL_GPU_SAMPLECOUNT_4 : k.samples >= 2 ? SDL_GPU_SAMPLECOUNT_2 : SDL_GPU_SAMPLECOUNT_1;
    SDL_GPUColorTargetDescription cd{};
    cd.format = (SDL_GPUTextureFormat)k.color_fmt;
    cd.blend_state.enable_blend = k.blend != 0;
    cd.blend_state.src_color_blendfactor = bfac(k.src);
    cd.blend_state.dst_color_blendfactor = bfac(k.dst);
    cd.blend_state.color_blend_op = bop(k.op);
    if (k.sep) {
        cd.blend_state.src_alpha_blendfactor = bfac(k.srca);
        cd.blend_state.dst_alpha_blendfactor = bfac(k.dsta);
        cd.blend_state.alpha_blend_op = bop(k.opa);
    } else {
        cd.blend_state.src_alpha_blendfactor = cd.blend_state.src_color_blendfactor;
        cd.blend_state.dst_alpha_blendfactor = cd.blend_state.dst_color_blendfactor;
        cd.blend_state.alpha_blend_op = cd.blend_state.color_blend_op;
    }
    if (k.src == 12) { cd.blend_state.src_color_blendfactor = cd.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA; cd.blend_state.dst_color_blendfactor = cd.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; }
    if (k.src == 13) { cd.blend_state.src_color_blendfactor = cd.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA; cd.blend_state.dst_color_blendfactor = cd.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA; }
    cd.blend_state.color_write_mask = k.colormask;
    cd.blend_state.enable_color_write_mask = true;
    ci.target_info.color_target_descriptions = &cd;
    ci.target_info.num_color_targets = 1;
    if (k.has_depth) {
        ci.target_info.has_depth_stencil_target = true;
        ci.target_info.depth_stencil_format = (SDL_GPUTextureFormat)k.depth_fmt;
        ci.depth_stencil_state.enable_depth_test = k.ztest;
        ci.depth_stencil_state.enable_depth_write = k.ztest && k.zwrite;
        ci.depth_stencil_state.compare_op = cmpop(k.zfunc);
        if (k.stencil) {
            ci.depth_stencil_state.enable_stencil_test = true;
            SDL_GPUStencilOpState so{sop(k.sfail), sop(k.spass), sop(k.szfail), cmpop(k.sfunc)};
            ci.depth_stencil_state.front_stencil_state = so;
            ci.depth_stencil_state.back_stencil_state = so;
            ci.depth_stencil_state.compare_mask = k.smask;
            ci.depth_stencil_state.write_mask = k.swmask;
        }
    }
    SDL_GPUGraphicsPipeline* p = SDL_CreateGPUGraphicsPipeline(gpu, &ci);
    if (!p) log("SDL_CreateGPUGraphicsPipeline: %s (vs %p ps %p, %u attributes)", SDL_GetError(), (void*)k.vs, (void*)k.ps, na);
    (void)ps;
    pipes[k] = p;
    return p;
}

static inline void mul_plane_by_inverse_vp(const D3DMATRIX& view, const D3DMATRIX& proj, const float* plane, float* out)
{
    // a world-space plane in clip space: P' = inverse(V*P) * P (column form)
    D3DMATRIX vp, inv;
    mat_mul(vp, view, proj);
    if (!mat_inverse(inv, nullptr, vp)) { memcpy(out, plane, 16); return; }
    for (int i = 0; i < 4; i++) out[i] = inv.m[i][0] * plane[0] + inv.m[i][1] * plane[1] + inv.m[i][2] * plane[2] + inv.m[i][3] * plane[3];
}

bool Device::prepare_draw(D3DPRIMITIVETYPE prim, const VertexDecl* decl, SDL_GPUBuffer* up_vb, uint32_t up_off, uint32_t up_stride)
{
    if (!decl) return false;
    if (pass_targets_dirty || !pass) {
        if (!begin_pass(false, 0, false, 1.0f, false, 0)) return false;
    }
    VShader* vs = st.vs ? st.vs : ffp->vs();
    PShader* ps = st.ps ? st.ps : ffp->ps();
    VsVariant* vv = vs->variant(decl);
    if (!vv || !vv->ok || !ps->cs.shader) return false;
    if (vs->is_ffp || ps->is_ffp) ffp->update(decl);

    UINT strides[MAX_STREAMS] = {};
    for (UINT s = 0; s < MAX_STREAMS; s++) strides[s] = st.streams[s].stride;
    if (up_vb) strides[0] = up_stride;

    PipeKey k{};
    k.vs = vv->shader;
    k.ps = ps->cs.shader;
    uint64_t lh = hash_bytes(vv->inputs.data(), vv->inputs.size() * sizeof(VsInput));
    k.layout = hash_bytes(strides, sizeof strides, lh);
    k.prim = (uint8_t)prim;
    k.fill = (uint8_t)st.rs[D3DRS_FILLMODE];
    k.cull = (uint8_t)st.rs[D3DRS_CULLMODE];
    k.samples = (uint8_t)(rt[0]->img->msaa ? rt[0]->img->samples : 1);
    k.has_depth = pass_ds != nullptr;
    if (k.has_depth) {
        k.ztest = st.rs[D3DRS_ZENABLE] != 0;
        k.zwrite = st.rs[D3DRS_ZWRITEENABLE] != 0;
        k.zfunc = (uint8_t)st.rs[D3DRS_ZFUNC];
        k.depth_fmt = pass_ds->img->gpu_format;
        if (st.rs[D3DRS_STENCILENABLE] && gpu_has_stencil(pass_ds->img->gpu_format)) {
            k.stencil = 1;
            k.sfail = (uint8_t)st.rs[D3DRS_STENCILFAIL];
            k.szfail = (uint8_t)st.rs[D3DRS_STENCILZFAIL];
            k.spass = (uint8_t)st.rs[D3DRS_STENCILPASS];
            k.sfunc = (uint8_t)st.rs[D3DRS_STENCILFUNC];
            k.smask = (uint8_t)st.rs[D3DRS_STENCILMASK];
            k.swmask = (uint8_t)st.rs[D3DRS_STENCILWRITEMASK];
        }
        k.bias = fval(st.rs[D3DRS_DEPTHBIAS]);
        k.slope = fval(st.rs[D3DRS_SLOPESCALEDEPTHBIAS]);
    }
    k.colormask = (uint8_t)(st.rs[D3DRS_COLORWRITEENABLE] & 0xf);
    k.blend = st.rs[D3DRS_ALPHABLENDENABLE] != 0;
    if (k.blend) {
        k.src = (uint8_t)st.rs[D3DRS_SRCBLEND];
        k.dst = (uint8_t)st.rs[D3DRS_DESTBLEND];
        k.op = (uint8_t)st.rs[D3DRS_BLENDOP];
        k.sep = st.rs[D3DRS_SEPARATEALPHABLENDENABLE] != 0;
        if (k.sep) {
            k.srca = (uint8_t)st.rs[D3DRS_SRCBLENDALPHA];
            k.dsta = (uint8_t)st.rs[D3DRS_DESTBLENDALPHA];
            k.opa = (uint8_t)st.rs[D3DRS_BLENDOPALPHA];
        }
    }
    k.color_fmt = target_color_format();
    if (prim == D3DPT_TRIANGLEFAN) k.prim = D3DPT_TRIANGLELIST;
    SDL_GPUGraphicsPipeline* p = pipeline_for(k, *vv, decl, strides);
    if (!p) return false;
    if (p != bound_pipe) {
        SDL_BindGPUGraphicsPipeline(pass, p);
        bound_pipe = p;
    }
    if (k.stencil) SDL_SetGPUStencilReference(pass, (Uint8)st.rs[D3DRS_STENCILREF]);
    if (k.blend && (k.src == D3DBLEND_BLENDFACTOR || k.dst == D3DBLEND_BLENDFACTOR || k.src == 15 || k.dst == 15)) {
        DWORD c = st.rs[193];
        SDL_FColor bc{((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f, (c & 255) / 255.0f, (c >> 24) / 255.0f};
        SDL_SetGPUBlendConstants(pass, bc);
    }

    // viewport / scissor
    UINT tw = rt[0]->width(), th = rt[0]->height();
    if (vp_dirty) {
        SDL_GPUViewport v{(float)st.vp.X, (float)st.vp.Y, (float)st.vp.Width, (float)st.vp.Height, st.vp.MinZ, st.vp.MaxZ};
        if (v.x + v.w > tw) v.w = (float)tw - v.x;
        if (v.y + v.h > th) v.h = (float)th - v.y;
        if (v.w <= 0 || v.h <= 0) return false;
        SDL_SetGPUViewport(pass, &v);
        vp_dirty = false;
    }
    if (scissor_dirty) {
        SDL_Rect r{0, 0, (int)tw, (int)th};
        if (st.rs[D3DRS_SCISSORTESTENABLE]) {
            int x0 = std::max<LONG>(0, st.scissor.left), y0 = std::max<LONG>(0, st.scissor.top);
            int x1 = std::min<LONG>((LONG)tw, st.scissor.right), y1 = std::min<LONG>((LONG)th, st.scissor.bottom);
            r = SDL_Rect{x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
        }
        SDL_SetGPUScissor(pass, &r);
        scissor_dirty = false;
    }

    // vertex buffers
    SDL_GPUBufferBinding vb[MAX_STREAMS];
    bool used[MAX_STREAMS] = {};
    for (const VsInput& in : vv->inputs) if (in.stream < MAX_STREAMS) used[in.stream] = true;
    for (UINT s = 0; s < MAX_STREAMS; s++) {
        if (!used[s]) continue;
        if (s == 0 && up_vb) {
            vb[0] = SDL_GPUBufferBinding{up_vb, up_off};
        } else {
            VertexBuffer* b = st.streams[s].vb;
            if (!b || !b->b.gpu) return false;
            b->b.used_batch = batch;
            vb[s] = SDL_GPUBufferBinding{b->b.gpu, st.streams[s].offset};
        }
        SDL_BindGPUVertexBuffers(pass, s, &vb[s], 1);
    }

    // textures
    size_t ns = ps->cs.refl.samplers.size();
    if (ns) {
        SDL_GPUTextureSamplerBinding tsb[MAX_SAMPLERS];
        for (size_t i = 0; i < ns && i < MAX_SAMPLERS; i++) {
            int stage = i < ps->stage_of_binding.size() ? ps->stage_of_binding[i] : (int)i;
            if (stage < 0 || stage >= MAX_SAMPLERS) stage = 0;
            bool bound = false;
            tsb[i].texture = texture_for((DWORD)stage, &bound);
            tsb[i].sampler = sampler_for((DWORD)stage);
        }
        SDL_BindGPUFragmentSamplers(pass, 0, tsb, (Uint32)std::min<size_t>(ns, MAX_SAMPLERS));
    }

    // device uniform blocks
    VsDevBlock vd{};
    float vw = (float)std::max<DWORD>(1, st.vp.Width), vh = (float)std::max<DWORD>(1, st.vp.Height);
    vd.halfpix[0] = 1.0f / vw;
    vd.halfpix[1] = -1.0f / vh;
    vd.halfpix[2] = 2.0f / vw;
    vd.halfpix[3] = -2.0f / vh;
    vd.vp[0] = (float)st.vp.X;
    vd.vp[1] = (float)st.vp.Y;
    vd.vp[2] = st.vp.MinZ;
    vd.vp[3] = st.vp.MaxZ - st.vp.MinZ == 0.0f ? 1.0f : st.vp.MaxZ - st.vp.MinZ;
    if (st.rs[D3DRS_CLIPPLANEENABLE] & 1) {
        vd.flags[0] = 1.0f;
        if (vs->is_ffp) mul_plane_by_inverse_vp(st.view, st.proj, st.clip[0], vd.clip_plane);
        else memcpy(vd.clip_plane, st.clip[0], 16);
    }
    if (memcmp(&vd, &vsdev, sizeof vd) != 0) { vsdev = vd; vsdev_src.version = ++dev_ver; }
    PsDevBlock pd{};
    DWORD fc = st.rs[D3DRS_FOGCOLOR];
    pd.fog_color[0] = ((fc >> 16) & 255) / 255.0f;
    pd.fog_color[1] = ((fc >> 8) & 255) / 255.0f;
    pd.fog_color[2] = (fc & 255) / 255.0f;
    pd.fog_color[3] = 1.0f;
    pd.alpha[0] = st.rs[D3DRS_ALPHATESTENABLE] ? (float)st.rs[D3DRS_ALPHAFUNC] : 0.0f;
    pd.alpha[1] = (float)(st.rs[D3DRS_ALPHAREF] & 255);
    pd.alpha[2] = st.rs[D3DRS_FOGENABLE] ? 1.0f : 0.0f;
    if (memcmp(&pd, &psdev, sizeof pd) != 0) { psdev = pd; psdev_src.version = ++dev_ver; }

    auto push = [&](int stage, int slot, const UniformSource* u) {
        if (!u || !u->data || !u->size) return;
        if (pushed_src[stage][slot] == u->data && pushed_ver[stage][slot] == u->version) return;
        if (stage == 0) SDL_PushGPUVertexUniformData(cmd, (Uint32)slot, u->data, u->size);
        else SDL_PushGPUFragmentUniformData(cmd, (Uint32)slot, u->data, u->size);
        pushed_src[stage][slot] = u->data;
        pushed_ver[stage][slot] = u->version;
    };
    push(0, 0, vs->ubo0);
    push(0, 1, &vsdev_src);
    push(1, 0, ps->ubo0);
    push(1, 1, &psdev_src);
    return true;
}

static UINT prim_vertices(D3DPRIMITIVETYPE t, UINT n)
{
    switch (t) {
    case D3DPT_POINTLIST: return n;
    case D3DPT_LINELIST: return n * 2;
    case D3DPT_LINESTRIP: return n + 1;
    case D3DPT_TRIANGLELIST: return n * 3;
    case D3DPT_TRIANGLESTRIP: return n + 2;
    case D3DPT_TRIANGLEFAN: return n + 2;
    default: return 0;
    }
}

const VertexDecl* current_decl(Device* d) { return d->st.decl; }

// staging for per-draw data (UP draws, fan index lists): one vertex buffer and one index buffer per batch
static bool stream_alloc(Device* d, SDL_GPUBuffer*& buf, uint32_t& cap, uint32_t& used, uint32_t size, bool index, uint32_t& off)
{
    if (!buf || used + size > cap) {
        if (buf) d->retire_buffer(buf);
        cap = std::max<uint32_t>(4u << 20, (size + 0xffff) & ~0xffffu);
        buf = d->create_buffer(cap, index);
        used = 0;
        if (!buf) return false;
    }
    off = used;
    used = (used + size + 15) & ~15u;
    return true;
}

HRESULT Device::DrawPrimitive(D3DPRIMITIVETYPE t, UINT start, UINT count)
{
    UINT nv = prim_vertices(t, count);
    if (!nv) return D3D_OK;
    if (!prepare_draw(t, st.decl, nullptr, 0, 0)) return D3D_OK;
    if (t == D3DPT_TRIANGLEFAN) {
        std::vector<uint16_t> idx;
        for (UINT i = 1; i + 1 < nv; i++) { idx.push_back(0); idx.push_back((uint16_t)i); idx.push_back((uint16_t)(i + 1)); }
        SDL_GPUBuffer* buf = create_buffer((uint32_t)(idx.size() * 2), true);
        if (!buf) return D3D_OK;
        queue_buffer_upload(buf, 0, idx.data(), (uint32_t)(idx.size() * 2));
        retire_buffer(buf);
        SDL_GPUBufferBinding b{buf, 0};
        SDL_BindGPUIndexBuffer(pass, &b, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(pass, (Uint32)idx.size(), 1, 0, (Sint32)start, 0);
        return D3D_OK;
    }
    SDL_DrawGPUPrimitives(pass, nv, 1, start, 0);
    return D3D_OK;
}

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE t, INT base, UINT, UINT, UINT start, UINT count)
{
    UINT ni = prim_vertices(t, count);
    if (!ni || !st.indices || !st.indices->b.gpu) return D3D_OK;
    if (!prepare_draw(t, st.decl, nullptr, 0, 0)) return D3D_OK;
    IndexBuffer* ib = st.indices;
    bool i32 = ib->format == D3DFMT_INDEX32;
    if (t == D3DPT_TRIANGLEFAN) {
        std::vector<uint32_t> idx;
        auto at = [&](UINT i) -> uint32_t {
            if (i32) { uint32_t v; memcpy(&v, &ib->b.shadow[(start + i) * 4], 4); return v; }
            uint16_t v; memcpy(&v, &ib->b.shadow[(start + i) * 2], 2); return v;
        };
        for (UINT i = 1; i + 1 < ni; i++) { idx.push_back(at(0)); idx.push_back(at(i)); idx.push_back(at(i + 1)); }
        SDL_GPUBuffer* buf = create_buffer((uint32_t)(idx.size() * 4), true);
        if (!buf) return D3D_OK;
        queue_buffer_upload(buf, 0, idx.data(), (uint32_t)(idx.size() * 4));
        retire_buffer(buf);
        SDL_GPUBufferBinding b{buf, 0};
        SDL_BindGPUIndexBuffer(pass, &b, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(pass, (Uint32)idx.size(), 1, 0, base, 0);
        return D3D_OK;
    }
    ib->b.used_batch = batch;
    SDL_GPUBufferBinding b{ib->b.gpu, 0};
    SDL_BindGPUIndexBuffer(pass, &b, i32 ? SDL_GPU_INDEXELEMENTSIZE_32BIT : SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, ni, 1, start, base, 0);
    return D3D_OK;
}

HRESULT Device::DrawPrimitiveUP(D3DPRIMITIVETYPE t, UINT count, CONST void* data, UINT stride)
{
    UINT nv = prim_vertices(t, count);
    if (!nv || !data || !stride) return D3D_OK;
    uint32_t size = nv * stride, off = 0;
    if (!stream_alloc(this, up_buf, up_buf_size, up_buf_used, size, false, off)) return D3D_OK;
    queue_buffer_upload(up_buf, off, data, size);
    // D3D9: DrawPrimitiveUP unbinds stream 0
    set_ref(st.streams[0].vb, (VertexBuffer*)nullptr);
    st.streams[0].offset = 0;
    st.streams[0].stride = 0;
    if (!prepare_draw(t, st.decl, up_buf, off, stride)) return D3D_OK;
    if (t == D3DPT_TRIANGLEFAN) {
        std::vector<uint16_t> idx;
        for (UINT i = 1; i + 1 < nv; i++) { idx.push_back(0); idx.push_back((uint16_t)i); idx.push_back((uint16_t)(i + 1)); }
        SDL_GPUBuffer* buf = create_buffer((uint32_t)(idx.size() * 2), true);
        if (!buf) return D3D_OK;
        queue_buffer_upload(buf, 0, idx.data(), (uint32_t)(idx.size() * 2));
        retire_buffer(buf);
        SDL_GPUBufferBinding b{buf, 0};
        SDL_BindGPUIndexBuffer(pass, &b, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(pass, (Uint32)idx.size(), 1, 0, 0, 0);
        return D3D_OK;
    }
    SDL_DrawGPUPrimitives(pass, nv, 1, 0, 0);
    return D3D_OK;
}

HRESULT Device::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE t, UINT minv, UINT numv, UINT count, CONST void* idata, D3DFORMAT ifmt,
                                       CONST void* vdata, UINT stride)
{
    UINT ni = prim_vertices(t, count);
    if (!ni || !idata || !vdata || !stride) return D3D_OK;
    bool i32 = ifmt == D3DFMT_INDEX32;
    uint32_t vsize = (minv + numv) * stride, voff = 0;
    if (!stream_alloc(this, up_buf, up_buf_size, up_buf_used, vsize, false, voff)) return D3D_OK;
    queue_buffer_upload(up_buf, voff, vdata, vsize);
    std::vector<uint32_t> fan;
    const void* ip = idata;
    uint32_t isize = ni * (i32 ? 4 : 2);
    if (t == D3DPT_TRIANGLEFAN) {
        auto at = [&](UINT i) -> uint32_t { return i32 ? ((const uint32_t*)idata)[i] : ((const uint16_t*)idata)[i]; };
        for (UINT i = 1; i + 1 < ni; i++) { fan.push_back(at(0)); fan.push_back(at(i)); fan.push_back(at(i + 1)); }
        ip = fan.data();
        i32 = true;
        isize = (uint32_t)(fan.size() * 4);
        ni = (UINT)fan.size();
    }
    SDL_GPUBuffer* ibuf = create_buffer(isize, true);
    if (!ibuf) return D3D_OK;
    queue_buffer_upload(ibuf, 0, ip, isize);
    retire_buffer(ibuf);
    set_ref(st.streams[0].vb, (VertexBuffer*)nullptr);
    st.streams[0].offset = 0;
    st.streams[0].stride = 0;
    set_ref(st.indices, (IndexBuffer*)nullptr);
    if (!prepare_draw(t, st.decl, up_buf, voff, stride)) return D3D_OK;
    SDL_GPUBufferBinding b{ibuf, 0};
    SDL_BindGPUIndexBuffer(pass, &b, i32 ? SDL_GPU_INDEXELEMENTSIZE_32BIT : SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(pass, ni, 1, 0, 0, 0);
    return D3D_OK;
}

} // namespace mbg
