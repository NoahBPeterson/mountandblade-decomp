// Slice s004bb3f0: the settings list refresh, the screenshot writer and the configuration file loader.
//   0x4bb3f0 refresh_settings_list, 0x4bb9c0 save_screenshot, 0x4bba90 split_line, 0x4bbb70 value_as_int,
//   0x4bbba0 Config::set, 0x4bbf70 Config::load_file. Class and field names are provisional; the configuration
// layout comes from the setters in slice s004bbba0 (0x4bc020..0x4bc170).
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>

struct tstring;

// The engine string (16 bytes), as in match/slices/s00463790/tstring.h.
struct tstring {
    int pos;                // +0  read cursor
    char* buf;              // +4  (null while empty)
    int cap;                // +8
    int len;                // +0xc
    tstring();                                              // 0x42bbd0
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
    char equals(const char* s) const;                       // 0x42bd10
    void clear();                                           // 0x42be50
    void push_back(char c);                                 // 0x42be70
    void to_lower();                                        // 0x42c020
    int find(const char& c, int start) const;               // 0x42c080
    void append(const tstring& o);                          // 0x42c1e0
    int find(const tstring& s, int start) const;            // 0x42c440
    tstring substr(int start, int end) const;               // 0x42c4a0
    void erase_range(int start, int end);                   // 0x42c560
    void insert(int at, const tstring& s);                  // 0x42c900
};

extern "C" int _atoi(const char* str);                      // 0x53734d (jmp _atol)

// ---- globals --------------------------------------------------------------------------------------------------
extern IDirect3DDevice9* g_d3d_device;                      // 0xa17f28
extern char g_screenshot_cropped;                           // 0x8ee3cec (non-zero: crop the window area)
extern int g_view_x;                                        // 0x5d75d4
extern int g_view_y;                                        // 0x5d75d0
extern int g_view_w;                                        // 0x5d75c8
extern int g_view_h;                                        // 0x5d75cc
extern HWND g_settings_dlg;                                 // 0x8ee3fc0  the settings dialog (list box id 0x4b0)
extern char* g_game;                                        // 0x5e2930 (settings text tstring at +0x1a080)
extern "C" const char g_eq_sep[];                           // 0x593160  " = "
extern int g_edit_key;                                      // 0x5e28d0  first value of the line being edited
extern int g_edit_value;                                    // 0x5e28d4  second value of the line being edited
extern float g_line_pitch_ratio;                            // 0x590d94  0.6f: visible lines vs window height

// A vtable-based object whose methods are issued from a flat pointer.
struct VtblObject;

// ---- configuration object (layout from 0x4bc020 and the setters) ----------------------------------------------
struct Config {
    int first_time;                 // +0x00
    int texture_detail;             // +0x04
    int show_framerate;             // +0x08
    int max_framerate;              // +0x0c
    int start_windowed;             // +0x10
    int skip_tutorial;              // +0x14
    int enable_edit_mode;           // +0x18
    int force_single_threading;     // +0x1c
    int debug_mode;                 // +0x20
    int cheat_mode;                 // +0x24
    int enable_blood;               // +0x28
    int fake_reflections;           // +0x2c
    int disable_frequency_variation;// +0x30
    int use_vertex_shaders;         // +0x34
    int use_pixel_shaders;          // +0x38
    int disable_music;              // +0x3c
    int disable_sound;              // +0x40
    int unk44;                      // +0x44
    int render_buffer_size;         // +0x48
    int display_width;              // +0x4c
    int display_height;             // +0x50
    int display_bits;               // +0x54
    int antialiasing;               // +0x58
    int sample_quality;             // +0x5c
    int force_vsync;                // +0x60
    int unk64;                      // +0x64
    int shadowmap_quality;          // +0x68
    int unk6c;                      // +0x6c
    void __fastcall set(tstring& key, tstring& value);
    void load_file(FILE* f);
};

// 0x4bbb70: the value of a "name = value" line, 1 when the value is empty.
int __stdcall value_as_int(const tstring& s);

