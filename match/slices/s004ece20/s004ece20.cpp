// Slice s004ece20: the campaign map camera tick and the map widget's constructor / destructor.
//  0x4ece20  Screen::camera_update steps the campaign map camera by dt:
//            * integrates the pan stick (+0x2bc / +0x2c0) and the yaw rate (+0x2c4) at 10x, then the
//              zoom rate (+0x2c8) at 10x into the zoom (+0x2b8), clamped to 3.5 .. 105.  While the
//              tactical-view globals 0xa17148 / 0xa1717c / 0x5a9fbc are all set the pan and yaw are
//              pinned to 0 and the zoom limit becomes 235.
//            * re-marks the parties (0x4ecce0) whenever the zoom changed.
//            * eases the four rates to zero, each through its own (1 - dt*9.8, floored at 0) clamp.
//            * clamps the camera to the +/-50 map rectangle (x: 0x5aa120 / 0x5aa118, y: 0x5aa124 /
//              0x5aa11c) and derives the pitch from the zoom (0.014*zoom + 0.23, clamped 0.1 .. pi/2).
//            * when g_game+0x88 is set, follows the selected party: inside 0.05*zoom of the party
//              centre (+0x30, lifted by 1) it adds 4*dt*delta (delta clamped to +/-10) to the velocity,
//              and outside it the velocity is zeroed.
//            * integrates the velocity into the camera target (+0x290 .. +0x298) and halves it
//              (1 - dt) per frame.
//            * builds the frame: identity (0x40cd00), a +90 degree matrix_rotate_fu (0x40e570), then
//              matrix_rotate_su (0x40e670, -yaw) and matrix_rotate_fu (-pitch).  The eye is the camera
//              target (+0x290) plus zoom * (frame row s + row f), pulled half a row u back; 0x470f40
//              finds the map position under it, which is moved to at least eye.z + 0.5.
//            * writes the frame into the map widget's scene (frustum 0x41e4f0 at +0x1ac), computes the
//              fog distance parameter (from g_game+0x13b3c#0x13b40 and the zoom, with the 1.7 / 1.6 /
//              1.5 range factors), packs the fog colour (0x40e440 at g_game+0x58) and sets the fade
//              (0x4624c0), then stores the inverse frame (scene+0x30c) and the listener position
//              (scene+0x31c), copies the frame into the map label matrix (+0x1ac of the widget) and,
//              the first time only, sets the 3D listener (0x5342dc).
//  0x4ed630  MapWidget ctor  widget base 0x48ae20, vptr 0x598698, embedded scene 0x429740 at +0x17c,
//                            then +0xe0 = 0, +0xe4 = 0, +0xf4 = 1.75, +0xf0 = 0, +0x2e6 = 1, +0x308 = 0x200.
//  0x4ed6b0  MapWidget dtor  vptr 0x598698, scene dtor 0x41f4e0 at +0x17c, widget dtor 0x487c00, then
//                            operator delete when the deleting flag's low bit is set.
// flags /O2 /MT /Gy /TP
#include <new>
#include <string.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;
typedef rgl::matrix mat4;

// ---- the 16-byte engine string -------------------------------------------------------------------------
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
};

// ---- widgets and overlays (see s004dada0 / s004ec4d0) --------------------------------------------------
struct Widget {
    Widget();                                   // 0x48ae20
    virtual void w00(int); virtual void w01(int); virtual void w02(int); virtual void w03(int);
    virtual void w04(int); virtual void w05(int); virtual void w06(int); virtual void w07(int);
    virtual void w08(int); virtual void w09(int); virtual void w10(int); virtual void w11(int);
    virtual void w12(int); virtual void w13(int); virtual void w14(int); virtual void w15(int);
    virtual void w16(int); virtual void w17(int); virtual void w18(int); virtual void w19(int);
    virtual void w20(int); virtual void w21(int);
    virtual void set_pos(const float* p);       // +0x58
    virtual void w23(int); virtual void w24(int);
    virtual void set_scale(const float* p);     // +0x64
    virtual void w26(int); virtual void w27(int);
    virtual void set_size(const float* p);      // +0x6c
    virtual void w29(int);
    virtual void set_flag(const int* f, int extra);   // +0x74
    virtual void w31(int);
    virtual void set_mesh(void* m, int kind);   // +0x7c
    virtual void w33(int); virtual void w34(int); virtual void w35(int); virtual void w36(int);
    virtual void w37(int); virtual void w38(int); virtual void w39(int); virtual void w40(int);
    virtual void w41(int); virtual void w42(int); virtual void w43(int); virtual void w44(int);
    virtual void w45(int); virtual void w46(int); virtual void w47(int); virtual void w48(int);
    virtual void w49(int);
    virtual void set_color(const int* c);       // +0x98
    virtual void w51(int); virtual void w52(int);
    virtual void set_text(const void* a, const void* b);    // +0xa4
};

