// Slice s00514d40: one function, 0x514d40, the campaign screen's per-frame update (Screen::frame_update).
//
// It rebuilds the camera/frustum frame (the 0x120-byte block at Screen+0x170) from the two view-scale
// globals and copies it into the map widget's scene, picks the time-of-day caption ("ui_midnight",
// "ui_dawn", ...) from the game clock, feeds the raw clock to script event 0xf, shows the level/time-limit
// explanations of trial mode, turns the party at the current scene node into the two agents of the coming
// battle, and then runs the map's menu keys, the overlay visibility and the debug HUD.
// `this` is the campaign screen of s004ece20 (0x4ece20 = Screen::camera_update, called at the end);
// 0x5e2930 is the mission/game data object used throughout the neighbouring slices.
// Names are provisional; every offset is read off this slice's disassembly.
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "rgl.h"

// ---- tstring (s004608d0 / s004ef9f0) -------------------------------------------------------------
struct tstring {
    int pos;                                        // +0
    char* buf;                                      // +4
    int cap;                                        // +8
    int len;                                        // +0xc
    tstring();
    tstring(const char* s);                         // 0x42bbe0
    tstring(const tstring& o);                      // 0x42c110
    ~tstring();                                     // 0x42bc40
    void assign(const char* s);                     // 0x42bcb0
};

// the translation table at 0x5e2a18 (returns a scratch "STR NOT FOUND: ..." string when absent)
struct StrTable { tstring* lookup(const tstring& key); };   // 0x43a1e0 (ret 4)
extern StrTable g_strtable;                         // 0x5e2a18

// scratch tstring the original clears with "" and then measures again
extern tstring g_scratch;                           // 0xa70464

// ---- engine helpers ------------------------------------------------------------------------------
struct pos : rgl::matrix {
    rgl::vector4 transform_point(const rgl::vector4& p) const;   // 0x408460
};
rgl::matrix* identity_pos(rgl::matrix* out);        // 0x40cd00 (cdecl, returns out)

extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f
extern "C" double __cdecl fmod(double, double);            // 0x536a6a (__CIfmod, x87 args)

// ---- widgets -------------------------------------------------------------------------------------
struct WidgetVt {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03(); virtual void s04();
    virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08(); virtual void s09();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13(); virtual void s14();
    virtual void s15(); virtual void s16();
    virtual char key(int code);                     // +0x44
    virtual void s18(); virtual void s19(); virtual void s20(); virtual void s21(); virtual void s22();
    virtual void s23(); virtual void s24(); virtual void s25(); virtual void s26(); virtual void s27();
    virtual void s28(); virtual void s29(); virtual void s30(); virtual void s31(); virtual void s32();
    virtual void s33(); virtual void s34(); virtual void s35(); virtual void s36(); virtual void s37();
    virtual void s38(); virtual void s39(); virtual void s40();
    virtual void text_size(float* out, const tstring* text);      // +0xa4
    virtual void s42(); virtual void s43();
    virtual void hide();                            // +0xb0
    virtual void show();                            // +0xb4
    virtual void s46();
};
struct Widget { WidgetVt* vt; };

// the console/log object at 0x5a9f30 (slot +0x28 prints one line)
struct TextLog {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03(); virtual void s04();
    virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08(); virtual void s09();
    virtual void print(const char* s);              // +0x28
    virtual void s11();
};
extern TextLog g_textlog;                           // 0x5a9f30

