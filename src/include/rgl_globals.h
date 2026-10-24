// rgl_globals.h: canonical declarations of the rgl-side globals that several modules declared under different names (sorted by address).
#ifndef INCLUDE_RGL_GLOBALS_H
#define INCLUDE_RGL_GLOBALS_H

#include "../rgl/stream.h"
#include "../rgl/scene/world.h"

// rgl/particles/mesh_obj.cpp should_batch compares against it (base vertex budget).
extern int g_vert_budget; // 0x5a9c34 base vertex budget of should_batch
// The engine text log: all views (log_stream, Output, logger, char[] g_log_obj, LogSink) call slot 10 (w_cstr) on it.
extern text_stream g_log; // 0x5a9f60 the engine text log
// The current level/scene: world class (mission.h: typedef world Level;
extern rgl::world* g_level; // 0xa732b4

#endif // INCLUDE_RGL_GLOBALS_H
