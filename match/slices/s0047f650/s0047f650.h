// Slice s0047f650: the character skin/collect pipeline (EChar::collect 0x47f650, Animator::collect
// 0x47fb10), the scene-object loader (0x47fbd0) and the shadow-map passes (0x47fce0 / 0x47fdd0 / 0x47fef0).
// Render-layer types come from the slice-local mesh.h (copied from s0047c1d0). ALL NAMES ARE PROVISIONAL.
#ifndef S0047F650_H
#define S0047F650_H
#include <stddef.h>
#include <stdio.h>
#include <vector>
#include "rgl.h"
#include "tstring.h"
#include "mesh.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mesh;
using rgl::lod_mesh;
using rgl::res_t;

// ------------------------------------------------------------------ character skinning records

// Colour ramp / material record (0xb0 bytes): the Ramp of 0x463790 plus the two material resources.
struct MatRec {
    char pad0[0x10];
    res_t* res0;                 // 0x10
    res_t* res1;                 // 0x14
    char pad18[0x20 - 0x18];
    int* vals;                   // 0x20
    int* vals_end;               // 0x24
    char pad28[0x2c - 0x28];
    struct Stop { float c[3]; float pad; } stops[8];   // 0x2c
    int n;                       // 0xac
    unsigned eval(float* t);     // 0x463790
};
typedef char matrec_size_check[(sizeof(MatRec) == 0xb0) ? 1 : -1];

// One entry of the animation-set table (0x18 bytes); its mesh is at +0x14.
struct SetRec { char pad0[0x14]; lod_mesh* mesh; };

// One entry of the blend tables at EChar+0x4c / +0x5c (0x38 bytes); layout lives in s0047c1d0.
struct Blend { char b[0x38]; };

// The 0x1f8-byte character record; it doubles as the "skin" whose blend methods live at 0x47c6b0/0x47ddf0.
struct EChar {
    char pad0[0x38];
    lod_mesh* base;              // 0x38
    int bone_count;              // 0x3c
    rgl::bone34* bones;          // 0x40
    SetRec* sets;                // 0x44
    char pad48[4];
    Blend* blend_a;              // 0x4c
    char pad50[0xc];
    Blend* blend_b;              // 0x5c
    char pad60[0x88 - 0x60];
    int set_count;               // 0x88
    lod_mesh* items[0x40];       // 0x8c
    int item_count;              // 0x18c
    int mat_count;               // 0x190
    int variant_count;           // 0x194
    int name_count;              // 0x198
    char pad19c[4];
    tstring* names_a;            // 0x1a0
    char pad1a4[0xc];
    tstring* names_b;            // 0x1b0
    char pad1b4[0xc];
    MatRec* mats;                // 0x1c0
    char pad1c4[0x1f8 - 0x1c4];

    void blend(mesh* m, int slot, const float* weights);                    // 0x47c6b0
    void blend_all(lod_mesh* m, void* set, const float* weights);           // 0x47ddf0
    void build_mirror(mesh* m, int* remap, int* count, char flip);          // 0x47c1d0
    __declspec(noinline) void collect(lod_mesh** a, lod_mesh** b, lod_mesh** c,
                                      const float* weights, int item, int set, int material,
                                      int variant, float scale, float time); // 0x47f650
};
typedef char echar_size_check[(sizeof(EChar) == 0x1f8) ? 1 : -1];

// The animation state (global object at 0x8ed4f60).
struct Animator {
    int set;                     // 0x00
    int item;                    // 0x04
    int cur;                     // 0x08
    int variant;                 // 0x0c
    char pad10[4];
    int material;                // 0x14
    char pad18[4];
    EChar* chars;                // 0x1c
    char pad20[0xc];
    float weights[63];           // 0x2c (one weight per bone, up to +0x128)
    float scale;                 // 0x128
    float time;                  // 0x12c
    void collect(char flag, lod_mesh** a, lod_mesh** b, lod_mesh** c);   // 0x47fb10
};
typedef char animator_weights_check[(offsetof(Animator, scale) == 0x128) ? 1 : -1];
extern Animator g_anim;          // 0x8ed4f60

// ------------------------------------------------------------------ shadow map

