// Slice s004ea8e0: the character-sheet camera/rotation widget. 0x4ea8e0 updates the model preview
// (zoom slider, clipboard pose dump, reset keys); 0x4eb050 is the per-frame orbit/zoom step.
// Names provisional; all offsets come from the disassembly of this range.
#include <windows.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

struct mat4 {
    vector4 s, f, u, o;
};
struct mat3 { vector4 s, f, u; };

mat4* identity_pos(mat4* out);                          // 0x40cd00 (cdecl; returns out)
struct mat4_self {
    void rotate_fu(const float* angle);                 // 0x40e570 (thiscall, ret 4)
    void rotate_su(const float* angle);                 // 0x40e670 (thiscall, ret 4)
};

// key state / settings globals
extern int g_a17204;                                    // 0xa17204
extern int g_a17214;                                    // 0xa17214
extern int g_a1720c;                                    // 0xa1720c
extern int g_a171fc;                                    // 0xa171fc
extern int g_a17454;                                    // 0xa17454
extern int g_a17854;                                    // 0xa17854
extern int g_a17458;                                    // 0xa17458
extern float g_5d7094;                                  // 0x5d7094
extern float g_5d7098;                                  // 0x5d7098
extern float g_a178d4;                                  // 0xa178d4
extern float g_a178d8;                                  // 0xa178d8

// globals of the face-generator update (0x4ea8e0)
extern int g_no_cam;                                    // 0x5a9fb0
extern int g_a17148;                                    // 0xa17148
extern int g_a17348;                                    // 0xa17348
extern int g_a1711c;                                    // 0xa1711c
extern int g_a1751c;                                    // 0xa1751c
extern int g_world_time;                                // 0x5d8790
extern float g_scroll;                                  // 0x590d7c
extern char* g_game;                                    // 0x5e2930
extern float g_dummy_mat;                               // 0x8ed4f8c (float; one per deform key)
extern float g_dummy1;                                  // 0x8ed508c

struct SkinSet {
    void regen(int arg, float* values);                 // 0x4735e0 (thiscall, ret 8)
};
extern SkinSet* g_skins_begin;                          // 0x8ed4f7c
extern int g_cur;                                       // 0x8ed4f68

struct World { void step(float dt); };                  // 0x427f40
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s);                             // 0x42bbe0
    ~tstring();                                         // 0x42bc40
    void operator=(const tstring& o);                   // 0x42bc50
};
struct StrTable { const tstring* lookup(const tstring* key); };      // 0x43a1e0
extern StrTable g_strtable;                             // 0x5e2a18
struct MenuRec { void reset(int a); };                  // 0x494260
struct main_menu {
    char pad0[4];
    int  open;                                          // +0x04
    char pad08[0x1c - 0x08];
    tstring title;                                      // +0x1c
    char pad2c[0x4c - 0x2c];
    int  choice;                                        // +0x4c
    int  v50;                                           // +0x50
    char pad54[0x68 - 0x54];
    tstring header;                                     // +0x68
    void reset(int a);                                  // 0x494260
    void choose(int option);                            // 0x495700
};
#define g_main_menu (*(main_menu*)0xa73980)

void __cdecl mb_snprintf(char* out, unsigned size, const char* fmt, ...);   // 0x415100
struct Animator { void randomise(); };                  // 0x4eb320
extern Animator g_anim;                                 // 0x8ed4f60

// a UI element; only the vtable slots this function calls are named
struct Ctrl {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30();
    virtual void v34(float v);                          // +0x34
    virtual float v38();                                // +0x38
    virtual void v3c(); virtual void v40();
    virtual char v44(int key);                          // +0x44
    virtual void v48(int key);                          // +0x48
    virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void v58(); virtual void v5c(); virtual void v60(); virtual void v64();
    virtual void v68(); virtual void v6c(); virtual void v70(); virtual void v74();
    virtual void v78(); virtual void v7c(); virtual void v80(); virtual void v84();
    virtual void v88(); virtual void v8c(); virtual void v90(); virtual void v94();
    virtual void v98(); virtual void v9c(); virtual void va0(); virtual void va4();
    virtual void va8(); virtual void vac();
    virtual void vb0();                                 // +0xb0
    virtual void vb4();                                 // +0xb4
    void set_value(float v);                            // 0x48baa0
};

