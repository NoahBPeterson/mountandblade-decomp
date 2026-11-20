// port/gfx/gfx_internal.h: shared declarations of the gfx lane (Direct3D 9 + D3DX over SDL_GPU).
//
// Model: the D3D9 device is immediate; SDL_GPU records command buffers. Draws are recorded into the current "draw"
// command buffer as they happen (render passes opened lazily for the bound targets). Uploads (Lock/Unlock of buffers
// and textures, DrawPrimitiveUP data) are staged on the CPU and copied in one copy pass on a separate command buffer
// that is submitted BEFORE the draw command buffer when the batch is flushed (Present, readback, or a texture upload
// that a pending draw already sampled). Buffers keep D3D semantics by renaming their GPU storage when a lock would
// overwrite data a pending draw still reads (D3DLOCK_DISCARD, or a plain lock of a buffer used in the batch).
#pragma once

#include <SDL3/SDL.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "port_prelude.h"
#include <d3d9.h>
#include <d3dx9.h>
#undef min
#undef max
// the CRT shim maps these to the game's path layer (mb_fopen ...); this library resolves game paths itself
#undef fopen
#undef remove
#undef rename

namespace mbg {

// ---- counters (mbg_stat, for tests and diagnostics) ----
enum Stat { STAT_DRAWS, STAT_DRAWS_SKIPPED, STAT_PIPELINES, STAT_SHADERS, STAT_SHADER_ERRORS, STAT_UPLOAD_BYTES, STAT_COUNT };
extern uint64_t g_stats[STAT_COUNT];

// ---- logging ----
void log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
bool verbose();

// ---- COM helper: an object implementing interface I with a plain reference count ----
template<class I> struct Com : public I {
    std::atomic<ULONG> refs{1};
    virtual ~Com() {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void** out) override
    {
        if (out) { *out = static_cast<I*>(this); this->AddRef(); return S_OK; }
        return E_POINTER;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG r = --refs;
        if (r == 0) delete this;
        return r;
    }
};
template<class T> inline void safe_addref(T* p) { if (p) p->AddRef(); }
template<class T> inline void safe_release(T* p) { if (p) p->Release(); }
// replace a held reference (AddRef new, Release old)
template<class T> inline void set_ref(T*& slot, T* p) { if (slot == p) return; safe_addref(p); safe_release(slot); slot = p; }

// ---- formats ----
inline bool gpu_has_stencil(SDL_GPUTextureFormat f) { return f == SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT || f == SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT; }
struct FormatInfo {
    D3DFORMAT d3d;
    SDL_GPUTextureFormat gpu;      // format of the GPU texture (after conversion)
    int bpp;                       // D3D bytes per pixel (0 for block formats)
    int block;                     // bytes per 4x4 block for DXTn, else 0
    bool depth, stencil;
    bool convert;                  // the D3D layout differs from the GPU layout: converted on upload / download
};
const FormatInfo* format_info(D3DFORMAT f);
bool format_is_block(D3DFORMAT f);
UINT format_pitch(D3DFORMAT f, UINT w);            // bytes per row (DXT: per row of blocks)
UINT format_rows(D3DFORMAT f, UINT h);             // rows in memory (DXT: block rows)
UINT format_level_size(D3DFORMAT f, UINT w, UINT h);
// D3D-layout pixels -> GPU-layout pixels (w x h, src pitch = format_pitch). Returns false if unsupported.
bool convert_to_gpu(D3DFORMAT f, SDL_GPUTextureFormat gpu, const uint8_t* src, UINT w, UINT h, std::vector<uint8_t>& out);
bool convert_from_gpu(D3DFORMAT f, SDL_GPUTextureFormat gpu, const uint8_t* src, UINT w, UINT h, uint8_t* dst, UINT dst_pitch);
// any D3D format -> BGRA8 (A8R8G8B8 memory order) for screenshots, cursors and the CPU DXT fallback
bool decode_to_bgra(D3DFORMAT f, const uint8_t* src, UINT src_pitch, UINT w, UINT h, uint8_t* dst, UINT dst_pitch);
void decode_dxt_block(D3DFORMAT f, const uint8_t* blk, uint8_t out_bgra[16][4]);

class Device;

// ---- GPU texture storage shared by textures / surfaces ----
struct GpuImage {
    Device* dev = nullptr;
    SDL_GPUTexture* tex = nullptr;        // null for SYSTEMMEM / SCRATCH (CPU only)
    SDL_GPUTexture* msaa = nullptr;       // multisampled colour/depth target resolved into tex
    D3DFORMAT format = D3DFMT_UNKNOWN;
    SDL_GPUTextureFormat gpu_format = SDL_GPU_TEXTUREFORMAT_INVALID;
    bool cpu_convert = false;             // GPU format is BGRA8 decoded on the CPU (formats the GPU lacks)
    UINT width = 0, height = 0, levels = 1, faces = 1;
    DWORD usage = 0;
    D3DPOOL pool = D3DPOOL_DEFAULT;
    UINT samples = 1;
    bool is_depth = false;
    uint64_t used_batch = 0;              // last batch serial a draw sampled / rendered it
    bool gpu_newer = false;               // rendered to since the CPU shadow was filled
    std::vector<std::vector<uint8_t>> shadow; // per level*face, D3D layout
    std::vector<uint8_t> shadow_valid;
    bool autogen = false, need_mips = false;
    ~GpuImage();
    UINT level_w(UINT l) const { return std::max(1u, width >> l); }
    UINT level_h(UINT l) const { return std::max(1u, height >> l); }
    std::vector<uint8_t>& level_shadow(UINT l, UINT face = 0);
};

struct UniformSource {          // a block pushed to a shader uniform slot
    const void* data = nullptr;
    uint32_t size = 0;
    uint64_t version = 0;
};

// ---- shaders ----
enum class Stage { Vertex, Fragment };
struct UboMember {
    std::string name;
    uint32_t offset = 0, array_stride = 0, matrix_stride = 0, array_count = 0;
    uint32_t vecsize = 1, columns = 1;       // SPIR-V vector size / matrix column count
    bool spv_row_major = false;
    int base = 0;                            // 0 float, 1 int, 2 uint/bool
};
struct ShaderReflection {
    uint32_t ubo0_size = 0;                  // $Global / user block at binding 0
    std::vector<UboMember> ubo0;
    std::vector<std::string> samplers;       // sampler names by binding (compacted 0..n-1)
    std::vector<int> sampler_global;         // the binding each had before compaction
};
struct CompiledShader {
    SDL_GPUShader* shader = nullptr;
    ShaderReflection refl;
    std::string error;
};
// compile one HLSL entry point (glslang HLSL, DX9 compatibility) -> SPIR-V -> (Metal: MSL) -> SDL_GPUShader.
// Binding contract the generated sources follow:
//   vertex:   uniform blocks set 1 (binding 0 = user/$Global, 1 = device block), no samplers
//   fragment: uniform blocks set 3 (0 = user/$Global, 1 = device block), samplers set 2 (compacted to 0..n-1)
bool compile_shader(SDL_GPUDevice* gpu, const std::string& source, const char* entry, Stage stage, CompiledShader& out);
std::string shader_cache_dir();

struct VertexDecl;
struct DeclSig;          // a vertex layout signature (what the VS consumes)

// A vertex input the generated VS reads: location -> decl element
struct VsInput { uint32_t location; WORD stream; WORD offset; BYTE type; BYTE usage; BYTE usage_index; };
struct VsVariant {
    SDL_GPUShader* shader = nullptr;
    std::vector<VsInput> inputs;
    ShaderReflection refl;
    uint32_t outputs = 0;     // bitmask of varyings written (bit i = location i)
    bool ok = false;
};

// Base of every vertex shader object: effect passes, the fixed-function generator, CreateVertexShader
struct VShader : public Com<IDirect3DVertexShader9> {
    Device* dev = nullptr;
    UniformSource* ubo0 = nullptr;           // what slot 0 receives
    bool is_ffp = false;
    std::unordered_map<uint64_t, VsVariant> variants;     // by decl hash
    virtual VsVariant* variant(const VertexDecl* decl) = 0;
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    HRESULT STDMETHODCALLTYPE GetFunction(void*, UINT* sz) override { if (sz) *sz = 0; return D3DERR_INVALIDCALL; }
};
struct PShader : public Com<IDirect3DPixelShader9> {
    Device* dev = nullptr;
    UniformSource* ubo0 = nullptr;
    CompiledShader cs;
    std::vector<DWORD> tokens;               // bytecode (CreatePixelShader)
    uint32_t inputs = 0;                     // varyings read (bit = location)
    std::vector<int> stage_of_binding;       // sampler binding k reads device sampler stage stage_of_binding[k]
    bool is_ffp = false;
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    HRESULT STDMETHODCALLTYPE GetFunction(void* p, UINT* sz) override;
};

// varying locations shared by every generated shader
enum : uint32_t {
    VAR_COLOR0 = 0, VAR_COLOR1 = 1, VAR_TEX0 = 2,       // TEXCOORD0..7 = 2..9
    VAR_FOG = 10, VAR_CLIP = 11, VAR_COUNT = 12
};
int varying_for_semantic(const std::string& sem);       // -1 if not a varying (POSITION etc)

// Device uniform blocks (slot 1) of the generated shaders; layouts mirrored in the HLSL (gfx_shaders.cpp)
struct VsDevBlock {
    float halfpix[4];       // x = 1/vpW, y = -1/vpH (D3D9 pixel-centre convention), z = rhw: 2/vpW, w = -2/vpH
    float vp[4];            // viewport X, Y, MinZ, MaxZ-MinZ
    float clip_plane[4];
    float flags[4];         // x = clip plane enabled
};
struct PsDevBlock {
    float fog_color[4];
    float alpha[4];         // x = alpha test func (0 = off), y = ref (0..255), z = fog enabled
};

// ---- vertex declarations ----
struct VertexDecl : public Com<IDirect3DVertexDeclaration9> {
    Device* dev = nullptr;
    std::vector<D3DVERTEXELEMENT9> elems;    // without the end marker
    uint64_t hash = 0;
    DWORD fvf = 0;                           // the FVF it was built from (0 if from elements)
    bool rhw = false;                        // POSITIONT
    const D3DVERTEXELEMENT9* find(BYTE usage, BYTE index) const;
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    HRESULT STDMETHODCALLTYPE GetDeclaration(D3DVERTEXELEMENT9* p, UINT* n) override;
};
void decl_from_fvf(DWORD fvf, std::vector<D3DVERTEXELEMENT9>& out);
UINT fvf_stride(DWORD fvf);
uint64_t hash_bytes(const void* p, size_t n, uint64_t seed = 1469598103934665603ull);

// ---- resources ----
struct Resource {
    D3DPOOL pool = D3DPOOL_DEFAULT;
    DWORD priority = 0;
};

struct Texture;
struct Surface : public Com<IDirect3DSurface9> {
    Device* dev = nullptr;
    std::shared_ptr<GpuImage> img;
    UINT level = 0, face = 0;
    Texture* container = nullptr;            // ref counts forward to it when set
    D3DRESOURCETYPE type = D3DRTYPE_SURFACE;
    bool locked = false;
    RECT lock_rect{};
    bool lock_readonly = false;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    UINT width() const { return img->level_w(level); }
    UINT height() const { return img->level_h(level); }
    // IDirect3DResource9
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, CONST void*, DWORD, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, void*, DWORD*) override { return D3DERR_NOTFOUND; }
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID) override { return D3D_OK; }
    DWORD STDMETHODCALLTYPE SetPriority(DWORD) override { return 0; }
    DWORD STDMETHODCALLTYPE GetPriority() override { return 0; }
    void STDMETHODCALLTYPE PreLoad() override {}
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_SURFACE; }
    // IDirect3DSurface9
    HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** pp) override;
    HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* d) override;
    HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT* lr, CONST RECT* r, DWORD flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect() override;
    HRESULT STDMETHODCALLTYPE GetDC(HDC*) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE ReleaseDC(HDC) override { return D3DERR_INVALIDCALL; }
};

