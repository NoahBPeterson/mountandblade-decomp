// resource_manager.h: what src/engine/resource/resource_manager_init.cpp (0x417bb0..0x41918f) defines or uses besides the
// classes of stream.h / resource.h: the game options record, render configuration and globals.
// Names provisional.
#ifndef RGL_RESOURCE_MANAGER_H
#define RGL_RESOURCE_MANAGER_H
#include <stdio.h>
#include <d3d9.h>
#include "engine/io/stream.h"
#include "engine/resource/resource.h"

#include "game/config/options.h"   // struct options / g_options (0x5aa008)
#include "engine/render/render_flags.h"

// rgl_config.txt: Config::save (0x415fb0, game/config/config_block.h; global g_cfg at 0x5a9f98)



// reads a key array, then flips each quaternion so neighbours have a non-negative dot product
stream_base* read_continuous_quats(stream_base* s, key_array* a);     // 0x417e50
// writes a key clip: name, track count, tracks (yzwx), the extra key array (xyz)
stream_base* save_key_clip(stream_base* s, key_clip* c);              // 0x419030

#endif