// the character-sheet / face-generator preview (ctor 0x4f49d0); offsets from this range
struct CamView {
    char pad0[0x08];
    int  state;                                         // +0x08
    char pad0c[0x14 - 0x0c];
    int  troop;                                         // +0x14
    int  choice;                                        // +0x18
    char pad1c[0x30 - 0x1c];
    char* target;                                       // +0x30
    char pad34[0x164 - 0x34];
    float yaw;                                          // +0x164
    float pitch;                                        // +0x168
    float zoom;                                         // +0x16c
    unsigned char follow;                               // +0x170
    char pad171[0x174 - 0x171];
    int  skin;                                          // +0x174
    World* world;                                       // +0x178
    Ctrl* slider;                                       // +0x17c
    char pad180[0x188 - 0x180];
    Ctrl* cam_btn;                                      // +0x188
    Ctrl* rows[0x1c];                                   // +0x18c
    char pad25c[0x30c - 0x1fc];
    Ctrl* b_next_beard;                                 // +0x30c
    Ctrl* b_next_hair;                                  // +0x310
    Ctrl* b_next_face;                                  // +0x314
    Ctrl* b_prev_beard;                                 // +0x318
    Ctrl* b_prev_hair;                                  // +0x31c
    Ctrl* b_prev_face;                                  // +0x320
    char pad324[0x330 - 0x324];
    Ctrl* sl_age;                                       // +0x330
    Ctrl* sl_hair_color;                                // +0x334
    char pad338[0x340 - 0x338];
    Ctrl* b_done;                                       // +0x340
    Ctrl* b_reset;                                      // +0x344
    Ctrl* b_randomize;                                  // +0x348
    void* ent;                                          // +0x34c
    int  f350;                                          // +0x350
    int  f354;                                          // +0x354
    int  f358;                                          // +0x358
    int  f35c;                                          // +0x35c
    char pad360[0x368 - 0x360];
    float pose[8];                                      // +0x368 (x..., 8 pose floats)
    char buf388[0x28];                                  // +0x388
    char buf3b0[0x28];                                  // +0x3b0
    char buf3d8[0x28];                                  // +0x3d8
    char buf400[0x10];                                  // +0x400

    void update(float dt);                              // 0x4eb050
    void setup(float dt);                               // 0x4eb270
    void refresh();                                     // 0x4eb3c0
    void update_model(float dt);
};

#define SKIN_INT(off) (*(int*)(g_skins_begin + g_cur * 0x1f8 + (off)))
#define g_dummy_mat_arr (&g_dummy_mat)

// @ 0x4eb050
void CamView::update(float dt)
{
    if (g_a17204)
        pitch = pitch - dt * 1.1f;
    if (g_a17214)
        pitch = dt * 1.1f + pitch;
    if (g_a1720c)
        zoom = zoom - dt * 2.1f;
    if (g_a171fc)
        zoom = dt * 2.1f + zoom;
    if (g_a17454) {
        if (g_a17854 == 0 &&
            g_5d7094 < *(float*)(target + 0xf0) &&
            g_5d7098 > *(float*)(target + 0xe4))
            follow = 1;
        if (g_a17454)
            goto check_follow;
    }
    if (g_a17854)
        follow = 0;
check_follow:
    if (g_a17458 || follow) {
        yaw = (g_a178d4 * dt) * 1.7f + yaw;
        pitch = pitch - (g_a178d8 * dt) * 1.7f;
    }
    if (pitch < -0.6519978f)
        pitch = -0.6519978f;
    else if (pitch > 0.6519978f)
        pitch = 0.6519978f;
    if (zoom < 0.3f)
        zoom = 0.3f;
    else if (zoom > 2.0f)
        zoom = 2.0f;
    mat4 m;
    identity_pos(&m);
    float angle = 1.5707964f;
    ((mat4_self*)&m)->rotate_fu(&angle);
    ((mat4_self*)&m)->rotate_su(&yaw);
    angle = -pitch;
    ((mat4_self*)&m)->rotate_fu(&angle);
    m.o.z = -0.1f;
    *(mat4*)((char*)ent + 0x58) = m;
}

