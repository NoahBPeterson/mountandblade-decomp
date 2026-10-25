// resource_manager.h: what src/rgl/resource_manager.cpp (0x417bb0..0x41918f) defines or uses besides the
// classes of stream.h / resource.h: the game options record, render configuration and globals.
// Names provisional.
#ifndef RGL_RESOURCE_MANAGER_H
#define RGL_RESOURCE_MANAGER_H
#include <stdio.h>
#include <d3d9.h>
#include "./stream.h"
#include "./resource.h"

#include "./options.h"   // struct options / g_options (0x5aa008)

// rgl_config.txt (written by 0x415fb0, global at 0x5a9f98)
struct rgl_config { void save(FILE* f); };     // 0x415fb0
extern rgl_config g_rgl_config;                // 0x5a9f98  // old name of g_cfg, removed after the names merge

extern IDirect3DDevice9* g_device;             // 0xa17f28  // old name of g_render, removed after the names merge
extern int g_shadow_quality;                   // 0x5aa000 (shadowmap quality step, counted down on failure)
extern int g_use_shaders;                      // 0x5e28fc: non-zero when the programmable pipeline is in use
extern text_stream g_log;                      // 0x5a9f60 (the engine's text log)

// reads a key array, then flips each quaternion so neighbours have a non-negative dot product
stream_base* read_continuous_quats(stream_base* s, key_array* a);     // 0x417e50
// writes a key clip: name, track count, tracks (yzwx), the extra key array (xyz)
stream_base* save_key_clip(stream_base* s, key_clip* c);              // 0x419030

#endif
