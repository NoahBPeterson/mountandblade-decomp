//-----------------------------------------------------------------------------
// File: progress_bar_slots.cpp
//
// Desc: the virtual slots progress_bar (vtable 0x597f20) overrides: each a stub /O2 compiles to a bare `jmp` to the
//       member the original's slot names (see ../ui/widget_slots.cpp for the scheme). Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./progress_bar.h"

typedef void (progress_bar::*SetRatioPF)(int);

void progress_bar::d10(rgl::scene* sc, float* count, int c, int d) { draw_label(sc, count, c, d); }
void progress_bar::d13(int v) { (this->*(SetRatioPF)&progress_bar::set_ratio)(v); }
void progress_bar::update_children() { update(); }
