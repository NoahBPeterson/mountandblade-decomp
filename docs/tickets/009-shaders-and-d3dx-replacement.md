# 009 Shaders, effects and the D3DX replacement

Area: render | Effort: L | Impact: high | Depends on: 008 (parallel), 004

## Summary

D3DX9_31 is 12 imports, but two of them (`D3DXCreateEffectFromFileA`, `D3DXAssembleShaderFromFileA`) carry the game's entire programmable pipeline.
Replace everything with own code: math helpers, DDS/TGA loading, screenshot writing, and a hand-ported shader set compiled offline.

## Evidence

- D3DX imports (work/mb.analysis.bin): `SaveSurfaceToFileA, MatrixTranspose, PlaneTransform, MatrixPerspectiveOffCenterRH, MatrixOrthoOffCenterLH/RH, CreateTexture,
  AssembleShaderFromFileA, MatrixInverse, MatrixMultiply, CreateTextureFromFileExA, CreateEffectFromFileA`. In src: `D3DXMATRIX` 30, `D3DXVECTOR4` 21, `D3DXPLANE` 5, `D3DXHANDLE` 5.
- `build_shader()` src/game/triggers/trigger_vec.cpp:793-: loads `mb.fx` (fallback precompiled `mb.fxo`, flag 0x800) then `SetFloat("fShadowMapNextPixel"/"fShadowMapSize")`,
  `SetVector("output_gamma"/"output_gamma_inv")`; `g_effect` referenced 77 times (terrain_b.cpp 51, trigger_vec 12, renderer 5, video_page 3).
- `mb.fx`: 3,084 lines, 77 `technique`/`pass` keywords counted, `vs_2_0` / `ps_2_0` profiles (grep of the shipped file). It is shipped *with the game data*, not in this repo;
  the port must not redistribute it, so the plan converts it with a script run by the user or reimplements shaders under the repo's licence.
- Pixel shader assembly: three `.pp` files (7-9 lines each) assembled at runtime (resource_io.cpp:490) and set with `SetPixelShader` (5 sites).
- DDS: 370 files, DXT1/3/5 (verify per file with a header scan: tool in this ticket's plan step 1).
- Effects used by modules: materials reference technique names from `mb.fx` (module `tableau_materials.txt`, `shaders` in BRF: `tok == "shader"` in resmgr.cpp), so the **technique
  names are a modding interface** (ticket 014).

## Detailed plan

1. Inventory: parse `mb.fx` (HLSL SM2) for techniques, passes, render states set inside passes, samplers, constants (a ~200-line python script using regexes; output a CSV).
2. Choose the shader language: author in HLSL SM5.0-style or Slang/GLSL; compile with SDL_shadercross (HLSL -> SPIR-V/MSL/DXIL) offline into `.spv/.msl/.dxil` blobs shipped with the engine.
   Note: SDL_shadercross uses DXC (SM6+), so SM2 `ps_2_0` syntax (`tex2D`, `:COLOR`, `compile ps_2_0`) must be converted: replace `tex2D(s, uv)` with `Texture.Sample`, drop effect-file state blocks.
3. Replace the effect framework with a `technique -> pipeline` table generated from step 1 (pass count is mostly 1-2): `ShaderLibrary::find("diffuse_normal_skin")` returns pipeline variants keyed by (technique, vertex decl, blend/z state).
4. Constants: map `SetFloat/SetVector/SetMatrix` (terrain_b.cpp et al.) to a uniform struct per stage with named offsets generated from step 1.
5. The 3 asm pixel shaders -> small HLSL equivalents (ps_1_x asm is 4 instructions each).
6. Fixed-function uber-shader (008 B5): stage ops D3DTOP_* used (SetTextureStageState COLOROP/ALPHAOP/TEXCOORDINDEX), 8 lights max (`light_on[64]` cache but D3D9 FF limits), fog, specular.
7. D3DX replacements: matrices (`D3DXMatrixPerspectiveOffCenterRH` etc.) -> 30 lines of own math (src/rgl/rgl_math.h already has `matrix`); `D3DXPlaneTransform`; DDS loader (header + BC blocks; skip mips; ~150 lines);
   TGA/BMP via `stb_image`; screenshots via `stb_image_write` (`D3DXSaveSurfaceToFile` formats BMP/JPG/PNG: config.cpp:566); `D3DXCreateTexture` -> `rhi_create_texture`.
8. Tool: `tools/data/fx_inventory.py` and a test that every technique name referenced by `meshes.txt`/`tableau_materials.txt` of Native has a pipeline.

## Code sketch

```cpp
// port/dds.cpp
struct DdsInfo { uint32_t w, h, mips; SDL_GPUTextureFormat fmt; const uint8_t* data; };
bool dds_parse(const void* file, size_t n, int skip_mips, DdsInfo* out);   // DXT1->BC1_RGBA_UNORM, DXT3->BC2, DXT5->BC3, A8R8G8B8, R5G6B5 (expand on CPU)
```

## Risks

- Effect-state semantics (render states set per pass, `FogEnable`, `AlphaTestEnable`) must be reproduced exactly or lighting/alpha will look off; automate the diff with screenshot comparison.
- Mod shaders: mods may ship their own `mb.fx`. With the port, shader sources become a supported mod surface (014) rather than an accident.
- Legal: do not commit `mb.fx`.

## Acceptance

All Native techniques have pipelines; screenshot diff vs CrossOver build below tolerance on 20 reference scenes; no D3DX symbol in the link.

## Effort

L (8-12 wk), overlapping 008.