struct Overlay {
    Overlay();                                  // 0x4931a0
    virtual void n00(int); virtual void n01(int); virtual void n02(int); virtual void n03(int);
    virtual void n04(int); virtual void n05(int); virtual void n06(int); virtual void n07(int);
    virtual void n08(int); virtual void n09(int); virtual void n10(int); virtual void n11(int);
    virtual void n12(int); virtual void n13(int); virtual void n14(int); virtual void n15(int);
    virtual void n16(int); virtual void n17(int); virtual void n18(int); virtual void n19(int);
    virtual void n20(int); virtual void n21(int);
    virtual void set_pos(const float* p);       // +0x58
    virtual void n23(int); virtual void n24(int);
    virtual void set_scale(const float* p);     // +0x64
    virtual void n26(int); virtual void n27(int);
    virtual void set_size(const float* p);      // +0x6c
    virtual void n29(int); virtual void n30(int); virtual void n31(int);
    virtual void set_mesh(void* m, int kind);   // +0x7c
    virtual void n33(int); virtual void n34(int); virtual void n35(int); virtual void n36(int);
    virtual void n37(int); virtual void n38(int); virtual void n39(int); virtual void n40(int);
    virtual void n41(int); virtual void n42(int); virtual void n43(int); virtual void n44(int);
    virtual void n45(int); virtual void n46(int); virtual void n47(int); virtual void n48(int);
    virtual void n49(int);
    virtual void set_color(const int* c);       // +0x98
    virtual void n51(int); virtual void n52(int);
    virtual void set_text(const void* a, const void* b);    // +0xa4
    virtual void n54(int);
    virtual void n55(int); virtual void n56(int); virtual void n57(int); virtual void n58(int);
    virtual void n59(int); virtual void n60(int); virtual void n61(int); virtual void n62(int);
    virtual void n63(int); virtual void n64(int); virtual void n65(int); virtual void n66(int);
    virtual void n67(int); virtual void n68(int); virtual void n69(int); virtual void n70(int);
    virtual void n71(int); virtual void n72(int); virtual void n73(int); virtual void n74(int);
    virtual void n75(int); virtual void n76(int); virtual void n77(int); virtual void n78(int);
    virtual void n79(int); virtual void n80(int); virtual void n81(int); virtual void n82(int);
    virtual void n83(int); virtual void n84(int); virtual void n85(int); virtual void n86(int);
    virtual void n87(int); virtual void n88(int); virtual void n89(int); virtual void n90(int);
    virtual void n91(int); virtual void n92(int);
    virtual void set_text2(const void* a, const void* b);   // +0xac
    virtual void add_child(void* c);            // +0xc8
};

template <class T> struct vec {
    void* alloc;
    T* first;
    T* last;
    T* end;
};

// ---- the render scene (0x1d8 bytes, ctor 0x429740; see s00428240/world.h) ------------------------------
struct frustum {
    char b[0x120];
    void build_planes();                        // 0x41e4f0
};
struct scene {
    int f0, f4, f8, fc, f10;                    // 0x00
    float f14;                                  // 0x14
    int f18, f1c, f20, f24, f28;                // 0x18
    float f2c;                                  // 0x2c
    frustum fr;                                 // 0x30
    char pad150[0x30c - 0x150];
    vector4 f30c;                               // +0x30c  the inverse frame
    vector4 f31c;                               // +0x31c  the eye
    char pad32c[0x350 - 0x32c];
    int f350;                                   // +0x350
    scene();                                    // 0x429740
};

