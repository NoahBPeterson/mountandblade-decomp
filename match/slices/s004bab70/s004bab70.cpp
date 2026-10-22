// Slice s004bab70: user data folders, the "\n" escape decoder, the 3D view rect cache, Direct3D start-up and
// the choice of the start-up display mode.
//   0x4bab70 user_path, 0x4babf0 screenshots_path (SHGetFolderPathA + CreateDirectoryA),
//   0x4bac70 unescape, 0x4bace0 update_view_rect, 0x4bad70 init_d3d, 0x4badf0 choose_display_mode,
//   0x4bb260 set_list_scroll, 0x4bb310 pump_messages, 0x4bb390 set_debug_label, 0x4bb3b0 set_progress.
// Class and field names are provisional; the layouts below come from this range's own accesses.
#include <windows.h>
#include <d3d9.h>
#include <stdio.h>

// ---- globals --------------------------------------------------------------------------------------------------
extern HWND  g_hWnd;                                        // 0x8ee3fc0 (the options dialog)
extern HINSTANCE g_hInstance;                               // 0x8ee3fbc
extern int g_perf_freq_lo;                                  // 0x5a9fe4 (set once the config has been loaded)
extern int g_cfg_display_width;                             // 0x5a9fe8
extern int g_cfg_display_height;                            // 0x5a9fec
extern int g_cfg_display_bits;                              // 0x5a9ff0
extern int g_cfg_antialiasing;                              // 0x5a9ff4
extern int g_cfg_first_time;                                // 0x5a9ff8
extern int g_screenshot_cropped;                            // 0x8ee3cec
extern int g_view_x;                                        // 0x5d75d4
extern int g_view_y;                                        // 0x5d75d0
extern int g_view_w;                                        // 0x5d75c8
extern int g_view_h;                                        // 0x5d75cc
extern struct GameData* g_game;                             // 0x5e2930

// 0x5492c0 shfolder!SHGetFolderPathA      0x5491a8 kernel32!CreateDirectoryA
// 0x5493dc user32!GetWindowRect           0x54932c user32!GetDlgItem
// 0x549378 user32!SendMessageA            0x549350 user32!LoadAcceleratorsA
// 0x549354 user32!PeekMessageA            0x549364 user32!TranslateMessage
// 0x5492dc user32!DispatchMessageA        0x54936c user32!DestroyAcceleratorTable
// 0x549328 user32!SetDlgItemTextA

int sprintf(char* buf, const char* fmt, ...);               // 0x536f8f (_sprintf)

extern "C" __declspec(dllimport) long __stdcall SHGetFolderPathA(HWND hwnd, int csidl, HANDLE token, DWORD flags, char* path);

int shader_rank(int code)                                   // inline helper (0x4badf0 sorts textures by it)
{
    switch (code) {
    case 0x47: return 10;
    case 0x4d: return 9;
    case 0x4b: return 8;
    case 0x4f: return 7;
    case 0x50: return 6;
    case 0x46: return 5;
    case 0x49: return 4;
    }
    return 0;
}

extern "C" IDirect3D9* __stdcall Direct3DCreate9(UINT);      // 0x534342 (d3d9!Direct3DCreate9)
void DisplayErrorMsg(int result, bool fatal);               // 0x4014f0 Application::DisplayErrorMsg

// The Direct3D setup object (the "CD3DEnumeration" of the SDK framework, at +0xc of the core object).
int confirm_device_change(void* params);                    // 0x401010

// The engine's custom array: element at() (0x407ba0) is either the inline storage or an indirect table.
struct carray {
    int dynamic;                // +0
    void* data;                 // +4
    int stride;                 // +8
    int count;                  // +0xc
    void* at(int i);                                        // 0x407ba0
};

struct tstring {
    int pos;                // +0
    char* buf;              // +4
    int cap;                // +8
    int len;                // +0xc
};

struct GameData {                                           // object at 0x5e2930 (provisional)
    char pad0[0x1a0b8];
    int  list_top;              // +0x1a0b8
    int  list_sel;              // +0x1a0bc
};

