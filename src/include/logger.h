// logger.h: Logger, the debug-log object class (vptr first; slot 10 at +0x28 takes the message text and returns the logger, slot 11 at
// +0x2c takes a tstring). ONE definition for src/game/terrain_b (scene_file.h: the loader warns through g_log 0x5a9f60 .get(msg))
// and src/rgl/scene (render_scene.cpp: the debug object at 0x5a9f30, slot 10 prints "RENDERING REFLECTIONS;", result unused).
#ifndef MB_LOGGER_H
#define MB_LOGGER_H
#include "../game/tstring.h"

struct Logger {
    virtual void l0(); virtual void l1(); virtual void l2(); virtual void l3();
    virtual void l4(); virtual void l5(); virtual void l6(); virtual void l7();
    virtual void l8(); virtual void l9();
    virtual Logger* get(const char* msg);       // +0x28 (render_scene.cpp: print, result unused)
    virtual void put(const tstring* s);         // +0x2c
    // The data of a stream_base (rgl/stream.h: this object is a text_stream, vtable 0x54a7d0); the ctor is static_init.cpp's.
    tstring name;                               // +0x04 file name
    int state;                                  // +0x14
    char autoflush;                             // +0x18
    int flushed;                                // +0x1c
    tstring buf;                                // +0x20 the text written so far: the save/load screen (char_sheet/facegen_misc.cpp) draws it, ';' separated
};

extern Logger g_debug_log; // 0x5a9f30 debug log object (slot 10 prints one line; buf is the status text of the save/load screen)
#endif // MB_LOGGER_H
