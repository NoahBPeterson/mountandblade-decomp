// Slice s005261e0: the mission screen / game-loop tick (0x5261e0, 6818 bytes, thiscall, ret 4).
//
// The `this` object is the mission HUD/controller: its fields +0x2c8 (mode), +0x2cc (selection),
// +0x2d4 (flag), +0x2d8/+0x2dc, +0x2f1/+0x2f2/+0x2f4 and +0x324 are read/written here; the item source
// object at +0x30 is MissionSource (0x4d5410/0x4dd920, see slice s004d5410).  clobbered globals are the
// mission (0x5e292c), the game data (0x5e2930) and the GUI/message log (0x5d7080).
//
// One frame: lock, key toggles, scene time-scale, renderer "ready" rate, HUD/camera update, the no-camera
// position save, the mission-editor key block (modes 0x15..0x24 and the three group flags), the region
// change/timers (0x29c..0x2c8), the item-use keys (kinds 1/4/5/6/7), the sound-slot ticks, menu actions
// (0xa73980), the conversation auto-close timer, and finally the menu-option / quick-save / dialog handlers.
//
// Offsets were read from this function; names are provisional.  Declarations reuse the verified layouts
// from slices s004d5410 (MissionSource, tstring, MsgLog), s004f1390 (MsgLog), s0043ac40 (AgentTable),
// s004532f0 (GameData/Mission) and s004d5410 (MainMenu/Widget), kept local to this slice.
#include <windows.h>
#include <math.h>
#include "rgl.h"

// -----------------------------------------------------------------------------------------------------
// rgl string table (16-byte tstring: read cursor, buffer, capacity, length); ctor 0x42bbe0, copy
// 0x42c110, dtor 0x42bc40.  c_str() is the inline `buf ? buf : (char*)&buf` seen at 0x526b7f.
struct tstring {
    int  pos;
    char* buf;
    int  cap;
    int  len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    tstring& operator=(const tstring& o);               // 0x42bc50
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* lookup(tstring* key);                      // 0x43a1e0 (thiscall, ret 4)
};
extern StrTable g_strtable;                             // 0x5e2a18

// -----------------------------------------------------------------------------------------------------
// message log (global object 0x5d7080) -- see s004f1390 / s004529e0
struct MsgLog {
    char pad0[0xb0b8];
    void add(tstring msg, int color);                   // 0x452c00 (ret 0x14)
    void flush();                                       // 0x4f1390
    void set_o568(int a, int b);                        // 0x42cdb0
    void set_view(float* v);                            // 0x4c5190
};
extern MsgLog g_gui;                                    // 0x5d7080

// -----------------------------------------------------------------------------------------------------
// key tables: two 256-entry int arrays at 0xa170d4 (down) and 0xa174d4 (up); 0x480 apart.
extern int g_key_a[256];                                // 0xa170d4
extern int g_key_b[256];                                // 0xa174d4

struct KeyTable {
    char down(int key);                                 // 0x48ac90 (thiscall, ret 4)
};
extern KeyTable g_keytable;                             // 0xa73730

struct KeyReady {
    char is_ready(int key);                             // 0x487900 (thiscall, ret 4)
};
extern KeyReady g_keyready;                             // 0xa170c8

// 2-entry binding arrays of the mission-editor actions
extern int g_keys_7350[2];                              // 0xa73750
extern int g_keys_7388[2];                              // 0xa73788
extern int g_keys_73a8[2];                              // 0xa737a8
extern int g_keys_73b0[2];                              // 0xa737b0
extern int g_keys_73b8[2];                              // 0xa737b8
extern int g_keys_73c0[2];                              // 0xa737c0
extern int g_keys_73c8[2];                              // 0xa737c8

// -----------------------------------------------------------------------------------------------------
// agents / mission / game data
struct Agent;
struct AgentTable {
    int  count;                                         // +0
    char pad4[0x0c];
    int  first_used();                                  // 0x43b1b0
    void next_used(unsigned* i);                        // 0x43b160 (ret 4)
};

struct Agent {
    char pad0[0x10];
    int  f10;                                           // +0x10
    int  f14;                                           // +0x14
    char pad18[0x50 - 0x18];
    float px, py, pz;                                   // +0x50
    char pad5c[0x88 - 0x5c];
    int  f88;                                           // +0x88
    char pad8c[0x1c4 - 0x8c];
    int  f1c4;                                          // +0x1c4 (kind)
    bool is_enemy(const Agent* o) const;                // 0x432350 (ret 4)
};

struct MissionSub {
    float elapsed() const;                              // 0x432c10 (when this = mission+0x6328)
    void  build_geometry();                             // 0x470a00 (when this = mission+0x6698)
};
struct LabelSub {
    void make_label(char* out);                         // 0x48d940 (ret 4)
};

