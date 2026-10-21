// Slice s00402be0: D3D application framework (CD3DApplication-like): fullscreen toggle, reset, message loop.
// windows.h from SDK 6.0A does not parse under cl 13.10 (needs sal.h): declare what we use.
typedef struct HWND__* HWND; typedef struct HACCEL__* HACCEL; typedef int BOOL; typedef unsigned int UINT;
typedef unsigned long DWORD; typedef unsigned int WPARAM_T; typedef unsigned int UINT_PTR; typedef long LONG;
struct POINT { LONG x, y; };
struct MSG { HWND hwnd; UINT message; UINT wParam; long lParam; DWORD time; POINT pt; };
#define WINAPI __stdcall
#define DLLI extern "C" __declspec(dllimport)
DLLI HACCEL WINAPI LoadAcceleratorsA(void*, const char*);
DLLI BOOL WINAPI PeekMessageA(MSG*, HWND, UINT, UINT, UINT);
DLLI BOOL WINAPI PeekMessageW(MSG*, HWND, UINT, UINT, UINT);
DLLI BOOL WINAPI GetMessageW(MSG*, HWND, UINT, UINT);
DLLI int WINAPI TranslateAcceleratorA(HWND, HACCEL, MSG*);
DLLI BOOL WINAPI TranslateMessage(const MSG*);
DLLI long WINAPI DispatchMessageW(const MSG*);
DLLI long WINAPI SendMessageA(HWND, UINT, UINT, long);
DLLI void WINAPI Sleep(DWORD);
DLLI BOOL WINAPI DestroyAcceleratorTable(HACCEL);
DLLI BOOL WINAPI SetWindowPos(HWND, HWND, int, int, int, int, UINT);
DLLI int WINAPI MessageBoxA(HWND, const char*, const char*, UINT);
DLLI BOOL WINAPI ShowWindow(HWND, int);
DLLI BOOL WINAPI UpdateWindow(HWND);
#define MAKEINTRESOURCE(i) ((const char*)(unsigned long)(unsigned short)(i))
#define WM_QUIT 0x12
#define WM_CLOSE 0x10
#define PM_REMOVE 1
#include <string.h>

struct ModeSize { int width, height; };
struct ModeSize;
struct SettingsBlock {                  // 0x6c bytes at App+0x38
    bool windowed; char p0[0xb];
    ModeSize* win_mode;                 // +0xc
    char p1[0x44 - 0x10];
    ModeSize* fs_mode;                  // +0x44
    char p2[0x6c - 0x48];
};
struct SettingsDlg {                    // temp object built by 0x4056a0, run by 0x407230 (provisional)
    int unk0, arg;
    SettingsBlock s;
    SettingsDlg(void* a, void* b);                  // 0x4056a0
    int run(HWND h);                                // 0x407230
};

struct App {
    virtual bool v0(int, int);                      // slot 0
    virtual void v1();                              // slot 1 (+4)
    virtual void v2(); virtual void v3();
    virtual int  v4();                              // +0x10
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8();
    virtual void v9(); virtual void v10(); virtual void v11(); virtual void v12();
    virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(int);                          // +0x40
    bool exit_flag;                                 // +4
    char pad0[7];
    int  dlg_arg[11];                               // +0xc
    SettingsBlock settings;                         // +0x38 (settings.d[0] low byte = windowed)
    bool windowed;                                   // +0xa4
    bool active;                                    // +0xa5
    bool sleep_flag;                                // +0xa6
    char pad4[2];
    bool in_reset;                                  // +0xa9
    char pad5[2];
    bool f_ac;                                      // +0xac
    bool f_ad;                                      // +0xad
    char pad6[0xe8 - 0xae];
    HWND hwnd;                                      // +0xe8
    char pad7[0xf4 - 0xec];
    struct IUnk { virtual long __stdcall a(); virtual long __stdcall b(); virtual unsigned long __stdcall rel(); }* obj;   // +0xf4
    char pad8[0x254 - 0xf8];
    int rc_left, rc_top, rc_right, rc_bottom;       // +0x254
    char pad9[0x338 - 0x264];
    const char* caption;                            // +0x338
    char pad10[0x346 - 0x33c];
    bool f346;

    int  reset_a();                                 // 0x4026e0
    int  reset_b();                                 // 0x4027c0
    int  render();                                  // 0x402430
    int  fail(int hr, int flag);                    // 0x4014f0
    bool check(int, int);                           // 0x401a60

