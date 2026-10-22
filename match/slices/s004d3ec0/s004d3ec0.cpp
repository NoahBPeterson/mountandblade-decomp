// Slice s004d3ec0: the game application's constructor (0x4d3ec0), a window-proc thunk to the
// application's WndProc (0x4d4130) and the settings panel's window procedure (0x4d4140).
#include <windows.h>
#include <shellapi.h>
#include <new>

// ---- engine string (16 bytes) ---------------------------------------------------------------------------
struct tstring {
    int  pos;                                       // +0
    char* buf;                                      // +4
    int  cap;                                       // +8
    int  len;                                       // +0xc
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    tstring(const tstring& o);                      // 0x42c110
    ~tstring();                                     // 0x42bc40
    tstring& operator=(const tstring& o);           // 0x42bc50
    void assign(const char* s);                     // 0x42bcb0
    void append(const tstring& o);                  // 0x42c1e0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};
void __fastcall tstring_ctor(tstring* p);           // 0x42bbd0

// ---- callees -------------------------------------------------------------------------------------------
void* __cdecl operator new(unsigned size);          // 0x53545e
struct CD3DFont {
    char pad[0x67c];
    CD3DFont(const char* name, DWORD height, DWORD flags);    // 0x404980
};
struct Big {
    char pad[0x6868];
    Big();                                          // 0x4d3880
};
void  sub_4bec60(void* array, int* count);          // 0x4bec60
void  keys_read(HKEY* key, const char* name, tstring* out);   // 0x4b9720
extern "C" {
void __stdcall FSOUND_SetMaxHardwareChannels(int n);          // 0x534300
int  __stdcall FSOUND_Init(int mixrate, int maxsoftwarechannels, int flags);  // 0x5342fa
void __stdcall FSOUND_SetSFXMasterVolume(int volume);         // 0x5342f4
void __stdcall FSOUND_3D_SetRolloffFactor(float factor);      // 0x5342ee
}

// ---- globals -------------------------------------------------------------------------------------------
extern int g_5e28fc;                                // 0x5e28fc
extern int g_5e28c8;                                // 0x5e28c8
extern int g_5e28cc;                                // 0x5e28cc
extern char g_5e2a08;                               // 0x5e2a08
extern void* g_5e2930;                              // 0x5e2930
extern Big* g_5e292c;                               // 0x5e292c
extern int g_5a9fd8;                                // 0x5a9fd8
extern int g_5e27c8[0x40];                          // 0x5e27c8

struct LogStream {
    virtual void v0(); virtual void v1(); virtual void v2(); virtual void v3(); virtual void v4();
    virtual void v5(); virtual void v6(); virtual void v7(); virtual void v8(); virtual void v9();
    virtual void w_cstr(const char* msg);           // slot 10 (+0x28)
};

// ---- the Direct3D application base class (layout from s00401910) --------------------------------------
struct Application {
    char pad[0x350];
    Application();                                  // 0x401910
};

// +0x350..+0x590: the game application's own fields, with the module-string array at +0x390
struct GameApp : Application {
    char   f350;                                    // +0x350
    char   pad0[0x354 - 0x351];
    HBITMAP hbmp;                                   // +0x354
    char   pad1a[0x364 - 0x358];
    HWND   hwnd;                                    // +0x364
    char   pad1[0x374 - 0x368];
    int    f374;                                    // +0x374
    int    f378;                                    // +0x378
    int    f37c;                                    // +0x37c
    char   pad2[0x38c - 0x380];
    int    f38c;                                    // +0x38c
    char   arr[0x200];                              // +0x390: 0x20 tstrings
    GameApp();
    int  msg_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);   // 0x4d4140
    tstring* mod(int i) { return ((tstring*)arr) + i; }
};

