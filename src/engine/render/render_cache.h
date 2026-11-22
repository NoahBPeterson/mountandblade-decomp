// render_cache.h: the part of the Direct3D 9 render-state cache (global object 0xa17910) that mesh.cpp
// uses: the blend-mode and texture-coordinate-generation setters (0x40e940, 0x40ea00) and the fields the mesh
// material setup (0x40f3f0) reads. It is the same object as src/engine/render/renderer_include.h's `renderer` (declared here under the same name, as a partial view) (which has the
// rest of the fields and the other setters); that header cannot be included next to src/engine/resource/resource.h (both
// define global `material` and `mesh_inst`-style classes), so this module keeps its own view of it under
// another name. Merge into one `renderer` class when the headers are unified.
#ifndef RGL_RENDER_CACHE_H
#define RGL_RENDER_CACHE_H
#include <windows.h>
#include <d3d9.h>
#include "engine/math/rgl_math.h"

#include "engine/render/renderer_include.h"     // renderer, g_render (0xa17910): the one definition (its 64-bit layout is the class's own)

#endif
