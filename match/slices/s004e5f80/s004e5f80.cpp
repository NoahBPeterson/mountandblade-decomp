// clang-format off
// Slice s004e5f80: the scene/state object that owns the 32 cached mesh pairs and their per-slot
// scales, plus the two range helpers next to it.
//   0x4e5f80 assign_elems    -- element-wise copy loop (tstring + 0x10-byte block, 0x20-byte element).
//   0x4e5fc0 fill_records    -- fill `count` records from a source array; on an exception it destroys
//                               the records it already built and rethrows (this is where the slice's
//                               EH frame and the 0x4e602b cleanup funclet come from).
//   0x4e602b                 the catch(...) funclet of 0x4e5fc0 (no own symbol, see nonmatching.txt).
//   0x4e6050 BigObj::build   -- the big scene build: camera frustum + the two scenes + the meshes.
//   0x4e64b0 BigObj::reset_geometry -- clear the 32+32 cached meshes, reset their scale/alpha arrays.
//   0x4e6510 BigObj::update  -- pending message flush, the per-slot scale timers and the fade clock.
// Layouts follow the verified neighbours: mesh 0x260 (s00428b10, mesh::clone 0x41c6d0), scene
// (s00428b10: reset 0x428f30), frustum 0x120 (s004291c0: ctor 0x41ef60, build_planes 0x41e4f0),
// identity_pos 0x40cd00 / identity3 0x40cd60 (s0040c670), MsgLog::add 0x452c00 (s004529e0).
// Names are provisional.
#include <windows.h>
#include <math.h>
#include "rgl.h"

// rgl::string: 16 bytes { pos, buf, cap, len } (ctor 0x42bbe0, dtor 0x42bc40); match/include/rgl.h
// has the same object with unverified field names, so declare it here as in the neighbouring slices.
struct tstring {
    int pos;            // +0x00
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    tstring(const char* s);                                 // 0x42bbe0
    tstring(const tstring& o);                              // 0x42c110
    ~tstring();                                             // 0x42bc40
    tstring& operator=(const tstring& o);                   // 0x42bc50
};

// a 0x10-byte block with an out-of-line assignment operator (0x4dc1c0, reached from the same helper
// family as the 0x1c8-byte loading-config records of s004dbf7e).
struct vector16 {
    char pad[0x10];
    vector16& operator=(const vector16& o);                 // 0x4dc1c0
};

// ---- the resources the scene object is built from -----------------------------------------------------

// 0x260 bytes (s00428b10; mesh::clone 0x41c6d0 copies one).
struct mesh {
    void clear_geometry();                                  // 0x428e80
    void render(void* scene, const float* xform);           // 0x413750
    char pad[0x260];
};

// scene::reset 0x428f30 clears the vectors at +0x150/+0x15c/+0x1a4.
struct scene {
    void reset();                                           // 0x428f30
    char pad[0x1d8];
};

// frustum (s004291c0): build_planes 0x41e4f0 reads the half-size fields at +0x108..+0x11c.
struct frustum {
    void build_planes();                                    // 0x41e4f0
    char pad[0x120];
};

// a 2-float pair (the scene build passes these by pointer)
struct vec2 {
    float x, y;
    __forceinline vec2(float a, float b) { x = a; y = b; }
};

// the object inserted into a scene; the scene build calls vtable slot +0x28.
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09();
    virtual void build(scene* s, float* t, const vec2* pos, const vec2* size);  // +0x28
};

void identity_pos(void* m);                                 // 0x40cd00
void* identity3(void* m);                                   // 0x40cd60 (fills 0x30 bytes, returns m)

// the on-screen message log base of the big object (MsgLog::add 0x452c00 takes the text by value).
struct MsgLog {
    void add(tstring text, int flag);                       // 0x452c00
};

extern int g_pending;                                       // 0x5e2a14
extern char g_pending_text[];                               // 0x5e25b0

// ---- the 0x1f8-byte record the two range helpers move around (loading-config style) ------------------

struct Rec {
    void destroy();                                         // 0x4d3720
    char pad[0x1f8];
};

// the builder the fill loop constructs through (0x4e4320: `if (p) p->copy_ctor(src)`).
struct Builder {
    void construct(Rec* p, const Rec* src);                 // 0x4e4320
};

// ---- definitions --------------------------------------------------------------------------------------

// the 0x20-byte element the copy loop moves (tstring at +0, the 0x10-byte block at +0x10)
struct vector16_pair {
    tstring name;                                           // +0x00
    vector16 v;                                             // +0x10
};