// @ 0x4d3ec0
GameApp::GameApp()
{
    tstring* arrp = (tstring*)arr;
    *(void**)this = (void*)0x5948e0;

    tstring* p = arrp;
    int n = 0x20;
    do {
        tstring_ctor(p);
        p++;
    } while (--n);

    f350 = 0;

    *(int*)((char*)this + 0x334) = 0x5a9c3c;
    *(int*)((char*)this + 0x338) = 0x5a9c54;
    *(char*)((char*)this + 0x30) = 1;
    *(int*)((char*)this + 0x33c) = 0x190;
    *(int*)((char*)this + 0x340) = 0x12c;
    *(char*)((char*)this + 0x344) = 1;

    CD3DFont* font = new CD3DFont("Arial", 9, 1);
    *(CD3DFont**)((char*)this + 0x348) = font;
    *(int*)((char*)this + 0x34c) = -1;

    for (int i = 0; i < 0x40; i++)
        g_5e27c8[i] = 0;

    g_5e28cc = 0;
    g_5e28c8 = 0;
    sub_4bec60(arrp, &f38c);
    if (f38c > 0) {
        *(tstring*)0x8ee3c38 = *arrp;

        HKEY  key;
        DWORD disp;
        RegCreateKeyExA((HKEY)0x80000001, "Software\\MountAndBladeKeys", 0, 0, 0, 0x20007, 0,
                        &key, &disp);
        if (disp == 1)
            ((LogStream*)0x5a9f60)->w_cstr("Creating new Registry Key");
        if (key != 0) {
            tstring modname;
            keys_read(&key, "last_module", &modname);
            if (key != 0)
                RegCloseKey(key);
            if (modname.len > 0)
                *(tstring*)0x8ee3c38 = modname;
        }
    }

    ((tstring*)0x8ee3c28)->assign("Modules\\");
    ((tstring*)0x8ee3c28)->append(*(tstring*)0x8ee3c38);
    {
        tstring tmp("\\");
        ((tstring*)0x8ee3c28)->append(tmp);
    }

    g_5e28fc = 0;
    g_5e2a08 = 0;
    g_5e2930 = 0;
    g_5e292c = new Big;

    if (g_5a9fd8 == 0) {
        FSOUND_SetMaxHardwareChannels(0);
        FSOUND_Init(0xac44, 0x40, 0);
        FSOUND_SetSFXMasterVolume(0xff);
        FSOUND_3D_SetRolloffFactor(0.5f);
    }

    *(tstring*)0xa75e8c = *(tstring*)0x8ee3c28;
    *(int*)((char*)this + 0x364) = 0;
    f374 = 0;
    f378 = 0;
    f37c = 0;
}

// @ 0x4d4130
struct AppWnd {
    int WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp);      // 0x4d0240
};
int __stdcall wndproc_thunk(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    return ((AppWnd*)0x8ee3c48)->WndProc(h, msg, wp, lp);
}

// ---- callees used by the main window procedure (0x4d4140) ----------------------------------------------
void __fastcall app_frame(void* self);                        // 0x4b9af0
void __fastcall app_init(void* self);                         // 0x4c7290
int  __cdecl    app_check_version();                          // 0x4b9aa0
void __fastcall unescape_string(tstring* s);                  // 0x4bac70
void __cdecl    keys_open(HKEY* out);                         // 0x4b96d0
void __cdecl    keys_write(HKEY* key, const char* name, tstring* val);   // 0x4beb80
void* __cdecl   crt_fopen(const char* name, const char* mode);           // 0x536f7c
int  __cdecl    crt_fclose(void* f);                          // 0x537168
int  __cdecl    crt_sprintf(char* buf, const char* fmt, ...); // 0x536f8f
void __fastcall module_info(void* self, void* f);             // 0x4bd630
tstring* __fastcall strtab_lookup(void* table, const tstring& key);      // 0x43a1e0
void __cdecl    set_window_name(HWND h, const char* s);       // 0x535446

extern tstring g_module;                                      // 0x8ee3c38
extern tstring g_path;                                        // 0x8ee3c28
#define STRTAB ((void*)0x5e2a18)
#define UI(s) strtab_lookup(STRTAB, tstring(s))