struct Texture : public Com<IDirect3DTexture9> {
    Device* dev = nullptr;
    std::shared_ptr<GpuImage> img;
    std::vector<Surface*> surfaces;          // one per level (owned)
    DWORD lod = 0;
    D3DTEXTUREFILTERTYPE autogen_filter = D3DTEXF_LINEAR;
    ~Texture() override;
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, CONST void*, DWORD, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, void*, DWORD*) override { return D3DERR_NOTFOUND; }
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID) override { return D3D_OK; }
    DWORD STDMETHODCALLTYPE SetPriority(DWORD) override { return 0; }
    DWORD STDMETHODCALLTYPE GetPriority() override { return 0; }
    void STDMETHODCALLTYPE PreLoad() override {}
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_TEXTURE; }
    DWORD STDMETHODCALLTYPE SetLOD(DWORD l) override { DWORD o = lod; lod = l; return o; }
    DWORD STDMETHODCALLTYPE GetLOD() override { return lod; }
    DWORD STDMETHODCALLTYPE GetLevelCount() override { return img->autogen ? 1 : img->levels; }
    HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE f) override { autogen_filter = f; return D3D_OK; }
    D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override { return autogen_filter; }
    void STDMETHODCALLTYPE GenerateMipSubLevels() override;
    HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT level, D3DSURFACE_DESC* d) override;
    HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT level, IDirect3DSurface9** pp) override;
    HRESULT STDMETHODCALLTYPE LockRect(UINT level, D3DLOCKED_RECT* lr, CONST RECT* r, DWORD flags) override;
    HRESULT STDMETHODCALLTYPE UnlockRect(UINT level) override;
    HRESULT STDMETHODCALLTYPE AddDirtyRect(CONST RECT*) override { return D3D_OK; }
};