// A reference object of a skin resource: loaded flag at +0x18, dword at +0x24.
struct Ref { char pad0[0x18]; char loaded; char pad19[0x24 - 0x19]; int f24; };

// Render-target / texture resource (vtable 0x54abf0 / 0x54aaac, cf. s0041b5a0).
struct Skin {
    char pad0[0x1c];
    unsigned id;                 // 0x1c
    tstring file;                // 0x20
    int shader;                  // 0x30
    char pad34[0x48 - 0x34];
    Ref* refs[5];                // 0x48
};

// The light camera / world context (0x421110 attach, 0x429890 render_scene).
struct World {
    void attach(rgl::scene* view, int rem, int div);    // 0x421110
    void render_scene(rgl::scene* sc, bool reflect);    // 0x429890
};

// The scene registry object at 0x5fe898.
struct SceneReg {
    void select(rgl::scene* sc);                        // 0x47e520
};
extern SceneReg g_scene_reg;     // 0x5fe898

struct ShadowMap {
    Skin* target;                // 0x00
    char pad4[4];
    Skin* target2;               // 0x08
    char padc[4];
    int w;                       // 0x10
    int h;                       // 0x14
    rgl::scene sc;               // 0x18 (0x1d8 bytes)
    World* cam;                  // 0x1f0

    void create_target(Skin** slot);    // 0x463e10
    void setup();                       // 0x463e90
    void bind_target();                 // 0x464070
    void sync_readback();               // 0x4640d0
    __declspec(noinline) void build_hw();   // 0x47fce0
    __declspec(noinline) void build_sw();   // 0x47fdd0
    __declspec(noinline) void render();     // 0x47fef0
};
typedef char shadowmap_layout_check[(offsetof(ShadowMap, sc) == 0x18 &&
                                     offsetof(ShadowMap, cam) == 0x1f0) ? 1 : -1];

// ------------------------------------------------------------------ game state / scene objects

// The engine's binary/text stream (vtable 0x54a740 -> 0x54ae48, dtor trampoline 0x54ad68).
// tstring is written as a POD here so the vptr store can precede the string constructors.
struct btstr {
    int pos;                     // +0
    char* buf;                   // +4
    int cap;                     // +8
    int len;                     // +0xc
    void ctor();                 // 0x42bbd0
    void dtor();                 // 0x42bc40
};

struct bin_reader {
    void* vt;                    // 0x00
    btstr name;                  // 0x04
    int state;                   // 0x14
    char autoflush;              // 0x18
    char pad19[3];
    int flushed;                 // 0x1c
    btstr buf;                   // 0x20
    FILE* fp;                    // 0x30
    char failed;                 // 0x34

    bin_reader()
    {
        vt = (void*)0x54a740;
        name.ctor();
        buf.ctor();
        autoflush = 0;
        flushed = 0;
        failed = 1;
        fp = 0;
        state = 0;
        vt = (void*)0x54ae48;
    }
    ~bin_reader()
    {
        vt = (void*)0x54ad68;
        buf.dtor();
        name.dtor();
    }
    char open_read(const char* file);   // 0x415290
};
typedef char bin_reader_size_check[(sizeof(bin_reader) == 0x38) ? 1 : -1];

// 0x494-byte scene-object record.
struct SceneObjRec {
    char pad0[0x10];
    tstring name;                // 0x10
    char pad20[0x494 - 0x20];
    void load(void* s, int version);    // 0x479000
};

struct GameState {
    char pad0[0x1c];
    tstring module;              // 0x1c
    char pad2c[0x8c - 0x2c];
    int cur_agent;               // 0x8c
    char pad90[0x13a50 - 0x90];
    SceneObjRec* sceneobjs;      // 0x13a50
    tstring scene_path(int idx);                // 0x463950
    const char* module_path(const char* sub);   // 0x4639d0
    void load_scene(int idx);                   // 0x47fbd0
};

struct MsgLog {
    void add(tstring text, int color);          // 0x452c00 (ret 0x14)
};
extern MsgLog g_msglog;          // 0x5d7080

// ------------------------------------------------------------------ globals

extern int g_x5aa034;            // 0x5aa034
extern char g_use_hw_shadow;     // 0xa17f24
extern int g_dinput_enabled;     // 0x5e28fc
extern int g_water_mode;         // 0x5aa084

#endif