// mission object 0x5e292c
struct Mission {
    char  pad0[0x1c];
    void** pages;                                       // +0x1c (pages of 16 agents)
    char  pad20[0x5c - 0x20];
    int   local_id;                                     // +0x5c
    char  pad60[0x5df0 - 0x60];
    int   cur_rec;                                      // +0x5df0
    int   f5df4;                                        // +0x5df4
    char  pad5df8[0x6328 - 0x5df8];
    // +0x6328  MissionSub (elapsed)
    char  pad6328[0x6338 - 0x6328];
    unsigned char b6338;                                // +0x6338
    char  pad6339[0x633c - 0x6339];
    int   f633c;                                        // +0x633c
    char  pad6340[0x6524 - 0x6340];
    int   f6524;                                        // +0x6524
    unsigned char f6528;                                // +0x6528
    char  pad6529[0x6554 - 0x6529];
    unsigned char b6554;                                // +0x6554
    char  pad6555[0x6558 - 0x6555];
    int   f6558;                                        // +0x6558
    char  pad655c[0x6620 - 0x655c];
    int   f6620;                                        // +0x6620
    char  pad6624[0x666c - 0x6624];
    int   f666c;                                        // +0x666c
    int   f6670;                                        // +0x6670
    int   f6674;                                        // +0x6674
    char  pad6678[0x668c - 0x6678];
    char* f668c;                                        // +0x668c (per-item array, 0x1c8 apart)
    char  pad6690[0x6698 - 0x6690];
    // +0x6698  MissionSub (build_geometry)
    char  pad6698[0x67f4 - 0x6698];
    float f67f4;                                        // +0x67f4
    float f67f8;                                        // +0x67f8
    char  pad67fc[0x6800 - 0x67fc];
    int   f6800;                                        // +0x6800
    char  pad6804[0x6808 - 0x6804];
    __int64 f6808;                                      // +0x6808
    float f6810;                                        // +0x6810
    void  mode_flag1_stub();
    bool  mode_flag1() const;                           // 0x435840
    void  f_527f90();                                   // 0x527f90
    void  f_529d80();                                   // 0x529d80
    void  f_528410(int arg);                            // 0x528410 (ret 4)
    void  frame(float dt, char spawn);                  // 0x483f60
};
extern Mission* g_mission;                              // 0x5e292c

struct GameData {
    char  pad0[0x18];
    unsigned char b18;                                  // +0x18
    char  pad19[0x8c - 0x19];
    int   f8c;                                          // +0x8c
    char  pad90[0xa8 - 0x90];
    int   fa8;                                          // +0xa8
    char  padac[0xc8 - 0xac];
    int   fc8;                                          // +0xc8
    char  padcc[0x139e4 - 0xcc];
    int   f139e4;                                       // +0x139e4
    char  pad139e8[0x13a00 - 0x139e8];
    char* chars;                                        // +0x13a00
    int   f13a04;                                       // +0x13a04
    int   f13a08;                                       // +0x13a08
    char  pad13a0c[0x13a50 - 0x13a0c];
    int*  f13a50;                                       // +0x13a50
    char  pad13a54[0x13ad0 - 0x13a54];
    float f13ad0;                                       // +0x13ad0
    int   f13ad4;                                       // +0x13ad4
    char  pad13ad8[0x13af4 - 0x13ad8];
    int   f13af4;                                       // +0x13af4
    int   f13af8;                                       // +0x13af8
    char  pad13afc[0x13b04 - 0x13afc];
    int   f13b04;                                       // +0x13b04
    int   f13b08;                                       // +0x13b08
    int   f13b0c;                                       // +0x13b0c
    char  pad13b10[0x1a090 - 0x13b10];
    int   f1a090;                                       // +0x1a090
    char  pad1a094[0x1a1a4 - 0x1a094];
    float v1a1a4[4];                                    // +0x1a1a4
    int   f1a1b4;                                       // +0x1a1b4
    char  pad1a1b8[0x1a208 - 0x1a1b8];                  // +0x1a1b8 vector<int>
    int   f1a208;                                       // +0x1a208
    int   f1a20c;                                       // +0x1a20c
    void  f_527c90(int arg);                            // 0x527c90 (ret 4)
};
extern GameData* g_game;                                // 0x5e2930

struct VectorInt {
    void resize(unsigned n, int v);                     // 0x43f310 (ret 8)
};

// mission-source subobject at Hud+0x30 (slice s004d5410)
struct Src {
    void pick(int* arg);                                // 0x4d5410 (ret 4)
    void init(int a, int b);                            // 0x4dd920 (ret 8)
};

// menu + widgets (slice s004d5410 / s00494e40)
struct MainMenu {
    char pad0[0x18];
    int  sel;                                           // +0x18
    void choose(int option);                            // 0x495700 (ret 4)
    void activate(int idx);                             // 0x494260 (ret 4)
    void setup2(tstring* a, tstring* b, tstring* c, int* v);  // 0x48a3d0 (ret 0x10)
};
extern MainMenu g_main_menu;                            // 0xa73980

struct Widget {
    char pad0[4];
    int  f4;                                            // +4
    char pad8[0x14 - 8];
    int  f14;                                           // +0x14 (dword: float bits or int)
    int  f18;                                           // +0x18
};
extern Widget* g_w_a739a0;                              // 0xa739a0
extern Widget* g_w_a739bc;                              // 0xa739bc
extern Widget* g_w_a739b0;                              // 0xa739b0
extern Widget* g_w_a739b4;                              // 0xa739b4
extern Widget* g_w_a739e8;                              // 0xa739e8
extern Widget* g_w_a739e4;                              // 0xa739e4
extern Widget* g_w_a73994;                              // 0xa73994
extern Widget* g_w_a739c8;                              // 0xa739c8
extern Widget* g_w_a739c0;                              // 0xa739c0