struct GpuBuffer {
    SDL_GPUBuffer* buf = nullptr;
    uint32_t size = 0;
};
struct Buffer {                              // shared body of vertex / index buffers
    Device* dev = nullptr;
    std::vector<uint8_t> shadow;
    SDL_GPUBuffer* gpu = nullptr;
    uint32_t size = 0;
    DWORD usage = 0;
    D3DPOOL pool = D3DPOOL_DEFAULT;
    bool index = false;
    uint64_t used_batch = 0;
    UINT lock_off = 0, lock_size = 0;
    DWORD lock_flags = 0;
    int lock_count = 0;
    bool init(Device* d, UINT len, DWORD usage, D3DPOOL pool, bool is_index);
    void release_gpu();
    HRESULT lock(UINT off, UINT sz, void** pp, DWORD flags);
    HRESULT unlock();
};
struct VertexBuffer : public Com<IDirect3DVertexBuffer9> {
    Buffer b;
    DWORD fvf = 0;
    ~VertexBuffer() override { b.release_gpu(); }
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, CONST void*, DWORD, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, void*, DWORD*) override { return D3DERR_NOTFOUND; }
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID) override { return D3D_OK; }
    DWORD STDMETHODCALLTYPE SetPriority(DWORD) override { return 0; }
    DWORD STDMETHODCALLTYPE GetPriority() override { return 0; }
    void STDMETHODCALLTYPE PreLoad() override {}
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_VERTEXBUFFER; }
    HRESULT STDMETHODCALLTYPE Lock(UINT o, UINT s, void** pp, DWORD f) override { return b.lock(o, s, pp, f); }
    HRESULT STDMETHODCALLTYPE Unlock() override { return b.unlock(); }
    HRESULT STDMETHODCALLTYPE GetDesc(D3DVERTEXBUFFER_DESC* d) override;
};
struct IndexBuffer : public Com<IDirect3DIndexBuffer9> {
    Buffer b;
    D3DFORMAT format = D3DFMT_INDEX16;
    ~IndexBuffer() override { b.release_gpu(); }
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, CONST void*, DWORD, DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, void*, DWORD*) override { return D3DERR_NOTFOUND; }
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID) override { return D3D_OK; }
    DWORD STDMETHODCALLTYPE SetPriority(DWORD) override { return 0; }
    DWORD STDMETHODCALLTYPE GetPriority() override { return 0; }
    void STDMETHODCALLTYPE PreLoad() override {}
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_INDEXBUFFER; }
    HRESULT STDMETHODCALLTYPE Lock(UINT o, UINT s, void** pp, DWORD f) override { return b.lock(o, s, pp, f); }
    HRESULT STDMETHODCALLTYPE Unlock() override { return b.unlock(); }
    HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC* d) override;
};

