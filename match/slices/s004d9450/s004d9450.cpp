// Slice s004d9450: the font definition object's constructor (0x4d99d0), the out-of-line copy
// assignment of std::vector<E176> (0x4d9840) and the WinMain / editor start-up routine (0x4d9450).
#include <windows.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------- tstring (0x10)
struct tstring {
    int pos;                                // +0x00
    char* buf;                              // +0x04
    int cap;                                // +0x08
    int len;                                // +0x0c
    tstring();
    tstring(const char*);
    tstring(const tstring&);
    ~tstring();
    tstring& operator=(const tstring&);
    void assign(const char*);
};

// ---------------------------------------------------------------- elements used by the font code
struct b16 { int v[4]; };                    // 0x10 trivial block

struct E176 {                               // 0xb0 (see s004d9aa0 / s004cd010)
    tstring s;                              // +0x00
    int a;                                  // +0x10
    int b;                                  // +0x14
    int c;                                  // +0x18
    std::vector<int> v;                     // +0x1c
    b16 big[8];                             // +0x2c
    int t;                                  // +0xac
};

template class std::vector<E176>;
template class std::vector<int>;

// ---------------------------------------------------------------- the font object
struct font_details {                       // 0x1c, seven floats
    float a, b, c, d, e, f, g;
    font_details() { a = b = c = d = e = f = g = 0.0f; }   // 0x4bd120 (zero-fill)
};

struct point2i {
    int x, y;
    point2i(int a_, int b_) : x(a_), y(b_) {}
};

struct app_list_node {
    app_list_node* next;
    app_list_node* prev;
    int            value;
};
struct app_list {
    app_list_node* head;                    // +0x00
    app_list_node* w04;                     // +0x04
    int            count;                   // +0x08
    app_list();
};

// 0x4c4dc0: std::list<int>::_Buynode
void* __fastcall list_int_buynode(app_list* self);

app_list::app_list()
{
    w04 = (app_list_node*)list_int_buynode(this);
    count = 0;
}

struct Font {
    int p00, p04;                           // +0x00
    int width;                              // +0x08
    int height;                             // +0x0c
    int padding;                            // +0x10
    int font_size;                          // +0x14
    float x_ratio;                          // +0x18
    float y_ratio;                          // +0x1c
    float font_scale;                       // +0x20
    float line_spacing;                     // +0x24
    point2i p28;                            // +0x28
    point2i p30;                            // +0x30
    int i38;                                // +0x38
    int i3c;                                // +0x3c
    int i40;                                // +0x40
    char b44;                               // +0x44
    app_list list;                          // +0x48
    std::vector<int> ids;                   // +0x54
    int i64;                                // +0x64
    int i68;                                // +0x68
    font_details glyphs[0x200];             // +0x6c
    Font();
};

// @ 0x4d99d0
Font::Font()
    : p28(0, 0), p30(0, 0), i38(0), i3c(0), i40(0), list(), ids(9, (int)(size_t)list.w04)
{
    char uninit;
    b44 = uninit;
    i64 = 1;
    i68 = 1;
    for (int i = 0; i < 0x200; i++)
        glyphs[i] = font_details();
}