struct DialogMgr {
    void start(int a, int b, int c, int d, char f);     // 0x4610b0 (ret 0x14)
};
// The dialog manager is a fixed object at 0x8ed50c8 (outside the tool's annotation range, so it is
// named through a literal cast here).
#define g_dialogmgr (*(DialogMgr*)0x8ed50c8)

struct Flags5e291c {
    char b0, b1, b2, pad3;
    int  f4, f8;
    void commit(char* out);                             // 0x513710 (ret 4)
};
extern Flags5e291c g_flags;                             // 0x5e291c

struct SoundSlot {
    void update(int arg);                               // 0x4625d0 (ret 4)
};
struct SoundBank {
    void update_all(int arg);                           // 0x462760 (ret 4)
};
struct EmitSub {
    char emit_simple(int idx);                          // 0x4875e0 (ret 4)
};

// ---- plain globals ----------------------------------------------------------------------------------
extern unsigned char g_lock_enabled;                    // 0x5e2a09
extern CRITICAL_SECTION g_cs;                           // 0x5e28dc
extern HANDLE g_event;                                  // 0x5e28d8
extern int  g_a5a9fbc;                                  // 0x5a9fbc
extern int  g_a5a9f18;                                  // 0x5a9f18
extern int  g_a5a9c34;                                  // 0x5a9c34
extern int  g_no_cam;                                   // 0x5a9fb0
extern unsigned char g_a17908;                          // 0xa17908
extern int  g_a17414;                                   // 0xa17414
extern int  g_a173f4;                                   // 0xa173f4
extern __int64 g_ticks[];                               // 0x5e2948
extern void* g_rec58;                                   // 0x5e2a00 (points at the 0x58-byte record array)
extern int  g_a735c4;                                   // 0xa735c4
extern void* g_5e29b0;                                  // 0x5e29b0
extern int  g_a5e299c;                                  // 0x5e299c
extern void* g_a17f28;                                  // 0xa17f28
extern unsigned char g_5d75e0;                          // 0x5d75e0
extern int  g_result;                                   // 0xa70458 (?g_result@@3HA)
extern int  g_store1;                                   // 0xa7045c
extern __int64 g_spawn_lo;                              // 0xa70448 (low dword +0, high dword +4)
#define g_clamp_max (*(int*)0x8ee3ac4)                  // 0x8ee3ac4
extern int  g_a7333c;                                   // 0xa7333c
struct Obj73300 { void f_5295a0(); };                   // 0x5295a0
extern Obj73300 g_a73300;                               // 0xa73300
extern int  g_a181b4;                                   // 0xa181b4
extern int  g_a170d8;                                   // 0xa170d8
extern int  g_a174d8;                                   // 0xa174d8

// the 4-int-block deque-like container at 0xa739f8 (map at +4, mapsize at +8, offset at +0xc, size at +0x10)
struct IntDeque {
    int   pad0;
    void** map;
    int   mapsize;
    int   off;
    int   size;
};
extern IntDeque g_deque;                                // 0xa739f8

// float constants in .rdata
extern const float g_f54a6e4;                           // 0x54a6e4 = 0.2f
extern const float g_f549ca8;                           // 0x549ca8 = 0.0f
extern const float g_f54a6d0;                           // 0x54a6d0 = 1e-05f
extern const float g_f549cdc;                           // 0x549cdc = 1.0f
extern const float g_f54b8b0;                           // 0x54b8b0 = 1.5f
extern const float g_f590b38;                           // 0x590b38 = 8.0f
extern const float g_f549d7c;                           // 0x549d7c = 0.1f

// ---- plain callees ----------------------------------------------------------------------------------
void __cdecl restore_defaults();                        // 0x4d3a50
void __cdecl f_516a70();                                // 0x516a70 (quick save)
void __cdecl f_47ce00(void* p, int n);                  // 0x47ce00
char __cdecl f_52b5c0(int agent, int* pair);            // 0x52b5c0
void __cdecl op_delete(void* p);                        // 0x535712 (operator delete, ??3@YAXPAX@Z)
extern "C" int __cdecl _sprintf(char*, const char*, ...);   // 0x536f8f

static __forceinline bool any_pressed(const int* keys, int n)
{
    bool r = false;
    for (int i = 0; i < n; i++) {
        int k = keys[i];
        if (k >= 0 && g_key_a[k] != 0 && g_key_b[k] == 0)
            r = true;
    }
    return r;
}

static __forceinline Agent* page_agent(int id)
{
    char** pages = *(char***)((char*)g_mission + 0x1c);
    return (Agent*)(pages[id >> 4] + (id & 0xf) * 0x5928);
}

static __forceinline int* rec_base(int id)
{
    return (int*)((char*)g_rec58 + id * 0x58 + 0x4c);
}