// ---- device state ----
enum { MAX_STAGES = 8, MAX_SAMPLERS = 16, MAX_STREAMS = 4, MAX_LIGHTS_ACTIVE = 8 };
struct Light { D3DLIGHT9 l{}; bool enabled = false; };
struct StreamSrc { VertexBuffer* vb = nullptr; UINT offset = 0, stride = 0, freq = 1; };
struct DevState {
    DWORD rs[256];
    DWORD tss[MAX_STAGES][33];
    DWORD ss[MAX_SAMPLERS][14];
    IDirect3DBaseTexture9* tex[MAX_SAMPLERS];
    D3DMATRIX world[4], view, proj, texm[MAX_STAGES];
    D3DMATERIAL9 material;
    float clip[6][4];
    D3DVIEWPORT9 vp;
    RECT scissor;
    VertexDecl* decl;
    DWORD fvf;
    StreamSrc streams[MAX_STREAMS];
    IndexBuffer* indices;
    VShader* vs;
    PShader* ps;
    float vsc[256][4];
    float psc[224][4];
    int vsi[16][4], psi[16][4];
    BOOL vsb[16], psb[16];
    std::map<DWORD, Light> lights;           // last: everything before it is zero-initialised as plain data
    DevState() { memset((void*)this, 0, offsetof(DevState, lights)); }
    DevState(const DevState&) = delete;
    DevState& operator=(const DevState&) = delete;
};