// ---- menu records --------------------------------------------------------------------------------
struct MenuRec {
    char pad4[4];
    int f4;                                         // +0x4
    char pad8[0x14 - 0x8];
    int f14;                                        // +0x14
    int f18;                                        // +0x18
};
struct NamedRec { void setup(const tstring* a, const tstring* b); };   // 0x48a3a0 (ret 8)
struct MainMenu {
    NamedRec* rec;                                  // +0x0 (the explanation record 0x48a3a0 fills)
    void choose(int id);                            // 0x495700 (ret 4)
    void reset(int arg);                            // 0x494260 (ret 4)
};
extern MainMenu g_main_menu;                        // 0xa73980
extern MenuRec* g_menu_94;                          // 0xa73994
extern MenuRec* g_menu_a0;                          // 0xa739a0
extern MenuRec* g_menu_b0;                          // 0xa739b0
extern MenuRec* g_menu_b4;                          // 0xa739b4
extern MenuRec* g_menu_bc;                          // 0xa739bc
extern MenuRec* g_menu_c0;                          // 0xa739c0
extern MenuRec* g_menu_d8;                          // 0xa739d8
extern MenuRec* g_menu_dc;                          // 0xa739dc
extern MenuRec* g_menu_e4;                          // 0xa739e4
extern MenuRec* g_menu_e8;                          // 0xa739e8
extern MenuRec* g_menu_ec;                          // 0xa739ec

// key table 0xa73730 / keyboard state 0xa170c8
struct KeyTable { char down(int key); };            // 0x48ac90 (ret 4)
extern KeyTable g_keys;                             // 0xa73730
struct KeyNames { char bound(int key); };           // 0x487900 (ret 4)
extern KeyNames g_key_names;                        // 0xa170c8

// message log
struct MsgLog { void flush(); };                    // 0x4f1390
extern MsgLog g_msglog;                             // 0x5d7080

// the mission UI object at 0xa73300
struct MissionUi {
    void f514890(int a, int b, int c, int d);       // 0x514890 (ret 0x10)
    void f516250(int a);                            // 0x516250 (ret 4)
    void f516370(int a);                            // 0x516370 (ret 4)
};
extern MissionUi g_mission_ui;                      // 0xa73300

// the framed command block at 0x5e291c (0x513710 commits it; the byte handed to it is its out flag)
struct Flags5e291c {
    char b0;                                        // +0x0
    char b1;                                        // +0x1
    char b2;                                        // +0x2
    char pad3[1];
    int f4;                                         // +0x4
    int f8;                                         // +0x8
    void commit(char* out);                         // 0x513710 (ret 4)
};
extern Flags5e291c g_flags;                         // 0x5e291c

// ---- mission data (offsets shared with s0049b630 / s0049f060) --------------------------------------
struct TickPair {                                   // 64-bit millisecond pairs, lo first
    int lo, hi;
};
union Tick64 {                                      // g_ticks[i].lo + g_ticks[i].hi * 2^32
    TickPair p;
    __int64 v;
};
extern Tick64 g_ticks[];                            // 0x5e2948
extern __int64 g_a70448;                            // 0xa70448 (the 64-bit clock the map last played)

struct Party {
    char pad0[0x30];
    float x;                                        // +0x30
    float y;                                        // +0x34
    char pad38[0x54 - 0x38];
    int f54;                                        // +0x54
    int f58;                                        // +0x58
    char pad5c[0x88 - 0x5c];
    int f88;                                        // +0x88 (scene object index, < 0 = not deployed)
    char pad8c[0x918 - 0x8c];
    int f918;                                       // +0x918 (order / icon type)
    char pad91c[0x5ac8 - 0x91c];
    void reset_order();                             // 0x4b3bc0
};
struct PartyMgr { Party* get(int id); };            // 0x51ee50 (ret 4)

struct ItemSound { void play(int id); };            // 0x48add0 (ret 4)

struct Rec98 {                                      // the 0x98-byte scene node record
    char pad0[0x10];
    int f10;                                        // +0x10
    char pad14[0x54 - 0x14];
    int f54;                                        // +0x54
    int f58;                                        // +0x58
};

struct TroopRec {
    char pad0[0xb0];
    int level;                                      // +0xb0
};

struct Psys {
    char pad0[0x48];
    int* f48;                                       // +0x48
    void reset();                                   // 0x47fef0
};
extern Psys* g_psys;                                // 0xa70248

