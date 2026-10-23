// Slice s004fa790 (one function): the per-tick update of the "camera + sun + flare" object at 0xa76630
// (__thiscall member, ret 8; called from several places). It stores the camera frame, drives the FMOD listener,
// builds the sun node's frame, and for the flare: takes the sun's edge position from the 24-step edge table,
// points and orthonormalizes the flare node, accumulates the flare amount from a shadow-map sample or a ray
// test, then rebuilds, colours and scales the flare mesh.
// Names are provisional; every callee is declared with its VA and is not defined in this slice.
#include <windows.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- 16-byte engine string (0x42bbe0 ctor, 0x42bc40 dtor) ----------------------------------------------
struct tstring {
    char* p; int size, cap, extra;
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
};

// ---- resources -------------------------------------------------------------------------------------------
struct resource;
struct mesh;
struct resmgr {
    mesh* get_object(const tstring* name);                  // 0x419540
    resource* get_material(const tstring* name);            // 0x418a70
};
extern resmgr g_resmgr;                                     // 0x5d6fa0

struct mesh {                                               // rgl::mesh, 0x260 bytes (s0041bf40/mesh.h)
    char pad0[0x2c];
    resource* res;                                          // +0x2c
    char pad30[0x3c - 0x30];
    unsigned color;                                         // +0x3c
    void clear_geometry();                                  // 0x428e80
    void append_skinned(mesh* other, const matrix* m, int bone, unsigned color, char flip);  // 0x44eb00
    void scale(float s);                                    // 0x44f0c0
};

matrix* identity_pos(matrix* out);                          // 0x40cd00 (cdecl)
unsigned pack_color(const vector4* c);                      // 0x40e440 (cdecl)

// ---- globals ---------------------------------------------------------------------------------------------
extern int g_view_active;                                   // 0x5a9fd8 (0: the sound listener follows us)
extern matrix g_listener;                                   // 0x5aa138
extern float g_listener_pos[3];                             // 0x5aa148/4c/50
extern float g_listener_fwd[3];                             // 0x5aa158/5c/60
extern float g_listener_up[3];                              // 0x5aa168/6c/70 (also the particle spawn point)
extern int g_psys;                                          // 0xa70248 (particle system object)
extern float g_amp;                                         // 0x5a9c28 flare amplitude per second
extern float g_flare_fx;                                    // 0x590b50 flare direction factor
extern float g_flare_time_scale;                            // 0x59393c
extern float g_flare_mesh_scale;                            // 0x590b70
extern __int64 g_ticks[];                                      // 0x5e2948 (the game's 64-bit millisecond clock)

struct Game {
    char pad0[0x13a50];
    unsigned* obj_flags;                                    // +0x13a50 records of 0x494 bytes
    char pad54[0x13b18 - 0x13a54];
    int frame;                                              // +0x13b18 (index into g_ticks)
    char pad1c[0x13b20 - 0x13b1c];
    __int64 frame_start;                                    // +0x13b20 (lo/hi)
    float time_of_day();                                    // 0x4356e0
};
extern Game* g_game;                                        // 0x5e2930

struct FlareKind {                                          // 0x4c-byte kind record array at 0x5e28f4
    char pad0[0x1c];
    float size;                                             // +0x1c
    char pad20[0x4c - 0x20];
};
extern FlareKind* g_kinds;                                  // 0x5e28f4

// ---- callees ---------------------------------------------------------------------------------------------
void step_particles_this(const matrix* frame, int flags);   // 0x4faee0 (thiscall; ecx = this at the call)
int  edge_pos(vector4* out, int t, int day);                // 0x463610 (thiscall, ret 0xc)
int  trace_ray(void* level, const vector4* a, const vector4* b, float len, int a5, int a6);  // 0x439d60
int  create_particles(int a, int b, const float* p, float size, int c);   // 0x4428a0
void instance_shadow(const vector4* p);                     // 0x4f3e10 (thiscall on 0xa76630)

// ---- the 0xa76630 object (only the members touched here; layout provisional) -----------------------------
struct Node {                                               // a scene node: transform pos at +0x58, origin +0x88
    char pad0[0x58];
    matrix pos;
};

struct View {
    int f0;                                                 // +0x0 the camera scene node's object
    char pad4[0x30];
    matrix g_frame;                                         // +0x30 (the same frame, in grid layout)
    char pad70[0x30c - 0x70];
    vector4 neg;                                            // +0x30c (-x, -y, -z of the frame origin)
    vector4 cam;                                            // +0x31c (frame s row)
    char pad32c[0x65b8 - 0x32c];
    Node* sun;                                              // +0x65b8 sun scene node
    char padbc[0x65c0 - 0x65bc];
    Node* flare_node;                                       // +0x65c0 flare scene node
    mesh* flare;                                            // +0x65c4 flare mesh
    float flare_amt;                                        // +0x65cc
    char padd0[0x6604 - 0x65d0];
    int   f6604;                                            // +0x6604
    int   flare_phase;                                      // +0x660c
    float flare_gain;                                       // +0x6610
    char pad14[0x6624 - 0x6614];
    matrix last_frame;                                      // +0x6624
    int f6664;                                              // +0x6664 particle handle (-1: none)
    void update(const matrix* frame, int flags);            // 0x4fa790
    bool shadow_sample(const vector4* p, float* out);       // 0x4faee0
};