struct MeshTexture {
    int  f0;                    // +0x00
    int  f4;                    // +0x04
    int  f8;                    // +0x08
    int  fc;                    // +0x0c
    char f10;                   // +0x10
    char pad11[0x18 - 0x11];
    carray sub18;               // +0x18
    char pad1c[0x24 - 0x1c];
    carray sub24;               // +0x24
};

struct MeshObject {
    int  kind;                  // +0x04 (1 / 2: the kinds choose_display_mode filters on)
    char pad8[0x14 - 0x8];
    carray types;               // +0x14 (int codes, used for the shader quality rank)
    char pad18[0x138 - 0x18];
    carray textures;            // +0x138
};

struct Module {
    char  pad0[0x450];
    carray objects;             // +0x450 (MeshTexture entries)
    carray object_types;        // +0x454 (MeshObject entries)
};

struct CD3DEnumeration {
    IDirect3D9* pD3D;                                       // +0x0
    int Enumerate();                                        // 0x404140
};

struct Core {
    char     pad0[0xc];
    IDirect3D9* enum_pD3D;                                  // +0xc (D3DEnumeration::pD3D, set by init_d3d)
    carray*  modules;                                       // +0x10 (module array)
    void*    confirm_cb;                                    // +0x14 (0x401010 stored by init_d3d)
    char     pad18p[0x30 - 0x18];
    bool     show_fps;          // +0x30
    char     pad31[0x38 - 0x31];
    bool     fps_visible;       // +0x38
    char     pad39[0x68 - 0x39];
    int      f68;               // +0x68
    char     pad6c[0x74 - 0x6c];
    Module*  f74;               // +0x74
    MeshObject* f78;            // +0x78
    MeshTexture* f7c;           // +0x7c
    int      f80;               // +0x80 (width)
    int      f84;               // +0x84 (height)
    int      f88;               // +0x88 (refresh)
    int      f8c;               // +0x8c (format)
    int      f90;               // +0x90 (shadow quality)
    int      f94;               // +0x94 (display mode / antialiasing)
    int      f98;               // +0x98
    int      f9c;               // +0x9c
    int      fa0;               // +0xa0
    bool     crop;              // +0xa4 (screenshot crop flag)
    char     padA8[0xe8 - 0xa8];
    HWND     hwnd;              // +0xe8
    char     padEC[0xf4 - 0xec];
    IDirect3D9* d3d;            // +0xf4
    char     padF8[0x244 - 0xf8];
    int      client_w;          // +0x244
    int      client_h;          // +0x248
    char     pad24c[0x590 - 0x24c];
    D3DDISPLAYMODE mode;        // +0x590 (width, height, refresh, format)

    void update_view_rect();                                // 0x4bace0
    int  init_d3d();                                        // 0x4bad70
    bool choose_display_mode(bool kind1, bool kind2);       // 0x4badf0
    int  DisplayErrorMsg(int result, bool fatal);           // 0x4014f0 (Application::DisplayErrorMsg)
};
// @ 0x4bab70
// The probes only read the low dword of `name`, so the original keeps a 0x200-byte scratch buffer at
// [esp] and the tstring value object at [esp + 0x100].
static void user_path(char* out)
{
    char path[0x104];
    char name[0x104];

    SHGetFolderPathA(0, 0x1a, 0, 0, path);              // CSIDL_APPDATA
    sprintf(name, "%s\\Mount&Blade", path);
    CreateDirectoryA(name, 0);
    sprintf(name, "%s", out);
    sprintf(out, "%s\\Mount&Blade\\%s", path, name);
}

// @ 0x4babf0
static void screenshots_path(char* out)
{
    char path[0x104];
    char name[0x104];

    SHGetFolderPathA(0, 0x05, 0, 0, path);              // CSIDL_PERSONAL
    sprintf(name, "%s\\Mount&Blade Screenshots", path);
    CreateDirectoryA(name, 0);
    sprintf(name, "%s", out);
    sprintf(out, "%s\\Mount&Blade Screenshots\\%s", path, name);
}