// ---------------------------------------------------------------- declarations from other slices
extern "C" int __stdcall FUN_004d9440(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// skinning engine (SkinMagic) entries, reached through two import ordinals
extern "C" void __stdcall SkinMagic_Init(HINSTANCE, const char*, int, int);   // 0x535452 (ordinal 1)
extern "C" void __stdcall SkinMagic_Load(const char*);                        // 0x53544c (ordinal 3)

struct LangTables { void load(tstring* name); };                              // 0x4cda90
struct Config { void load_file(FILE* f); };                                  // 0x4bbf70
struct MapEditorDialog {
    int  init();                                                             // 0x4bad70
    void init_config(void* cfg, int a);                                      // 0x4b9b90
};
struct Application {
    int Create(HINSTANCE);                                                   // 0x401070
    int Create3DEnvironment(HINSTANCE);                                      // 0x403010
    int Run();                                                               // 0x402ee0
};

extern LangTables       g_strtable;          // 0x5e2a18
extern Config           g_config;            // 0x5a9f98
extern MapEditorDialog  g_editor_dialog;     // 0x8ee3c48
extern Application      g_application;       // 0x8ee3c48 (same object)
extern tstring          g_skin;              // 0x5fe888
extern HINSTANCE        g_hinstance;         // 0x8ee3fa0
extern int              g_init_flag;         // 0x8ee3f9c
extern HWND             g_editor_hwnd;       // 0x8ee3d30
extern unsigned char    g_mt_inited;         // 0x5e2a09
extern void*            g_mt_event;          // 0x5e28d8
extern CRITICAL_SECTION g_mt_cs;             // 0x5e28dc
extern unsigned char    g_fullscreen_flag;   // 0x8ee3f8e
extern float            g_res_scale;         // 0x8ee3c50
extern int              g_video_played;      // 0x5e2a10
// config fields (the object at 0x5a9f98)
extern int              g_cfg_9fa4;          // 0x5a9fa4
extern int              g_cfg_9fa8;          // 0x5a9fa8
extern int              g_cfg_9fb4;          // 0x5a9fb4

FILE* __cdecl mb_open_user_file(const char*, const char*);                   // 0x415160
int   __cdecl fclose_537168(FILE*);                                         // 0x537168
void  __fastcall init_5aa008(void* self);                                    // 0x4c3a40
void  __cdecl FUN_004c8900(void* arg);                                       // 0x4c8900
float __cdecl timer_op(int op);                                             // 0x407660
extern "C" unsigned long __cdecl _beginthread(void (__cdecl*)(void*), unsigned, void*);  // 0x538f64

// @ 0x4d9450
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    (void)hPrevInstance;

    // skip the command line
    for (char* p = lpCmdLine; *p > 0; p++)
        ;

    g_hinstance = hInstance;
    g_init_flag = 0;
    g_skin.assign("en");

    tstring empty("");
    g_strtable.load(&empty);
    empty.~tstring();

    LoadLibraryA("riched20.dll");
    SkinMagic_Init(hInstance, "SCRIBBLE", 0, 0);

    char skin_path[0x104];
    sprintf(skin_path, ".\\Skins\\mb_skin_%s.smf",
            g_skin.buf ? g_skin.buf : (char*)&g_skin.buf);

    WIN32_FIND_DATAA fd;
    HANDLE hfind = FindFirstFileA(skin_path, &fd);
    const char* chosen;
    if (hfind == INVALID_HANDLE_VALUE) {
        chosen = ".\\Skins\\mb_skin_int.smf";
    } else {
        FindClose(hfind);
        chosen = skin_path;
    }
    SkinMagic_Load(chosen);

    int r = g_editor_dialog.init();
    if (r < 0)
        return 0;

    INT_PTR dlg = DialogBoxParamA(hInstance, MAKEINTRESOURCEA(0xaa), NULL, FUN_004d9440, 0);
    if (dlg != 0x410)
        return (int)dlg;

    FILE* f = mb_open_user_file("rgl_config.txt", "r");
    if (f != NULL) {
        g_config.load_file(f);
        fclose_537168(f);
    }
    init_5aa008((void*)0x5aa008);
    g_editor_dialog.init_config((void*)0x5a9f98, 0);

    DWORD_PTR proc_mask = 0, sys_mask = 0;
    GetProcessAffinityMask(GetCurrentProcess(), &proc_mask, &sys_mask);
    (void)proc_mask;
    if (g_cfg_9fb4 == 0 && sys_mask > 1) {
        g_mt_inited = 1;
        InitializeCriticalSection(&g_mt_cs);
        g_mt_event = CreateEventA(NULL, FALSE, FALSE, NULL);
        _beginthread(FUN_004c8900, 0x40000, NULL);
    }

    g_fullscreen_flag = (g_cfg_9fa8 == 0);
    g_res_scale = 1.0f / (float)g_cfg_9fa4;

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    float t0 = timer_op(5);

    hfind = FindFirstFileA("paradox.bik", &fd);
    if (hfind != INVALID_HANDLE_VALUE) {
        FindClose(hfind);
        BOOL ok = CreateProcessA("binkplay.exe",
                                 " paradox.bik /P /I2 /Z /J /U1 /W-1 /H-1 /C /B2",
                                 NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
        if (ok) {
            g_video_played = 1;
            WaitForSingleObject(pi.hProcess, 0x57e4);
            CloseHandle(pi.hProcess);
        }
    }

    if (g_video_played == 1 && timer_op(5) - t0 > 15.0f) {
        STARTUPINFOA si2;
        PROCESS_INFORMATION pi2;
        memset(&si2, 0, sizeof(si2));
        si2.cb = sizeof(si2);
        memset(&pi2, 0, sizeof(pi2));

        hfind = FindFirstFileA("taleworlds_intro.bik", &fd);
        if (hfind != INVALID_HANDLE_VALUE) {
            FindClose(hfind);
            BOOL ok = CreateProcessA("binkplay.exe",
                                     " taleworlds_intro.bik /P /I2 /Z /J /U1 /W-1 /H-1 /C /B2",
                                     NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2);
            if (ok) {
                g_video_played = 2;
                WaitForSingleObject(pi2.hProcess, 0x1c84);
                CloseHandle(pi2.hProcess);
            }
        }
    }

    r = g_application.Create(hInstance);
    if (r < 0)
        return 0;

    SetActiveWindow(g_editor_hwnd);
    SetForegroundWindow(g_editor_hwnd);
    UpdateWindow(g_editor_hwnd);

    r = g_application.Create3DEnvironment(hInstance);
    if (r < 0)
        return 0;

    return g_application.Run();
}
