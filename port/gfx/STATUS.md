# port/gfx status (gfx lane: Direct3D 9 + D3DX over SDL_GPU)

Library `mb_gfx` (port/gfx/CMakeLists.txt) exports `Direct3DCreate9` and the D3DX functions of port/include/d3dx9.h.
It replaces the D3D9 / D3DX placeholders in port/stubs/platform_stubs.cpp (lines "Direct3D 9" and "D3DX 9"): the
integration must drop those from the stubs when it links `mb_gfx`. The window comes from the platform lane's
`mbp_sdl_window(HWND)` (port/platform/mb_sdl.h). Runtime deps: SDL3 >= 3.4, glslang (C API), SPIRV-Cross (C API).

## Layout

| file | what |
|---|---|
| gfx_internal.h | shared types: Device, resources, shader objects, state, pipeline key |
| d3d9_main.cpp | Direct3DCreate9, IDirect3D9 (adapter modes from SDL, caps, format checks, CreateDevice), logging, `mbg_stat` |
| device.cpp | device state (render / stage / sampler states, transforms, lights, material, clip planes, viewport, scissor), resource creation, targets, state blocks (D3D9 record-without-apply semantics), cursor, Reset, Present |
| device_draw.cpp | batches (uploads staged, copied on a command buffer submitted before the draws), render passes, Clear (load-op or quad), pipelines / samplers caches, Draw* (fans converted), readback |
| resources.cpp | GpuImage (SDL texture + D3D-layout CPU shadow), Surface, Texture, VB/IB lock semantics (rename on DISCARD / plain lock of a buffer in flight), vertex declarations, FVF |
| formats.cpp | D3DFORMAT <-> SDL formats, pitches, DXT1-5 decode, CPU conversions (16-bit, L8, X8 ...) |
| ffp.cpp + shaders/ffp_vs.hlsl, ffp_ps.hlsl | fixed function: VS per vertex layout (transform / RHW, lighting, texgen, texture transforms, fog), uber PS (4 stages, all D3DTOP ops) |
| shaders/mbp_common.hlsl | varying block shared by every generated shader + device blocks (half-pixel, clip plane, alpha test, fog) |
| shader_compiler.cpp | glslang HLSL (DX9 mode) -> SPIR-V, binding patching, SPIRV-Cross reflection / MSL, on-disk cache |
| effect.cpp | .fx parser (techniques, passes, sampler_state, texture params, defaults), per-pass entry generation, ID3DXEffect |
| asm.cpp | D3DXAssembleShaderFromFileA (ps_1_x / ps_2_0 -> bytecode) and bytecode -> HLSL for CreatePixelShader |
| d3dx_misc.cpp | matrix / plane helpers, ID3DXBuffer, D3DXCreateTexture, D3DXCreateTextureFromFileExA (DDS + skip mips, TGA, BMP, PNG), D3DXSaveSurfaceToFileA (BMP, PNG, TGA, DDS), game path resolver |

## Verified (tests/gfx_test.cpp, `mb_gfx_test game <outdir>`; Metal, Apple silicon; also with MBG_TEST_MSAA=4)

clear (full + viewport-limited), XYZRHW triangles, FFP alpha test / texture transform (COUNT2) / linear vertex fog, D3D9 cull winding, A8R8G8B8 and DXT1 game textures through
VB/IB + transforms, FFP lighting (directional + ambient + material), render-to-texture, ps.1.1 asm (game/myshader.pp),
mb.fx `no_shading` (matrices via SetMatrixTranspose), mb.fx `diffuse` with sky light + gamma + the fog stage,
CD3DFont pattern (state blocks Capture/Apply, DISCARD / NOOVERWRITE dynamic VB, A4R4G4B4 texture), R32F shadow RT +
D24S8 + depth test + GetRenderTargetData readback, GetFrontBufferData + BMP screenshot, Reset to a new size + hardware
cursor, and all 76 mb.fx techniques compiled and drawn with the game's 4 shader vertex declarations (8 s cold, 25 ms
from the shader cache). Screenshots: <outdir>/*.png.
Perf scenario (the game's per-mesh pattern: 3 SetMatrixTranspose + SetVector + CommitChanges + DrawIndexedPrimitive):
~0.75 us of CPU per draw recorded (2000 draws = 1.5 ms); frames are paced by the display (vsync) on this Mac.

## Remaining / not verified

- Linux / Vulkan: the SPIR-V path is built but never run (no Linux box here). Check winding (cull scenario) and y
  orientation there first.
- Not run inside the game yet (needs the integration: `mb` linking mb_gfx + mb_platform).
- Hardware shadow maps (D24S8 depth *textures* with compare, the PCF_NVIDIA techniques): CheckDeviceFormat refuses
  them on purpose, so the game takes its R32F path. Could be done with SDL compare samplers.
- Dynamic buffers locked whole with DISCARD upload the whole buffer (correct, may cost bandwidth with large
  buffer_size_* settings); compiled effects (.fxo) are not parsed (mb.fx source is required).
- D3DXSaveSurfaceToFileA: JPG not written (src/ only saves BMP).
- Fixed-function: vertex blending (D3DRS_VERTEXBLEND) and stages 4-7 are not implemented (src/ uses neither);
  point sprites unsupported; MULTISAMPLEANTIALIAS toggling ignored (the pipeline follows the target's sample count).
- Methods src/ never calls return D3DERR_INVALIDCALL / NOTAVAILABLE: CreateVertexShader (bytecode), volume / cube
  textures, additional swap chains, patches, ProcessVertices, effect Get*Desc / parameter blocks / CloneEffect.

## Environment knobs

MBG_VERBOSE=1 (log), MBG_GPU_DEBUG=1 (SDL GPU debug layer), MBG_SHADER_CACHE=<dir> (default SDL pref path
mb_port/shadercache), MBG_NO_SHADER_CACHE=1, MBG_FX_LAZY=1 (do not compile every pass's pixel shader at
D3DXCreateEffectFromFile, which is the default as in D3DX; vertex shaders always compile at first draw per declaration).
