// Shared declarations of slice s004333c0 (provisional names; evidence in comments).
#ifndef S004333C0_H
#define S004333C0_H
#include "rgl.h"
#include <math.h>
using rgl::vector4;
using rgl::matrix;

// rgl string with a read cursor (16 bytes), layout from s00429890/tstring.h
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    tstring(const tstring& o);                  // 0x42c110
    void operator=(const tstring& o);           // 0x42bc50
};
struct res_obj;
struct resmgr { res_obj* get_object(const tstring* name); };   // 0x419540 (global object at 0x5d6fa0)
extern resmgr g_resmgr;                                        // 0x5d6fa0
extern __int64 g_time[];                                       // 0x5e2948 clock table (1e-5 s ticks)

// ---- collision shapes (copied from s0040c670; vtable 0x54a63c is composite) ----
struct stream;
struct pos;
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                            // slot 0
    virtual int shape_type();                           // slot 1
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13();
    virtual void copy_from(const shape_base* o);        // slot 14 (0x54a674 in composite's vtable)
};
struct composite : shape_base {                          // vtable 0x54a63c, size 0x28
    shape_base** bodies; int count;
    composite(const composite& o);   // 0x433470
};

// polygon shape (vtable 0x54a588, size 0xb4): up to 8 vertices from +0x20, plane normal at +0xa0
struct face {
    char hdr[0x20];
    vector4 v[8];                // 0x20
    vector4 normal;              // 0xa0
    int count;                   // 0xb0
    bool contains_xy(const float* p) const;      // 0x4333c0
    float plane_z(const float* p) const;         // 0x433440
};
bool in_box(const vector4* lo, const vector4* hi, const vector4* p);   // 0x4336d0 (cdecl)

// quaternion, memory order w,x,y,z (s00422db0)
struct quaternion {
    float w, x, y, z;
    quaternion() {}
    quaternion(float x_, float y_, float z_, float w_) { x = x_; y = y_; z = z_; w = w_; }
};

// Animated parameter / effect record (0xd8 bytes), see s00422db0 (Curve::phase / shape)
struct Curve {
    void* m0;                           // +0x00
    char pad4[0x10 - 4];
    __int64 m10;                        // +0x10
    char pad18[0x1c - 0x18];
    int cur;                            // +0x1c
    struct Timer { int clock; int pad; __int64 start; } tm;   // +0x20 (clock id, start tick)
    float m30;                          // +0x30
    char pad34[0x40 - 0x34];
    Curve* recs;                        // +0x40 (array of 0xd8-byte records)
    char pad44[0x5c - 0x44];
    int f5c;                            // +0x5c
    char pad60[0x68 - 0x60];
    quaternion* p68;                    // +0x68 (orientation)
    vector4 v6c;                        // +0x6c
    char pad7c[0xa0 - 0x7c];
    float period;                       // +0xa0
    char pada4[4];
    int mode;                           // +0xa8
    int mac;                            // +0xac
    char padb0[4];
    int shape_mode;                     // +0xb4
    char padb8[0xd0 - 0xb8];
    int tbl_idx;                        // +0xd0
    char padd4[0xd8 - 0xd4];
    float phase();                                                  // 0x4231f0 (s00422db0)
    float shape(float x)
    {
        if (shape_mode == 1) return x;
        if (shape_mode == 2) return (float)sin(x * 1.5707964f);
        if (shape_mode == 5) return (float)sqrt(x);
        if (shape_mode == 4) return x * x;
        return 0.0f;
    }
    bool step_reached();                                            // 0x433840
    __declspec(noinline) float unshape(float y);                    // 0x4338e0
    void set_phase(float period);                                   // 0x433930
    void reset_rec(int i);                                          // 0x4339d0
    float rec_shaped_phase(int i);                                  // 0x433ac0
    void set_rec_phase(int i, float p);                             // 0x433b20
};
extern float g_step_tbl[];                                          // 0x5a9c20

// height field (tiles of 16 bytes in columns)
struct HCell { float h; unsigned flags; int id; int id2; };
struct HeightField {
    HCell** cols;                       // +0
    char pad4[0x14 - 4];
    int w, h;                           // +0x14, +0x18
    float cell;                         // +0x1c
    void sample(float x, float y, float* out_h, int* out_id);      // 0x433510
};

// generic scene object with a placement block (names provisional)
struct Obj3 {
    char pad0[0xe8];
    unsigned flags;                     // +0xe8
    char pade8[0x134 - 0xec];
    matrix m134;                        // +0x134
    vector4 v174;                       // +0x174
    vector4 v184;                       // +0x184
    int m194;                           // +0x194
    char pad198[4];
    int m19c, m1a0, m1a4;               // +0x19c
    void set_v184(const vector4* v);                                // 0x433740
    void set_placement(const matrix* m, const vector4* a, const vector4* b, int c);   // 0x433770
    void reset_pending(int v);                                      // 0x433810
};

// message log with a 32-entry ring buffer
struct MsgLog {
    char pad0[0x1714];
    char enabled;                       // +0x1714
    char pad1715[0xb0b8 - 0x1715];
    struct Ring {
        int count;                      // +0xb0b8
        int head;                       // +0xb0bc
        tstring names[32];              // +0xb0c0
        int vals[32];                   // +0xb2c0
    } ring;
    void push(const tstring& s, int v);                             // 0x4334b0
};

// item kind record (0x238 bytes, array at 0x5e29d0): name at +0x20, mesh list (count +0x44, entries of 0x18 from +0x48)
struct ItemMesh { tstring name; unsigned __int64 flags; };      // 0x18
struct ItemKind {
    char pad0[0x20];
    tstring name;                       // +0x20
    char pad30[0x44 - 0x30];
    int mesh_count;                     // +0x44
    ItemMesh meshes[1];                 // +0x48
    tstring mesh_name_for(int bit);     // 0x433bf0
};
#endif