struct Renderer {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03();
    virtual void s04(); virtual void s05(); virtual void s06(); virtual void s07();
    virtual void s08(); virtual void s09(); virtual void s10(); virtual void s11();
    virtual void s12(); virtual void s13(); virtual void s14(); virtual void s15();
    virtual void s16(); virtual void s17(); virtual void s18(); virtual void s19();
    virtual void s20(); virtual void s21();
    virtual void set_frame(const rgl::matrix* m);   // +0x54
};

// ---- the mission / game data at 0x5e2930 ---------------------------------------------------------
extern char* g_game;                                // 0x5e2930

void f516a70();                                     // 0x516a70
int  save_game(int arg, char* name);                // 0x4e8980

struct GameData {
    void f516ac0(float dt);                                     // 0x516ac0 (ret 4)
    void emit_event(int id, int a, int b);                      // 0x435790 (ret 0xc)
    void attach_party_center(const float* pt, int a, int b);    // 0x49e9c0 (ret 0xc)
};
#define GG ((GameData*)g_game)
#define G_INT(off) (*(int*)(g_game + (off)))
#define G_FLT(off) (*(float*)(g_game + (off)))
#define G_BYTE(off) (*(char*)(g_game + (off)))
#define G_PARTIES ((PartyMgr*)(g_game + 0xf8))

// ---- globals -------------------------------------------------------------------------------------
extern int g_a170c0;            // 0xa170c0
extern int g_a170d8;            // 0xa170d8
extern int g_a174d8;            // 0xa174d8
extern int g_a181b4;            // 0xa181b4
extern int g_a171b8;            // 0xa171b8
extern int g_a17454;            // 0xa17454
extern int g_a17854;            // 0xa17854
extern int g_a17148;            // 0xa17148
extern int g_a17348;            // 0xa17348
extern int g_scene;             // 0xa732b4
extern int g_a70438;            // 0xa70438
extern int g_use_shaders;       // 0x5e28fc
extern int g_no_cam;            // 0x5a9fb0
extern char g_saved;            // 0x5e2a08
extern int g_trial_mode;        // 0x5e29c4
extern int g_a735c8;            // 0xa735c8
extern float g_view_a;          // 0x5d75d8
extern float g_view_b;          // 0x5d75dc

// ---- the campaign screen (`this`) -----------------------------------------------------------------
struct Cam {                                        // 0x120 bytes at Screen+0x170
    unsigned char f0;                               // +0x0
    char pad1[0xc8 - 1];
    pos frame;                                      // +0xc8
    float f108;                                     // +0x108
    float f10c;                                     // +0x10c
    float f110;                                     // +0x110
    float f114;                                     // +0x114
    float f118;                                     // +0x118
    float f11c;                                     // +0x11c
    void build_planes();                            // 0x41e4f0
};

struct Screen {
    char pad0[0x30];
    void* f30;                                      // +0x30 (map widget whose scene receives the frame)
    char pad34[0x38 - 0x34];
    Widget* f38;                                    // +0x38
    Widget* f3c;                                    // +0x3c
    Widget* f40;                                    // +0x40
    Widget* f44;                                    // +0x44
    Widget* f48;                                    // +0x48
    Widget* f4c;                                    // +0x4c
    Widget* f50;                                    // +0x50
    Widget* f54;                                    // +0x54
    Widget* f58;                                    // +0x58
    Widget* f5c;                                    // +0x5c
    Widget* f60;                                    // +0x60
    char f64;                                       // +0x64
    char pad65[0x170 - 0x65];
    Cam cam;                                        // +0x170
    char pad290[0x2cc - 0x290];
    float f2cc;                                     // +0x2cc
    float f2d0;                                     // +0x2d0
    void frame_update(float dt);                    // 0x514d40
    void camera_update(float dt);                   // 0x4ece20 (ret 4)
    void f515d80(float dt);                         // 0x515d80 (ret 4)
    void f5191e0();                                 // 0x5191e0
};

