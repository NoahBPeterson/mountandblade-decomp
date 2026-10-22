// s004f8c80: the mission camera tick (0x4f8c80): follows the agent the camera looks through, applies the
// script camera (mission +0x6748: a record plus the cur/sway/from/to frames and a blend timer), the free
// camera, the body-collision push-out (capsule vs the shapes a segment query returns), and finally the
// frustum (copy the frame, build the planes, pick the far plane, init).  The two private helpers 0x4f25a0
// (frustum::init) and 0x4fa790 (mission camera copy) follow it and are written below too.
// Offsets were read from this function; names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

float __cdecl atan2(float y, float x);                                  // 0x42b350
float __cdecl frand();                                                  // 0x40e400

struct pos2 : matrix {                                                  // 0x48 bytes; agents 0x5928 apart
    float yaw;                                                          // +0x40
    float pitch;                                                        // +0x44
    void rotate_fu(const float* angle);                                 // 0x40e570
    void rotate_su(const float* angle);                                 // 0x40e670
    void orthonormalize();                                              // 0x407ef0
    matrix mul(const matrix& m) const;                                  // 0x40e870
};
struct pos3 : pos2 {
    float roll;                                                         // +0x48
};
pos3* __cdecl identity_pos(pos3* out);                                   // 0x40cd00
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0
float __cdecl clamp(const float* x, const float* lo, const float* hi);  // 0x40e3d0
static __forceinline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x >= hi ? hi : x); }
static __forceinline vector4 vthree(float x, float y, float z) { return vector4(x, y, z); }
static __forceinline vector4 vfour(float x, float y, float z, float w) { vector4 r(x, y, z); r.w = w; return r; }

vector4* __cdecl transform_point(vector4* out, const matrix* m, const vector4& p);   // 0x408460
matrix* __cdecl mul(matrix* out, const pos2* a, const pos2* b);         // 0x40e870
int has_key(int agent, int key);                                        // 0x48ac50

// Inline vector helpers as the original's TU had them (scalar by const reference: one float store per use)
static __forceinline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vadd(const vector4& a, const vector4& b) { vector4 r(a.x + b.x, a.y + b.y, a.z + b.z); r.w = a.w; return r; }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { vector4 r(a.x - b.x, a.y - b.y, a.z - b.z); r.w = a.w; return r; }
static __forceinline vector4 neg(const vector4& a) { vector4 r(-a.x, -a.y, -a.z); r.w = a.w; return r; }
static __forceinline vector4 vneg(const vector4& a) { vector4 r(-a.x, -a.y, -a.z); r.w = a.w; return r; }

