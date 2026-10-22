// Slice s004beb80: path/registry string helpers (0x4beb80 registry write, 0x4bebc0 module.ini path
// test, 0x4bec60 module folder scan) plus the game application's destructor (0x4bed90..0x4bee10).
// ALL NAMES PROVISIONAL. Flags: default /O2 /MT /Gy /TP.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <new>

// tstring: the engine's own 16-byte string with a read cursor (layout verified by s00415c80/s00418150).
// Plain aggregate with the real out-of-line members declared on it, exactly as called here.
struct tstring {
    int pos;        // +0  read cursor
    char* buf;      // +4  (null while empty)
    int cap;        // +8
    int len;        // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    tstring& operator=(const tstring& o);       // 0x42bc50
    void assign(const char* s);                 // 0x42bcb0
    void push_back(char c);                     // 0x42be70
    void append(const tstring& o);              // 0x42c1e0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---- globals / imports -------------------------------------------------------------------------------
extern "C" __declspec(dllimport) LONG __stdcall RegSetValueExA(HKEY, LPCSTR, DWORD, DWORD,
                                                               const BYTE*, DWORD);  // 0x549008
extern "C" __declspec(dllimport) HANDLE __stdcall FindFirstFileA(LPCSTR, LPWIN32_FIND_DATAA); // 0x5491a4
extern "C" __declspec(dllimport) BOOL   __stdcall FindClose(HANDLE);                         // 0x5491a0

extern const char s_modules[];      // 0x591540  "Modules\"
extern const char s_ini[];          // 0x5948c8  "\\module.ini"
extern const char s_glob[];         // 0x5948d4  "Modules\\*"

// The CRT's file-find data record (0x118 bytes) and primitives reached through the static CRT.
struct _finddata_t {
    unsigned attrib;        // +0
    long     time_create;   // +4
    long     time_access;   // +8
    long     time_write;    // +0xc
    long     size;          // +0x10
    char     name[260];     // +0x14
};
extern "C" int __cdecl _findfirst(const char* spec, _finddata_t* data);   // 0x538c59
extern "C" int __cdecl _findnext(int handle, _finddata_t* data);          // 0x538d36
extern "C" int __cdecl _findclose(int handle);                            // 0x538bd5

// ---- callees -----------------------------------------------------------------------------------------
void* __cdecl operator new(unsigned size);          // 0x53545e
void  __cdecl operator delete(void* p);             // 0x535712
struct CD3DFont {
    ~CD3DFont();                                    // 0x4049d0
};
struct CD3DEnumeration {
    ~CD3DEnumeration();                             // 0x404480
};

// ---- the Direct3D application base (layout from s00401910.h/ s004d3ec0) --------------------------------
struct Application {
    char pad[0x350];
    Application();                                  // 0x401910
};

// The game application: vptr 0x5948e0, font pointer at +0x348, 0x20 module-name tstrings at +0x390.
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
    int    f38c;                                    // +0x38c  module count
    char   arr[0x200];                              // +0x390: 0x20 tstrings
    GameApp();
    ~GameApp();
    int  msg_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);   // 0x4d4140
    void* deleting_dtor(unsigned flags);                     // 0x4bedf0
    tstring* mod(int i) { return ((tstring*)arr) + i; }
};

// The engine's options dialog panel: hwnd at +0x380 (see s004bf170).
struct EditorPanel {
    char pad0[0x380];
    HWND hwnd;              // +0x380  the options dialog
};

// ---- engine globals of the options panel (see s004bf170) -----------------------------------------------
extern int g_opt_5aa0ac;    // 0x5aa0ac
extern int g_opt_5aa0b0;    // 0x5aa0b0
extern int g_opt_5aa0b4;    // 0x5aa0b4
extern int g_opt_5aa0b8;    // 0x5aa0b8
extern int g_opt_5aa0c0;    // 0x5aa0c0
extern int g_opt_5aa0c4;    // 0x5aa0c4
extern int g_opt_5aa0cc;    // 0x5aa0cc
extern int g_opt_5aa0d0;    // 0x5aa0d0
extern int g_opt_5aa0d8;    // 0x5aa0d8
extern int g_opt_5aa0dc;    // 0x5aa0dc
extern int g_opt_5aa0e0;    // 0x5aa0e0
extern int g_opt_5aa0e4;    // 0x5aa0e4
extern int g_opt_5aa0e8;    // 0x5aa0e8
extern int g_opt_5aa0f4;    // 0x5aa0f4  resolution width
extern int g_opt_5aa0f8;    // 0x5aa0f8  resolution height
extern int g_opt_5aa0fc;    // 0x5aa0fc  colour depth (0x10 / 0x20)
extern int g_opt_5aa108;    // 0x5aa108
extern int g_opt_5aa110;    // 0x5aa110

// Control ids of the options dialog (see s004bf170).
enum {
    IDC_OPT_1 = 0x43e, IDC_OPT_441 = 0x441, IDC_OPT_442 = 0x442, IDC_OPT_443 = 0x443,
    IDC_OPT_444 = 0x444, IDC_OPT_445 = 0x445, IDC_OPT_446 = 0x446, IDC_OPT_448 = 0x448,
    IDC_OPT_44a = 0x44a, IDC_OPT_44c = 0x44c, IDC_OPT_44d = 0x44d, IDC_OPT_452 = 0x452,
    IDC_OPT_45f = 0x45f, IDC_OPT_467 = 0x467, IDC_OPT_468 = 0x468, IDC_OPT_487 = 0x487,
};
enum {
    MSG_USER_SEL = 0x405,   // CB_GETCURSEL-style read used for the combo at 0x43e
};
extern "C" __declspec(dllimport) HWND    __stdcall GetDlgItem(HWND, int);              // 0x54932c
extern "C" __declspec(dllimport) LRESULT __stdcall SendMessageA(HWND, UINT, WPARAM, LPARAM); // 0x549378
extern "C" __declspec(dllimport) BOOL    __stdcall SetWindowTextA(HWND, LPCSTR);       // 0x5492e4
extern "C" __declspec(dllimport) BOOL    __stdcall SetDlgItemInt(HWND, int, UINT, BOOL); // 0x5492e0
int __cdecl crt_sprintf(char* buf, const char* fmt, ...);                             // 0x536f8f
extern "C" char* __cdecl _itoa(int val, char* buf, int radix);                         // 0x5371f7