    ModeSize* cur_mode() { return *(volatile bool*)&settings.windowed ? settings.win_mode : settings.fs_mode; }
    void reset_c();                                 // 0x402280
    int  reset_d();                                 // 0x401290 (original calls it with ecx left from 0x402280)
    int  handle_size_change();                      // 0x402be0
    int  toggle_fullscreen();                       // 0x402d40
    UINT_PTR run();                                 // 0x402ee0
    int  initialize(int);                           // 0x403010
};


extern bool g_reset_msg_shown;                      // 0x5d6f48
double __stdcall timer_op(int);                     // 0x407660

#define WINDOWED (settings.windowed)
#define CUR_MODE cur_mode()

// @ 0x402be0
int App::handle_size_change()
{
    int old_w = CUR_MODE->width;
    int old_h = CUR_MODE->height;
    v16(1);
    in_reset = true;
    windowed = !windowed;
    WINDOWED = windowed;
    v1();
    int hr;
    if (CUR_MODE->width == old_w && CUR_MODE->height == old_h) {
        reset_c();
        hr = reset_d();
    } else {
        reset_a();
        hr = reset_b();
    }
    if (hr < 0) {
        if (hr != (int)0x8876017c)
            hr = (int)0x8200000c;
        in_reset = false;
        if (!windowed) {
            windowed = true;
            WINDOWED = 1;
            v1();
            SetWindowPos(hwnd, (HWND)-2, rc_left, rc_top, rc_right - rc_left, rc_bottom - rc_top, 0x40);
        }
        return fail(hr, 1);
    }
    in_reset = false;
    if (windowed)
        SetWindowPos(hwnd, (HWND)-2, rc_left, rc_top, rc_right - rc_left, rc_bottom - rc_top, 0x40);
    v16(0);
    return 0;
}

// @ 0x402d40
int App::toggle_fullscreen()
{
    char msg[512];
    if (!windowed && handle_size_change() < 0) {
        if (!g_reset_msg_shown) {
            strcpy(msg, "Could not reset the Direct3D device.");
            g_reset_msg_shown = true;
            strcat(msg, "\n\nThis sample will now exit.");
            MessageBoxA(0, msg, caption, 0x10);
            if (hwnd)
                SendMessageA(hwnd, 0x10, 0, 0);
        }
        return 0x80004005;
    }
    SettingsDlg dlg(dlg_arg, &settings);
    if (dlg.run(hwnd) == 1) {
        settings = dlg.s;
        windowed = WINDOWED;
        reset_a();
        int hr = reset_b();
        if (hr < 0) {
            if (hr != (int)0x8876017c)
                hr = (int)0x8200000c;
            if (!windowed) {
                windowed = true;
                WINDOWED = 1;
                v1();
                SetWindowPos(hwnd, (HWND)-2, rc_left, rc_top, rc_right - rc_left, rc_bottom - rc_top, 0x40);
            }
            return fail(hr, 1);
        }
        if (!f_ac) {
            f_ad = true;
            timer_op(1);
            timer_op(2);
        }
    }
    return 0;
}

// @ 0x402ee0
UINT_PTR App::run()
{
    HACCEL accel = LoadAcceleratorsA(0, MAKEINTRESOURCE(0x71));
    MSG msg;
    msg.message = 0;
    PeekMessageA(&msg, 0, 0, 0, 0);
    while (msg.message != WM_QUIT) {
        bool got;
        if (active)
            got = PeekMessageW(&msg, 0, 0, 0, PM_REMOVE) != 0;
        else
            got = GetMessageW(&msg, 0, 0, 0) != 0;
        if (got) {
            if (!accel || !hwnd || !TranslateAcceleratorA(hwnd, accel, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (exit_flag)
                SendMessageA(hwnd, WM_CLOSE, 0, 0);
        } else {
            if (sleep_flag)
                Sleep(100);
            if (active && render() < 0)
                SendMessageA(hwnd, WM_CLOSE, 0, 0);
        }
    }
    if (accel)
        DestroyAcceleratorTable(accel);
    return msg.wParam;
}

// @ 0x403010
int App::initialize(int)
{
    bool a = v0(0, 0);
    bool b = check(0, 0);
    if (f346 && a)
        WINDOWED = 0;
    if (!b) {
        if (!a)
            goto fail1;
        WINDOWED = b;
    }
    if (!a && !b) {
fail1:
        if (obj) {
            obj->rel();
            obj = 0;
        }
        return fail((int)0x82000003, 1);
    }
    timer_op(1);
    int hr = v4();
    if (hr < 0) {
        if (obj) { obj->rel(); obj = 0; }
        return fail(hr, 1);
    }
    hr = reset_b();
    if (hr < 0) {
        if (obj) { obj->rel(); obj = 0; }
        return fail(hr, 1);
    }
    v16(0);
    ShowWindow(hwnd, 1);
    UpdateWindow(hwnd);
    return 0;
}