// @ 0x4d4140
int GameApp::msg_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    tstring path;

    switch (msg) {
    case 0xf: {                                             // WM_PAINT
        app_frame(this);
        if (*(int*)0x5e29c4 == 0 && f350 == 0) {
            f350 = 1;
            *(int*)0x5e29c4 = 1;
        }
        return 1;
    }
    case 0x110: {                                           // WM_INITDIALOG
        hwnd = h;
        set_window_name(h, "MainFrame");
        app_init(this);
        if (f38c == 0) {
            ShowWindow(GetDlgItem(hwnd, 0x410), 0);
            ShowWindow(GetDlgItem(hwnd, 0x425), 0);
            MessageBoxA(0, UI("ui_there_are_no_modules_installed")->c_str(), "ERROR", 0x10);
            break;
        }
        HWND list = GetDlgItem(hwnd, 0x420);
        if (list == 0)
            break;
        SendMessageA(list, 0x14b, 0, 0);
        for (int i = 0; i < f38c; i++) {
            int r = SendMessageA(list, 0x14a, 0, (LPARAM)mod(i)->c_str());
            SendMessageA(list, 0x151, r, i);
        }
        int r = SendMessageA(list, 0x158, (WPARAM)-1, (LPARAM)g_module.c_str());
        SendMessageA(list, 0x14e, r, 0);
        PostMessageA(hwnd, 0x111, 0x10000, (LPARAM)list);
        ShowWindow(GetDlgItem(hwnd, 0x420), 5);
        ShowWindow(GetDlgItem(hwnd, 0x425), 5);
        break;
    }
    case 0x111: {                                           // WM_COMMAND
        switch (LOWORD(wp)) {
        case 2:
            EndDialog(h, 2);
            break;
        case 0x405:
            DialogBoxParamA(0, (LPCSTR)0xc1, hwnd, (DLGPROC)0x4d4130, 0);
            break;
        case 0x406:
            ShellExecuteA(0, "open", "http://www.gamersgate.com", 0, 0, 5);
            break;
        case 0x410: {
            *(char*)0x5e2630 = 0;
            int sel = 0;
            if (f38c > 1) {
                HWND c = GetDlgItem(h, 0x420);
                int idx = SendMessageA(c, 0x147, 0, 0);
                sel = SendMessageA(c, 0x150, idx, 0);
            }
            g_module = *mod(sel);
            g_path.assign("Modules\\");
            g_path.append(g_module);
            g_path.append(tstring("\\"));
            HKEY key;
            keys_open(&key);
            keys_write(&key, "last_module", &g_module);
            if (key != 0)
                RegCloseKey(key);
            EndDialog(h, 0x410);
            break;
        }
        case 0x41f: {                                       // check for a newer version
            int v = app_check_version();
            if (v > 0x3f3) {
                tstring text(*UI("ui_notification_new_version"));
                unescape_string(&text);
                char buf[0x100];
                crt_sprintf(buf, text.c_str(), 1, 0, 1, 1,
                            v / 1000, (v / 100) % 10, (v / 10) % 10, v % 10);
                MessageBoxA(hwnd, buf, UI("ui_new_version_found")->c_str(), 0);
            }
            else if (v == -1) {
                tstring a("ui_new_version");
                tstring b("ui_notification_unable_to_retreive_new_version");
                const char* cap = strtab_lookup(STRTAB, a)->c_str();
                const char* txt = strtab_lookup(STRTAB, b)->c_str();
                MessageBoxA(hwnd, txt, cap, 0);
            }
            else {
                tstring a("ui_mount_and_blade_is_up_to_date");
                tstring b("ui_notification_latest_version");
                const char* cap = strtab_lookup(STRTAB, a)->c_str();
                const char* txt = strtab_lookup(STRTAB, b)->c_str();
                MessageBoxA(hwnd, txt, cap, 0);
            }
            break;
        }
        case 0x469:
            EndDialog(h, 2);
            break;
        case 0x46b:
            ShellExecuteA(0, "open", "http://www.paradoxplaza.com/Mount&Blade/", 0, 0, 5);
            break;
        case 0x420: {                                       // module selection changed
            if (HIWORD(wp) != 9)
                break;
            HWND c = GetDlgItem(h, 0x420);
            int idx = SendMessageA(c, 0x147, 0, 0);
            int sel = SendMessageA(c, 0x150, idx, 0);
            if (f38c <= sel)
                sel = f38c - 1;
            g_module = *mod(sel);

            path.assign("Modules\\");
            path.append(g_module);
            path.append(tstring("\\main.bmp"));

            DeleteObject(hbmp);
            hbmp = (HBITMAP)LoadImageA(0, path.c_str(), 0, 0, 0, 0x10);

            tstring ini("Modules\\");
            ini.append(g_module);
            ini.append(tstring("\\module.ini"));

            void* f = crt_fopen(ini.c_str(), "r");
            if (f != 0) {
                module_info((void*)0xa18238, f);
                crt_fclose(f);
            }

            tstring v1;
            tstring v2;
            if (*(int*)0xa181c8 > 0x3f3) {
                MessageBoxA(hwnd, UI("ui_module_newer_than_application")->c_str(), 0, 0x30);
            }
            else if (*(int*)0xa181cc < 0x3f3) {
                MessageBoxA(hwnd, UI("ui_module_older_than_application")->c_str(), 0, 0x40);
            }

            if (hbmp == 0)
                hbmp = (HBITMAP)LoadImageA(0, "main.bmp", 0, 0, 0, 0x10);
            RedrawWindow(hwnd, 0, 0, 0x105);
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        return 0;
    }
    return 1;
}
