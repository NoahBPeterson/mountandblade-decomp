// head_rec.h: the "head record" family of the editor/module tables: every object starts with the vtable
// 0x5947d8 (head_rec), a status int at +4 (0), the constant 0x63 at +8 and a name tstring at +0x1c.
// Derived classes store their own vtable after the base constructor ran (the vtables are named in
// symbols/slices/rec_tables.txt and symbols/slices/gui_edit.txt; a derived class declares one virtual so
// that cl emits its vptr store).
// Layout evidence: 0x4c2c50 (C4c2c50::C4c2c50: vt, name ctor +0x1c, +4 = 0, +8 = 0x63), 0x4c4c90 (head_rec_delete:
// vt 0x5947d8, name dtor), 0x4c49f0 (head_cam::head_cam), 0x4c4cc0 (head_obj2::head_obj2).
// Merged from s004c2b20 (Base0 / C4c2da8), s004c46d0 (head_rec), s004c3d90 (particle_cam).
#ifndef MESH2_HEAD_REC_H
#define MESH2_HEAD_REC_H
#include "../../game/tstring.h"
#include "../rgl_math.h"
#include "../scene/frustum.h"
#include "./mesh2_common.h"

#include "../../game/head_rec.h"

// The camera-like object of 0x4c49f0 (vtable 0x595bd8), written/read by 0x4c4b70 / 0x4c4c00. The slices called it
// head_cam (s004c46d0) and particle_cam (s004c3d90).
struct w_stream {
    virtual void v01();                                 // 0   0x04
    virtual void v02();                                 // 1   0x08
    virtual void v03();                                 // 2
    virtual w_stream* w_float(float* v);                // 3   0x0c, returns the next writer
    virtual void v04();                                 // 4   0x10
    virtual void v05();                                 // 5
    virtual void v06();                                 // 6
    virtual void v07();                                 // 7
    virtual void v08();                                 // 8
    virtual void v09();                                 // 9
    virtual void v10();                                 // 10
    virtual void v11();                                 // 11
    virtual void v12();                                 // 12
    virtual void v13();                                 // 13
    virtual w_stream* r_float(float* v);                // 14  0x38, returns the next reader
};

struct head_cam : head_rec {
    char    pad30[0x64 - 0x2c];
    char    flag64;            // +0x64
    char    pad65[0x170 - 0x65];
    rgl::frustum fr;           // +0x170 (0x120 bytes)
    rgl::vector4 p290;         // +0x290
    rgl::vector4 p2a0;         // +0x2a0
    int     w2b0;              // +0x2b0
    float   f2b4;              // +0x2b4
    float   f2b8;              // +0x2b8
    head_cam();                // 0x4c49f0
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    virtual void scale_5(const float*);
    virtual void scale_6(const float*);
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
    void write_stream(w_stream* w);     // 0x4c4b70
    void read_stream(w_stream* r);      // 0x4c4c00
};

// The second head object of 0x4c4cc0 (vtable 0x595c20, frustum at +0x2ec).
struct head_obj2 : head_rec {
    char    pad30[0x2dc - 0x2c];
    int     w2dc;              // +0x2dc  -1
    int     w2e0;              // +0x2e0
    int     w2e4;              // +0x2e4  0
    char    pad2e8[4];
    rgl::frustum fr;           // +0x2ec
    head_obj2();               // 0x4c4cc0
    virtual void slot_empty_1();
    virtual void slot_empty_2();
    virtual void scale_3(const float*);
    virtual char pure_7() { return 1; }    // slot 7: 0x48b170
    // own functions: Rec428's are release/init at 1/2, and its table carries nine entries).
};

#endif