// @ 0x4ea8e0
void CamView::update_model(float dt)
{
    int n = *(int*)(g_skins_begin + skin * 0x1f8 + 0x3c) - 1;

    if (g_no_cam && (g_a17148 || g_a17348) && g_a1711c && !g_a1751c) {
        if (*(int*)(g_game + 0x1a090) == 0) {
            *(int*)(g_game + 0x1a090) = 1;
            cam_btn->vb4();
        } else {
            *(int*)(g_game + 0x1a090) = 0;
            cam_btn->vb0();
        }
    }

    if (g_world_time != 0) {
        Ctrl* sl = *(Ctrl**)(*(char**)((char*)this + 0x17c) + 0x484);
        float a = *(float*)((char*)sl + 0x168) - *(float*)((char*)sl + 0x16c);
        float b = *(float*)((char*)sl + 0x170) - *(float*)((char*)sl + 0x16c);
        float* arr = (float*)((char*)sl + *(int*)((char*)sl + 0x178) * 4 + 0xf0);
        float v = (a / b) * (arr[0] - *(float*)((char*)sl + 0x17c))
                + ((float)g_world_time / arr[0]) * *(float*)((char*)sl + 0x17c) * g_scroll;
        sl->set_value(v);
        sl->v48(0x10000);
    }

    if (cam_btn && cam_btn->v44(0x10000)) {
        if (OpenClipboard(0)) {
            char text[0x200];
            mb_snprintf(text, 0x200, "0x%s%s%s%s", buf388, buf3b0, buf3d8, buf400);
            int len = 0;
            while (text[len])
                len++;
            len++;
            HGLOBAL h = GlobalAlloc(0x2002, len);
            if (h) {
                EmptyClipboard();
                char* dst = (char*)GlobalLock(h);
                char* src = text;
                while (*src) {
                    *dst++ = *src++;
                }
                *dst = 0;
                GlobalUnlock(h);
                SetClipboardData(1, h);
            }
            CloseClipboard();
        }
    }

    char flag = 0;
    for (int i = 0; i < n; i++) {
        Ctrl* row = rows[i];
        if (row->v44(0x10000)) {
            g_dummy_mat_arr[i] = row->v38();
            SkinSet* set = (SkinSet*)(g_skins_begin + g_cur * 0x1f8);
            set->regen(i, g_dummy_mat_arr);
            for (int j = 0; j < n; j++)
                rows[j]->v34(g_dummy_mat_arr[j]);
            flag = 1;
        }
    }

    if (b_next_beard && b_next_beard->v44(0x10000)) {
        f350 = f350 + 1;
        flag = 1;
    }
    if (b_next_hair->v44(0x10000)) {
        f354 = f354 + 1;
        flag = 1;
    }
    if (b_next_face->v44(0x10000)) {
        f35c = f35c + 1;
        flag = 1;
    }
    if (b_prev_beard && b_prev_beard->v44(0x10000)) {
        int m = SKIN_INT(0x88);
        f350 = (f350 + m) % (m + 1);
        flag = 1;
    }
    if (b_prev_hair->v44(0x10000)) {
        int m = SKIN_INT(0x18c);
        f354 = (f354 + m) % (m + 1);
        flag = 1;
    }
    if (b_prev_face->v44(0x10000)) {
        int m = SKIN_INT(0x190);
        f35c = (m - 1 + f35c) % m;
        flag = 1;
    }
    if (sl_age->v44(0x10000))
        flag = 1;
    if (sl_hair_color->v44(0x10000))
        flag = 1;
    if (b_randomize->v44(0x10000)) {
        g_anim.randomise();
        for (int i = 0; i < n; i++)
            rows[i]->v34(g_dummy_mat_arr[i]);
        int m = SKIN_INT(0x18c);
        f354 = (m < 2 ? 0 : rand() % m) + 1;
        f350 = rand();
        f35c = rand();
        f358 = rand();
        sl_hair_color->v34(g_dummy1);
        flag = 1;
    }
    if (b_reset->v44(0x10000)) {
        float* p = g_dummy_mat_arr;
        for (int i = 0x3f; i != 0; i--)
            *p++ = 0.5f;
        SkinSet* set = (SkinSet*)(g_skins_begin + g_cur * 0x1f8);
        set->regen(-1, g_dummy_mat_arr);
        flag = 1;
        for (int i = 0; i < n; i++)
            rows[i]->v34(g_dummy_mat_arr[i]);
    }

    f350 = f350 % (SKIN_INT(0x88) + 1);
    f354 = f354 % (SKIN_INT(0x18c) + 1);
    f35c = f35c % SKIN_INT(0x190);

    if (flag)
        refresh();

    update(dt);
    world->step(dt);
    setup(dt);

    if (state == 0) {
        state = 1;
        tstring t1("ui_tutorial_face_generator");
        tstring t2("ui_tutorial");
        const tstring* r1 = g_strtable.lookup(&t1);
        const tstring* r2 = g_strtable.lookup(&t2);
        g_main_menu.header = *r2;
        g_main_menu.title = *r1;
        g_main_menu.choice = 0;
        g_main_menu.v50 = 0;
        g_main_menu.open = 0;
        g_main_menu.reset(0);
        return;
    }

    if (b_done->v44(0x10000)) {
        if (troop >= 0) {
            char* rec = *(char**)(g_game + 0x13a00);
            int idx = troop;
            *(float*)(rec + idx * 0x508 + 0x3c0) = pose[0];
            *(float*)(rec + idx * 0x508 + 0x3c4) = pose[1];
            *(float*)(rec + idx * 0x508 + 0x3c8) = pose[2];
            *(float*)(rec + idx * 0x508 + 0x3cc) = pose[3];
            *(float*)(rec + idx * 0x508 + 0x3d0) = pose[4];
            *(float*)(rec + idx * 0x508 + 0x3d4) = pose[5];
            *(float*)(rec + idx * 0x508 + 0x3d8) = pose[6];
            *(float*)(rec + idx * 0x508 + 0x3dc) = pose[7];
        }
        int ch;
        if (state == 1) {
            state = -1;
            *(int*)(g_game + 0x13aec) = 1;
            *(int*)(0xa739dc + 0x18) = 7;
            ch = 0x17;
        } else {
            ch = choice;
        }
        g_main_menu.choose(ch);
    }
}