// @ 0x4e5f80
void assign_elems(vector16_pair* first, vector16_pair* last, const vector16_pair* src)
{
    for (; first != last; ++first)
        *first = *src;
}

// the 0x1f8-byte element destructor over a range; 0x4e5fc0's catch handler inlines it (this is the
// same helper slice s004e55f0 matched at 0x4e5f50).
static __forceinline void destroy_range(Rec* first, Rec* last)
{
    for (; first != last; ++first)
        first->destroy();
}

// @ 0x4e5fc0
// @ 0x4e602b  (the catch(...) funclet of this function)
void fill_records(Rec* first, unsigned count, const Rec* src, Builder* c)
{
    Rec* start = first;
    try {
        for (; count > 0; --count, ++first)
            c->construct(first, src);
    } catch (...) {
        destroy_range(start, first);
        throw;
    }
}

// the big scene/state object; only the offsets this slice touches are named.
struct BigObj : MsgLog {
    char pad000[0x04];
    Widget** obj_begin;                                     // +0x04
    Widget** obj_end;                                       // +0x08
    char pad00c[0x08];
    vec2* vec_14;                                           // +0x14 (0x16e8/0x16ec read the two floats)
    char pad018[0x2c];
    scene scene1;                                           // +0x44
    frustum frustum74;                                      // +0x74
    char pad194[0x21c - 0x194];
    scene scene2;                                           // +0x21c
    frustum frustum_3f4;                                    // +0x3f4
    char pad514[0x548 - 0x514];
    int h548;                                               // +0x548
    int w54c;                                               // +0x54c
    float f550;                                             // +0x550
    float f558;                                             // +0x558
    float f55c;                                             // +0x55c
    char pad560[0x04];
    void* p564;                                             // +0x564
    void* p568;                                             // +0x568
    void* p56c;                                             // +0x56c
    mesh mesh570;                                           // +0x570
    char pad7d0[0x158c - 0x570 - 0x260];
    void* p158c;                                            // +0x158c

    void build();                                           // 0x4e6050
    void reset_geometry();                                  // 0x4e64b0
    void update(float dt);                                  // 0x4e6510
};

// the 12-float basis + three scalars the scene build hands to mesh::render
struct xform {
    float m[12];                                            // +0x00
    float f30;                                              // +0x30
    float f34;                                              // +0x34
    float f38;                                              // +0x38
};

void scene_walk(void* self, scene* s, float* t);            // 0x4bd170