// Keep the internal-linkage helpers above emitted (the originals are static and referenced by their callers).
void keep_paths(char* a, char* b) { user_path(a); screenshots_path(b); }

// @ 0x4bac70
// Decodes the "\n" and "\r" escapes of a config/translation line in place, shortening the string.
void __fastcall unescape(tstring* s)
{
    int dst = 0;
    int src = 0;
    while (dst < s->len - 1) {
        char c = s->buf[src];
        if (c == '\\' && s->buf[src + 1] == 'n') {
            s->buf[dst] = '\n';
            src++;
            s->len--;
        } else if (s->buf[src] == '\\' && s->buf[src + 1] == 'r') {
            s->buf[dst] = '\r';
            src++;
            s->len--;
        } else {
            s->buf[dst] = c;
        }
        dst++;
        src++;
    }
    s->buf[dst] = s->buf[src];
    s->buf[dst + 1] = 0;
}

// @ 0x4bace0
void Core::update_view_rect()
{
    RECT r;
    GetWindowRect(this->hwnd, &r);
    if (this->crop) {
        g_view_x = r.left + 4;
        g_view_y = (r.bottom - this->client_h) - 4;
    } else {
        g_view_x = 0;
        g_view_y = 0;
    }
    g_view_w = this->client_w;
    g_view_h = this->client_h;
}

// @ 0x4bad70
int Core::init_d3d()
{
    this->d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!this->d3d) {
        return this->DisplayErrorMsg(0x82000001, true);
    }
    this->enum_pD3D = this->d3d;
    this->confirm_cb = (void*)confirm_device_change;
    this->d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &this->mode);
    int hr = ((CD3DEnumeration*)((char*)this + 0xc))->Enumerate();
    if (hr < 0) {
        if (this->d3d) {
            this->d3d->Release();
            this->d3d = 0;
        }
        this->DisplayErrorMsg(hr, true);
    }
    return hr;
}

