// Mount & Blade 1.011 - batch ds1, slice s004c17c0.
// Shader/effect setup (0x4c17c0..0x4c1b00), stream copy helpers and the trigger-record arrays.
// Class and record names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <new>             // declares operator new[]/delete[] (0x535482/0x53570d); without it cl calls operator new/delete
#include "../../include/rgl.h"

// tstring {cursor, buf, cap, len}; the ctor at 0x42bbd0 zeroes the last three fields.
// Plain aggregate (no compiler-generated members) with the real methods declared on it.
struct tstring {
    int pos;    // +0
    char* buf;  // +4
    int cap;    // +8
    int len;    // +0xc
    void ctor();                                    // 0x42bbd0
    void ctor_c(const char* c);                     // 0x42bbe0
    void ctor_copy(const tstring& o);               // 0x42c110
    void assign_c(const char* c);                   // 0x42bcb0
    void dtor();                                    // 0x42bc40
    void append(const tstring& o);                  // 0x42c1e0
};
// thin __fastcall wrappers -> plain `lea ecx,[s]; call` at the call site
inline void __fastcall tstring_ctor(tstring* s) { s->ctor(); }                          // 0x42bbd0
inline void __fastcall tstring_ctor_c(tstring* s, const char* c) { s->ctor_c(c); }      // 0x42bbe0
inline void __fastcall tstring_ctor_copy(tstring* s, const tstring& o) { s->ctor_copy(o); } // 0x42c110
inline void __fastcall tstring_assign_c(tstring* s, const char* c) { s->assign_c(c); }  // 0x42bcb0
inline void __fastcall tstring_dtor(tstring* s) { s->dtor(); }                          // 0x42bc40
inline void __fastcall tstring_append(tstring* s, const tstring& o) { s->append(o); }  // 0x42c1e0

struct tvec4 { float x, y, z, w; };

// ---- globals ----
extern IDirect3DDevice9* g_device;          // 0xa17f28
extern ID3DXEffect*      g_effect;          // 0x5e293c
extern __int64 g_ticks[];                   // 0x5e2948 clock table (100 kHz ticks, 8 bytes each)
extern tstring g_shader_dir;                // 0x8ee3c28 shader directory prefix
extern int g_use_shaders;                   // 0x5e28fc
extern unsigned char g_5d75e0;              // 0x5d75e0
extern int g_a70258;                        // 0xa70258 (shadow map size, int)
extern float g_5aa088;                      // 0x5aa088 (shadow map texel ratio)
extern float g_549cdc;                      // 0x549cdc 1.0f
extern IDirect3DTexture9* g_shadow_tex;     // 0xa70254

// ---- engine objects ----
struct resmgr_t {                           // 0x5d6fa0 primary resource manager
    void m418fb0();                         // 0x418fb0
    void m418da0();                         // 0x418da0
    void m4195e0();                         // 0x4195e0
    void m419580(ID3DXEffect* e);           // 0x419580
    void* get_texture(tstring* name);       // 0x418870 -> resource, texture at +0x40
};
extern resmgr_t g_resmgr;                    // 0x5d6fa0
extern char g_5fe8ac[];                      // 0x5fe8ac mapping entries of 0x98 bytes, up to 0x5feba4
extern char g_a70248;                        // 0xa70248 particle system manager

// graphics object holding the device at +0xf8 and the font at +0x348 (open layout)
struct gfx_t {
    char pad0[0xf8];
    IDirect3DDevice9* dev;                   // +0xf8
    char padfc[0x348 - 0xfc];
    void* font;                              // +0x348 (CD3DFont*)
};
extern gfx_t g_gfx;                          // 0x8ee3c48

// Font device-object handlers (all __thiscall).  Declared as members of the font class so the
// compiler emits the plain `mov ecx,self; call` shape the original uses.
struct cfont_t {
    void RestoreDeviceObjects();     // 0x4045b0
    void InvalidateDeviceObjects();  // 0x404900
    void DeleteDeviceObjects();      // 0x404950
};

// Free helper entry points reached through `mov ecx,<self>; call`.
struct gfx_helpers {
    void m4bace0();                                   // 0x4bace0
    void m4bcfb0();                                   // 0x4bcfb0
    void m4bd0c0();                                   // 0x4bd0c0
    void m4bc540();                                   // 0x4bc540 per-entry component reset
    void m4bd830(void* rec, void* s, int idx, int sub);   // 0x4bd830
    void m4bd910(void* rec, void* s, int idx);            // 0x4bd910
    void m4bdb60();                                       // 0x4bdb60
};
void __cdecl gfx_4bd830(void* s, int idx, int sub);              // 0x4bd830 (record = ecx)
void __cdecl gfx_4bd910(void* s, int idx);                       // 0x4bd910 (record = ecx)

