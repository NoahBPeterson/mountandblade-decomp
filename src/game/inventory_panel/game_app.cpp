// Slice s0050ec40: the main loop's per-frame "game update" (0x50ec40), the application's window
// procedure override (0x50f130, CD3DApplication::MsgProc at 0x403120) and the loading screen's
// step function (0x50f390).  Class layouts follow the verified wave-1 slices of the DirectX framework
// (s00401010: CD3DApplication = `Application`) and the engine's tstring (s0042a1f0) / widget vtables.
// Names are provisional.
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>
#include <math.h>

// ---- tstring (0x10 bytes: read cursor, buffer, capacity, length) -------------------------------------
#include "../tstring.h"

// the engine's "null or pointer" read used all over the code base (buf == 0 -> the field itself)
static __forceinline const char* c_str(const tstring* s)
{
    char* p = s->buf;
    if (!p) p = (char*)&s->buf;
    return p;
}

// ---- CD3DApplication (verified layout: s00401010/s00401010.h) -----------------------------------------
// only the data members are modelled; the base vtable is never dispatched through here (MsgProc is
// called non-virtually at 0x403120).
struct Application {
    char pad0[4];
    char f4;                            // 0x4 (a flag set when 0x5e2a08 is set)
    char pad5[0xa4 - 5];
    char m_bWindowed;                   // 0xa4
    bool m_bActive;                     // 0xa5
    char padA6[0xe8 - 0xa6];
    HWND m_hWnd;                        // 0xe8
    char padEC[0xc];                    // 0xec
    IDirect3DDevice9* m_pd3dDevice;     // 0xf8
    char padFC[0x278 - 0xfc];
    float m_fElapsedTime;               // 0x278
    char pad27C[0x344 - 0x27c];
    char m_bShowCursorWhenFullscreen;   // 0x344
    char pad345[0x34c - 0x345];
    int  m_34c;                         // 0x34c (last char / key; -1 when consumed)
    char pad350[0x358 - 0x350];
    HINSTANCE m_hInstance;              // 0x358
    char pad35C[0x374 - 0x35c];
    HWND m_hMapEditorDlg;               // 0x374
    HWND m_hScriptDlg;                  // 0x378

    int MsgProc(HWND hWnd, unsigned msg, unsigned wparam, unsigned lparam);   // 0x403120 (base)
};

// the derived application class the two functions above belong to
struct GameApp : Application {
    int MsgProc(HWND hWnd, unsigned msg, unsigned wparam, unsigned lparam);   // 0x50f130
    int Update();                                                            // 0x50ec40
};

// ---- screens: g_screens[30] (0xa73980) is the global screen table; the id stack at 0xa739f8 keeps
//      the current screen id (a ring of 4-int blocks). ------------------------------------------------
// ---- the engine widget interface: only the slots this slice dispatches ---------------------------------
#include "../ui_widget.h"                 // Widget (the UI widget interface)

struct ScreenObj : Widget {
    void update_all();                                       // 0x4ec460 (non-virtual)
};
extern ScreenObj* g_screens[30];       // 0xa73980 (.. 0xa739f4)

struct IdStack {                    // 0xa739f8
    int f0;                         // 0xa739f8
    int** blocks;                   // 0xa739fc
    unsigned block_count;           // 0xa73a00
    unsigned a;                     // 0xa73a04
    unsigned b;                     // 0xa73a08
};
extern IdStack g_id_stack;              // 0xa739f8

static __forceinline int current_screen_id()
{
    unsigned i = g_id_stack.b + g_id_stack.a - 1;
    unsigned q = i >> 2;
    unsigned r = i - q * 4;
    if (q >= g_id_stack.block_count) q -= g_id_stack.block_count;
    return g_id_stack.blocks[q][r];
}

