// Slice s00520520 (batch ds1).
//   0x520520  Anim::reset      -- re-seeds an animation's three axes and smooths its 63-float track table.
//   0x520b20  GameState::build_map -- load "map.txt" through a text stream into the global Map.
//   0x520b2d  (spurious Ghidra split: the fall-through body of build_map; see partial.txt)
//   0x520c00  read_map         -- deserialise the vertex/group arrays of a Map from a text stream.
// Class layouts copied from s00423ed0 (V36/V136/Group/Vert), s00416ef0 (mbstream.h) and s0051f1a0 (GameState).
// Names provisional; every offset is read off this slice's disassembly.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <vector>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- tstring / engine text stream (layout from s00416ef0/mbstream.h) ---------------------------------
struct tstring {
    int pos;                                        // +0
    char* buf;                                      // +4
    int cap;                                        // +8
    int len;                                        // +0xc
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    ~tstring();                                     // 0x42bc40
};
void mb_error(const char* fmt, ...);                // 0x42cc80 (cdecl)

// Stream classes with the verified layout of s0041db20/s0041db20.h (vtable 0x54a740 root, 0x54a890 text stream,
// 0x54ac88 text reader).
struct stream_base {
    tstring name;                                   // +4
    int state;                                      // +0x14
    char autoflush;                                 // +0x18
    int flushed;                                    // +0x1c
    tstring buf;                                    // +0x20
    __forceinline stream_base() { autoflush = 0; flushed = 0; }
    virtual void reset();                           // 0
    virtual void rewind();                          // 1
    virtual stream_base& w_stream(stream_base* o) = 0;   // 2
    virtual stream_base& w_float(float* v) = 0;          // 3
    virtual stream_base& w_bool(char* v) = 0;            // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0; // 5
    virtual stream_base& w_i64(__int64* v) = 0;          // 6
    virtual stream_base& w_uint(unsigned* v) = 0;        // 7
    virtual stream_base& w_int(int* v) = 0;              // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;   // 9
    virtual stream_base& w_cstr(const char* s) = 0;      // 10
    virtual stream_base& w_tstr(const tstring* s) = 0;   // 11
    virtual stream_base& slot12(void* v) = 0;            // 12
    virtual stream_base& r_stream(stream_base* o) = 0;   // 13
    virtual stream_base& r_float(float* v) = 0;          // 14 (+0x38)
    virtual stream_base& r_bool(char* v) = 0;            // 15
    virtual stream_base& r_uint(unsigned* v) = 0;        // 16
    virtual stream_base& r_u64(unsigned __int64* v) = 0; // 17
    virtual stream_base& r_i64(__int64* v) = 0;          // 18
    virtual stream_base& r_int(int* v) = 0;              // 19 (+0x4c)
    virtual stream_base& r_byte(char* v) = 0;            // 20
    virtual stream_base& r_tstr(tstring* v) = 0;         // 21
    virtual stream_base& slot22(void* v) = 0;            // 22
    virtual bool at_end();                               // 23
    virtual void set_autoflush(char f);                  // 24
    virtual char get_autoflush();                        // 25
    virtual int flush();                                 // 26
};

#define STREAM_SLOTS \
    virtual stream_base& w_stream(stream_base* o); \
    virtual stream_base& w_float(float* v); \
    virtual stream_base& w_bool(char* v); \
    virtual stream_base& w_u64(unsigned __int64* v); \
    virtual stream_base& w_i64(__int64* v); \
    virtual stream_base& w_uint(unsigned* v); \
    virtual stream_base& w_int(int* v); \
    virtual stream_base& w_byte(unsigned char* v); \
    virtual stream_base& w_cstr(const char* s); \
    virtual stream_base& w_tstr(const tstring* s); \
    virtual stream_base& slot12(void* v); \
    virtual stream_base& r_stream(stream_base* o); \
    virtual stream_base& r_float(float* v); \
    virtual stream_base& r_bool(char* v); \
    virtual stream_base& r_uint(unsigned* v); \
    virtual stream_base& r_u64(unsigned __int64* v); \
    virtual stream_base& r_i64(__int64* v); \
    virtual stream_base& r_int(int* v); \
    virtual stream_base& r_byte(char* v); \
    virtual stream_base& r_tstr(tstring* v); \
    virtual stream_base& slot22(void* v);