struct psys_t {
    void m463bc0();                                       // 0x463bc0
};
extern psys_t* g_psys;                               // 0xa70248
void __cdecl mb_error(const char* msg);           // 0x42cc80

extern "C" int __cdecl mb_sprintf(char* buf, unsigned size, const char* fmt, ...);   // 0x535717

// ---- stream interface ----
struct TriggerRec;
struct GameRec;
struct MouseRec;
struct TriggerVec;
struct GameVec;
struct MouseVec;
struct stream_reader;
struct stream {
    stream_reader* vt;
};
// The same stream seen as a real virtual class (vtable slot n at +4n): read_count is slot 19 (+0x4c).
struct vstream {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18();
    virtual void read_count(int* dst);       // +0x4c (thiscall, ret 4)
};
struct stream_reader {
    void s0(); void s1(); void s2(); void s3(); void s4(); void s5(); void s6(); void s7();
    void s8(); void s9(); void sa(); void sb(); void sc(); void sd(); void se(); void sf();
    void s10(); void s11(); void s12(); void s13(); void s14(); void s15(); void s16();
    void __cdecl read_int(void* dst);  // +0x4c (reads one value into dst)
    void s18();
};

// ---------------------------------------------------------------------------------------------
// Records (shapes proven by the compiler-generated ctors/dtors of this TU).
// ---------------------------------------------------------------------------------------------
struct TimedVal {           // 0x10 bytes: an id and the clock reading taken at it
    int      id;            // +0
    int      pad;           // +4
    __int64  t;             // +8
};

// The clock reading at index c, copied whole (the original emits one load/store pair per half).
inline void set_time(TimedVal& v, int c)
{
    v.t = g_ticks[c];
}

// trigger record (0x78 bytes): two string blocks then three timed values (ctor 0x4c1c20)
struct TriggerRec {
    char     pad0[0xc];     // +0x00  (not initialised)
    int      f0c;           // +0x0c
    int      f10;           // +0x10
    int      f14;           // +0x14
    char     f18;           // +0x18
    tstring  s1;            // +0x1c
    int      f2c;           // +0x2c
    int      f30;           // +0x30
    char     f34;           // +0x34
    tstring  s2;            // +0x38
    TimedVal t0;            // +0x48
    TimedVal t1;            // +0x58
    TimedVal t2;            // +0x68
    TriggerRec();           // 0x4c1c20
};

// game record (0x30 bytes): one string block and one timed value (ctor 0x4c1de0)
struct GameRec {
    int      f0;            // +0x00  (not initialised)
    int      a;             // +0x04
    int      b;             // +0x08
    char     c;             // +0x0c
    tstring  s;             // +0x10
    TimedVal t0;            // +0x20
    GameRec();              // 0x4c1de0
};

// 8-byte vector with an empty user ctor: the 2-element member arrays go through the compiler's array-ctor
// helper (0x401800) with the empty ctor at 0x407c20 (`mov eax, ecx; ret`).
struct V2 {
    float x, y;
    V2() {}                                  // 0x407c20
};

// tstring as a MouseRec member: the real ctor/dtor are the outlined routines 0x42bbd0 / 0x42bc40.
struct mstr {
    int pos; char* buf; int cap; int len;
    mstr() throw();                          // 0x42bbd0
    ~mstr() throw();                         // 0x42bc40
};

struct MouseRec {                            // 0xc0 bytes; ctor and dtor are inlined into 0x4c1f70 / 0x4c1f20
    mstr     s0;      // +0
    int      v10;     // +0x10
    mstr     s1;      // +0x14
    int      v24, v28, v2c, v30, v34, v38, v3c, v40;    // +0x24
    V2       m0[2];   // +0x44
    V2       m1[2];   // +0x54
    V2       m2[2];   // +0x64
    V2       m3[2];   // +0x74
    V2       m4[2];   // +0x84
    char     pad94[0x2c];
    void read(stream& s);                    // 0x4bd990 (thiscall, ret 4)
};

// 0x2c-byte element of the global table at 0x8ee3c28
struct SmallRec {
    tstring s;      // +0
    tstring s2;     // +0x10
    int     v20;    // +0x20
    int     v24;    // +0x24
    int     v28;    // +0x28
};

// Reads the element count out of the stream through vtable slot +0x4c, straight into `dst`.
void __cdecl read_count(void* s, int* dst);   // `mov eax,[s]; push dst; call [eax+0x4c]`