// A recorded state item (state blocks)
struct SbItem { int kind; DWORD a, b; };
struct StateBlock : public Com<IDirect3DStateBlock9> {
    Device* dev = nullptr;
    std::vector<SbItem> items;
    std::unique_ptr<DevState> st;            // captured values
    ~StateBlock() override;
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    HRESULT STDMETHODCALLTYPE Capture() override;
    HRESULT STDMETHODCALLTYPE Apply() override;
};

struct Query : public Com<IDirect3DQuery9> {
    Device* dev = nullptr;
    D3DQUERYTYPE type = D3DQUERYTYPE_EVENT;
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9** pp) override;
    D3DQUERYTYPE STDMETHODCALLTYPE GetType() override { return type; }
    DWORD STDMETHODCALLTYPE GetDataSize() override { return type == D3DQUERYTYPE_EVENT ? sizeof(BOOL) : sizeof(DWORD); }
    HRESULT STDMETHODCALLTYPE Issue(DWORD) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetData(void* p, DWORD n, DWORD) override;
};

// ---- staged uploads of one batch ----
struct PendingUpload {
    bool is_tex;
    SDL_GPUBuffer* buf; uint32_t buf_off;
    std::shared_ptr<GpuImage> img; UINT level, face, x, y, w, h;
    SDL_GPUTexture* raw_tex;          // a device-owned texture (dummies) when img is null
    uint32_t staging_off, size, row_pixels, rows;
};

// pipeline key: everything SDL bakes into a graphics pipeline
struct PipeKey {
    SDL_GPUShader* vs; SDL_GPUShader* ps;
    uint64_t layout;              // vertex layout hash
    uint8_t prim, fill, cull, samples;
    uint8_t ztest, zwrite, zfunc, stencil;
    uint8_t sfail, szfail, spass, sfunc;
    uint8_t smask, swmask, colormask, blend;
    uint8_t src, dst, op, srca, dsta, opa, sep, has_depth;
    uint32_t color_fmt, depth_fmt;
    float bias, slope;
    bool operator==(const PipeKey& o) const { return memcmp(this, &o, sizeof(*this)) == 0; }
};
struct PipeKeyHash { size_t operator()(const PipeKey& k) const { return (size_t)hash_bytes(&k, sizeof(k)); } };

class D3D9;
class Ffp;

// ---- the device ----
class Device final : public Com<IDirect3DDevice9> {
public:
    D3D9* d3d = nullptr;
    SDL_GPUDevice* gpu = nullptr;
    SDL_Window* window = nullptr;
    HWND hwnd = nullptr;
    D3DPRESENT_PARAMETERS pp{};
    DWORD behavior = 0;
    bool owns_gpu = false;

    DevState st;
    bool recording = false;
    StateBlock* rec = nullptr;
    StateBlock* rec_pre = nullptr;           // the state before BeginStateBlock (restored by EndStateBlock)

    // targets
    Surface* backbuffer = nullptr;           // device-owned
    Surface* frontbuffer = nullptr;
    Surface* auto_depth = nullptr;
    Surface* rt[4] = {};
    Surface* ds = nullptr;

    // batch / command state
    uint64_t batch = 1;
    SDL_GPUCommandBuffer* cmd = nullptr;
    SDL_GPURenderPass* pass = nullptr;
    bool pass_targets_dirty = true;
    Surface* pass_rt = nullptr;
    Surface* pass_ds = nullptr;
    std::vector<uint8_t> staging;
    std::vector<PendingUpload> uploads;
    std::vector<std::shared_ptr<GpuImage>> mip_gen;   // textures needing mip generation before the next pass
    SDL_GPUBuffer* up_buf = nullptr;         // DrawPrimitiveUP stream of the batch
    uint32_t up_buf_size = 0, up_buf_used = 0;
    std::vector<SDL_GPUBuffer*> retired;     // released at flush
    SDL_GPUFence* last_fence = nullptr;

    // bound-state caches inside the pass
    SDL_GPUGraphicsPipeline* bound_pipe = nullptr;
    uint64_t pushed_ver[2][2] = {};
    const void* pushed_src[2][2] = {};
    bool vp_dirty = true, scissor_dirty = true;