// the map widget: 0x358-byte widget with its embedded scene at +0x17c.  (Written with raw arrays so the
// layout does not depend on how the compiler treats `scene` as a member; see MapView below.)
struct MapWidget : Widget {
    char pad178[0x358 - 0x178];                 // +0x178 (all fields are addressed through get_scene()/offsets)
    scene* get_scene() { return (scene*)((char*)this + 0x17c); }
};

// ---- the campaign screen (`this` of 0x4ece20) ----------------------------------------------------------
struct Screen {
    virtual void s00(int); virtual void s01(int); virtual void s02(int); virtual void s03(int);
    virtual void s04(int); virtual void s05(int); virtual void s06(int); virtual void s07(int);
    virtual void s08(int); virtual void s09(int);
    int f4;                                     // +0x4
    char pad8[0x2c - 8];
    Overlay* overlay;                           // +0x2c   map bar
    MapWidget* f30;                             // +0x30   the map widget
    void* f34;                                  // +0x34
    void* f38;                                  // +0x38
    char pad3c[0x170 - 0x3c];
    vector4 camera_pos;                         // +0x170  the frame this screen renders the map with
    vector4 camera_dir;                         // +0x180
    vector4 camera_up;                          // +0x190
    float f1a0;                                 // +0x1a0
    char pad1a4[0x290 - 0x1a4];
    float f290;                                 // +0x290  camera target x
    float f294;                                 // +0x294  camera target y
    float f298;                                 // +0x298  camera target z
    float f29c;                                 // +0x29c
    float f2a0;                                 // +0x2a0  velocity x
    float f2a4;                                 // +0x2a4  velocity y
    float f2a8;                                 // +0x2a8  velocity z
    float f2ac;                                 // +0x2ac
    float f2b0;                                 // +0x2b0  yaw (rad)
    float f2b4;                                 // +0x2b4  pitch (rad)
    float f2b8;                                 // +0x2b8  zoom / height
    float f2bc;                                 // +0x2bc  pan rate x
    float f2c0;                                 // +0x2c0  pan rate y
    float f2c4;                                 // +0x2c4  yaw rate
    float f2c8;                                 // +0x2c8  zoom rate
    void camera_mark_entities();                // 0x4ecce0
    void camera_update(float dt);               // 0x4ece20
};

// 0x4903e0 / 0x48ad00: thiscall on the party manager (g_game + 0xf8), one argument each
struct PartyMgr {
    int is_slot(int slot);                      // 0x4903e0
    void* index_of(int slot);                   // 0x48ad00
};

// ---- game objects -------------------------------------------------------------------------------------
struct Game {
    char pad0[0x58];
    vector4 fog_color;                          // +0x58  fog colour passed to 0x40e440
    char pad68[0x84 - 0x68];
    int selected;                               // +0x84  selected party
    char f88;                                   // +0x88  map live
    char pad89[0x13b3c - 0x89];
    float f13b3c;                               // +0x13b3c
    float f13b40;                               // +0x13b40
    PartyMgr* parties() { return (PartyMgr*)((char*)this + 0xf8); }   // +0xf8
};
struct PartyRec {                               // 0x5ac8-byte party record
    char pad0[0x30];
    vector4 rect;                               // +0x30  map rectangle
};

// ---- globals ------------------------------------------------------------------------------------------
extern Game* g_game;                            // 0x5e2930
extern scene* g_scene;                          // 0xa732b4
extern int g_a17148;                            // 0xa17148
extern int g_a1717c;                            // 0xa1717c
extern int g_5a9fbc;                            // 0x5a9fbc
extern int g_5a9fd8;                            // 0x5a9fd8  listener already set
extern int g_5aa120;                            // 0x5aa120  map min x (-50)
extern int g_5aa118;                            // 0x5aa118  map max x ( 50)
extern int g_5aa124;                            // 0x5aa124  map min y (-50)
extern int g_5aa11c;                            // 0x5aa11c  map max y ( 50)
extern char g_world0[0x28];                     // the strategy world whose +0x28 cell table 0x470f40 reads