// The vectors themselves are plain counted arrays.
struct TriggerVec {
    int         count;     // +0
    TriggerRec* data;      // +4
    int         cur;       // +8
    void resize(stream& s);
    void stamp(int c);
};
struct GameVec {
    int      count;       // +0
    GameRec* data;        // +4
    int      cur;         // +8
    int  resize(stream& s, int extra);
    void stamp(int c);
};
struct MouseVec {                            // data first (0x4c1f20 reads [ecx])
    MouseRec* data;       // +0
    int       count;      // +4
    int       cur;        // +8
    void clear();
    void resize(stream& s);
};

// =============================================================================================
// Shader/effect setup
// =============================================================================================

// @ 0x4c17c0
HRESULT build_shader()
{
    if (g_effect != 0) {
        g_effect->Release();
        g_effect = 0;
    }
    tstring path;
    tstring_ctor_copy(&path, g_shader_dir);
    tstring prefix;
    tstring_ctor_c(&prefix, "mb.fx");
    tstring_append(&path, prefix);
    tstring_dtor(&prefix);
    WIN32_FIND_DATAA fd;
    if (FindFirstFileA(path.buf, &fd) != INVALID_HANDLE_VALUE) {
        FindClose(&fd);
    } else {
        tstring_assign_c(&path, "mb.fxo");
    }
    HRESULT hr = D3DXCreateEffectFromFileA(g_device, path.buf, 0, 0, 0x800, 0, &g_effect, 0);
    if (hr >= 0) {
        g_resmgr.m418fb0();
        g_psys->m463bc0();
        g_effect->SetFloat("fShadowMapNextPixel", g_549cdc / (float)g_a70258);
        g_effect->SetFloat("fShadowMapSize", (float)g_a70258);
        float g = g_549cdc / g_5aa088;
        tvec4 gamma = { g, g, g, g };
        g_effect->SetVector("output_gamma", (D3DXVECTOR4*)&gamma);
        g_effect->SetVector("output_gamma_inv", (D3DXVECTOR4*)&gamma);
    }
    tstring_dtor(&path);
    return hr;
}

// @ 0x4c1940
int __fastcall gfx_setup(gfx_t* self)
{
    ((cfont_t*)self->font)->RestoreDeviceObjects();
    self->dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    self->dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    ((gfx_helpers*)self)->m4bace0();
    g_device = self->dev;
    g_device->BeginScene();
    if (g_use_shaders) {
        if (build_shader() < 0)
            mb_error("Unable to compile the effect file.");
    }
    ((gfx_helpers*)0x5fe898)->m4bcfb0();
    g_resmgr.m419580(g_effect);
    if (g_device)
        g_device->SetSamplerState(0, D3DSAMP_MIPFILTER, g_5d75e0);
    tstring name;
    tstring_ctor_c(&name, "cursor");
    void* res = g_resmgr.get_texture(&name);
    IDirect3DTexture9* tex = *(IDirect3DTexture9**)((char*)res + 0x40);
    tstring_dtor(&name);
    IDirect3DSurface9* surf;
    tex->GetSurfaceLevel(0, &surf);
    g_device->SetTexture(1, (IDirect3DBaseTexture9*)surf);
    if (surf)
        surf->Release();
    return 0;
}

// @ 0x4c1a60
int __fastcall gfx_release(gfx_t* self)
{
    ((cfont_t*)self->font)->InvalidateDeviceObjects();
    if (g_effect != 0) {
        g_effect->Release();
        g_effect = 0;
    }
    g_resmgr.m418da0();
    if (g_shadow_tex != 0) {
        g_shadow_tex->Release();
        g_shadow_tex = 0;
    }
    int* m = (int*)0x5fe8ac;
    do {
        ((gfx_helpers*)m)->m4bc540();
        m = (int*)((char*)m + 0x98);
    } while (m < (int*)0x5feba4);
    ((gfx_helpers*)0x5feba4)->m4bc540();
    return 0;
}

// @ 0x4c1ad0
int __fastcall gfx_cleanup(gfx_t* self)
{
    ((cfont_t*)self->font)->DeleteDeviceObjects();
    g_resmgr.m4195e0();
    ((gfx_helpers*)0x5fe898)->m4bd0c0();
    return 0;
}

// @ 0x4c1b00
void gfx_restore()
{
    gfx_release(&g_gfx);
    gfx_setup(&g_gfx);
}

// =============================================================================================
// Copy helpers (stream reads of 0x2c- and 0x204-byte items)
// =============================================================================================

struct Block44 { int d[11]; };               // 0x2c bytes
struct Block204 { int d[0x81]; };            // 0x204 bytes

// @ 0x4c1b20
void __stdcall copy_block44(void* dst, const void* src)
{
    if (dst)
        *(Block44*)dst = *(const Block44*)src;
}

// @ 0x4c1b40
void __stdcall copy_block204(void* dst, const void* src)
{
    if (dst)
        *(Block204*)dst = *(const Block204*)src;
}