// ---- game (0x5e2930).  Only the members touched here. ------------------------------------------------
struct Game {
    char pad0[0x1a060];
    int f1a060;                         // 0x1a060  script dialog requested
    int f1a064;                         // 0x1a064  script dialog shown
    int f1a068;                         // 0x1a068
    void fn_4c8ca0(int which);          // 0x4c8ca0 (ECX = this)
    char pad1a06c[0x1a090 - 0x1a06c];
    int f1a090;                         // 0x1a090  map editor active
    char pad1a094[0x1a160 - 0x1a094];
    int f1a160;                         // 0x1a160  map editor dialog shown
    char pad1a164[0x1a1b4 - 0x1a164];
    int f1a1b4;                         // 0x1a1b4
    char pad1a1b8[0x1a1bc - 0x1a1b8];
    int* f1a1bc;                        // 0x1a1bc  int vector begin
    int* f1a1c0;                        // 0x1a1c0  int vector end
    char pad1a1c4[0x1a1c8 - 0x1a1c4];
    int f1a1c8;                         // 0x1a1c8  map editor list needs rebuilding
};
extern Game* g_game;                    // 0x5e2930

// ---- globals ------------------------------------------------------------------------------------------
extern int   g_counter;                 // 0x5e28cc
extern float g_8ee42f0;                 // 0x8ee42f0
extern __int64 g_5e2970;                // 0x5e2970 (frame time step, scaled)
extern __int64 g_ticks;                 // 0x5e2948
extern float g_5a9c20;                  // 0x5a9c20
extern int   g_5d8798;                  // 0x5d8798
extern int   g_5e293c;                  // 0x5e293c
extern char  g_5d75e0;                  // 0x5d75e0
extern char  g_5e2a08;                  // 0x5e2a08
extern int   g_no_cam;                  // 0x5a9fb0
extern int   g_a17158, g_a17558, g_a17348, g_a17148;    // 0xa17158 ..
extern int   g_a1719c, g_a1759c, g_a171ac, g_a1717c;    // 0xa1719c ..
extern char  g_5e2630;                  // 0x5e2630
extern char  g_keys[256];               // 0x5e27c8
// the Direct3D9 device (0xa17f28); __stdcall virtuals -> the COM-style call pattern
struct IDev9 {
    virtual long __stdcall s0(); virtual long __stdcall s1(); virtual long __stdcall s2();
    virtual long __stdcall s3(); virtual long __stdcall s4(); virtual long __stdcall s5();
    virtual long __stdcall s6(); virtual long __stdcall s7(); virtual long __stdcall s8();
    virtual long __stdcall s9(); virtual long __stdcall s10();
    virtual void __stdcall SetCursorPosition(int x, int y, unsigned flags);   // slot 11 (+0x2c)
    virtual int  __stdcall ShowCursor(int show);                              // slot 12 (+0x30)
};
extern IDev9* g_a17f28;                 // 0xa17f28 (the Direct3D device)
extern char  g_a739b8[];                // 0xa739b8 (current editor entity)
// the current editor entity (a pointer at 0xa739b8)
struct EditorEntity {
    void pick();                        // 0x4ed8d0
    char pad2f9[0x2f9];
    char f2f9;                          // 0x2f9
};
extern EditorEntity* g_entity;          // 0xa739b8

// gui manager object (0x5d7080) and a few of its member functions
struct GuiManager {
    void frame(float dt);               // 0x4ebfd0
    void add_log(tstring s, int type);  // 0x452c00 (tstring by value, then the type)
};
extern GuiManager g_gui_manager;        // 0x5d7080

struct ResMgr {
    void fn(int id);                    // 0x418ff0
};
extern ResMgr g_resmgr;                 // 0x5d6fa0

struct MissionObj {
    void fn(float dt);                  // 0x487930
};
extern MissionObj g_a170c8;             // 0xa170c8

struct MusicList {
    void fn();                          // 0x4c4100
};
extern MusicList g_a73a10;              // 0xa73a10

struct Output {
    virtual void o0(); virtual void o1(); virtual void o2(); virtual void o3();
    virtual void o4(); virtual void o5(); virtual void o6(); virtual void o7();
    virtual void o8(); virtual void o9();
    virtual void print(const char* s);  // +0x28
};
extern Output g_5a9f60;                 // 0x5a9f60

struct MainMenu { void choose(int i); };                     // 0x495700
extern MainMenu g_main_menu;            // 0xa73980

#include "../map_editor_obj.h"        // MapEditor (the object at 0x8ee3c48)