// the by-name float constants (the originals live in the read-only pools noted here)
#define C_549D74 (*(float*)0x549d74)             // 10.0
#define C_549D70 (*(float*)0x549d70)             // 0.5
#define C_549CDC (*(float*)0x549cdc)             // 1.0
#define C_549CA8 (*(float*)0x549ca8)             // 0.0
#define C_549D7C (*(float*)0x549d7c)             // 0.1
#define C_54A534 (*(float*)0x54a534)             // 1.5707964
#define C_54A6D0 (*(float*)0x54a6d0)             // 1e-05
#define C_598690 (*(float*)0x598690)             // 105.0
#define C_59868C (*(float*)0x59868c)             // 235.0
#define C_598688 (*(float*)0x598688)             // -1e-05
#define C_590D8C (*(float*)0x590d8c)             // 3.5
#define C_590AF0 (*(float*)0x590af0)             // 9.8
#define C_590F7C (*(float*)0x590f7c)             // 0.014
#define C_590E68 (*(float*)0x590e68)             // 0.23
#define C_54B4D0 (*(float*)0x54b4d0)             // 0.05
#define C_5915A0 (*(float*)0x5915a0)             // -10.0
#define C_5935C8 (*(float*)0x5935c8)             // 4.0
#define C_54B3B8 (*(float*)0x54b3b8)             // 0.4
#define C_590C74 (*(float*)0x590c74)             // 0.08
#define C_590FA4 (*(float*)0x590fa4)             // 1.7
#define C_591AF4 (*(float*)0x591af4)             // 0.94
#define C_590C48 (*(float*)0x590c48)             // 1.6
#define C_5911D8 (*(float*)0x5911d8)             // 0.98
#define C_54B8B0 (*(float*)0x54b8b0)             // 1.5
#define C_549D6C (*(float*)0x549d6c)             // 0.9

// the listener block (written by 0x5342dc, from the map frame)
struct Listener3D {
    vector4 pos;                                // +0 (0x5aa138)
    vector4 vel;                                // +0x10 (0x5aa148)
    vector4 dir;                                // +0x20 (0x5aa158)
    vector4 up;                                 // +0x30 (0x5aa168)
    float extra[2];                             // +0x40 (0x5aa178)
};
extern Listener3D g_listener;                   // 0x5aa138

// the 0x40-byte frame the rotations act on; rotate_fu / rotate_su are member functions of it (0x40e570
// / 0x40e670, thiscall: ecx = the matrix, one stack argument popped by the callee)
struct Matrix {
    vector4 s, f, u, o;
    void rotate_fu(const float* a);             // 0x40e570
    void rotate_su(const float* a);             // 0x40e670
};

// ---- callees ------------------------------------------------------------------------------------------
mat4* identity(void* out);                                  // 0x40cd00 (cdecl, ret 0)
unsigned pack_color(const vector4* c);                      // 0x40e440 (cdecl, ret 0)

struct World;                                               // the strategy world (DAT_00a732b4-ish object)
struct Envelope;                                            // the scene-envelope object 0x4624c0 works on

// 0x470f40: thiscall on the world; ecx = *0x5e2930, two stack args, callee pops 8 bytes
struct World {
    char pad0[0x28];
    void scene_query(void* scene, const vector4* point, vector4* out);      // 0x470f40
};

// 0x41e4f0: thiscall on the frustum, no arguments
struct Frustum {
    void build_planes();
};

// 0x4624c0: thiscall on the envelope (ecx = g_scene both times), two stack args
struct Envelope {
    void set_fade(float* dist, const unsigned* color);
};

void __fastcall scene_ctor(scene* s);                       // 0x429740
void __fastcall scene_dtor(scene* s);                       // 0x41f4e0
void __fastcall widget_dtor(void* w);                       // 0x487c00
extern "C" void __fastcall widget_ctor(void* w);            // 0x48ae20 (this in ECX)