    std::unordered_map<PipeKey, SDL_GPUGraphicsPipeline*, PipeKeyHash> pipes;
    std::unordered_map<uint64_t, SDL_GPUSampler*> samplers;
    std::unordered_map<DWORD, VertexDecl*> fvf_decls;
    SDL_GPUTexture* dummy_tex = nullptr;     // (0,0,0,1)
    SDL_GPUTexture* white_tex = nullptr;
    std::unique_ptr<Ffp> ffp;
    UniformSource ps_consts{};               // the register file for asm pixel shaders
    uint64_t ps_consts_ver = 1;
    VsDevBlock vsdev{};
    PsDevBlock psdev{};
    UniformSource vsdev_src{}, psdev_src{};
    uint64_t dev_ver = 1;

    // cursor
    SDL_Cursor* cursor = nullptr;
    bool cursor_visible = true;

    // gamma
    D3DGAMMARAMP gamma{};

    bool in_scene = false;
    uint64_t frame = 0;

    Device();
    ~Device() override;
    bool init(D3D9* d3d, HWND focus, DWORD flags, D3DPRESENT_PARAMETERS* p);
    bool create_swap_targets();
    void destroy_swap_targets();
    void reset_state();

    // batch machinery
    void ensure_cmd();
    void end_pass();
    bool begin_pass(bool clear_color, D3DCOLOR color, bool clear_depth, float z, bool clear_stencil, DWORD stencil);
    void flush(bool wait = false);
    uint32_t stage_bytes(const void* data, uint32_t size, uint32_t align = 16);
    void queue_buffer_upload(SDL_GPUBuffer* buf, uint32_t dst_off, const void* data, uint32_t size);
    void queue_texture_upload(const std::shared_ptr<GpuImage>& img, UINT level, UINT face);
    bool readback(GpuImage* img, UINT level, std::vector<uint8_t>& out_gpu_layout);
    SDL_GPUBuffer* create_buffer(uint32_t size, bool index);
    void retire_buffer(SDL_GPUBuffer* b) { if (b) retired.push_back(b); }

    // draw
    bool prepare_draw(D3DPRIMITIVETYPE prim, const VertexDecl* decl, SDL_GPUBuffer* up_vb, uint32_t up_off, uint32_t up_stride);
    bool prepare_inner(D3DPRIMITIVETYPE prim, const VertexDecl* decl, SDL_GPUBuffer* up_vb, uint32_t up_off, uint32_t up_stride);
    SDL_GPUGraphicsPipeline* pipeline_for(const PipeKey& k, const VsVariant& vv, const VertexDecl* decl, const UINT* strides);
    SDL_GPUSampler* sampler_for(DWORD stage);
    SDL_GPUTexture* texture_for(DWORD stage, bool* bound);
    void clear_quad(DWORD flags, D3DCOLOR color, float z, DWORD stencil, const D3DRECT* r);
    VertexDecl* decl_for_fvf(DWORD fvf);
    SDL_GPUTextureFormat target_color_format() const;
    void draw_ffp_clear_pipelines();

    std::shared_ptr<GpuImage> make_image(UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT fmt, D3DPOOL pool, UINT samples = 1, UINT faces = 1);
    void record(int kind, DWORD a, DWORD b = 0);