// @ 0x4badf0
// Chooses the start-up display mode: scans every module's mesh objects for a texture entry that fits the
// configured mode, then looks up the module's own entry for that mode and stores everything on the core object.
bool Core::choose_display_mode(bool kind1, bool kind2)
{
    int width = 0, height = 0, sel_w = 0, sel_h = 0, sel_f = 0;
    int a = 0, b = 0, d = 0, grip = 0, rank = 0;
    int format;
    Module* chosen_module = 0;
    MeshObject* chosen_object = 0;
    MeshTexture* chosen_texture = 0;
    if (g_perf_freq_lo > 0) {
        width = g_perf_freq_lo;
        format = (g_cfg_display_height == 0x10) + 0x16;   // D3DFMT_X8R8G8B8 (22) or R5G6B5 (23)
        height = g_cfg_display_width;
    } else {
        width = this->mode.Width;
        height = this->mode.Height;
        format = this->mode.Format;
    }
    sel_f = this->mode.RefreshRate;
    for (int m = 0; m < this->modules->count; m++) {
        Module* mod = (Module*)this->modules->at(m);
        for (int o = 0; o < mod->object_types.count; o++) {
            MeshObject* obj = (MeshObject*)mod->object_types.at(o);
            if ((kind1 && obj->kind != 1) || (kind2 && obj->kind != 2)) continue;
            for (int t = 0; t < obj->textures.count; t++) {
                MeshTexture* tex = (MeshTexture*)obj->textures.at(t);
                int tf = tex->f8;
                bool same_fc = tex->fc == tf;
                bool same_fmt = tf == format;
                if (tex->f10) continue;
                if ((!chosen_texture
                     || (chosen_texture->f4 != 1 && obj->kind == 1)
                     || (tex->f4 == 1
                         && ((chosen_texture->f8 != format && same_fmt) || (same_fmt && same_fc))))) {
                    sel_w = width;
                    chosen_texture = tex;
                    b = o;
                    chosen_module = mod;
                    chosen_object = obj;
                    sel_h = height;
                    sel_f = width;
                    if (obj->kind == 1 && same_fmt) {
                        if (same_fc) goto found;
                    }
                }
            }
        }
    }
found:
    if (!chosen_texture) return false;
    for (int i = 0; i < chosen_module->objects.count; i++) {
        MeshTexture* tex = (MeshTexture*)chosen_module->objects.at(i);
        if (tex->fc != chosen_texture->f8) continue;
        if (tex->f0 == sel_w) {
            if (tex->f4 == sel_h && tex->f8 == sel_f) {
                a = tex->f0;
                b = tex->f4;
                d = tex->f8;
                grip = tex->fc;
                break;
            }
        } else if (a != 0) {
            break;
        }
        a = tex->f0;
        b = tex->f4;
        d = tex->f8;
        grip = tex->fc;
    }
    this->f74 = chosen_module;
    this->f78 = chosen_object;
    this->f80 = a;
    this->f84 = b;
    this->f88 = d;
    this->f8c = grip;
        this->f7c = chosen_texture;
        this->fps_visible = 0;
        if (this->show_fps) {
        rank = *(int*)chosen_object->types.at(0);
        for (int i = 1; i < chosen_object->types.count; i++) {
            int t = *(int*)chosen_object->types.at(i);
            if (shader_rank(rank) > shader_rank(t)) rank = t;
        }
        this->f90 = rank;
    }
    if (g_cfg_display_bits == 2) {
        this->f94 = 2;
        this->f98 = g_cfg_antialiasing;
    } else if (g_cfg_display_bits == 4) {
        this->f94 = 4;
        this->f98 = g_cfg_antialiasing;
    } else {
        this->f94 = *(int*)chosen_texture->sub18.at(0);
        this->f98 = 0;
    }
    this->f9c = *(int*)chosen_texture->sub24.at(0);
    if (g_cfg_first_time) {        this->fa0 = 1;
        this->f68 = 1;
    } else {
        this->fa0 = 0x80000000;
        this->f68 = 0x80000000;
    }
    return true;
}

// @ 0x4bb260
// Moves the list box selection of the options page by `delta` entries (clamped) and remembers the new
// top-visible and selected entry in the game data object.
void set_list_scroll(int delta)
{
    HWND list = GetDlgItem(g_hWnd, 0x431);
    HWND combo = GetDlgItem(g_hWnd, 0x433);
    g_game->list_top = SendMessageA(list, 0x150, SendMessageA(list, 0x147, 0, 0), 0);   // LB_GETTOPINDEX, LB_GETTEXT?
    int cur = SendMessageA(combo, 0x188, 0, 0);             // CB_GETCOUNT
    int count = SendMessageA(combo, 0x18b, 0, 0);           // CB_GETCURSEL? (as coded)
    cur += delta;
    if (cur < 0) cur = 0;
    else if (cur > count) cur = count;
    SendMessageA(combo, 0x186, (WPARAM)cur, 0);             // CB_SETCURSEL
    g_game->list_sel = SendMessageA(combo, 0x199, SendMessageA(combo, 0x188, 0, 0), 0);
}

// @ 0x4bb310
// Pumps the message queue until WM_QUIT, translating accelerators is not done here.
void pump_messages()
{
    MSG msg;
    HACCEL accel = LoadAcceleratorsA(0, MAKEINTRESOURCEA(0x71));
    msg.message = 0;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if (accel) DestroyAcceleratorTable(accel);
}

// @ 0x4bb390
void set_debug_label()
{
    SetDlgItemTextA(g_hWnd, 0x4b3, "Debug");
    return;
}

// @ 0x4bb3b0
// Sets the two-dword progress range of the dialog's progress bar (PBM_SETRANGE32).
void set_progress_range(int low, int high)
{
    int range[2];
    range[0] = low;
    range[1] = high;
    SendMessageA(GetDlgItem(g_hWnd, 0x4b0), 0x437, 0, (LPARAM)range);
}