// -----------------------------------------------------------------------------------------------------
// the HUD / mission-screen object (`this` of 0x5261e0)
struct Screen {
    char  pad0[0x18];
    int   f18;                                          // +0x18
    char  pad1c[0x2c - 0x1c];
    int   f2c;                                          // +0x2c
    char  src30[4];                                     // +0x30 item source (Src)
    void* f34;                                          // +0x34
    int   f38;                                          // +0x38
    int   f3c;                                          // +0x3c
    char  pad40[0x44 - 0x40];
    unsigned char flags44;                              // +0x44
    char  pad45[0x48 - 0x45];
    int   f48;                                          // +0x48
    char  pad4c[0x50 - 0x4c];
    __int64 f50;                                        // +0x50
    float f58;                                          // +0x58
    void* f5c;                                          // +0x5c
    char  pad60[0x218 - 0x60];
    float v218[4];                                      // +0x218
    char  pad228[0x29c - 0x228];
    int   f29c;                                         // +0x29c
    int   f2a0;                                         // +0x2a0
    char  pad2a4[0x2a8 - 0x2a4];
    int   f2a8;                                         // +0x2a8
    char  pad2ac[0x2b0 - 0x2ac];
    __int64 f2b0;                                       // +0x2b0
    int   f2b8;                                         // +0x2b8
    char  pad2bc[0x2c0 - 0x2bc];
    __int64 f2c0;                                       // +0x2c0
    int   f2c8;                                         // +0x2c8
    int   f2cc;                                         // +0x2cc
    char  pad2d0[0x2d4 - 0x2d0];
    int   f2d4;                                         // +0x2d4
    int   f2d8;                                         // +0x2d8
    int   f2dc;                                         // +0x2dc
    unsigned char f2e0;                                 // +0x2e0
    unsigned char f2e1;                                 // +0x2e1
    char  pad2e2[0x2f1 - 0x2e2];
    unsigned char f2f1;                                 // +0x2f1
    unsigned char f2f2;                                 // +0x2f2
    char  pad2f3[0x2f4 - 0x2f3];
    int   f2f4;                                         // +0x2f4
    char  pad2f8[0x324 - 0x2f8];
    int   f324;                                         // +0x324

    void tick(float dt);                                // 0x5261e0
    void f_5284a0(float dt);                            // 0x5284a0 (ret 4)
    void f_529130();                                    // 0x529130
    void f_52b300();                                    // 0x52b300
    void f_529ec0();                                    // 0x529ec0
    void f_52ba10();                                    // 0x52ba10
    void f_52ae70();                                    // 0x52ae70
    void refresh();                                     // 0x4f8b40 (ignores this, but takes ecx)
    void camera_update(float dt);                       // 0x4f8c80 (ret 4)
    void status_widget_update();                        // 0x4fb350
};

// vtable of the object at Screen+0x5c: only slot 0xc4 (index 49) is called here.
struct SrcVtbl {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void v24(); virtual void v25(); virtual void v26(); virtual void v27();
    virtual void v28(); virtual void v29(); virtual void v30(); virtual void v31();
    virtual void v32(); virtual void v33(); virtual void v34(); virtual void v35();
    virtual void v36(); virtual void v37(); virtual void v38(); virtual void v39();
    virtual void v40(); virtual void v41(); virtual void v42(); virtual void v43();
    virtual void v44(); virtual void v45(); virtual void v46(); virtual void v47();
    virtual void v48(); virtual void v49();
};