// @ 0x4bb3f0
// Re-renders the settings page `page` in the list box and sends the review message after a settings change.
// The list box lines are built as `name = value,value`; a quoted part is shown in red (green once the value
// differs from the stored setting). Afterwards the list height is compared against the window height.
void __cdecl refresh_settings_list(const tstring* page, char detailed)
{
    char text[0x1000];
    RECT item;
    int color;
    int key_end;
    int lineheight;
    int item_rgb[3];
    RECT window;
    char quote;

    SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x434, 0, (LPARAM)&item);
    (*(tstring*)(g_game + 0x1a080)).erase_range(item.left, item.right);
    (*(tstring*)(g_game + 0x1a080)).insert(item.left, *page);

    tstring sel_text;
    SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x43e, 0, (LPARAM)text);
    {
        tstring line(text);
        sel_text.append(line);
    }

    item_rgb[0] = 2;
    SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x461, (WPARAM)item_rgb, (LPARAM)page->c_str());

    item.right = item.left + page->len;
    quote = '"';
    int first = page->find(quote, 0);
    int separated = 0;
    item_rgb[1] = 0x3c;
    item_rgb[2] = 0x40000000;
    color = detailed ? 0x00c80000 : 0;

    for (;;) {
        item.left = separated + item.right;
        item.top = first < 0 ? item.right : first + item.right;
        SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
        item_rgb[0] = color;
        SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x444, 1, (LPARAM)item_rgb);
        if (first < 0) {
            break;
        }
        separated = page->find(quote, first + 1) + 1;
        if (separated < 1) {
            first = -1;
        } else {
            item.left = item.right + first;
            item.top = item.right + separated;
            SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
            item_rgb[0] = (detailed ? 0x00c80000 : 0) + 0xc800;
            SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x444, 1, (LPARAM)item_rgb);
            first = page->find(quote, separated);
        }
    }

    if (!detailed) {
        first = page->find(tstring(g_eq_sep), 0);
        key_end = page->find(tstring(g_eq_sep), 0) + 3;
        while ((first = first + 3, first > 2)) {
            int k = page->find((const char)',', first);
            if (k < 0) {
                k = page->len;
            }
            if (key_end > 2) {
                int e = page->find((const char)',', key_end);
                if (e < 0) {
                    e = page->len;
                }
                long l = _atoi(page->substr(first, k).c_str());
                long r = _atoi(page->substr(key_end, e).c_str());
                if (l != r) {
                    goto mark;
                }
            } else {
            mark:
                item.left = item.right + first;
                item.top = item.right + k;
                SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
                item_rgb[0] = 0xff;
                SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x444, 1, (LPARAM)item_rgb);
            }
            first = page->find(tstring(g_eq_sep), first) + 3;
            key_end = page->find(tstring(g_eq_sep), key_end) + 3;
        }
    }

    WPARAM top = SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x436, 0, (LPARAM)item.left);
    GetWindowRect(GetDlgItem(g_settings_dlg, 0x4b0), &window);
    int count = window.bottom - window.top;
    lineheight = (top - 5) * 0x11;
    item_rgb[0] = 0;
    if ((float)count * g_line_pitch_ratio < (float)abs(lineheight - g_edit_value)) {
        SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x4de, 0, (LPARAM)item_rgb);
        g_edit_key = item_rgb[0];
        g_edit_value = lineheight;
    }

    SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
    item_rgb[2] = detailed != 0;
    item_rgb[0] = 0x3c;
    item_rgb[1] = 1;
    SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x444, 1, (LPARAM)item_rgb);

    SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
    item.right = item.left;
    SendMessageA(GetDlgItem(g_settings_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
    sel_text.~tstring();
}

// @ 0x4bb9c0
// Grabs a front-buffer copy into a system-memory surface and writes it as an image file, optionally cropped to
// the view rectangle. Returns false when the grab or the save failed.
bool __cdecl save_screenshot(const char* file_name, D3DXIMAGE_FILEFORMAT format)
{
    D3DDISPLAYMODE mode;
    RECT rect;
    IDirect3DSurface9* surface = 0;

    g_d3d_device->GetDisplayMode(0, &mode);
    g_d3d_device->CreateOffscreenPlainSurface(mode.Width, mode.Height, D3DFMT_A8R8G8B8,
                                              D3DPOOL_SYSTEMMEM, &surface, 0);
    bool ok = false;
    if (SUCCEEDED(g_d3d_device->GetFrontBufferData(0, surface))) {
        if (g_screenshot_cropped) {
            rect.left = g_view_x;
            rect.right = g_view_x + g_view_w;
            rect.top = g_view_y;
            rect.bottom = g_view_y + g_view_h;
            ok = SUCCEEDED(D3DXSaveSurfaceToFileA(file_name, format, surface, 0, &rect));
        } else {
            ok = SUCCEEDED(D3DXSaveSurfaceToFileA(file_name, format, surface, 0, 0));
        }
    }
    if (surface) {
        surface->Release();
    }
    return ok;
}

// @ 0x4bba90
// Splits a line into its runs of identifier characters: the leading run goes to `first`, the run after the first
// separator to `second`, anything later is dropped. The state machine walks 0 -> 1 (in a run) -> 2 (between
// runs) -> 3 (in the second run) -> 4 (after the second run).
void __stdcall split_line(const char* line, tstring* first, tstring* second)
{
    int i = 0;
    char ch;
    first->clear();
    second->clear();

    const char* p = line;
    for (;;) {
        ch = *p;
        p++;
        if (ch == 0) {
            break;
        }
    }
    int count = (int)(p - (line + 1));
    int state = 0;

    for (; i < count; i++) {
        ch = line[i];
        if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_') {
            if (state == 0) {
                state = 1;
                first->push_back(ch);
            } else if (state == 2) {
                state = 3;
                second->push_back(ch);
            } else if (state == 1) {
                first->push_back(ch);
            } else if (state == 3) {
                second->push_back(ch);
            }
        } else {
            if (state == 1) state = 2;
            if (state == 3) state = 4;
        }
    }
}