struct tstream_b : stream_base {                    // vtable 0x54a890
    FILE* fp;                                       // +0x30
    char failed;                                    // +0x34
    STREAM_SLOTS
    __forceinline tstream_b() {}
    ~tstream_b() {}
};
struct tstream_base : tstream_b {
    __forceinline tstream_base() { failed = 1; fp = 0; state = 0; }
};
struct text_stream : tstream_base {                 // vtable 0x54ac88: text reader
    __forceinline text_stream() {}
    STREAM_SLOTS
    bool open_read_text(const char* path);          // 0x415d60 (ret 4)
};

// ---- map vertex / group stores (layouts from s00423ed0) ---------------------------------------------
struct Vert { vector4 p; vector4 n; int f20; };     // 0x24
struct Group {
    int n;                                          // +0
    int vidx[4];                                    // +4
    char pad14[0x10];                               // +0x14
    int f24;                                        // +0x24
    int link[4];                                    // +0x28
    int f38[4];                                     // +0x38
    int f48, f4c, f50;                              // +0x48
    char pad54[0x10];                               // +0x54
    float f64;                                      // +0x64
    int f68, f6c, f70;                              // +0x68
    vector4 c;                                      // +0x74
    int f84;                                        // +0x84
    Group();                                        // 0x4246f0
};
struct V36 {                                        // vector<Vert>, _Myfirst at +4
    char pad0[4];
    Vert* first; Vert* last; Vert* end;
    void resize(unsigned n, Vert v);                // 0x424740
};
struct V136 {                                       // vector<Group>, _Myfirst at +4
    char pad0[4];
    Group* first; Group* last; Group* end;
    void resize(unsigned n, Group v);               // 0x424cc0
};
struct Store {                                      // the global Map (0xa732b8), 0x68 bytes
    V36 verts;                                      // +0
    V136 groups;                                    // +0x10
    char pad20[0x68 - 0x20];
    Store();                                        // 0x478150
    void post_load();                               // 0x47ee60
};
extern Store* g_map;                                // 0xa732b8
extern char g_map_loaded;                           // 0xa73090

// ---- animation object (0x8ed4f60) -------------------------------------------------------------------
struct Anim {
    int f00;                                        // +0x00
    int f04;                                        // +0x04
    int f08;                                        // +0x08
    int f0c;                                        // +0x0c
    int f10;                                        // +0x10
    int f14;                                        // +0x14
    int f18;                                        // +0x18
    int f1c;                                        // +0x1c
    char pad20[0x2c - 0x20];
    float track[63];                                // +0x2c
    float f128;                                     // +0x128 (weight)
    float f12c;                                     // +0x12c (age)
    void begin(void* a);                            // 0x473800 (ret 4)
    void end(void* a);                              // 0x4eb660 (ret 4)
    void reset(void* a, void* b);                   // 0x520520 (ret 8)
};

// ---- game state (only the fields build_map touches) -------------------------------------------------
struct GameState {
    char pad0[0x139d4];
    void build_map();                               // 0x520b20
    const char* module_path(const char* name);      // 0x4639d0 (ret 4)
};
extern GameState* g_game;                           // 0x5e2930
extern unsigned g_rng;                              // 0x5d7078