struct vector2 { float x, y; vector2(float a, float b) { x = a; y = b; } };
void __stdcall listener_set(float* pos, float* vel,
                            float dx, float dy, float dz,
                            float ux, float uy, float uz);  // 0x5342dc (FMOD 3; 8 args)

// @ 0x4ece20
void Screen::camera_update(float dt)
{
    // --- pan: the stick and yaw rates are integrated at 10x ---------------------------------------
    f290 = dt * C_549D74 * f2bc + f290;
    f294 = dt * C_549D74 * f2c0 + f294;
    f2b0 = dt * f2c4 * C_549D74 + f2b0;

    float max_zoom = C_598690;                  // 105
    if (g_a17148 != 0 && g_a1717c != 0 && g_5a9fbc != 0) {
        f2b0 = 0.0f;
        f2c4 = 0.0f;
        max_zoom = C_59868C;                    // 235
    }

    // --- zoom -------------------------------------------------------------------------------------
    if (f2c8 > C_54A6D0 || f2c8 < C_598688) {
        f2b8 = dt * f2c8 * C_549D74 + f2b8;
        if (f2b8 < C_590D8C) {
            f2b8 = 3.5f;
        } else if (f2b8 > max_zoom) {
            f2b8 = max_zoom;
        }
        camera_mark_entities();
    }

    // --- ease the four rates back to zero (each through its own 0-floor clamp) ---------------------
    float d = 1.0f - dt * C_590AF0;
    if (d < C_549CA8)
        d = 0.0f;
    f2bc *= d;
    d = 1.0f - dt * C_590AF0;
    if (d < C_549CA8)
        d = 0.0f;
    f2c0 *= d;
    d = 1.0f - dt * C_590AF0;
    if (d < C_549CA8)
        d = 0.0f;
    f2c4 *= d;
    d = 1.0f - dt * C_590AF0;
    if (d < C_549CA8)
        d = 0.0f;
    f2c8 *= d;

    // --- keep the camera over the map --------------------------------------------------------------
    if (f290 < *(float*)&g_5aa120)
        f290 = *(float*)&g_5aa120;
    else if (f290 > *(float*)&g_5aa118)
        f290 = *(float*)&g_5aa118;
    if (f294 < *(float*)&g_5aa124)
        f294 = *(float*)&g_5aa124;
    else if (f294 > *(float*)&g_5aa11c)
        f294 = *(float*)&g_5aa11c;

    // --- the pitch follows the zoom ----------------------------------------------------------------
    f2b4 = f2b8 * C_590F7C + C_590E68;
    if (f2b4 < C_549D7C) {
        f2b4 = 0.1f;
    } else if (f2b4 > C_54A534) {
        f2b4 = 1.5707964f;
    }

    // --- follow the selected party while the map is live -------------------------------------------
    if (g_game->f88) {
        if (g_game->parties()->is_slot(g_game->selected)) {
            PartyRec* p = (PartyRec*)g_game->parties()->index_of(g_game->selected);
            vector4 rect = p->rect;
            rect.z = rect.z + C_549CDC;
            float dx = rect.x - f290;
            float dy = rect.y - f294;
            float dz = rect.z - f298;
            if (f2b8 * 0.05f >= sqrtf(dz * dz + dy * dy + dx * dx)) {
                if (dx < C_5915A0)
                    dx = -10.0f;
                else if (dx > C_549D74)
                    dx = 10.0f;
                if (dy < C_5915A0)
                    dy = -10.0f;
                else if (dy > C_549D74)
                    dy = 10.0f;
                f2a0 += (dx * C_5935C8) * dt;
                f2a4 += (dy * C_5935C8) * dt;
                f2a8 += (dz * C_5935C8) * dt;
            }
        } else {
            f2a0 *= C_549CA8;
            f2a4 *= C_549CA8;
            f2a8 *= C_549CA8;
        }
    }

    // --- integrate the velocity and decay it (1 - 2*dt) -------------------------------------------
    f290 = dt * f2a0 + f290;
    f294 = dt * f2a4 + f294;
    f298 = dt * f2a8 + f298;
    float d2 = 1.0f - (dt + dt);
    f2a0 *= d2;
    f2a4 *= d2;
    f2a8 *= d2;

    // --- the frame: identity, +90deg, -yaw, then -pitch; the eye becomes frame.o --------------------
    Matrix frame;
    identity(&frame.s);
    float ang = 1.5707964f;
    frame.rotate_fu(&ang);
    ang = -f2b0;
    frame.rotate_su(&ang);
    ang = -f2b4;
    frame.rotate_fu(&ang);
    f29c = 0.0f;
    f290 = frame.f.x * f2b8 + f290;
    f294 = frame.f.y * f2b8 + f294;
    f298 = frame.f.z * f2b8 + f298;
    float hz = frame.u.z * C_549D70;
    float hx = f290 - frame.u.x * C_549D70;
    float hy = f294 - frame.u.y * C_549D70;
    float q = 0.0f;
    ((World*)g_game)->scene_query(g_scene, (vector4*)&hx, (vector4*)&q);
    q = q + C_549D70;
    if (f298 < q)
        f298 = q;

    // the fog distance parameter (the 0.9 / 0.94 / 0.98 range steps shift it progressively)
    float g1 = g_game->f13b40 + 0.4f;
    float fogdist = (((g_game->f13b3c + 1.0f) * g1) * g1) * 0.08f / (f2b8 + 10.0f);
    if (g_game->f13b40 > C_549D6C) {
        fogdist = fogdist * C_590FA4;
        if (g_game->f13b40 > C_591AF4) {
            fogdist = fogdist * C_590C48;
            if (g_game->f13b40 > C_5911D8)
                fogdist = fogdist * C_54B8B0;
        }
    }
    unsigned fog = pack_color(&g_game->fog_color);
    ((Envelope*)g_scene)->set_fade(&fogdist, &fog);

    // --- frustum planes, the frame into this+0x238, then the widget's label matrix and the scene ---
    memcpy((char*)this + 0x238, &frame, 0x40);
    ((Frustum*)((char*)this + 0x170))->build_planes();
    MapWidget* w = f30;
    float nx = -frame.f.x;
    memcpy((char*)w + 0x1ac, (char*)this + 0x170, 0x120);
    float ny = -frame.f.y;
    float nz = -frame.f.z;
    scene* sc = (scene*)g_scene;
    sc->f30c.x = nx;
    sc->f30c.y = ny;
    sc->f30c.z = nz;
    sc->f30c.w = frame.s.w;
    sc->f31c.x = f290;
    sc->f31c.y = f294;
    sc->f31c.z = f298;
    sc->f31c.w = f29c;
    if (!g_5a9fd8) {
        *(Matrix*)&g_listener.pos = frame;
        listener_set(&g_listener.up.x, 0, g_listener.vel.x, g_listener.vel.y, g_listener.vel.z,
                     g_listener.dir.x, g_listener.dir.y, g_listener.dir.z);
    }
}

MapWidget* __fastcall ctor_4ed630(MapWidget* w);   // 0x4ed630 (ecx = object)

// @ 0x4ed630
MapWidget* __fastcall ctor_4ed630(MapWidget* w)
{
    widget_ctor(w);
    *(void**)w = (void*)0x598698;
    scene_ctor(w->get_scene());
    *(char*)((char*)w + 0x2e6) = 1;
    *(vector2*)((char*)w + 0xe0) = vector2(0.0f, 0.0f);
    *(vector2*)((char*)w + 0xf0) = vector2(1.0f, 0.75f);
    *(int*)((char*)w + 0x308) = 0x200;
    return w;
}
// @ 0x4ed6b0
void* __fastcall dtor_4ed6b0(MapWidget* w, int, unsigned flags)
{
    *(void**)w = (void*)0x598698;
    scene_dtor(w->get_scene());
    widget_dtor(w);
    if (flags & 1)
        operator delete(w);
    return w;
}
