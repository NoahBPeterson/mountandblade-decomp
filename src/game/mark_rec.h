// mark_rec.h: the scene's entry-point record (MarkRec, 0x130 bytes; 64 of them at Mission +0x670), split out of mission.h
// so rgl/mesh2/rec_tables.cpp, which defines its constructor (0x4c2d10), can take the class without the mission's headers.
// Needs rgl::matrix (rgl_base.h picks the module's own).
#ifndef GAME_MARK_REC_H
#define GAME_MARK_REC_H
#include "../include/layout32.h"
#include "../include/rgl_base.h"
#include "../include/mat4.h"
#include "../include/timer.h"

struct face;
// replace_mark 0x472160, start_setup 0x484590 (fills num_marks of them), constructor 0x4c2d10
struct MarkRec {
    rgl::matrix origin;                         // +0x00 the entry point's placed frame (start_setup: the prop's frame)
    rgl::matrix m;                              // +0x40 current frame (s, f, u, o; the spawn frame of spawn_full)
    rgl::matrix m1;                             // +0x80 (start_setup: a copy of m)
    rgl::matrix m2;                             // +0xc0 next frame
    int cnt2;                                   // +0x100
    int cnt;                                    // +0x104
    int f108, f10c;                             // +0x108 (evidence: no function of src/ names them: the timer starts at the next 8-byte boundary)
    clock2_timer timer;                         // +0x110 (replace_mark restarts it)
    float delay;                                // +0x120 (replace_mark: 0.1 s; start_setup clears it)
    float f124;                                 // +0x124 (6.0 from the constructor)
    face* floor;                                // +0x128
    int f12c;                                   // +0x12c (evidence: no function of src/ names it; the constructor 0x4c2d10 leaves it)
    MarkRec();                                  // 0x4c2d10
};
MB_LAYOUT32_ASSERT(MarkRec_size_check, sizeof(MarkRec) == 0x130);

#endif // GAME_MARK_REC_H
