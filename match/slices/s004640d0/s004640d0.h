// Slice s004640d0: ShadowMap read-back helpers (system-memory copy of the shadow map via Direct3D 9), the sco
// terrain reader (TerrainDesc / TerrainDescEntry: run-length coded 420x420 layer weights), the blend-weight record
// (Blend: 15 layer weights) and bit-field accessors. ALL NAMES PROVISIONAL.
#ifndef S004640D0_H
#define S004640D0_H
#include <stddef.h>
#include <new>
#include <math.h>
#include <windows.h>
#include <d3d9.h>
#include "rgl.h"
#include "tstring.h"

struct stream_base {
    virtual void reset();                                  // 0
    virtual void rewind();                                 // 1
    virtual stream_base& w_stream(stream_base* o) = 0;     // 2
    virtual stream_base& w_float(float* v) = 0;            // 3
    virtual stream_base& w_bool(char* v) = 0;              // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5
    virtual stream_base& w_i64(__int64* v) = 0;            // 6
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7
    virtual stream_base& w_int(int* v) = 0;                // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9
    virtual stream_base& w_cstr(const char* s) = 0;        // 10
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 11
    virtual stream_base& slot12(void* v) = 0;              // 12
    virtual stream_base& r_stream(stream_base* o) = 0;     // 13
    virtual stream_base& r_float(float* v) = 0;            // 14
    virtual stream_base& r_bool(char* v) = 0;              // 15
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17
    virtual stream_base& r_i64(__int64* v) = 0;            // 18
    virtual stream_base& r_int(int* v) = 0;                // 19
    virtual stream_base& r_byte(char* v) = 0;              // 20
    virtual stream_base& r_tstr(tstring* v) = 0;           // 21
};

void mb_error(const char* fmt, ...);                       // 0x42cc80

extern IDirect3DDevice9* g_device;                         // 0xa17f28
extern char g_use_hw_shadow;                               // 0xa17f24
extern int g_flag_5aa084;                                  // 0x5aa084 (cleared when the system-memory texture cannot be created)

// shadow-map render target (see s00463790): only the chain to the Direct3D texture is used here
struct TexRes { char pad0[0x40]; IDirect3DTexture9** tex; };   // tex at +0x40
struct Skin {
    char pad0[0x48];
    TexRes* refs[5];                                       // +0x48
};
struct ShadowMap {
    Skin* target;                                          // +0
    char pad4[8];
    IDirect3DSurface9* surf;                               // +0xc system-memory copy of the map
    int w, h;                                              // +0x10 / +0x14
    void sync_readback();                                  // 0x4640d0
    void read_pixel(int unused, int x, int y, unsigned* a, unsigned* b);   // 0x4641a0
};

// run-length coded layer of an sco terrain
struct GroundType { tstring name; char pad10[0x20 - 0x10]; };
extern GroundType g_ground_types[11];                        // 0x8ed4df8 (stride 0x20; bound by symbols/slices/s004640d0.txt: beyond the annotation range)
struct Dims { int w, h; };
struct TerrainDescEntry {
    tstring name;                                          // +0
    int type;                                              // +0x10 (-1: none)
    float* data;                                           // +0x14 (420 x 420 floats, index x*420 + y)
    void read(stream_base* s, Dims* d);                    // 0x464200
};
struct TerrainDesc {
    int n;                                                 // +0 (layers in use, max 13)
    int n_minus2;                                          // +4
    int n_minus1;                                          // +8
    TerrainDescEntry layers[13];                               // +0xc
    Dims dims;                                             // +0x144 / +0x148
    void init();                                           // 0x464400
    void read(stream_base* s);                             // 0x464440
};

// 15 blend weights (+0x48) plus a few fields
struct Blend {
    rgl::vector4 p;                                        // +0
    int f10, f14;                                          // +0x10 / +0x14
    float f18, f1c;                                        // +0x18 / +0x1c
    char pad20[0x30 - 0x20];
    rgl::vector4 v30;                                      // +0x30
    char pad40[0x44 - 0x40];
    int f44;                                               // +0x44
    float w[15];                                           // +0x48
    float f84;                                             // +0x84
    float f88;                                             // +0x88
    int f8c;                                               // +0x8c
    float f90;                                             // +0x90
    void init(const float* p4);                            // 0x4644e0
    bool visible(int i);                                   // 0x4645b0
    int dominant();                                        // 0x4646b0
};

// 0x464760
struct Seg {
    int f0, f4, f8;                                        // +0 / +4 / +8
    rgl::vector4 a;                                        // +0xc
    rgl::vector4 b;                                        // +0x1c
    int f2c, f30, f34;                                     // +0x2c / +0x30 / +0x34
    void init();                                           // 0x464760
};

// 0x4647e0: reset of a huge state object (two arrays of 15 records)
struct Terrain {
    void reset();                                          // 0x4647e0
};

// bit-field accessors: word array at this, widths from the power-of-two table at 0x5a8e70
extern unsigned __int64 g_pow2[];                          // 0x5a8e70 (g_pow2[n] = 2 << n)
extern unsigned g_mask7;                                   // 0x5a8ea0 (= g_pow2[6])
struct BitWords {
    unsigned w[8];
    void set(const unsigned& lo, const int& hi, const unsigned& v);   // 0x464960
    unsigned get(const unsigned& lo, const int& hi);                  // 0x4649b0
};
struct Pct {
    char pad0[0x10];
    unsigned bits;                                         // +0x10
    void set_pct(const float& f);                          // 0x4649f0
};
#endif