// @ 0x4e6050
void BigObj::build()
{
    scene* s1 = (scene*)((char*)this + 0x44);
    s1->reset();                                            // 0x428f30

    int h = *(int*)((char*)this + 0x548);
    int w = *(int*)((char*)this + 0x54c);
    float r = (float)w / h;
    float sq = (float)sqrt(r);
    *(float*)((char*)this + 0x55c) = sq;
    *(float*)((char*)this + 0x558) = 1.0f / sq;
    *(float*)((char*)this + 0xb028) = r * 1.3333333730697632f;

    if (*(char*)((char*)this + 0xb0ac))
        *(int*)((char*)this + 0x1d0) |= 0x200;

    *(int*)((char*)this + 0x4c) = h;
    *(int*)((char*)this + 0x44) = 0;
    *(int*)((char*)this + 0x48) = 0;
    *(int*)((char*)this + 0x50) = w;
    *(int*)((char*)this + 0x54) = 0;
    *(float*)((char*)this + 0x58) = 1.0f;

    frustum* fr = (frustum*)((char*)this + 0x3f4);
    *(char*)fr = 0;
    *(int*)((char*)fr + 0x118) = 0;
    *(float*)((char*)fr + 0x11c) = 500.0f;
    *(float*)((char*)fr + 0x108) = -0.5f;
    *(float*)((char*)fr + 0x10c) = 0.5f;
    *(float*)((char*)fr + 0x114) = -0.375f;
    *(float*)((char*)fr + 0x110) = 0.375f;
    fr->build_planes();                                     // 0x41e4f0

    rgl::vector4 o;
    char mslot[0x40];           // one frame slot (the original's ES+0x28) serves as both the
    rgl::matrix* m = (rgl::matrix*)mslot;   // frustum matrix and the render xform below
    o.x = 0.5f;
    o.y = 0.375f;
    o.z = 400.0f;
    identity_pos(mslot);                                    // 0x40cd00
    m->o = o;
    *(rgl::matrix*)((char*)fr + 0xc8) = *m;
    fr->build_planes();
    *(frustum*)((char*)this + 0x74) = *fr;

    scene* s2 = (scene*)((char*)this + 0x21c);
    s2->reset();                                            // 0x428f30
    *(int*)((char*)this + 0x228) = w;
    *(int*)((char*)this + 0x21c) = 0;
    *(int*)((char*)this + 0x220) = 0;
    *(int*)((char*)this + 0x224) = h;
    *(int*)((char*)this + 0x22c) = 0;
    *(float*)((char*)this + 0x230) = 1.0f;
    *(frustum*)((char*)this + 0x24c) = *fr;
    *(int*)((char*)this + 0x3a8) |= 0x1000;

    float t = 0.0f;
    for (unsigned i = 0;; ++i) {
        Widget** data = *(Widget***)((char*)this + 4);
        if (!data)
            break;
        unsigned n = (unsigned)((char*)*(void**)((char*)this + 8) - (char*)data) >> 2;
        if (i >= n)
            break;
        vec2 zero(0.0f, 0.0f);
        vec2 size(1.0f, 0.75f);
        data[i]->build(s1, &t, &zero, &size);
    }
    t += 1.0f;
    scene_walk(this, s2, &t);                               // 0x4bd170

    float ident[12];
    xform* x = (xform*)mslot;

    if (*(void**)((char*)this + 0x158c)) {
        t += 1.0f;
        memcpy(x->m, identity3(ident), 0x30);
        x->f38 = t;
        x->f30 = 0.5f;
        x->f34 = 0.65f;
        ((mesh*)((char*)this + 0x1220))->render(s2, x->m);   // 0x413750
        ((mesh*)((char*)this + 0x1480))->render(s2, x->m);
    }
    if (*(void**)((char*)this + 0x568)) {
        t += 1.0f;
        memcpy(x->m, identity3(ident), 0x30);
        float k = *(float*)((char*)this + 0x16f0);
        for (int r = 0; r < 3; ++r) {
            x->m[r * 4 + 0] *= k;
            x->m[r * 4 + 1] *= k;
            x->m[r * 4 + 2] *= k;
        }
        x->f30 = *(float*)((char*)this + 0x16e8);
        x->f34 = *(float*)((char*)this + 0x16ec);
        x->f38 = t;
        ((mesh*)((char*)this + 0xfb0))->render(s2, x->m);
        for (int i = 0; i < 4; ++i)
            (*(mesh**)((char*)this + 0x1210 + 4 * i))->render(s2, x->m);
        (*(mesh**)((char*)this + 0x568))->render(s2, x->m);
    }
    if (*(void**)((char*)this + 0x56c)) {
        t += 1.0f;
        memcpy(x->m, identity3(ident), 0x30);
        x->f30 = *(float*)((char*)this + 0x16f8);
        x->f34 = *(float*)((char*)this + 0x16fc);
        x->f38 = t;
        ((mesh*)((char*)this + 0x570))->render(s2, x->m);
        (*(mesh**)((char*)this + 0x56c))->render(s2, x->m);
    }
    if (*(void**)((char*)this + 0x564)) {
        t += 1.0f;
        memcpy(x->m, identity3(ident), 0x30);
        x->f30 = *(float*)((char*)this + 0x14);
        x->f34 = *(float*)((char*)this + 0x18);
        x->f38 = t;
        (*(mesh**)((char*)this + 0x564))->render(s2, x->m);
    }
}

// @ 0x4e64b0
void BigObj::reset_geometry()
{
    *(int*)((char*)this + 0x171c) = 0;
    *(int*)((char*)this + 0x1720) = 0;
    for (int i = 0; i < 0x20; ++i) {
        ((float*)((char*)this + 0xaf28))[i] = 1000000.0f;
        ((mesh*)((char*)this + 0x1728))[i].clear_geometry();    // 0x428e80
        ((mesh*)((char*)this + 0x6328))[i].clear_geometry();
        ((float*)((char*)this + 0xafa8))[i] = 0.1f;
    }
}

// @ 0x4e6510
void BigObj::update(float dt)
{
    if (g_pending) {
        add(tstring(g_pending_text), -1);                   // 0x452c00
        g_pending = 0;
    }
    for (int i = 0; i < 0x20; ++i)
        *(float*)((char*)this + 0xaf28 + 4 * i) += dt;
    float v = *(float*)((char*)this + 0x171c) - dt * 0.1f;
    *(float*)((char*)this + 0x171c) = v;
    if (v < 0.0f)
        *(float*)((char*)this + 0x171c) = 0.0f;
}