// @ 0x4bbb70
// The value of a "name = value" line, 1 when the value is empty.
int __stdcall value_as_int(const tstring& s)
{
    int r = 1;
    if (s.len != 0) {
        r = _atoi(s.c_str());
    }
    return r;
}

// @ 0x4bbba0
// Applies one "name = value" line. Both strings are lowered in place first.
void __fastcall Config::set(tstring& key, tstring& value)
{
    key.to_lower();
    value.to_lower();

    if (key.equals("start_windowed")) { start_windowed = value_as_int(value); return; }
    if (key.equals("skip_tutorial")) { skip_tutorial = value_as_int(value); return; }
    if (key.equals("enable_edit_mode")) { enable_edit_mode = value_as_int(value); return; }
    if (key.equals("force_single_threading")) { force_single_threading = value_as_int(value); return; }
    if (key.equals("debug_mode")) { debug_mode = value_as_int(value); return; }
    if (key.equals("cheat_mode")) { cheat_mode = value_as_int(value); return; }
    if (key.equals("enable_blood")) { enable_blood = value_as_int(value); return; }
    if (key.equals("disable_music")) { disable_music = value_as_int(value); return; }
    if (key.equals("disable_sound")) { disable_sound = value_as_int(value); return; }
    if (key.equals("show_framerate")) { show_framerate = value_as_int(value); return; }
    if (key.equals("max_framerate")) { max_framerate = value_as_int(value); return; }
    if (key.equals("use_pixel_shaders")) { use_pixel_shaders = value_as_int(value); return; }
    if (key.equals("use_vertex_shaders")) { use_vertex_shaders = value_as_int(value); return; }
    if (key.equals("fake_reflections")) { fake_reflections = value_as_int(value); return; }
    if (key.equals("disable_frequency_variation")) { disable_frequency_variation = value_as_int(value); return; }
    if (key.equals("texture_detail")) {
        int d = value_as_int(value);
        texture_detail = d;
        if (d < 0) { texture_detail = 0; return; }
        if (d > 100) { texture_detail = 100; return; }
        return;
    }
    if (key.equals("render_buffer_size")) { render_buffer_size = value_as_int(value); return; }
    if (key.equals("first_time")) { first_time = value_as_int(value); return; }
    if (key.equals("display_width")) { display_width = value_as_int(value); return; }
    if (key.equals("display_height")) { display_height = value_as_int(value); return; }
    if (key.equals("display_bits")) { display_bits = value_as_int(value); return; }
    if (key.equals("antialiasing")) { antialiasing = value_as_int(value); return; }
    if (key.equals("sample_quality")) { sample_quality = value_as_int(value); return; }
    if (key.equals("force_vsync")) { force_vsync = value_as_int(value); return; }
    if (key.equals("shadowmap_quality")) {
        int q = value_as_int(value);
        shadowmap_quality = q;
        unk6c = q;
    }
}

// @ 0x4bbf70
// Reads the file line by line; lines starting with '#' are comments. The line is read with getc into a
// 256-byte buffer (at most 250 chars), so each line terminates at '\n' or at end of file.
void Config::load_file(FILE* f)
{
    tstring key;
    tstring value;
    char line[0x100];

    while (!(f->_flag & _IOEOF)) {
        int n = 0;
        do {
            line[n] = (char)getc(f);
            if ((f->_flag & _IOEOF) || line[n] == '\n') break;
        } while (++n < 250);
        line[n + 1] = 0;
        if (line[0] != '#') {
            split_line(line, &key, &value);
            set(key, value);
        }
    }
}
