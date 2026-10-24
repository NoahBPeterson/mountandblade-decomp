// config_dialog.cpp: the launcher configuration dialog (0x4cf100..0x4d0620): the display-mode list, the tab fill and the
// dialog procedure (rgl_config.txt). Function names FUN_* are the slices' provisional names.
// clang-format off
#include <windows.h>
#include <d3d9.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <new>
#include "../tstring.h"

// 0x43a1e0: string-table lookup on 0x8ee3c48 (key -> localized text; the empty case falls back to that address).
const char* strtab_lookup(tstring& key);

// 0x08ee3d3c: renderer display-mode enumerator. Verified call offsets: +0x10 adapter count,
// +0x14 adapter display mode, +0x18 mode count, +0x1c enumerate mode.
struct IModeEnum {
    virtual void v0();
    virtual int v1();
    virtual int v2();
    virtual int v3();
    virtual int count();                                  // +0x10
    virtual void display_mode(int adapter, int fmt, D3DDISPLAYMODE* mode);   // +0x14
    virtual int mode_count(int adapter, int fmt);                            // +0x18
    virtual void enum_mode(int adapter, int fmt, int index, D3DDISPLAYMODE* m); // +0x1c
};

// 0x08ee3d3c: renderer singleton pointer.
#define g_renderer (*(IModeEnum**)0x8ee3d3c)

// Temporary sorted result, copied into the return value at the end (as the original does).
struct ModeList {
    std::vector<tstring> names;
    ModeList() {}
    ModeList(const ModeList& o) : names(o.names) {}   // 0x4cc680 (copy with EH frame)
};

// @ 0x4cf100
std::vector<tstring> list_modes()
{
    std::vector<tstring> mode_names;
    std::vector<int> widths;
    std::vector<int> heights;
    std::vector<int> bpps;

    unsigned adapters = g_renderer->count();
    int adapter_geometry[32];
    for (unsigned a = 0; a < adapters; a++) {
        D3DDISPLAYMODE adapter_mode;
        g_renderer->display_mode(a, 0, &adapter_mode);
        adapter_geometry[a] = adapter_mode.Height;
        D3DFORMAT formats[2];
        formats[0] = D3DFMT_R5G6B5;      // 16-bit
        formats[1] = D3DFMT_X8R8G8B8;    // 32-bit
        for (int f = 0; f < 2; f++) {
            int modes = g_renderer->mode_count(a, formats[f]);
            for (int m = 0; m < modes; m++) {
                D3DDISPLAYMODE mode;
                g_renderer->enum_mode(a, formats[f], m, &mode);
                if (mode.Width < 640 || mode.Height < 480)
                    continue;
                char buf[128];
                tstring name(_itoa(mode.Width, buf, 10));
                name.push_back('x');
                name.append(tstring(_itoa(mode.Height, buf, 10)));
                name.append(tstring(f ? " 32 bits" : " 16 bits"));
                int bpp = f ? 32 : 16;
                bool dup = false;
                for (unsigned i = 0; i < widths.size(); i++) {
                    if (widths[i] == (int)mode.Width && heights[i] == (int)mode.Height && bpps[i] == bpp)
                        dup = true;
                }
                if (!dup) {
                    mode_names.push_back(name);
                    widths.push_back(mode.Width);
                    heights.push_back(mode.Height);
                    bpps.push_back(bpp);
                }
            }
        }
    }

    ModeList sorted;
    unsigned n = (unsigned)widths.size();
    int cur_w = 0, cur_h = 0, cur_bpp = 0;
    for (unsigned k = 0; k < n; k++) {
        int best = -1;
        int best_w = 1000000, best_h = 1000000, best_bpp = 1000000;
        for (unsigned i = 0; i < n; i++) {
            int w = widths[i], h = heights[i], bpp = bpps[i];
            if (w == cur_w) {
                if (h == cur_h) {
                    if (bpp > cur_bpp) {
                        best = i; best_w = w; best_h = h; best_bpp = bpp;
                    }
                } else if (h > cur_h) {
                    if (best_h == h) {
                        if (best_bpp >= bpp) {
                            best = i; best_w = w; best_h = h; best_bpp = bpp;
                        }
                    } else if (best_h > h) {
                        best = i; best_w = w; best_h = h; best_bpp = bpp;
                    }
                }
            } else if (w > cur_w) {
                if (best_w == w) {
                    if (best_h == h) {
                        if (best_bpp >= bpp) {
                            best = i; best_w = w; best_h = h; best_bpp = bpp;
                        }
                    } else if (best_h > h) {
                        best = i; best_w = w; best_h = h; best_bpp = bpp;
                    }
                } else if (best_w > w) {
                    best = i; best_w = w; best_h = h; best_bpp = bpp;
                }
            }
        }
        if (best >= 0)
            sorted.names.push_back(tstring(mode_names[best].buf));
        cur_w = best_w;
        cur_h = best_h;
        cur_bpp = best_bpp;
    }

    std::vector<tstring> result(sorted.names);
    return result;
}