// ---- free functions --------------------------------------------------------------------------------
extern "C" void __stdcall _FSOUND_Update(void);              // 0x53433c
void bump_counter(void);                                     // 0x42cb70 (?bump_counter@@YAXXZ)
int  build_shader(void);                                     // 0x4c17c0
void mb_error(const char* msg);                              // 0x42cc80
void fn_4c3350(void);                                        // 0x4c3350
void fn_4d0c20(void);                                        // 0x4d0c20
void fn_50d5e0(void);                                        // 0x50d5e0
void map_dialog_proc();                                      // 0x50d250 (dialog proc, never called here)
void script_dialog_proc();                                   // 0x4c3340
void global_4bf870(void*);                                   // 0x4c3330
void global_4c03b0(void*);                                   // 0x4c3340

// ---- the loading screen (0x50f440 lives in slice s0050f440; see there) ------------------------------
struct LoadingScreen {                  // fields used by 0x50f390
    char pad0[0x30];
    ScreenObj* text_widget;                // 0x30
    char pad34[4];
    ScreenObj* bar_widget;                 // 0x38
    int step;                           // 0x3c
    int total;                          // 0x40
    int f44;                            // 0x44
    int f48;                            // 0x48
    void update(int unused);            // 0x50f390
    bool advance(int* step, int* total, tstring* text);   // 0x50f440
};

// ---- the per-frame update ---------------------------------------------------------------------------
// @ 0x50ec40
int GameApp::Update()
{
    float dt;
    __int64 t;

    g_counter++;
    bump_counter();

    dt = m_fElapsedTime;
    if (dt > 0.1f) dt = 0.1f;
    *(float*)0x8ee42f0 = dt;
    t = (__int64)(dt * *(float*)0x54b86c);
    g_5e2970 = t;
    g_ticks += t;
    g_5a9c20 = dt;
    g_5d8798 = m_34c;
    m_34c = -1;

    g_gui_manager.frame(dt);
    g_a170c8.fn(dt);
    g_a73a10.fn();
    _FSOUND_Update();

    if (g_no_cam) {
        if (g_a17158 && !g_a17558 && (g_a17348 || g_a17148)) {
            if (build_shader() < 0) {
                mb_error("Unable to compile the effect file.");
            } else {
                g_gui_manager.add_log(tstring("Effect file recompiled."), -1);
            }
            g_resmgr.fn(g_5e293c);
        }
    }
    if (g_no_cam) {
        if (g_a1719c && !g_a1759c) {
            if ((g_a17348 || g_a17148) && (g_a171ac || g_a1717c)) {
                fn_4c3350();
                g_gui_manager.add_log(tstring("Animation data restored."), -1);
            } else if (g_a17348 || g_a17148) {
                fn_4d0c20();
                fn_50d5e0();
                g_gui_manager.add_log(tstring("Module data restored."), -1);
            }
        }
    }

    g_screens[current_screen_id()]->update_all();
    g_screens[current_screen_id()]->update(dt);

    if (g_game) {
        if (current_screen_id() == 0xe) {
            if (g_game->f1a090) {
                if (!g_game->f1a160) {
                    if (!m_hMapEditorDlg) {
                        m_hMapEditorDlg = CreateDialogParamA(GetModuleHandleA(0), (LPCSTR)0xc0, m_hWnd,
                                                             (DLGPROC)map_dialog_proc, 0);
                    } else {
                        ((MapEditor*)this)->update_object_type();
                    }
                    ((MapEditor*)this)->update_objects();
                    ShowWindow(m_hMapEditorDlg, 5);
                    g_game->f1a160 = 1;
                }
                if (g_game->f1a1c8) {
                    HWND item = GetDlgItem(m_hMapEditorDlg, 0x438);
                    int n = (int)SendMessageA(item, 0x18b, 0, 0);
                    int i;
                    for (i = 0; i < n; i++)
                        SendMessageA(item, 0x185, 0, i);
                    if (g_game->f1a1b4 >= 0) {
                        for (i = 0; i < n; i++) {
                            int v = (int)SendMessageA(item, 0x199, i, 0);
                            int* p = g_game->f1a1bc;
                            unsigned j = 0;
                            while (p && j < (unsigned)(g_game->f1a1c0 - p)) {
                                if (v == p[j]) {
                                    SendMessageA(item, 0x185, 1, i);
                                    break;
                                }
                                j++;
                            }
                        }
                    }
                    g_entity->pick();
                    g_entity->f2f9 = 1;
                    SetFocus(m_hWnd);
                    g_game->f1a1c8 = 0;
                }
            }
        } else if (!g_game->f1a090) {
            if (g_game->f1a160) {
                if (m_hMapEditorDlg) {
                    ShowWindow(m_hMapEditorDlg, 0);
                    g_game->f1a160 = 0;
                }
            }
        }
        if (g_game->f1a060) {
            if (!g_game->f1a064) {
                if (!m_hScriptDlg) {
                    m_hScriptDlg = CreateDialogParamA(GetModuleHandleA(0), (LPCSTR)0xdf, m_hWnd,
                                                      (DLGPROC)global_4bf870, 0);
                }
                ShowWindow(m_hScriptDlg, 5);
                g_game->f1a064 = 1;
            }
        } else if (g_game->f1a064) {
            if (m_hScriptDlg) {
                EndDialog(m_hScriptDlg, 2);
                m_hScriptDlg = 0;
            }
            g_game->f1a064 = 0;
        }
    }

    m_bShowCursorWhenFullscreen = g_5d75e0;
    if (g_5e2a08) ((GameApp*)this)->f4 = true;
    return 0;
}