// the camera/listener object: three rows s, f, u and origin o from +0, with copies at +0x30
struct CameraNode {
    vector4 grid[6];                                        // +0x00 .. +0x5f
    char pad60[0x30c - 0x60];
    vector4 neg;                                            // +0x30c
    vector4 cam;                                            // +0x31c
};

extern "C" __stdcall void _FSOUND_3D_Listener_SetAttributes(float* pos, float* vel, float fx, float fy, float fz,
                                                            float ux, float uy, float uz);   // 0x5342dc

// @ 0x4fa790
void View::update(const matrix* frame, int flags)
{
    last_frame = *frame;
    step_particles_this((const matrix*)((const char*)frame + 0x30), flags);
    CameraNode* cam = (CameraNode*)(size_t)f0;
    cam->neg = vector4(-frame->o.x, -frame->o.y, -frame->o.z);
    cam->cam = frame->s;
    if (g_view_active == 0) {
        g_listener = *frame;
        _FSOUND_3D_Listener_SetAttributes((float*)&g_listener, 0, g_listener_pos[0], g_listener_pos[1],
                                          g_listener_pos[2], g_listener_fwd[0], g_listener_fwd[1],
                                          g_listener_fwd[2]);
    }
    if (sun) {
        matrix f;
        f.o = frame->s;
        f.s = vector4(-frame->o.x, -frame->o.y, -frame->o.z);
        f.f = vector4(0.0f, 0.0f, 1.0f);
        f.u = cross(f.f, f.s);
        f.s.normalize();
        f.u = cross(f.s, f.f);
        sun->pos = f;
        if (f6664 < 0 && f6604 == 1)
            f6664 = create_particles(8, 3, g_listener_up, 1.0f, 0);
    }
    if (!flare_node)
        return;
    Node* fl = flare_node;
    float tod = g_game->time_of_day();
    bool day = tod >= 4.0f && tod < 20.0f;
    int a = (int)((g_ticks[g_game->frame] - g_game->frame_start) * 0.001f);   // fild; fmul; call __ftol2
    vector4 dir;
    edge_pos(&dir, a % 24, day);
    fl->pos.s = dir;
    fl->pos.o = cam->cam;
    fl->pos.f = vector4(0.0f, 0.0f, 1.0f);
    fl->pos.u = cross(fl->pos.f, fl->pos.s);
    fl->pos.s.normalize();
    fl->pos.u = cross(fl->pos.s, fl->pos.f);
    fl->pos.orthonormalize();
    float vis;
    if (shadow_sample(&cam->cam, &vis)) {
        vis = vis + vis;
        flare_amt = (vis - flare_amt) * g_amp * 10.0f + flare_amt;
    } else {
        vector4 b;
        b.x = cam->cam.x - dir.x * g_flare_fx;
        b.y = cam->cam.y - dir.y * g_flare_fx;
        b.z = cam->cam.z - dir.z * g_flare_fx;
        b.x = b.x * 1000.0f;
        b.y = b.y * 1000.0f;
        b.z = b.z * 1000.0f;
        b.x = cam->cam.x - b.x;
        b.y = cam->cam.y - b.y;
        b.z = cam->cam.z - b.z;
        if (trace_ray((void*)(size_t)g_psys, &cam->cam, &b, 1000.0f, 0, 0))
            flare_amt = g_amp * 3.5f + flare_amt;
        else
            flare_amt = flare_amt - g_amp * 3.5f;
    }
    if (flare_amt < 0.0f)
        flare_amt = 0.0f;
    else if (flare_amt >= 1.0f)
        flare_amt = 1.0f;
    flare->clear_geometry();
    tstring* sample = new tstring("sample_flare");
    matrix id;
    identity_pos(&id);
    mesh* obj = g_resmgr.get_object(sample);
    flare->append_skinned(obj, &id, -1, -1, 0);
    delete sample;
    tstring* flare_name = new tstring("flare");
    flare->res = g_resmgr.get_material(flare_name);
    delete flare_name;
    float k = dir.x * cam->cam.x + dir.y * cam->cam.y + dir.z * cam->cam.z - 0.3f;
    k = k * 1.4285715f;
    if (k < 0.0f)
        k = 0.0f;
    else if (k >= 1.0f)
        k = 1.0f;
    float sc = k * k;
    float d = 1.0f - 150.0f / flare_gain;
    if (d < 0.0f)
        d = 0.0f;
    else if (d >= 1.0f)
        d = 1.0f;
    float sz = g_kinds[flare_phase].size;
    float base = d * sz;
    float in = sc * ((float)flare_amt * base * g_flare_time_scale);
    vector4 col;
    col.x = in;
    col.x = col.x * 1.1f;
    col.y = in;
    col.z = in * 0.8f;
    flare->color = pack_color(&col) | 0xff000000;
    float s2 = sc * sc;
    flare->scale(((s2 * 5.0f + 2.0f) * sz) * 50.0f);
    if (((unsigned*)g_game->obj_flags)[(flare_phase * 0x494 + 0x30) / 4] & 0x100)
        instance_shadow(&cam->cam);
}