int float_to_int(float* p);                         // 0x432540 (cdecl)

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520520 -----------------------------------------------------------------------------------------
void Anim::reset(void* a, void* b)
{
    begin(a);
    int s3 = f0c;
    float s128 = f128;
    float s12c = f12c;
    int s1 = f04;
    int s0 = f00;
    int s5 = f14;

    float save[63];
    float* pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    begin(b);

    int r = rand();
    float t = (float)(f04 - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f04 = s1 + float_to_int(&t);

    r = rand();
    t = (float)(f00 - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f00 = s0 + float_to_int(&t);

    r = rand();
    t = (float)(f14 - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = float_to_int(&t) + s5;
    f14 = v;

    f0c = -1;

    int base = *(int*)(f08 * 0x1f8 + 0x1c0 + f1c);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(f0c - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        f0c = s3 + float_to_int(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        f0c = *(int*)(*(int*)(base + 0x20 + f14 * 0xb0) + k * 4);
    }

    r = rand();
    f128 = (f128 - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    f12c = (f12c - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rng = ((g_rng * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rng % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    end(a);
}

// @ 0x520b20 -----------------------------------------------------------------------------------------
void GameState::build_map()
{
    if (!g_map_loaded) {
        g_map = new Store();
        text_stream s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

stream_base* read_map(stream_base* s, Store* map);

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520520 -----------------------------------------------------------------------------------------
void Anim::reset(void* a, void* b)
{
    begin(a);
    int s3 = f0c;
    float s128 = f128;
    float s12c = f12c;
    int s1 = f04;
    int s0 = f00;
    int s5 = f14;

    float save[63];
    float* pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    begin(b);

    int r = rand();
    float t = (float)(f04 - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f04 = s1 + float_to_int(&t);

    r = rand();
    t = (float)(f00 - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f00 = s0 + float_to_int(&t);

    r = rand();
    t = (float)(f14 - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = float_to_int(&t) + s5;
    f14 = v;

    f0c = -1;

    int base = *(int*)(f08 * 0x1f8 + 0x1c0 + f1c);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(f0c - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        f0c = s3 + float_to_int(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        f0c = *(int*)(*(int*)(base + 0x20 + f14 * 0xb0) + k * 4);
    }

    r = rand();
    f128 = (f128 - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    f12c = (f12c - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rng = ((g_rng * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rng % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    end(a);
}

// @ 0x520b20 -----------------------------------------------------------------------------------------
void GameState::build_map()
{
    if (!g_map_loaded) {
        g_map = new Store();
        text_stream s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

stream_base* read_map(stream_base* s, Store* map);

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520520 -----------------------------------------------------------------------------------------
void Anim::reset(void* a, void* b)
{
    begin(a);
    int s3 = f0c;
    float s128 = f128;
    float s12c = f12c;
    int s1 = f04;
    int s0 = f00;
    int s5 = f14;

    float save[63];
    float* pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    begin(b);

    int r = rand();
    float t = (float)(f04 - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f04 = s1 + float_to_int(&t);

    r = rand();
    t = (float)(f00 - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f00 = s0 + float_to_int(&t);

    r = rand();
    t = (float)(f14 - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = float_to_int(&t) + s5;
    f14 = v;

    f0c = -1;

    int base = *(int*)(f08 * 0x1f8 + 0x1c0 + f1c);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(f0c - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        f0c = s3 + float_to_int(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        f0c = *(int*)(*(int*)(base + 0x20 + f14 * 0xb0) + k * 4);
    }

    r = rand();
    f128 = (f128 - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    f12c = (f12c - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rng = ((g_rng * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rng % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    end(a);
}

// @ 0x520b20 -----------------------------------------------------------------------------------------
void GameState::build_map()
{
    if (!g_map_loaded) {
        g_map = new Store();
        text_stream s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

stream_base* read_map(stream_base* s, Store* map);

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520520 -----------------------------------------------------------------------------------------
void Anim::reset(void* a, void* b)
{
    begin(a);
    int s3 = f0c;
    float s128 = f128;
    float s12c = f12c;
    int s1 = f04;
    int s0 = f00;
    int s5 = f14;

    float save[63];
    float* pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    begin(b);

    int r = rand();
    float t = (float)(f04 - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f04 = s1 + float_to_int(&t);

    r = rand();
    t = (float)(f00 - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f00 = s0 + float_to_int(&t);

    r = rand();
    t = (float)(f14 - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = float_to_int(&t) + s5;
    f14 = v;

    f0c = -1;

    int base = *(int*)(f08 * 0x1f8 + 0x1c0 + f1c);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(f0c - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        f0c = s3 + float_to_int(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        f0c = *(int*)(*(int*)(base + 0x20 + f14 * 0xb0) + k * 4);
    }

    r = rand();
    f128 = (f128 - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    f12c = (f12c - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rng = ((g_rng * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rng % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    end(a);
}

// @ 0x520b20 -----------------------------------------------------------------------------------------
void GameState::build_map()
{
    if (!g_map_loaded) {
        g_map = new Store();
        text_stream s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

stream_base* read_map(stream_base* s, Store* map);

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520520 -----------------------------------------------------------------------------------------
void Anim::reset(void* a, void* b)
{
    begin(a);
    int s3 = f0c;
    float s128 = f128;
    float s12c = f12c;
    int s1 = f04;
    int s0 = f00;
    int s5 = f14;

    float save[63];
    float* pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    begin(b);

    int r = rand();
    float t = (float)(f04 - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f04 = s1 + float_to_int(&t);

    r = rand();
    t = (float)(f00 - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f00 = s0 + float_to_int(&t);

    r = rand();
    t = (float)(f14 - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = float_to_int(&t) + s5;
    f14 = v;

    f0c = -1;

    int base = *(int*)(f08 * 0x1f8 + 0x1c0 + f1c);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(f0c - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        f0c = s3 + float_to_int(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        f0c = *(int*)(*(int*)(base + 0x20 + f14 * 0xb0) + k * 4);
    }

    r = rand();
    f128 = (f128 - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    f12c = (f12c - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rng = ((g_rng * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rng % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    end(a);
}

// @ 0x520b20 -----------------------------------------------------------------------------------------
void GameState::build_map()
{
    if (!g_map_loaded) {
        g_map = new Store();
        text_stream s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

stream_base* read_map(stream_base* s, Store* map);

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520520 -----------------------------------------------------------------------------------------
void Anim::reset(void* a, void* b)
{
    begin(a);
    int s3 = f0c;
    float s128 = f128;
    float s12c = f12c;
    int s1 = f04;
    int s0 = f00;
    int s5 = f14;

    float save[63];
    float* pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    begin(b);

    int r = rand();
    float t = (float)(f04 - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f04 = s1 + float_to_int(&t);

    r = rand();
    t = (float)(f00 - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f00 = s0 + float_to_int(&t);

    r = rand();
    t = (float)(f14 - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = float_to_int(&t) + s5;
    f14 = v;

    f0c = -1;

    int base = *(int*)(f08 * 0x1f8 + 0x1c0 + f1c);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(f0c - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        f0c = s3 + float_to_int(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        f0c = *(int*)(*(int*)(base + 0x20 + f14 * 0xb0) + k * 4);
    }

    r = rand();
    f128 = (f128 - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    f12c = (f12c - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rng = ((g_rng * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rng % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    end(a);
}

// @ 0x520b20 -----------------------------------------------------------------------------------------
void GameState::build_map()
{
    if (!g_map_loaded) {
        g_map = new Store();
        text_stream s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

stream_base* read_map(stream_base* s, Store* map);

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520520 -----------------------------------------------------------------------------------------
void Anim::reset(void* a, void* b)
{
    begin(a);
    int s3 = f0c;
    float s128 = f128;
    float s12c = f12c;
    int s1 = f04;
    int s0 = f00;
    int s5 = f14;

    float save[63];
    float* pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    begin(b);

    int r = rand();
    float t = (float)(f04 - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f04 = s1 + float_to_int(&t);

    r = rand();
    t = (float)(f00 - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f00 = s0 + float_to_int(&t);

    r = rand();
    t = (float)(f14 - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = float_to_int(&t) + s5;
    f14 = v;

    f0c = -1;

    int base = *(int*)(f08 * 0x1f8 + 0x1c0 + f1c);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(f0c - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        f0c = s3 + float_to_int(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        f0c = *(int*)(*(int*)(base + 0x20 + f14 * 0xb0) + k * 4);
    }

    r = rand();
    f128 = (f128 - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    f12c = (f12c - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rng = ((g_rng * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rng % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    end(a);
}

// @ 0x520b20 -----------------------------------------------------------------------------------------
void GameState::build_map()
{
    if (!g_map_loaded) {
        g_map = new Store();
        text_stream s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

stream_base* read_map(stream_base* s, Store* map);

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520520 -----------------------------------------------------------------------------------------
void Anim::reset(void* a, void* b)
{
    begin(a);
    int s3 = f0c;
    float s128 = f128;
    float s12c = f12c;
    int s1 = f04;
    int s0 = f00;
    int s5 = f14;

    float save[63];
    float* pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++)
            save[i + k] = pf[k];
        pf += 9;
    }

    begin(b);

    int r = rand();
    float t = (float)(f04 - s1) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f04 = s1 + float_to_int(&t);

    r = rand();
    t = (float)(f00 - s0) * ((float)(r % 15817) * 6.322311674011871e-05f);
    f00 = s0 + float_to_int(&t);

    r = rand();
    t = (float)(f14 - s5) * ((float)(r % 15817) * 6.322311674011871e-05f);
    int v = float_to_int(&t) + s5;
    f14 = v;

    f0c = -1;

    int base = *(int*)(f08 * 0x1f8 + 0x1c0 + f1c);
    int* q = (int*)(base + 0x1c + v * 0xb0);
    int cnt = q[1];
    int m = (q[2] - cnt) >> 2;
    if (cnt == 0 || m < 1) {
        r = rand();
        t = (float)(f0c - s3) * ((float)(r % 15817) * 6.322311674011871e-05f);
        f0c = s3 + float_to_int(&t);
    } else {
        int k = (m < 2) ? 0 : rand() % m;
        f0c = *(int*)(*(int*)(base + 0x20 + f14 * 0xb0) + k * 4);
    }

    r = rand();
    f128 = (f128 - s128) * ((float)(r % 15817) * 6.322311674011871e-05f) + s128;
    r = rand();
    f12c = (f12c - s12c) * ((float)(r % 15817) * 6.322311674011871e-05f) + s12c;

    pf = track;
    for (int i = 0; i < 0x3f; i += 9) {
        for (int k = 0; k < 9; k++) {
            g_rng = ((g_rng * 0x1c13 + 0xf2f67aaa) * 0x1ddcf5f + 0xf2f67aaa) * 0x1105;
            pf[k] = ((pf[k] - save[i + k]) * ((float)(g_rng % 15817) * 1.2644623348023742e-04f)) * 0.5f + save[i + k];
        }
        pf += 9;
    }

    end(a);
}

// @ 0x520b20 -----------------------------------------------------------------------------------------
void GameState::build_map()
{
    if (!g_map_loaded) {
        g_map = new Store();
        text_stream s;
        const char* path = module_path("map.txt");
        if (!s.open_read_text(path))
            mb_error("Unable to open %s", path);
        if (!s.failed) {
            read_map(&s, g_map);
            s.failed = 1;
        }
    }
}

stream_base* read_map(stream_base* s, Store* map);

// @ 0x520c00 -----------------------------------------------------------------------------------------
__declspec(noinline) stream_base* read_map(stream_base* s, Store* map)
{
    int n;
    s->r_int(&n);
    map->verts.resize(n, Vert());
    int i;
    for (i = 0; i < n; i++) {
        char* p = (char*)map->verts.first + i * 0x24;
        s->r_float((float*)p).r_float((float*)p + 1).r_float((float*)p + 2);
        *(float*)((char*)map->verts.first + i * 0x24 + 0x20) = -1.0f;
    }
    int n2;
    s->r_int(&n2);
    map->groups.resize(n2, Group());
    for (i = 0; i < n2; i++) {
        char* g = (char*)map->groups.first + i * 0x88;
        s->r_int((int*)(g + 0x24));
        s->r_int((int*)(g + 0x68));
        s->r_int((int*)g);
        int nn = *(int*)g;
        for (int j = 0; j < nn; j++)
            s->r_int((int*)(g + 4 + j * 4));
    }
    map->post_load();
    return s;
}