extern const char s_fmt_pct_d[];        // 0x54a0f4  "%d"
extern const char s_bits16[];           // 0x594934  " 16 bits"
extern const char s_bits32[];           // 0x594928  " 32 bits"

// =============================================================================================
// Registry / path helpers
// =============================================================================================

// @ 0x4beb80
void keys_write_value(HKEY* key, LPCSTR name, const tstring* v)
{
    if (*key != (HKEY)0) {
        int len = v->len;
        const char* data = v->buf ? v->buf : (const char*)v + 4;
        RegSetValueExA(*key, name, 0, 1 /*REG_SZ*/, (const BYTE*)data, len + 1);
    }
}

// @ 0x4bebc0
bool module_ini_exists(const char* module)
{
    bool found = false;
    tstring path(s_modules);
    {
        tstring name(module);
        path.append(name);
    }
    {
        tstring ini(s_ini);
        path.append(ini);
    }
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path.buf ? path.buf : (const char*)&path.buf, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        FindClose(h);
        found = true;
    }
    return found;
}

// @ 0x4bec60
void scan_module_folders(tstring* out, int* count)
{
    *count = 0;
    _finddata_t findbuf;
    tstring names[0x20];

    int h = _findfirst(s_glob, &findbuf);
    if ((findbuf.attrib & 0x10) != 0 && module_ini_exists(findbuf.name)) {
        names[*count].assign(findbuf.name);
        (*count)++;
    }
    if (h != -1) {
        while (_findnext(h, &findbuf) == 0) {
            if (*count >= 0x20)
                break;
            if (module_ini_exists(findbuf.name)) {
                names[*count].assign(findbuf.name);
                (*count)++;
            }
        }
    }
    _findclose(h);

    for (int i = 0; i < *count; i++)
        out[i] = names[(*count - i) - 1];
}

// =============================================================================================
// GameApp destructor
// =============================================================================================

// @ 0x4bed90
GameApp::~GameApp()
{
    CD3DFont* font = *(CD3DFont**)((char*)this + 0x348);
    *(void**)this = (void*)0x5948e0;
    if (font != 0) {
        font->~CD3DFont();
        operator delete(font);
        *(CD3DFont**)((char*)this + 0x348) = 0;
    }
    {
        tstring* p = (tstring*)((char*)this + 0x590);
        int n = 0x20;
        do {
            p--;
            p->~tstring();
        } while (--n);
    }
    *(void**)this = (void*)0x549c60;
    ((CD3DEnumeration*)((char*)this + 0xc))->~CD3DEnumeration();
}

// @ 0x4bedf0
void* GameApp::deleting_dtor(unsigned flags)
{
    this->~GameApp();
    if ((flags & 1) != 0)
        ::operator delete(this);
    return this;
}

// =============================================================================================
// Options dialog refresh
// =============================================================================================

// @ 0x4bee10
void __fastcall gfx_query(EditorPanel* self)
{
    char buf[0x200];

    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_1), MSG_USER_SEL, 1, g_opt_5aa0ac);
    crt_sprintf(buf, s_fmt_pct_d, g_opt_5aa0ac);
    SetWindowTextA(GetDlgItem(self->hwnd, 0x450), buf);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_441), BM_SETCHECK, g_opt_5aa0e0 != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_444), BM_SETCHECK, g_opt_5aa0b8 != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_445), BM_SETCHECK, g_opt_5aa0b0 != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_487), BM_SETCHECK, g_opt_5aa108 != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_452), BM_SETCHECK, g_opt_5aa0e8 != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_446), BM_SETCHECK, g_opt_5aa0e4 != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_44d), BM_SETCHECK, g_opt_5aa0d8 == 0, 0);
    SetDlgItemInt(self->hwnd, IDC_OPT_448, g_opt_5aa0b4, FALSE);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_442), BM_SETCHECK, g_opt_5aa0cc != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_443), BM_SETCHECK, g_opt_5aa0d0 == 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_44c), BM_SETCHECK, g_opt_5aa0c0 != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_44a), BM_SETCHECK, g_opt_5aa0c4 != 0, 0);
    SendMessageA(GetDlgItem(self->hwnd, 0x467), 0x14e, g_opt_5aa110, 0);
    SendMessageA(GetDlgItem(self->hwnd, 0x468), 0x14e, g_opt_5aa0dc, 0);
    if (g_opt_5aa0f4 == 0) {
        SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_45f), 0x14e, 0, 0);
    } else {
        char num[0x80];
        tstring s(_itoa(g_opt_5aa0f4, num, 10));
        s.push_back('x');
        {
            tstring h(_itoa(g_opt_5aa0f8, num, 10));
            s.append(h);
        }
        {
            tstring b(g_opt_5aa0fc == 0x10 ? s_bits16 : s_bits32);
            s.append(b);
        }
        SendMessageA(GetDlgItem(self->hwnd, IDC_OPT_45f), 0x14d, 0,
                     (LPARAM)(s.buf ? s.buf : (const char*)&s.buf));
    }
}
