// resource_manager.h: what src/rgl/resource_manager.cpp (0x417bb0..0x41918f) defines or uses besides the
// classes of stream.h / resource.h: the game options record, render configuration and globals.
// Names provisional.
#ifndef RGL_RESOURCE_MANAGER_H
#define RGL_RESOURCE_MANAGER_H
#include <stdio.h>
#include <d3d9.h>
#include "./stream.h"
#include "./resource.h"

// ---- game options (global at 0x5aa008) --------------------------------------------------------
// Field names provisional. Serialised by options::save() (options.dat) in the order of the code; floats go
// through slot 3, ints through slot 8.
struct options {
    int   i00, i04;
    float f08, f0c, f10;
    int   i14, i18, i1c, i20, i24;
    int   i28;                 // 0x5aa030: cleared when a render-target texture cannot be created
    int   i2c;                 // 0x5aa034: cleared when the environment shadow map cannot be created
    int   i30, i34, i38, i3c, i40, i44, i48, i4c, i50, i54, i58, i5c, i60, i64, i68, i6c, i70;
    float f74;
    int   i78, i7c;
    float f80, f84, f88;
    int   i8c, i90, i94, i98;
    void save();               // 0x4181c0
};
extern options g_options;                      // 0x5aa008

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
