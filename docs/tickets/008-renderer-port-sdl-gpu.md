# 008 Renderer: full port from Direct3D 9 to the SDL3 GPU API

Area: render | Effort: XL | Impact: very high (Metal on macOS, Vulkan on Linux, D3D12 on Windows, no translation layer) | Depends on: 004, 007; shaders in 009

## Summary

Replace the D3D9 device usage with a small render-hardware interface (`rhi::`) implemented on SDL_GPU. The D3D9 surface in this
game is much smaller than the engine's size suggests, and it is already funnelled through one state-cache object, so this is a
contained port.

## Evidence (src/, 2026-11-02)

- `IDirect3DDevice9` method calls: **314 sites, ~50 methods**. Top: `SetRenderState` 114 (8 files), `SetTextureStageState` 37 (4 files), `SetSamplerState` 17,
  `SetTexture` 15, `SetTransform` 11, `Lock/Unlock` 8+8, `GetDeviceCaps` 6, `Clear` 5, `SetPixelShader` 5, `CreateVertexDeclaration` 5, `LightEnable` 4,
  `SetStreamSource` 4, `SetRenderTarget` 3, `Present` 3.
- **Only two scene draw calls**: `DrawIndexedPrimitive(D3DPT_TRIANGLELIST, ...)` at src/game/terrain_a/terrain_a.cpp:3345 and src/game/terrain_b/terrain_b.cpp:598;
  three `DrawPrimitiveUP(D3DPT_LINELIST)` debug draws in src/rgl/shapes/shapes.cpp:185,456,754; two `DrawPrimitive` for CD3DFont (d3dfont.cpp:542,555).
- The state cache: `struct renderer` (`g_render`, src/include/renderer.h, 0x618 = `dev`): fog/lighting/specular/normalize/z-write/z-func flags, `light_on[64]`,
  per-stage color/alpha/xform/texgen/sampler arrays (3 stages), `blend_mode`, `alpha_mode` (0 off, 1 ref 8, 2 ref 0x88, 3 ref 0xfb), `tex_factor`, material power, matrix stack
  (20 deep), `view`/`cur` matrices; methods `set_view 0x461630, reset_state 0x4617b0, set_lighting 0x461ba0, set_stage_color 0x461bd0, set_alpha_mode 0x461cf0,
  set_stage_sampler 0x461d90, set_material 0x461e10`. This struct **is** the seam: everything that changes pipeline state goes through it or through the
  ~51 `g_effect` references in terrain_b.cpp.
- Vertex formats: 5 vertex declarations built in config.cpp:1086-1094 (`ffp`, `regular`, `normalmap`, `skinning`, `normalmap_skin`); CPU skinning exists too (`FxDef::skin` 0x4291c0
  blends position+normal over 4 bones; `mesh::ensure_skin` 0x412700).
- Render targets / shadows: `SetRenderTarget` 3, `CreateDepthStencilSurface` 2, `D3DXCreateTexture` with `D3DUSAGE_RENDERTARGET` / `AUTOGENMIPMAP` / `DEPTHSTENCIL` (resource_manager.cpp:270-272),
  `g_shadow_map` (pick_size, `fShadowMapSize` uniforms), `GetRenderTargetData` 1 (screenshot/readback), `SetClipPlane` 2 (water reflection).
- Textures: 370 DDS + 3 TGA (156 MB), loaded with `D3DXCreateTextureFromFileExA(..., D3DX_FROM_FILE, ...)` (resource_io.cpp:350) honouring `D3DX_SKIP_DDS_MIP_LEVELS` for texture_detail.
- Present: `D3DSWAPEFFECT_DISCARD`, interval `IMMEDIATE` (0x80000000) unless `force_vsync`; MSAA 2/4 from config.
- The profile of a real run under CrossOver (patches/profile, `work` samples) shows opengl32.dll + win32u + ntdll = ~41% of main-thread samples: the D3D9 -> wined3d -> GL stack is a measurable tax today.

## Is SDL_GPU a good fit? (decision)

Yes, with two caveats.
- Needs: vertex/index buffers (static + dynamic with discard-style updates), 2D/cube textures with BC1/BC2/BC3 (DXT1/3/5) and mipmaps, depth/stencil
  (D16/D24S8/D32), render-to-texture, MSAA, blending/alpha-to-coverage/alpha-test (shader `discard`), uniform buffers, sampler states, scissor/viewport, readback.
  All available in SDL_GPU. No geometry/tessellation/compute needed.
