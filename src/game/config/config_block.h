// config_block.h: Config, the option block read from and written to rgl_config.txt (global g_cfg 0x5a9f98).
// Split out of config.h so the renderer-side sources can use its members (g_cfg.use_vertex_shaders ...) without
// pulling in the framework headers config.h brings; config.h includes this header.
#ifndef GAME_CONFIG_CONFIG_BLOCK_H
#define GAME_CONFIG_CONFIG_BLOCK_H

#include <stdio.h>
#include "../tstring.h"

// ---- the option block (rgl_config.txt), global object g_cfg at 0x5a9f98 ----------------------------------------
// Offsets from the setters 0x4bc020..0x4bc170 and the readers; the block is the int array that
// src/rgl/core/resource_io.cpp (0x415fb0, rgl_config::save) writes out. Field names are the file's keys.
struct Config {
    int first_time;                 // +0x00  0x4bc020, 0x4badf0 (selects the present interval)
    int texture_detail;             // +0x04
    int show_framerate;             // +0x08
    int max_framerate;              // +0x0c
    int start_windowed;             // +0x10
    int skip_tutorial;              // +0x14
    int enable_edit_mode;           // +0x18
    int force_single_threading;     // +0x1c
    int debug_mode;                 // +0x20
    int cheat_mode;                 // +0x24
    int enable_blood;               // +0x28
    int fake_reflections;           // +0x2c
    int disable_frequency_variation;// +0x30
    int use_vertex_shaders;         // +0x34
    int use_pixel_shaders;          // +0x38
    int disable_music;              // +0x3c
    int disable_sound;              // +0x40
    int unk44;                      // +0x44
    int render_buffer_size;         // +0x48  megabytes of buffer memory (0x4bc5b0 reads 0x5a9fe0)
    int display_width;              // +0x4c  0x5a9fe4 (0x4badf0)
    int display_height;             // +0x50  0x5a9fe8
    int display_bits;               // +0x54  0x5a9fec (16: R5G6B5, else X8R8G8B8)
    int antialiasing;               // +0x58  0x5a9ff0 (multisample type 2 / 4)
    int sample_quality;             // +0x5c  0x5a9ff4
    int force_vsync;                // +0x60  0x5a9ff8 (0x4badf0: present interval ONE)
    int unk64;                      // +0x64
    int shadowmap_quality;          // +0x68
    int unk6c;                      // +0x6c
    void init_defaults();                           // 0x4bc020
    void set(tstring& key, tstring& value);         // 0x4bbba0 (fastcall-like: thiscall, ret 8)
    void load_file(FILE* f);                        // 0x4bbf70
    void set_triple_buffering(int t);               // 0x4bc090
    void set_texture_quality(int q);                // 0x4bc100
    void split_line(const char* line, tstring* first, tstring* second);   // 0x4bba90 (thiscall: `this` unused)
    int value_as_int(const tstring& s);             // 0x4bbb70 (thiscall: `this` unused)
};
extern Config g_cfg;                                // 0x5a9f98

#endif