// @ 0x5261e0
void Screen::tick(float dt)
{
    float sdt;              // local_438
    bool  b6;               // local at esp+0x12
    unsigned char flag13;   // esp+0x13 ("value valid")
    int   val14;            // esp+0x14 (dword; compared as int in the original)

    sdt = dt;

    if (g_lock_enabled)
        EnterCriticalSection(&g_cs);

    // ---- key toggles (slow-motion / pause) ----------------------------------------------------------
    if (g_a5a9fbc) {
        if ((g_key_a[29] || g_key_a[157]) && g_key_a[67] && !g_key_b[67])
            f2e1 = (unsigned char)!f2e1;
        if (g_a5a9fbc && (g_key_a[29] || g_key_a[157]) && g_key_a[87] && !g_key_b[87])
            f2e0 = (unsigned char)!f2e0;
    }
    if (f2e0)
        sdt = 0.0f;
    else if (f2e1)
        sdt = dt * g_f54a6e4;

    // ---- scene time-scale blend --------------------------------------------------------------------
    if (g_a5a9fbc) {
        Mission* m = g_mission;
        if (m->f6810 > 0.0f) {
            __int64 d = g_ticks[m->f6800] - *(__int64*)((char*)m + 0x6808);
            float r = (float)d * g_f54a6d0 / m->f6810;
            if (r >= 1.0f) {
                *(int*)&m->f67f4 = *(int*)&m->f67f8;
                sdt = sdt * m->f67f4;
                m->f6810 = 0.0f;
            } else {
                sdt = (r * m->f67f8 + (1.0f - r) * m->f67f4) * sdt;
            }
        } else {
            sdt = sdt * m->f67f4;
        }
    }

    // ---- renderer "ready" rate keys ----------------------------------------------------------------
    {
        char c7 = g_keyready.is_ready(0xd0);
        unsigned char c8 = g_a17908;
        if (c7 || (g_a17414 != 0 && c8 != 0))
            g_a5a9c34 -= 5;
        c7 = g_keyready.is_ready(0xc8);
        if (c7 || (g_a173f4 != 0 && c8 != 0))
            g_a5a9c34 += 5;
    }

    f_5284a0(sdt);
    g_mission->b6554 = 0;
    g_mission->frame(sdt, 1);
    if (g_mission->local_id >= 0) {
        status_widget_update();
        f_529130();
    }
    camera_update(sdt);
    g_gui.flush();

    // ---- no-camera position save -------------------------------------------------------------------
    if (g_no_cam && (g_key_a[29] || g_key_a[157]) && g_key_a[18] && !g_key_b[18]) {
        if (g_game->f1a090 != 0) {
            restore_defaults();
            if (g_lock_enabled) {
                LeaveCriticalSection(&g_cs);
                SetEvent(g_event);
            }
            return;
        }
        if (g_mission->local_id < 0) {
            *(int*)((char*)g_game + 0x1a1a4) = *(int*)((char*)this + 0x218);
            *(int*)((char*)g_game + 0x1a1a8) = *(int*)((char*)this + 0x21c);
            *(int*)((char*)g_game + 0x1a1ac) = *(int*)((char*)this + 0x220);
            *(int*)((char*)g_game + 0x1a1b0) = *(int*)((char*)this + 0x224);
        } else {
            char* p = (char*)page_agent(g_mission->local_id) + 0x50;
            *(int*)((char*)g_game + 0x1a1a4) = *(int*)p;
            *(int*)((char*)g_game + 0x1a1a8) = *(int*)(p + 4);
            *(int*)((char*)g_game + 0x1a1ac) = *(int*)(p + 8);
            *(int*)((char*)g_game + 0x1a1b0) = *(int*)(p + 12);
            *(float*)((char*)g_game + 0x1a1ac) += g_f54b8b0;
        }
        g_game->f139e4 = 0;
        g_game->f1a090 = 1;
        *(int*)((char*)g_game + 0x1a20c) = 0;
        g_game->f1a1b4 = -1;
        ((VectorInt*)((char*)g_game + 0x1a1b8))->resize(0, 0);
        *(int*)((char*)g_game + 0x1a1cc) = 0;
        f_52ae70();
    }
    if (g_game->f1a090 != 0) {
        f_52ba10();
        if (g_lock_enabled) {
            LeaveCriticalSection(&g_cs);
            SetEvent(g_event);
        }
        return;
    }

    // ---- "mission started" pending item ------------------------------------------------------------
    if (g_mission->b6338 == 0) {
        b6 = true;
    } else {
        float e = ((MissionSub*)((char*)g_mission + 0x6328))->elapsed();
        if (!(0.0f <= e)) {
            b6 = true;
        } else {
            int u14 = g_deque.off + g_deque.size - 1;
            unsigned page = (unsigned)u14 >> 2;
            unsigned sub = (unsigned)u14 & 3;
            if ((unsigned)g_deque.mapsize <= page)
                page -= (unsigned)g_deque.mapsize;
            int* blk = (int*)g_deque.map[page];
            b6 = (blk[sub] == 0);
        }
    }
    if (g_a5a9f18 != -1) {
        if (g_a5a9f18 == 0) {
            ((MissionSub*)((char*)g_mission + 0x6698))->build_geometry();
            g_mission->f_527f90();
            g_mission->f_529d80();
            g_game->f_527c90(g_mission->f5df4);
        } else {
            b6 = false;
        }
        g_a5a9f18 = -1;
    }

    // ---- mission-editor key block ------------------------------------------------------------------
    if (g_mission->local_id >= 0) {
        if (g_key_a[29] == 0 && g_key_a[157] == 0) {
            if (g_keytable.down(0x15)) {
                f2a0 = 0;
                f_52b300();
            } else if (g_keytable.down(0x16)) {
                f2a0 = 1;
                f_52b300();
            } else if (g_keytable.down(0x17)) {
                f2a0 = 2;
                f_52b300();
            } else if (g_keytable.down(0x1a)) {
                f2a0 = 5;
                f_52b300();
            } else if (g_keytable.down(0x1b)) {
                f2a0 = 6;
                f_52b300();
            } else if (g_keytable.down(0x1c)) {
                f2a0 = 7;
                f_52b300();
            } else if (g_keytable.down(0x1d)) {
                f2a0 = 8;
                f_52b300();
            } else if (g_keytable.down(0x18)) {
                if (g_mission->f6674 == 0) {
                    f2a0 = 4;
                    g_mission->f6674 = 1;
                } else {
                    f2a0 = 3;
                    g_mission->f6674 = 0;
                }
                f_52b300();
            } else if (g_keytable.down(0x1e)) {
                if (g_mission->f666c == 0) {
                    f2a0 = 9;
                    g_mission->f666c = 1;
                } else {
                    f2a0 = 10;
                    g_mission->f666c = 0;
                }
                f_52b300();
            } else if (g_keytable.down(0x19)) {
                if (g_mission->f6670 == 0) {
                    f2a0 = 0xc;
                    g_mission->f6670 = 1;
                } else {
                    f2a0 = 0xd;
                    g_mission->f6670 = 0;
                }
                f_52b300();
            }
        }
        if (g_keytable.down(0x1f) || g_keytable.down(0x20) || g_keytable.down(0x21) ||
            g_keytable.down(0x22) || g_keytable.down(0x23)) {
            bool b3 = (g_key_a[29] || g_key_a[157] || g_key_a[42] || g_key_a[54]);
            if (b3)
                *(int*)((char*)g_game + 0x1a1d4) = 1;
            else
                *(int*)((char*)g_game + 0x1a1d4) = 0;

            int kind = page_agent(g_mission->local_id)->f1c4;
            if (kind < 0 || 8 <= kind)
                kind = 7;
            char* base = (char*)g_mission + 0x527c + kind * 0x16c;

            if (g_keytable.down(0x23)) {
                bool changed = false;
                for (int i = 0; i < 3; i++) {
                    int* p = (int*)(base + 0x4c + i * 0x6c);
                    if (*p == 0) {
                        *p = 1;
                        changed = true;
                    } else {
                        *p = 0;
                    }
                }
                if (!changed) {
                    for (int i = 0; i < 3; i++)
                        *(int*)(base + 0x4c + i * 0x6c) = 1;
                }
            } else if (g_keytable.down(0x1f)) {
                for (int i = 0; i < 3; i++)
                    *(int*)(base + 0x4c + i * 0x6c) = 1;
            } else {
                if (!b3) {
                    for (int i = 0; i < 3; i++)
                        *(int*)(base + 0x4c + i * 0x6c) = 0;
                }
                int idx = 0;
                if (g_keytable.down(0x20))
                    idx = 0;
                else if (g_keytable.down(0x21))
                    idx = 1;
                else if (g_keytable.down(0x22))
                    idx = 2;
                *(int*)(base + 0x4c + idx * 0x6c) = 1;
            }

            bool all = true;
            for (int i = 0; i < 3; i++) {
                if (*(int*)(base + 0x4c + i * 0x6c) == 0)
                    all = false;
            }
            if (all) {
                g_gui.add(*g_strtable.lookup(&tstring("ui_everyone_hear_me")), 0xffddee88);
            } else {
                char label[512];
                ((LabelSub*)base)->make_label(label);
                const char* fmt = g_strtable.lookup(&tstring("ui_str_hear_me"))->c_str();
                char buf[512];
                _sprintf(buf, fmt, label, buf);
                g_gui.add(tstring(buf), 0xffddee88);
            }
        }
    }

    // ---- region-change / texture timers ------------------------------------------------------------
    {
        __int64 d = g_ticks[f2a8] - f2b0;
        if ((float)d * g_f54a6d0 > g_f590b38 && g_mission->local_id >= 0) {
            f29c = 0;
            __int64 d2 = g_ticks[f2b8] - f2c0;
            if ((float)d2 * g_f54a6d0 > g_f549d7c) {
                f_529ec0();
                f2c0 = g_ticks[f2b8];
            }
            if (f2c8 == -1) {
                g_gui.set_o568(0, 1);
                float v[2];
                v[0] = 0.5f;
                v[1] = 0.5f;
                g_gui.set_view(v);
            }
        }
    }

    // ---- item-use keys + sound slots ---------------------------------------------------------------
    flag13 = 0;
    val14 = -1;
    if (g_mission->local_id >= 0) {
        if (any_pressed(g_keys_7350, 2) && f2c8 == 4 && f2d4 != 0) {
            int v = *(int*)((char*)g_game->f13a50 + 0x7c +
                            (g_mission->f5df4 * 0x125 + f2cc) * 4);
            if (v == 100000)
                b6 = false;
            else {
                g_game->f13af4 = v;
                g_game->f13af8 = 0;
            }
        } else if (any_pressed(g_keys_7350, 2) && f2c8 == 1 && f2d4 != 0) {
            g_mission->f6620 = f2cc;
        } else if (any_pressed(g_keys_7350, 2) && f2c8 == 7 && f2d4 != 0) {
            int off = f2cc * 0x1c8;
            int pair[2];
            pair[0] = *(int*)((char*)g_mission->f668c + off + 0x18c);
            pair[1] = *(int*)((char*)g_mission->f668c + off + 0x1a0);
            if (f_52b5c0(g_mission->local_id, pair)) {
                g_mission->f_528410(f2cc);
                refresh();
            }
        } else if (any_pressed(g_keys_7350, 2) && f2c8 == 5 && f2d4 != 0) {
            int v = *(int*)((char*)g_mission->f668c + f2cc * 0x1c8 + 0x1a0);
            if (v < *(int*)((char*)g_game->f13a50 + g_mission->f5df4 * 0x494 + 0x100) && 0 <= v)
                val14 = *(int*)((char*)g_game->f13a50 + 0xc0 +
                                (g_mission->f5df4 * 0x125 + v) * 4);
            else
                val14 = -1;
            flag13 = 1;
        } else if (any_pressed(g_keys_7350, 2) && f2c8 == 6 && f2d4 != 0) {
            int idx = *(int*)((char*)g_mission->f668c + f2cc * 0x1c8 + 0x1a0);
            g_game->f13af8 = idx;
            char* obj = *(char**)((char*)g_rec58 + g_a735c4 * 0x60 + 0x58);
            ((EmitSub*)(obj + *(int*)((char*)g_mission->f668c + f2cc * 0x1c8 + 0x1a4) * 0x68 + 0x4c))
                ->emit_simple(0x10);
        }

        if (any_pressed(g_keys_73b0, 2)) {
            g_result = 0;
            g_store1 = 0;
            int* slots = rec_base(g_mission->cur_rec);
            for (int i = 0; i < *slots; i++)
                ((SoundSlot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-0x16);
            if (g_result != 0 || g_store1 != 0)
                flag13 = 1;
        }
    }

    if (g_no_cam && g_key_a[56] && g_key_a[38] == 0 && g_key_b[38] != 0)
        f_47ce00((char*)g_mission + 0x6340, g_mission->f633c);

    if (f2d8 == 0) {
        b6 = false;
    } else if (f2dc == 0) {
        int* slots = rec_base(g_mission->cur_rec);
        for (int i = 0; i < *slots; i++)
            ((SoundSlot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-0x1e);
        b6 = false;
    }

    if (any_pressed(g_keys_7388, 2) && g_key_a[56] == 0 && g_key_a[184] == 0) {
        g_result = 0;
        g_store1 = 0;
        ((SoundBank*)rec_base(g_mission->cur_rec))->update_all(-0x15);
        if (g_result != 0 || g_store1 != 0)
            b6 = false;
    }

    if (g_game->f1a208 >= 0) {
        int v = g_game->f1a208;
        g_spawn_lo = v;
        int* slots = rec_base(g_mission->cur_rec);
        for (int i = 0; i < *slots; i++)
            ((SoundSlot*)((char*)*(void**)(slots + 1) + i * 0x78))->update(-8);
        g_game->f1a208 = -1;
    }

    // ---- "leave area" / agent selection ------------------------------------------------------------
    if ((g_mission->f6528 & 1) == 0) {
        int flags20 = *(int*)((char*)g_rec58 + g_mission->cur_rec * 0x58 + 0x20);
        if (any_pressed(g_keys_7350, 2) && f2c8 == 8 &&
            (((flags20 >> 1) & 1) == 0 || g_mission->f6524 > 0)) {
            f2dc = 0;
        } else if (((flags20 >> 1) & 1) != 0 && g_mission->local_id >= 0 &&
                   g_keytable.down(4) && f2c8 == 8) {
            AgentTable* table = (AgentTable*)((char*)g_mission + 4);
            float best = 10000.0f;
            unsigned id = (unsigned)table->first_used();
            Agent* player = page_agent(g_mission->local_id);
            while (id < (unsigned)table->count) {
                Agent* a = page_agent((int)id);
                if (a->f14 == 1 && a->f10 == 0 && player->is_enemy(a)) {
                    float dx = a->px - player->px;
                    float dy = a->py - player->py;
                    float dz = a->pz - player->pz;
                    float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
                    if (dist < best)
                        best = dist;
                }
                table->next_used(&id);
            }
            if (g_keytable.down(4) && f2c8 == 8) {
                if (best > 8.0f) {
                    f2dc = -1;
                    g_main_menu.setup2(
                        g_strtable.lookup(&tstring("ui_question_leave_area")),
                        g_strtable.lookup(&tstring("ui_yes")),
                        g_strtable.lookup(&tstring("ui_no")),
                        &f2dc);
                    g_main_menu.activate(0);
                } else {
                    g_gui.add(*g_strtable.lookup(&tstring("ui_cant_retreat_there_are_enemies_nearby")),
                              0xffff3344);
                }
            }
        }
    } else {
        if (any_pressed(g_keys_7350, 2) && f2c8 == 8)
            f2dc = 0;
    }

    // ---- conversation / mission setup --------------------------------------------------------------
    if (g_game->fc8 == 0xf) {
        g_game->fc8 = 0;
        f324 = g_game->f13b04;
    }
    if (f324 >= 0 && g_a5e299c == 0) {
        ((Src*)((char*)this + 0x30))->init(f2c, f324);
        if ((flags44 & 1) == 0) {
            if (g_a17f28 != 0)
                ((void (__cdecl*)(void*, int))*(void**)(*(int*)g_a17f28 + 0x30))(g_a17f28, 1);
            g_5d75e0 = 1;
        }
    }
    if (g_a5e299c != 0) {
        ((Src*)((char*)this + 0x30))->pick((int*)&val14);
        __int64 d = g_ticks[f48] - f50;
        if ((float)d * g_f54a6d0 > f58) {
            g_a5e299c = 0;
            if (f34 != 0)
                op_delete(f34);
            f34 = 0;
            f38 = 0;
            f3c = 0;
            ((SrcVtbl*)f5c)->v49();
            if (g_a17f28 != 0)
                ((void (__cdecl*)(void*, int))*(void**)(*(int*)g_a17f28 + 0x30))(g_a17f28, 0);
            g_5d75e0 = 0;
            f324 = -1;
        }
    }

    if (b6) {
        // ---- boolean-true branch -------------------------------------------------------------------
        if (flag13 != 0 && g_mission->local_id >= 0 && g_game->b18 == 0) {
            f2f1 = 1;
            g_w_a739a0->f18 = 0xe;
            if (val14 < 0) {
                g_w_a739a0->f4 = 0;
                g_main_menu.choose(8);
            } else {
                g_w_a739a0->f4 = 3;
                g_w_a739a0->f14 = val14;
                if (g_w_a739a0->f14 < 0) {
                    g_w_a739a0->f14 = 0;
                    g_main_menu.choose(8);
                } else {
                    int lim = g_game->f13a04 - 1;
                    if (g_w_a739a0->f14 > lim)
                        g_w_a739a0->f14 = lim;
                    g_main_menu.choose(8);
                }
            }
            goto after;
        }

        if (any_pressed(g_keys_73a8, 2) && g_mission->local_id >= 0 && g_game->b18 == 0) {
            f2f2 = 1;
            char* rec = (char*)g_game->chars;
            int v = (unsigned char)rec[0x7a] & 0xf;
            if (v > g_clamp_max)
                v = g_clamp_max;
            f2f4 = *(int*)(rec + 0x58) + 0x23 + v * 2;
            g_w_a739bc->f18 = 0xe;
            g_w_a739bc->f4 = 0;
            g_w_a739bc->f14 = g_game->f8c;
            g_main_menu.choose(0xf);
            goto after;
        }

        {
            int flags20 = *(int*)((char*)g_rec58 + g_mission->cur_rec * 0x58 + 0x20);
            if (any_pressed(g_keys_73b8, 2) && g_game->b18 == 0 &&
                (((flags20 >> 1) & 1) == 0)) {
                g_w_a739b0->f18 = 0xe;
                g_w_a739b0->f4 = 5;
                g_main_menu.choose(0xc);
                goto after;
            }
        }

        if (any_pressed(g_keys_73c0, 2) && g_game->b18 == 0) {
            if (g_a181b4 == 0) {
                g_w_a739b4->f18 = 0xe;
                g_w_a739b4->f4 = 0;
                g_main_menu.choose(0xd);
            } else {
                g_w_a739e8->f18 = 0xe;
                *(int*)((char*)g_game + 0x13b08) = 4;
                *(int*)((char*)g_game + 0x13b0c) = 0;
                g_main_menu.choose(0x1a);
            }
            goto after;
        }

        if (any_pressed(g_keys_73c8, 2) && g_game->b18 == 0) {
            g_w_a739e4->f18 = 0xe;
            g_w_a739e4->f4 = 0;
            g_main_menu.choose(0x19);
            goto after;
        }

        if (g_a170d8 != 0 && g_a174d8 == 0) {
            char c = g_mission->mode_flag1();
            if (c == 0) {
                g_w_a73994->f18 = 0xe;
                g_main_menu.choose(5);
            } else {
                g_w_a739c8->f18 = 0xe;
                g_main_menu.choose(0x12);
            }
            goto after;
        }

        if (g_game->b18 == 0 && g_keytable.down(0x24)) {
            if (!g_mission->mode_flag1() && g_game->b18 == 0) {
                f_516a70();
            } else {
                g_gui.add(*g_strtable.lookup(&tstring("ui_cant_quick_save")), 0xffffffff);
            }
            goto after;
        }

        if (g_mission->f6620 != -1) {
            if (g_mission->f6620 < 0)
                goto after;
            f2f1 = 1;
            unsigned id = (unsigned)g_mission->f6620;
            g_mission->f6620 = -1;
            Agent* a = page_agent((int)id);
            int dialog_id = a->f88;
            g_w_a739c0->f18 = 0xe;
            g_dialogmgr.start(dialog_id, (int)id, -1, 0, 0);
            g_mission->f6558 = a->f88;
            g_game->f13a08 = a->f88;
            g_main_menu.choose(0x10);
            goto after;
        }

        if (g_game->f13af4 < 0)
            goto after;
        g_a73300.f_5295a0();
        g_a7333c |= 8;
        g_flags.commit((char*)&flag13);
        goto unlock;
    }

    // ---- boolean-false branch ----------------------------------------------------------------------
    g_a73300.f_5295a0();
    if (f18 != 100) {
        if (g_game->f13ad0 > 0.0f && g_game->f13ad4 != 0)
            g_main_menu.choose(7);
        else
            g_main_menu.choose(f18);
        goto unlock;
    }
    if (0 < g_a735c4) {
        int f = *(int*)((char*)g_5e29b0 + g_a735c4 * 0x60 + 0x20);
        if ((f & 0x10) != 0)
            g_game->fa8 |= 1;
    }
    g_flags.commit((char*)&flag13);

after:
unlock:
    if (g_lock_enabled) {
        LeaveCriticalSection(&g_cs);
        SetEvent(g_event);
    }
}