- Caveat 1: no OpenGL/GLES fallback: needs Vulkan 1.0+, Metal, or D3D12 (older GPUs and some VMs out). If that is unacceptable choose **bgfx** (GL fallback, shader cross-compile via shaderc), at the cost of an
  extra dependency and its own render-state model. I do not recommend it unless a GL requirement appears.
- Caveat 2: no fixed-function: lights, fog, texture stage ops, alpha test, texgen must be reproduced in shaders (an "FF uber-shader" keyed by the state cache). That is real work but it
  is data-driven by ~10 state variables listed above.
- BC formats on Apple GPUs: supported on Apple Silicon Macs (M1+); verify on the minimum target.
- Maturity risk: SDL_GPU appeared in SDL 3.2 (2025); expect API churn between 3.x releases; pin a version.

## Detailed plan

B0 (optional, Linux-only bring-up): compile against dxvk-native's D3D9 headers/library to get pixels on screen before the RHI exists. Throw away afterwards. Not needed on macOS.
B1 `rhi` interface (~40 functions): `create_buffer/texture/sampler/pipeline`, `begin_pass(target, depth, clear)`, `set_pipeline`, `bind_vertex/index`, `bind_texture(stage)`, `push_uniforms`, `draw_indexed`, `present`.
B2 Replace `struct renderer`'s calls to `dev->SetRenderState/...` with state-cache mutators that build a **pipeline key** (blend, z, cull, alpha mode, fog, lighting, stage ops, vertex decl) and look up a cached `SDL_GPUGraphicsPipeline`. The existing cache bits (`b505..b54b`, `alpha_mode`, `blend_mode`, `stage_*`) become the key. 114 SetRenderState sites collapse into the ~12 `renderer::set_*` methods plus ~8 files that call the device directly (list: `rg -l 'dev->|m_pd3dDevice->'`: d3dapp, d3dfont, renderer.cpp, resource_manager, terrain_a/b, mesh_2, rec_tables, trigger_vec, mesh_obj, resource_io, config, mesh_pass, shapes).
B3 Replace `Lock/Unlock` (dynamic VB/IB) with `SDL_MapGPUTransferBuffer` ring buffers; `DrawPrimitiveUP` -> per-frame scratch vertex buffer.
B4 Textures: own DDS loader (009), `SDL_UploadToGPUTexture`; mip skipping by header; render targets as `SDL_GPUTexture` with COLOR_TARGET|SAMPLER.
B5 Port shaders (009), implement the FF uber-shader, shadow map pass, water reflection (clip plane -> `discard`/plane in vertex shader), post path.
B6 Delete `d3dapp.cpp`'s device-reset logic: SDL_GPU has no device-lost; windowed/fullscreen switching becomes a swapchain parameter change.
B7 CD3DFont: the GDI font bake (CreateFont/ExtTextOut) goes; the game's own `font.dds` + `font_data.xml` path stays; debug text via stb_truetype or the same bitmap font.

## Code sketch

```cpp
// src/render/rhi.h (new, portable config) -- the seam renderer.h hands to
struct PipelineKey { uint8_t blend, alpha_mode, zfunc, cull, fog, lighting, specular, normalize; uint8_t stage_color[3], stage_alpha[3]; uint8_t decl; uint32_t fx_technique; };
SDL_GPUGraphicsPipeline* rhi_pipeline(const PipelineKey&);          // cached by hash
void renderer::set_alpha_mode(int m) { alpha_mode = m; dirty |= DIRTY_PIPE; }   // was SetRenderState(ALPHATESTENABLE/ALPHAREF)
void renderer::draw_indexed(const MeshSlot& s, int nv, int first, int nface) { flush_state(); rhi_draw_indexed(nface*3, first, s.base_vertex); }
```

## Risks

- The 8 sites that read caps (`GetDeviceCaps` 6, `CheckDeviceFormat` 2) steer behaviour (pixel shader version, texture formats): decide capability answers statically.
- Visual parity vs the D3D9 original (fog formula, specular, texgen, alpha-ref values) needs reference screenshots: capture from the CrossOver build (`D3DXSaveSurfaceToFile` call exists at config.cpp:566).
- State leaks: the cache has sentinel values (-457, 0xffbd6370) that mean "unknown"; keep semantics.

## Acceptance

Menu, campaign map, a field battle, a siege render within visual tolerance of the CrossOver build (screenshot diff tool with a perceptual threshold), on Metal (macOS) and Vulkan (Linux); 60 fps at 1080p on a mid GPU.

## Effort

XL (10-16 wk for B1-B6 incl. FF uber-shader). Estimate only; the small number of draw sites and one state cache reduce risk, the shader port (009) adds.
