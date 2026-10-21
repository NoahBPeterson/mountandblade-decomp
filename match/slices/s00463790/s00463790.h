// Slice s00463790: shadow-map manager (ShadowMap: render targets, light camera, frustum fit) plus a colour ramp
// sampler, game-state path helpers and a noise vector. ALL NAMES PROVISIONAL.
#ifndef S00463790_H
#define S00463790_H
#include <stddef.h>
#include <new>
#include <math.h>
#include <windows.h>
#include "rgl.h"
#include "tstring.h"

using rgl::vector4;
using rgl::matrix;

unsigned pack_color(const vector4* c);                       // 0x40e440 (cdecl) 4 floats -> 0xAARRGGBB
void noise_fractal(float freq, float gain, int octaves, int sharpen, const float* p, float* out);   // 0x414fc0

// colour ramp: nstops colours (3 floats each, stride 0x10) from +0x2c, count at +0xac
struct Ramp {
    char pad0[0x2c];
    struct Stop { float c[3]; float pad; } stops[8];       // +0x2c
    int n;                                                  // +0xac
    unsigned eval(float* t);                                // 0x463790
};
extern __int64 g_ticks_1;                                    // 0x5e2950
extern int g_flag_5e28fc;                                    // 0x5e28fc (0: colours are gamma-corrected on output)
extern float g_gamma;                                        // 0x5a9bfc

// 16-byte default block (0, 0.5, 0, 0)
vector4 __stdcall wind_noise();                              // 0x463a80
struct Vec2 { float x, y; Vec2() {} Vec2(float a, float b) : x(a), y(b) {} };
struct Block16 {
    Vec2 p, q;                                               // (0, 0.5), (0, 0)
    Block16();                                               // 0x4638b0
};

struct Agent {                                               // 0x508-byte agent record
    char pad0[0xa0];
    float f_a0;                                              // +0xa0
    char pad_a4[0x508 - 0xa4];
};
struct SceneObjRec {                                         // 0x494-byte record
    char pad0[0x10];
    tstring name;                                            // +0x10
    char pad20[0x494 - 0x20];
};
extern float g_thr_self;                                     // 0xa1813c
extern float g_thr_other;                                    // 0xa18138
extern char g_path_buf[];                                    // 0x5e2480

struct GameState {
    char pad0[0x1c];
    tstring module;                                          // +0x1c
    char pad2c[0x8c - 0x2c];
    int cur_agent;                                           // +0x8c
    char pad90[0x13a00 - 0x90];
    Agent* agents;                                           // +0x13a00
    char pad13a04[0x13a50 - 0x13a04];
    SceneObjRec* sceneobjs;                                  // +0x13a50
    bool agent_low(int idx);                                 // 0x4638f0
    tstring scene_path(int idx);                             // 0x463950
    const char* module_path(const char* sub);                // 0x4639d0
};

// shadow-map render targets and the light camera
struct plane { vector4 pos, normal; };
struct frustum {
    char perspective;           // +0
    char flag1;                 // +1
    char pad2[2];
    int nplanes;                // +4
    plane planes[6];            // +8
    matrix m;                   // +0xc8
    float left, right, top, bottom, nearz, farz;   // +0x108
    void build_planes();        // 0x41e4f0
};
struct LightCam {
    char pad0[0x104];
    char dirty;                 // +0x104
    char pad105[3];
    void* target;               // +0x108 (render target of the light camera)
    frustum fr;                 // +0x10c
    vector4 vmin;               // +0x22c
    vector4 vmax;               // +0x23c
    vector4 size;               // +0x24c
    vector4 center;             // +0x25c
    vector4 wcenter;            // +0x26c
    matrix world;               // +0x27c
    matrix light;               // +0x2bc
};
struct Skin {                   // render target / skin resource (0x41c420)
    char pad0[0x1c];
    unsigned id;                // +0x1c
    tstring file;               // +0x20
    int shader;                 // +0x30
    char pad34[0x48 - 0x34];
    struct { char pad[0x24]; int f24; } *refs[5];   // +0x48
};
struct ResMgr {                 // global at 0x5d6fa0
    Skin* get_shadow_skin(int w, int h, int fmt, int f34);   // 0x41c420
    int resolve_shader(tstring* name);                       // 0x418e90
    void release(void* p);                                   // 0x4175f0
};
extern ResMgr g_resmgr;         // 0x5d6fa0
extern int g_shadow_quality;    // 0x5aa000
extern int g_max_tex_w;         // 0xa17f84
extern int g_max_tex_h;         // 0xa17f88
extern char g_use_hw_shadow;    // 0xa17f24

struct ShadowMap {
    Skin* target;               // +0
    char pad4[4];
    Skin* target2;              // +8
    IUnknown* surf;         // +0xc (system-memory copy of the map, see s004640d0)
    int w, h;                   // +0x10 / +0x14
    int f18, f1c;               // +0x18 / +0x1c
    int w2, h2;                 // +0x20 / +0x24
    int f28;                    // +0x28
    float f2c;                  // +0x2c
    char pad30[0x48 - 0x30];
    frustum fr;                 // +0x48 (copy of the light camera frustum)
    char pad168[0x17a - 0x168];
    char valid;                 // +0x17a
    char pad17b[0x1f0 - 0x17b];
    LightCam* cam;              // +0x1f0
    void release_all();         // 0x463b70
    void pick_size();           // 0x463bc0
    vector4 to_unit(const vector4& p);       // 0x463c20
    void fit_bounds();          // 0x463cc0
    void create_target(Skin** slot);         // 0x463e10
    void setup();               // 0x463e90
    void bind_target();         // 0x464070
};
#endif
