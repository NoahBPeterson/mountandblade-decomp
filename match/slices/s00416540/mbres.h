// mbres.h: the engine's named resource records (material, texture, mesh skin, effect, body part).
// Shared by slices s00416540 and s00416ef0 (copied into each). Names are provisional; layouts come from the
// constructors/destructors at 0x416830, 0x4168f0, 0x416cd0, 0x417010, 0x4171c0, 0x417600..0x4177b0.
#ifndef MB_RES_H
#define MB_RES_H
#include "mbstream.h"
#include "../../include/rgl.h"
#include <d3d9.h>

// Root of the resource classes, vtable 0x54a6c8 (slot 0 _purecall = pure virtual destructor, slot 1 0x408020).
struct res_base {
    virtual ~res_base() = 0;
    virtual int type();
    tstring name;                  // +4
    int f14;                       // +0x14
    res_base() : f14(0) {}
};
inline res_base::~res_base() {}
inline int res_base::type() { return 0; }

// Axis-aligned box with centre and bounding radius (0x38 bytes): reset 0x416e40, extend 0x416ef0,
// update 0x416f70 (the 4th lane of each vector4 is carried along by whole-struct copies).
struct aabb {
    rgl::vector4 min;              // +0
    rgl::vector4 max;              // +0x10
    rgl::vector4 center;           // +0x20
    float radius;                  // +0x30
    void reset();                                  // 0x416e40 (void: not a constructor)
    void extend(const rgl::vector4* p);            // 0x416ef0
    void update();                                 // 0x416f70
};

// One texture-stage record of a material (0x14 bytes).
struct stage_t {
    int a, b, c, d, e;
    __declspec(noinline) void load(stream_base* s);     // 0x416ad0
    __declspec(noinline) void save(stream_base* s);     // 0x416bc0
};

// Material record, vtable 0x54ab30, 0xc8 bytes.
struct material : res_base {
    int f18;                       // +0x18
    unsigned flags1c;              // +0x1c (bit 12: needs pixel shader 1.1)
    unsigned flags20;              // +0x20 (bit 16 disabled, bit 28 has pixel shader, bit 29 needs vertex shader)
    int nalt;                      // +0x24
    tstring alts[3];               // +0x28
    tstring ps_file;               // +0x58
    stage_t stages[4];             // +0x68
    int nstages;                   // +0xb8
    IDirect3DPixelShader9* ps;     // +0xbc
    int fc0;                       // +0xc0
    char ps_done;                  // +0xc4
    material();                    // 0x416830
    virtual ~material();           // 0x4168f0 (deleting: 0x417690)
    virtual int type();            // 0x48c460
    virtual void load(stream_base* s);      // 0x4169a0
    virtual void save(stream_base* s);      // 0x416b10
    bool supported();                       // 0x416930
    void create_shaders();                  // 0x416c00
};

// Mesh-skin record, vtable 0x54abf0, 0x6c bytes (provisional name): five referenced resources, a colour
// and a scalar.
struct skin : res_base {
    char pad18[4];                 // +0x18
    unsigned id;                   // +0x1c
    tstring file;                  // +0x20
    int f30;                       // +0x30
    rgl::vector4 color;            // +0x34
    float f44;                     // +0x44
    res_base* refs[5];             // +0x48
    int f5c, f60, f64, f68;        // +0x5c
    skin();                        // 0x416cd0
    virtual ~skin();               // 0x417600 (deleting)
    virtual int type();            // 0x48b880
    virtual void load(stream_base* s);      // 0x4190d0
    virtual void save(stream_base* s);      // 0x416d50
};
#endif