// @ 0x4c1b60
char* copy_block204_range(Block204* first, Block204* last, Block204* dst)
{
    try {
        for (; first != last; first++, dst++)
            *dst = *first;
    } catch (...) {
        throw;
    }
    return (char*)dst;
}

// @ 0x4c1bcc
void throw_oom()
{
    void __stdcall _CxxThrowException(void*, void*);
    _CxxThrowException(0, 0);
    __debugbreak();
}

// =============================================================================================
// Records and their arrays
// =============================================================================================

// @ 0x4c1be0
SmallRec* init_small_table(SmallRec* t)
{
    int n = 0x1000;
    do {
        tstring_ctor(&t->s);
        tstring_ctor(&t->s2);
        t = (SmallRec*)((char*)t + 0x2c);
    } while (--n);
    *(int*)t = 0;
    return t;
}

// @ 0x4c1c20
TriggerRec::TriggerRec()
{
    tstring_ctor(&s1);
    f14 = 0;
    f10 = 0;
    f18 = 0;
    tstring_ctor(&s2);
    f30 = 0;
    f2c = 0;
    f34 = 0;
    TimedVal* p = &t0;
    for (int i = 0; i < 3; i++) {
        p->id = 0;
        p->t = g_ticks[0];
        p++;
    }
    f0c = 0;
}

// @ 0x4c1c90
void TriggerVec::resize(stream& s)
{
    read_count(&s, &count);
    TriggerRec* o = data;
    if (o != 0) {
        delete[] o;
        data = 0;
    }
    int n = count;
    // The original calls the compiler's array-ctor helper (?_H@@...) here instead of inlining the
    // ctor loop; spelled explicitly this is the closest reachable shape (see nonmatching.txt).
    TriggerRec* p = (TriggerRec*)operator new(n * sizeof(TriggerRec));
    *(int*)p = n;
    data = p + 1;
    int c = cur;
    for (int i = 0; i < n; i++) {
        TriggerRec* r = data + i;
        r->t0.id = c;
        r->t0.t = g_ticks[c];
        r->t1.id = c;
        r->t1.t = g_ticks[c];
        r->t2.id = c;
        r->t2.t = g_ticks[c];
        gfx_4bd830(&s, i, c);
    }
}

// @ 0x4c1d70
void TriggerVec::stamp(int c)
{
    for (int i = 0; i < count; i++) {
        TriggerRec* r = data + i;
        r->t0.id = c;
        r->t0.t = g_ticks[c];
        r->t1.id = c;
        r->t1.t = g_ticks[c];
        r->t2.id = c;
        r->t2.t = g_ticks[c];
    }
}

// @ 0x4c1de0
GameRec::GameRec()
{
    tstring_ctor(&s);
    b = 0;
    a = 0;
    c = 0;
    t0.id = 0;
    set_time(t0, 0);
}

// @ 0x4c1e10
int GameVec::resize(stream& s, int extra)
{
    read_count(&s, &count);
    int n = count + extra;
    if (n > 0) {
        GameRec* o = data;
        if (o != 0) {
            delete[] o;
            data = 0;
        }
        data = new GameRec[n];
    }
    int c = cur;
    for (int i = 0; i < n; i++) {
        GameRec* r = data + i;
        r->t0.id = c;
        r->t0.t = g_ticks[c];
        if (i < n - extra)
            gfx_4bd910(&s, i);
    }
    count = n;
    return n - extra;
}

// @ 0x4c1ee0
void GameVec::stamp(int c)
{
    for (int i = 0; i < count; i++) {
        char* b = (char*)data + i * 0x30;
        TimedVal* t = (TimedVal*)(b + 0x20);
        t->id = c;
        t->t = g_ticks[c];
    }
}

// @ 0x4c1f20
void MouseVec::clear()
{
    MouseRec* q = data;
    if (q != 0)
        delete[] q;                          // vector deleting dtor: -4 cookie, ~mstr x2 per element, delete[]
}

// @ 0x4c1f70
void MouseVec::resize(stream& s)
{
    ((vstream&)s).read_count(&count);
    MouseRec* o = data;
    if (o != 0) {
        delete[] o;
        data = 0;
    }
    data = new MouseRec[count];
    for (int i = 0; i < count; i++)
        data[i].read(s);
    ((gfx_helpers*)this)->m4bdb60();
}

// @ 0x4c20c0
int read_map_icon(void* self, stream& s)
{
    char buf[0x200];
    s.vt->s16();
    char* name = *(char**)((char*)self + 4);
    if (!name)
        name = (char*)self + 4;
    mb_sprintf(buf, 0x200, "Map icon %s", name);
    GameVec* v = (GameVec*)((char*)self + 0x3c);
    v->resize(s, 0);
    v->stamp(2);
    return 0;
}