    // ---- IDirect3DDevice9 ----
    HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override;
    UINT STDMETHODCALLTYPE GetAvailableTextureMem() override;
    HRESULT STDMETHODCALLTYPE EvictManagedResources() override;
    HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9** pp) override;
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9* pCaps) override;
    HRESULT STDMETHODCALLTYPE GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* p) override;
    HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) override;
    void STDMETHODCALLTYPE SetCursorPosition(int X, int Y, DWORD Flags) override;
    BOOL STDMETHODCALLTYPE ShowCursor(BOOL bShow) override;
    HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9**) override;
    HRESULT STDMETHODCALLTYPE GetSwapChain(UINT, IDirect3DSwapChain9**) override;
    UINT STDMETHODCALLTYPE GetNumberOfSwapChains() override { return 1; }
    HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* p) override;
    HRESULT STDMETHODCALLTYPE Present(CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*) override;
    HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9** pp) override;
    HRESULT STDMETHODCALLTYPE GetRasterStatus(UINT, D3DRASTER_STATUS* p) override;
    HRESULT STDMETHODCALLTYPE SetDialogBoxMode(BOOL) override { return D3D_OK; }
    void STDMETHODCALLTYPE SetGammaRamp(UINT, DWORD, CONST D3DGAMMARAMP* p) override { if (p) gamma = *p; }
    void STDMETHODCALLTYPE GetGammaRamp(UINT, D3DGAMMARAMP* p) override { if (p) *p = gamma; }
    HRESULT STDMETHODCALLTYPE CreateTexture(UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT fmt, D3DPOOL pool, IDirect3DTexture9** pp, HANDLE*) override;
    HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DVolumeTexture9**, HANDLE*) override { return D3DERR_NOTAVAILABLE; }
    HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, IDirect3DCubeTexture9**, HANDLE*) override { return D3DERR_NOTAVAILABLE; }
    HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT len, DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9** pp, HANDLE*) override;
    HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT len, DWORD usage, D3DFORMAT fmt, D3DPOOL pool, IDirect3DIndexBuffer9** pp, HANDLE*) override;
    HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT w, UINT h, D3DFORMAT fmt, D3DMULTISAMPLE_TYPE ms, DWORD q, BOOL lockable, IDirect3DSurface9** pp, HANDLE*) override;
    HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT w, UINT h, D3DFORMAT fmt, D3DMULTISAMPLE_TYPE ms, DWORD q, BOOL discard, IDirect3DSurface9** pp, HANDLE*) override;
    HRESULT STDMETHODCALLTYPE UpdateSurface(IDirect3DSurface9* src, CONST RECT* sr, IDirect3DSurface9* dst, CONST POINT* dp) override;
    HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture9* src, IDirect3DBaseTexture9* dst) override;
    HRESULT STDMETHODCALLTYPE GetRenderTargetData(IDirect3DSurface9* rt, IDirect3DSurface9* dst) override;
    HRESULT STDMETHODCALLTYPE GetFrontBufferData(UINT, IDirect3DSurface9* dst) override;
    HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DSurface9* src, CONST RECT* sr, IDirect3DSurface9* dst, CONST RECT* dr, D3DTEXTUREFILTERTYPE f) override;
    HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9* s, CONST RECT* r, D3DCOLOR c) override;
    HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(UINT w, UINT h, D3DFORMAT fmt, D3DPOOL pool, IDirect3DSurface9** pp, HANDLE*) override;
    HRESULT STDMETHODCALLTYPE SetRenderTarget(DWORD idx, IDirect3DSurface9* s) override;
    HRESULT STDMETHODCALLTYPE GetRenderTarget(DWORD idx, IDirect3DSurface9** pp) override;
    HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9* s) override;
    HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9** pp) override;
    HRESULT STDMETHODCALLTYPE BeginScene() override;
    HRESULT STDMETHODCALLTYPE EndScene() override;
    HRESULT STDMETHODCALLTYPE Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) override;
    HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) override;
    HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) override;
    HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*) override;
    HRESULT STDMETHODCALLTYPE SetViewport(CONST D3DVIEWPORT9* pViewport) override;
    HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9* pViewport) override;
    HRESULT STDMETHODCALLTYPE SetMaterial(CONST D3DMATERIAL9* pMaterial) override;
    HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9* pMaterial) override;
    HRESULT STDMETHODCALLTYPE SetLight(DWORD Index, CONST D3DLIGHT9*) override;
    HRESULT STDMETHODCALLTYPE GetLight(DWORD Index, D3DLIGHT9*) override;
    HRESULT STDMETHODCALLTYPE LightEnable(DWORD Index, BOOL Enable) override;
    HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD Index, BOOL* pEnable) override;
    HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD Index, CONST float* pPlane) override;
    HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD Index, float* pPlane) override;
    HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) override;
    HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) override;
    HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) override;
    HRESULT STDMETHODCALLTYPE BeginStateBlock() override;
    HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9** ppSB) override;
    HRESULT STDMETHODCALLTYPE SetClipStatus(CONST D3DCLIPSTATUS9*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS9* p) override { if (p) memset(p, 0, sizeof(*p)); return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) override;
    HRESULT STDMETHODCALLTYPE SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) override;
    HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) override;
    HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override;
    HRESULT STDMETHODCALLTYPE GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) override;
    HRESULT STDMETHODCALLTYPE SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) override;
    HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD* pNumPasses) override { if (pNumPasses) *pNumPasses = 1; return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT, CONST PALETTEENTRY*) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT, PALETTEENTRY*) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT* p) override { if (p) *p = 0; return D3D_OK; }
    HRESULT STDMETHODCALLTYPE SetScissorRect(CONST RECT* pRect) override;
    HRESULT STDMETHODCALLTYPE GetScissorRect(RECT* pRect) override;
    HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(BOOL) override { return D3D_OK; }
    BOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing() override { return FALSE; }
    HRESULT STDMETHODCALLTYPE SetNPatchMode(float) override { return D3D_OK; }
    float STDMETHODCALLTYPE GetNPatchMode() override { return 0.0f; }
    HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override;
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override;
    HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
    HRESULT STDMETHODCALLTYPE ProcessVertices(UINT, UINT, UINT, IDirect3DVertexBuffer9*, IDirect3DVertexDeclaration9*, DWORD) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl) override;
    HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) override;
    HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) override;
    HRESULT STDMETHODCALLTYPE SetFVF(DWORD FVF) override;
    HRESULT STDMETHODCALLTYPE GetFVF(DWORD* pFVF) override;
    HRESULT STDMETHODCALLTYPE CreateVertexShader(CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader) override;
    HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9* pShader) override;
    HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9** ppShader) override;
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) override;
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) override;
    HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) override;
    HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override;
    HRESULT STDMETHODCALLTYPE SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) override;
    HRESULT STDMETHODCALLTYPE GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride) override;
    HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(UINT StreamNumber, UINT Setting) override;
    HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(UINT StreamNumber, UINT* pSetting) override;
    HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9* pIndexData) override;
    HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9** ppIndexData) override;
    HRESULT STDMETHODCALLTYPE CreatePixelShader(CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader) override;
    HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9* pShader) override;
    HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9** ppShader) override;
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount) override;
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) override;
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount) override;
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) override;
    HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount) override;
    HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) override;
    HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT, CONST float*, CONST D3DRECTPATCH_INFO*) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT, CONST float*, CONST D3DTRIPATCH_INFO*) override { return D3DERR_INVALIDCALL; }
    HRESULT STDMETHODCALLTYPE DeletePatch(UINT) override { return D3D_OK; }
    HRESULT STDMETHODCALLTYPE CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) override;
};

