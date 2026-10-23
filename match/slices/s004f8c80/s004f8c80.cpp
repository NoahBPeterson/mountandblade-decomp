// s004f8c80: the mission camera tick (0x4f8c80, Camera::update, ret 4).  Each frame it builds the camera frame
// k (this+0x228) and position, from one of: the free/script camera (g_game+0x1a090: keys move g_game+0x1a1a4),
// the mission's scripted camera slot (mission+0x6748: frames cur/sway, zoom blend, optional attachment to an
// agent), the spectator view through an agent (g_game+0x139e4), or the player's third-person camera with the
// capsule push-out against the level's faces.  Then (second pass) a scripted blend towards the previous
// camera, the frustum (aspect from 0x5d75dc / 0x5d75d8, fov clamped, near 0.03) and the mission's camera_to
// (0x4fa790).  The private helpers 0x4f25a0 (frustum::init) and 0x4fa790 follow, as before.
// Offsets were read from this function; names are provisional.
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

#pragma intrinsic(atan2, atan, sqrt)


float __cdecl frand();                                                  // 0x40e400
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0 (larger of the two)
float __cdecl clamp(const float* x, const float* lo, const float* hi);  // 0x40e3d0
matrix* __cdecl identity_pos(matrix* out);                              // 0x40cd00
static __forceinline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x >= hi ? hi : x); }
static __forceinline vector4 vfour(float x, float y, float z, float w) { vector4 r(x, y, z); r.w = w; return r; }

// Inline vector helpers as the original's TU had them (scalar by const reference: one float store per use).
// The fourth lane of every result is left unset (the original copies whole vector4s with an unset w).
static __forceinline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vmulv(const vector4& a, float s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 vneg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }
static __forceinline void vadd_to(vector4& a, const vector4& b) { a.x = a.x + b.x; a.y = a.y + b.y; a.z = a.z + b.z; }
static __forceinline void vsub_to(vector4& a, const vector4& b) { a.x = a.x - b.x; a.y = a.y - b.y; a.z = a.z - b.z; }

// a frame with the engine's matrix helpers
struct frame : matrix {
    matrix mul(const matrix& m) const;                                  // 0x40e870 (hidden return pointer)
    void rotate_fu(const float& angle);                                 // 0x40e570
    void rotate_su(const float& angle);                                 // 0x40e670
    vector4 transform_point(const vector4& p) const;                    // 0x408460 (hidden return pointer)
};
struct vec3 { float x, y, z; };
struct pos3 : matrix { float yaw, pitch, roll; };
struct mat3 { vector4 s, f, u; };
static __forceinline vector4 cross2(const vector4& a, const vector4& b)
{
    vector4 r(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    r.w = 0.0f;
    return r;
}

// ---- collision shapes (layouts verified in s00439100 / s0040c670; the capsule ctor is 0x433310) --------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1), min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                                            // slot 0
    virtual int shape_type();                                           // slot 1 (0 sphere, 1 capsule, 2 face)
};
struct capsule : shape_base {                                           // vtable 0x54a540, size 0x64
    vector4 p0, p1, p2, p3;
    float radius;
    capsule(float r, const vector4* a, const vector4* b);               // 0x433310
};
struct face_shape : shape_base {                                        // vtable 0x54a588, size 0xb4
    vector4 v[8];                                                       // +0x20
    vector4 normal;                                                     // +0xa0
    int count;                                                          // +0xb0
};
struct contact {                                                        // 0x24 bytes
    vector4 normal;
    vector4 point;
    float depth;
};
int __cdecl shape_contact_count(shape_base* a, shape_base* b, int max, void* out);   // 0x40bfe0

// ---- keys / globals ---------------------------------------------------------------------------------------------
struct KeyTable { char down(int key); };                                // 0x48ac50 (thiscall, ret 4)
extern KeyTable g_keys;                                                 // 0xa73730
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
extern int g_cam_fix;                                                   // 0x5a9fd8
extern float g_screen_a;                                                // 0x5d75dc
extern float g_screen_b;                                                // 0x5d75d8
extern __int64 g_ticks[];                                               // 0x5e2948 (8 bytes per tick clock)