void refresh_list(int v);      // 0x4c6f90
void set_active_tab(int v);      // 0x4b9790

const float g_width_scale = 0.0023752970155328512f;    // 0x59642c
const float g_height_scale = 0.0020833334419876337f;   // 0x596430

// skinmagic ordinal 9: skins a dialog window with a registered skin class name.
int __stdcall SkinDialog(HWND hwnd, const char* cls);    // SkinMagic ord9, thunk 0x535446

#include "../../include/crt.h"   // canonical: sprintf (0x536f8f)

void user_path(char* out);                       // 0x4bab70
bool mb_file_exists(const char* path);           // 0x415120
void* mb_open_user_file(const char* name, const char* mode);   // 0x415160
void load_file(void* file);                       // 0x4bbf70 (thiscall on &g_render_cfg)
void save(void* file);                       // 0x415fb0 (thiscall on &g_render_cfg)
void choose_dx_settings(int reset);                        // 0x4b9b90 (thiscall on &g_render_cfg)
void show_options(int dummy);                       // 0x4bee10 (fastcall on this)
void read_options();                                // 0x4bf170 (fastcall on this)

const char k_config[] = "rgl_config.txt";        // 0x54b048

class Dialog {
public:
    char pad[0x380];
    HWND m_hwnd;      // 0x380
    int m_ctrl;       // 0x384
    void fill_video_tab();                          // 0x4cf7b0
    LRESULT proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);   // 0x4d0240
};

// A table key whose object lives only for the full expression that uses it (the original's
// `tstring key("..."); strtab_lookup(key)` pairs are an inlined helper of exactly this shape).
struct key_text {
    char* text;
    key_text(const char* s) { tstring k(s); text = (char*)strtab_lookup(k); }
};

// @ 0x4cf7b0
void Dialog::fill_video_tab()
{
    int item[3];
    item[0] = 1;
    item[2] = -1;
    m_ctrl = 0;

    HWND tabs = GetDlgItem(m_hwnd, 0x43d);
    item[1] = (int)key_text("ui_video").text;
    SendMessageA(tabs, 0x1307, 0, (LPARAM)item);
    item[1] = (int)key_text("ui_audio").text;
    SendMessageA(tabs, 0x1307, 1, (LPARAM)item);
    item[1] = (int)key_text("ui_game").text;
    SendMessageA(tabs, 0x1307, 2, (LPARAM)item);
    item[1] = (int)key_text("ui_advanced").text;
    SendMessageA(tabs, 0x1307, 3, (LPARAM)item);

    RECT dlg;
    RECT rc;
    GetWindowRect(m_hwnd, &dlg);
    GetWindowRect(tabs, &rc);
    rc.top -= 0x3c;
    rc.left -= 10;
    int top = rc.top;
    int left = rc.left;
    int h = (int)((float)(rc.bottom - 0x3c) * g_height_scale) - top;
    int w = (int)((float)(rc.right - 0x3c) * g_width_scale) - rc.left;
    SetWindowPos(tabs, NULL, left - dlg.left, top - dlg.top, w, h, 0x14);

    HWND modes = GetDlgItem(m_hwnd, 0x45f);
    if (modes) {
        SendMessageA(modes, 0x14b, 0, 0);
        std::vector<tstring> list = list_modes();
        for (unsigned i = 0; i < list.size(); i++)
            SendMessageA(modes, 0x14a, i, (LPARAM)list[i].buf);
    }
    SendMessageA(modes, 0x14a, 0, (LPARAM)key_text("ui_use_desktop_resolution").text);

    HWND quality = GetDlgItem(m_hwnd, 0x467);
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)key_text("ui_m_ultra_high").text);
        SendMessageA(quality, 0x151, i, 2);
    }
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)key_text("ui_m_high").text);
        SendMessageA(quality, 0x151, i, 1);
    }
    {
        int i = SendMessageA(quality, 0x14a, 0, (LPARAM)key_text("ui_m_low").text);
        SendMessageA(quality, 0x151, i, 0);
    }
    {
        int i = SendMessageA(GetDlgItem(m_hwnd, 0x468), 0x14a, 0, (LPARAM)key_text("ui_directx_9").text);
        SendMessageA(GetDlgItem(m_hwnd, 0x468), 0x151, i, 1);
    }
    {
        int i = SendMessageA(GetDlgItem(m_hwnd, 0x468), 0x14a, 0, (LPARAM)key_text("ui_directx_7").text);
        SendMessageA(GetDlgItem(m_hwnd, 0x468), 0x151, i, 0);
    }

    refresh_list(1);

    {
        tstring caption(key_text("ui_configure_caption").text);
        char* p = caption.buf;
        if (!p)
            p = (char*)&caption;
        SendMessageA(m_hwnd, 0xc, 0, (LPARAM)p);
    }

    SetDlgItemTextA(m_hwnd, 0x43f, key_text("ui_texture_detail").text);
    SetDlgItemTextA(m_hwnd, 0x449, key_text("ui_max_frame_rate").text);
    SetDlgItemTextA(m_hwnd, 0x460, key_text("ui_screen_size").text);
    SetDlgItemTextA(m_hwnd, 0x462, key_text("ui_antialiasing").text);
    SetDlgItemTextA(m_hwnd, 0x492, key_text("ui_shadow_quality").text);
    SetDlgItemTextA(m_hwnd, 0x493, key_text("ui_enable_vertex_shaders").text);
    SetDlgItemTextA(m_hwnd, 0x44b, key_text("ui_create_brf").text);
    SetDlgItemTextA(m_hwnd, 0x443, key_text("ui_disable_blood").text);
    SetDlgItemTextA(m_hwnd, 0x446, key_text("ui_disable_music").text);
    SetDlgItemTextA(m_hwnd, 0x452, key_text("ui_disable_sound").text);
    SetDlgItemTextA(m_hwnd, 0x442, key_text("ui_enable_cheats").text);
    SetDlgItemTextA(m_hwnd, 0x44c, key_text("ui_enable_edit_mode").text);
    SetDlgItemTextA(m_hwnd, 0x44a, key_text("ui_force_single_threading").text);
    SetDlgItemTextA(m_hwnd, 0x441, key_text("ui_enable_pixel_shaders").text);
    SetDlgItemTextA(m_hwnd, 0x44d, key_text("ui_enable_sound_variation").text);
    SetDlgItemTextA(m_hwnd, 0x445, key_text("ui_show_framerate").text);
    SetDlgItemTextA(m_hwnd, 0x487, key_text("ui_force_vsync").text);
    SetDlgItemTextA(m_hwnd, 0x444, key_text("ui_start_windowed").text);
    SetDlgItemTextA(m_hwnd, 2, key_text("ui_cancel2").text);
    SetDlgItemTextA(m_hwnd, 1, key_text("ui_ok2").text);
    SetDlgItemTextA(m_hwnd, 0x45e, key_text("ui_auto_adjust").text);

    m_ctrl = SendMessageA(GetDlgItem(m_hwnd, 0x43e), 0x406, 0, 0x64000a);
    set_active_tab(m_ctrl);
}