// @ 0x514d40 ---------------------------------------------------------------------------------------
void Screen::frame_update(float dt)
{
    float t = 0.0f;                                 // game hours since the clock's reference tick
    char flag;                                      // 1 while the map is idle
    bool start;

    if (g_psys != 0 && g_psys->f48[0x50 / 4] != g_a170c0
        && g_a70438 == g_scene && g_use_shaders != 0)
        g_psys->reset();

    cam.f0 = 1;
    cam.f118 = 0.2f;
    cam.f11c = 300.0f;
    cam.f108 = g_view_a * -0.1f;
    cam.f10c = g_view_a * 0.1f;
    cam.f114 = g_view_b * -0.095f;
    cam.f110 = g_view_b * 0.095f;
    cam.build_planes();
    *(Cam*)((char*)f30 + 0x1ac) = cam;

    Tick64 cur = g_ticks[G_INT(0x13b18)];
    Tick64 ref = *(Tick64*)(g_game + 0x13b20);
    t = (float)(cur.v - ref.v) * 1e-5f;

    float hour = (float)fmod((double)t, 24.0);

    float sz[2];
    f5c->vt->text_size(sz, g_strtable.lookup(
        hour < 1.0f  ? tstring("ui_midnight") :
        hour < 4.0f  ? tstring("ui_late_night") :
        hour < 6.0f  ? tstring("ui_dawn") :
        hour < 8.0f  ? tstring("ui_early_morning") :
        hour < 11.0f ? tstring("ui_morning") :
        hour < 13.0f ? tstring("ui_noon") :
        hour < 16.0f ? tstring("ui_afternoon") :
        hour < 18.0f ? tstring("ui_late_afternoon") :
        hour < 20.0f ? tstring("ui_dusk") :
        hour < 23.0f ? tstring("ui_evening") :
                       tstring("ui_midnight")));

    int hi = (int)t;
    g_scratch.assign("");
    GG->emit_event(0xf, hi / 24, hi);
    {
        tstring tmp(g_scratch);
        f60->vt->text_size(sz, &tmp);
    }

    flag = 1;
    if (G_FLT(0x13ad0) > 0.0f)
        flag = 0;
    if (G_BYTE(0x18) == 0 && g_keys.down(0x24))
        f516a70();
    if (G_INT(0x1a208) >= 0) {
        g_a70448 = G_INT(0x1a208);
        ((ItemSound*)(g_game + 0x139f4))->play(-8);
        G_INT(0x1a208) = -1;
    }

    int cmd = G_INT(0xc8);
    if (cmd == 0x12) {
        G_INT(0xc8) = 0;
        g_menu_dc->f18 = 7;
        g_main_menu.choose(0x17);
        return;
    }
    if (cmd == 0xf) {
        G_INT(0xc8) = 0;
        g_menu_ec->f18 = 7;
        g_main_menu.choose(0x1b);
        return;
    }
    if (cmd == 0x10) {
        G_INT(0xc8) = 0;
        g_menu_e8->f18 = 7;
        g_main_menu.choose(0x1a);
        return;
    }
    if (g_a170d8 != 0 && g_a174d8 == 0) {
        g_menu_94->f18 = 7;
        g_main_menu.choose(5);
        return;
    }
    if (f48 != 0 && f48->vt->key(0x10000)) {
        g_menu_d8->f18 = 7;
        g_menu_d8->f4 = 0;
        g_main_menu.choose(0x16);
        return;
    }
    if (flag) {
        if (f50->vt->key(0x10000) || g_keys.down(0x12)) {
            if (g_a181b4 != 0) {
                g_menu_e8->f18 = 7;
                G_INT(0x13b08) = 4;
                G_INT(0x13b0c) = 0;
                g_main_menu.choose(0x1a);
                return;
            }
            g_menu_b4->f18 = 7;
            g_menu_b4->f4 = 0;
            g_main_menu.choose(0xd);
            return;
        }
    }

    if (f64 != 0) {
        if (g_saved == 0)
            save_game(-1, 0);
        g_saved = 1;
        return;
    }
    if (G_BYTE(0x13ac8) != 0) {
        g_main_menu.rec->setup((tstring*)(g_game + 0x13ab8), (tstring*)(g_game + 0x13aa8));
        g_main_menu.reset(0);
        G_BYTE(0x13ac8) = 0;
        return;
    }
    if (G_BYTE(0x13a3c) != 0) {
        G_BYTE(0x13a3c) = 0;
        g_mission_ui.f514890(G_INT(0x13a44), -1, -1, G_INT(0x13a40));
        g_menu_c0->f18 = 7;
        g_main_menu.choose(0x10);
        return;
    }

    int fpid = G_INT(0x78);
    Party* party = G_PARTIES->get(fpid);
    if (party->f88 < 0)
        goto no_party;
    party = G_PARTIES->get(fpid);
    {
        char** pages = *(char***)(g_game + 0xe4);
        unsigned idx = (unsigned)party->f88;
        Rec98* node = (Rec98*)(pages[idx >> 4] + (idx & 0xf) * 0x98);
        if (node->f10 != 1)
            goto no_party;
    }
    party = G_PARTIES->get(fpid);
    {
        char** pages = *(char***)(g_game + 0xe4);
        unsigned idx = (unsigned)party->f88;
        Rec98* node = (Rec98*)(pages[idx >> 4] + (idx & 0xf) * 0x98);
        start = true;
        if (node->f54 == G_INT(0x78)) {
            G_INT(0x13a0c) = node->f58;
            G_INT(0x13a10) = -100000;
        } else if (node->f58 == G_INT(0x78)) {
            G_INT(0x13a0c) = node->f54;
            G_INT(0x13a10) = -100000;
        } else {
            G_INT(0x13a0c) = node->f58;
            G_INT(0x13a10) = node->f54;
        }
    }
    if (((TroopRec*)(g_game + 0x13a00))[G_INT(0x8c)].level <= 6 || g_trial_mode != 0) {
        if (1 - (int)(t * -0.041666667f) > 30) {
            if (g_trial_mode == 0) {
                tstring a("ui_explanation_time_limit");
                tstring b("ui_time_limit_reached");
                g_main_menu.rec->setup(g_strtable.lookup(b), g_strtable.lookup(a));
                g_main_menu.reset(g_trial_mode);
                f64 = 1;
                start = false;
            }
        }
    } else {
        tstring a("ui_explanation_level_limit");
        tstring b("ui_level_limit_reached");
        g_main_menu.rec->setup(g_strtable.lookup(b), g_strtable.lookup(a));
        g_main_menu.reset(g_trial_mode);
        f64 = 1;
        start = false;
    }

    if (!start)
        return;

    party = G_PARTIES->get(G_INT(0x78));
    g_mission_ui.f516250(party->f88);
    G_INT(0x13aec) = 0;
    G_INT(0x13af8) = 0;
    ((ItemSound*)(g_game + 0x139f4))->play(-6);
    GG->emit_event(1, G_INT(0x13a0c), G_INT(0x13a10));
    if (G_INT(0x13aec) > 0) {
        g_mission_ui.f516370(G_INT(0x13aec));
        party = G_PARTIES->get(G_INT(0x78));
        g_flags.f8 = G_INT(0x13aec);
        g_flags.f4 = party->f88;
    } else {
        g_mission_ui.f516370(-1);
        g_a735c8 = 1;
        party = G_PARTIES->get(G_INT(0x78));
        g_flags.f8 = -1;
        g_flags.f4 = party->f88;
    }
    g_flags.b0 = 1;
    g_flags.b1 = 0;
    g_flags.b2 = 0;
    g_flags.commit(&flag);
    return;

no_party:
    if (flag) {
        if (f40->vt->key(0x10000) || g_keys.down(0x10)) {
            g_menu_a0->f18 = 7;
            g_menu_a0->f4 = 0;
            g_main_menu.choose(8);
            return;
        }
        if (f3c->vt->key(0x10000) || g_keys.down(0x11)) {
            g_menu_b0->f18 = 7;
            g_menu_b0->f4 = 0;
            g_main_menu.choose(0xc);
            return;
        }
        if (f44->vt->key(0x10000) || g_keys.down(0xf)) {
            g_menu_bc->f18 = 7;
            g_menu_bc->f4 = 0;
            g_menu_bc->f14 = G_INT(0x8c);
            g_main_menu.choose(0xf);
            return;
        }
        if (g_keys.down(0x13)) {
            g_menu_e4->f18 = 7;
            g_menu_e4->f4 = 0;
            g_main_menu.choose(0x19);
            return;
        }
        if (f4c != 0 && f4c->vt->key(0x10000)) {
            Party* p = G_PARTIES->get(G_INT(0x78));
            GG->attach_party_center(&p->x, G_INT(0x78), G_INT(0x80));
            return;
        }
    }
    if (f38 != 0 && f38->vt->key(0x10000)) {
        G_INT(0x13aec) = 4;
        g_menu_dc->f18 = 7;
        g_main_menu.choose(0x17);
        return;
    }
    if (G_FLT(0x14058) == 0.0f)
        f54->vt->show();
    else
        f54->vt->hide();
    g_msglog.flush();
    if (G_FLT(0x13ad0) > 0.0f && G_INT(0x13ad8) != 0) {
        f58->vt->show();
        if (g_a17454 == 0 && g_a17854 != 0)
            G_FLT(0x13ad0) = 0.0f;
    } else {
        f58->vt->hide();
    }
    if (!(G_FLT(0x13ad0) > 0.0f && G_INT(0x13ad8) != 0))
        f515d80(dt);
    camera_update(dt);
    if (G_FLT(0x13ad0) <= 0.0f) {
        f5191e0();
        f2cc += dt;
        f2d0 += dt;
        G_INT(0x13aa4) = 0;
        {
            int bpid = G_INT(0x78);
            if (G_PARTIES->get(bpid)->f918 == 4 || G_PARTIES->get(bpid)->f918 == 1
                || G_PARTIES->get(bpid)->f918 == 6 || G_PARTIES->get(bpid)->f918 == 9)
                G_INT(0x13aa4) = 1;
        }
        if (g_a171b8 != 0) {
            G_PARTIES->get(G_INT(0x78))->f918 = 0;
            G_PARTIES->get(G_INT(0x78))->reset_order();
            G_INT(0x13aa4) = 2;
        }
    }

    GG->f516ac0(dt);
    rgl::matrix m;
    identity_pos(&m);
    {
        float vx = 0.0175f - g_view_a * 0.1975f;
        float vy = 0.015f - g_view_b * 0.18f;
        rgl::vector4 v(vx, vy, -0.4f);
        rgl::vector4 p = cam.frame.transform_point(v);
        m.o = p;
    }
    (*(Renderer**)(g_game + 0x1a2a8))->set_frame(&m);

    if (g_no_cam != 0 && (g_a17148 != 0 || g_a17348 != 0) && g_key_names.bound(0x12))
        G_INT(0x1a090) = (G_INT(0x1a090) == 0);

    if (G_INT(0x1a090) != 0) {
        char buf[0x200];
        int bpid = G_INT(0x78);
        sprintf(buf, "Position: %3.2f, %3.2f ;",
                (double)G_PARTIES->get(bpid)->x, (double)G_PARTIES->get(bpid)->y);
        g_textlog.print(";-------------------;");
        g_textlog.print(buf);
        g_textlog.print(";-------------------;");
    }
}