// ---- agent / game records ---------------------------------------------------------------------------------------
struct Skeleton {
    int nbones;                                                         // +0
    char* recs;                                                         // +4 (the matrix at +0x1140 is the head bone, +0x1170 its origin)
    char pad8[0x50 - 8];
    char ready;                                                         // +0x50
    void ensure_updated();                                              // 0x43e740
};
struct AgentBody {
    char pad0[0x58];
    frame m;                                                            // +0x58 (m.o at +0x88)
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                                     // +0x304
};
struct DefList { char* defs; int count; };                              // agent +0x12c: 0x30-byte records, flags at +0x18
struct Agent {                                                          // 0x5928 bytes
    char pad0[0x10];
    int f10;                                                            // +0x10
    char pad14[0x2c - 0x14];
    int f2c;                                                            // +0x2c target / mount id, -1 = none
    char pad30[0x50 - 0x30];
    vector4 pos;                                                        // +0x50
    char pad60[0x98 - 0x60];
    float ex, ey, ez;                                                   // +0x98 eye offset
    char pada4[0xd4 - 0xa4];
    int fd4;                                                            // +0xd4 animation slot 0 id (the record index)
    char padd8[0x12c - 0xd8];
    DefList* defs;                                                      // +0x12c
    char pad130[0x250 - 0x130];
    AgentBody* body;                                                    // +0x250
    char pad254[0x56e8 - 0x254];
    float f56e8;                                                        // +0x56e8
    char pad56ec[0x5928 - 0x56ec];
};

struct level {
    char pad0[0x30c];
    vector4 eye;                                                        // +0x30c
    vector4 target;                                                     // +0x31c
    char pad32c[0xaa0 - 0x32c];
    void query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count);  // 0x439870
};
struct GameData {
    char pad0[0x139e4];
    int f139e4;                                                         // +0x139e4: spectating
    char pad139e8[0x13a50 - 0x139e8];
    char* f13a50;                                                       // +0x13a50 (row array, 0x494 per row)
    char pad13a54[0x13b18 - 0x13a54];
    int f13b18, f13b1c;                                                 // +0x13b18
    unsigned f13b20;                                                    // +0x13b20 low word of the tick clock
    int f13b24;                                                         // +0x13b24 high word
    char pad13b28[0x1a090 - 0x13b28];
    int f1a090;                                                         // +0x1a090: free/script camera active
    char pad1a094[0x1a0b0 - 0x1a094];
    int f1a0b0;                                                         // +0x1a0b0 zoom step
    int f1a0b4;                                                         // +0x1a0b4 last zoom tick
    char pad1a0b8[0x1a13c - 0x1a0b8];
    vector4 eye;                                                        // +0x1a13c script camera eye
    char pad1a14c[0x1a150 - 0x1a14c];
    int f1a150;                                                         // +0x1a150 tick clock index
    char pad1a154[0x1a158 - 0x1a154];
    __int64 f1a158;                                                     // +0x1a158 script clock start
    char pad1a160[0x1a1a4 - 0x1a160];
    vector4 cam;                                                        // +0x1a1a4 script camera position
};
extern GameData* g_game;                                                // 0x5e2930

