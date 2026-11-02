// render_flags.h: the renderer's mode flags, one declaration for every module. No includes.
#ifndef MB_RENDER_FLAGS_H
#define MB_RENDER_FLAGS_H
extern int g_use_shaders;                    // 0x5e28fc programmable pipeline in use (rec_tables.cpp renderer_setup decides; 0 = colours are gamma-corrected on the CPU); also read as g_dinput_enabled
#endif // MB_RENDER_FLAGS_H
