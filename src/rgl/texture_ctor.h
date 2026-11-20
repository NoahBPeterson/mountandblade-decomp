// texture_ctor.h: texture's inline constructor (rgl/resource.h declares it). Only the objects that construct textures
// include it (core/resmgr.cpp: load_stream's and get_render_target's array news; resource_manager.cpp): the definition in
// resource.h itself moved cl's symbol numbering in TUs that never use it (map_tooltip.cpp 0x51c090, video_page.cpp 0x4d63b0).
#ifndef RGL_TEXTURE_CTOR_H
#define RGL_TEXTURE_CTOR_H
#include "./resource.h"
inline texture::texture()
{
    loaded = 0;
    id = -1;
    width = 0x80;
    height = 0x80;
    kind = 0;
    d3d_tex = 0;
    surface = 0;
    level0 = 0;
    owns_texture = 0;
    used_format = 0x1c;
    format = 0x1c;
    rendered = 0;
    last_frame = -1;
    mipmaps = 0;
}
#endif