// ---- the mission's camera slot ------------------------------------------------------------------------------------
extern float g_cam_near;                                                // 0x5a9c28
extern int g_cam_ui;                                                    // 0x5e28fc
matrix* rotate_axis(matrix* out, float* angles);                        // 0x463610: yaw, pitch, roll
int round_half_down(float f);                                           // 0x535a64
struct CamSlot {                                                        // mission +0x6748
    void* rec;                                                          // +0x6748 (non-null = scripted camera)
    matrix cur;                                                         // +0x674c rows as stored: f = -forward
    matrix sway;                                                        // +0x678c blend target
    float zoom_cur;                                                     // +0x67cc
    float zoom_to;                                                      // +0x67d0
    float value;                                                        // +0x67d4 blend duration (seconds), 0 = none
    int clock;                                                          // +0x67d8 tick clock index
    int pad67dc;
    __int64 start;                                                      // +0x67e0 blend start tick
    int agent;                                                          // +0x67e8 attached agent id, -1 = none
    int attached;                                                       // +0x67ec 1 = frame is relative to the agent's body
    int num;                                                            // +0x67f0 smoothing strength
};

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
struct Mission {
    level* scene;                                                       // +0
    char pad4[0x18 - 4];
    std::vector<Agent*> pages;                                          // +0x18 (id >> 4 -> page of 16 agents; first at +0x1c)
    char pad28[0x5c - 0x28];
    int local_id;                                                       // +0x5c
    char pad60[0x6624 - 0x60];
    matrix m6624;                                                       // +0x6624 (camera_to copies the frame here)
    char pad6664[0x6748 - 0x6664];
    CamSlot cam;
    void camera_to(const matrix* eye, const float& far_plane);          // 0x4fa790 (ret 8)
    void camera_to(pos3* eye, float far_plane);                         // 0x4fa790, the partial reconstruction below
    void face_level(pos3* eye, float* far_plane);                       // remainder of camera_to (level part)
};
extern Mission* g_mission;                                              // 0x5e292c

static __forceinline Agent* AG(int i) { return &g_mission->pages[(unsigned)i >> 4][i & 0xf]; }

// ---- the camera object thiscall'ed here ----------------------------------------------------------------------------
struct frustum {
    char flag;                                                          // +0x00
    char pad1[0xc8 - 1];
    matrix frame;                                                       // +0xc8
    float f108, f10c, f110, f114;
    float f118, f11c;                                                   // +0x118 / +0x11c (the scale pair stored by init)
    void build_planes();                                                // 0x41e4f0
    void init(float fov, float aspect, float znear, float zfar);        // 0x4f25a0
};
struct View {
    char pad0[0x1ac];
    frustum fr;                                                         // +0x1ac
};
struct Camera {
    char pad0[0x60];
    View* view;                                                         // +0x60
    char pad64[0xf8 - 0x64];
    frustum fr;                                                         // +0xf8
    vector4 head;                                                       // +0x218
    frame k;                                                            // +0x228
    float yaw;                                                          // +0x268
    float pitch;                                                        // +0x26c
    char pad270[0x278 - 0x270];
    float f278;                                                         // +0x278
    float f27c;                                                         // +0x27c
    float f280;                                                         // +0x280
    float f284;                                                         // +0x284
    float zoom;                                                         // +0x288
    vector4 lag;                                                        // +0x28c
    char pad29c[0x2ec - 0x29c];
    float f2ec;                                                         // +0x2ec
    char pad2f0[0x300 - 0x2f0];
    float f300;                                                         // +0x300
    void update(float dt);                                              // 0x4f8c80
};