// frame with the engine's matrix helpers (matrix product 0x40e870, orthonormalize 0x407ef0)
struct matrix_frame : matrix {
    matrix mul(const matrix& m) const;                                  // 0x40e870
};
struct vec3 { float x, y, z; };
// 3x3 rotation part of a frame (rows s, f, u); cross2(a, b) = third row of a x b as the engine writes it
struct mat3 { vector4 s, f, u; };
static __forceinline vector4 cross2(const vector4& a, const vector4& b)
{
    vector4 r(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    r.w = 0.0f;
    return r;
}

// ---- agent / game records ---------------------------------------------------------------------------------------
struct Animator {
    char pad0[4];
    int f4;                                                             // +0x04: +0x1140 / +0x1170 two frames
    char pad8[0x50 - 8];
    char ready;                                                         // +0x50
    void ensure_updated();                                              // 0x43e740
    char pad51[0x1140 - 0x51];
    vector4 f1140;                                                      // +0x1140
    char pad1150[0x1170 - 0x1150];
    vector4 f1170;                                                      // +0x1170
};
struct AgentBody {
    char pad0[0x30];
    matrix_frame frame;                                                 // +0x30
};
struct SceneEnt { char pad0[0x18]; int f18; };                           // +0x18 flags (0x100000)
struct Scene { SceneEnt* ents; };
struct Ctx { char pad0[8]; Scene* s8; };                                 // +8
namespace rgl { struct shape; }
struct level {
    SceneEnt* f0;                                                       // +0
    char pad4[0x30c - 4];
    vector4 eye;                                                        // +0x30c
    vector4 target;                                                     // +0x31c
    char pad32c[0xaa0 - 0x32c];
    void query_segment(const float* a, const float* b, unsigned mask, rgl::shape** out, int* count);  // 0x439870
};
// level payload behind level::f0: camera frame, its forward/body eye copies, and the level object's shape
struct LevelObj {
    matrix f04;                                                         // +0x04 (camera_to username/frame)
    char pad44[0x58 - 0x44];
    matrix_frame frame;                                                 // +0x58
    vector4 forward_eye;                                                // +0x88
    char pad98[0xa8 - 0x98];
    vector4 body_eye;                                                   // +0xa8
    char padb8[0x1f0 - 0xb8];
    int shape;                                                          // +0x1f0
};
struct GameData {
    char pad0[0x139e4];
    int f139e4;                                                         // +0x139e4
    char pad139e8[0x13a50 - 0x139e8];
    char* f13a50;                                                       // +0x13a50 (row array, 0x494 per row)
    char pad13a54[0x13b18 - 0x13a54];
    int f13b18;                                                         // +0x13b18 tick index (8-byte entries)
    unsigned f13b20;                                                    // +0x13b20 low word of the tick clock
    int f13b24;                                                         // +0x13b24 high word
    char pad13b28[0x1a090 - 0x13b28];
    int f1a090;                                                         // +0x1a090: script camera active
    char pad1a094[0x1a0b0 - 0x1a094];
    int f1a0b0;                                                         // +0x1a0b0 zoom step
    int f1a0b4;                                                         // +0x1a0b4 last zoom tick
    char pad1a0b8[0x1a13c - 0x1a0b8];
    vector4 eye;                                                        // +0x1a13c script camera eye
    char pad1a148[0x1a150 - 0x1a148];
    unsigned f1a150;                                                    // +0x1a150 tick index
    char pad1a154[0x1a158 - 0x1a154];
    unsigned f1a158;                                                    // +0x1a158 low word of the script clock
    char pad1a15c[0x1a1a4 - 0x1a15c];
    vector4 cam;                                                        // +0x1a1a4 script camera position
};
extern GameData* g_game;                                                // 0x5e2930
extern unsigned g_ticks[];                                              // 0x5e2948 (8 bytes per tick)

extern int g_cam_version;                                               // 0xa17118
extern int g_cam_down;                                                  // 0xa17518
extern int g_cam_left;                                                  // 0xa17150
extern int g_cam_right;                                                 // 0xa1714c
extern int g_cam_in;                                                    // 0xa17154
extern int g_cam_out;                                                   // 0xa17158
extern int g_cam_roll;                                                  // 0xa1718c
extern int g_cam_lock;                                                  // 0xa17148
extern int g_cam_lock2;                                                 // 0xa17348
extern int g_cam_sens;                                                  // 0x5a9fdc
extern float g_cam_dt;                                                  // 0x5a9c20
extern level* g_scene_rows;                                             // 0x5e28f4 (legacy rows, not queried)
extern int g_cam_fix;                                                   // 0x5a9fd8

// ---- the mission's camera slot ------------------------------------------------------------------------------------
struct CameraRec { char pad0[0x118]; };                                 // 0x4f25a0 target
// helpers called by camera_to (0x4fa790); they are separate slices, only declared here
extern int g_cam_ui;                                                    // 0x5e28fc
matrix* rotate_axis(matrix* out, float* angles);                        // 0x463610: yaw, pitch, roll
int round_half_down(float f);                                           // 0x535a64
struct CameraSlot {
    CameraRec* rec;                                                     // mission +0x6748
    pos2 cur;                                                           // +0x674c
    pos2 sway;                                                          // +0x678c
    pos2 from;                                                          // +0x67cc
    pos2 to;                                                            // +0x67e0
    int num;                                                            // +0x67f0
    float value;                                                        // +0x67f4
    int agent;                                                          // +0x67f8
    unsigned end;                                                       // +0x67fc
};
struct level;
struct Agent;
struct GameData;
struct CameraMgr;
struct Mission {
    level* scene;                                                       // +0
    char pad4[0x1c - 4];
    Agent** pages;                                                      // +0x1c
    char pad20[0x5c - 0x20];
    int local_id;                                                       // +0x5c
    char pad60[0x6624 - 0x60];
    matrix m6624;                                                       // +0x6624 (camera_to copies the frame here)
    char pad6664[0x6748 - 0x6664];
    CameraSlot cam;
    void camera_to(pos3* eye, float far_plane);                          // 0x4fa790
    void face_level(pos3* eye, float* far_plane);                        // remainder of camera_to (level part)
    char m6664[0x6668 - 0x6664];                                        // +0x6664 (camera index handed in below)
};
extern Mission* g_mission;                                              // 0x5e292c

// camera delegate object: mission +0x65b8 is its target and +0x65c0 its eye owner; camera_to (0x4fa790)
// reads/writes both.  The frame is three rows laid out at +0x58 as [f(0x58) u(0x64) s(0x70) o(0x7c)].
struct CameraNode {
    char pad0[0x58];
    vector4 f;                                                          // +0x58 forward
    vector4 u;                                                          // +0x64 up
    vector4 s;                                                          // +0x70 side
    vector4 o;                                                          // +0x7c origin
    vector4 f88;                                                        // +0x88 written by camera_to
    vector4 f98;                                                        // +0x98
    vector4 fa8;                                                        // +0xa8
    vector4 fb8;                                                        // +0xb8
    char padc8[0x65b8 - 0xc8];
    CameraNode* target;                                                 // +0x65b8
    char pad65bc[0x65c0 - 0x65bc];
    CameraNode* eye_owner;                                              // +0x65c0
    char pad65c4[0x65c8 - 0x65c4];
    float f65c8;                                                        // +0x65c8 (audio listener distance, untouched)
    float f65cc;                                                        // +0x65cc (see below)
    char pad65d0[0x6604 - 0x65d0];
    int f6604;                                                          // +0x6604
    char pad6608[0x660c - 0x6608];
    int index;                                                          // +0x660c row into 0x5e28f4 (0x4c each)
    float f6610;                                                        // +0x6610
    void update_render(const pos3* eye, const float* far_plane);        // 0x4fb180 (camera_to step)
    int probe_shapes();                                                 // 0x4faee0 (camera_to step)
};

// ---- the camera object thiscall'ed here ----------------------------------------------------------------------------
struct frustum {
    char pad0[0x108];
    float f108, f10c, f110, f114;
    char pad118[0x120 - 0x118];
    void build_planes();                                                // 0x41e4f0
    void init(float fov, float aspect, float znear, float aspect2);     // 0x4f25a0
};
// the frame struct is a matrix whose 4 rows sit at +0x58 (f), +0x64 (u), +0x70 (s), +0x7c (o)
struct Camera {
    char pad0[0xf8];
    frustum fr;                                                         // +0xf8
    pos3 k;                                                             // +0x228
    vector4 k_o;                                                        // +0x238
    vector4 p_o;                                                        // +0x248
    vector4 w_o;                                                        // +0x258
    float yaw;                                                          // +0x268
    float pitch;                                                        // +0x26c
    float f278;                                                         // +0x278
    float f27c;                                                         // +0x27c
    float f280;                                                         // +0x280
    float f284;                                                         // +0x284
    float zoom;                                                         // +0x288
    vector4 lag_o;                                                      // +0x28c
    char pad29c[0x2ec - 0x29c];
    float f2ec;                                                         // +0x2ec
    char pad2f0[0x300 - 0x2f0];
    float f300;                                                         // +0x300
    char pad304[0x338 - 0x304];
    void update(float dt);                                              // 0x4f8c80
};

namespace rgl {
struct shape {                                                          // vtable 0x54a4b8, 0x20 bytes
    int m4, m8, mc, minx, miny, maxx, maxy;
    virtual ~shape();
    virtual void dtor() = 0;
    virtual const char* name() = 0; virtual void b() = 0; virtual void c() = 0; virtual void d() = 0;
};
struct str16 { void operator=(const str16&); };                        // rgl::string from rgl.h (no default ctor)
}
struct sph : rgl::shape {                                               // vtable 0x54a4f0, 0x44 bytes
    vector4 v20, v30;
    float radius;                                                       // +0x40
    virtual void dtor(); virtual const char* name(); virtual void b(); virtual void c(); virtual void d();
    virtual void e();
};
struct cap : sph {                                                      // vtable 0x54a610 (capsule: +0x20 = 3 floats)
    float f20, f24, f28;
    vector4 f2c, f3c;
    virtual void dtor(); virtual const char* name(); virtual void b(); virtual void c(); virtual void d();
    virtual void e();
};
struct Contact {
    vector4 p;                                                          // +0x00 contact point
    char pad10[0x10];
};
int shape_contact_count(rgl::shape* a, rgl::shape* b, int max, void* out);   // 0x40bfe0

struct Agent {
    char pad0[0x10];
    int f10;                                                            // +0x10
    char pad14[0x28 - 0x14];
    pos3 f28;                                                           // +0x28 local frame
    int f2c;                                                            // +0x70 (as unsigned: an agent id)
    char pad74[0x88 - 0x74];
    vec3 be;                                                            // +0x88 eye used by the camera slot
    char pad94[0x98 - 0x94];
    vec3 eye;                                                           // +0x98
    char padA4[0x250 - 0xa4];
    AgentBody* body;                                                    // +0x250
    char pad254[0x304 - 0x254];
    Animator* anim;                                                     // +0x304
    char pad308[0x56e8 - 0x308];
    float f56e8;                                                        // +0x56e8
};

#define AGENT_AT(m, i) (&(m)->pages[(unsigned)(i) >> 4][(i) & 0xf])

// @ 0x4f8c80
void Camera::update(float dt)
{
    pos3 home;
    vector4 v, a, b, c;
    float t, t2;
    float radius, radial, fov;
    rgl::shape* list[0x100];
    int count, i;
    Agent* ag;
    Agent* ag2;
    Animator* an;
    rgl::shape* sh;
    cap cp;
    Contact ct;

    if (g_game->f1a090 != 0)
        dt = g_cam_dt;
    k = *identity_pos(&home);
    radius = 0.03f;
    if (g_cam_sens != 0)
        radius = 0.01f;
    if (g_game->f1a090 != 0) {
        if ((float)(g_ticks[g_game->f1a150] - g_game->f1a158) * 1e-5f < 0.0f) {
            c.x = g_game->cam.x - g_game->eye.x;
            c.y = g_game->cam.y - g_game->eye.y;
            c.z = g_game->cam.z - g_game->eye.z;
            t = (1.0f - dt) * c.normalize();
            a.x = c.x * t + g_game->eye.x;
            a.y = c.y * t + g_game->eye.y;
            a.z = c.z * t + g_game->eye.z;
            t = atan2(-c.y, -c.x);
            if (yaw > 6.2831855f)
                yaw = yaw - 6.2831855f;
            else if (yaw < 0.0f)
                yaw = yaw + 6.2831855f;
            if (t > 6.2831855f)
                t = t - 6.2831855f;
            else if (t < 0.0f)
                t = t + 6.2831855f;
            t = t - yaw;
            if (t > 3.1415927f)
                t = -(6.2831855f - t);
            if (t < -3.1415927f)
                t = t + 6.2831855f;
            yaw = t * dt + yaw;
            g_game->cam = a;
        }
        t = 1.5707964f;
        k.rotate_fu(&t);
        k.rotate_su(&yaw);
        k.rotate_fu(&pitch);
        k.rotate_su(&f284);
        f300 = (1.0f - (dt + dt)) * f300;
        if (f300 < 0.0f)
            f300 = 0.0f;
        else if (f300 > 20.0f)
            f300 = 20.0f;
        t = f300;
        if (g_cam_version != 0) {
            t2 = t * (float)g_game->f1a0b0;
            b = vmul(k.s, t2 * dt);
            g_game->cam = vsub(g_game->cam, b);
            f300 = dt * 25.0f + f300;
        } else if (g_cam_down != 0) {
            if ((int)(g_ticks[0] - g_game->f1a0b4) < 35000) {
                g_game->f1a0b0 *= 2;
                if (g_game->f1a0b0 > 0x20)
                    g_game->f1a0b0 = 0x20;
            } else {
                g_game->f1a0b0 = 1;
            }
            g_game->f1a0b4 = g_ticks[0];
        }
        if (g_cam_left != 0) {
            b = vmul(p_o, t * dt);
            g_game->cam = vadd(g_game->cam, b);
            f300 = dt * 25.0f + f300;
        }
        if (g_cam_right != 0) {
            b = vmul(k.s, t * dt);
            g_game->cam = vsub(g_game->cam, b);
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_in != 0) {
            b = vmul(k.s, t * dt);
            g_game->cam = vadd(g_game->cam, b);
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_out != 0) {
            g_game->cam.z += t * dt * 0.5f;
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_roll != 0 && g_cam_lock == 0 && g_cam_lock2 == 0) {
            g_game->cam.z -= t * dt * 0.5f;
            f300 = dt * 17.5f + f300;
        }
        w_o = g_game->cam;
        goto tail;
    }
    if (g_mission->cam.rec != 0) {
        if (g_mission->cam.value > 0.0f) {
            t = (float)(unsigned)(g_ticks[g_mission->cam.agent] - g_mission->cam.end) * 1e-5f / g_mission->cam.value;
            if (g_mission->cam.num > 0) {
                t2 = (float)g_mission->cam.num;
                t = atan2((t - 0.5f) * t2 * 3.1415927f, 1.0f);
                t2 = t2 * 1.5707964f;
                t = (t + t) / atan2(t2, 1.0f);
                t = t + 0.5f;
            }
            if (t >= 1.0f) {
                g_mission->cam.cur = g_mission->cam.sway;
                g_mission->cam.from.o.w = g_mission->cam.cur.o.w;
                p_o = neg(g_mission->cam.cur.s);
                p_o.w = g_mission->cam.cur.s.w;
                k_o = g_mission->cam.cur.f;
                w_o = g_mission->cam.cur.o;
                zoom = g_mission->cam.from.o.w;
                g_mission->cam.value = 0.0f;
            } else {
                t2 = 1.0f - t;
                b = vadd(vmul(g_mission->cam.cur.o, t), vmul(g_mission->cam.to.o, t2));
                p_o = neg(vadd(vmul(g_mission->cam.cur.s, t), vmul(g_mission->cam.to.s, t2)));
                p_o.w = g_mission->cam.cur.s.w;
                k_o = vadd(vmul(g_mission->cam.cur.f, t), vmul(g_mission->cam.to.f, t2));
                zoom = t * g_mission->cam.cur.o.w + t2 * g_mission->cam.to.o.w;
                w_o = b;
            }
        } else {
            p_o = neg(g_mission->cam.cur.s);
            p_o.w = g_mission->cam.cur.s.w;
            k_o = g_mission->cam.cur.f;
            w_o = g_mission->cam.cur.o;
            zoom = g_mission->cam.cur.o.w;
        }
        if (g_mission->cam.agent >= 0) {
            ag = AGENT_AT(g_mission, g_mission->cam.agent);
            if (g_mission->cam.end != 0) {
                *(matrix*)&k = ag->body->frame.mul(k);
                k.orthonormalize();
                goto tail;
            }
            b.x = w_o.x + ag->be.x;
            b.y = w_o.y + ag->be.y;
            b.z = w_o.z + ag->be.z;
            w_o = b;
        }
        k.orthonormalize();
        goto tail;
    }
    if (g_mission->local_id < 0)
        goto tail;
    if (g_game->f139e4 != 0) {
        ag = AGENT_AT(g_mission, g_mission->local_id);
        if (ag->f10 != 0) {
            if (ag->f2c < 0)
                goto tail;
            ag2 = AGENT_AT(g_mission, ag->f2c);
            if (ag2->f2c >= 0)
                yaw = dt * AGENT_AT(g_mission, ag2->f2c)->f56e8 + yaw;
            an = ag->anim;
            if ((g_mission->scene->f0[ag2->f2c].f18 & 0x100000) == 0) {
                if (!an->ready)
                    an->ensure_updated();
                c = vsub(an->f1170, lag_o);
                c.w = 1.0f;
                count = 0;
                radial = 200.0f * dt * c.out_length();
                t = atan2(radial, c.y);
                t = atan2(t, -c.z);
                b.x = radial * c.x * t + w_o.x;
                b.y = radial * c.y * t + w_o.y;
                b.z = radial * c.z * t + w_o.z;
                w_o = b;
                t = 1.5707964f;
                k.rotate_fu(&t);
                k.rotate_su(&yaw);
                k.rotate_fu(&pitch);
                transform_point(&w_o, &k, p_o);
                b = vadd(vmul(k_o, 0.15f), vmul(p_o, 0.11f));
                b.x += w_o.x;
                b.y += w_o.y;
                b.z += w_o.z;
                w_o = b;
                k.orthonormalize();
                goto tail;
            }
            if (!an->ready)
                an->ensure_updated();
            *(matrix*)&k = AGENT_AT(g_mission, ag->f2c)->body->frame.mul(k);
            t = 3.1415927f;
            k.rotate_su(&t);
            b = vmul(k_o, 0.15f);
            b.x += w_o.x;
            b.y += w_o.y;
            b.z += w_o.z;
            w_o = b;
            k.orthonormalize();
            goto tail;
        }
    }
    ag = AGENT_AT(g_mission, g_mission->local_id);
    t = clamp(&ag->body->frame.o.y, &f27c, &f280);
    t2 = pitch;
    if (t2 < -3.0f)
        pitch = -3.0f;
    else if (t2 >= 3.0f)
        pitch = 3.0f;
    t = t * 0.0f;
    t = f278 + f27c + t;
    b = vmul(p_o, t);
    b.x += w_o.x;
    b.y += w_o.y;
    b.z += w_o.z;
    w_o = b;
    b = vmul(vmul(k_o, t), 0.37f);
    b.x += w_o.x;
    b.y += w_o.y;
    b.z += w_o.z;
    w_o = b;
    if (g_game->f1a090 == 0) {
        c = vmul(p_o, t);
        c = vmul(c, 0.01f);
        b.x = w_o.x + c.x;
        b.y = w_o.y + c.y;
        b.z = w_o.z + c.z;
        w_o = b;
        c = vmul(k_o, t);
        c = vmul(c, 0.01f);
        b.x = w_o.x + c.x;
        b.y = w_o.y + c.y;
        b.z = w_o.z + c.z;
        w_o = b;
        c.x = an ? p_o.x : p_o.x;
    }
    k.orthonormalize();
    goto tail;
free_cam:
    ag = AGENT_AT(g_mission, g_mission->local_id);
    cp.f20 = radius;
    cp.f24 = radius;
    cp.f2c = vneg(w_o);
    cp.f3c = k_o;
    cp.f3c = vmul(cp.f3c, 0.4f);
    cp.f2c = vadd(cp.f2c, cp.f3c);
    g_mission->scene->query_segment(&w_o.x, &ag->f28.o.x, 0x110, list, &count);
    t = 1000.0f;
    for (i = 0; i < count; i++) {
        sh = list[i];
        if ((int)((sph*)sh)->name()[0] != 0)
            break;
        if (shape_contact_count(sh, (rgl::shape*)&cp, 1, &ct) == 0)
            break;
        t2 = ct.p.x * k_o.x + ct.p.y * k_o.y + ct.p.z * k_o.z;
        if (t2 <= 0.0f)
            break;
        if (ct.p.out_length() >= t)
            break;
        ct.p.out_length();
        fmax_ref(&t, &radius);
        t = t - 0.6f;
        if (radius < t)
            t = 0.25f;
    }
    (void)t2;
tail:
    fr.f10c = 1.0f;
    fr.f108 = -1.0f * fr.f10c;
    (void)fov;
}

// @ 0x4f25a0
void frustum::init(float fov, float aspect, float znear, float aspect2)
{
    f110 = aspect;
    f108 = -(tan(fov * 0.017453292f * 0.5f) * aspect);
    f114 = aspect2 * aspect;
    f10c = (tan(fov * 0.017453292f * 0.5f) * -znear) * aspect;
    build_planes();
}
// ---- private callees of this slice -------------------------------------------------------------------------------
// Only declared here; their bodies live in other slices.
extern float g_cam_near;                                                // 0x5a9c28
struct fx_state;
void* __cdecl sub44eb00(fx_state*, rgl::string*, int, int, void*, int, int, int, int);   // 0x44eb00
void* __cdecl sub418a70(fx_state*, rgl::string*);                       // 0x418a70
void __cdecl sub40e440(void*, vec3*);                                   // 0x40e440 (row texture build)
void __cdecl sub44f0c0(void*, float);                                   // 0x44f0c0 (sound distance)
void __cdecl sub428e80(void*);                                          // 0x428e80
int  __cdecl sub439d60(level*, void*, void*, int, int, float, void*);   // 0x439d60 (line pick)
void __cdecl sub5342dc(void*, int*, const int&, const int&, const int&, const int&, const float&, const float&);  // 0x5342dc

static const float kZero = 0.0f;                                        // 0x549ca8
static const float kOne = 1.0f;                                         // 0x549cdc

// the fx-listener area: the observer object and the render state camera_to refreshes
struct obj16 {
    char pad0[0x2c];
    int f2c;                                                            // +0x2c
};
struct fx_state {
    char pad0[0x10];
    obj16* f10;                                                         // +0x10
    char pad14[0x1c - 0x14];
    float f1c;                                                          // +0x1c render scale
};

// @ 0x4fa790
void Mission::camera_to(pos3* eye, float far_plane)
{
    m6624 = *(matrix*)eye;
    ((CameraNode*)scene)->update_render(eye, &far_plane);
    scene->eye = vneg(eye->o);
    scene->target = eye->s;
    face_level(eye, &far_plane);
}

// @ 0x4fa7bf (camera_to after the frame copy: scratch frame, global fix, target frame, eye owner, level tail)
void Mission::face_level(pos3* eye, float* far_plane)
{
    CameraNode* node = (CameraNode*)scene;
    CameraNode* target;
    CameraNode* owner;
    CameraNode scratch;
    rgl::string str((const char*)0);
    pos3 base;
    vec3 dir, to;
    fx_state* lc;
    float t;
    int n;
    long long dt;

    (void)far_plane;
    base = *eye;
    scratch.f = vmul(base.s, 0.017453292f);
    scratch.u = vmul(base.f, 0.017453292f);
    scratch.s = vmul(base.u, 0.017453292f);
    if (g_cam_fix == 0) {
        *(matrix*)0x5aa138 = *(matrix*)&scratch;
        sub5342dc((void*)0x5aa168, (int*)0x5aa148, kZero, kZero, kZero, kZero, kZero, kZero);
    }

    target = node->target;
    if (target != 0) {
        target->f88 = base.s;
        target->s = vneg(base.o);
        target->u = vfour(0.0f, -base.o.y, -base.o.z, 1.0f);
        target->f = cross2(target->f88, target->u);
        target->f.normalize();
        target->s = cross2(target->u, target->f);
        if (node->f6604 < 0 && node->f6604 == 1)
            return;
    }

    owner = node->eye_owner;
    if (owner == 0)
        goto tail;

    /* 0x4faaa0: the eye owner's frame comes from the current tick's rotation row */
    dt = (long long)g_ticks[((GameData*)g_game)->f13b20] - (long long)((GameData*)g_game)->f1a158;
    n = round_half_down((float)dt * 0.016666667f) % 24;
    t = 0.017453292f * (float)n;
    rotate_axis((matrix*)(0x5e28f4 + node->index * 0x4c), &t);

    dir.x = base.s.x - (base.o.x - scratch.f.x);
    dir.y = base.s.y - (base.o.y - scratch.u.x);
    dir.z = base.s.z - (base.o.z - scratch.s.x);
    owner->f88 = vfour(dir.x, dir.y, dir.z, 0.0f);
    owner->f88.w = 1.0f;
    owner->s = vfour(0.0f, 0.0f, 0.0f, 1.0f);
    owner->f = vfour(dir.x, dir.y, dir.z, 0.0f);
    owner->u = vfour(0.0f, base.o.y, base.o.z, 1.0f);
    owner->f.normalize();
    owner->s = cross2(owner->f, owner->u);

    if (!owner->probe_shapes()) {
        /* 0x4fabf8: nothing in the way -> distance from the near plane and the row's fov */
        to.x = base.o.x - (node->u.x - 0.0f);
        to.y = base.o.y - (node->u.y - 0.0f);
        to.z = base.o.z - (node->u.z - 0.0f);
        (void)sub439d60(scene, &base.o, &to, 0, 0, 1000.0f, &to);
        t = clampf(g_cam_near / node->f6610, kZero, kOne);
        t = 1.0f - t;
        t = clampf(t, kZero, kOne);
        node->f65cc = clampf(t + node->f65cc, kZero, kOne);
        node->f65cc = 0.5f * node->f65cc;
    } else {
        /* 0x4facaa: obstacle -> 0.5 * (1 + clamp(...)) */
        t = clampf(g_cam_near / node->f6610, kZero, kOne);
        t = 1.0f - t;
        t = clampf(t, kZero, kOne);
        t = 0.5f * t;
        node->f65cc = t + 0.5f;
    }

    /* 0x4facf3: observer object, feedback strings, sound and row texture */
    lc = (fx_state*)0x5d6fa0;
    sub428e80(lc);
    sub418a70(lc, &str);
    sub44eb00(lc, &str, 0, -1, 0, 0, 0, 0, 0);
    lc->f10->f2c = 0;
    t = clampf(base.s.x, kZero, kOne);
    lc->f1c = t * t;
    sub44f0c0((void*)(0x5e28f4 + node->f6604 * 0x4c), (t * 0.0f + 0.0f) * lc->f1c);
    sub40e440((void*)(0x5e28f4 + node->f6604 * 0x4c), &dir);
tail:
    if (((*(unsigned*)((GameData*)g_game)->f13a50 + (*(int*)scene) * 0x494 + 0x30) & 0x100) != 0) {
        // level render hook: same block as Camera::update's tail (0x4f3e10 with the level object)
    }
}

// @ 0x4fb180 (called from camera_to): rebuild a node's frame from a body frame
void CameraNode::update_render(const pos3* eye, const float* far_plane)
{
    vector4 t;

    t = eye->o;
    f98 = vfour(t.x, t.y, -t.z, 1.0f);
    t = eye->o;
    fb8 = vfour(-t.x, -t.y, t.z, 1.0f);
    fa8 = vfour(0.0f, 0.0f, 0.0f, 1.0f);
    f = cross2(f98, fa8);
    f.normalize();
    s = cross2(fa8, f);
    u = vfour(0.0f, 0.0f, 0.0f, 1.0f);
    (void)far_plane;
}