// record kinds for state blocks
enum SbKind { SB_RS, SB_TSS, SB_SS, SB_TEX, SB_XFORM, SB_MATERIAL, SB_LIGHT, SB_LIGHTEN, SB_CLIP, SB_VP, SB_SCISSOR,
              SB_DECL, SB_FVF, SB_STREAM, SB_INDICES, SB_VS, SB_PS, SB_VSC, SB_PSC };

// ---- IDirect3D9 ----
class D3D9 final : public Com<IDirect3D9> {
public:
    std::vector<SDL_DisplayMode> modes;
    void enum_modes();
    HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void*) override { return D3D_OK; }
    UINT STDMETHODCALLTYPE GetAdapterCount() override { return 1; }
    HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) override;
    UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT Adapter, D3DFORMAT Format) override;
    HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT, BOOL) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT, D3DDEVTYPE, D3DFORMAT, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT, D3DDEVTYPE, D3DFORMAT, BOOL, D3DMULTISAMPLE_TYPE, DWORD* pQualityLevels) override;
    HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT, D3DFORMAT) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(UINT, D3DDEVTYPE, D3DFORMAT, D3DFORMAT) override;
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) override;
    HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT) override { return (HMONITOR)(uintptr_t)1; }
    HRESULT STDMETHODCALLTYPE CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** ppDev) override;
};
void fill_caps(D3DCAPS9* c);
SDL_GPUDevice* shared_gpu_device();         // one SDL_GPU device per process (created on first use)

// fixed-function generator (ffp.cpp)
class Ffp {
public:
    struct Impl;
    explicit Ffp(Device* d);
    ~Ffp();
    VShader* vs();                         // the fixed-function vertex shader object
    PShader* ps();                         // the fixed-function pixel shader object
    void update(const VertexDecl* decl);   // fill uniform blocks from the device state before a draw
private:
    Impl* p;
};

// asm shaders (asm.cpp)
bool assemble_shader(const std::string& text, std::vector<DWORD>& tokens, std::string& err);
bool translate_ps_tokens(const DWORD* tokens, std::string& hlsl, std::string& err, uint32_t* inputs);

// path helper: game paths use backslashes and Windows case rules
std::string resolve_path(const char* path);
bool read_file(const char* path, std::vector<uint8_t>& out);

// matrices
void mat_mul(D3DMATRIX& out, const D3DMATRIX& a, const D3DMATRIX& b);
bool mat_inverse(D3DMATRIX& out, float* det, const D3DMATRIX& m);
void mat_identity(D3DMATRIX& m);

} // namespace mbg