// @ 0x4f8c80
void Camera::update(float dt)
{
    float radius, zoomL;

    if (g_game->f1a090 != 0)
        dt = g_cam_dt;
    {
        matrix home;
        (matrix&)k = *identity_pos(&home);
    }
    radius = 0.03f;
    if (g_cam_sens != 0)
        radius = 0.01f;
    zoomL = zoom;
    if (g_game->f1a090 != 0) {
        if ((float)(g_ticks[g_game->f1a150] - g_game->f1a158) * 1e-5f < 0.0f) {
            vector4 c;
            c.x = g_game->cam.x - g_game->eye.x;
            c.y = g_game->cam.y - g_game->eye.y;
            c.z = g_game->cam.z - g_game->eye.z;
            vector4 a = vadd(g_game->eye, vmulv(c, (1.0f - dt) * c.normalize()));
            float ya = atan2(-(-c.x), -c.y);
            if (yaw > 6.2831855f)
                yaw = yaw - 6.2831855f;
            else if (yaw < 0.0f)
                yaw = yaw + 6.2831855f;
            if (ya > 6.2831855f)
                ya = ya - 6.2831855f;
            else if (ya < 0.0f)
                ya = ya + 6.2831855f;
            ya = ya - yaw;
            if (ya > 3.1415927f)
                ya = -(6.2831855f - ya);
            if (ya < -3.1415927f)
                ya = ya + 6.2831855f;
            yaw = ya * dt + yaw;
            g_game->cam = a;
        }
    }
    if (g_game->f1a090 != 0) {
        // free camera: keys move g_game->cam along the camera's own axes
        float ang, t, u;
        ang = 1.5707964f;
        k.rotate_fu(ang);
        k.rotate_su(yaw);
        k.rotate_fu(pitch);
        k.rotate_su(f284);
        f300 = (1.0f - (dt + dt)) * f300;
        if (f300 < 0.0f)
            f300 = 0.0f;
        else if (f300 > 20.0f)
            f300 = 20.0f;
        t = f300;
        if (g_cam_version == 0 && g_cam_down != 0) {
            int now = (int)g_ticks[0];
            if (now - g_game->f1a0b4 < 35000) {
                int step = g_game->f1a0b0 * 2;
                if (step >= 0x20)
                    step = 0x20;
                g_game->f1a0b0 = step;
            } else {
                g_game->f1a0b0 = 1;
            }
            g_game->f1a0b4 = now;
        }
        if (g_cam_version != 0) {
            u = (float)g_game->f1a0b0 * t;
            vsub_to(g_game->cam, vmul(vmul(k.u, u), dt));
            f300 = dt * 25.0f + f300;
        }
        if (g_cam_left != 0) {
            vadd_to(g_game->cam, vmul(vmul(k.u, t), dt));
            f300 = dt * 25.0f + f300;
        }
        if (g_cam_right != 0) {
            vsub_to(g_game->cam, vmul(vmul(k.s, t), dt));
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_in != 0) {
            vadd_to(g_game->cam, vmul(vmul(k.s, t), dt));
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_out != 0) {
            g_game->cam.z = t * dt * 0.5f + g_game->cam.z;
            f300 = dt * 17.5f + f300;
        }
        if (g_cam_roll != 0 && g_cam_lock == 0 && g_cam_lock2 == 0) {
            g_game->cam.z = g_game->cam.z - t * dt * 0.5f;
            f300 = dt * 17.5f + f300;
        }
        k.o = g_game->cam;
    } else if (g_mission->cam.rec != 0) {
        // the scripted camera slot: cur -> sway over cam.value seconds
        if (g_mission->cam.value > 0.0f) {
            float t = (float)(g_ticks[g_mission->cam.clock] - g_mission->cam.start) * 1e-5f / g_mission->cam.value;
            if (g_mission->cam.num > 0) {
                float nn = (float)g_mission->cam.num;
                t = atan((t - 0.5f) * nn * 3.1415927f) / (atan(nn * 1.5707964f) * 2.0f) + 0.5f;
            }
            if (t >= 1.0f) {
                g_mission->cam.cur = g_mission->cam.sway;
                g_mission->cam.zoom_cur = g_mission->cam.zoom_to;
                k.u = vneg(g_mission->cam.cur.f);
                k.f = g_mission->cam.cur.u;
                k.s = g_mission->cam.cur.s;
                k.o = g_mission->cam.cur.o;
                zoomL = g_mission->cam.zoom_to;
                g_mission->cam.value = 0.0f;
            } else {
                float u = 1.0f - t;
                k.o = vadd(vmulv(g_mission->cam.sway.o, t), vmulv(g_mission->cam.cur.o, u));
                k.u = vneg(vadd(vmulv(g_mission->cam.sway.f, t), vmulv(g_mission->cam.cur.f, u)));
                k.f = vadd(vmulv(g_mission->cam.sway.u, t), vmulv(g_mission->cam.cur.u, u));
                k.s = vadd(vmulv(g_mission->cam.sway.s, t), vmulv(g_mission->cam.cur.s, u));
                zoomL = g_mission->cam.zoom_to * t + g_mission->cam.zoom_cur * u;
            }
        } else {
            k.u = vneg(g_mission->cam.cur.f);
            k.f = g_mission->cam.cur.u;
            k.s = g_mission->cam.cur.s;
            k.o = g_mission->cam.cur.o;
            zoomL = g_mission->cam.zoom_cur;
        }
        if (g_mission->cam.agent >= 0) {
            if (g_mission->cam.attached != 0) {
                (matrix&)k = AG(g_mission->cam.agent)->body->m.mul(k);
                k.orthonormalize();
                goto pass2;
            }
            vector4* bo = &AG(g_mission->cam.agent)->body->m.o;
            k.o.x = bo->x + k.o.x;
            k.o.y = bo->y + k.o.y;
            k.o.z = bo->z + k.o.z;
        }
        k.orthonormalize();
    } else {
      int id = g_mission->local_id;
      if (id >= 0) {
        if (g_game->f139e4 != 0 && AG(g_mission->local_id)->f10 == 0) {
            // spectating: look through the agent this one is bound to
            Agent* ag = AG(g_mission->local_id);
            AgentBody* body = AG(g_mission->local_id)->body;
            Skeleton* sk;
            if (AG(g_mission->local_id)->f2c >= 0)
                yaw = dt * AG(ag->f2c)->f56e8 + yaw;
            sk = body->skel;
            if ((*(unsigned*)(ag->defs->defs + ag->fd4 * 0x30 + 0x18) & 0x100000) != 0) {
                float ang;
                if (!sk->ready)
                    sk->ensure_updated();
                (matrix&)k = body->m.mul(*(matrix*)(sk->recs + 0x1140));
                ang = 3.1415927f;
                k.rotate_su(ang);
                vadd_to(k.o, vmul(k.f, 0.15f));
            } else {
                float lo = 0.0f, hi = 1.0f, ang, radial;
                vector4 c, n;
                if (!sk->ready)
                    sk->ensure_updated();
                n = *(vector4*)(sk->recs + 0x1170);
                c.x = n.x - lag.x;
                c.y = n.y - lag.y;
                c.z = n.z - lag.z;
                radial = c.out_length() * dt * 200.0f;
                vadd_to(lag, vmulv(c, clamp(&radial, &lo, &hi)));
                k.o = body->m.transform_point(lag);
                ang = 1.5707964f;
                k.rotate_fu(ang);
                k.rotate_su(yaw);
                k.rotate_fu(pitch);
                vadd_to(k.o, vadd(vmul(k.f, 0.15f), vmul(k.u, 0.11f)));
            }
        } else if (id >= 0) {
            // third person: the player's own agent
            Agent* ag = AG(g_mission->local_id);
            float ang, d;
            if (g_keys.down(0xd)) {
                float fa = 0.8f, fb = 0.5f;
                if (AG(g_mission->local_id)->f2c >= 0) {
                    fb = 1.0f;
                    fa = 1.1f;
                }
                f280 = (fa - f280) * dt * 3.0f + f280;
                f27c = (fb - f27c) * dt * 3.0f + f27c;
            } else {
                if (AG(g_mission->local_id)->f2c >= 0) {
                    f27c = (0.1f - f27c) * dt + f27c;
                    d = 2.4f;
                } else {
                    f27c = (0.0f - f27c) * dt + f27c;
                    d = 1.65f;
                }
                f280 = (d - f280) * dt + f280;
            }
            head = ag->pos;
            head.z = head.z + f280;
            ang = 1.5707964f;
            head.x = head.x + ag->ex;
            head.y = ag->ey + head.y;
            head.z = ag->ez + head.z;
            k.rotate_fu(ang);
            k.rotate_su(yaw);
            k.rotate_fu(pitch);
            k.rotate_su(f284);
            k.o = head;
            d = (pitch < -3.0f ? -3.0f : (pitch < 3.0f ? pitch : 3.0f)) * 0.0f + f278 + f27c;
            vadd_to(k.o, vmul(k.u, d));
            vadd_to(k.o, vmul(vmul(k.f, 0.37f), d));
            if (g_game->f1a090 == 0) {
                // keep the camera out of the walls: sweep a thin capsule from the head to the camera
                shape_base* list[0x2000];
                contact ct;
                int count, i;
                float t;
                vector4 p0 = k.o;
                vector4 q = vsub(k.o, vmul(k.u, d));
                vector4 c = vsub(q, p0);
                c.normalize();
                vector4 a = vsub(p0, vmul(c, 0.4f));
                capsule cap(1e-5f, &q, &a);
                g_mission->scene->query_segment(&p0.x, &q.x, 0x110, list, &count);
                t = 1000.0f;
                for (i = 0; i < count; i++) {
                    if (list[i]->shape_type() == 2 && shape_contact_count(list[i], &cap, 1, &ct) != 0) {
                        vector4 n = ((face_shape*)list[i])->normal;
                        float dx = ct.point.x - q.x, dy = ct.point.y - q.y, dz = ct.point.z - q.z;
                        float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
                        float dot = n.z * c.z + n.y * c.y + n.x * c.x;
                        if (dot > 0.0f && dist < t) {
                            float lo = 0.1f;
                            t = dist;
                            d = dist - (radius * 2.5f / fmax_ref(&dot, &lo) + 0.6f);
                            if (d < 0.25f)
                                d = 0.25f;
                        }
                    }
                }
                f2ec = (d - f2ec) * dt * 11.0f + f2ec;
                k.o = vadd(vmul(k.u, f2ec), q);
            }
        }
      }
    }
pass2:
    if (g_mission->cam.rec == 0) {
        if (g_mission->cam.value > 0.0f) {
            float t = (float)(g_ticks[g_mission->cam.clock] - g_mission->cam.start) * 1e-5f / g_mission->cam.value;
            if (g_mission->cam.num > 0) {
                float nn = (float)g_mission->cam.num;
                t = atan((t - 0.5f) * nn * 3.1415927f) / (atan(nn * 1.5707964f) * 2.0f) + 0.5f;
            }
            if (t >= 1.0f) {
                g_mission->cam.value = 0.0f;
            } else {
                matrix m;
                float u;
                m.u = vneg(g_mission->cam.sway.f);
                m.f = g_mission->cam.sway.u;
                m.s = g_mission->cam.sway.s;
                m.o = g_mission->cam.sway.o;
                if (g_mission->cam.agent >= 0) {
                    if (g_mission->cam.attached != 0) {
                        m = AG(g_mission->cam.agent)->body->m.mul(m);
                    } else {
                        vector4* bo = &AG(g_mission->cam.agent)->body->m.o;
                        m.o.x = m.o.x + bo->x;
                        m.o.y = m.o.y + bo->y;
                        m.o.z = m.o.z + bo->z;
                    }
                }
                u = 1.0f - t;
                k.o = vadd(vmul(k.o, t), vmul(m.o, u));
                k.u = vadd(vmul(k.u, t), vmul(m.u, u));
                k.f = vadd(vmul(k.f, t), vmul(m.f, u));
                k.s = vadd(vmul(k.s, t), vmul(m.s, u));
                zoomL = u * g_mission->cam.zoom_to + t * zoomL;
            }
            k.orthonormalize();
        }
        if (g_mission->cam.rec == 0) {
            g_mission->cam.cur.f = vneg(k.u);
            g_mission->cam.cur.u = k.f;
            g_mission->cam.cur.s = vneg(k.s);
            g_mission->cam.cur.o = k.o;
            g_mission->cam.zoom_cur = zoomL;
        }
    }
    // the frustum
    float aspect = g_screen_a / g_screen_b;
    fr.frame = k;
    fr.build_planes();
    float v = (float)sqrt(1.0f / aspect);
    v = 1.17f < v ? 1.17f : v;
    v = v * zoomL * 0.86f;
    v = 102.0f < v ? 102.0f : v;
    fr.init(v * 0.017453292f, aspect, radius, 800.0f);
    view->fr = fr;
    g_mission->camera_to(&k, 97.0f / zoomL);
}

// @ 0x4f25a0
#pragma intrinsic(tan)
void frustum::init(float fov, float aspect, float znear, float aspect2)
{
    flag = 1;
    f118 = znear;
    double t = tan(fov * 0.5f);
    f11c = aspect2;
    double p = t * znear;
    f108 = (float)-p;
    znear = (float)(znear * t);
    f10c = znear;
    f114 = (float)-(p * aspect);
    f110 = znear * aspect;
    build_planes();
}
#pragma function(tan)
// ---- private callees of this slice -------------------------------------------------------------------------------
// Only declared here; their bodies live in other slices.
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