// ---- the window procedure ---------------------------------------------------------------------------
// @ 0x50f130
int GameApp::MsgProc(HWND hWnd, unsigned msg, unsigned wparam, unsigned lparam)
{
    POINT pt;

    // the case bodies are laid out by the compiler in this order (see the disassembly at 0x50f163..)
    switch (msg) {
    case 0x101:                                 // WM_KEYUP
        *(char*)(0x5e27c8 + wparam) = 0;
        break;
    case 0x100:                                 // WM_KEYDOWN
        *(char*)(0x5e27c8 + wparam) = 1;
        break;
    case 0x20:                                  // WM_SETCURSOR
        if (g_a17f28 && m_bWindowed) {
            SetCursor(0);
            g_a17f28->ShowCursor(1);
            return 1;
        }
        break;
    case 0x111:                                 // WM_COMMAND
        switch ((unsigned short)wparam) {
        case 0x9c5b:
            if (g_game) {
                g_game->fn_4c8ca0(0);
                MessageBoxA(0, "Language Template Files Generated Under new_language directory!",
                            "INFO", 0);
            } else {
                MessageBoxA(0, "Create a new game or restore a game before using this function!",
                            "INFO", 0);
            }
            break;
        case 0x9c5c:
            if (g_game) {
                g_game->fn_4c8ca0(1);
                MessageBoxA(0, "Language Template Files Generated Under new_language directory! "
                               "Non-translated texts are marked with a {***} sign. You can use this "
                               "as the search keyword.", "INFO", 0);
            } else {
                MessageBoxA(0, "Create a new game or restore a game before using this function!",
                            "INFO", 0);
            }
            break;
        case 0x9c5a:
            fn_4d0c20();
            fn_50d5e0();
            g_gui_manager.add_log(tstring("Module data restored."), -1);
            break;
        case 0x9c56:
            if (!g_game) goto check_focus;
            g_game->f1a060 = 1;
            g_game->f1a068 = 0;
            ((tstring*)((char*)g_game + 0x1a06c))->clear();
            break;
        case 0x9c58:
            DialogBoxParamA(m_hInstance, (LPCSTR)0xe0, m_hWnd, (DLGPROC)global_4c03b0, 0);
            // falls through
        }
        // falls through
    case 0x200:                                 // WM_MOUSEMOVE
    check_focus:
        if (g_game && (g_game->f1a090 || g_game->f1a060) && !g_5e2630)
            SetFocus(m_hWnd);
        if (m_bActive && m_pd3dDevice)
            GetCursorPos(&pt);
        break;
    case 0x109:
        if (wparam == 0xffff) return 1;
        m_34c = wparam;
        break;
    case 0x102:                                 // WM_CHAR
        m_34c = wparam;
        break;
    }
    return Application::MsgProc(hWnd, msg, wparam, lparam);
}

// ---- the loading screen's step ----------------------------------------------------------------------
// @ 0x50f390
void LoadingScreen::update(int unused)
{
    char buf[0x100];
    tstring text;
    char tmp[8];
    bool finished = advance(&step, &total, &text);
    const char* fmt = "%d";
    _snprintf(buf, 0x100, fmt, step);
    text_widget->set_text(tmp, text);
    bar_widget->set_value((float)step * 0.14285715f);
    if (finished) {
        g_main_menu.choose(4);
        ((Output*)0x5a9f60)->print("Finished All...\n");
    }
}
