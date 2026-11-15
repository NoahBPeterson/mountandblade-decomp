// patches/gfx/gfx_config.cpp: the gfx set's settings in rgl_config.txt (patch set "gfx", opt-in).
//
// CHANGES BEHAVIOUR on purpose: rgl_config::save (0x415fb0) writes four more lines after the original 24.
//
// Keys (defaults in brackets; all off means the set changes nothing at run time):
//   gfx_anisotropy = 0       [0]    anisotropic filtering level: 0 (off), 2, 4, 8 or 16 (rounded down to one of
//                                   those), capped by D3DCAPS9.MaxAnisotropy
//   gfx_render_scale = 1.00  [1.0]  the 3D scene renders at this fraction of the backbuffer per axis, 0.25 .. 1.0
//   gfx_fsr = 0              [0]    0: bilinear upscale of the scaled scene, 1: AMD FSR 1 (EASU + RCAS)
//   gfx_fsr_sharpness = 0.20 [0.2]  RCAS sharpness in stops, 0 = sharpest .. 2
//   gfx_log = 1              [0]    (diagnostics, not written unless set) gfx_log.txt in the game folder
//
// Reading. The game's own reader (Config::load_file 0x4bbf70 -> Config::set 0x4bbba0) ignores names it has no handler
// for, and its line splitter stops a value at the '.', so the new lines are harmless to it. This file reads the same
// rgl_config.txt itself (mb_open_user_file, %APPDATA%\Mount&Blade) from a static constructor, before WinMain.
//
// Writing. Every writer of the file (the launcher dialog, Video Options, the shadow-map fallback in
// resource_manager.cpp) goes through rgl_config::save, so replacing it is the one place to keep the keys: the body below
// is the faithful one (src/rgl/core/resource_io.cpp) plus the gfx lines, which carry the values read at start-up.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "./gfx.h"

FILE* mb_open_user_file(const char* name, const char* mode);          // 0x415160 (src/rgl/text_stream.cpp)

gfx_settings g_gfx = { 0, 1.0f, 0, 0.2f, 0 };
static int s_loaded;

static int round_aniso(int a)
{
    if (a >= 16) return 16;
    if (a >= 8) return 8;
    if (a >= 4) return 4;
    if (a >= 2) return 2;
    return 0;
}

void gfx_load_settings()
{
    if (s_loaded)
        return;
    s_loaded = 1;
    FILE* f = mb_open_user_file("rgl_config.txt", "r");
    if (!f)
        return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        char key[64], val[64];
        if (line[0] == '#' || sscanf(line, " %63[A-Za-z0-9_] = %63s", key, val) != 2)
            continue;
        if (_stricmp(key, "gfx_anisotropy") == 0)
            g_gfx.anisotropy = atoi(val);
        else if (_stricmp(key, "gfx_render_scale") == 0)
            g_gfx.render_scale = (float)atof(val);
        else if (_stricmp(key, "gfx_fsr") == 0)
            g_gfx.fsr = atoi(val);
        else if (_stricmp(key, "gfx_fsr_sharpness") == 0)
            g_gfx.fsr_sharpness = (float)atof(val);
        else if (_stricmp(key, "gfx_log") == 0)
            g_gfx.log = atoi(val);
    }
    fclose(f);
    g_gfx.anisotropy = round_aniso(g_gfx.anisotropy);
    if (!(g_gfx.render_scale >= 0.25f)) g_gfx.render_scale = g_gfx.render_scale > 0.0f ? 0.25f : 1.0f;
    if (g_gfx.render_scale > 1.0f) g_gfx.render_scale = 1.0f;
    g_gfx.fsr = g_gfx.fsr != 0;
    if (!(g_gfx.fsr_sharpness >= 0.0f)) g_gfx.fsr_sharpness = 0.0f;
    if (g_gfx.fsr_sharpness > 2.0f) g_gfx.fsr_sharpness = 2.0f;
    gfx_log("settings: anisotropy %d, render_scale %.3f, fsr %d, fsr_sharpness %.3f\n",
            g_gfx.anisotropy, g_gfx.render_scale, g_gfx.fsr, g_gfx.fsr_sharpness);
}

bool gfx_scaling()
{
    return g_gfx.render_scale < 0.999f;
}

void gfx_log(const char* fmt, ...)
{
    if (!g_gfx.log)
        return;
    static FILE* f;
    if (!f)
        f = fopen("gfx_log.txt", "w");
    if (!f)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fflush(f);
}

// ---- rgl_config::save ------------------------------------------------------------------------------------------------
struct rgl_config { void save(FILE* f); };

#define WRITE_OPT(f, name, val) \
    do { _snprintf(line, 0x100, "%s = %d\n", name, val); fprintf(f, "%s\n", line); } while (0)

// @ replaces 0x415fb0
void rgl_config::save(FILE* f)
{
    char line[0x100];
    int* c = (int*)this;
    WRITE_OPT(f, "first_time", c[0]);
    WRITE_OPT(f, "texture_detail", c[1]);
    WRITE_OPT(f, "render_buffer_size", c[18]);
    WRITE_OPT(f, "max_framerate", c[3]);
    WRITE_OPT(f, "start_windowed", (c[4] != 0));
    WRITE_OPT(f, "use_pixel_shaders", (c[14] != 0));
    WRITE_OPT(f, "use_vertex_shaders", (c[13] != 0));
    WRITE_OPT(f, "fake_reflections", (c[11] != 0));
    WRITE_OPT(f, "show_framerate", (c[2] != 0));
    WRITE_OPT(f, "disable_music", (c[15] != 0));
    WRITE_OPT(f, "disable_sound", (c[16] != 0));
    WRITE_OPT(f, "disable_frequency_variation", (c[12] != 0));
    WRITE_OPT(f, "cheat_mode", (c[9] != 0));
    WRITE_OPT(f, "enable_blood", (c[10] != 0));
    WRITE_OPT(f, "enable_edit_mode", (c[6] != 0));
    WRITE_OPT(f, "force_single_threading", (c[7] != 0));
    WRITE_OPT(f, "debug_mode", (c[8] != 0));
    WRITE_OPT(f, "display_width", c[19]);
    WRITE_OPT(f, "display_height", c[20]);
    WRITE_OPT(f, "display_bits", c[21]);
    WRITE_OPT(f, "antialiasing", c[22]);
    WRITE_OPT(f, "sample_quality", c[23]);
    WRITE_OPT(f, "force_vsync", c[24]);
    WRITE_OPT(f, "shadowmap_quality", c[26]);
    // the gfx set's keys, as read at start-up (the game has no UI for them)
    gfx_load_settings();
    WRITE_OPT(f, "gfx_anisotropy", g_gfx.anisotropy);
    _snprintf(line, 0x100, "%s = %.2f\n", "gfx_render_scale", g_gfx.render_scale); fprintf(f, "%s\n", line);
    WRITE_OPT(f, "gfx_fsr", g_gfx.fsr);
    _snprintf(line, 0x100, "%s = %.2f\n", "gfx_fsr_sharpness", g_gfx.fsr_sharpness); fprintf(f, "%s\n", line);
    if (g_gfx.log)
        WRITE_OPT(f, "gfx_log", g_gfx.log);
}