// @ 0x4d0240
LRESULT Dialog::proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        m_hwnd = hwnd;
        SkinDialog(hwnd, "Dialog");
        char path[256];
        sprintf(path, "%s", k_config);
        user_path(path);
        if (mb_file_exists(path)) {
            void* f = mb_open_user_file(k_config, "r");
            load_file(f);
            fclose((FILE*)f);
        } else {
            *(int*)0x5aa0ac = 0x46;
        }
        choose_dx_settings(0);
        fill_video_tab();
        show_options(0);
        return 1;
    case 0x114: {
        int v = (int)SendMessageA(GetDlgItem(m_hwnd, 0x43e), 0x400, 0, 0);
        char text[256];
        sprintf(text, "%d", v);
        SetWindowTextA(GetDlgItem(m_hwnd, 0x450), text);
        return 1;
    }
    case WM_COMMAND:
        if (wParam != 0x43d) {
            DefWindowProcA(hwnd, msg, wParam, lParam);
            __asm pop edi
            __asm pop esi
            __asm add esp, 208h
            __asm ret 10h
        }
        set_active_tab((int)SendMessageA(GetDlgItem(m_hwnd, 0x43d), 0x130b, 0, 0));
        // falls through to the control dispatch: a tab notification also runs it
        switch (wParam & 0xffff) {
        case 1: {               // ok: write the config back
            read_options();
            void* f = mb_open_user_file(k_config, "w");
            save(f);
            fclose((FILE*)f);
            EndDialog(hwnd, 2);
            return 1;
        }
        case 2:                 // cancel
            EndDialog(hwnd, 2);
            return 1;
        case 0x448:             // refresh-rate edit: keep 10..999, else restore
            if ((wParam >> 16) == 0x200) {
                BOOL ok;
                UINT v = GetDlgItemInt(m_hwnd, 0x448, &ok, 0);
                if (ok && v >= 10 && v < 1000) {
                    *(int*)0x5aa0b4 = v;
                } else {
                    SetDlgItemInt(m_hwnd, 0x448, *(int*)0x5aa0b4, 0);
                }
            }
            return 1;
        case 0x45e:             // auto adjust: re-detect and re-read the config
            read_options();
            *(int*)0x5aa0a8 = 0;
            choose_dx_settings(1);
            show_options(0);
            return 1;
        case 0x45f:             // display-mode list changed
            if ((wParam >> 16) == 9) {
                refresh_list(0);
                return 1;
            }
            break;
        }
        return 1;
    }
    return 0;
}

// clang-format on
